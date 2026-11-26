"""Trace Watcom 9.5b candidate ordering, leaf colours and stack homes."""
import argparse
import hashlib
import os
from pathlib import Path
import re
import shutil
import struct
import sys
import tempfile
from types import SimpleNamespace

sys.dont_write_bytecode = True
ROOT = next(p for p in Path(__file__).resolve().parents if (p / "config/units.json").is_file())
WORKER = ROOT / "build/cg95"
WORKER.mkdir(parents=True, exist_ok=True)
sys.path.insert(0, str(ROOT / "tools"))
from common import dump_json, load_json
import doshost
import match
import omf
import wcc
from debug95 import Debugger


COMPILER_SHA256 = "8120ccd90e6cdfdcfebca3ef647e8a4489618a12d1548a43cd7fbae75e110956"
PROC_NAME = 0x3748D
PROC_RELEASE = 0x4703C
SYMBOL_COPY = 0x306E2
SORT_ENTER = 0x4C22D
SORT_LEAVE = 0x4C26A
LEAF_ENTER = 0x52FF6
COLOUR_NEXT = 0x5300F
COLOUR_GREED = 0x53049
COLOUR_SCORE = 0x53061
COLOUR_COST = 0x530DF
COLOUR_LEAVE = 0x53116
HOMES_DONE = 0x3EFF9
FRAME_READY = 0x5E177

GIVEN = 0x9207C
TEMP_HEAD = 0x920BE
BLOCK_HEAD = 0x92040
PROC = 0x92078

SORTS = {0x3CE14: "candidate", 0x53951: "conflict", 0x4D5BC: "home"}
REGISTER_MASKS = dict(zip(
    ("eax", "edx", "ebx", "ecx", "esi", "edi", "ebp"),
    (0x01000003, 0x080000C0, 0x0200000C, 0x04000030, 0x10000100, 0x20000200, 0x400),
))
REGISTER_MASKS.update(ax=3, dx=0xc0, bx=0xc, cx=0x30, si=0x100, di=0x200,
                      al=1, ah=2, bl=4, bh=8, cl=0x10, ch=0x20, dl=0x40, dh=0x80)
MASK_NAMES = {value: name for name, value in REGISTER_MASKS.items()}


def signed(value):
    return struct.unpack("<i", struct.pack("<I", value))[0]


def register(mask):
    return MASK_NAMES.get(mask, hex(mask)) if mask else None


def sort_key(kind, row):
    if kind == "conflict":
        return (-row["savings"],)
    if kind == "candidate":
        savings = row["savings"]
        return (row["flags"] >> 8 & 1, savings is None,
                -(savings if savings is not None else signed(row["id"])))
    return (not bool(row["flags"] >> 8 & 2), row["size"])


def replay_sort(kind, records):
    """Replay the normal array sort at 4c3c5, including its alternating gap rule."""
    ordered = [(sort_key(kind, row), row) for row in records]
    gap, phase = len(records), 1
    if not ordered:
        return []
    while True:
        phase ^= 1
        gap = gap // 2 + phase
        swapped = True
        while swapped:
            swapped = False
            for i in range(len(ordered) - gap):
                a, b = ordered[i], ordered[i + gap]
                alias_swap = (kind == "home" and a[0] == b[0] and a[1]["id"] == b[1]["id"]
                              and a[1]["alias_offset"] < b[1]["alias_offset"])
                if b[0] < a[0] or alias_swap:
                    ordered[i], ordered[i + gap] = b, a
                    swapped = True
        if gap == 1:
            return [row["address"] for _, row in ordered]


