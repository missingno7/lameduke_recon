"""Rank compiled donor functions against the pinned LameDuke LE oracle.

Run build_corpus.py first. Run xscan.py from any directory to write donors.json
and DONORS.md under build/workers/xscan. The ranking is a source-search aid.
"""
from bisect import bisect_right
from collections import Counter, defaultdict
from dataclasses import dataclass
from datetime import datetime, timezone
import argparse
import hashlib
import json
import math
from pathlib import Path
import sys

sys.dont_write_bytecode = True
from build_corpus import ROOT, WORK
sys.path.insert(0, str(ROOT / "tools"))
import capstone as cs
from capstone import x86 as x86
from common import dump_json, load_json
from image import Image, fmt_addr, parse_addr
from match import demangle, verify_object
from objcmp import norm_fixups
import omf

MD = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_32)
MD.detail = True
MASK = 0xffffffff
WEIGHTS = {"mnemonics": .40, "operands": .27, "calls": .13, "constants": .13, "size": .07}
GAME_UNITS = {
    "game_game": (0x10, 0xf160),
    "game_premap": (0xf160, 0x14fb0),
    "game_sector": (0x14fb0, 0x1f280),
    "game_player": (0x1f280, 0x25160),
    "game_actors": (0x25160, 0x2ba30),
    "game_sounds": (0x2ba30, 0x2c1c0),
    "game_menues": (0x2c1c0, 0x2e700),
    "game_gamedef": (0x2e700, 0x33e5c),
}


def normalized(name):
    return demangle(name).lower()


def dispatch_group(ins, group):
    return ins.group(group) or group == cs.CS_GRP_JUMP and ins.mnemonic.startswith("loop")


def operand_shape(ins, operand, relative, relocated_imm, relocated_disp):
    if operand.type == x86.X86_OP_REG:
        reg = ins.reg_name(operand.reg)
        if reg in {"esp", "ebp", "sp", "bp"}:
            return reg
        return ("reg", operand.size)
    if operand.type == x86.X86_OP_IMM:
        if relative:
            return "branch"
        return "address" if relocated_imm else ("imm", operand.size)
    if operand.type == x86.X86_OP_MEM:
        mem = operand.mem
        base = ins.reg_name(mem.base) if mem.base else ""
        if base not in {"esp", "ebp", "sp", "bp"}:
            base = bool(base)
        displacement = "address" if relocated_disp else bool(mem.disp)
        return ("mem", operand.size, base, mem.scale if mem.index else 0, displacement,
                ins.reg_name(mem.segment) if mem.segment else "")
    return ("other", operand.size)


def shingles(tokens):
    # Distinct lengths also provide useful scores for tiny functions.
    return Counter((*zip(tokens, tokens[1:]), *zip(tokens, tokens[1:], tokens[2:])) or tokens)


@dataclass
class Signature:
    size: int
    reached: int
    instructions: int
    mnemonics: Counter
    operands: Counter
    calls: Counter
    constants: Counter
    unknown_calls: int
    fingerprint: bytes
    unresolved_jumps: int


