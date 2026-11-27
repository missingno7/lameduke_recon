#!/usr/bin/env python3
"""Search safe C spellings with Watcom 9.5 and the repository verifier."""
import argparse
from contextlib import redirect_stdout
from dataclasses import dataclass
from difflib import unified_diff
from hashlib import sha256
from itertools import islice
import json
from io import StringIO
import os
from pathlib import Path
import re
import subprocess
import sys
from time import perf_counter

sys.dont_write_bytecode = True
ROOT = next(p for p in Path(__file__).resolve().parents if (p / "config/units.json").is_file())
HERE = Path(__file__).resolve().parent
DEFAULT_WORK = HERE.parent if HERE.name == "tools_proposed" else ROOT / "build/permute"
WORK = Path(os.environ.get("PERMUTE_WORK", str(DEFAULT_WORK))).resolve()
sys.path.insert(0, str(ROOT / "tools"))

import blockdiff
import doshost
import match
import omf
import wcc
from common import CONFIG, dump_json, load_json
from c_subset import IDENT, Source, Unsupported
from guidance import focus_from_report, trace_report
from object_model import object_model
from transforms import NAMES, edits


DIRECTIVE = re.compile(r"^[ \t]*#[ \t]*(define|include|pragma)\b([^\r\n]*)", re.M)
INCLUDE = re.compile(r'\s*(?:<([^>]+)>|"([^"]+)")')
PP_TRIVIA = re.compile(r"""(?P<comment>/\*(?:[^*]|\*(?!/))*\*/|//[^\r\n]*)|"(?:[^"\\\r\n]|\\.)*"|'(?:[^'\\\r\n]|\\.)*'""")
LOCATION = {"__LINE__", "__COUNTER__"}


def macro_context(source, includes, watcom, flags):
    """Overapproximate definitions from both arms of conditional includes."""
    stage = {}
    for directory in reversed(includes):
        for path in directory.iterdir():
            if path.is_file() and path.suffix.lower() in (".c", ".h", ".inc"):
                stage[path.name.lower()] = path
    seen, definitions, replacements = set(), {}, {}

    def visit(path):
        if path in seen:
            return
        seen.add(path)
        text = path.read_bytes().decode("latin1").replace("\\\r\n", "").replace("\\\n", "")
        text = PP_TRIVIA.sub(lambda m: re.sub(r"[^\r\n]", " ", m[0]) if m["comment"] else m[0], text)
        for directive, body in DIRECTIVE.findall(text):
            if directive == "include":
                m = INCLUDE.match(body)
                if not m:
                    raise Unsupported(f"{path.name}: computed #include requires preprocessing")
                name = (m[1] or m[2]).replace("\\", "/").lower()
                header = stage.get(name, watcom / name)
                if header.is_file():
                    visit(header)
            elif directive == "define":
                m = re.match(r"\s*([A-Za-z_]\w*)(.*)", body)
                if m:
                    name, value = m.groups()
                    replacements.setdefault(name, set()).add(value.strip())
                    body = PP_TRIVIA.sub(" ", value)
                    definitions.setdefault(name, set()).update(re.findall(r"[A-Za-z_]\w*", body))
                    if any(token in body for token in ("{", "}", ";")):
                        definitions[name].add("<statement tokens>")
                    # Token pasting can synthesize a location-sensitive macro.
                    if "##" in value:
                        definitions[name].update(("__LINE__", "##"))
            elif re.match(r"\s*include_alias\b", body):
                raise Unsupported(f"{path.name}: #pragma include_alias requires preprocessing")

    visit(source)
    for flag in flags:
        if flag.lower().startswith("-d"):
            name, _, value = flag[2:].partition("=")
            if IDENT.fullmatch(name):
                replacements.setdefault(name, set()).add(value if "=" in flag else "1")
                definitions.setdefault(name, set()).update(re.findall(r"[A-Za-z_]\w*", value))
    locations = LOCATION.copy()
    while True:
        more = {name for name, deps in definitions.items() if deps & locations} - locations
        if not more:
            break
        locations.update(more)
    for name in definitions:
        closure = definitions[name].copy()
        while True:
            more = set().union(*(definitions.get(dep, ()) for dep in closure)) - closure
            if not more:
                break
            closure.update(more)
        definitions[name] = closure
    storage = {name for name, forms in replacements.items() if forms <= {"", "extern", "static"}}
    return set(definitions) | LOCATION, locations, definitions, seen, storage


