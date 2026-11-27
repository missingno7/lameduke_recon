"""Unmodified search traces and diagnostic flip targets for Watcom 9.5."""
from collections import Counter, defaultdict
from itertools import combinations
from pathlib import Path
import sys
import tempfile

ROOT = next(p for p in Path(__file__).resolve().parents if (p / "config/units.json").is_file())
sys.path.insert(0, str(ROOT / "tools/cg95"))
import climb95
import explain95 as ex
import flip95 as flip
from flip95 import cg


def allocation(oracle, mod, verdict, bases):
    """Aligned instruction operands, retaining register and frame displacement positions."""
    obj, address = ex.semir.parse_addr(verdict["oracle"])
    if obj != 1:
        return {"gaps": 0, "locations": 0, "pairs": 0}
    sides = ex.semir.OracleSide(oracle, address), ex.semir.CandSide(oracle, mod, verdict, bases)
    left, right = [ex.machine(side) for side in sides]
    tokens = [[ex.token(side, ins) for ins in code] for side, code in zip(sides, (left, right))]
    pairs, _ = ex.correspond(*tokens)
    mismatches = Counter()

    def components(ins):
        for operand in ex.physical(ins):
            if isinstance(operand, tuple):
                yield from operand
            else:
                yield operand

    for ci, oi in pairs.items():
        a, b = list(components(left[oi])), list(components(right[ci]))
        for x, y in zip(a, b):
            if x != y:
                mismatches[x, y] += 1
        if len(a) != len(b):
            mismatches["operand count", None] += abs(len(a) - len(b))
    return {"gaps": len(left) + len(right) - 2 * len(pairs),
            "locations": sum(mismatches.values()), "pairs": len(mismatches)}


def sort_items(event):
    records = {row["address"]: row for row in event["input"]}
    return [records[address] for address in event["output_order"]]


def temp(record):
    return record.get("temp", record.get("name", record))


def identity(record):
    row = temp(record)
    return row["class"], row["size"], row["type"], row.get("symbol"), row.get("alias_offset")


def operand_signature(row):
    if row is None:
        return None
    return (row["class"], row["type"], row["size"], row.get("symbol"), row.get("offset"),
            row.get("reg"), operand_signature(row.get("base")), operand_signature(row.get("index")))


def anchor(row):
    inst = row.get("first_instruction")
    if not inst:
        return None
    return (inst["position"][0], inst["opcode"], operand_signature(inst["result"]),
            tuple(map(operand_signature, inst["operands"])))


def locate(reference, rows, fingerprint):
    """Resolve a temp after source changes, refusing ambiguous renamed/anonymous values."""
    possible = [r for r in rows if identity(reference) == identity(r)]
    mark = fingerprint(reference)
    if mark is not None:
        found = [r for r in possible if fingerprint(r) == mark]
        if len(found) == 1:
            return found[0], "defining IR"
        if found:
            possible = found
        elif not temp(reference).get("symbol"):
            return None, "changed defining IR"
    if temp(reference).get("symbol") and len(possible) == 1:
        return possible[0], "symbol"
    found = [r for r in possible if flip.temp_key(r) == flip.temp_key(reference)]
    if len(found) == 1:
        return found[0], "compatible backend ID"
    return None, "unresolved"


class TraceIndex:
    def __init__(self, events):
        self.sorts = defaultdict(list)
        self.colours = []
        self.definitions = defaultdict(set)
        for row in events:
            if row["event"] == "sort":
                self.sorts[row["kind"]].append(row)
            elif row["event"] == "colour":
                self.colours.append(row)
                self.definitions[flip.temp_key(row)].add(anchor(row))
        self.colours_by_identity = defaultdict(list)
        for row in self.colours:
            self.colours_by_identity[identity(row)].append(row)

    def definition(self, record):
        marks = self.definitions.get(flip.temp_key(record), set()) - {None}
        return next(iter(marks)) if len(marks) == 1 else None


def compiler_margin(kind, first, second):
    """Positive means the desired first item has a better key."""
    a, b = cg.sort_key(kind, first), cg.sort_key(kind, second)
    for i, (left, right) in enumerate(zip(a, b)):
        if left != right:
            return len(a) - i, right - left
    return 0, 0