@dataclass
class Code:
    data: bytes
    # The values are (address within this segment, known callee names).
    # A relocation to a different segment without a symbol has (None, ()).
    relocations: dict
    names: dict

    def signature(self, lo, hi):
        seen = {}
        occupied = set()
        pending = [lo]
        unresolved = 0
        while pending:
            address = pending.pop()
            while lo <= address < hi and address not in occupied:
                ins = next(MD.disasm(self.data[address:min(hi, address + 15)], address, count=1), None)
                if ins is None or any(p in occupied for p in range(address, address + ins.size)):
                    break
                seen[address] = ins
                occupied.update(range(address, address + ins.size))
                ops = ins.operands
                if dispatch_group(ins, cs.CS_GRP_JUMP) and ops:
                    if ops[0].type == x86.X86_OP_IMM:
                        target, _ = self.direct(ins)
                        if target is not None and lo <= target < hi:
                            pending.append(target)
                    elif ins.mnemonic == "jmp":
                        targets = self.switch(ins, lo, hi)
                        unresolved += not targets
                        pending.extend(reversed(targets))
                    if ins.mnemonic in {"jmp", "ljmp"}:
                        break
                if ins.group(cs.CS_GRP_RET) or ins.group(cs.CS_GRP_IRET) or ins.mnemonic == "hlt":
                    break
                address += ins.size

        mnemonics, shapes = [], []
        constants, calls = Counter(), Counter()
        unknown_calls = 0
        fingerprint = hashlib.sha256()
        for address, ins in sorted(seen.items()):
            jump = dispatch_group(ins, cs.CS_GRP_JUMP)
            call = ins.group(cs.CS_GRP_CALL)
            ops = ins.operands
            relocated_imm = self.has_relocation(address, ins.imm_offset, ins.imm_size)
            relocated_disp = self.has_relocation(address, ins.disp_offset, ins.disp_size)
            if ops and (call or jump and ins.mnemonic == "jmp") and ops[0].type == x86.X86_OP_IMM:
                target, names = self.direct(ins)
                if call or target is None or not lo <= target < hi:
                    if names:
                        # One call is one event, even when a binding has aliases.
                        calls[names[0]] += 1
                    else:
                        unknown_calls += 1
            for op in ops:
                if op.type == x86.X86_OP_IMM and not (jump or call or relocated_imm):
                    value = op.imm & MASK
                    constants[value if value < 0x80000000 else value - 0x100000000] += 1
            if ins.mnemonic == "nop":
                continue
            mnemonics.append(ins.mnemonic)
            shapes.append((ins.mnemonic, tuple(
                operand_shape(ins, op, jump or call, relocated_imm, relocated_disp) for op in ops)))
            fingerprint.update(repr(shapes[-1]).encode("ascii"))
            fingerprint.update(b"\0")
        return Signature(hi - lo, len(occupied), len(mnemonics), shingles(mnemonics), shingles(shapes),
                         calls, constants, unknown_calls, fingerprint.digest(), unresolved)

    def has_relocation(self, address, offset, width):
        return offset and any(address + p in self.relocations for p in range(offset, offset + width))

    def direct(self, ins):
        ref = self.relocations.get(ins.address + ins.imm_offset) if ins.imm_offset else None
        if ref is not None:
            return ref
        target = ins.operands[0].imm & MASK
        return target, self.names.get(target, ())

    def switch(self, ins, lo, hi):
        operand = ins.operands[0]
        if (operand.type != x86.X86_OP_MEM or operand.size != 4 or not ins.disp_offset
                or not (operand.mem.base or operand.mem.index)):
            return []
        table, _ = self.relocations.get(ins.address + ins.disp_offset, (None, ()))
        if table is None:
            return []
        targets = []
        for offset in range(table, table + 4 * 4096, 4):
            target, _ = self.relocations.get(offset, (None, ()))
            if target is None:
                break
            if lo <= target < hi:
                targets.append(target)
        return targets


@dataclass
class Donor:
    file: str
    name: str
    profile: str
    host: str
    build_id: str
    segment: str
    start: int
    local: bool
    signature: Signature


def segment_code(mod, sid, names, aliases):
    seg = mod.segments[sid]
    relocations = {}
    for fixup in mod.fixups:
        if fixup.seg != sid:
            continue
        method, dest = fixup.target
        method &= 3
        if method == 0 and dest == sid:
            addend = int.from_bytes(seg.data[fixup.off:fixup.off + fixup.size()], "little")
            target = (fixup.disp + addend) & MASK
            relocations[fixup.off] = target, names.get(target, ())
        elif method == 2:
            name = normalized(mod.externs[dest])
            relocations[fixup.off] = None, (aliases.get(name, name),)
        else:
            relocations[fixup.off] = None, ()
    return Code(bytes(seg.data), relocations, names)


def object_functions(path, file, profile, host, build_id, aliases):
    for mod in omf.load(path):
        for sid, seg in enumerate(mod.segments):
            if not seg or seg.cls.upper() != "CODE" or not seg.use32:
                continue
            publics = defaultdict(list)
            for raw, si, offset, local in mod.publics:
                if si == sid and offset < seg.size:
                    publics[offset].append((demangle(raw), local))
            names = {start: tuple(sorted({aliases.get(n.lower(), n.lower()) for n, _ in entries}))
                     for start, entries in publics.items()}
            code = segment_code(mod, sid, names, aliases)
            boundaries = sorted(publics)
            for start, end in zip(boundaries, boundaries[1:] + [seg.size]):
                sig = code.signature(start, end)
                if sig.instructions:
                    for name, local in publics[start]:
                        yield Donor(file, name, profile, host, build_id, seg.name, start, local, sig)


