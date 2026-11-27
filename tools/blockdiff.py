"""Diagnostic only (never proof): how much of a non-STRICT function is already right, block by block.

    python tools/blockdiff.py UNIT [FUNC ...] [--all]       metrics per function of a unit (default: non-STRICT),
                                                           from build/verify/UNIT.obj (built by tools/verify.py)
    python tools/blockdiff.py --obj OBJ [FUNC ...] [--all]  same for any object (e.g. a worker candidate)
    python tools/blockdiff.py --src SRC.c --profile P [-i=DIR ...] [FUNC ...]   compile, then compare
    python tools/blockdiff.py ... FUNC --worklist          missing/extra blocks, frame, references, renaming,
                                                           and residue regions of one function
    python tools/blockdiff.py --census [--json OUT]        every unit: where the non-STRICT bytes are
    python tools/blockdiff.py --selftest                   every STRICT function must score 1.0 everywhere

Why (ported from jng_recon tools/blockdiff.py and blupi_recon tools/residues.py, 2026-11-26): the verifier's byte
diff and linear instruction alignment are dominated by layout shifts and register cascades in large bodies, so
real progress can look like a regression. Both bodies are cut into basic blocks (branch targets, jump-table
targets, after every jump/ret) and compared as sets:

  content   oracle bytes in blocks whose abstracted content occurs in ours, in any order. Abstracted: registers,
            stack offsets, branch targets and polarity, unconditional jumps (layout artefacts). Kept: mnemonics,
            calls, globals, data contents (strings/constants), immediates, structure field offsets
  loose     content with register and stack-slot operands unified and reg-reg moves dropped (a variable kept
            in a register by one allocation and in a stack home by the other)
  fuzzy     content with partial credit: an unmatched oracle block counts with its best similarity to one of
            our unused blocks
  skeleton  Dice agreement of the call / global / data reference multisets
  layout    oracle block pairs adjacent in address order whose matched blocks are adjacent in ours too
  regs      oracle bytes in matched blocks that also agree in registers and stack offsets

References are compared through the oracle: a relocation of ours resolves to the oracle address it would have
(config/symbols.json bindings, segment bases placed by tools/match.py); unnamed initialised data compares by its
contents (both sides read their own bytes), BSS by address. Jump tables (runs of relocated fields) are read as
data, never as instructions. Oracle extent = bound address up to the next bound function start (trailing zero
padding trimmed). Strict status comes only from tools/match.py.
"""
import argparse
import bisect
import difflib
import json
import re
import struct
import sys
from collections import Counter, defaultdict
from pathlib import Path

import capstone
from capstone import x86

sys.path.insert(0, str(Path(__file__).resolve().parent))
import match  # noqa: E402
import omf  # noqa: E402
from common import BUILD, CONFIG, ROOT, load_json  # noqa: E402
from image import Image  # noqa: E402

MD = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
MD.detail = True
GP = {"eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "ax", "bx", "cx", "dx", "si", "di", "bp",
      "al", "bl", "cl", "dl", "ah", "bh", "ch", "dh"}
POLARITY = {"je": "jE", "jne": "jE", "jl": "jL", "jge": "jL", "jle": "jLE", "jg": "jLE", "jb": "jB", "jae": "jB",
            "jbe": "jBE", "ja": "jBE", "js": "jS", "jns": "jS", "jp": "jP", "jnp": "jP", "jo": "jO", "jno": "jO",
            "jecxz": "jCXZ"}
FILE_BACKED = 0xb000          # obj2 bytes below this are initialised (file-backed) data
INTERNAL = "<internal>"


def render(raw):
    """Unnamed initialised data by its contents: a string, else the first 8 bytes."""
    text = raw.split(b"\0", 1)[0]
    if text and len(text) < len(raw) and all(32 <= c < 127 or c in (9, 10, 13) for c in text):
        return "S:" + repr(text[:40].decode("latin-1"))
    if raw[:1] == b"\0" and len(raw) > 1 and raw[1:2] != b"\0":
        return "S:''"
    return "K:" + raw[:8].hex()


