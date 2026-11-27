"""Function census and cross-image features. These scores are diagnostic."""
from bisect import bisect_left
from collections import Counter, defaultdict
from dataclasses import dataclass
from hashlib import sha256
import json
import re
import zlib

import capstone
from capstone import x86

import blockdiff
from binary_formats import Reloc


MODES = {16: capstone.CS_MODE_16, 32: capstone.CS_MODE_32, 64: capstone.CS_MODE_64}
LIGHT = {bits: capstone.Cs(capstone.CS_ARCH_X86, mode) for bits, mode in MODES.items()}
DETAIL = {bits: capstone.Cs(capstone.CS_ARCH_X86, mode) for bits, mode in MODES.items()}
for engine in DETAIL.values():
    engine.detail = True
JUMPS = frozenset(blockdiff.POLARITY) | {"jmp", "ljmp", "jcxz", "jrcxz"}
TERMINALS = {"ret", "retf", "iret", "iretd", "iretq", "hlt", "ud2", "int3", "sysret"}
HEX = re.compile(r"(?<![\w+-])0x[0-9a-f]+")
DIRECT_OPERAND = re.compile(r"(?:-?0x[0-9a-f]+|\d+)")
FAR_OPERAND = re.compile(r"0x[0-9a-f]+(?:, |:)0x[0-9a-f]+")
STRING = re.compile(rb"[ -~\t\r\n]{4,}\0")
GENERIC = {"C:": "C:callee", "D:": "D:data", "S:": "D:string", "K:": "D:constant"}
DETAIL[32] = blockdiff.MD


class TokenDecoder:
    """Feed blockdiff's one-instruction API from runs of detailed Capstone decoding."""

    def __init__(self, engine, code):
        self.engine, self.code = engine, code
        self.run = iter(())
        self.next_address = -1
        self.decoded = {}

    def disasm(self, _data, address, count):
        if address != self.next_address:
            self.run = self.engine.disasm(self.code[address:], address)
        insn = next(self.run, None)
        if insn is None:
            self.next_address = -1
            return iter(())
        self.decoded[address] = insn
        self.next_address = address + insn.size
        return iter((insn,))


def symbol(name):
    return name.strip("_").lower()


def code_address(program, address):
    if address is None:
        return False
    index, off = address
    region = program.regions.get(index)
    return bool(region and region.executable and 0 <= off < len(region.data))


def jump_tables(region, program):
    # Consecutive absolute pointers to code are labels when an instruction or
    # another relocation references the run; a relocation from data can instead
    # be a constructor or dispatch table of function entries.
    if not region.executable:
        return {}
    pointers = {
        off: (fix.width, fix.target) for off, fix in region.relocs.items()
        if fix.width in (2, 4, 8) and fix.kind == "absolute" and code_address(program, fix.target)
    }
    local_refs = {f.target[1] for src in program.regions.values() for f in src.relocs.values()
                  if f.target and f.target[0] == region.index}
    tables = {}
    for off, (width, _) in pointers.items():
        if off - width in pointers or off + width not in pointers or off not in local_refs:
            continue
        cursor = off
        while cursor in pointers and pointers[cursor][0] == width:
            _, target = pointers[cursor]
            tables[cursor] = target
            cursor += width
    return tables