def call_names(canonical, libraries, runtime):
    labels = defaultdict(list)
    for symbols in (canonical, libraries, runtime):
        for name, ent in symbols.items():
            obj, start = parse_addr(ent["addr"])
            if obj == 1 and not name.endswith(("_text_end", "_table")):
                labels[start].append(normalized(name))
    names = {offset: (spellings[0],) for offset, spellings in labels.items()}
    by_spelling = defaultdict(set)
    for start, spellings in labels.items():
        for name in spellings:
            by_spelling[name].add(names[start][0])
    aliases = {name: next(iter(spellings)) for name, spellings in by_spelling.items()
               if len(spellings) == 1}
    return names, aliases


def target_functions(image, units, verification, names, canonical, census):
    refs = {}
    for (obj, site), (_, target_obj, offset) in image.fixups.items():
        if obj == 1:
            refs[site] = (offset, names.get(offset, ())) if target_obj == 1 else (None, ())
    code = Code(image.obj[1], refs, names)
    trusted_starts = {f["start"] for f in census if {"entry", "call", "dataref"}.intersection(f["why"])}
    boundaries = sorted(trusted_starts | set(names) |
                        {parse_addr(ent["addr"])[1] for n, ent in canonical.items()
                         if n.endswith("_text_end") and ent["addr"].startswith("1:")} |
                        {hi for _, hi in GAME_UNITS.values()} | {0x47abf})
    for unit in units:
        if unit not in GAME_UNITS and unit != "engine":
            continue
        functions = verification.get(unit, {})
        for n in dict.fromkeys([*functions, *units[unit]["functions"]]):
            state = functions.get(n, {})
            status = state.get("status", units[unit]["functions"].get(n, {}).get("status"))
            if unit == "engine" and status != "NEAR_MATCH":
                continue
            address = state.get("oracle") or canonical.get(n, {}).get("addr")
            if not address:
                yield unit, n, status, None, None, None
                continue
            obj, lo = parse_addr(address)
            if obj != 1:
                raise ValueError(f"{unit}:{n} uses a non-code address {address}")
            if status in {"STRICT", "CODE_BYTES_MATCH"} and state.get("size"):
                hi, extent = lo + state["size"], "verified extent"
            else:
                hi, extent = boundaries[bisect_right(boundaries, lo)], "next trusted start or unit end"
            yield unit, n, status, (obj, lo), extent, code.signature(lo, hi)


def dice(a, b, frequency=None):
    if not (a or b):
        return None
    weight = frequency.get if frequency is not None else None
    if weight:
        shared = sum(min(count, b.get(k, 0)) * weight(k, 1) for k, count in a.items() if k in b)
        total = sum(v * weight(k, 1) for counter in (a, b) for k, v in counter.items())
    else:
        shared = sum(min(count, b.get(k, 0)) for k, count in a.items())
        total = sum(a.values()) + sum(b.values())
    return 2 * shared / total


def rarity(signatures, attr):
    counts = Counter(k for sig in signatures for k in getattr(sig, attr))
    n = len(signatures)
    return {k: 1 + math.log((n + 1) / (count + 1)) for k, count in counts.items()}


def parts(a, b, idf):
    return {
        "mnemonics": dice(a.mnemonics, b.mnemonics),
        "operands": dice(a.operands, b.operands),
        "calls": dice(a.calls, b.calls, idf["calls"]),
        "constants": dice(a.constants, b.constants, idf["constants"]),
        "size": size_ratio(a, b),
    }


def size_ratio(a, b):
    return min(a.reached, b.reached) / max(a.reached, b.reached, 1)


def aggregate(components):
    active = {k: v for k, v in components.items() if v is not None}
    return sum(WEIGHTS[k] * v for k, v in active.items()) / sum(WEIGHTS[k] for k in active)


def signature_meta(sig):
    return {
        "size": sig.size, "reachable_bytes": sig.reached, "instructions": sig.instructions,
        "named_calls": sum(sig.calls.values()), "unknown_calls": sig.unknown_calls,
        "unresolved_indirect_jumps": sig.unresolved_jumps,
    }


