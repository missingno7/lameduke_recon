"""Correlate a 9.5b allocator trace with an independently bound DOS function."""
import argparse
from collections import Counter, defaultdict
from difflib import SequenceMatcher
import hashlib
from pathlib import Path
import sys

sys.dont_write_bytecode = True
from cgtrace95 import COMPILER_SHA256, dump_json, load_json, match, omf, worker_output
import semir
from capstone import x86 as X


def machine(side):
    decoded = []
    semir.build_cfg(side, decoded)
    return sorted(decoded, key=lambda ins: ins.address)


def relocation(side, ins, operand):
    if operand.type == X.X86_OP_MEM and ins.disp_size == 4:
        return side.ref(ins.address + ins.disp_offset)
    if operand.type == X.X86_OP_IMM and ins.imm_size == 4:
        return side.ref(ins.address + ins.imm_offset)
    return None


def token(side, ins):
    if ins.mnemonic in ("cwde", "cbw"):
        return "movsx", (("reg", 4), ("reg", 2)) if ins.mnemonic == "cwde" else (("reg", 2), ("reg", 1))
    fields = []
    for op in ins.operands:
        ref = relocation(side, ins, op)
        if op.type == X.X86_OP_REG:
            fields.append(("reg", op.size))
        elif op.type == X.X86_OP_MEM:
            stack = op.mem.base in (X.X86_REG_ESP, X.X86_REG_EBP) and op.mem.index == 0 and not ref
            fields.append(("mem", op.size, bool(op.mem.base), bool(op.mem.index),
                           op.mem.scale, op.mem.segment, "stack" if stack else ref or op.mem.disp))
        elif op.type == X.X86_OP_IMM:
            if ins.mnemonic == "call" or ins.mnemonic.startswith("j") or ins.mnemonic == "loop":
                target = side.rel_target(ins)
                fields.append(("target", target if target[0] in ("abs", "name") else "local"))
            else:
                fields.append(("imm", op.size, ref or op.imm))
    return ins.mnemonic, tuple(fields)


def physical(ins):
    """Explicit register operands and frame-relative displacements."""
    if ins.mnemonic == "cwde":
        return ["eax", "ax"]
    if ins.mnemonic == "cbw":
        return ["ax", "al"]
    result = []
    for op in ins.operands:
        if op.type == X.X86_OP_REG:
            result.append(ins.reg_name(op.reg))
        elif op.type == X.X86_OP_MEM:
            base, index = ins.reg_name(op.mem.base), ins.reg_name(op.mem.index)
            result.append((base, index, op.mem.disp if base in ("esp", "ebp") and not index else None))
        else:
            result.append(None)
    return result


def correspond(original, candidate):
    left, right = map(list, (original, candidate))
    pairs, gaps = {}, []
    matcher = SequenceMatcher(a=left, b=right, autojunk=False)
    for tag, a, b, c, d in matcher.get_opcodes():
        if tag == "equal":
            pairs.update(zip(range(c, d), range(a, b)))
        else:
            gaps.append((a, b, c, d))
    # Pair a displaced operation only where both unmatched streams contain
    # exactly one copy in a small neighborhood of the same anchor.
    taken = set(pairs.values())
    for a, b, c, d in gaps:
        for index in range(c, d):
            key = right[index]
            if right[max(0, c - 6):d + 6].count(key) != 1:
                continue
            candidates = [j for j in range(max(0, a - 6), min(len(left), b + 6))
                          if j not in taken and left[j] == key]
            if len(candidates) == 1:
                pairs[index] = candidates[0]
                taken.add(candidates[0])
    return pairs, gaps


def name_of(temp):
    return temp.get("symbol") or f"t{temp.get('id', '?')}"


def stack_disp(ins):
    """The observed ESP slot of a direct stack operand."""
    for op in ins.operands:
        if op.type == X.X86_OP_MEM and op.mem.base == X.X86_REG_ESP and not op.mem.index:
            return op.mem.disp
    return None