class Refs:
    """Oracle address -> canonical token, shared by both sides."""

    def __init__(self, img, binds):
        self.img = img
        self.names = {}
        for table in (binds.lib, binds.game):
            for n, a in table.items():
                self.names.setdefault(a, match.demangle(n))
        self.code_starts = sorted(a[1] for a in self.names if a[0] == 1)
        self.data_named = sorted(a[1] for a in self.names if a[0] == 2)

    def token(self, obj, off, content=None):
        off &= 0xFFFFFFFF
        if (obj, off) in self.names:
            return ("C:" if obj == 1 else "D:") + self.names[(obj, off)]
        if obj == 1:
            return f"F@1:{off:05x}"
        if obj == 2 and off < FILE_BACKED:
            raw = content if content is not None else self.img.obj[2][off:off + 48]
            return render(raw)
        return f"D@2:{off:05x}"

    def pretty(self, tok):
        """Readable form of an address token (worklists only)."""
        m = re.match(r"([FD])@(\d):([0-9a-f]+)", tok)
        if not m:
            return tok
        obj, off = int(m.group(2)), int(m.group(3), 16)
        starts = self.code_starts if obj == 1 else self.data_named
        i = bisect.bisect_right(starts, off) - 1
        if i >= 0 and off - starts[i] < 0x10000:
            return f"{tok}({self.names[(obj, starts[i])]}+{off - starts[i]:#x})"
        return tok


def trim_zero_tail(code):
    """Drop linker zero padding before the next module: trailing whole instructions made only of zero bytes
    within the last 16 bytes (compiler filler such as `lea esi,[esi+0]` ends in zeros too, so trimming bytes
    would cut it). Used only when the extent end is not a confirmed boundary."""
    ends, pos = [], 0
    while pos < len(code):
        ins = next(MD.disasm(code[pos:pos + 16], pos, count=1), None)
        size = ins.size if ins else 1
        ends.append((pos, pos + size))
        pos += size
    while ends and not any(code[ends[-1][0]:ends[-1][1]]) and len(code) - ends[-1][0] < 16:
        ends.pop()
    return code[:ends[-1][1]] if ends else code


class Ins:
    __slots__ = ("addr", "size", "shape", "exact", "events", "target", "ends")

    def __init__(self, addr, size, shape, exact, events, target, ends):
        self.addr, self.size, self.shape, self.exact = addr, size, shape, exact
        self.events, self.target, self.ends = events, target, ends


def is_pad(ins):
    if ins.mnemonic == "nop":
        return True
    if ins.mnemonic in ("mov", "xchg") and len(ins.operands) == 2 and all(o.type == x86.X86_OP_REG for o in ins.operands):
        return ins.operands[0].reg == ins.operands[1].reg
    if ins.mnemonic == "lea" and len(ins.operands) == 2:
        d, s = ins.operands
        return d.type == x86.X86_OP_REG and s.type == x86.X86_OP_MEM and s.mem.base == d.reg and \
            not s.mem.index and s.mem.disp == 0
    return False