def show_donor(target, donor, components, score, idf, current, target_name):
    sig = donor.signature
    shared_calls = sorted(target.calls.keys() & sig.calls.keys(),
                          key=lambda key: (-idf["calls"].get(key, 1), key))
    shared_constants = sorted(target.constants.keys() & sig.constants.keys(),
                              key=lambda key: (-idf["constants"].get(key, 1), key))
    same_name = target_name.lower() == donor.name.lower()
    distinctive = [value for value in shared_constants if abs(value) >= 8]
    low_coverage = (target.unresolved_jumps or sig.unresolved_jumps) and (
        target.reached / target.size < .65 or sig.reached / sig.size < .65)
    exact_shape = target.fingerprint == sig.fingerprint and target.instructions >= 16
    # Short generic routines need identity evidence even when the arithmetic score is high.
    corroborated = (same_name or len(shared_calls) >= 2 or
                    bool(shared_calls) and len(distinctive) >= 3 or exact_shape)
    strong = score >= .72 and corroborated and not low_coverage and (
        same_name or min(target.instructions, sig.instructions) >= 12)
    delta = score - current if current is not None else None
    return {
        "file": donor.file, "function": donor.name, "profile": donor.profile, "host": donor.host,
        "build_id": donor.build_id, "segment": donor.segment, "offset": f"{donor.start:x}",
        "local_symbol": donor.local, **signature_meta(sig),
        "score": round(score, 6),
        "components": {k: None if v is None else round(v, 6) for k, v in components.items()},
        "shared_calls": [{"name": k, "count": min(target.calls[k], sig.calls[k])} for k in shared_calls],
        "shared_constants": [{"value": k, "count": min(target.constants[k], sig.constants[k])}
                             for k in shared_constants],
        "same_name": same_name, "same_instruction_shape": exact_shape,
        "low_coverage": bool(low_coverage), "strong": strong,
        "baseline_delta": None if delta is None else round(delta, 6),
        "clearly_closer": bool(strong and delta is not None and delta >= .08),
    }


class DonorIndex:
    def __init__(self, donors):
        self.donors = donors
        self.idf = {key: rarity([d.signature for d in donors], key) for key in ("calls", "constants")}
        self.postings = {}
        self.totals = {}
        for key in ("mnemonics", "operands", "calls", "constants"):
            postings = defaultdict(list)
            frequency = self.idf.get(key, {})
            totals = []
            for i, donor in enumerate(donors):
                values = getattr(donor.signature, key)
                totals.append(sum(count * frequency.get(term, 1) for term, count in values.items()))
                for term, count in values.items():
                    postings[term].append((i, count))
            self.postings[key] = postings
            self.totals[key] = totals

    def compare_feature(self, target, key):
        frequency = self.idf.get(key, {})
        values = getattr(target, key)
        totals = self.totals[key]
        overlap = [0.0] * len(totals)
        target_total = 0
        for term, count in values.items():
            weight = frequency.get(term, 1)
            target_total += count * weight
            for i, other in self.postings[key].get(term, ()):
                overlap[i] += min(count, other) * weight
        return [2 * n / (target_total + total) if target_total + total else None
                for n, total in zip(overlap, totals)]

    def rank(self, target, name, baseline, count=3):
        fields = (*self.postings, "size")
        values = [self.compare_feature(target, key) for key in self.postings]
        values.append([size_ratio(target, d.signature) for d in self.donors])
        rows = []
        for d, observed in zip(self.donors, zip(*values)):
            components = dict(zip(fields, observed))
            score = aggregate(components)
            rows.append((score, d, components))
        rows.sort(key=lambda r: (-round(r[0], 10), r[1].name.lower() != name.lower(),
                                 r[1].host != "dos", r[1].file.lower(), r[1].name, r[1].profile))
        picked = {}
        for score, d, components in rows:
            identity = d.file, d.name.lower()
            if identity not in picked:
                picked[identity] = show_donor(target, d, components, score, self.idf, baseline, name)
                if len(picked) == count:
                    break
        return list(picked.values())


def files_summary(builds):
    grouped = defaultdict(list)
    for build in builds.values():
        grouped[build["source"]].append(build)
    return grouped


def short_file(path):
    parts = Path(path).parts
    if parts[:3] == ("external", "duke3d-1.3d", "src"):
        return f"Duke 1.3D tree/{parts[-1]}"
    if parts[:3] == ("external", "duke3d-1.3d", "dukenukem3d"):
        return f"Duke 1.5/{parts[-1]}"
    return "/".join(parts[-3:]) if len(parts) < 6 else "/".join(
        (parts[2], *parts[4:]))


def format_score(score):
    return "--" if score is None else f"{score:.3f}"


