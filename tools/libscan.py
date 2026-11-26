"""Locate OMF modules (library members or historical .OBJ code segments) inside the oracle LE code object.

Adapted from rayman_recon/tools/libscan.py (GPLv2) for the LE container.

For every CODE segment of every module, search obj1 for the segment bytes with the module's fixup fields masked.
Each hit is classified:
  exact+sites   every unmasked byte equal, every absolute 32-bit module fixup coincides with an oracle fixup site,
                the oracle has no extra fixup inside the extent, and every fixup whose target is resolvable inside
                the same placement (own segment, absolute or self-relative) points at the right oracle address;
  masked        bytes equal modulo fixup fields, but sites or internal targets disagree.
Hits are identity evidence only (LIBRARY_MEMBER / HISTORICAL_OBJECT), never reconstruction credit.

    python tools/libscan.py LIB_OR_OBJ [...]                   print hits sorted by address
    python tools/libscan.py --json OUT.json LIB_OR_OBJ [...]   also write the hits
    python tools/libscan.py --compare A.LIB B.LIB              members found with only one release
    python tools/libscan.py --split ...                        compare each public's extent separately
    python tools/libscan.py --min N ...                        ignore segments smaller than N bytes (default 8)
"""
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import omf  # noqa: E402
from common import dump_json, oracle_le  # noqa: E402


def masked_pattern(mod, si):
    seg = mod.segments[si]
    mask = bytearray(seg.covered) if len(seg.covered) == seg.size else bytearray(b"\x01" * seg.size)
    fx = []
    for f in mod.fixups:
        if f.seg == si:
            fx.append(f)
            for k in range(f.size()):
                if f.off + k < len(mask):
                    mask[f.off + k] = 0
    return bytes(seg.data[:seg.size]), bytes(mask), fx


def find_masked(hay, pat, mask, min_anchor=8):
    n = len(pat)
    best, cur, start = (0, 0), 0, 0
    for i, m in enumerate(mask):
        if m:
            if cur == 0:
                start = i
            cur += 1
            if cur > best[1]:
                best = (start, cur)
        else:
            cur = 0
    a0, alen = best
    if alen < min_anchor:
        return []
    anchor = pat[a0:a0 + alen]
    hits, p = [], hay.find(anchor)
    while p != -1:
        base = p - a0
        if base >= 0 and base + n <= len(hay) and all(not mask[i] or hay[base + i] == pat[i] for i in range(n)):
            hits.append(base)
        p = hay.find(anchor, p + 1)
    return hits


def classify(le, fmap, mod, si, base, fx, a=0, b=None):
    """Return (class, notes) for a placement of mod segment si at obj1:base, judged over extent [a, b)."""
    o1 = le.obj(1)
    if b is None:
        b = mod.segments[si].size
    notes = []
    want_sites = set()
    for f in fx:
        absolute = not f.self_rel and f.size() == 4
        if absolute:
            want_sites.add(base + f.off)
        m, d = f.target
        if (m & 3) == 0 and d == si:                       # target = own segment (+disp)
            disp = f.disp
            if m & 4 == 0 and f.disp == 0:                  # displacement may live in the field itself
                disp = struct.unpack_from("<i", mod.segments[si].data, f.off)[0] if f.size() == 4 else 0
            tgt = base + disp
            if not (a <= disp < b):
                continue                                    # target outside the compared piece: unverifiable here
            if absolute:
                ofx = fmap.get((1, base + f.off))
                if not ofx or ofx.target_obj != 1 or ofx.target_off != tgt:
                    notes.append(f"internal abs target mismatch at +{f.off:x}")
            elif f.self_rel and f.size() == 4:
                got = struct.unpack_from("<i", o1.data, base + f.off)[0] + base + f.off + 4
                if got != tgt:
                    notes.append(f"internal rel target mismatch at +{f.off:x}")
    have_sites = {k[1] for k in fmap if k[0] == 1 and base + a <= k[1] < base + b}
    if want_sites != have_sites:
        notes.append(f"sites differ: module {len(want_sites)} oracle {len(have_sites)}")
    return ("exact+sites" if not notes else "masked"), notes


