"""One-stop donor lookup for a LameDuke function: check what history already holds before (re)writing it.

    python tools/donors.py FUNC [FUNC ...] [--top N]

Prints, per function:
  - current status and size (build/verify.json) and block closeness (build/blockdiff.json, if present);
  - SOURCE donors: historical C compiled with LameDuke's own Watcom 9.5 (tools/xscan, config/donors.json):
    file, function, score (same compiler, so a low score means the source really differed);
  - BINARY donors: dated objects/executables (tools/binscan, build/binscan/out/top_donors.json), incl. whether a
    relocation-aware identical copy exists;
  - the Ken-Build GAME.C ancestry rows (docs/ANCESTRY.md) and Duke 1.5 / raynorpat hits by name.
Diagnostic only; nothing here is matching evidence. AGENTS.md: run this before rewriting a function.
"""
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def load(path):
    p = ROOT / path
    return json.load(open(p, encoding="utf-8")) if p.exists() else None


def grep_name(path, name, limit=3):
    out = []
    root = ROOT / path
    if not root.exists():
        return out
    pat = re.compile(rb"^\s*[\w\s\*]*\b" + re.escape(name.encode()) + rb"\s*\(", re.I | re.M)
    for f in sorted(root.rglob("*")):
        if f.suffix.lower() not in (".c", ".cpp", ".h") or not f.is_file():
            continue
        try:
            data = f.read_bytes()
        except OSError:
            continue
        m = pat.search(data)
        if m:
            line = data.count(b"\n", 0, m.start()) + 1
            out.append(f"{f.relative_to(ROOT).as_posix()}:{line}")
            if len(out) >= limit:
                break
    return out


def main(argv):
    top = 3
    if "--top" in argv:
        i = argv.index("--top")
        top = int(argv[i + 1])
        del argv[i:i + 2]
    names = argv[1:]
    if not names:
        print(__doc__)
        return 1
    verify = load("build/verify.json") or {"units": {}}
    status = {n: (u, r) for u, fs in verify["units"].items() for n, r in fs.items() if isinstance(r, dict)}
    bd = load("build/blockdiff.json")
    bdrows = {r["function"]: r for r in (bd or {}).get("functions", [])}
    xs = load("config/donors.json")
    xtargets = {t["function"]: t for t in (xs or {}).get("targets", [])}
    bs = load("build/binscan/out/top_donors.json")
    btargets = {}
    for t in (bs or {}).get("targets", []):
        if isinstance(t, dict) and "name" in t:
            btargets.setdefault(t["name"], t)
    ancestry = (ROOT / "docs/ANCESTRY.md").read_text(encoding="utf-8") if (ROOT / "docs/ANCESTRY.md").exists() else ""
    for name in names:
        print(f"== {name}")
        if name in status:
            u, r = status[name]
            print(f"   status {r['status']} in {u}, {r.get('size')} B at {r.get('oracle')}")
        if name in bdrows:
            r = bdrows[name]
            print(f"   blocks: content {r['content']:.2f} fuzzy {r['fuzzy']:.2f} regs {r['regs']:.2f} ({r['category']})")
        t = xtargets.get(name)
        if t:
            print("   source donors (compiled with LameDuke's Watcom 9.5):")
            for d in sorted(t.get("donors", []), key=lambda x: -x["score"])[:top]:
                print(f"     {d['score']:.3f} {d['file']} :: {d['function']} [{d.get('profile')}]"
                      + ("  same name" if d.get("same_name") else ""))
        else:
            print("   source donors: not scanned (function was STRICT at scan time or outside the game/engine units)")
        t = btargets.get(name)
        if t:
            print("   binary donors (dated objects/executables):")
            for d in t.get("donors", [])[:top]:
                rep = d.get("representative") or {}
                where = rep.get("path", "?")
                date = rep.get("first_date") or {}
                date = date.get("timestamp", "")[:10] if isinstance(date, dict) else ""
                names_ = ",".join(d.get("public_names", [])[:2])
                flag = " IDENTICAL(reloc-aware)" if d.get("relocation_aware_bytes") else ""
                print(f"     {d['score']:.3f} {where} {names_} {date}{flag}")
        rows = [ln for ln in ancestry.splitlines() if ln.startswith("|") and re.search(rf"\b{re.escape(name)}\b", ln)]
        for ln in rows[:top]:
            print(f"   ancestry: {ln[:200]}")
        for label, path in (("Duke 1.5", "external/duke3d-1.3d/dukenukem3d/source"),
                            ("Duke extras", "external/duke3d-1.3d/dukenukem3d/extras"),
                            ("raynorpat", "external/lameduke-raynorpat")):
            hits = grep_name(path, name)
            if hits:
                print(f"   {label}: " + ", ".join(hits))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