def discovery(program):
    """Generalized funcmap descent. Code addresses are (region index, offset)."""
    starts = defaultdict(set)
    tables = {reg.index: jump_tables(reg, program) for reg in program.regions.values() if reg.executable}
    if code_address(program, program.entry):
        starts[program.entry].add("entry")
    for point in program.metadata.get("unwind_extents", {}):
        if code_address(program, point):
            starts[point].add("unwind")
    for reg in program.regions.values():
        for offset in reg.publics:
            if code_address(program, (reg.index, offset)):
                starts[reg.index, offset].add("symbol")
        for offset, fix in reg.relocs.items():
            if code_address(program, fix.target) and offset not in tables.get(reg.index, {}):
                if fix.kind == "absolute":
                    starts[fix.target].add("code pointer" if reg.executable else "data pointer")
    for index, offset in tuple(starts):
        if offset in tables.get(index, {}) and starts[index, offset] == {"code pointer"}:
            del starts[index, offset]
    work = sorted(set(starts) | {a for table in tables.values() for a in table.values()}, reverse=True)
    reached = defaultdict(dict)
    while work:
        index, offset = work.pop()
        reg = program.regions[index]
        walked, table = reached[index], tables[index]
        # LE's near instructions are object-relative, even across a 16-bit object.
        base = reg.base if program.metadata.get("machine") else 0
        while offset not in walked and offset not in table and 0 <= offset < len(reg.data):
            progressed = False
            for address, size, op, args in LIGHT[reg.bits].disasm_lite(
                    reg.data[offset:offset + 196], base + offset, count=48):
                offset = address - base
                if offset in walked or any(source in table for source in range(offset, offset + size)):
                    break
                progressed = True
                walked[offset] = size
                if op in JUMPS or op.startswith(("loop", "call", "lcall")):
                    fixup = next((reg.relocs[f] for f in range(offset + 1, offset + size)
                                  if f in reg.relocs), None)
                    target = None
                    if DIRECT_OPERAND.fullmatch(args) or (fixup and FAR_OPERAND.fullmatch(args)):
                        if fixup:
                            target = fixup.target
                        else:
                            value = int(args, 16 if "0x" in args else 10)
                            target = program.va(value) if base else (index, value)
                        if code_address(program, target):
                            if op.startswith(("call", "lcall")):
                                starts[target].add("direct call")
                            work.append(target)
                offset += size
                if op in TERMINALS or op in {"jmp", "ljmp"}:
                    progressed = False
                    break
            if not progressed:
                break
    return starts, reached, tables


@dataclass(frozen=True)
class Extent:
    region: int
    start: int
    end: int
    basis: str
    reasons: tuple
    reached: int | None = None
    bound_end: int | None = None


def extents(program):
    if program.image:
        starts, reached, tables = discovery(program)
    else:
        starts = {(reg.index, off): {"public"} for reg in program.regions.values() for off in reg.publics
                  if code_address(program, (reg.index, off))}
        reached, tables = {}, {}
    offsets = defaultdict(list)
    for index, offset in sorted(starts):
        offsets[index].append(offset)
    for index, points in offsets.items():
        region = program.regions[index]
        instructions = sorted(reached.get(index, {}))
        table = tables.get(index, {})
        fields = sorted(table)
        for i, start in enumerate(points):
            bound = points[i + 1] if i + 1 < len(points) else len(region.data)
            reasons = tuple(sorted(starts[index, start]))
            if not program.image:
                # Preserve the public-to-public/segment-end byte range and reject
                # holes instead of comparing manufactured zero bytes.
                if region.coverage is not None and region.coverage.count(1, start, bound) != bound - start:
                    continue
                yield Extent(index, start, bound, "public to next public or section end", reasons)
                continue
            unwind_end = program.metadata.get("unwind_extents", {}).get((index, start))
            if unwind_end and start < unwind_end <= bound:
                yield Extent(index, start, unwind_end, "PE unwind extent", reasons)
                continue
            ins = instructions[bisect_left(instructions, start):bisect_left(instructions, bound)]
            if not ins or ins[0] != start:
                continue
            nbytes = sum(reached[index][pos] for pos in ins)
            end = min(bound, ins[-1] + reached[index][ins[-1]])
            for field in fields[bisect_left(fields, end):bisect_left(fields, bound)]:
                if table[field][0] == index and start <= table[field][1] < end:
                    end = max(end, field + region.relocs[field].width)
            yield Extent(index, start, min(end, bound), "recursive descent", reasons, nbytes, bound)


def data_content(program, index, offset):
    region = program.regions.get(index)
    if not region or region.executable or not 0 <= offset < len(region.data):
        return None
    raw = region.data[offset:offset + 48]
    if region.coverage is not None and not region.coverage[offset]:
        return None
    if STRING.match(raw):
        return "S:" + raw.split(b"\0", 1)[0].decode("ascii")
    if len(raw) >= 8 and any(raw[:8]):
        return "K:" + raw[:8].hex()
    return None


def refs_at(region, start, end, sites):
    return {offset - start: region.relocs[offset]
            for offset in sites[bisect_left(sites, start):bisect_left(sites, end)]}


def digest(value):
    return sha256(value).hexdigest()


def smallkey(value):
    return zlib.crc32(value.encode("utf-8"))


def freeze(counter):
    return sorted(counter.items())


def relative_address(program, region, target):
    if program.image and program.metadata.get("machine"):
        return program.va(region.base + target)
    return region.index, target


