"""Map cg95's first attributed allocator decision to editable source names."""
from collections import defaultdict
import re
import sys


def focus_from_report(function, report, trace=None):
    evidence = (report.get("first_attributed") or {}).get("evidence", {})
    labels = set()
    for key in ("temp", "candidate_slot_holder", "earlier_overlapping_choices", "same_savings_order"):
        values = evidence.get(key) or []
        for value in [values] if isinstance(values, str) else values:
            labels.update(re.findall(r"[A-Za-z_]\w*", value))
    if not trace:
        return (labels & function.uses.keys()) - {n for n in labels if re.fullmatch(r"t\d+", n)}

    # Resolve only the named decision, its reported competitors and the
    # operands of their defining instructions. Unrelated conflicts stay out.
    definitions, names = defaultdict(set), defaultdict(set)
    addressed = set()
    for event in trace["events"]:
        if event["event"] != "colour":
            continue
        temp = event["temp"]
        label = temp.get("symbol") or f"t{temp.get('id', '?')}"
        if temp.get("symbol"):
            addressed.add(label)

        def collect(operand):
            if not operand:
                return
            if operand.get("symbol"):
                names[label].add(operand["symbol"])
            elif operand.get("id") is not None:
                definitions[label].add(f"t{operand['id']}")
            for key in ("base", "index"):
                collect(operand.get(key))

        first = event.get("first_instruction")
        if first and (first.get("result") or {}).get("address") == temp["address"]:
            for operand in first["operands"]:
                collect(operand)

    resolved = {n for n in labels if n in addressed or not re.fullmatch(r"t\d+", n)}
    seen = set()
    while labels - seen:
        label = min(labels - seen)
        seen.add(label)
        resolved.update(names[label])
        # A named source value is already a useful stopping point for focus.
        if label not in addressed:
            labels.update(definitions[label])
    return resolved & function.uses.keys()


def trace_report(root, work, out, cfg, function):
    sys.path.insert(0, str(root / "tools/cg95"))
    import cgtrace95
    import explain95

    cgtrace95.WORKER = work
    path = out / f"{function}-trace.json"
    trace = cgtrace95.trace(cfg.source, function, cfg.profile, cfg.flags, cfg.includes, path)
    report = explain95.analyse(path, str(cfg.symbols) if cfg.symbols else None)
    explain95.dump_json(path.with_name(f"{function}-explain.json"), report)
    return report, trace