def compact_hits(hits, field, limit=5):
    shown = [str(r[field]) + (f" ×{r['count']}" if r["count"] > 1 else "") for r in hits[:limit]]
    return ", ".join(shown) + (f" (+{len(hits) - limit})" if len(hits) > limit else "") if shown else "--"


def shortlist(targets):
    rows = [(target, target["best_strong"] or target["donors"][0])
            for target in targets if target["status"] == "NEAR_MATCH" and target["donors"]
            and target["baseline_score"] is not None]
    rows.sort(key=lambda row: (-row[1]["clearly_closer"], -row[1]["strong"],
                               -max(row[1]["baseline_delta"], 0), -row[1]["score"], row[0]["address"]))
    return rows[:10]


def write_report(result):
    targets = result["targets"]
    totals = result["summary"]
    builds = result["builds"]
    grouped = files_summary(builds)
    lines = [
        "# LameDuke cross-source donor scan",
        "",
        f"Watcom 9.5 GA (wc95b) compiled **{totals['compiled_files']}/{totals['source_files']}** source files "
        f"and produced **{totals['successful_builds']}** successful host/profile builds. The scan evaluated "
        f"**{totals['donor_variants']:,}** named function variants across "
        f"**{totals['distinct_donors']:,}** source-file/function pairs.",
        "",
        f"**{totals['targets']} targets** include {totals['game_targets']} game functions and "
        f"{totals['engine_near_targets']} engine NEAR functions. **{totals['strong_targets']}** have a "
        f"strong donor in the top three ({totals['strong_near_targets']} are NEAR). "
        f"The strong donor beats the current score by at least 0.08 for "
        f"**{totals['clearly_closer_targets']}** targets.",
        "",
        "## Most promising NEAR counterparts",
        "",
        f"Ranked by clear gains, corroboration, positive gain, and donor score among the {totals['near_targets']} NEAR targets.",
        "",
        "| Target | Donor | Profile | Score | Current | Gain | Evidence | Shared calls |",
        "|---|---|---|---:|---:|---:|---|---|",
    ]
    best = shortlist(targets)
    for r, d in best:
        evidence = "Clearly closer" if d["clearly_closer"] else "Strong" if d["strong"] else "Weak"
        lines.append(f"| {r['address']} `{r['function']}` | {short_file(d['file'])} `{d['function']}` | "
                     f"{d['profile']} | {d['score']:.3f} | {format_score(r['baseline_score'])} | "
                     f"{d['baseline_delta']:+.3f} | {evidence} | {compact_hits(d['shared_calls'], 'name')} |")
    if not best:
        lines.append("| -- | -- | -- | -- | -- | -- | -- | -- |")
    lines.extend([
        "",
        "## Scoring and evidence",
        "",
        f"Baseline and oracle roster captured at {result['captured_at_utc']}. The freshly compiled objects "
        "were checked against that verification snapshot before ranking.",
        "- The score blends mnemonic bigram/trigram Dice similarity (40%), operand-shape bigram/trigram "
        "Dice similarity (27%), named-call overlap (13%), nonrelocated immediate overlap (13%), and reachable-byte "
        "size ratio (7%). Call and constant counts use inverse donor frequency. A component absent on both "
        "sides is omitted and its weight redistributed proportionally. Names only break score ties and corroborate hits.",
        "- Reachability follows branches and relocation-backed indexed jump tables. Calls and jump displacements "
        "and relocated address values are excluded from immediate evidence. Known direct calls and external "
        "tail jumps are scored by name. Unknown call targets and unresolved indirect jumps are counted in the JSON.",
        "- Strong requires 0.72 and either the same function name, two shared call names, a shared call with three "
        "shared immediates of magnitude at least eight, or identical normalized instruction shapes over at least 16 "
        "instructions. Renamed counterparts need at least 12 instructions on both sides. Incomplete jump-table "
        "coverage can suppress the strong label. Clearly closer also requires an improvement of at least 0.08.",
        "- The size columns show full extents. Known verified oracle extents are retained. For NEAR targets, the "
        "next bound function, trusted entry or TU boundary supplies the end. Bare code-reference starts can be "
        "switch data and are excluded. Regular and local OMF code symbols supply donor boundaries. "
        "The scan measures similarity, and the repository verifier determines STRICT status.",
        "- Current-source scores come from fresh DOS-hosted compiles using unit flags. The historical donor "
        "matrix uses the NT host with `-bt=dos` for the broad search. DOS-hosted donor controls take "
        "precedence for the same source/profile combination. Each donor row records its host and build ID.",
        "- For repeated flag variants of a source-file/function pair the ranking retains its highest score. "
        "Historical copies in distinct archives can appear as separate donors. The JSON contains full paths, "
        "per-component scores, complete shared features and build logs. Baseline verifier fields record "
        "the first non-fixup byte difference and the first reported problem where available.",
        "",
        "## Target rankings",
    ])
    if result["live_inputs_changed_during_scan"]:
        position = lines.index("## Target rankings")
        details = ", ".join(result["live_inputs_changed_during_scan"])
        lines[position:position] = [f"Live input files updated after capture: {details}. Rankings use the capture.", ""]
    current_unit = None
    for r in targets:
        unit = r["unit"]
        if unit != current_unit:
            lines.extend(["", f"### {unit}", "",
                          "| Oracle function | Oracle B | Current | Rank | Donor | Score | Donor B | Shared calls | Shared immediates |",
                          "|---|---:|---:|---:|---|---:|---:|---|---|"])
            current_unit = unit
        for i, d in enumerate(r["donors"], 1):
            marker = " **closer**" if d["clearly_closer"] else " **strong**" if d["strong"] else ""
            target = f"{r['address']} `{r['function']}` ({r['status']})" if i == 1 else ""
            lines.append(f"| {target} | {r['oracle']['size'] if i == 1 else ''} | "
                         f"{format_score(r['baseline_score']) if i == 1 else ''} | {i} | "
                         f"{short_file(d['file'])} `{d['function']}` ({d['profile']}/{d['host']}) | "
                         f"{d['score']:.3f}{marker} | {d['size']} | {compact_hits(d['shared_calls'], 'name')} | "
                         f"{compact_hits(d['shared_constants'], 'value')} |")
        if not r["donors"]:
            lines.append(f"| {r['address']} `{r['function']}` ({r['status']}) | -- | -- | -- | -- | -- | -- | -- | -- |")
    controls = result["host_controls"]
    lines.extend(["", "## Compile inventory", "",
                  f"DOS-hosted versus DOS-targeted NT controls had identical CODE bytes, named fixups, "
                  f"and code symbols for {sum(r['identical_code_layout'] for r in controls)}/{len(controls)} pairs.",
                  "The earlier default NT target used a larger `dirent` in BUIL0422/BUILD.C, changing "
                  "`getfilenames` from 180 to 186 bytes under both engine profiles. Explicit DOS targeting "
                  "restored the 180-byte body and the full code layout in the control.",
                  "",
                  "| Source group | Files | Compiled | Successful builds |",
                  "|---|---:|---:|---:|"])
    labels = sorted({records[0]["label"] for records in grouped.values()})
    for label in labels:
        files = [records for records in grouped.values() if records[0]["label"] == label]
        successes = sum(any(b["returncode"] == 0 for b in records) for records in files)
        variants = sum(b["returncode"] == 0 for records in files for b in records)
        lines.append(f"| {label} | {len(files)} | {successes} | {variants} |")
    lines.extend(["",
                  "The Duke 1.3D tree includes reconstructed or backported code and borrowed declarations. "
                  "The Duke 1.5 sources use the declarations supplied with that release. Compatibility builds "
                  "for incomplete Ken snapshots use the header sources and edits recorded in the build logs "
                  "and in `tools_proposed/xscan_compat.py`. The March 1995 ENGINE.C ends inside "
                  "dragpoint, so the compile uses the complete prefix through neartag. The later engine "
                  "adaptations adjust inline assembler syntax for Watcom 9.5. Failed attempts include "
                  "old 16-bit inline assembly, incomplete files and inliner timeouts.", "",
                  "| Source | Successful profiles | Failed attempts by host/profile |",
                  "|---|---|---|"])
    for file, variants in grouped.items():
        def format_build(b):
            attempt = b["attempts"][-1]
            stem = Path(attempt["diagnostics"]).name
            log = f"logs-{b['host']}/{stem}"
            return f"[{b['profile']}/{b['host']}]({log})"
        success = ", ".join(format_build(b) for b in variants if b["returncode"] == 0)
        failed = ", ".join(format_build(b) for b in variants if b["returncode"] != 0)
        lines.append(f"| {short_file(file)} | {success or '--'} | {failed or '--'} |")
    lines.extend(["", "## Reproduce", "",
                  "```powershell",
                  "python -B build/workers/xscan/tools_proposed/build_corpus.py --host nt",
                  "python -B build/workers/xscan/tools_proposed/build_corpus.py --host nt --retry",
                  "python -B build/workers/xscan/tools_proposed/build_corpus.py --host dos --replay-nt "
                  "--probe BUIL0422 --probe buildold_12_30_94 --probe build_1995_03_28_partial --probe buil0723 "
                  "--probe 19950920/POLYTEX5.C --probe /SRC_9505/ENGINE.C --probe /src/game/gamedef.c "
                  "--probe /source/SECTOR.C",
                  "python -B build/workers/xscan/tools_proposed/build_corpus.py --host dos --replay-nt "
                  "--timeouts-only --probe /source/MENUES.C",
                  "python -B build/workers/xscan/tools_proposed/build_corpus.py --baseline",
                  "python -B build/workers/xscan/tools_proposed/xscan.py",
                  "```", ""])
    (WORK / "DONORS.md").write_text("\n".join(lines), encoding="utf-8")