def tokenize(code, relocs, branch):
    """code: function bytes (addresses relative to the function start).
    relocs: field offset -> token, INTERNAL-tuple ("L", off) for a reference into this function, or None.
    branch(rel_target, is_call) -> token for a target outside the function (None = inside)."""
    out, leaders = [], set()
    frame = {"esp"}
    pos = 0
    first = True
    while pos < len(code):
        if pos in relocs:                                   # a relocated field cannot start an instruction: data
            ref = relocs[pos]
            if isinstance(ref, tuple):
                leaders.add(ref[1])
            out.append(Ins(pos, 4, None, None, [], None, True))
            pos += 4
            continue
        ins = next(MD.disasm(code[pos:pos + 16], pos, count=1), None)
        if ins is None:
            out.append(Ins(pos, 1, "db", "db", [], None, False))
            pos += 1
            continue
        fields = {ins.address + ins.disp_offset if ins.disp_offset else None,
                  ins.address + ins.imm_offset if ins.imm_offset else None} - {None}
        inside = [s for s in relocs if ins.address < s < ins.address + ins.size]
        if any(s not in fields for s in inside):              # would swallow a relocated field: not code here
            out.append(Ins(pos, 1, "db", "db", [], None, False))
            pos += 1
            continue
        pos += ins.size
        if first and ins.mnemonic == "push":
            pass
        if ins.mnemonic == "mov" and ins.op_str == "ebp, esp":
            frame = {"esp", "ebp"}
        first = False
        if is_pad(ins):
            out.append(Ins(ins.address, ins.size, None, None, [], None, False))
            continue
        m = ins.mnemonic
        ends = m in ("ret", "retf", "iretd", "jmp") or m in POLARITY or m.startswith("loop")
        if m in ("call", "jmp") or m in POLARITY or m.startswith("loop"):
            op = ins.operands[0] if ins.operands else None
            if op is not None and op.type == x86.X86_OP_IMM:
                fld = ins.address + ins.imm_offset if ins.imm_offset else None
                ref = relocs.get(fld) if fld is not None else None
                if isinstance(ref, tuple):                      # relocated branch to a label of this function
                    t, name = ref[1], None
                    if m == "call":                             # relocated recursive call (the entry may follow
                        name = branch(t, True)                     # a leading jump table): a call, as on the oracle side
                elif ref is not None:
                    name = ref
                else:
                    t = op.imm & 0xFFFFFFFF
                    t = t - (1 << 32) if t & 0x80000000 else t
                    name = branch(t, m == "call")
                if name is None:
                    if True:
                        cls = POLARITY.get(m, m)
                        shape = None if m == "jmp" else cls
                        exact = None if m == "jmp" else m
                        out.append(Ins(ins.address, ins.size, shape, exact, [], t, ends))
                        continue
                tok = f"{m} {name}"
                out.append(Ins(ins.address, ins.size, tok, tok, [name], None, ends))
                continue
        sh, ex, events = [], [], []
        for op in ins.operands:
            if op.type == x86.X86_OP_REG:
                r = ins.reg_name(op.reg)
                ex.append(r)
                sh.append("R" if r in GP else r)
            elif op.type == x86.X86_OP_IMM:
                fld = ins.address + ins.imm_offset if ins.imm_offset else None
                ref = relocs.get(fld) if fld is not None else None
                if fld in relocs:
                    tok = "L" if isinstance(ref, tuple) else (ref or "?")
                    if isinstance(ref, tuple):
                        leaders.add(ref[1])
                    events.append(tok)
                    ex.append("&" + tok)
                    sh.append("&" + tok)
                else:
                    v = f"{op.imm & 0xFFFFFFFF:#x}"
                    ex.append(v)
                    sh.append(v)
            elif op.type == x86.X86_OP_MEM:
                b = ins.reg_name(op.mem.base) if op.mem.base else ""
                i = ins.reg_name(op.mem.index) if op.mem.index else ""
                seg = ins.reg_name(op.mem.segment) + ":" if op.mem.segment else ""
                idx_e = f"+{i}*{op.mem.scale}" if i else ""
                idx_s = f"+R*{op.mem.scale}" if i else ""
                fld = ins.address + ins.disp_offset if ins.disp_offset else None
                if fld is not None and fld in relocs:
                    ref = relocs[fld]
                    tok = "D:table" if isinstance(ref, tuple) else (ref or "?")
                    if isinstance(ref, tuple):
                        leaders.add(ref[1])
                    events.append(tok)
                    bb = "+R" if b else ""
                    ex.append(f"{seg}m{op.size}[{tok}{'+' + b if b else ''}{idx_e}]")
                    sh.append(f"{seg}m{op.size}[{tok}{bb}{idx_s}]")
                elif b in frame and not i:
                    ex.append(f"{seg}m{op.size}[{b}{op.mem.disp:+#x}]")
                    sh.append(f"{seg}m{op.size}[{b}+S]")
                else:
                    ex.append(f"{seg}m{op.size}[{b}{idx_e}{op.mem.disp:+#x}]")
                    sh.append(f"{seg}m{op.size}[{'R' if b else ''}{idx_s}{op.mem.disp:+#x}]")
        out.append(Ins(ins.address, ins.size, m + " " + ",".join(sh), m + " " + ",".join(ex), events, None, ends))
    return out, leaders


LOOSE = re.compile(r"(?:[a-z]+:)?m\d+\[(?:esp|ebp)\+S\]|\bR\b")
MOVE_VV = re.compile(r"(mov|xchg) V,V$")


class Body:
    """One function cut into basic blocks."""

    def __init__(self, ins, extra_leaders):
        self.ins = ins
        addrs = [i.addr for i in ins]
        leaders = {addrs[0]} if addrs else set()
        for k, i in enumerate(ins):
            if i.target is not None:
                j = bisect.bisect_left(addrs, i.target)
                if j < len(addrs):
                    leaders.add(addrs[j])
            if i.ends and k + 1 < len(ins):
                leaders.add(addrs[k + 1])
        for t in extra_leaders:
            j = bisect.bisect_left(addrs, t)
            if j < len(addrs):
                leaders.add(addrs[j])
        self.blocks = []
        cur = []
        for i in ins:
            if i.addr in leaders and cur:
                self.blocks.append((cur[0].addr, cur))
                cur = []
            cur.append(i)
        if cur:
            self.blocks.append((cur[0].addr, cur))
        self.keys = [tuple(x.shape for x in b if x.shape is not None) for _, b in self.blocks]
        self.exact = [tuple(x.exact for x in b if x.exact is not None) for _, b in self.blocks]
        self.sizes = [sum(x.size for x in b if x.shape is not None) for _, b in self.blocks]
        # loose: a variable in a register and one in a stack slot look alike (Watcom spill/home choices), and
        # register-to-register moves (which exist only under one allocation) are dropped
        self.loose = [tuple(t for t in (LOOSE.sub("V", x) for x in k) if not MOVE_VV.match(t)) for k in self.keys]
        self.events = Counter(e for i in ins for e in i.events)