def features(program, extent, sites):
    region = program.regions[extent.region]
    start, end = extent.start, extent.end
    code = region.data[start:end]
    fixups = refs_at(region, start, end, sites)
    reloc_tokens, named_refs, data_refs = {}, Counter(), Counter()
    for offset, fix in fixups.items():
        target_reg, target_off = fix.target or (None, None)
        if target_reg == region.index and start <= target_off < end:
            reloc_tokens[offset] = ("L", target_off - start)
            continue
        is_code = code_address(program, fix.target) or fix.kind == "relative"
        name = fix.name or address_name(program, fix.target)
        content = data_content(program, *fix.target) if fix.target else None
        kind = "C:" if is_code else "D:"
        if name:
            tok = kind + symbol(name)
            named_refs[tok] += 1
        else:
            tok = content or kind + ("callee" if is_code else "data")
        if content:
            data_refs[content] += 1
        reloc_tokens[offset] = tok

    def branch(target, is_call):
        if 0 <= target < len(code) and not (is_call and target == 0):
            return None
        name = address_name(program, relative_address(program, region, start + target))
        return "C:" + (symbol(name) if name else "callee")

    old = blockdiff.MD
    decoder = TokenDecoder(DETAIL[region.bits], code)
    blockdiff.MD = decoder
    try:
        ins, leaders = blockdiff.tokenize(code, reloc_tokens, branch)
    finally:
        blockdiff.MD = old
    body = blockdiff.Body(ins, leaders)

    # Preserve widths and kinds at all declared relocation sites, plus rel8/rel32
    # fields for direct control transfers known to leave the extent.
    code_shape = bytearray(code)
    normalized = dict(fixups)
    calls, switch_edges = [], {}
    for insn in ins:
        op = insn.shape.split(" ", 1)[0] if insn.shape else ""
        if not (op in JUMPS or op.startswith(("j", "loop", "call", "lcall")) or
                (not op and code[insn.addr:insn.addr + 1] in (b"\xeb", b"\xe9"))):
            continue
        cap = decoder.decoded.get(insn.addr)
        if not cap:
            continue
        is_call = cap.group(x86.X86_GRP_CALL)
        if not (is_call or cap.group(x86.X86_GRP_JUMP) or cap.mnemonic.startswith("loop")):
            continue
        operand = cap.operands[0] if cap.operands else None
        if operand is None:
            continue
        field = cap.address + (cap.imm_offset if operand.type == x86.X86_OP_IMM else cap.disp_offset)
        fixup = fixups.get(field)
        target, name = (fixup.target, fixup.name) if fixup else (None, None)
        if (fixup is None and operand.type == x86.X86_OP_IMM and
                cap.mnemonic not in {"lcall", "ljmp"} and 0 < cap.imm_size <= 4):
            delta = int.from_bytes(code[field:field + cap.imm_size], "little", signed=True)
            indirect_target = insn.addr + cap.size + delta
            target = relative_address(program, region, start + indirect_target)
            if not 0 <= indirect_target < len(code):
                normalized[field] = Reloc(cap.imm_size, "relative", target)
        if is_call:
            name = name or address_name(program, target)
            if name:
                named_refs["C:" + symbol(name)] += int(fixup is None)
            calls.append((target, symbol(name) if name else None, "direct" if operand.type == x86.X86_OP_IMM else "indirect"))
        elif (op == "jmp" and operand.type == x86.X86_OP_MEM and operand.mem.index and target and
              target[0] == region.index and start <= target[1] < end):
            cursor = target[1] - start
            labels = []
            while (slot := fixups.get(cursor)) and slot.width > 0 and slot.kind == "absolute":
                if not slot.target or slot.target[0] != region.index or not start <= slot.target[1] < end:
                    break
                labels.append(slot.target[1] - start)
                cursor += slot.width
            if labels:
                switch_edges[insn.addr] = labels
    site_shape = []
    complete = True
    for offset, fix in sorted(normalized.items()):
        if not fix.width or not 0 <= offset < offset + fix.width <= len(code):
            complete = False
            continue
        code_shape[offset:offset + fix.width] = bytes(fix.width)
        label = None
        if fix.target and fix.target[0] == region.index and start <= fix.target[1] < end:
            label = fix.target[1] - start
        site_shape.append((offset, fix.width, fix.kind, label))
    signature = digest(bytes([region.bits]) + code_shape + b"\0" + json.dumps(site_shape).encode("ascii"))
    name = region.publics.get(start, [])
    ordinal = {"region": region.index, "section": region.name, "offset": start, "end": end,
               "size": len(code), "bits": region.bits, "basis": extent.basis, "reasons": extent.reasons,
               "names": list(name), "member": program.module, "member_offset": program.member_offset,
               "sha256": digest(code), "relocation_sha256": signature if complete else None,
               "relocation_sites": site_shape, "unknown_relocation_fields": len(normalized) - len(site_shape),
               "relocation_targets": [(off, fix.target, fix.name, fix.addend) for off, fix in sorted(normalized.items())],
               "direct_calls": [(target, name) for target, name, kind in calls if kind == "direct"]}
    if extent.reached is not None:
        ordinal.update(reached_bytes=extent.reached, next_boundary=extent.bound_end)
    stats = function_stats(body, calls, named_refs, data_refs, switch_edges)
    return ordinal, stats, calls


