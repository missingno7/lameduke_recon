"""Apply a section of the semantic rename plan (docs/RENAMES.md) atomically and safely.

    python tools/rename.py SECTION [--dry-run] [--files GLOB...]
    SECTION: a heading prefix such as "0.1" or "Priority 1" or "Priority 2"

Reads the Markdown table(s) under the section: columns 'current' and 'proposed' (bold marks stripped), optional
'conf' (only PROVEN*/STRONG rows are applied) and optional '_BSS verdict' (only NEUTRAL rows are applied).
Renames are whole-word, done in two phases through placeholders so swaps (svel<->vel) work, across:
  src/game/*.[ch], src/global/*.c, config/symbols.json keys, config/units.json function records,
  tools/link/game.lnk.
A rename is refused when its new name already occurs as a word anywhere in those sources and is not itself being
renamed away (collision). Run `python tools/gate.py` afterwards: renames must not change any bytes.
Also usable on worker outputs: --files build/workers/x/src/*.c applies the same plan to other files.
"""
import glob
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PLAN = ROOT / "docs" / "RENAMES.md"


def parse(section):
    lines = PLAN.read_text(encoding="utf-8").splitlines()
    out, active, header = [], False, None
    for ln in lines:
        if ln.startswith("#"):
            title = ln.lstrip("#").strip()
            active = title.startswith(section)
            header = None
            continue
        if not active or not ln.startswith("|"):
            continue
        cells = [c.strip() for c in ln.strip().strip("|").split("|")]
        if header is None:
            header = [c.lower() for c in cells]
            continue
        if set(cells[0]) <= set("-: "):
            continue
        row = dict(zip(header, cells))
        cur = row.get("current", "").strip("`* ")
        new = row.get("proposed", "").strip("`* ")
        if not cur or not new or not re.fullmatch(r"[A-Za-z_]\w*", cur) or not re.fullmatch(r"[A-Za-z_]\w*", new):
            continue
        conf = row.get("conf", "STRONG").upper()
        if not (conf.startswith("PROVEN") or conf.startswith("STRONG")):
            continue
        verdict = next((v for k, v in row.items() if "bss" in k), "NEUTRAL")
        if "NEUTRAL" not in verdict.upper():
            continue
        out.append((cur, new))
    return out


def main(argv):
    args = argv[1:]
    dry = "--dry-run" in args
    files = []
    if "--files" in args:
        i = args.index("--files")
        for g in args[i + 1:]:
            files += glob.glob(g)
        args = args[:i]
    section = " ".join(a for a in args if not a.startswith("--"))
    plan = parse(section)
    if not files:
        files = glob.glob(str(ROOT / "src/game/*.[ch]")) + glob.glob(str(ROOT / "src/global/*.c"))
        extra = [ROOT / "tools/link/game.lnk"]
        registries = True
    else:
        extra, registries = [], False
    texts = {f: Path(f).read_bytes().decode("latin-1") for f in files}
    alltext = "\n".join(texts.values())
    olds = {c for c, _ in plan}
    applied, refused = [], []
    for cur, new in plan:
        if not re.search(r"\b" + re.escape(cur) + r"\b", alltext):
            refused.append((cur, new, "old name not found"))
            continue
        if new not in olds and re.search(r"\b" + re.escape(new) + r"\b", alltext):
            refused.append((cur, new, "collision: new name already used"))
            continue
        applied.append((cur, new))
    tmp = {cur: f"__REN{i}__" for i, (cur, _) in enumerate(applied)}

    def rewrite(s):
        for cur, _ in applied:
            s = re.sub(r"\b" + re.escape(cur) + r"\b", tmp[cur], s)
        for cur, new in applied:
            s = s.replace(tmp[cur], new)
        return s
    if not dry:
        for f, s in texts.items():
            n = rewrite(s)
            if n != s:
                Path(f).write_bytes(n.encode("latin-1"))
        for p in extra:
            s = p.read_text(encoding="latin-1")
            n = re.sub(r"\b(\w+?)_\b", lambda m: m.group(0), s)
            for cur, new in applied:      # link aliases use Watcom-decorated names (name_ / _name)
                n = re.sub(r"\b" + re.escape(cur) + r"_\b", new + "_", n)
            p.write_text(n, encoding="latin-1", newline="")
        if registries:
            sp = ROOT / "config/symbols.json"
            sym = json.load(open(sp, encoding="utf-8"))
            for kind in ("functions", "data"):
                d = sym["d3d"][kind]
                staged = {}
                for cur, new in applied:
                    if cur in d:
                        staged[new] = d.pop(cur)
                        staged[new]["evidence"] = staged[new].get("evidence", "") + f" | renamed from {cur} (docs/RENAMES.md)"
                d.update(staged)
            json.dump(sym, open(sp, "w", encoding="utf-8", newline="\n"), indent=1)
            up = ROOT / "config/units.json"
            units = json.load(open(up, encoding="utf-8"))
            for u in units["units"].values():
                fs = u.get("functions", {})
                staged = {new: fs.pop(cur) for cur, new in applied if cur in fs}
                fs.update(staged)
            json.dump(units, open(up, "w", encoding="utf-8", newline="\n"), indent=1)
    print(f"section {section!r}: {len(plan)} candidate rows, {len(applied)} applied{' (dry run)' if dry else ''}, {len(refused)} refused")
    for cur, new in applied:
        print(f"  {cur} -> {new}")
    for cur, new, why in refused:
        print(f"  REFUSED {cur} -> {new}: {why}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