class AllocatorTrace(Debugger):
    def __init__(self, function):
        super().__init__()
        self.function = function
        self.breakpoints = (PROC_NAME,)
        self.events = []
        self.order = []
        self.focus = None
        self.active_sort = None
        self.active_choice = None
        self.temp_symbols = {}
        self.finished = False

    def global_word(self, rva):
        return self.word(self.image + rva)

    def words(self, address, count):
        return struct.unpack(f"<{count}I", self.read(address, 4 * count)) if count else ()

    def linked(self, pointer, read_record, limit=8192):
        seen = set()
        while pointer:
            if pointer in seen or len(seen) == limit:
                raise RuntimeError("unexpected backend list cycle or size")
            seen.add(pointer)
            record = read_record(pointer)
            yield record
            pointer = record["next"]

    def symbol(self, handle):
        # Read only symbols already resident in a Watcom frontend page.
        if not 0 < handle < 65536:
            return None
        if handle not in self.temp_symbols:
            try:
                page, index = divmod(handle, 22)
                pointer = 0
                table = self.global_word(0x90650)
                if handle == self.global_word(0x90672) & 0xffff:
                    pointer, index = self.image + 0x9171c, 0
                elif table and page < 16:
                    pointer = self.word(table + 4 * page)
                elif self.global_word(0x903d4) == page:
                    pointer = self.global_word(0x90558)
                elif self.global_word(0x903d8) == page // 16:
                    backing = self.global_word(0x90450)
                    pointer = backing + (page % 16) * 1024 if backing else 0
                if pointer:
                    name = self.string(self.word(pointer + 46 * index + 0x22))
                    if name and re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name):
                        self.temp_symbols[handle] = name
            except OSError:
                pass
        return self.temp_symbols.get(handle)

    def name(self, address):
        if not address:
            return None
        fields = self.words(address, 5)
        result = {"address": address, "next": fields[0], "class": fields[1] & 255,
                  "type": (fields[1] >> 8) & 255, "size": fields[2]}
        if result["class"] == 2:
            sym, usage, alias, location, temp_id, masks, flags = self.words(address + 20, 7)
            result.update(id=temp_id, handle=sym, symbol=self.symbol(sym), alias=alias,
                          alias_offset=signed(fields[4]), conflict=fields[3],
                          usage=usage & 255, flags=flags & 0xffff,
                          home=signed(location) if usage & 0x40 else None)
        elif result["class"] == 1:
            handle = self.word(address + 0x14)
            result.update(handle=handle, symbol=self.symbol(handle), offset=signed(fields[4]))
        elif result["class"] == 3:
            result.update(reg=register(fields[3]), register_mask=fields[3])
        return result

    def conflict(self, address):
        fields = self.words(address, 22)
        return dict(address=address, next=fields[0], name=self.name(fields[2]),
                    savings=fields[5], first=fields[14], last=fields[15],
                    block=fields[3], unavailable_mask=fields[8], flags=fields[21] & 0xffff)

    def operand(self, address, depth=0):
        name = self.name(address)
        if name and name["class"] == 4 and depth < 2:
            index, base, offset = self.words(address + 12, 3)
            name.update(index=self.operand(index, depth + 1), base=self.operand(base, depth + 1),
                        offset=signed(offset))
        return name

    def instruction(self, address, position):
        header = self.read(address, 0x44)
        count = header[0x3d]
        if count > 12:
            raise RuntimeError(f"invalid operand count {count} at {address:#x}")
        operands = self.words(address + 0x44, count)
        return {"position": position, "opcode": header[0x22],
                "result": self.operand(struct.unpack_from("<I", header, 0x30)[0]),
                "operands": [self.operand(p) for p in operands]}

    def graph(self):
        """Index allocator IR live-range endpoints by block and instruction order."""
        locations = {}
        block = self.global_word(BLOCK_HEAD)
        blocks = set()
        while block:
            if block in blocks or len(blocks) >= 8192:
                raise RuntimeError("unexpected backend CFG list")
            blocks.add(block)
            address = self.word(block + 4)
            for index in range(8192):
                opcode = self.read(address + 0x22, 1)[0]
                locations[address] = [len(blocks) - 1, index, opcode]
                if opcode == 0x4B:
                    break
                address = self.word(address + 4)
            else:
                raise RuntimeError("backend instruction list lacks a block sentinel")
            block = self.word(block + 0x28)
        return locations

    def sort_records(self, head, kind):
        if kind == "conflict":
            graph = self.graph()
            records = list(self.linked(head, self.conflict))
            for record in records:
                record["first_ir"] = graph.get(record["first"])
                record["last_ir"] = graph.get(record["last"])
            return records
        records = list(self.linked(head, self.name))
        if kind == "candidate":
            for record in records:
                conflict = record.get("conflict")
                record["savings"] = self.word(conflict + 20) if conflict else None
        return records

    def replace(self, old, new):
        self.breakpoints = tuple(new if site == old else site for site in self.breakpoints)

    def hit(self, rva, ctx):
        if rva == PROC_NAME:
            name = self.string(ctx.eax)
            handle = ctx.ebx & 0xffff
            self.order.append(name)
            if name == self.function:
                if self.focus is not None:
                    raise RuntimeError(f"multiple backend procedures named {name!r}")
                self.focus = handle
                self.temp_symbols[handle] = name
                self.breakpoints = (PROC_RELEASE, SORT_ENTER, LEAF_ENTER, SYMBOL_COPY)
        elif rva == PROC_RELEASE:
            proc = self.global_word(PROC)
            if not proc or self.word(self.word(proc + 0x84) + 8) & 0xffff != self.focus:
                raise RuntimeError("focused procedure did not reach its own release boundary")
            self.finished = True
            self.breakpoints = (PROC_NAME,)
        elif rva == SYMBOL_COPY:
            handle = ctx.edx & 0xffff
            pointer = self.word(self.word(ctx.esp) + 0x22)
            if handle and pointer:
                try:
                    name = self.string(pointer)
                    if name and re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name):
                        self.temp_symbols[handle] = name
                except OSError:
                    pass
        elif rva == SORT_ENTER:
            kind = SORTS.get(ctx.ebx - self.image)
            if kind:
                if kind == "home" and SYMBOL_COPY in self.breakpoints:
                    self.replace(SYMBOL_COPY, HOMES_DONE)
                if ctx.edx:
                    raise RuntimeError("unexpected allocator sort list link offset")
                self.active_sort = {"event": "sort", "kind": kind,
                                    "caller": hex(self.word(ctx.esp) - self.image),
                                    "input": self.sort_records(ctx.eax, kind)}
                self.replace(SORT_ENTER, SORT_LEAVE)
        elif rva == SORT_LEAVE:
            row = self.active_sort
            output = list(self.linked(ctx.eax, self.conflict if row["kind"] == "conflict" else self.name))
            row["output_order"] = [item["address"] for item in output]
            if sorted(row["output_order"]) != sorted(item["address"] for item in row["input"]):
                raise RuntimeError("allocator sort changed list membership")
            by_address = {item["address"]: item for item in row["input"]}
            groups = {}
            for address in row["output_order"]:
                item = by_address[address]
                key = sort_key(row["kind"], item)
                groups.setdefault(key, []).append(address)
            # The home comparator can further order two aliases of the same temp ID.
            row["same_priority_groups"] = [group for group in groups.values() if len(group) > 1]
            row["sort_replay_equal"] = row["output_order"] == replay_sort(row["kind"], row["input"])
            self.events.append(row)
            self.active_sort = None
            self.replace(SORT_LEAVE, SORT_ENTER)
        elif rva in (HOMES_DONE, FRAME_READY):
            proc = self.global_word(PROC)
            self.events.append({"event": "homes" if rva == HOMES_DONE else "frame",
                                "local_bytes": self.word(proc + 0x2c),
                                "temps": list(self.linked(self.global_word(TEMP_HEAD), self.name))})
            self.replace(HOMES_DONE, FRAME_READY)
        else:
            self.colour(rva, ctx)

    def colour(self, rva, ctx):
        if rva == LEAF_ENTER:
            conflict = self.conflict(ctx.edi)
            tree = self.word(ctx.ebp - 0x28)
            temp = self.name(self.word(tree + 8))
            graph = self.graph()
            conflict.update(first_ir=graph.get(conflict["first"]), last_ir=graph.get(conflict["last"]))
            self.active_choice = {"event": "colour", "temp": temp, "conflict": conflict,
                                  "excluding": self.word(ctx.ebp - 0x24),
                                  "required": bool(self.read(ctx.ebp - 0x10, 1)[0]), "choices": []}
            if conflict["first_ir"] and conflict["first_ir"][2] != 0x4b:
                self.active_choice["first_instruction"] = self.instruction(conflict["first"], conflict["first_ir"])
            self.replace(LEAF_ENTER, COLOUR_NEXT)
        elif rva == COLOUR_NEXT:
            mask = self.word(self.word(ctx.ebp - 0x1c))
            if not mask:
                cost_check = (not self.active_choice["required"] and self.word(ctx.ebp - 0x14)
                              and not self.read(ctx.ebp - 4, 1)[0])
                self.replace(COLOUR_NEXT, COLOUR_COST if cost_check else COLOUR_LEAVE)
                return
            row = self.active_choice
            candidate = {"register_mask": mask, "register": register(mask),
                         "previously_used": self.global_word(GIVEN) & mask == mask}
            row["choices"].append(candidate)
            if mask & row["conflict"]["unavailable_mask"]:
                candidate["rejected"] = "live-register interference"
            elif mask & row["excluding"]:
                candidate["rejected"] = "excluded by caller"
            else:
                self.replace(COLOUR_NEXT, COLOUR_GREED)
        elif rva == COLOUR_GREED:
            self.active_choice["choices"][-1]["eligibility_code"] = ctx.eax & 255
            self.replace(COLOUR_GREED, COLOUR_NEXT if ctx.eax & 255 else COLOUR_SCORE)
        elif rva == COLOUR_SCORE:
            choice = self.active_choice["choices"][-1]
            score = signed(ctx.eax)
            best_score = signed(self.word(ctx.ebp - 0x18))
            best = self.word(ctx.ebp - 0x14)
            best_used = self.global_word(GIVEN) & best == best
            choice.update(score=score, best_at_evaluation=register(best), best_score_at_evaluation=best_score)
            if score == best_score:
                choice["tie_decision"] = ("prefer already-used register" if choice["previously_used"] and not best_used
                                          else "keep incumbent")
            self.replace(COLOUR_SCORE, COLOUR_NEXT)
        elif rva == COLOUR_COST:
            self.active_choice["cost_check_passed"] = bool(ctx.eax & 255)
            self.replace(COLOUR_COST, COLOUR_LEAVE)
        elif rva == COLOUR_LEAVE:
            row = self.active_choice
            row["chosen"] = register(ctx.edx)
            row["chosen_mask"] = ctx.edx
            row["best_before_cost_check"] = register(self.word(ctx.ebp - 0x14))
            self.events.append(row)
            self.active_choice = None
            self.replace(COLOUR_LEAVE, LEAF_ENTER)
        else:
            raise RuntimeError(f"unexpected allocator hook at {rva:#x}")