def entry_frame(insns, before, frame_size):
    """Recognize the fixed ESP prologue up to the first disputed access."""
    seen = False
    for ins in insns[:before]:
        if ins.mnemonic == "sub" and physical(ins)[0] == "esp" and ins.operands[1].type == X.X86_OP_IMM:
            if seen or ins.operands[1].imm != frame_size:
                return False
            seen = True
        elif seen and (ins.mnemonic in ("push", "pop", "call", "enter", "leave")
                       or X.X86_REG_ESP in ins.regs_access()[1]):
            return False
    return seen


def home_explanation(events, candidate, oracle, pairs, index):
    frames = [row for row in events if row["event"] == "frame"]
    if len(frames) != 1:
        return None
    frame = frames[0]
    c, o = candidate[index], oracle[pairs[index]]
    cdisp, odisp = stack_disp(c), stack_disp(o)
    if cdisp is None or odisp is None or cdisp == odisp:
        return None
    size = frame["local_bytes"]
    if not entry_frame(candidate, index, size) or not entry_frame(oracle, pairs[index], size):
        return None
    temps = defaultdict(dict)
    for temp in frame["temps"]:
        if temp.get("home") is not None and temp["alias_offset"] == 0:
            temps[temp["home"] + size][temp["address"]] = temp
    ctemps, otemps = list(temps[cdisp].values()), list(temps[odisp].values())
    if len(ctemps) != 1:
        return None
    ctemp = ctemps[0]
    otemp = otemps[0] if len(otemps) == 1 else None
    order = [row for row in events if row.get("kind") == "home"][-1]
    records = {temp["address"]: temp for temp in order["input"]}
    same_priority = bool(otemp and any({ctemp["address"], otemp["address"]} <= set(group)
                                       for group in order["same_priority_groups"]))
    evidence = {"temp": name_of(ctemp), "candidate_displacement": cdisp,
                "oracle_displacement": odisp, "candidate_slot_holder":
                    name_of(otemp) if otemp else None, "same_home_priority": same_priority}
    if ctemp["address"] in records:
        evidence["candidate_input_rank"] = list(records).index(ctemp["address"])
        evidence["candidate_home_rank"] = order["output_order"].index(ctemp["address"])
    if otemp and otemp["address"] in records:
        evidence["other_input_rank"] = list(records).index(otemp["address"])
        evidence["other_home_rank"] = order["output_order"].index(otemp["address"])
    if same_priority and otemp:
        evidence["lever"] = (f"Test source forms that change the relative backend order or lifetime of "
                             f"{name_of(otemp)} and {name_of(ctemp)}. Recheck the two home slots and all fixups.")
    else:
        evidence["lever"] = f"Inspect where {name_of(ctemp)} becomes stack-resident and how long it stays live."
    return {"mechanism": "stack home ordering", "evidence": evidence}


def ir_memory(operand, binds):
    if not operand:
        return None
    base, offset = operand, operand.get("offset", 0)
    if operand["class"] == 4:
        base = operand.get("base")
        offset += base.get("offset", 0) if base else 0
    if not base or base["class"] != 1 or not base.get("symbol"):
        return None
    name = base["symbol"]
    for spelling in ("_" + name, name, name + "_"):
        address, _ = binds.lookup(spelling)
        if address:
            return ("abs", address[0], address[1] + offset)
    return None


def parameter_registers(events):
    registers = {}
    for row in events:
        inst = row.get("first_instruction", {})
        if inst.get("position", [None])[0] != 0 or inst.get("opcode") != 0x26:
            continue
        result = inst["result"]
        operands = inst["operands"]
        if result and result.get("symbol") and len(operands) == 1 and operands[0]:
            reg = operands[0].get("reg")
            if reg:
                registers[result["handle"]] = reg
    return registers


def defining_choices(events, opcodes, chosen):
    for row in events:
        inst = row.get("first_instruction")
        if row["event"] != "colour" or row["chosen"] != chosen or not inst or inst["opcode"] not in opcodes:
            continue
        result = inst["result"]
        if result and result["address"] == row["temp"]["address"] and len(inst["operands"]) == 1:
            yield row, inst["operands"][0]


