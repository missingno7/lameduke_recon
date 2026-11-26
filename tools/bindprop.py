"""Propose symbol bindings from relocation evidence of byte-matching functions.

    python tools/bindprop.py RESULTS.json [...] [--min-funcs 2] [--out OVERLAY.json]

Reads tools/match.py --json outputs. For every function whose bytes and fixup sites equal the oracle
(status STRICT or CODE_BYTES_MATCH), each reference to a named external symbol votes for
symbol address = oracle target - addend (off32: disp + field; rel32: disp). A proposal requires all votes to agree
and at least --min-funcs distinct functions to vote (default 2), so a function never proves its own binding.
Conflicting votes are reported and never proposed. Output: an overlay in config/symbols.json layout with evidence.
"""
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from common import dump_json  # noqa: E402
from match import demangle  # noqa: E402


def main(argv):
    args = list(argv[1:])
    min_funcs, out = 2, None
    if "--min-funcs" in args:
        i = args.index("--min-funcs"); min_funcs = int(args[i + 1]); del args[i:i + 2]
    if "--out" in args:
        i = args.index("--out"); out = args[i + 1]; del args[i:i + 2]
    votes = {}
    for p in args:
        for r in json.load(open(p, encoding="utf-8")):
            if r["status"] not in ("STRICT", "CODE_BYTES_MATCH"):
                continue
            for ref in r["references"]:
                t = ref["target"]
                if t.startswith("seg:") or t.startswith("grp:") or "oracle_target" not in ref:
                    continue
                o, a = ref["oracle_target"].split(":")
                addend = ref["disp"] if ref["self_rel"] else ref["disp"] + ref["field"]
                votes.setdefault(t, []).append(((int(o), int(a, 16) - addend), r["function"]))
    funcs, data, conflicts = {}, {}, {}
    for t, vs in sorted(votes.items()):
        addrs = {a for a, _ in vs}
        fns = sorted({f for _, f in vs})
        if len(addrs) > 1:
            conflicts[t] = sorted(f"{o}:{a:05x}" for o, a in addrs)
            continue
        if len(fns) < min_funcs:
            continue
        o, a = addrs.pop()
        ent = {"addr": f"{o}:{a:05x}",
               "evidence": f"RELOCATION_VOTES {len(vs)} sites in {len(fns)} byte-matching functions: {', '.join(fns[:6])}"}
        (funcs if o == 1 else data)[demangle(t)] = ent
    print(f"{len(funcs)} function and {len(data)} data proposals; {len(conflicts)} conflicts")
    for t, c in conflicts.items():
        print(f"  CONFLICT {t}: {c}")
    if out:
        dump_json(out, {"d3d": {"functions": funcs, "data": data}})
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