def address_name(program, address):
    if address is None:
        return None
    reg = program.regions.get(address[0])
    return next(iter(reg.publics.get(address[1], ())), None) if reg else None


def abstract(ins):
    if ins.shape is None:
        return None
    shape = ins.shape
    for event in sorted(set(ins.events), key=len, reverse=True):
        generic = GENERIC.get(event[:2])
        if generic:
            shape = shape.replace(event, generic)
    return shape


def cfg_summary(body, switch_edges):
    edges = set()
    by_addr = {address: i for i, (address, _) in enumerate(body.blocks)}
    for i, (_, block) in enumerate(body.blocks):
        last = block[-1]
        op = (last.shape or last.exact or "").split(" ", 1)[0]
        nxt = by_addr.get(last.target)
        if nxt is not None and op != "call":
            edges.add((i, nxt))
        for target in switch_edges.get(last.addr, ()):
            if target in by_addr:
                edges.add((i, by_addr[target]))
        if i + 1 < len(body.blocks) and (not last.ends or op.startswith(("j", "loop")) and op != "jmp"):
            edges.add((i, i + 1))
    outdegree = Counter(source for source, _ in edges)
    indegree = Counter(dest for _, dest in edges)
    degrees = Counter((indegree[i], outdegree[i]) for i in range(len(body.blocks)))
    return [len(body.blocks), len(edges), sum(a >= b for a, b in edges),
            [(*pair, count) for pair, count in sorted(degrees.items())]]


def function_stats(body, calls, names, data, switch_edges):
    seq = [value for ins in body.ins if (value := abstract(ins)) is not None and value != "db"]
    mnemonics = [token.split(" ", 1)[0] for token in seq]
    mn_grams = Counter(smallkey(" ".join(gram)) for gram in zip(mnemonics, mnemonics[1:], mnemonics[2:]))
    if len(mnemonics) in (1, 2):
        mn_grams[smallkey(" ".join(mnemonics))] += 1
    op_grams = Counter(smallkey("|".join(gram)) for gram in zip(seq, seq[1:]))
    constants = Counter(hit.group() for tok in seq for hit in HEX.finditer(tok.split(" ", 1)[-1])
                        if int(hit.group(), 16) > 7)
    blocks, loose = [], []
    for i, (_, block) in enumerate(body.blocks):
        key = tuple(value for ins in block if (value := abstract(ins)) is not None)
        loosened = tuple(value for tok in key
                          if not blockdiff.MOVE_VV.match(value := blockdiff.LOOSE.sub("V", tok)))
        if key:
            blocks.append((smallkey("|".join(key)), body.sizes[i]))
        if loosened:
            loose.append((smallkey("|".join(loosened)), body.sizes[i]))
    return {"mn": freeze(mn_grams), "op": freeze(op_grams), "blocks": blocks, "loose": loose,
            "named_blocks": [(smallkey("|".join(key)), sz) for key, sz in zip(body.keys, body.sizes) if key],
            "shape": cfg_summary(body, switch_edges), "constants": freeze(constants), "references": freeze(names),
            "data": freeze(data), "calls": freeze(Counter(name for _, name, _ in calls if name)),
            "call_shape": freeze(Counter(kind for _, _, kind in calls)),
            "head": smallkey(" ".join(mnemonics[:8])) if len(mnemonics) >= 5 else None,
            "instructions": len(seq)}


def collect(program, spans=None):
    sites = {reg.index: sorted(reg.relocs) for reg in program.regions.values()}
    rows, heads = [], {}
    for span in extents(program) if spans is None else spans:
        occurrence, stats, calls = features(program, span, sites[span.region])
        rows.append((occurrence, stats, calls))
        heads[span.region, span.start] = stats["head"]
    for occurrence, stats, calls in rows:
        stats["call_signatures"] = freeze(Counter(heads[target] for target, _, _ in calls if heads.get(target)))
        yield occurrence, stats