def describe_choice(events, row, desired_reg, origin=None):
    cr = row["chosen"]
    actual = next((choice for choice in row["choices"] if choice["register"] == cr), None)
    desired = next((choice for choice in row["choices"] if choice["register"] == desired_reg), None)
    temp = name_of(row["temp"]) if not origin else f"{name_of(row['temp'])} ({origin})"
    if desired is None:
        mechanism = "register class or fixed constraint"
        lever = f"Check the width and addressing form of {temp}'s defining expression."
    elif desired.get("rejected") == "live-register interference":
        mechanism = "register interference" if cr else "spill under register interference"
        lever = f"Test the definition and last-use order around {temp}, especially values of equal conflict savings."
    elif actual and "score" in desired and desired["score"] == actual.get("score"):
        mechanism = "equal-score colour choice"
        lever = (f"Test first-definition/use order around {temp} or shapes that make a physical register "
                 "already used when the allocator reaches this conflict.")
    elif "score" in desired:
        mechanism = "colour score"
        lever = f"Compare copies and constrained operands within {temp}'s conflict range."
    else:
        mechanism = "colour eligibility"
        lever = f"Check constrained aliases and uses within {temp}'s conflict range."
    evidence = {"temp": temp, "candidate_register": cr, "oracle_register": desired_reg,
                "backend_range": [row["conflict"]["first_ir"], row["conflict"]["last_ir"]],
                "savings": row["conflict"]["savings"], "chosen_option": actual,
                "oracle_option_in_candidate": desired, "lever": lever}
    first = row["conflict"]["first_ir"]
    last = row["conflict"]["last_ir"]
    if desired and desired.get("rejected") and first and last and first[0] == last[0]:
        overlaps = []
        for earlier in events:
            if earlier is row:
                break
            if earlier["event"] != "colour" or not earlier["chosen_mask"] & desired["register_mask"]:
                continue
            start, end = earlier["conflict"]["first_ir"], earlier["conflict"]["last_ir"]
            if start and end and start[0] == end[0] == first[0] and start[1] <= last[1] and end[1] >= first[1]:
                overlaps.append(name_of(earlier["temp"]))
        if overlaps:
            evidence["earlier_overlapping_choices"] = overlaps
    for event in events:
        if event.get("kind") != "conflict":
            continue
        records = {n["address"]: n for n in event["input"]}
        conflict = row["conflict"]["address"]
        if conflict in records and records[conflict]["name"]["id"] == row["conflict"]["name"]["id"]:
            rank = event["output_order"].index(conflict)
            evidence["conflict_rank"] = rank
            group = next((g for g in event["same_priority_groups"] if conflict in g), [])
            evidence["same_savings_order"] = [name_of(records[a]["name"]) for a in group]
    return {"mechanism": mechanism, "evidence": evidence}


def colour_explanation(events, cs, binds, c, o):
    if c.mnemonic not in ("mov", "movsx", "movzx", "cwde", "cbw"):
        return None
    cregs, oregs = physical(c), physical(o)
    if len(cregs) != 2 or not isinstance(cregs[0], str) or cregs[0] == oregs[0]:
        return None
    if isinstance(cregs[1], str) and c.mnemonic != "mov" and cregs[1] == oregs[1]:
        parameters = parameter_registers(events)
        possible = [(row, operand.get("symbol")) for row, operand in defining_choices(events, (0x24,), cregs[0])
                    if operand and parameters.get(operand.get("handle")) == cregs[1]]
    elif len(c.operands) == 2 and c.operands[1].type == X.X86_OP_MEM:
        ref = relocation(cs, c, c.operands[1])
        if not ref or ref[0] != "abs":
            return None
        possible = [(row, None) for row, operand in defining_choices(events, (0x24, 0x26), cregs[0])
                    if ir_memory(operand, binds) == ref]
    else:
        return None
    if len(possible) == 1:
        (row, origin), = possible
        return describe_choice(events, row, oregs[0], origin)
    return None