def worker_output(path):
    path = Path(path).resolve()
    if not path.is_relative_to(WORKER.resolve()):
        raise ValueError(f"output must be under {WORKER}")
    return path


def settings(source, unit, profile, includes=(), extra=()):
    spec = load_json(ROOT / "config/units.json")["units"][unit] if unit else {}
    source = Path(source).resolve() if source else ROOT / spec["source"]
    profile = profile or spec.get("profile", "game")
    config = wcc.profile(profile)
    if config["install"] != "wc95b":
        raise ValueError("only Watcom 9.5b profiles are supported")
    flags = [*spec.get("flags_override", config["flags"]), *spec.get("extra_flags", []), *extra]
    includes = [*(ROOT / path for path in spec.get("include", [])), *map(Path, includes)]
    return source, profile, flags, includes


def projection(data):
    module, = omf.parse_object(data)
    segments = {i: s for i, s in enumerate(module.segments) if s and s.cls.upper() == "CODE"}
    return ([(i, bytes(s.data), bytes(s.covered)) for i, s in segments.items()],
            [p for p in module.publics if p[1] in segments],
            [(f, module.target_name(f)) for f in module.fixups if f.seg in segments])


def compile95(source, output, flags, includes, scratch, host="dos", observer=None):
    old_stage, old_dos = wcc.STAGE_ROOT, doshost.WORKER
    adapter = wcc.subprocess
    old_host = os.environ.get("LAMEDUKE_HOST")
    try:
        wcc.STAGE_ROOT, doshost.WORKER = scratch / "cc", scratch / "dos"
        os.environ["LAMEDUKE_HOST"] = host
        if observer:
            install, base = wcc.install("wc95b")
            source_exe = base / install["wcc386"]["nt"]
            copied_exe = scratch / "WCC386.EXE"
            shutil.copyfile(source_exe, copied_exe)
            if hashlib.sha256(copied_exe.read_bytes()).hexdigest() != COMPILER_SHA256:
                raise RuntimeError("hook map does not apply to this compiler hash")

            def run(cmd, **kwargs):
                return observer.run([str(copied_exe), *cmd[1:]], **kwargs)

            wcc.subprocess = SimpleNamespace(run=run)
        rc, diagnostics = wcc.compile_c(source, output, flags, "wc95b", includes)
        if rc:
            raise RuntimeError(f"{source}: compiler exited {rc}\n{diagnostics}")
    finally:
        wcc.STAGE_ROOT, doshost.WORKER, wcc.subprocess = old_stage, old_dos, adapter
        if old_host is None:
            os.environ.pop("LAMEDUKE_HOST", None)
        else:
            os.environ["LAMEDUKE_HOST"] = old_host