@dataclass
class Settings:
    unit: str | None
    source: Path
    profile: str
    install: str
    flags: list
    includes: list
    symbols: Path | None


def settings(args):
    units = load_json(CONFIG / "units.json")["units"]
    if args.input in units:
        name = args.input
        row = units[name]
        if row.get("kind") != "c":
            raise ValueError(f"{name}: expected a registered C unit")
        canonical = ROOT / row["source"]
        source = args.source or canonical
        profile = args.profile or row["profile"]
        includes = [ROOT / inc for inc in row.get("include", [])]
    else:
        if args.source or not args.profile:
            raise ValueError("an unregistered C path requires --profile; --source overrides a registered unit")
        name, row = None, {}
        source = canonical = Path(args.input)
        profile, includes = args.profile, []
    source = source.resolve(strict=True)
    if not source.is_file() or source.suffix.lower() != ".c":
        raise ValueError(f"expected a C source file, got {source}")
    conf = wcc.profile(profile)
    if conf["install"] != "wc95b":
        raise ValueError("the supported model is the 32-bit Watcom 9.5 GA compiler (wc95b)")
    flags = list(conf["flags"])
    if not args.profile:
        flags = list(row.get("flags_override", flags)) + row.get("extra_flags", [])
    includes = list(dict.fromkeys(p.resolve(strict=True)
                                  for p in [source.parent, *args.include, *includes, canonical.parent]))
    if not all(path.is_dir() for path in includes):
        raise ValueError("every include path must be a directory")
    overlay = args.symbols.resolve(strict=True) if args.symbols else None
    return Settings(name, source, profile, conf["install"], flags, includes, overlay)


def location_uses(src, sensitive):
    return [(t.value, src.text.count("\n", 0, t.start)) for t in src.tokens
            if t.value in sensitive or t.kind == "directive" and (
                re.match(r"\s*#\s*line\b", t.value) or
                sensitive.intersection(re.findall(r"[A-Za-z_]\w*", t.value)) and
                re.match(r"\s*#\s*(if|elif|include)\b", t.value))]


def make_diff(original, selected, name):
    output = []
    for line in unified_diff(StringIO(original).readlines(), StringIO(selected).readlines(),
                             fromfile=f"a/{name}", tofile=f"b/{name}"):
        output.append(line)
        if not line.endswith("\n"):
            output.append("\n\\ No newline at end of file\n")
    return "".join(output)


def strict(rows):
    return {name for name, r in rows.items() if r["status"] == "STRICT"}


def focus_names(args, source, cfg, out):
    manual = set(args.focus.split(",")) if args.focus else set()
    functions = {fn: source.function(fn) for fn in args.functions}
    reports, traces = {}, {}
    for path in args.cg95_explain or ():
        report = load_json(path)
        fn = report["function"]
        if fn not in functions or fn in reports:
            raise ValueError(f"{path}: expected a distinct search target")
        reports[fn] = report
    if args.cg95:
        for fn in functions:
            reports[fn], traces[fn] = trace_report(ROOT, WORK, out, cfg, fn)
    allowed = set().union(*(fun.uses for fun in functions.values()))
    if manual - allowed:
        raise ValueError("focus names absent from targets: " + ", ".join(sorted(manual - allowed)))
    decisions = {}
    for fn, fun in functions.items():
        report = reports.get(fn)
        named = focus_from_report(fun, report, traces.get(fn)) if report else set()
        explicit = manual & fun.uses.keys()
        guided = args.cg95 or args.cg95_explain or args.focus
        chosen = named if named else explicit
        decisions[fn] = {
            "names": sorted(chosen) if guided else None,
            "basis": ("trace" if named else "manual" if explicit else "unresolved" if guided else "unrestricted"),
            "decision": report.get("first_attributed") if report else None,
            "first_aligned_difference": report.get("first_aligned_difference") if report else None,
            "classification": report.get("classification") if report else None,
        }
    return decisions


