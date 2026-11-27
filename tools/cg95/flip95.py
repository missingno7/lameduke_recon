"""Diagnostic FLIP extension for the pinned CG95 observer."""
import argparse
from collections import Counter
import ctypes as C
import hashlib
import json
import re
from pathlib import Path
import struct
import sys
import tempfile

sys.dont_write_bytecode = True
ROOT = next(p for p in Path(__file__).resolve().parents if (p / "config/units.json").is_file())
WORKER = Path(os.environ.get("FLIP95_WORK", str(ROOT / "build/flip95")))
sys.path.insert(0, str(ROOT / "tools/cg95"))
import cgtrace95 as cg


def temp_key(row):
    temp = row.get("name", row.get("temp", row))
    symbol = temp.get("symbol")
    identity = f"t{temp['id']}" if "id" in temp else f"m{temp['handle']}"
    if symbol:
        identity += f"_{symbol}"
    return f"{identity}.{temp.get('alias_offset', temp.get('offset', 0))}"


def pair_key(a, b):
    return "~".join(sorted((temp_key(a), temp_key(b))))


def selector(function, phase, kind, operands, occurrence):
    return f"{function}:{phase}:{kind}:{operands}:{occurrence}"


def ordinal(function, phase, kind, operands, counts):
    key = (phase, kind, operands)
    counts[key] += 1
    return selector(function, phase, kind, operands, counts[key])


def eligible_choices(row):
    return [c for c in row["choices"] if "score" in c]


def sort_pairs(row, output=None):
    """Pairs the original comparator considers equal, in natural result order."""
    records = {item["address"]: item for item in row["input"]}
    positions = {address: i for i, address in enumerate(output or row["output_order"])}
    for group in row["same_priority_groups"]:
        records_in_order = [records[a] for a in sorted(group, key=positions.__getitem__)]
        for i, a in enumerate(records_in_order):
            for b in records_in_order[i + 1:]:
                if temp_key(a) == temp_key(b):
                    continue
                if row["kind"] == "home" and a["id"] == b["id"]:
                    continue  # The comparator orders aliases of the same backend identity.
                yield a, b


def decisions(function, events, keep_pair=None):
    """Enumerate selectors independently of debugger addresses and output rank."""
    counts = Counter()
    for row in events:
        if row["event"] == "sort":
            phase = row["kind"]
            ranks = {address: i for i, address in enumerate(row["output_order"])}
            for a, b in sort_pairs(row, row.get("natural_output_order")):
                kind = "home-order" if phase == "home" else "priority-tie"
                sel = ordinal(function, phase, kind, pair_key(a, b), counts)
                if keep_pair is None or keep_pair(row, a, b, ranks):
                    yield sel, row, a, b
        elif row["event"] == "colour":
            if not eligible_choices(row):
                continue
            temp = temp_key(row["temp"])
            for choice in eligible_choices(row):
                reg = choice["register"]
                sel = ordinal(function, "leaf", "register", f"{temp}>{reg}", counts)
                if not row.get("forced") and reg != row["best_before_cost_check"] and reg != row["chosen"]:
                    yield sel, row, choice, None