def spill_extension(events, cs, os, bins, oins, i, binds):
    """Find MOV byte / AND 255 / store versus a MOVZX of the same field."""
    chunk = bins[i:i + 3]
    if len(chunk) != 3 or [ins.mnemonic for ins in chunk] != ["mov", "and", "mov"]:
        return None
    move, mask, store = chunk
    if move.size + move.address != mask.address or mask.size + mask.address != store.address:
        return None
    dest, source = move.operands
    if dest.type != X.X86_OP_REG or dest.size != 1 or source.type != X.X86_OP_MEM or source.size != 1:
        return None
    base = semir.PART.get(move.reg_name(dest.reg))
    if not base or base[1] != 0 or mask.operands[1].type != X.X86_OP_IMM or mask.operands[1].imm != 255:
        return None
    if physical(mask)[0] != base[0] or physical(store)[1] != base[0]:
        return None
    slot = stack_disp(store)
    ref = relocation(cs, move, source)
    if slot is None or not ref or ref[0] != "abs":
        return None
    targets = [ins for ins in oins if abs((ins.address - os.lo) - (move.address - cs.lo)) <= 24
               and ins.mnemonic == "movzx" and ins.operands[1].type == X.X86_OP_MEM
               and ins.operands[1].size == 1 and relocation(os, ins, ins.operands[1]) == ref]
    frames = [frame for frame in events if frame["event"] == "frame"]
    if len(targets) != 1 or len(frames) != 1 or not entry_frame(bins, i, frames[0]["local_bytes"]):
        return None
    frame = frames[0]
    temps = {temp["id"] for temp in frame["temps"] if temp.get("home") is not None
             and temp["home"] + frame["local_bytes"] == slot and temp["alias_offset"] == 0}
    rows = [row for row, operand in defining_choices(events, (0x24,), None)
            if row["temp"]["id"] in temps and ir_memory(operand, binds) == ref]
    if len(rows) != 1:
        return None
    original, = targets
    row, = rows
    operand, = row["first_instruction"]["operands"]
    base = operand.get("base") or operand
    origin = (f"{base['symbol']}+{operand['offset']:#x}" if base.get("symbol") else None)
    diagnosis = describe_choice(events, row, original.reg_name(original.operands[0].reg), origin)
    diagnosis["evidence"]["candidate_displacement"] = slot
    return original, diagnosis


def compare_traces(before, after):
    def named_choices(trace):
        counts = Counter()
        rows = {}
        for event in trace["events"]:
            if event["event"] != "colour":
                continue
            name = event["temp"].get("symbol")
            if name:
                counts[name] += 1
                rows[name, counts[name]] = event
        return rows

    old, new = named_choices(before), named_choices(after)
    changes = [{"name": name, "occurrence": ordinal, "before": row["chosen"],
                "after": new[name, ordinal]["chosen"]}
               for (name, ordinal), row in old.items()
               if (name, ordinal) in new and row["chosen"] != new[name, ordinal]["chosen"]]

    def named_temps(trace):
        return {n["symbol"] for r in trace["events"] if r.get("kind") == "candidate"
                for n in r["input"] if n.get("symbol")}

    def homes(trace):
        return {n["symbol"]: n["home"] for r in trace["events"] if r["event"] == "frame"
                for n in r["temps"] if n.get("symbol") and n.get("home") is not None}

    before_names, after_names = named_temps(before), named_temps(after)
    before_homes, after_homes = homes(before), homes(after)
    def phase_counts(trace):
        return dict(Counter((row.get("kind") or row["event"]) for row in trace["events"]))

    return {"colour_changes": changes, "added_named_temps": sorted(after_names - before_names),
            "removed_named_temps": sorted(before_names - after_names),
            "phase_counts_before": phase_counts(before), "phase_counts_after": phase_counts(after),
            "home_changes": [{"name": name, "before": before_homes.get(name), "after": after_homes.get(name)}
                             for name in sorted(before_homes.keys() | after_homes.keys())
                             if before_homes.get(name) != after_homes.get(name)]}