# ---------------------------------------------------------------- the two sides

class Pair:
    """One candidate function and its oracle counterpart."""

    def __init__(self, name, status, cand_code, cand_relocs, cand_branch, orig_code, orig_relocs, orig_branch, addr):
        self.name, self.status, self.addr = name, status, addr
        ci, cl = tokenize(cand_code, cand_relocs, cand_branch)
        oi, ol = tokenize(orig_code, orig_relocs, orig_branch)
        self.cand, self.orig = Body(ci, cl), Body(oi, ol)
        self.cand_size, self.orig_size = len(cand_code), len(orig_code)


class Analyzer:
    def __init__(self):
        self.img = Image()
        self.binds = match.Bindings(self.img)
        self.refs = Refs(self.img, self.binds)
        starts = set()
        for table in (self.binds.game, self.binds.lib):
            starts.update(a[1] for a in table.values() if a[0] == 1)
        self.starts = sorted(starts)

    def oracle_extent(self, addr, cand_size, last, lead=0, table_end=False):
        """Oracle bytes of the function at addr: up to the candidate's end when that end is a confirmed boundary
        (STRICT extents, or an extent that ends at the next function's jump table, per tools/match.py), else up to
        the next bound function start; for a module's last public also up to the next funcmap start (the following
        module may be unbound library code). lead = bytes of this function's own jump tables before its label."""
        cend = addr + cand_size
        i = bisect.bisect_right(self.starts, addr + lead)
        end = self.starts[i] if i < len(self.starts) else len(self.img.obj[1])
        if table_end and cend <= end:
            end = cend
        elif cend <= end and (self.img.is_boundary(1, cend, self.binds) or
                            match.object_end_confirmed(self.img, 1, cend, self.img.obj[1][addr:cend], self.binds)):
            end = cend
        elif last and Image._starts is not None:
            fm = sorted(Image._starts)
            j = bisect.bisect_right(fm, addr + max(cand_size - 64, 0))
            if j < len(fm) and fm[j] < end:
                end = fm[j]
        code = self.img.obj[1][addr:end]
        return (code, True) if end == cend else (trim_zero_tail(code), False)

    def oracle_side(self, addr, code):
        relocs = {}
        for s in self.img.sites_in(1, addr, addr + len(code)):
            _, tobj, toff = self.img.fixups[(1, s)]
            toff &= 0xFFFFFFFF
            if tobj == 1 and addr <= toff < addr + len(code):
                relocs[s - addr] = ("L", toff - addr)
            else:
                relocs[s - addr] = self.refs.token(tobj, toff)

        def branch(t, is_call):
            if 0 <= t < len(code) and not is_call:      # a call is never internal: inside the extent it is recursion
                return None
            return self.refs.token(1, addr + t)
        return relocs, branch

    def object_pairs(self, obj_path, only=None, all_funcs=False):
        mod = omf.load(obj_path)[0]
        seg_rep = []
        res = match.verify_object(self.img, mod, None, seg_report=seg_rep)
        bases = {}
        for ent in seg_rep:
            if "oracle" in ent:
                o, b = ent["oracle"].split(":")
                bases[ent["segment"]] = (int(o), int(b, 16))
        si_by_name = {s.name: i for i, s in enumerate(mod.segments) if s}
        code_bases = {}
        for r in res:
            if r.get("oracle"):
                o, b = r["oracle"].split(":")
                code_bases.setdefault(si_by_name[r["segment"]], (int(o), int(b, 16) - r["seg_offset"]))
        pubs = {}
        for n, s, o, _ in mod.publics:
            pubs.setdefault(s, {})[o] = match.demangle(n)
        out = []
        for r in res:
            if r["status"] == "UNPLACED" or "oracle" not in r:
                continue
            if only and r["function"] not in only:
                continue
            if not only and not all_funcs and r["status"] == "STRICT":
                continue
            si = si_by_name[r["segment"]]
            seg = mod.segments[si]
            lead = r.get("leading_table", 0)          # switch tables before the label belong to this function
            o0, size = r["seg_offset"] - lead, r["size"]
            code = bytes(seg.data[o0:o0 + size])
            addr = int(r["oracle"].split(":")[1], 16) - lead
            own = dict(code_bases)
            own[si] = (1, addr - o0)          # this function's placement fixes its own segment's base (as match.py)
            relocs = {}
            for f in mod.fixups:
                if f.seg != si or not o0 <= f.off < o0 + size:
                    continue
                rel = f.off - o0
                field = struct.unpack_from("<i", seg.data, f.off)[0] if f.size() == 4 else 0
                tm, d = f.target[0] & 3, f.target[1]
                if f.self_rel:
                    if tm == 2:
                        a, _ = self.binds.lookup(mod.externs[d])
                        if a is None:                      # a public of this object
                            for n2, s2, oo, _l in mod.publics:
                                if n2 == mod.externs[d] and s2 in own:
                                    a = (own[s2][0], own[s2][1] + oo)
                        relocs[rel] = self.refs.token(*a) if a else "C:" + match.demangle(mod.externs[d])
                    elif tm == 0 and d == si:                   # MASM: relocated jump within the segment
                        t = f.disp + field
                        relocs[rel] = ("L", t - o0) if o0 <= t < o0 + size else self.refs.token(1, addr - o0 + t)
                    else:
                        relocs[rel] = None
                    continue
                addend = f.disp + field
                if tm == 2:
                    a, _ = self.binds.lookup(mod.externs[d])
                    relocs[rel] = self.refs.token(a[0], a[1] + addend) if a else \
                        "D:" + match.demangle(mod.externs[d]) + (f"{addend:+#x}" if addend else "")
                elif tm == 0:
                    tseg = mod.segments[d]
                    if d == si and o0 <= addend < o0 + size:
                        relocs[rel] = ("L", addend - o0)
                        continue
                    if tseg.cls.upper() == "CODE":
                        nm = pubs.get(d, {}).get(addend)
                        if d in own:
                            relocs[rel] = self.refs.token(own[d][0], own[d][1] + addend)
                        else:
                            relocs[rel] = "C:" + nm if nm else f"F:{tseg.name}+{addend:#x}"
                        continue
                    base = bases.get(tseg.name)
                    initialised = tseg.cls.upper() != "BSS" and addend < len(tseg.data)
                    content = bytes(tseg.data[addend:addend + 48]) if initialised else None
                    if base is not None:
                        a = (base[0], base[1] + addend)
                        if a in self.refs.names or not initialised:
                            relocs[rel] = self.refs.token(*a)
                        else:
                            relocs[rel] = render(content)
                    else:
                        relocs[rel] = render(content) if initialised else f"D:{tseg.name}+{addend:#x}"
                else:
                    relocs[rel] = "?"

            def branch(t, is_call, _size=size, _addr=addr):
                if 0 <= t < _size and not is_call:
                    return None
                return self.refs.token(1, _addr + t)
            last = all(oo <= o0 for oo in pubs.get(si, {}))
            orig, confirmed = self.oracle_extent(addr, size, last, lead, "trailing_table_of_next" in r)
            if not confirmed:
                code = trim_zero_tail(code)
            orel, obr = self.oracle_side(addr, orig)
            out.append(Pair(r["function"], r["status"], code, relocs, branch, orig, orel, obr, addr))
        return out