def trace(source, function, profile, flags, include, output, dos=True):
    output = worker_output(output)
    observer = AllocatorTrace(function)
    with tempfile.TemporaryDirectory(prefix="t", dir=WORKER) as tmp:
        scratch = Path(tmp)
        obj = scratch / "out.obj"
        compile95(source, obj, flags, include, scratch, "nt", observer)
        traced = obj.read_bytes()
        compile95(source, obj, flags, include, scratch, "nt")
        ordinary = obj.read_bytes()
        code, publics, fixups = projection(traced)
        if projection(ordinary) != (code, publics, fixups):
            raise RuntimeError("traced and ordinary NT code/publics/fixups differ")
        if not observer.finished or not observer.events:
            named = ", ".join(name for name in observer.order if name)
            raise RuntimeError(f"no completed allocator trace for {function!r}; procedures: {named}")
        if dos:
            compile95(source, obj, flags, include, scratch)
            if projection(obj.read_bytes()) != (code, publics, fixups):
                raise RuntimeError("traced NT and canonical DOS code/publics/fixups differ")
        result = {
            "format": 1, "function": function, "source": str(source),
            "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(), "profile": profile,
            "flags": flags, "includes": [str(p.resolve()) for p in include],
            "compiler_sha256": COMPILER_SHA256, "nt_code_equal": True,
            "whole_nt_omf_equal": traced == ordinary,
            "dos_code_equal": True if dos else None,
            "object_sha256": hashlib.sha256(obj.read_bytes()).hexdigest(),
            "events": observer.events,
        }
        output.parent.mkdir(parents=True, exist_ok=True)
        output.with_suffix(".obj").write_bytes(obj.read_bytes())
        dump_json(output, result)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", nargs="?", type=Path)
    parser.add_argument("function")
    parser.add_argument("--unit")
    parser.add_argument("--profile")
    parser.add_argument("-i", action="append", default=[])
    parser.add_argument("--flag", action="append", default=[])
    parser.add_argument("--out", type=Path, required=True, help="JSON path under the cg95 worker directory")
    parser.add_argument("--nt-only", action="store_true", help="skip the canonical DOS code/fixup control")
    args = parser.parse_args()
    if not (args.source or args.unit) or args.out.suffix.lower() != ".json":
        parser.error("supply a source file or --unit, and use a .json output")
    try:
        source, profile, flags, includes = settings(args.source, args.unit, args.profile, args.i, args.flag)
        result = trace(source, args.function, profile, flags, includes, args.out, not args.nt_only)
    except (RuntimeError, ValueError, OSError, KeyError) as exc:
        parser.exit(1, f"{exc}\n")
    print(f"{args.function}: {len(result['events'])} events; saved {args.out}")


if __name__ == "__main__":
    main()

