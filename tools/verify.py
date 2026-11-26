"""Zero-regression gate: rebuild every registered unit fresh and re-prove its accepted function statuses.

    python tools/verify.py [UNIT...] [--update]

config/units.json:
  {"units": {NAME: {"source": "src/...c", "profile": "engine", "include": ["src/engine"],
                    "provenance": "...", "functions": {FUNC: {"status": "STRICT", "quality": "NATURAL"}}}}}
A function regresses when its fresh status ranks below the recorded one (STRICT > CODE_BYTES_MATCH > NEAR_MATCH >
UNPLACED). --update records improvements (never lowers a status). Exit code 1 on any regression or build failure.
Writes build/verify.json with per-unit results and STRICT byte totals.
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import match  # noqa: E402
import omf  # noqa: E402
import wcc  # noqa: E402
from common import BUILD, CONFIG, ROOT, dump_json, load_json  # noqa: E402
from image import Image  # noqa: E402

RANK = {"STRICT": 3, "CODE_BYTES_MATCH": 2, "NEAR_MATCH": 1, "UNPLACED": 0, None: 0}


def build_unit(name, ent):
    src = ROOT / ent["source"]
    prof = wcc.profile(ent["profile"])
    out = BUILD / "verify" / f"{name}.obj"
    out.parent.mkdir(parents=True, exist_ok=True)
    inc = [ROOT / p for p in ent.get("include", [])] + [src.parent]
    rc, text = wcc.compile_c(src, out, list(prof["flags"]) + ent.get("extra_flags", []), prof["install"], inc)
    if rc:
        raise RuntimeError(f"{name}: build failed\n{text}")
    return out


def main(argv):
    update = "--update" in argv
    only = [a for a in argv[1:] if not a.startswith("--")]
    units = load_json(CONFIG / "units.json")
    img = Image()
    report, regressions, totals = {}, [], {"functions": 0, "strict": 0, "strict_bytes": 0, "code_bytes_match": 0}
    for name, ent in units["units"].items():
        if only and name not in only:
            continue
        try:
            obj = build_unit(name, ent)
        except RuntimeError as e:
            print(e)
            regressions.append(f"{name}: build failed")
            continue
        results = []
        for m in omf.load(obj):
            results += match.verify_object(img, m)
        got = {r["function"]: r for r in results}
        rec = ent.setdefault("functions", {})
        for fn, want in rec.items():
            r = got.get(fn)
            st = r["status"] if r else None
            if RANK[st] < RANK[want["status"]]:
                regressions.append(f"{name}:{fn} {want['status']} -> {st}")
        for fn, r in got.items():
            totals["functions"] += 1
            if r["status"] == "STRICT":
                totals["strict"] += 1
                totals["strict_bytes"] += r["size"]
            elif r["status"] == "CODE_BYTES_MATCH":
                totals["code_bytes_match"] += 1
            if update and RANK[r["status"]] > RANK[rec.get(fn, {}).get("status")]:
                rec.setdefault(fn, {})["status"] = r["status"]
        report[name] = {fn: {"status": r["status"], "size": r["size"], "oracle": r.get("oracle")} for fn, r in got.items()}
        print(f"{name:20s} " + " ".join(f"{k}={sum(1 for r in got.values() if r['status'] == k)}"
                                        for k in ("STRICT", "CODE_BYTES_MATCH", "NEAR_MATCH", "UNPLACED")))
    dump_json(BUILD / "verify.json", {"totals": totals, "units": report, "regressions": regressions})
    if update and not regressions:
        from common import dump_json as dj
        dj(CONFIG / "units.json", units)
    print(f"# totals {totals}")
    for r in regressions:
        print(f"REGRESSION {r}")
    return 1 if regressions else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