class Plan:
    def __init__(self, trace, baseline, selectors):
        self.reference = TraceIndex(trace["events"])
        self.baseline = TraceIndex(baseline["events"])
        self.required = []
        self.missing = []
        lookup = {key: (row, a, b) for key, row, a, b in
                  flip.decisions(trace["function"], baseline["events"])}
        for key in selectors:
            _, phase, _, operands, occurrence = key.split(":")
            if phase == "leaf":
                name, reg = operands.split(">")
                choices = [r for r in self.reference.colours if flip.temp_key(r) == name
                           and any(c["register"] == reg and "score" in c for c in r["choices"])]
                if len(choices) >= int(occurrence):
                    row = choices[int(occurrence)-1]
                    natural = [r for r in self.baseline.colours if flip.temp_key(r) == name
                               and any(c["register"] == reg for c in r["choices"])]
                    if len(natural) >= int(occurrence) and identity(natural[int(occurrence)-1]) == identity(row):
                        row = natural[int(occurrence)-1]
                    self.required.append((key, phase, row, reg))
                else:
                    self.missing.append(key)
                continue
            before = lookup.get(key)
            old_sorts = [r for r in baseline["events"] if r["event"] == "sort" and r["kind"] == phase]
            if before:
                number = next(i for i, row in enumerate(old_sorts) if row is before[0])
            else:
                # Later flips can expose a tie. Count matching comparator ties in the
                # diagnostic trace's natural list before applying its changes.
                found = [i for i, row in enumerate(self.reference.sorts[phase])
                         if any(flip.pair_key(a, b) == operands
                                for a, b in flip.sort_pairs(row, row.get("natural_output_order")))]
                if len(found) < int(occurrence):
                    self.missing.append(key)
                    continue
                number = found[int(occurrence)-1]
            reference = self.reference.sorts[phase]
            if number >= len(reference):
                self.missing.append(key)
                continue
            order = sort_items(reference[number])
            pair = [row for row in order if flip.temp_key(row) in operands.split("~")]
            if len(pair) != 2:
                self.missing.append(key)
                continue
            self.required.append((key, phase, number, pair))
        self.add_related_pairs()

    def add_related_pairs(self):
        groups = defaultdict(set)
        pairs = set()
        for _, phase, number, pair in self.required:
            if phase == "leaf":
                continue
            names = tuple(map(flip.temp_key, pair))
            groups[phase, number].update(names)
            pairs.add((phase, number, names))
        for (phase, number), names in groups.items():
            order = [r for r in sort_items(self.reference.sorts[phase][number]) if flip.temp_key(r) in names]
            for pair in combinations(order, 2):
                keys = tuple(map(flip.temp_key, pair))
                if (phase, number, keys) not in pairs:
                    selector = f"related-{phase}-{number+1}:{'~'.join(keys)}"
                    self.required.append((selector, phase, number, pair))

    def distance(self, events):
        index = TraceIndex(events)
        rows = []
        for key, phase, a, b in self.required:
            if phase == "leaf":
                actual, basis = locate(a, index.colours_by_identity[identity(a)],
                                       lambda r: anchor(r))
                record = self.colour_distance(actual, b)
                record["association"] = basis
            else:
                sorts = index.sorts[phase]
                record = (self.sort_distance(phase, b, sort_items(sorts[a]), index, a)
                          if a < len(sorts) else {"missing": "sort pass absent"})
            rows.append({"selector": key, **record})
        rows.extend({"selector": key, "missing": "decision absent from diagnostic trace"} for key in self.missing)
        score = (sum("missing" in r for r in rows),
                 sum(r.get("wrong", 0) for r in rows),
                 sum(r.get("key_tier", 0) for r in rows),
                 sum(r.get("deficit", 0) for r in rows),
                 sum(r.get("rank_distance", 0) for r in rows))
        return score, rows

    def sort_distance(self, phase, expected, order, index, pass_index):
        # Trace the role on the original unforced source. A prior diagnostic
        # flip may change its eventual defining IR without changing its ID.
        frozen = self.baseline.sorts[phase]
        natural = ({flip.temp_key(row): row for row in frozen[pass_index]["input"]}
                   if pass_index < len(frozen) else {})
        expected = [natural.get(flip.temp_key(row), row) for row in expected]

        def signature(row):
            if any(row is item for item in expected):
                return self.baseline.definition(row) or self.reference.definition(row)
            return index.definition(row)

        matches = [locate(ref, order, signature) for ref in expected]
        if any(row is None for row, _ in matches) or matches[0][0] is matches[1][0]:
            return {"missing": "sort operands unresolved", "association": [basis for _, basis in matches]}
        first, second = [row for row, _ in matches]
        ranks = order.index(first), order.index(second)
        tier, margin = compiler_margin(phase, first, second)
        return {"wrong": int(ranks[0] > ranks[1]), "ranks": ranks, "margin": margin,
                "key_tier": tier if margin < 0 else 0, "deficit": max(0, -margin),
                "rank_distance": max(0, ranks[0] - ranks[1]),
                "association": [basis for _, basis in matches],
                "keys": [cg.sort_key(phase, row) for row in (first, second)]}

    @staticmethod
    def colour_distance(row, desired):
        if row is None:
            return {"missing": "colour temp unresolved"}
        eligible = {c["register"]: c["score"] for c in row["choices"] if "score" in c}
        incumbent = row["best_before_cost_check"]
        base = {"wrong": int(row["chosen"] != desired), "chosen": row["chosen"], "desired": desired}
        if desired not in eligible:
            return {**base, "key_tier": 1, "deficit": 0, "ineligible": True}
        margin = eligible[desired] - eligible.get(incumbent, eligible[desired])
        return {**base, "margin": margin, "deficit": max(0, -margin)}