def code_layout(object_path):
    layout = []
    for mod in omf.load(ROOT / object_path):
        for sid, seg in enumerate(mod.segments):
            if seg and seg.cls.upper() == "CODE":
                symbols = sorted((name, offset, local) for name, si, offset, local in mod.publics if si == sid)
                layout.append((seg.name, bytes(seg.data), norm_fixups(mod, sid), symbols))
    return layout


def compare_hosts(builds):
    rows = []
    for key, dos in builds.items():
        if dos["host"] != "dos" or dos["returncode"]:
            continue
        nt = builds.get("nt:" + key.removeprefix("dos:"))
        if nt and not nt["returncode"]:
            rows.append({"file": dos["source"], "profile": dos["profile"],
                         "identical_code_layout": code_layout(dos["object"]) == code_layout(nt["object"])})
    return rows


def collect(builds, aliases):
    # Prefer the canonical host for an otherwise identical source/profile.
    preferred = {}
    for build_id, row in builds.items():
        if row["returncode"] == 0:
            preferred[row["source"], row["profile"]] = build_id, row
    donors = []
    for build_id, row in preferred.values():
        found = list(object_functions(ROOT / row["object"], row["source"], row["profile"], row["host"], build_id, aliases))
        row["scanned_functions"] = len(found)
        donors.extend(found)
    return donors


