"""Sequential, verifier-scored climb from a DOS-controlled CG95 trace."""
import argparse
from itertools import combinations
import hashlib
import json
from pathlib import Path
import tempfile

import flip95 as flip
from flip95 import cg


def score(row):
    return (len(row["problems"]), len(row["unverified"]), row.get("byte_diffs", 0))


def measures(rows):
    return {r["function"]: score(r) for r in rows}


def regressions(before, after, target):
    a, b = measures(before), measures(after)
    return [n for n in a if n != target and
            (n not in b or any(x > y for x, y in zip(b[n], a[n])))]


def opportunities(trace, blocks=None, names=None, regset=None, homes=False, priority=()):
    events = trace["events"]
    frames = [row for row in events if row["event"] == "frame"]
    allocated = {t["address"] for row in frames for t in row["temps"] if t["home"] is not None}

    def local(value):
        if names and flip.temp_key(value).split(".")[0] in names:
            return True
        row = value.get("conflict", value)
        start = row.get("first_ir")
        end = row.get("last_ir")
        return (blocks and start and end and
                any(start[0] <= b <= end[0] and
                    (start[0] != b or start[1] <= hi) and
                    (end[0] != b or end[1] >= lo)
                    for b, lo, hi in blocks))

    def keep_pair(event, a, b, _ranks):
        phase = event["kind"]
        if phase == "home":
            return homes and {a["address"], b["address"]} <= allocated
        if phase == "candidate":
            if not names:
                return False
            return all(flip.temp_key(t).split(".")[0] in names for t in (a, b))
        return local(a) and local(b)

    keys = []
    for sel, row, option, _ in flip.decisions(trace["function"], events, keep_pair):
        if row["event"] == "colour" and (not local(row) or regset and option["register"] not in regset):
            continue
        keys.append(sel)
    return sorted(keys, key=lambda sel: priority.index(sel) if sel in priority else len(priority))


def climb(baseline, unit, focus, destination, max_flips=6, max_round_trials=None, bridges=()):
    trace = cg.load_json(baseline)
    name = trace["function"]
    source, profile, flags, includes = cg.settings(trace["source"], unit, None)
    img = cg.match.Image()
    base_path = baseline.with_suffix(".obj")
    if (trace["dos_code_equal"] is not True or hashlib.sha256(base_path.read_bytes()).hexdigest() != trace["object_sha256"]
            or hashlib.sha256(source.read_bytes()).hexdigest() != trace["source_sha256"]):
        raise ValueError("climb requires a matching DOS-controlled baseline trace, source and object")
    base, = cg.omf.load(base_path)
    original = cg.match.verify_object(img, base)
    expected, = [r for r in original if r["function"] == name]
    destination.mkdir(exist_ok=True, parents=True)
    log = destination / "trials.jsonl"
    log.write_text("", encoding="utf-8")
    cache = {}

    def test(flips):
        key = tuple(flips)
        if key in cache:
            return cache[key]
        receipt = {"flips": flips}
        with tempfile.TemporaryDirectory(prefix="s", dir=flip.WORKER) as tmp:
            scratch = Path(tmp)
            obj = scratch / "out.obj"
            observer = flip.FlipTrace(name, flips, detail=False)
            try:
                cg.compile95(source, obj, flags, includes, scratch, "nt", observer)
                observer.validate()
                mod, = cg.omf.load(obj)
                verdict, = cg.match.verify_object(img, mod, {name})
                receipt.update(size=verdict["size"], status=verdict["status"],
                               score=score(verdict), first_diff=verdict.get("first_diff"),
                               applied=observer.applied, problems=verdict["problems"])
                if score(verdict) < score(expected):
                    receipt["regressions"] = regressions(original, cg.match.verify_object(img, mod), name)
            except (RuntimeError, ValueError, OSError, KeyError) as exc:
                receipt["error"] = str(exc)
        with log.open("a", encoding="utf-8") as stream:
            stream.write(json.dumps(receipt) + "\n")
        cache[key] = receipt
        return receipt

    current = {"flips": [], "score": score(expected), "size": expected["size"], "status": expected["status"]}
    history = [current]
    stop = "no improvement"
    for depth in range(max_flips):
        if current["status"] == "STRICT":
            stop = "oracle match"
            break
        if len(current["flips"]) >= max_flips:
            stop = "flip limit"
            break
        keys = [k for k in opportunities(trace, **focus) if k not in current["flips"]]
        print(f"{name}: depth {depth}, {len(keys)} regional choices, score {current['score']}", flush=True)
        trials = keys[:max_round_trials] if max_round_trials else keys
        best = current
        for key in trials:
            receipt = test(current["flips"] + [key])
            if (receipt.get("score", best["score"]) < best["score"]
                    and receipt.get("regressions") == []):
                best = receipt
            # A singleton oracle hit is already minimal. Still run the whole
            # first sweep; it identifies alternate explanatory decisions.
            if depth and best["status"] == "STRICT":
                break
        if best is current and not depth and max_flips >= 2:
            for pair in bridges:
                if len(set(pair)) != 2 or not all(k in trials for k in pair):
                    raise ValueError("a plateau bridge must contain two tested single flips")
                receipt = test(pair)
                if (receipt.get("score", best["score"]) < best["score"]
                        and receipt.get("regressions") == []):
                    best = receipt
        if best is current:
            stop = "trial budget" if len(trials) < len(keys) else "no improvement"
            break
        current = best
        history.append(current)
        trace = flip.trace(source, name, profile, flags, includes,
                           destination / f"accepted{depth + 1}.json", current["flips"])
        print(f"{name}: accepted {current['flips']}, score {current['score']}", flush=True)
    else:
        stop = "oracle match" if current["status"] == "STRICT" else "flip limit"

    # Prove subset minimality within the accepted sequence. A greedy climb
    # does not establish minimum cardinality over unvisited alternate paths.
    for size in range(1, len(current["flips"])):
        for subset in combinations(current["flips"], size):
            receipt = test(list(subset))
            if receipt.get("score") == current["score"] and receipt.get("regressions") == []:
                current = receipt
                flip.trace(source, name, profile, flags, includes,
                           destination / "minimized.json", current["flips"])
                break
        else:
            continue
        break
    report = {"diagnostic_only": True, "baseline": str(baseline), "focus": focus, "bridges": bridges, "tested": len(cache), "stop": stop,
              "best": current, "history": history}
    cg.dump_json(destination / "summary.json", report)
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("function")
    parser.add_argument("--focus", type=Path, default=flip.WORKER / "focus.json")
    parser.add_argument("--tag", default="search")
    parser.add_argument("--baseline", default="baseline.json")
    parser.add_argument("--max-flips", type=int, choices=range(1, 7), default=6)
    parser.add_argument("--max-round-trials", type=int)
    args = parser.parse_args()
    if args.max_round_trials is not None and args.max_round_trials < 1:
        parser.error("--max-round-trials must be positive")
    conf = cg.load_json(args.focus)
    focus = conf[args.function].copy()
    unit = focus.pop("unit")
    bridges = focus.pop("bridges", ())
    run = flip.WORKER / "runs" / args.function
    dest = flip.output_path(run / args.tag / "summary.json").parent
    baseline = flip.output_path(run / args.baseline)
    result = climb(baseline, unit, focus, dest, args.max_flips, args.max_round_trials, bridges)
    print(json.dumps(result["best"], indent=2))


if __name__ == "__main__":
    main()