def observe(cfg, function, source, output, work):
    observer = cg.AllocatorTrace(function)
    with tempfile.TemporaryDirectory(prefix="obs", dir=work) as scratch:
        cg.compile95(source, output, cfg.flags, cfg.includes, Path(scratch), "nt", observer)
    if not observer.finished or not observer.events:
        raise RuntimeError(f"{function}: unmodified allocator trace incomplete")
    return observer.events


def prepare(cfg, function, work, out, hint, trial_limit):
    """One flip exploration per search target, sharing that plan across all variants."""
    cg.WORKER = flip.WORKER = work
    base_path = out / "baseline.json"
    baseline = cg.trace(cfg.source, function, cfg.profile, cfg.flags, cfg.includes, base_path)
    explanation = ex.analyse(base_path, str(cfg.symbols) if cfg.symbols else None)
    cg.dump_json(out / "explain.json", explanation)
    img = cg.match.Image()
    base = cg.match.verify_object(img, cg.omf.load(base_path.with_suffix(".obj"))[0])
    verdict = next(r for r in base if r["function"] == function)
    explored = []
    paired = None
    selected = []
    reference = baseline
    best = climb95.score(verdict)
    status = verdict["status"]
    warm = hint.get("replay", [])
    if warm and status != "STRICT":
        for size in range(1, len(warm) + 1):
            for chain in combinations(warm, size):
                try:
                    probe = out / "replay.json"
                    traced = flip.trace(cfg.source, function, cfg.profile, cfg.flags, cfg.includes, probe, chain)
                    rows = cg.match.verify_object(img, cg.omf.load(probe.with_suffix(".obj"))[0])
                    row = next(r for r in rows if r["function"] == function)
                    score = climb95.score(row)
                    dos_collateral = climb95.regressions(base, rows, function)
                    collateral = dos_collateral
                    if dos_collateral and score < best:
                        # Debug hosting can alter private CODE prefix residue
                        # outside the checked CG95 function projection.
                        if paired is None:
                            control = out / "unforced-debug.obj"
                            observe(cfg, function, cfg.source, control, work)
                            if cg.projection(control.read_bytes()) != cg.projection(base_path.with_suffix(".obj").read_bytes()):
                                raise RuntimeError("unforced debug control differs from DOS function projection")
                            paired = cg.match.verify_object(img, cg.omf.load(control)[0])
                        collateral = climb95.regressions(paired, rows, function)
                    explored.append({"chain": chain, "score": score, "status": row["status"],
                                     "collateral": collateral, "dos_baseline_collateral": dos_collateral})
                    if score < best and not collateral:
                        best, selected, reference, status = score, chain, traced, row["status"]
                except (RuntimeError, ValueError, OSError, KeyError) as exc:
                    explored.append({"chain": chain, "error": str(exc)})
                if status == "STRICT":
                    break
            if status == "STRICT":
                break
    if status != "STRICT" and trial_limit:
        options = {k: hint[k] for k in ("blocks", "names", "regset", "homes", "priority") if k in hint}
        if not options:
            options = auto_focus(baseline, explanation)
        # Keep the exact registered flags/includes, including CLI overrides.
        old_settings = cg.settings
        cg.settings = lambda *_: (cfg.source, cfg.profile, cfg.flags, cfg.includes)
        try:
            climb = climb95.climb(base_path, cfg.unit, options, out / "climb",
                                  max_round_trials=trial_limit, bridges=hint.get("bridges", ()))
        finally:
            cg.settings = old_settings
        row = climb["best"]
        if tuple(row["score"]) < best:
            selected, status, best = row["flips"], row["status"], tuple(row["score"])
            reference = flip.trace(cfg.source, function, cfg.profile, cfg.flags, cfg.includes,
                                   out / "selected.json", selected)
    plan = Plan(reference, baseline, selected)
    cg.dump_json(out / "plan.json", {"selectors": selected, "diagnostic_status": status,
                                   "diagnostic_score": best, "replay_trials": explored,
                                   "baseline_distances": plan.distance(baseline["events"])})
    return plan, baseline["events"], explanation


def auto_focus(trace, explanation):
    event = explanation.get("first_attributed") or {}
    evidence = event.get("evidence") or {}
    labels = {str(v).split(".")[0] for k, v in evidence.items()
              if k in {"temp", "candidate_slot_holder"} and isinstance(v, str)}
    blocks, names = [], set()
    for row in trace["events"]:
        if row["event"] != "colour":
            continue
        value = temp(row)
        prefix = flip.temp_key(row).split(".")[0]
        if value.get("symbol") not in labels and prefix not in labels:
            continue
        names.add(prefix)
        position = row["conflict"].get("first_ir")
        if position:
            block, offset, _ = position
            blocks.append([block, max(0, offset-3), offset+3])
    return {"names": sorted(names), "blocks": blocks, "homes": True}