def round_robin(iterators):
    while iterators:
        active = []
        for stream in iterators:
            item = next(stream, None)
            if item is not None:
                yield item
                active.append(stream)
        iterators = active


def proposals(src, targets, transforms, focus):
    streams = []
    for name in targets:
        names = focus[name]["names"]
        if names == []:
            continue
        fun = src.function(name)
        for transform in transforms:
            def stream(fun=fun, transform=transform, names=names):
                for edit in edits(fun, {transform}):
                    if names is None or edit.variables.intersection(names):
                        if any(src.text[a:b] != text for a, b, text in edit.changes):
                            yield fun.name, edit
            streams.append(stream())
    yield from round_robin(streams)


def select_frontier(candidates, width):
    ranked = sorted(candidates, key=lambda c: (c.result.score, c.id))
    unique, repeats, seen = [], [], set()
    for candidate in ranked:
        digest = candidate.result.object_hash
        if digest in seen:
            repeats.append(candidate)
        else:
            seen.add(digest)
            unique.append(candidate)
    # Keep different objects first among equally scored sources. Retain
    # neutral source differences when the beam has room for them.
    return sorted(unique + repeats, key=lambda c: c.result.score)[:width]


@dataclass
class Result:
    id: int
    object_hash: str
    score: tuple
    targets: dict
    lost: list
    strict_names: set
    strict_bytes: int
    counts: dict

    def record(self):
        return {"object_sha256": self.object_hash, "score": self.score,
                "targets": self.targets, "lost_strict": self.lost,
                "unit_counts": self.counts, "strict_bytes": self.strict_bytes}


@dataclass
class Candidate:
    id: int
    text: str
    result: Result


