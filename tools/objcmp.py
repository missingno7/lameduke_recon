"""Compare two OMF objects segment by segment (bytes and fixups) and per public function.

    python tools/objcmp.py A.OBJ B.OBJ [--funcs]

Reports, for each segment present in both, whether bytes and normalized fixups (offset, location, self-relative,
target name, displacement) agree, and with --funcs the per-public verdicts for CODE segments matched by name.
Used for compiler calibration (historical source + historical object pairs).
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import omf  # noqa: E402


def norm_fixups(m, si):
    out = []
    for f in m.fixups:
        if f.seg == si:
            out.append((f.off, f.loc, f.self_rel, m.target_name(f), f.disp))
    return sorted(out)


def funcs(m, si):
    seg = m.segments[si]
    pubs = sorted((o, n) for n, s, o, _ in m.publics if s == si)
    out = {}
    for i, (o, n) in enumerate(pubs):
        e = pubs[i + 1][0] if i + 1 < len(pubs) else seg.size
        out[n] = (o, e)
    return out


def compare(a, b, show_funcs=False, quiet=False):
    ma, mb = omf.load(a)[0], omf.load(b)[0]
    segs_b = {s.name: (i, s) for i, s in enumerate(mb.segments) if s}
    total = {"funcs": 0, "same": 0, "bytes": 0, "same_bytes": 0}
    for ia, sa in enumerate(ma.segments):
        if not sa:
            continue
        if sa.name not in segs_b:
            print(f"{sa.name}: only in A")
            continue
        ib, sb = segs_b[sa.name]
        da, db = bytes(sa.data[:sa.size]), bytes(sb.data[:sb.size])
        fa, fb = norm_fixups(ma, ia), norm_fixups(mb, ib)
        same = da == db and fa == fb
        first = next((i for i in range(min(len(da), len(db))) if da[i] != db[i]), None)
        if not quiet:
            print(f"{sa.name:8s} size {sa.size:6d}/{sb.size:6d} bytes {'==' if da == db else '!='} "
                  f"fixups {'==' if fa == fb else '!='} ({len(fa)}/{len(fb)}) first-byte-diff {first}")
        if sa.cls.upper() == "CODE" and show_funcs:
            fna, fnb = funcs(ma, ia), funcs(mb, ib)
            for n, (s, e) in fna.items():
                if n not in fnb:
                    continue
                s2, e2 = fnb[n]
                ok = da[s:e] == db[s2:e2] and \
                    [(o - s, *r) for o, *r in fa if s <= o < e] == [(o - s2, *r) for o, *r in fb if s2 <= o < e2]
                total["funcs"] += 1
                total["bytes"] += e - s
                if ok:
                    total["same"] += 1
                    total["same_bytes"] += e - s
                elif not quiet:
                    print(f"   differs: {n} ({e-s} vs {e2-s2})")
    if show_funcs:
        print(f"functions identical: {total['same']}/{total['funcs']}, bytes {total['same_bytes']}/{total['bytes']}")
    return total


if __name__ == "__main__":
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    compare(args[0], args[1], "--funcs" in sys.argv, "--quiet" in sys.argv)