# ---------------------------------------------------------------- metrics (jng_recon)

def pair_blocks(ob, cb):
    avail = defaultdict(list)
    for j, k in enumerate(cb.keys):
        if k:
            avail[k].append(j)
    used = defaultdict(int)
    out = []
    for k in ob.keys:
        lst = avail.get(k) if k else None
        if lst and used[k] < len(lst):
            out.append(lst[used[k]])
            used[k] += 1
        else:
            out.append(None)
    return out


def fuzzy(ob, cb, m):
    total = sum(ob.sizes) or 1
    credit = sum(sz for sz, j in zip(ob.sizes, m) if j is not None)
    taken = {j for j in m if j is not None}
    free = [j for j, k in enumerate(cb.keys) if k and j not in taken]
    todo = sorted((i for i, (j, k) in enumerate(zip(m, ob.keys)) if j is None and k), key=lambda i: -ob.sizes[i])
    for i in todo:
        if not free:
            break
        sm = difflib.SequenceMatcher(None, autojunk=False)
        sm.set_seq2(ob.keys[i])
        ranked = []
        for j in free:
            sm.set_seq1(cb.keys[j])
            ranked.append((sm.real_quick_ratio(), sm.quick_ratio(), j))
        ranked.sort(reverse=True)
        best, bj = 0.0, None
        for _, q, j in ranked[:4]:
            if q <= best:
                break
            sm.set_seq1(cb.keys[j])
            r = sm.ratio()
            if r > best:
                best, bj = r, j
        if bj is not None:
            credit += best * ob.sizes[i]
            free.remove(bj)
    return credit / total


def loose_content(ob, cb):
    """content with register and stack-slot operands unified (allocation-insensitive)."""
    avail = Counter(k for k in cb.loose if k)
    total = sum(ob.sizes) or 1
    got = 0
    for k, sz in zip(ob.loose, ob.sizes):
        if not k:
            got += sz
        elif avail[k] > 0:
            avail[k] -= 1
            got += sz
    return got / total


