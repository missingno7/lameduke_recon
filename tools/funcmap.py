"""Recursive-descent code discovery and function-boundary census for oracle obj1.

    python tools/funcmap.py [--out build/map/funcmap.json]

Seeds: the LE entry point, every fixup target inside obj1 (code pointers in code and data: function pointers,
jump-table entries), and call targets found while decoding.  Function starts are: entry, rel32 call targets,
and obj1 addresses referenced from data (obj2 -> obj1 fixups) or by `mov reg, offset` style absolute references
outside jump tables.  Jump-table entries are code labels, not function starts.

Each function extent is [start, next start); the census reports reached bytes, unreached gaps, and padding.
This is a candidate model: boundaries are confirmed only by a strict match or an OMF public.
"""
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from capstone import CS_ARCH_X86, CS_MODE_32, Cs  # noqa: E402
from capstone import x86_const as X  # noqa: E402

from common import BUILD, dump_json, oracle_le  # noqa: E402


def analyse():
    le = oracle_le()
    o1 = le.obj(1)
    code = bytes(o1.data)
    fmap = le.fixup_map()
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    md.detail = True

    fix_sites = {off for (ob, off) in fmap if ob == 1}
    # bytes that are fixup fields in obj1: jump tables are runs of 4-byte abs fixups to obj1
    jt_entries = set()
    for (ob, off), fx in fmap.items():
        if ob == 1 and fx.target_obj == 1:
            if (off + 4) in fix_sites or (off - 4) in fix_sites:
                if fmap.get((1, off + 4), fx).target_obj == 1 or fmap.get((1, off - 4), fx).target_obj == 1:
                    jt_entries.add(off)
    starts = {le.hdr["eip"]}
    data_refs = set()
    for (ob, off), fx in fmap.items():
        if fx.target_obj == 1 and ob == 2:
            starts.add(fx.target_off)
            data_refs.add(fx.target_off)
    labels = {fmap[(1, o)].target_off for o in jt_entries}
    other_code_refs = {fx.target_off for (ob, off), fx in fmap.items()
                       if ob == 1 and fx.target_obj == 1 and off not in jt_entries}
    starts |= other_code_refs

    reached = bytearray(len(code))
    insn_at = {}
    work = list(starts | labels)
    calls = set()
    seen = set()
    while work:
        a = work.pop()
        if a in seen or not (0 <= a < len(code)):
            continue
        while 0 <= a < len(code):
            if a in seen:
                break
            seen.add(a)
            if any(s in jt_entries for s in range(a, a + 1)):
                break
            ins = next(md.disasm(code[a:a + 16], a), None)
            if ins is None:
                break
            insn_at[a] = ins.size
            for k in range(ins.size):
                reached[a + k] = 1
            g = set(ins.groups)
            if X.X86_GRP_CALL in g or X.X86_GRP_JUMP in g:
                op = ins.operands[0] if ins.operands else None
                if op is not None and op.type == X.X86_OP_IMM:
                    t = op.imm
                    if X.X86_GRP_CALL in g:
                        calls.add(t)
                    work.append(t)
            if X.X86_GRP_RET in g or ins.mnemonic in ("jmp", "hlt") or ins.id == X.X86_INS_IRETD:
                break
            a += ins.size
    starts |= calls
    starts = sorted(s for s in starts if 0 <= s < len(code))
    funcs = []
    for i, s in enumerate(starts):
        e = starts[i + 1] if i + 1 < len(starts) else len(code)
        funcs.append({"start": s, "end": e, "size": e - s,
                      "reached": sum(reached[s:e]),
                      "why": [w for w, ok in (("entry", s == le.hdr["eip"]), ("call", s in calls),
                                              ("dataref", s in data_refs), ("coderef", s in other_code_refs)) if ok]})
    unreached = []
    i = 0
    while i < len(code):
        if not reached[i] and i not in jt_entries:
            j = i
            while j < len(code) and not reached[j]:
                j += 1
            unreached.append((i, j))
            i = j
        else:
            i += 1
    return le, funcs, reached, unreached, jt_entries


def main(argv):
    out = BUILD / "map" / "funcmap.json"
    if "--out" in argv:
        out = Path(argv[argv.index("--out") + 1])
    le, funcs, reached, unreached, jt = analyse()
    total = len(le.obj(1).data)
    print(f"obj1 {total} bytes; {len(funcs)} candidate function starts; reached {sum(reached)} bytes; "
          f"jump-table entries {len(jt)} ({4*len(jt)} bytes)")
    gap = [(a, b) for a, b in unreached if b - a >= 1]
    zero = sum(b - a for a, b in gap if not any(le.obj(1).data[a:b]))
    print(f"unreached runs {len(gap)} totalling {sum(b-a for a,b in gap)} bytes ({zero} all-zero)")
    dump_json(out, {"functions": funcs, "unreached": gap, "jump_table_entries": sorted(jt)})
    print(f"-> {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