def machine_site(candidate, original, base):
    return {"candidate_offset": candidate.address - base, "oracle_address": f"1:{original.address:05x}",
            "candidate_instruction": f"{candidate.mnemonic} {candidate.op_str}".rstrip(),
            "oracle_instruction": f"{original.mnemonic} {original.op_str}".rstrip()}


def swapped_predicate(candidate, oracle, candidate_index, oracle_index):
    c = candidate[candidate_index:candidate_index + 2]
    o = oracle[oracle_index:oracle_index + 2]
    flipped = dict(setle="setge", setge="setle", setl="setg", setg="setl",
                   setb="seta", seta="setb", setbe="setae", setae="setbe")
    if len(c) != 2 or len(o) != 2 or c[0].mnemonic != "cmp" or o[0].mnemonic != "cmp":
        return None
    if c[0].address + c[0].size != c[1].address or o[0].address + o[0].size != o[1].address:
        return None
    operands = physical(c[0])
    if (not all(isinstance(reg, str) for reg in operands) or len(set(operands)) != 2
            or operands[::-1] != physical(o[0]) or flipped.get(c[1].mnemonic) != o[1].mnemonic
            or physical(c[1]) != physical(o[1])):
        return None
    return {"mechanism": "predicate operand order",
            "evidence": {"candidate_condition": c[1].mnemonic, "oracle_condition": o[1].mnemonic,
                         "lever": "Try naming the comparison inputs and comparing them in oracle operand order."}}


def analyse(trace_path, overlay=None):
    trace = load_json(trace_path)
    obj = trace_path.with_suffix(".obj")
    source = Path(trace["source"])
    if trace["format"] != 1 or trace["compiler_sha256"] != COMPILER_SHA256:
        raise ValueError("unsupported trace version or compiler")
    if hashlib.sha256(obj.read_bytes()).hexdigest() != trace["object_sha256"]:
        raise ValueError("object hash differs from the trace")
    if hashlib.sha256(source.read_bytes()).hexdigest() != trace["source_sha256"]:
        raise ValueError("source hash differs from the trace")
    if trace["dos_code_equal"] is not True:
        raise ValueError("oracle comparison requires a trace checked against the DOS host")
    function = trace["function"]
    orc = semir.Oracle(overlay)
    mod, = omf.load(obj)
    segments = []
    previous = match.Bindings.extra
    try:
        match.Bindings.extra = overlay
        rows = match.verify_object(orc.img, mod, {function}, segments)
    finally:
        match.Bindings.extra = previous
    if len(rows) != 1 or not rows[0].get("oracle"):
        raise ValueError(f"expected one independently bound public for {function}")
    row, = rows
    bases = semir.segment_bases(orc, mod, segments)
    classification = semir.inspect_function(orc, mod, row, trace["profile"], bases,
                                            source.read_text(encoding="latin1"), semir.CalleeModel(orc))
    result = {"function": function, "classification": classification.get("class"),
              "verifier": row["status"], "byte_diffs": row.get("byte_diffs"),
              "first_diff": row.get("first_diff"), "oracle": row["oracle"],
              "verified_bytes": row.get("verified_bytes", 0)}
    if row["status"] == "STRICT":
        return result
    obj_no, address = semir.parse_addr(row["oracle"])
    if obj_no != 1:
        raise ValueError("expected an LE object 1 function")
    oside, cside = semir.OracleSide(orc, address), semir.CandSide(orc, mod, row, bases)
    oracle, candidate = machine(oside), machine(cside)
    ot = [token(oside, ins) for ins in oracle]
    ct = [token(cside, ins) for ins in candidate]
    pairs, gaps = correspond(ot, ct)
    result["alignment"] = {"pairs": len(pairs), "candidate_count": len(ct), "oracle_count": len(ot),
                           "sequence_gaps": len(gaps)}
    result["semantic_difference"] = classification.get("first_difference")
    earliest = None
    high = -1
    for index, ins in enumerate(candidate):
        if classification.get("class") == "ALLOCATION-ONLY" and ins.mnemonic == "mov" and index not in pairs:
            spill = spill_extension(trace["events"], cside, oside, candidate, oracle, index, orc.binds)
            if spill:
                other, mechanism = spill
                site = machine_site(ins, other, cside.lo)
                earliest = earliest or site
                result["first_attributed"] = {**site, **mechanism}
                break
        if index not in pairs:
            continue
        other_index = pairs[index]
        other = oracle[other_index]
        moved = other_index < high or (index + 1 in pairs and other_index > pairs[index + 1])
        high = max(high, other_index)
        if physical(ins) == physical(other) and not moved:
            continue
        site = machine_site(ins, other, cside.lo)
        if earliest is None:
            earliest = site
        evidence = (home_explanation(trace["events"], candidate, oracle, pairs, index)
                    or colour_explanation(trace["events"], cside, orc.binds, ins, other)
                    or swapped_predicate(candidate, oracle, index, other_index))
        if evidence:
            result["first_attributed"] = {**site, **evidence}
            break
        if moved and len(pairs) == len(ct) == len(ot):
            result["first_attributed"] = {**site, "mechanism": "instruction order",
                                          "evidence": {"lever": "Inspect definition and operand evaluation order "
                                                               "around these independently aligned instructions."}}
            break
    result["first_aligned_difference"] = earliest
    if not result.get("first_attributed"):
        result["limitation"] = "The observed assembly difference has no unique traced decision anchor."
    return result