def metrics(ob, cb):
    m = pair_blocks(ob, cb)
    total = sum(ob.sizes) or 1
    matched = sum(sz for sz, j in zip(ob.sizes, m) if j is not None)
    regs = sum(sz for sz, j, ex in zip(ob.sizes, m, ob.exact) if j is not None and cb.exact[j] == ex)
    live = [(i, j) for i, j in enumerate(m) if ob.keys[i]]
    pairs = good = 0
    corder = {j: n for n, j in enumerate(j for j, k in enumerate(cb.keys) if k)}
    for (i1, j1), (i2, j2) in zip(live, live[1:]):
        if j1 is not None and j2 is not None:
            pairs += 1
            good += corder[j2] == corder[j1] + 1
    a, b = ob.events, cb.events
    inter = sum((a & b).values())
    skel = 2 * inter / (sum(a.values()) + sum(b.values())) if (a or b) else 1.0
    taken = {x for x in m if x is not None}
    return {"content": round(matched / total, 4), "loose": round(loose_content(ob, cb), 4),
            "fuzzy": round(fuzzy(ob, cb, m), 4), "skeleton": round(skel, 4),
            "layout": round(good / pairs, 4) if pairs else (1.0 if matched == total else 0.0),
            "regs": round(regs / total, 4), "blocks": sum(1 for k in ob.keys if k),
            "matched_blocks": sum(1 for i, j in live if j is not None),
            "extra_blocks": sum(1 for j, k in enumerate(cb.keys) if k and j not in taken)}


def category(r, size_ratio):
    """Where a non-STRICT function stands (diagnostic triage, jng_recon E52)."""
    if abs(size_ratio - 1) > 0.10:
        return "size off >10%"
    if r["content"] >= 0.95:
        return "code written: order/registers left"
    if r["loose"] >= 0.95:
        return "code written: register/stack homes differ"
    if r["fuzzy"] >= 0.95:
        return "nearly written: small block edits"
    return "statements missing or wrong"


# ---------------------------------------------------------------- residues (blupi_recon) and worklist

REG_RE = re.compile(r"\b(e[abcd]x|e[sd]i|ebp|[abcd][lhx]|[sd]i|bp)\b")
REG32 = {"al": "eax", "ah": "eax", "ax": "eax", "bl": "ebx", "bh": "ebx", "bx": "ebx", "cl": "ecx", "ch": "ecx",
         "cx": "ecx", "dl": "edx", "dh": "edx", "dx": "edx", "si": "esi", "di": "edi", "bp": "ebp"}
SLOT = re.compile(r"m(\d+)\[(esp|ebp)([+-]0x[0-9a-f]+)\]")


def classify(a, b):
    if not a:
        return "delete"
    if not b:
        return "insert"
    shape = lambda t: REG_RE.sub("R", SLOT.sub(r"m\1[\2+S]", t))
    noconst = lambda t: re.sub(r"0x[0-9a-f]+|\b\d+\b", "K", t)
    if len(a) == len(b):
        if [shape(x) for x in a] == [shape(y) for y in b]:
            return "register"
        if [noconst(x) for x in a] == [noconst(y) for y in b]:
            return "constant"
    if Counter(a) == Counter(b) or Counter(shape(x) for x in a) == Counter(shape(y) for y in b):
        return "order"
    return "mixed"


def residues(p):
    """Linear alignment of the exact instruction streams; every non-equal region classified (oracle side a)."""
    a = [i.exact for i in p.orig.ins if i.exact is not None]
    b = [i.exact for i in p.cand.ins if i.exact is not None]
    sm = difflib.SequenceMatcher(None, a, b, autojunk=False)
    counts = Counter()
    for op, i1, i2, j1, j2 in sm.get_opcodes():
        if op != "equal":
            counts[classify(a[i1:i2], b[j1:j2])] += max(i2 - i1, j2 - j1)
    return counts


def renaming(ob, cb, m):
    rv, sv = Counter(), Counter()
    pairs = [(i, j) for i, j in enumerate(m) if j is not None]
    for i, j in pairs:
        for eo, ec in zip(ob.exact[i], cb.exact[j]):
            for x, y in zip(REG_RE.findall(eo), REG_RE.findall(ec)):
                rv[REG32.get(x, x), REG32.get(y, y)] += 1
            for (_, _, x), (_, _, y) in zip(SLOT.findall(eo), SLOT.findall(ec)):
                sv[int(x, 16), int(y, 16)] += 1

    def greedy(votes):
        out, uo, uc = {}, set(), set()
        for (o, c), _ in votes.most_common():
            if o not in uo and c not in uc:
                out[c] = o
                uo.add(o)
                uc.add(c)
        return out, sum(n for (o, c), n in votes.items() if out.get(c) != o)
    rmap, rconf = greedy(rv)
    smap, sconf = greedy(sv)
    return {"reg_map": {c: o for c, o in rmap.items() if c != o}, "reg_conflicts": rconf,
            "slot_moves": sum(1 for c, o in smap.items() if c != o), "slot_conflicts": sconf}