class Search:
    def __init__(self, args, cfg, out, log):
        self.args, self.cfg, self.out, self.log = args, cfg, out, log
        self.targets = set(args.functions)
        self.protection = {}
        self.object_cache = {"nt": {}, "dos": {}}
        self.dos_tested = {}
        self.times = []
        self.nt_attempts = 0
        self.source_seen = set()
        self.shortlist = []
        self.dos_best = None
        self.best_dos_object = None
        self.trajectory = []
        self.next_id = 1
        self.workspace = out / "candidate"
        self.workspace.mkdir()
        self.file = self.workspace / cfg.source.name
        self.obj = out / "trial.obj"
        match.Bindings.extra = str(cfg.symbols) if cfg.symbols else None
        self.analyzer = blockdiff.Analyzer()
        # Match the repository wrappers while keeping every mutable path in WORK.
        wcc.STAGE_ROOT = WORK / "cc"
        doshost.WORKER = WORK / "dos"

    def write_log(self, row):
        self.log.write(json.dumps(row, ensure_ascii=True) + "\n")
        self.log.flush()

    def evaluate(self, text, host, ident, parent=None, fn=None, edit=None):
        began = perf_counter()
        data = text.encode("latin1")
        record = {"id": ident, "parent": parent, "host": host, "function": fn,
                  "source_sha256": sha256(data).hexdigest()}
        if edit:
            record.update(transform=edit.kind, detail=edit.detail,
                          variables=sorted(edit.variables), edits=edit.changes)
        self.file.write_bytes(data)
        os.environ["LAMEDUKE_HOST"] = host
        result = None
        try:
            started = perf_counter()
            rc, log = wcc.compile_c(self.file, self.obj, self.cfg.flags, self.cfg.install, self.cfg.includes)
            record.update(compile_seconds=round(perf_counter() - started, 6), compiler_exit=rc)
            if rc:
                raise RuntimeError(log)
            digest = sha256(self.obj.read_bytes()).hexdigest()
            cache = self.object_cache[host]
            if digest in cache:
                result = cache[digest]
                record.update(object_sha256=digest, duplicate_of=result.id)
            else:
                record["object_sha256"] = digest
                result = self.score(ident, digest, host)
                cache[digest] = result
                record.update(result.record())
        except (OSError, ValueError, subprocess.TimeoutExpired, RuntimeError) as exc:
            record["error"] = str(exc)
        finally:
            record["elapsed_seconds"] = round(perf_counter() - began, 6)
            if host == "nt" and ident:
                self.times.append(record["elapsed_seconds"])
            self.write_log(record)
        return result

    def score(self, ident, digest, host):
        module, = omf.load(self.obj)
        rows = match.verify_object(self.analyzer.img, module)
        by_name = {r["function"]: r for r in rows}
        if len(by_name) != len(rows):
            raise ValueError("ambiguous function names in OMF publics")
        if self.targets - by_name.keys() or any("oracle" not in by_name[name] for name in self.targets):
            raise ValueError("at least one target is absent or has no independent binding")
        strict_names = strict(by_name)
        lost = sorted(self.protection.get(host, set()) - strict_names)
        keys = ("status", "oracle", "byte_diffs", "first_diff", "size", "problems", "unverified", "verified_bytes")
        targets = {name: {k: by_name[name][k] for k in keys if k in by_name[name]} for name in self.args.functions}
        near = self.targets - strict_names
        metrics = {name: {"content": 1.0, "regs": 1.0} for name in self.targets - near}
        if near:
            for pair in self.analyzer.object_pairs(self.obj, only=near):
                report = blockdiff.metrics(pair.orig, pair.cand)
                metrics[pair.name] = {key: report[key] for key in ("content", "regs")}
        for name, row in targets.items():
            row["blockdiff"] = metrics.get(name, {"content": 0.0, "regs": 0.0})
        score = (len(lost), len(near), sum(r["byte_diffs"] for r in targets.values()),
                 -sum(r["blockdiff"]["content"] for r in targets.values()),
                 -sum(r["blockdiff"]["regs"] for r in targets.values()),
                 sum(len(r["problems"]) + len(r["unverified"]) for r in targets.values()))
        counts = {s: sum(row["status"] == s for row in rows)
                  for s in ("STRICT", "CODE_BYTES_MATCH", "NEAR_MATCH", "UNPLACED")}
        return Result(ident, digest, score, targets, lost, strict_names,
                      sum(r["verified_bytes"] for r in rows if r["status"] == "STRICT"), counts)

    def confirm(self, candidate):
        if candidate.id in self.dos_tested:
            return self.dos_tested[candidate.id]
        result = self.evaluate(candidate.text, "dos", candidate.id)
        self.dos_tested[candidate.id] = result
        if result and not result.lost and (not self.dos_best or result.score < self.dos_best.result.score):
            self.dos_best = Candidate(candidate.id, candidate.text, result)
            self.best_dos_object = self.obj.read_bytes()
            self.improve(candidate.id, "dos", result)
        return result

    def improve(self, ident, host, result):
        self.trajectory.append({"id": ident, "host": host, **result.record()})
        print(f"{host.upper()} #{ident} {result.score} " +
              ", ".join(f"{fn} {r['status']} ({r['byte_diffs']} differing bytes)"
                        for fn, r in result.targets.items()), flush=True)

    def baseline(self, original):
        dos = self.evaluate(original, "dos", 0)
        if dos is None:
            raise RuntimeError("DOS baseline failed; see variants.jsonl")
        self.best_dos_object = self.obj.read_bytes()
        nt = self.evaluate(original, "nt", 0)
        if nt is None:
            raise RuntimeError("NT baseline failed; see variants.jsonl")
        self.protection["dos"] = dos.strict_names - self.targets
        self.protection["nt"] = self.protection["dos"] & nt.strict_names
        self.dos_tested[0] = dos
        self.dos_best = Candidate(0, original, dos)
        self.improve(0, "dos", dos)
        self.improve(0, "nt", nt)
        self.source_seen.add(sha256(original.encode("latin1")).digest())
        return Candidate(0, original, nt)

    def trial(self, parent, fn, edit, source_factory, sensitive, original_locations):
        text = edit.apply(parent.text)
        digest = sha256(text.encode("latin1")).digest()
        if digest in self.source_seen:
            return None
        self.source_seen.add(digest)
        if original_locations and location_uses(source_factory(text), sensitive) != original_locations:
            self.write_log({"host": "source", "parent": parent.id, "function": fn, "edits": edit.changes,
                            "transform": edit.kind, "skipped": "location-sensitive macro or #line would move"})
            return None
        ident = self.next_id
        self.next_id += 1
        self.nt_attempts += 1
        result = self.evaluate(text, "nt", ident, parent.id, fn, edit)
        return Candidate(ident, text, result) if result else None

    def remember(self, candidate):
        if candidate.result.lost or any(c.result.object_hash == candidate.result.object_hash for c in self.shortlist):
            return
        self.shortlist.append(candidate)
        self.shortlist.sort(key=lambda c: (c.result.score, c.id))
        del self.shortlist[self.args.confirm:]

    def run(self, original, source_factory, sensitive, transforms, focus):
        baseline = self.baseline(original)
        original_source = source_factory(original)
        location = location_uses(original_source, sensitive)
        first = list(islice(proposals(original_source, self.args.functions, transforms, focus),
                            self.args.exhaustive_limit + 1))
        strategy = self.args.strategy
        if strategy == "auto":
            strategy = "exhaustive" if len(first) <= self.args.exhaustive_limit else "beam"
        self.write_log({"event": "search", "strategy": strategy, "focus": focus,
                        "protected_dos": sorted(self.protection["dos"]), "protected_nt": sorted(self.protection["nt"])})
        if not first:
            return strategy, "no applicable edits"
        if baseline.result.score[1] == 0 and self.dos_best.result.score[1] == 0 and not self.args.keep_going:
            return strategy, "baseline already STRICT"
        frontier, best = [baseline], baseline
        reason = "depth exhausted"
        for depth in range(1, self.args.depth + 1):
            next_level = []
            for position, parent in enumerate(frontier):
                src = original_source if parent.id == 0 else source_factory(parent.text)
                stream = proposals(src, self.args.functions, transforms, focus)
                remaining = self.args.budget - self.nt_attempts
                quota = remaining if strategy == "exhaustive" else max(
                    1, remaining // (len(frontier) - position + self.args.depth - depth))
                started = self.nt_attempts
                for fn, edit in stream:
                    if self.nt_attempts >= self.args.budget or self.nt_attempts - started >= quota:
                        break
                    candidate = self.trial(parent, fn, edit, source_factory, sensitive, location)
                    if not candidate:
                        continue
                    result = candidate.result
                    if strategy == "exhaustive" or not result.lost:
                        next_level.append(candidate)
                    self.remember(candidate)
                    if result.score < best.result.score:
                        best = candidate
                        self.improve(candidate.id, "nt", result)
                    if result.score[:2] == (0, 0) and not self.args.keep_going:
                        verified = self.confirm(candidate)
                        if verified and verified.score[:2] == (0, 0):
                            return strategy, "DOS-confirmed STRICT"
                if self.nt_attempts >= self.args.budget:
                    reason = "compile budget exhausted"
                    break
            if not next_level or self.nt_attempts >= self.args.budget:
                if not next_level and reason != "compile budget exhausted":
                    reason = "no new candidates"
                break
            if strategy != "exhaustive":
                frontier = select_frontier(next_level, 1 if strategy == "greedy" else self.args.beam_width)
            else:
                frontier = next_level
        for candidate in self.shortlist:
            self.confirm(candidate)
        return strategy, reason

    def finish(self, original, strategy, reason):
        selected = self.dos_best
        name = self.cfg.source.name
        patch = make_diff(original, selected.text, name)
        (self.out / "best.diff").write_bytes(patch.encode("latin1"))
        (self.out / "best").mkdir()
        (self.out / "best" / name).write_bytes(selected.text.encode("latin1"))
        (self.out / "best.obj").write_bytes(self.best_dos_object)
        self.file.unlink()
        self.workspace.rmdir()
        self.obj.unlink(missing_ok=True)
        self.write_log({"event": "finish", "strategy": strategy, "reason": reason, "selected": selected.id})
        dump_json(self.out / "trajectory.json", self.trajectory)
        dump_json(self.out / "summary.json", {
            "strategy": strategy, "reason": reason, "selected": selected.id, "best": selected.result.record(),
            "dos_baseline": self.dos_tested[0].record(), "protected_dos": sorted(self.protection["dos"]),
            "nt_guard_inconclusive": sorted(self.protection["dos"] - self.protection["nt"]),
            "nt_variants": self.nt_attempts, "nt_distinct_objects": len(self.object_cache["nt"]),
            "dos_confirmations": len(self.dos_tested) - 1,
            "mean_nt_trial_seconds": round(sum(self.times) / len(self.times), 6) if self.times else None,
        })


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", help="registered unit name or path to a C source")
    parser.add_argument("functions", nargs="+", help="bound C function names")
    parser.add_argument("--source", type=Path, help="use this source copy with the registered unit settings")
    parser.add_argument("--profile", help="override the compiler profile, or select one for a source path")
    parser.add_argument("-i", "--include", type=Path, action="append", default=[])
    parser.add_argument("--symbols", type=Path, help="standard match.py binding overlay")
    parser.add_argument("--out", type=Path, required=True, help="new output directory under PERMUTE_WORK (default build/permute)")
    parser.add_argument("--transforms", default="all", help="comma-separated: " + ", ".join(NAMES))
    parser.add_argument("--strategy", choices=("auto", "exhaustive", "greedy", "beam"), default="auto")
    parser.add_argument("--focus", help="comma-separated source identifiers to touch")
    parser.add_argument("--cg95-explain", type=Path, action="append", help="use named variables from explain95 JSON")
    parser.add_argument("--cg95", action="store_true", help="trace and explain each target's allocation before searching")
    parser.add_argument("--budget", type=int, default=100, help="maximum NT variant compilations")
    parser.add_argument("--depth", type=int, default=2, help="maximum successive source edits")
    parser.add_argument("--beam-width", type=int, default=4)
    parser.add_argument("--exhaustive-limit", type=int, default=24, help="initial neighbourhood limit for auto mode")
    parser.add_argument("--confirm", type=int, default=3, help="DOS shortlist size")
    parser.add_argument("--keep-going", action="store_true", help="continue after all targets reach STRICT")
    args = parser.parse_args()
    args.functions = list(dict.fromkeys(args.functions))
    if args.cg95 and args.cg95_explain:
        parser.error("--cg95 and --cg95-explain are alternatives")
    transforms = NAMES if args.transforms == "all" else tuple(dict.fromkeys(args.transforms.split(",")))
    if not set(transforms) <= set(NAMES):
        parser.error("unknown transform: " + ", ".join(sorted(set(transforms) - set(NAMES))))
    if min(args.budget, args.depth, args.beam_width, args.exhaustive_limit, args.confirm) < 1:
        parser.error("budget, depth, beam width, exhaustive limit and confirm count must be positive")
    out = args.out.resolve()
    if not out.is_relative_to(WORK.resolve()) or out == WORK.resolve():
        parser.error(f"--out must be a new directory below {WORK}")
    try:
        cfg = settings(args)
        data = cfg.source.read_bytes()
        original = data.decode("latin1")
        _, install = wcc.install(cfg.install)
        macros, sensitive, expansion, paths, storage = macro_context(cfg.source, cfg.includes, install / "H", cfg.flags)
        objects, members = object_model(sorted(paths), macros, storage)

        def source_factory(text):
            return Source(text, macros, objects, members, expansion)

        out.mkdir(parents=True, exist_ok=False)
        focus = focus_names(args, source_factory(original), cfg, out)
        dump_json(out / "run.json", {
            "unit": cfg.unit, "input": str(cfg.source), "input_sha256": sha256(data).hexdigest(),
            "targets": args.functions, "profile": cfg.profile, "install": cfg.install, "flags": cfg.flags,
            "include": list(map(str, cfg.includes)), "symbols": str(cfg.symbols) if cfg.symbols else None,
            "transforms": transforms, "focus": focus, "strategy": args.strategy, "depth": args.depth,
            "budget": args.budget, "beam_width": args.beam_width, "exhaustive_limit": args.exhaustive_limit,
            "confirm": args.confirm, "keep_going": args.keep_going,
        })
        with (out / "variants.jsonl").open("w", encoding="utf-8") as log:
            search = Search(args, cfg, out, log)
            strategy, reason = search.run(original, source_factory, sensitive, transforms, focus)
            search.finish(original, strategy, reason)
        print(f"{reason}; {search.nt_attempts} NT variants; results in {out}", flush=True)
    except (OSError, RuntimeError, ValueError, KeyError) as exc:
        parser.exit(1, f"permute: {exc}\n")


if __name__ == "__main__":
    main()