def render(result):
    print(f"{result['function']}: {result['classification']} / {result['verifier']}, "
          f"{result['byte_diffs']} differing bytes")
    if result["first_diff"] is not None:
        print(f"  Verifier begins at +{result['first_diff']:#x}.")
    earliest = result.get("first_aligned_difference")
    if earliest:
        print(f"  First aligned register, stack or order difference at \x2b{earliest['candidate_offset']:x}: "
              f"{earliest['candidate_instruction']} | {earliest['oracle_address']} {earliest['oracle_instruction']}")
    attributed = result.get("first_attributed")
    if attributed:
        print(f"  {attributed['mechanism']} at {attributed['oracle_address']}")
        evidence = attributed["evidence"]
        details = []
        for key, value in evidence.items():
            if key == "lever":
                continue
            if isinstance(value, list) and len(value) > 7:
                value = f"{len(value)} records (full order in JSON)"
            details.append(f"{key}={value}")
        if details:
            print("  " + ", ".join(details))
        print("  " + evidence["lever"])
    if result.get("limitation"):
        print("  " + result["limitation"])
    if "control" in result:
        control = result["control"]
        print(f"  Paired control is {result['control_verifier']} with {len(control['colour_changes'])} "
              f"named colour changes and {len(control['home_changes'])} named home changes.")
        for key in ("added_named_temps", "removed_named_temps"):
            if control[key]:
                print(f"  {key.replace('_', ' ')}: {', '.join(control[key])}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace", type=Path)
    parser.add_argument("--symbols", help="existing match.py symbol overlay")
    parser.add_argument("--reference", type=Path, help="optional trace of a second source spelling")
    parser.add_argument("--out", type=Path)
    args = parser.parse_args()
    try:
        result = analyse(args.trace, args.symbols)
        if args.reference:
            comparison = analyse(args.reference, args.symbols)
            before, after = load_json(args.trace), load_json(args.reference)
            if before["function"] != after["function"] or before["flags"] != after["flags"]:
                raise ValueError("paired traces require the same function and compiler flags")
            result["control"] = compare_traces(before, after)
            result["control_verifier"] = comparison["verifier"]
        if args.out:
            dump_json(worker_output(args.out), result)
        render(result)
    except (RuntimeError, ValueError, OSError, KeyError) as exc:
        parser.exit(1, f"{exc}\n")


if __name__ == "__main__":
    main()