def frame(body):
    saved, size = [], 0
    for x in body.ins[:16]:
        e = x.exact or ""
        if e.startswith("push ") and "," not in e:
            saved.append(e[5:])
        elif e.startswith("sub esp,"):
            try:
                size = int(e.split(",")[1], 16)
            except ValueError:
                pass
            break
    return {"size": size, "saved": saved}


def worklist(az, p):
    ob, cb = p.orig, p.cand
    m = pair_blocks(ob, cb)
    r = metrics(ob, cb)
    pretty = lambda toks: " ; ".join(re.sub(r"[FD]@\d:[0-9a-f]+", lambda mm: az.refs.pretty(mm.group(0)), t)
                                     for t in toks)
    print(f"{p.name} (oracle 1:{p.addr:05x}, {p.orig_size} B; ours {p.cand_size} B; {p.status}): content "
          f"{r['content']} loose {r['loose']} fuzzy {r['fuzzy']} skeleton {r['skeleton']} layout {r['layout']} regs {r['regs']} | "
          f"{r['matched_blocks']}/{r['blocks']} oracle blocks matched, {r['extra_blocks']} of ours extra")
    fo, fc = frame(ob), frame(cb)
    print(f"frame {'equal' if fo == fc else 'DIFFERS'}: oracle sub esp {fo['size']:#x} saves {' '.join(fo['saved']) or '-'}"
          f" | ours sub esp {fc['size']:#x} saves {' '.join(fc['saved']) or '-'}")
    only_o, only_c = ob.events - cb.events, cb.events - ob.events
    fmt = lambda cnt: ", ".join(pretty([k]) + (f" x{n}" if n > 1 else "") for k, n in sorted(cnt.items())) or "none"
    print(f"references only in the oracle: {fmt(only_o)}")
    print(f"references only in ours:       {fmt(only_c)}")
    ren = renaming(ob, cb, m)
    swaps = ", ".join(f"{a}->{b}" for a, b in sorted(ren["reg_map"].items())) or "none"
    print(f"registers in matched blocks: ours->oracle {swaps}; {ren['reg_conflicts']} register uses and "
          f"{ren['slot_conflicts']} stack-slot uses disagree with one consistent renaming; {ren['slot_moves']} slots moved")
    res = residues(p)
    print("residue regions (instructions, linear alignment): " +
          (", ".join(f"{k} {v}" for k, v in res.most_common()) or "none"))
    print("missing (oracle blocks without a counterpart; neighbours show where the code belongs):")
    for i, (j, k) in enumerate(zip(m, ob.keys)):
        if j is not None or not k:
            continue
        prv = next((m[x] for x in range(i - 1, -1, -1) if m[x] is not None), None)
        nxt = next((m[x] for x in range(i + 1, len(m)) if m[x] is not None), None)
        ctx = " / ".join(x for x in (prv is not None and f"after ours +{cb.blocks[prv][0]:#x}",
                                     nxt is not None and f"before ours +{cb.blocks[nxt][0]:#x}") if x)
        print(f"  oracle +{ob.blocks[i][0]:#x} (1:{p.addr + ob.blocks[i][0]:05x}) {ob.sizes[i]} B {len(k)} ins"
              f"{' (' + ctx + ')' if ctx else ''}")
        print("      " + pretty(ob.exact[i][:14]) + (" ..." if len(k) > 14 else ""))
    taken = {x for x in m if x is not None}
    print("extra (our blocks without a counterpart in the oracle):")
    for j, k in enumerate(cb.keys):
        if k and j not in taken:
            print(f"  ours +{cb.blocks[j][0]:#x} {cb.sizes[j]} B {len(k)} ins")
            print("      " + pretty(cb.exact[j][:14]) + (" ..." if len(k) > 14 else ""))


# ---------------------------------------------------------------- drivers

def unit_obj(unit):
    p = BUILD / "verify" / f"{unit}.obj"
    if not p.exists():
        raise SystemExit(f"{p} missing: run python tools/verify.py {unit}")
    return p


def compile_src(src, prof_name, includes):
    import os
    import wcc
    prof = wcc.profile(prof_name)
    out = BUILD / "match" / f"blockdiff.{os.getpid()}.obj"
    out.parent.mkdir(parents=True, exist_ok=True)
    rc, text = wcc.compile_c(Path(src).resolve(), out, list(prof["flags"]), prof["install"],
                             [str(Path(i).resolve()) for i in includes] + [str(Path(src).resolve().parent)])
    if rc:
        raise SystemExit(text)
    return out