def pieces(m, si, split):
    """(start, end, names) extents: the whole segment, or one per public when split."""
    seg = m.segments[si]
    if not split:
        return [(0, seg.size, [n for n, s, o, _ in m.publics if s == si])]
    offs = sorted({o for n, s, o, _ in m.publics if s == si} | {0})
    out = []
    for i, a in enumerate(offs):
        b = offs[i + 1] if i + 1 < len(offs) else seg.size
        out.append((a, b, [n for n, s, o, _ in m.publics if s == si and o == a]))
    return out


def sub_module_view(m, si, a, b):
    """Pattern, mask and fixups restricted to [a, b) of segment si, rebased to 0."""
    pat, mask, fx = masked_pattern(m, si)
    sub = [f for f in fx if a <= f.off < b]
    return pat[a:b], mask[a:b], sub


def scan(paths, min_size=8, split=False):
    le = oracle_le()
    fmap = le.fixup_map()
    code = bytes(le.obj(1).data)
    out = []
    for path in paths:
        for m in omf.load(path):
            for si, seg in enumerate(m.segments):
                if not seg or seg.cls.upper() != "CODE" or seg.size < min_size:
                    continue
                for a, b, names in pieces(m, si, split):
                    if b - a < min_size:
                        continue
                    pat, mask, fx = sub_module_view(m, si, a, b)
                    for hit in find_masked(code, pat, mask):
                        base = hit - a                      # placement of the whole segment
                        cls, notes = classify(le, fmap, m, si, base, fx, a, b)
                        out.append({"lib": str(path), "module": m.name, "seg": seg.name, "size": b - a, "at": hit,
                                    "class": cls, "notes": notes,
                                    "publics": {n: hit for n in names} if split else
                                    {n: base + o for n, s, o, _ in m.publics if s == si}})
    return out


def main(argv):
    argv = list(argv[1:])
    jout, min_size = None, 8
    if "--json" in argv:
        i = argv.index("--json"); jout = argv[i + 1]; del argv[i:i + 2]
    split = "--split" in argv
    if split:
        argv.remove("--split")
    if "--min" in argv:
        i = argv.index("--min"); min_size = int(argv[i + 1]); del argv[i:i + 2]
    if argv and argv[0] == "--compare":
        a, b = argv[1], argv[2]
        ra, rb = scan([a], min_size, split), scan([b], min_size, split)
        ka = {(r["module"], r["at"], r["class"]) for r in ra}
        kb = {(r["module"], r["at"], r["class"]) for r in rb}
        print(f"A {a}: {len(ka)} hits, {sum(1 for r in ra if r['class']=='exact+sites')} exact+sites, "
              f"{sum(r['size'] for r in ra if r['class']=='exact+sites')} bytes")
        print(f"B {b}: {len(kb)} hits, {sum(1 for r in rb if r['class']=='exact+sites')} exact+sites, "
              f"{sum(r['size'] for r in rb if r['class']=='exact+sites')} bytes")
        for k in sorted(ka - kb, key=lambda x: x[1]):
            print(f"  only A: 1:{k[1]:05x} {k[0]} {k[2]}")
        for k in sorted(kb - ka, key=lambda x: x[1]):
            print(f"  only B: 1:{k[1]:05x} {k[0]} {k[2]}")
        return 0
    res = scan(argv, min_size, split)
    for r in sorted(res, key=lambda r: r["at"]):
        print(f"1:{r['at']:05x} +{r['size']:<5x} {r['class']:11s} {Path(r['lib']).name}:{r['module']} "
              f"{' '.join(list(r['publics'])[:3])} {'; '.join(r['notes'][:2])}")
    ex = [r for r in res if r["class"] == "exact+sites"]
    print(f"# {len(res)} hits ({sum(r['size'] for r in res)} bytes), {len(ex)} exact+sites ({sum(r['size'] for r in ex)} bytes)")
    if jout:
        dump_json(jout, res)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
