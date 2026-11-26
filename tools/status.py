"""Progress metrics with a fixed denominator (obj1 code bytes and candidate function starts).

    python tools/status.py

Sources: build/map/funcmap.json (tools/funcmap.py), build/verify.json (tools/verify.py), and library identity scans
listed in LIB_SCANS (tools/libscan.py --json). Library identity, historical-object identity and reconstructed source
are separate columns; bytes are deduplicated by address so overlapping hits never count twice.
"""
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from common import BUILD, oracle_le  # noqa: E402

LIB_SCANS = {"watcom_runtime": ["build/scan/clib95b.json"],
             "apogee_sound_system": ["build/scan/ass104.json"]}


def covered(paths):
    mark = set()
    for p in paths:
        p = BUILD.parent / p
        if not p.exists():
            continue
        for r in json.load(open(p, encoding="utf-8")):
            if r["class"] == "exact+sites":
                mark.update(range(r["at"], r["at"] + r["size"]))
    return mark


def main():
    le = oracle_le()
    total = le.obj(1).vsize
    fm = json.load(open(BUILD / "map" / "funcmap.json"))
    nfunc = len(fm["functions"])
    print(f"obj1 code object: {total} bytes, {nfunc} candidate function starts (denominator is fixed; see funcmap)")
    for k, paths in LIB_SCANS.items():
        c = covered(paths)
        print(f"  identified {k:22s} {len(c):7d} bytes ({100*len(c)/total:5.1f}%)  [identity only, not reconstruction]")
    v = BUILD / "verify.json"
    if v.exists():
        t = json.load(open(v))["totals"]
        print(f"  reconstructed STRICT C/asm       {t['strict_bytes']:7d} bytes ({100*t['strict_bytes']/total:5.1f}%), "
              f"{t['strict']} functions; CODE_BYTES_MATCH {t['code_bytes_match']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