def row(p):
    r = metrics(p.orig, p.cand)
    r["category"] = category(r, p.cand_size / max(p.orig_size, 1))
    return r


def print_rows(pairs):
    for p in pairs:
        r = row(p)
        print(f"{p.status:16s} {p.name:28s} {p.orig_size:6d} B ours {p.cand_size:6d}  content {r['content']:.3f} "
              f"loose {r['loose']:.3f} fuzzy {r['fuzzy']:.3f} skel {r['skeleton']:.3f} layout {r['layout']:.3f} regs {r['regs']:.3f}  "
              f"blocks {r['matched_blocks']}/{r['blocks']} (+{r['extra_blocks']})  {r['category']}")


def census(az, out_json=None):
    units = load_json(CONFIG / "units.json")["units"]
    rows, cats = [], Counter()
    for unit, ent in units.items():
        if ent.get("kind") == "asm" or not (BUILD / "verify" / f"{unit}.obj").exists():
            continue
        for p in az.object_pairs(unit_obj(unit)):
            r = row(p)
            rows.append({"unit": unit, "function": p.name, "status": p.status, "oracle": f"1:{p.addr:05x}",
                         "oracle_size": p.orig_size, "candidate_size": p.cand_size, **r})
            cats[r["category"]] += p.orig_size
    rows.sort(key=lambda x: -x["oracle_size"])
    total = sum(x["oracle_size"] for x in rows) or 1
    print(f"{len(rows)} non-STRICT functions, {total:,} oracle bytes (diagnostic, not proof):")
    for c, b in cats.most_common():
        n = sum(1 for x in rows if x["category"] == c)
        print(f"  {c:40s} {n:4d} functions {b:8,d} B ({100 * b / total:5.1f}%)")
    wcontent = sum(x["content"] * x["oracle_size"] for x in rows) / total
    wfuzzy = sum(x["fuzzy"] * x["oracle_size"] for x in rows) / total
    print(f"  byte-weighted block content {wcontent:.3f}, fuzzy {wfuzzy:.3f}")
    summary = {"functions": len(rows), "bytes": total, "categories": dict(cats),
               "content_weighted": round(wcontent, 4), "fuzzy_weighted": round(wfuzzy, 4)}
    path = Path(out_json) if out_json else BUILD / "blockdiff.json"
    path.write_text(json.dumps({"summary": summary, "functions": rows}, indent=1), encoding="utf-8")
    print(f"wrote {path}")
    return 0


def selftest(az):
    units = load_json(CONFIG / "units.json")["units"]
    checked = bad = 0
    for unit, ent in units.items():
        if not (BUILD / "verify" / f"{unit}.obj").exists():
            continue
        for p in az.object_pairs(unit_obj(unit), all_funcs=True):
            if p.status != "STRICT":
                continue
            checked += 1
            r = metrics(p.orig, p.cand)
            if any(r[k] != 1.0 for k in ("content", "fuzzy", "skeleton", "layout", "regs")) or \
                    p.cand_size != p.orig_size:
                bad += 1
                print(f"FAIL {unit} {p.name}: {r} sizes {p.cand_size}/{p.orig_size}")
    print(f"selftest: {checked} STRICT functions, {bad} not at 1.0")
    return 1 if bad else 0


def main(argv):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("args", nargs="*")
    ap.add_argument("--obj")
    ap.add_argument("--src")
    ap.add_argument("--profile", default="game_oe")
    ap.add_argument("--all", action="store_true")
    ap.add_argument("--worklist", action="store_true")
    ap.add_argument("--census", action="store_true")
    ap.add_argument("--selftest", action="store_true")
    ap.add_argument("--json")
    includes = [a[3:] for a in argv[1:] if a.startswith("-i=")]
    a = ap.parse_args([x for x in argv[1:] if not x.startswith("-i=")])
    az = Analyzer()
    if a.selftest:
        return selftest(az)
    if a.census:
        return census(az, a.json)
    names = list(a.args)
    if a.obj:
        obj = Path(a.obj)
    elif a.src:
        obj = compile_src(a.src, a.profile, includes)
    else:
        if not names:
            ap.error("UNIT, --obj or --src required")
        obj = unit_obj(names.pop(0))
    pairs = az.object_pairs(obj, set(names) or None, a.all)
    if a.worklist:
        if len(pairs) != 1:
            raise SystemExit("--worklist takes exactly one function name")
        worklist(az, pairs[0])
        return 0
    print_rows(pairs)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
