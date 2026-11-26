"""Regenerate the progress section of README.md from build/verify.json (run after tools/verify.py --update).

    python tools/readme.py

Denominators are fixed oracle ranges of the code object (docs/STATUS.md code map), never candidate sizes, so a bar
can only grow by proving more bytes. Only STRICT functions count; library code identified by signature is shown
separately and never counts as reconstructed.
"""
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
README = ROOT / "README.md"
BEGIN, END = "<!-- progress:begin -->", "<!-- progress:end -->"

# component -> (label, oracle obj1 byte count from the code map, unit-name predicate)
COMPONENTS = [
    ("game", "Game code (GAME, PREMAP, SECTOR, PLAYER, ACTORS, SOUNDS, MENUES, GAMEDEF)", 0x33e5c - 0x10,
     lambda u: u.startswith("game_") or u == "global"),
    ("engine", "Build engine (ENGINE.C + A.ASM)", (0x47abf - 0x33e5c) + (0x6c445 - 0x6b685),
     lambda u: u in ("engine", "a_asm")),
    ("multi", "Multiplayer (MULTI.C)", 0x49c84 - 0x47abf, lambda u: u.startswith("multi")),
    ("apogee", "Apogee Sound System", 0x62d8d - 0x55b30, lambda u: u.startswith("ass_")),
]
OBJ1 = 0x6c445
LIBRARY = [("Watcom 9.5 C runtime (CLIB3R/MATH387R/EMU387)", (0x6b685 - 0x62d8d) + 0x10),
           ("Creative AWE32 + Gravis GF1 vendor objects", 0x55b30 - 0x49c84)]


def bar(done, total, width=30):
    frac = done / total if total else 0.0
    n = round(frac * width)
    return f"`{'█' * n}{'░' * (width - n)}` {100 * frac:5.1f}%"


def main():
    v = json.load(open(ROOT / "build/verify.json", encoding="utf-8"))
    stats = {}
    for unit, fs in v["units"].items():
        comp = next((c for c, _l, _t, pred in COMPONENTS if pred(unit)), None)
        if comp is None:
            continue
        s = stats.setdefault(comp, {"funcs": 0, "strict": 0, "bytes": 0})
        for r in fs.values():
            if not isinstance(r, dict) or "status" not in r:
                continue
            s["funcs"] += 1
            if r["status"] == "STRICT":
                s["strict"] += 1
                s["bytes"] += r["size"]
    t = v["totals"]
    own = sum(total for _c, _l, total, _p in COMPONENTS)
    strict = sum(s["bytes"] for s in stats.values())
    lines = [BEGIN, "", "| Part | Byte-identical (STRICT) | Functions | Bytes |", "|---|---|---:|---:|"]
    for comp, label, total, _p in COMPONENTS:
        s = stats.get(comp, {"funcs": 0, "strict": 0, "bytes": 0})
        lines.append(f"| {label} | {bar(s['bytes'], total)} | {s['strict']} / {s['funcs']} | "
                     f"{s['bytes']:,} / {total:,} |")
    lines.append(f"| **Whole program (all non-library code)** | {bar(strict, own)} | "
                 f"{sum(s['strict'] for s in stats.values())} / {sum(s['funcs'] for s in stats.values())} | "
                 f"{strict:,} / {own:,} |")
    lines.append("")
    lines.append(f"- Initialised data proven at its original address (CONST/_DATA): {bar(t.get('verified_data', 0), 0xb000)}"
                 f" ({t.get('verified_data', 0):,} of {0xb000:,} file-backed bytes).")
    lines.append(f"- Uninitialised data (_BSS) whose layout is proven: {t.get('verified_bss', 0):,} bytes.")
    lines.append(f"- Functions whose code bytes match but a referenced target is not yet independently proven: "
                 f"{t.get('code_bytes_match', 0)}.")
    lib = ", ".join(f"{n} ({b:,} bytes)" for n, b in LIBRARY)
    lines.append(f"- Identified library code, not counted as reconstruction: {lib}.")
    lines.append(f"- Every compiled object of the program already has candidate source; the code object is "
                 f"{OBJ1:,} bytes in total.")
    bd = ROOT / "build/blockdiff.json"
    if bd.exists():
        s = json.load(open(bd, encoding="utf-8"))["summary"]
        lines += ["", f"Code that does not match yet ({s['functions']} functions, {s['bytes']:,} bytes), by how much "
                  "of each function's basic blocks already have the original's content (`tools/blockdiff.py`; a "
                  "diagnostic, never counted as matched):", ""]
        for cat, b in sorted(s["categories"].items(), key=lambda x: -x[1]):
            lines.append(f"- {cat}: {b:,} bytes ({100 * b / s['bytes']:.1f}%)")
        lines.append(f"- byte-weighted share of blocks already right: {100 * s['content_weighted']:.1f}% exact, "
                     f"{100 * s['fuzzy_weighted']:.1f}% counting near-identical blocks")
    lines += ["", END]
    text = README.read_text(encoding="utf-8")
    a, b = text.index(BEGIN), text.index(END) + len(END)
    README.write_text(text[:a] + "\n".join(lines) + text[b:], encoding="utf-8", newline="\n")
    status_table(v)
    print(f"README.md and docs/STATUS.md updated: {strict:,} / {own:,} STRICT bytes")


def status_table(v):
    """Rewrite the per-unit table and totals line of docs/STATUS.md from the same verify.json."""
    rows = []
    for unit, fs in v["units"].items():
        fs = {k: r for k, r in fs.items() if isinstance(r, dict) and "status" in r}
        if not fs:
            continue
        s = [r for r in fs.values() if r["status"] == "STRICT"]
        rows.append(f"| {unit} | {len(fs)} | {len(s)} | {sum(r['size'] for r in s):,} |")
    t = v["totals"]
    c = t["by_component"]
    tot = (f"Totals: {t['strict']} STRICT / {t['functions']} functions, {t['strict_bytes']:,} bytes; LameDuke "
           f"{c['lameduke']['strict']} STRICT ({c['lameduke']['strict_bytes']:,} bytes); Apogee Sound System "
           f"{c['apogee_sound_system']['strict']} ({c['apogee_sound_system']['strict_bytes']:,} bytes); "
           f"CODE_BYTES_MATCH {t['code_bytes_match']}; verified data {t['verified_data']:,} bytes, verified _BSS "
           f"layout {t['verified_bss']:,} bytes.")
    p = ROOT / "docs/STATUS.md"
    s = p.read_text(encoding="utf-8")
    a, z = s.index("| Unit |"), s.index("Totals:")
    z2 = s.index("\n", z)
    s = s[:a] + "| Unit | Functions | STRICT | STRICT bytes |\n|---|---:|---:|---:|\n" + "\n".join(rows) + \
        "\n\n" + tot + s[z2:]
    p.write_text(s, encoding="utf-8", newline="\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