class FlipTrace(cg.AllocatorTrace):
    def __init__(self, function, flips, detail=True):
        super().__init__(function)
        if len(set(flips)) != len(flips) or len(flips) > 6:
            raise ValueError("use up to six distinct selectors")
        pattern = re.compile(r"([A-Za-z_]\w*):(candidate|conflict|home|leaf):(priority-tie|home-order|register):([^:]+):([1-9]\d*)")
        for flip in flips:
            found = pattern.fullmatch(flip)
            if found is None or found[1] != function:
                raise ValueError(f"invalid selector for {function}: {flip}")
        self.flips = tuple(flips)
        self.keys = {tuple(s.split(":")[1:4]) for s in flips}
        self.applied = {}
        self.counts = Counter()
        self.detail = detail
        self.k.WriteProcessMemory.argtypes = self.k.ReadProcessMemory.argtypes

    def graph(self):
        return super().graph() if self.detail else {}

    def write_word(self, address, value):
        raw = struct.pack("<I", value)
        wrote = C.c_size_t()
        cg.Debugger  # The stock debugger supplies the active process handle.
        if not self.k.WriteProcessMemory(self.process, address, raw, len(raw), C.byref(wrote)):
            raise C.WinError(C.get_last_error())
        if wrote.value != len(raw):
            raise RuntimeError(f"short debugger write at {address:#x}")

    def hit(self, rva, ctx):
        # At the leaf sentinel, best-mask is still provisional. Watcom runs
        # its normal cost check, assignment, interference updates and emission.
        if rva == cg.COLOUR_NEXT and self.word(self.word(ctx.ebp - 0x1c)) == 0:
            self.flip_colour(ctx)
        super().hit(rva, ctx)
        if rva == cg.SORT_LEAVE:
            self.flip_order(self.events[-1], ctx)

    def flip_colour(self, ctx):
        row = self.active_choice
        original = self.word(ctx.ebp - 0x14)
        temp = temp_key(row["temp"])
        for choice in eligible_choices(row):
            key = ("leaf", "register", f"{temp}>{choice['register']}")
            if key not in self.keys:
                continue
            sel = ordinal(self.function, *key, self.counts)
            if sel not in self.flips:
                continue
            if row.get("forced") or not original or original == choice["register_mask"]:
                raise ValueError(f"selector does not change one normal colour decision: {sel}")
            self.write_word(ctx.ebp - 0x14, choice["register_mask"])
            self.write_word(ctx.ebp - 0x18, choice["score"] & 0xffffffff)
            row["forced"] = sel
            self.applied[sel] = {"before": cg.register(original), "requested": choice["register"]}

    def flip_order(self, row, ctx):
        phase = row["kind"]
        kind = "home-order" if phase == "home" else "priority-tie"
        if not any((p, k) == (phase, kind) for p, k, _ in self.keys):
            return
        selected = []
        for a, b in sort_pairs(row):
            key = (phase, kind, pair_key(a, b))
            if key in self.keys:
                sel = ordinal(self.function, *key, self.counts)
                if sel in self.flips:
                    selected.append((sel, a["address"], b["address"]))
        if not selected:
            return
        order = row["output_order"].copy()
        ranks = {address: i for i, address in enumerate(order)}
        for sel, a, b in sorted(selected, key=lambda entry: self.flips.index(entry[0])):
            i, j = ranks[a], ranks[b]
            order[i], order[j] = order[j], order[i]
            ranks[a], ranks[b] = j, i
            self.applied[sel] = {"before_ranks": [i, j], "after_ranks": [j, i]}
        if phase == "home":
            records = {r["address"]: r for r in row["input"]}
            positions = {addr: i for i, addr in enumerate(order)}
            aliases = {}
            for addr in row["output_order"]:
                item = records[addr]
                key = (cg.sort_key(phase, item), item["id"])
                prev = aliases.get(key, -1)
                if positions[addr] <= prev:
                    raise ValueError("requested home-order swap crosses aliases of one temp")
                aliases[key] = positions[addr]
        # Exchange the selected tied ranks in the in-memory sorter result.
        # SORT_LEAVE is a ret whose EAX is the returned list head.
        row["natural_output_order"] = row["output_order"]
        row["natural_sort_replay_equal"] = row["sort_replay_equal"]
        row["sort_replay_equal"] = order == row["natural_output_order"] and row["natural_sort_replay_equal"]
        row["output_order"] = order
        old_next = dict(zip(row["natural_output_order"], row["natural_output_order"][1:] + [0]))
        for address, successor in zip(order, order[1:] + [0]):
            if old_next[address] != successor:
                self.write_word(address, successor)
        ctx.eax = order[0]

    def validate(self):
        if not self.finished or not self.events:
            raise RuntimeError(f"no completed allocator trace for {self.function}")
        missed = set(self.flips) - self.applied.keys()
        if missed:
            raise ValueError("selectors did not resolve: " + ", ".join(sorted(missed)))
        for row in self.events:
            sel = row.get("forced")
            if sel:
                choice = self.applied[sel]
                if row["chosen"] != choice["requested"]:
                    raise ValueError(f"allocator cost check vetoed {sel}")
                choice["chosen"] = row["chosen"]


def output_path(path):
    path = Path(path).resolve()
    if not path.is_relative_to(WORKER.resolve()) or path.suffix.lower() != ".json":
        raise ValueError(f"expected a JSON output under {WORKER}")
    return path


def trace(source, function, profile, flags, includes, output, flips, *, detail=True):
    output = output_path(output)
    if not flips:
        cg.WORKER = WORKER
        return cg.trace(source, function, profile, flags, includes, output)
    observer = FlipTrace(function, flips, detail)
    output.parent.mkdir(exist_ok=True, parents=True)
    with tempfile.TemporaryDirectory(prefix="t", dir=WORKER) as tmp:
        scratch = Path(tmp)
        obj = scratch / "out.obj"
        cg.compile95(source, obj, flags, includes, scratch, "nt", observer)
        observer.validate()
        data = obj.read_bytes()
    result = {"format": 1, "diagnostic_only": True, "function": function, "source": str(source),
              "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
              "profile": profile, "flags": flags, "includes": [str(p) for p in includes],
              "compiler_sha256": cg.COMPILER_SHA256, "nt_code_equal": None, "dos_code_equal": None,
              "object_sha256": hashlib.sha256(data).hexdigest(), "flips": observer.applied,
              "events": observer.events}
    output.with_suffix(".obj").write_bytes(data)
    cg.dump_json(output, result)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("function")
    parser.add_argument("--unit")
    parser.add_argument("--source", type=Path)
    parser.add_argument("--out", type=Path)
    parser.add_argument("--flip", action="append", default=[])
    parser.add_argument("--list", type=Path, help="list selectors from a completed trace")
    args = parser.parse_args()
    try:
        if args.list:
            saved = cg.load_json(args.list)
            if saved["function"] != args.function:
                raise ValueError("trace function does not match")
            for sel, _, _, _ in decisions(args.function, saved["events"]):
                print(sel)
        else:
            if not args.unit or not args.out:
                parser.error("--unit and --out are required to trace a compile")
            source, profile, flags, includes = cg.settings(args.source, args.unit, None)
            result = trace(source, args.function, profile, flags, includes, args.out, args.flip)
            print(json.dumps({"diagnostic_only": bool(args.flip), "flips": result.get("flips", {})}, indent=2))
    except (RuntimeError, ValueError, OSError, KeyError) as exc:
        parser.exit(1, f"{exc}\n")


if __name__ == "__main__":
    main()