def load_baselines(image, aliases, units, profiles):
    manifest = load_json(WORK / "baseline/manifest.json")
    baselines = {}
    verification = {}
    for unit, build in manifest.items():
        settings = units[unit]
        profile = settings["profile"]
        flags = settings.get("flags_override", profiles[profile]["flags"]) + settings.get("extra_flags", [])
        if (profile, flags) != (build["profile"], build["flags"]):
            raise RuntimeError(f"{unit}: current compile flags changed. Run build_corpus.py --baseline again.")
        source = ROOT / build["source"]
        obj = ROOT / build["object"]
        if hashlib.sha256(source.read_bytes()).hexdigest() != build["source_sha256"]:
            raise RuntimeError(f"{unit}: current source changed. Run build_corpus.py --baseline again.")
        if hashlib.sha256(obj.read_bytes()).hexdigest() != build["object_sha256"]:
            raise RuntimeError(f"{unit}: baseline object changed. Run build_corpus.py --baseline again.")
        for row in object_functions(obj, build["source"], build["profile"], build["host"], unit, aliases):
            baselines[unit, row.name.lower()] = row.signature
        verification[unit] = {r["function"]: r for mod in omf.load(obj) for r in verify_object(image, mod)}
    return manifest, baselines, verification


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", choices=("nt", "dos"), action="append",
                        help="Use only the selected host manifests. Default is NT plus any DOS controls.")
    args = parser.parse_args()
    builds = {}
    for host in ("nt", "dos"):
        manifest = WORK / f"corpus-{host}.json"
        if manifest.is_file() and (not args.host or host in args.host):
            builds.update({f"{host}:{k}": v for k, v in load_json(manifest).items()})
    paths = ["config/oracles.json", "config/symbols.json", "config/library_symbols.json",
             "config/units.json", "config/toolchain.json", "build/map/funcmap.json", "build/verify.json",
             "build/oracles/d3d/libsyms.json"]
    contents = {path: (ROOT / path).read_bytes() for path in paths if (ROOT / path).is_file()}
    digests = {path: hashlib.sha256(data).hexdigest() for path, data in contents.items()}
    inputs = {path: json.loads(data) for path, data in contents.items()}
    captured_at = datetime.now(timezone.utc).isoformat(timespec="seconds")
    image = Image()
    canonical = inputs["config/symbols.json"]["d3d"]["functions"]
    libraries = inputs["config/library_symbols.json"].get("d3d", {}).get("functions", {})
    runtime = inputs.get("build/oracles/d3d/libsyms.json", {})
    names, aliases = call_names(canonical, libraries, runtime)
    units = inputs["config/units.json"]["units"]
    verification = inputs["build/verify.json"]["units"]
    profiles = inputs["config/toolchain.json"]["profiles"]
    baseline_manifest, baselines, checks = load_baselines(image, aliases, units, profiles)
    census = inputs["build/map/funcmap.json"]["functions"]
    oracle_functions = list(target_functions(image, units, verification, names, canonical, census))
    for unit, name, status, address, _, oracle in oracle_functions:
        if oracle:
            check = checks.get(unit, {}).get(name)
            if check is None or (check["status"], check.get("oracle")) != (status, fmt_addr(address)):
                raise RuntimeError(f"{unit}:{name}: fresh baseline disagrees with build/verify.json")
    donors = collect(builds, aliases)
    if not donors:
        parser.error("No donor functions. Run build_corpus.py first.")
    print(f"Indexed {len(donors):,} donor variants", flush=True)
    index = DonorIndex(donors)
    targets = []
    for unit, name, status, address, extent, oracle in oracle_functions:
        baseline = baselines.get((unit, name.lower()))
        row = {
            "unit": unit, "function": name, "status": status,
            "address": fmt_addr(address) if address else None, "extent_evidence": extent,
            "oracle": signature_meta(oracle) if oracle else None,
            "baseline": signature_meta(baseline) if baseline else None,
            "baseline_score": None, "donors": [], "best_strong": None,
        }
        if oracle:
            check = checks[unit][name]
            problem = {"status": check["status"], "different_nonfixup_bytes": check.get("byte_diffs", 0),
                       "first_nonfixup_difference": check.get("first_diff"),
                       "first_problem": next(iter(check["problems"]), None)}
            row["baseline_verifier"] = problem
            if baseline:
                scores = parts(oracle, baseline, index.idf)
                row["baseline_score"] = round(aggregate(scores), 6)
                row["baseline_components"] = scores
            row["donors"] = index.rank(oracle, name, row["baseline_score"])
            row["best_strong"] = next((d for d in row["donors"] if d["strong"]), None)
        targets.append(row)
    grouped = files_summary(builds)
    summary = {
        "source_files": len(grouped),
        "compiled_files": sum(any(b["returncode"] == 0 for b in records) for records in grouped.values()),
        "successful_builds": sum(b["returncode"] == 0 for b in builds.values()),
        "donor_variants": len(donors),
        "distinct_donors": len({(d.file, d.name.lower()) for d in donors}),
        "targets": len(targets),
        "game_targets": sum(r["unit"] in GAME_UNITS for r in targets),
        "engine_near_targets": sum(r["unit"] == "engine" for r in targets),
        "near_targets": sum(r["status"] == "NEAR_MATCH" for r in targets),
        "strong_targets": sum(r["best_strong"] is not None for r in targets),
        "strong_near_targets": sum(r["best_strong"] is not None and r["status"] == "NEAR_MATCH" for r in targets),
        "clearly_closer_targets": sum(any(d["clearly_closer"] for d in r["donors"]) for r in targets),
        "missing_baselines": sum(r["baseline_score"] is None for r in targets),
    }
    changed = [path for path, expected in digests.items()
               if hashlib.sha256((ROOT / path).read_bytes()).hexdigest() != expected]
    result = {"summary": summary, "captured_at_utc": captured_at, "input_sha256": digests,
              "live_inputs_changed_during_scan": changed, "weights": WEIGHTS, "baseline_builds": baseline_manifest,
              "builds": builds, "targets": targets, "host_controls": compare_hosts(builds),
              "shortlist": [{"target": r["function"], "address": r["address"], "donor": d}
                            for r, d in shortlist(targets)]}
    write_report(result)
    dump_json(WORK / "donors.json", result)
    print(json.dumps(summary, indent=2), flush=True)


if __name__ == "__main__":
    main()
