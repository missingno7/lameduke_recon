"""Inventory local code artefacts, build a reusable function corpus, and rank LameDuke donors."""
import argparse
from bisect import bisect_right
from collections import Counter, defaultdict
from datetime import datetime, timezone
import gzip
from io import BytesIO
import inspect
import json
import math
from pathlib import Path
import re
import struct
import sys
import time
import zipfile
import zlib

sys.dont_write_bytecode = True
ROOT = next(p for p in Path(__file__).resolve().parents if (p / "tools/blockdiff.py").is_file())
WORK = ROOT / "build/binscan"
OUT = WORK / "out"
SOURCE = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / "tools"))

import blockdiff
import omf
from binary_formats import (ar_members, coff_program, fingerprints, identify, linear_program, omf_members)
from binary_features import Extent, collect, digest, symbol
from binary_maps import apply_symbols, map_index


DEFAULT_ROOTS = (ROOT / "external/x", ROOT / "external/duke3d-1.3d")
HANDLED = {"LE", "LX", "PE", "COFF", "COFF/ar archive", "OMF object", "OMF library"}
SUPPLEMENT = {"neartag", "drawline16", "rotatesprite", "screencapture"}
CODE_SUFFIXES = {".exe", ".obj", ".lib", ".com", ".dll", ".sys", ".ovl", ".ocx",
                 ".o", ".drv", ".386", ".vxd", ".coff", ".bin", ".a", ".b", ".c"}
LISTING = re.compile(r"^\s*\d+\s+(\d+)\s+(\d{4}-\d\d-\d\d)\s+(\d\d:\d\d:\d\d)\s+(.+?)\s*$")
WEIGHTS = {"mnemonic_ngrams": .31, "operand_ngrams": .25, "block_content": .14, "loose_blocks": .07,
           "cfg": .06, "call_names_or_signatures": .065, "constants": .065, "data": .04}
PARSE_ERRORS = (ValueError, IndexError, TypeError, KeyError, struct.error, OverflowError, RecursionError)


def display(path):
    return path.relative_to(ROOT).as_posix() if path.is_relative_to(ROOT) else path.as_posix()


def read_json(path):
    return json.loads(path.read_text(encoding="utf-8-sig"))


def write_json(path, data):
    temp = path.with_suffix(path.suffix + ".tmp")
    with temp.open("w", encoding="utf-8", newline="\n") as dest:
        if path.name == "corpus.json":
            dest.write('{"summary":')
            json.dump(data["summary"], dest, ensure_ascii=True)
            dest.write(',\n"functions":[\n')
            for i, row in enumerate(data["functions"]):
                if i:
                    dest.write(",\n")
                dest.write(json.dumps(row, ensure_ascii=True, separators=(",", ":")))
            dest.write("\n]}\n")
        else:
            json.dump(data, dest, ensure_ascii=True, indent=2)
            dest.write("\n")
    temp.replace(path)


def files(roots):
    return sorted({p.resolve() for root in roots for p in root.rglob("*")
                   if ".git" not in p.relative_to(root).parts and p.is_file()}, key=lambda p: str(p).lower())


def listed_dates(paths):
    entries = defaultdict(list)
    for path in paths:
        if path.name.casefold() != "fileslst.txt":
            continue
        for line in path.read_text(encoding="latin-1").splitlines():
            hit = LISTING.match(line)
            if not hit:
                continue
            size, date, clock, name = hit.groups()
            qualified = (path.parent / name.replace("\\", "/")).as_posix().casefold()
            entries[qualified, int(size)].append({
                "timestamp": f"{date}T{clock}", "source": display(path), "basis": "FILESLST archive member",
            })
    return entries


def zip_metadata(data):
    with zipfile.ZipFile(BytesIO(data)) as archive:
        return [{"name": e.filename, "size": e.file_size, "crc32": f"{e.CRC:08x}",
                 "timestamp": datetime(*e.date_time).isoformat()} for e in archive.infolist() if not e.is_dir()]


def extracted_zip_dates():
    results = defaultdict(list)
    directory = ROOT / "external/dl"
    for path in sorted(p for p in directory.rglob("*") if p.suffix.casefold() == ".zip" and p.is_file()):
        try:
            with zipfile.ZipFile(path) as archive:
                entries = archive.infolist()
        except (OSError, zipfile.BadZipFile):
            continue
        for entry in entries:
            if not entry.is_dir():
                key = Path(entry.filename).name.casefold(), entry.file_size, entry.CRC
                value = {"timestamp": datetime(*entry.date_time).isoformat(), "source": display(path),
                         "basis": "ZIP member basename, size and CRC match", "member": entry.filename}
                if value not in results[key]:
                    results[key].append(value)
    return results


def parse_programs(data, kind, meta, errors):
    if kind in {"LE", "LX"}:
        yield linear_program(data, kind)
    elif kind in {"PE", "COFF"}:
        yield coff_program(data, kind)
    elif kind.startswith("OMF"):
        for program, error in omf_members(data, kind == "OMF library"):
            if error:
                errors.append(error)
            else:
                yield program
    elif kind == "ZIP":
        meta["archive_members"] = zip_metadata(data)
    elif kind == "COFF/ar archive":
        meta["archive_members"] = []
        for name, offset, stamp, payload in ar_members(data):
            member_kind = identify(payload, Path(name).suffix.lower())
            meta["archive_members"].append({"name": name, "offset": offset, "timestamp": stamp,
                                             "container": member_kind, "size": len(payload)})
            try:
                if member_kind == "COFF":
                    yield coff_program(payload, "COFF", name, offset)
                if member_kind in {"OMF object", "OMF library"}:
                    for prog, error in omf_members(payload, member_kind == "OMF library"):
                        if error:
                            errors.append(f"{name}: {error}")
                        else:
                            prog.member_offset += offset
                            yield prog
            except PARSE_ERRORS as exc:
                errors.append(f"{name}: {type(exc).__name__}: {exc}")


def parse(data, kind, path):
    info = fingerprints(data, path)
    rows, errors, modules = [], [], []
    try:
        for program in parse_programs(data, kind, info, errors):
            try:
                funcs = list(collect(program))
            except PARSE_ERRORS as exc:
                errors.append(f"{program.module}: {type(exc).__name__}: {exc}")
                continue
            metadata = {k: v for k, v in program.metadata.items() if k != "unwind_extents"}
            metadata["function_count"] = len(funcs)
            metadata["section_modes"] = sorted({reg.bits for reg in program.regions.values() if reg.executable})
            modules.append(metadata)
            rows.extend(funcs)
    except (*PARSE_ERRORS, zipfile.BadZipFile) as exc:
        errors.append(f"{type(exc).__name__}: {exc}")
    if modules:
        info["modules"] = modules
    info["function_count"] = len(rows)
    if errors:
        info["parse_errors"] = errors
    if kind not in HANDLED:
        info["census"] = "inventory only"
    elif errors:
        info["census"] = "partial" if rows else "failed"
    else:
        info["census"] = "parsed"
    return {"metadata": info, "functions": rows}


def parse_key():
    reader_files = [SOURCE / f"{name}.py" for name in ("binary_features", "binary_formats")]
    reader_files.extend(ROOT / "tools" / f"{name}.py" for name in ("blockdiff", "le", "omf"))
    pipeline = "\n".join(inspect.getsource(part) for part in (parse, parse_programs, zip_metadata)).encode()
    return digest(b"\0".join(p.read_bytes() for p in reader_files) + pipeline +
                  blockdiff.capstone.__version__.encode())[:16]


def cache_parse(data, kind, path, sha, cache, symbol_map=None):
    suffix = ""
    if symbol_map:
        reader = (SOURCE / "binary_maps.py").read_bytes()
        suffix = "_" + digest(reader + symbol_map[0]["sha256"].encode())[:16]
    target = cache / (sha + suffix + ".json.gz")
    if target.is_file():
        try:
            with gzip.open(target, "rt", encoding="utf-8") as inp:
                return json.load(inp), True
        except (OSError, ValueError, EOFError):
            pass
    if symbol_map:
        result, _ = cache_parse(data, kind, path, sha, cache)
        meta, symbols = symbol_map
        program = linear_program(data, kind)
        apply_symbols(program, symbols)
        result["functions"] = list(collect(program))
        info = result["metadata"]
        info["linker_map"] = meta
        info["function_count"] = len(result["functions"])
        if info.get("modules"):
            info["modules"][0]["function_count"] = info["function_count"]
    else:
        result = parse(data, kind, path)
    temp = target.with_suffix(".tmp")
    with gzip.open(temp, "wt", encoding="utf-8", compresslevel=1) as dest:
        dest.write(json.dumps(result, separators=(",", ":")))
    temp.replace(target)
    return result, False


def plausible_header(sign, suffix):
    return (suffix in CODE_SUFFIXES or identify(sign, suffix) or sign[:1] in (b"\x80", b"\x82", b"\xf0") or
            sign[:2] in {b"LX", b"LE", b"\x4c\x01", b"\x64\x86"})


def zip_children(data, parent, errors, depth=0):
    if depth == 3:
        errors.append({"path": parent, "error": "ZIP recursion limit reached"})
        return
    try:
        with zipfile.ZipFile(BytesIO(data)) as archive:
            for entry in archive.infolist():
                if entry.is_dir():
                    continue
                label = f"{parent}!{entry.filename}"
                suffix = Path(entry.filename).suffix.lower()
                try:
                    with archive.open(entry) as member:
                        sign = member.read(8)
                        if not plausible_header(sign, suffix):
                            continue
                        if entry.file_size > 64 * 1024**2:
                            errors.append({"path": label, "error": "ZIP member exceeds 64 MiB census limit"})
                            continue
                        child = sign + member.read()
                except (OSError, ValueError, RuntimeError, NotImplementedError, zipfile.BadZipFile, zlib.error) as exc:
                    errors.append({"path": label, "error": f"{type(exc).__name__}: {exc}"})
                    continue
                kind = identify(child, suffix)
                if kind:
                    stamp = {"timestamp": datetime(*entry.date_time).isoformat(), "source": parent,
                             "basis": "ZIP member", "member": entry.filename}
                    yield child, kind, {"path": label, "dates": [stamp], "filesystem_mtime_utc": None}
                if kind == "ZIP":
                    yield from zip_children(child, label, errors, depth + 1)
    except (OSError, zipfile.BadZipFile) as exc:
        errors.append({"path": parent, "error": f"{type(exc).__name__}: {exc}"})


def artefact_inputs(roots, errors, paths=None):
    if paths is None:
        paths = files(roots)
    dates = listed_dates(paths)
    zip_dates = extracted_zip_dates()
    for p in paths:
        suffix, label = p.suffix.lower(), display(p)
        try:
            with p.open("rb") as f:
                sign = f.read(8)
                if not plausible_header(sign, suffix):
                    continue
                data = sign + f.read()
            mtime = datetime.fromtimestamp(p.stat().st_mtime, timezone.utc).isoformat()
        except OSError as exc:
            errors.append({"path": label, "error": f"{type(exc).__name__}: {exc}"})
            continue
        kind = identify(data, suffix)
        if not kind:
            continue
        stamp = list(dates.get((p.as_posix().casefold(), len(data)), ()))
        stamp.extend(zip_dates.get((p.name.casefold(), len(data), zlib.crc32(data)), ()))
        yield data, kind, {"path": label, "dates": stamp, "filesystem_mtime_utc": mtime}
        if kind == "ZIP":
            yield from zip_children(data, label, errors)


def roster(roots, oracle_hash):
    inventory, grouped, errors = {}, {}, []
    paths = files(roots)
    symbol_maps, rejected_maps = map_index(paths, roots, display)
    cache = OUT / "cache" / parse_key()
    cache.mkdir(parents=True, exist_ok=True)
    count = hits = 0
    for data, kind, file_meta in artefact_inputs(roots, errors, paths):
        sha, label = digest(data), file_meta["path"]
        if sha in inventory:
            inventory[sha]["paths"].append(file_meta)
            continue
        count += 1
        row = {"sha256": sha, "size": len(data), "container": kind, "paths": [file_meta]}
        inventory[sha] = row
        if sha == oracle_hash:
            row.update(fingerprints(data, label), census="oracle copy excluded", function_count=0)
            continue
        result, hit = cache_parse(data, kind, label, sha, cache, symbol_maps.get(sha))
        hits += hit
        row.update(result["metadata"])
        for occurrence, stats in result["functions"]:
            occurrence["artefact"] = sha
            key = occurrence["relocation_sha256"] or (
                f"raw:{occurrence['bits']}:{occurrence['sha256']}:{occurrence['relocation_sites']}")
            group = grouped.get(key)
            strength = (bool(occurrence["names"]), len(stats["calls"]) + len(stats["references"]),
                        len(stats["data"]), len(stats["call_signatures"]))
            if group is None:
                group = {"id": key, "size": occurrence["size"], "bits": occurrence["bits"],
                         "occurrences": [], "names": set(), "strength": strength, "stats": stats, "rep": occurrence}
                grouped[key] = group
            group["occurrences"].append(occurrence)
            group["names"].update(symbol(n) for n in occurrence["names"])
            if strength > group["strength"]:
                group.update(strength=strength, stats=stats, rep=occurrence)
        if count % 40 == 0:
            print(f"Parsed {count} unique artefacts ({hits} cache hits), {len(grouped)} function groups", flush=True)
    return inventory, list(grouped.values()), hits, errors, rejected_maps


def runtime_fingerprints(inventory, groups):
    """Corroborate banner evidence with complete Watcom CLIB/MATH member signatures."""
    lib_name = re.compile(r"(?:clib[36]\w*|math\w*|emu387\w*|3[cm](?:[rs].*|87.*))\.(?:lib|[abc]|bak)$", re.I)
    libraries = {
        sha: next((p["path"] for p in row["paths"] if lib_name.fullmatch(p["path"].rsplit("/", 1)[-1])), None)
        for sha, row in inventory.items() if row["container"] == "OMF library"
    }
    counts, examples = Counter(), defaultdict(list)
    for group in groups:
        if group["size"] < 32 or group["id"].startswith("raw:"):
            continue
        origins = [row for row in group["occurrences"] if row["names"] and libraries.get(row["artefact"])]
        if not origins:
            continue
        origin = origins[0]
        for sha in {row["artefact"] for row in group["occurrences"]}:
            if inventory[sha]["container"] not in {"LE", "LX", "PE"}:
                continue
            counts[sha] += 1
            if len(examples[sha]) < 16:
                examples[sha].append({"library": libraries[origin["artefact"]], "member": origin["member"],
                                      "name": origin["names"][0], "bytes": origin["size"], "id": group["id"]})
    for sha, count in counts.items():
        inventory[sha]["runtime_signature_evidence"] = {
            "matching_watcom_clib_math_variants": count, "examples": examples[sha],
            "basis": "matching code and relocation-site shape; runtime family fingerprint",
        }


def oracle_targets():
    analyzer = blockdiff.Analyzer()
    program = linear_program(analyzer.img.le.file, "LE")
    verify_doc = read_json(ROOT / "build/verify.json")
    units = verify_doc["units"]
    verify, last = {}, set()
    for unit, rows in units.items():
        by_address = []
        by_name = {}
        for name, row in rows.items():
            if not isinstance(row, dict) or "oracle" not in row:
                continue
            obj, offset = blockdiff.match.parse_addr(row["oracle"])
            verify[obj, offset] = (unit, name, row)
            by_address.append((obj, offset))
            by_name[symbol(name)] = (obj, offset)
        if by_address:
            obj_path = ROOT / "build/verify" / f"{unit}.obj"
            try:
                if not obj_path.is_file():
                    raise ValueError("missing verified object")
                before = len(last)
                for mod in omf.load(obj_path):
                    terminal = {}
                    for name, seg, offset, _ in mod.publics:
                        terminal[seg] = max(terminal.get(seg, -1), offset)
                    for name, seg, offset, _ in mod.publics:
                        if offset == terminal[seg] and symbol(name) in by_name:
                            last.add(by_name[symbol(name)])
                if len(last) == before:
                    last.add(max(by_address))
            except PARSE_ERRORS:
                last.add(max(by_address))
    for name, address in analyzer.binds.lib.items():
        program.regions[address[0]].publics[address[1]].append(name)
    registry = read_json(ROOT / "config/symbols.json")["d3d"]
    for section in ("functions", "data"):
        for name, binding in registry[section].items():
            obj, offset = blockdiff.match.parse_addr(binding["addr"])
            program.regions[obj].publics[offset].append(name)
    spans = []
    targets = defaultdict(list)
    for name, binding in registry["functions"].items():
        obj, start = blockdiff.match.parse_addr(binding["addr"])
        if not program.regions[obj].executable:
            raise ValueError(f"Bound function {name} is outside executable code")
        targets[obj, start].append(name)
    # Preserve the same last-public treatment as blockdiff for built units.
    for obj, start in sorted(targets):
        unit, name, row = verify.get((obj, start), (None, None, {}))
        size = row.get("size")
        if obj == 1:
            candidate = size if size else len(program.regions[obj].data) - start + 1
            code, confirmed = analyzer.oracle_extent(start, candidate, (obj, start) in last)
        else:
            points = sorted(x[1] for x in targets if x[0] == obj)
            bound = bisect_right(points, start)
            end = points[bound] if bound < len(points) else len(program.regions[obj].data)
            code, confirmed = program.regions[obj].data[start:end], False
        basis = "blockdiff confirmed candidate end" if confirmed else "blockdiff next known boundary"
        spans.append(Extent(obj, start, start + len(code), basis, ("oracle binding",)))
    # Additional runtime bindings provide local callee signatures without expanding the target roster.
    for start in analyzer.starts:
        if (1, start) in targets:
            continue
        i = bisect_right(analyzer.starts, start)
        end = analyzer.starts[i] if i < len(analyzer.starts) else len(program.regions[1].data)
        if 0 <= start < end <= len(program.regions[1].data):
            spans.append(Extent(1, start, end, "runtime name boundary", ("context",)))
    result = []
    for occurrence, stats in collect(program, spans):
        key = occurrence["region"], occurrence["offset"]
        if key not in targets:
            continue
        unit, verified_name, row = verify.get(key, (None, None, {}))
        for name in targets[key]:
            result.append({"name": name, "verified_name": verified_name,
                           "address": f"{key[0]}:{key[1]:05x}", "unit": unit, "status": row.get("status", "IDENTIFIED"),
                           "verify_size": row.get("size"), "occurrence": occurrence, "stats": stats})
    return result


def add_indices(row):
    stat = row["stats"]
    row["counters"] = {key: dict(stat[key]) for key in ("mn", "op", "data", "constants", "calls", "call_signatures")}
    row["counts"] = {key: sum(values.values()) for key, values in row["counters"].items()}
    row["blocksets"] = {key: Counter(k for k, _ in stat[key]) for key in ("blocks", "loose", "named_blocks")}


def ratio(a, b):
    return min(a, b) / max(a, b) if a or b else None


def dice(left, right, a, b):
    size = left["counts"][a] + right["counts"][b]
    if not size:
        return None
    x, y = left["counters"][a], right["counters"][b]
    if len(x) > len(y):
        x, y = y, x
    return 2 * sum(min(n, y[k]) for k, n in x.items() if k in y) / size


def block_overlap(left, right, key):
    # Equivalent to blockdiff's one-to-one block-content coverage, symmetrized by bytes.
    ours, theirs = left["stats"][key], right["stats"][key]
    total = sum(n for _, n in ours) + sum(n for _, n in theirs)
    if not total:
        return None
    cover = 0
    for rows, available in ((ours, right["blocksets"][key]), (theirs, left["blocksets"][key])):
        used = Counter()
        for tag, size in rows:
            if used[tag] < available[tag]:
                used[tag] += 1
                cover += size
    return cover / total


def compare(left, right):
    x, y = left["stats"], right["stats"]
    ngrams = dice(left, right, "mn", "mn")
    operands = dice(left, right, "op", "op")
    blocks = block_overlap(left, right, "blocks")
    loose = block_overlap(left, right, "loose")
    names = dice(left, right, "calls", "calls")
    signatures = dice(left, right, "call_signatures", "call_signatures")
    cfg = [ratio(a, b) for a, b in zip(x["shape"][:3], y["shape"][:3])]
    degrees_a, degrees_b = (Counter({(a, b): c for a, b, c in shape[3]}) for shape in (x["shape"], y["shape"]))
    if x["shape"][0] + y["shape"][0]:
        cfg.append(2 * sum((degrees_a & degrees_b).values()) / (x["shape"][0] + y["shape"][0]))
    cfg = [val for val in cfg if val is not None]
    calls = [val for val in (names, signatures) if val is not None]
    values = {"mnemonic_ngrams": ngrams, "operand_ngrams": operands, "block_content": blocks,
              "loose_blocks": loose, "cfg": sum(cfg) / len(cfg) if cfg else None,
              "call_names_or_signatures": max(calls) if calls else None,
              "constants": dice(left, right, "constants", "constants"), "data": dice(left, right, "data", "data")}
    support = sum(WEIGHTS[k] for k, val in values.items() if val is not None)
    size = ratio(left["occurrence"]["size"], right["size"]) or 0
    similarity = (sum(WEIGHTS[k] * val for k, val in values.items() if val is not None) / support if support else 0)
    score = similarity * math.sqrt(size)
    exact = left["occurrence"]["relocation_sha256"] == right["id"]
    if exact:
        score = 1.0
    values.update(size_ratio=size, named_call_overlap=names, callee_signature_overlap=signatures,
                  named_block_content=block_overlap(left, right, "named_blocks"))
    return score, {k: round(val, 4) if val is not None else None for k, val in values.items()}


def occurrence_dates(row, inventory):
    entry = inventory[row["artefact"]]
    for path in entry["paths"]:
        for date in path["dates"]:
            yield {**date, "path": path["path"]}
    for member in entry.get("archive_members", ()):
        if member.get("offset") == row["member_offset"] and member["timestamp"]:
            yield {"timestamp": member["timestamp"], "basis": "ar member timestamp",
                   "source": entry["paths"][0]["path"], "path": entry["paths"][0]["path"]}
    for module in entry.get("modules", ()):
        if module.get("offset") == row["member_offset"] and module.get("coff_timestamp"):
            yield {"timestamp": module["coff_timestamp"], "basis": "COFF header timestamp",
                   "source": entry["paths"][0]["path"], "path": entry["paths"][0]["path"]}
    if (linker_map := entry.get("linker_map")) and linker_map.get("created"):
        yield {"timestamp": linker_map["created"], "basis": "layout-checked WLINK map creation time",
               "source": linker_map["path"], "path": entry["paths"][0]["path"]}


def occurrence_label(row, inventory):
    entry = inventory[row["artefact"]]
    names = row["names"]
    dates = sorted(occurrence_dates(row, inventory), key=lambda d: (d["timestamp"], d["path"]))
    return {"path": dates[0]["path"] if dates else entry["paths"][0]["path"], "first_date": dates[0] if dates else None,
            "last_date": dates[-1] if dates else None, "artefact": row["artefact"],
            "name": names[0] if names else None, "member": row["member"], "member_offset": row["member_offset"],
            "address": f"{row['region']}:{row['offset']:05x}", "size": row["size"], "basis": row["basis"]}


def ranked(left, right, inventory, score, signals):
    literal = any(row["sha256"] == left["occurrence"]["sha256"] for row in right["occurrences"])
    dates = [date for row in right["occurrences"] for date in occurrence_dates(row, inventory)]
    first = min(dates, key=lambda x: x["timestamp"]) if dates else None
    last = max(dates, key=lambda x: x["timestamp"]) if dates else None
    shared = {}
    for key in ("calls", "constants", "data"):
        x, y = left["counters"][key], right["counters"][key]
        overlap = [(item, min(n, y[item])) for item, n in x.items() if item in y]
        shared[key] = sorted(overlap, key=lambda pair: (-pair[1], pair[0]))[:16]
    return {"id": right["id"], "score": round(score, 4), "signals": signals, "literal_bytes": literal,
            "relocation_aware_bytes": left["occurrence"]["relocation_sha256"] == right["id"],
            "public_names": sorted(right["names"]), "representative": occurrence_label(right["rep"], inventory),
            "direct_named_calls": right["stats"]["calls"], "data_references": right["stats"]["data"],
            "cfg_counts": right["stats"]["shape"][:3], "first_date": first, "last_date": last,
            "occurrences": len(right["occurrences"]), "shared_features": shared}


def rank_all(targets, groups, inventory, top, pool):
    by_mnemonic, by_name, by_hash, literal_ids = defaultdict(list), defaultdict(list), {}, defaultdict(set)
    by_block, by_string = defaultdict(list), defaultdict(list)
    by_length = defaultdict(list)
    for i, row in enumerate(groups):
        add_indices(row)
        by_length[row["bits"]].append((row["size"], i))
        for key in row["counters"]["mn"]:
            by_mnemonic[key].append(i)
        for key in {key for key, nbytes in row["stats"]["blocks"] if nbytes >= 12}:
            by_block[key].append(i)
        for key in row["counters"]["data"]:
            if key.startswith("S:"):
                by_string[key].append(i)
        for name in row["names"]:
            by_name[name].append(i)
        for code_hash in {occ["sha256"] for occ in row["occurrences"]}:
            literal_ids[row["bits"], code_hash].add(i)
        by_hash[row["id"]] = i
    for lengths in by_length.values():
        lengths.sort()
    results, lineage, controls = [], [], {}
    for index, target in enumerate(targets, 1):
        add_indices(target)
        occurrence = target["occurrence"]
        name = symbol(target["name"])
        same_hash = literal_ids[occurrence["bits"], occurrence["sha256"]]
        exact_id = by_hash.get(occurrence["relocation_sha256"])
        votes = defaultdict(float)
        keys = sorted((k for k in target["counters"]["mn"] if k in by_mnemonic),
                      key=lambda k: (len(by_mnemonic[k]), k))
        for key in keys[:64]:
            hits = by_mnemonic[key]
            weight = 1 / math.sqrt(len(hits))
            for i in hits:
                right = groups[i]
                if right["bits"] != occurrence["bits"]:
                    continue
                size = ratio(right["size"], occurrence["size"]) or 0
                if size >= .20:
                    votes[i] += weight * size
        seen = set(sorted(votes, key=lambda i: (-votes[i], i))[:pool])
        anchors = defaultdict(float)
        block_keys = {key for key, nbytes in target["stats"]["blocks"] if nbytes >= 12 and key in by_block}
        for key in sorted(block_keys, key=lambda k: (len(by_block[k]), k))[:32]:
            hits = by_block[key]
            for i in hits:
                right = groups[i]
                if right["bits"] == occurrence["bits"]:
                    size = ratio(right["size"], occurrence["size"]) or 0
                    if size >= .15:
                        anchors[i] += size / math.sqrt(len(hits))
        for key in target["counters"]["data"]:
            if key in by_string and len(by_string[key]) <= 80:
                for i in by_string[key]:
                    right = groups[i]
                    if right["bits"] == occurrence["bits"]:
                        size = ratio(right["size"], occurrence["size"]) or 0
                        if size >= .15:
                            anchors[i] += 2 * size / math.sqrt(len(by_string[key]))
        seen.update(sorted(anchors, key=lambda i: (-anchors[i], i))[:max(12, pool // 4)])
        seen.update(same_hash)
        seen.update(i for i in by_name[name] if groups[i]["bits"] == occurrence["bits"])
        if exact_id is not None:
            seen.add(exact_id)
        if not seen and occurrence["bits"] in by_length:
            lengths = by_length[occurrence["bits"]]
            pos = bisect_right(lengths, (occurrence["size"], len(groups)))
            seen.update(i for _, i in lengths[max(0, pos - pool // 2):pos + max(1, pool // 2)])
        candidates = []
        for i in seen:
            right = groups[i]
            score, signals = compare(target, right)
            candidates.append((score, name in right["names"], i, signals))
        candidates.sort(key=lambda row: (-row[0], row[2] != exact_id, row[2] not in same_hash,
                                         -row[1], groups[row[2]]["id"]))
        best = [ranked(target, groups[i], inventory, score, signals) for score, _, i, signals in candidates[:top]]
        named = [ranked(target, groups[i], inventory, score, signals) for score, yes, i, signals in candidates if yes]
        summary = {k: v for k, v in target.items() if k not in ("occurrence", "stats", "counters", "counts", "blocksets")}
        summary.update(size=occurrence["size"], extent_basis=occurrence["basis"])
        results.append({**summary, "target_sha256": occurrence["sha256"],
                        "target_relocation_sha256": occurrence["relocation_sha256"], "compared": len(seen),
                        "cfg_counts": target["stats"]["shape"][:3], "donors": best,
                        "best_named_donor": named[0] if named else None, "named_variants": named})
        if target["status"] == "STRICT":
            entries = []
            eligible = same_hash | ({exact_id} if exact_id is not None else set())
            for i in sorted(eligible):
                for donor in groups[i]["occurrences"]:
                    literal = donor["sha256"] == occurrence["sha256"]
                    site_equal = i == exact_id
                    entries.append({**occurrence_label(donor, inventory), "id": groups[i]["id"],
                                    "literal_bytes": literal, "relocation_aware_bytes": site_equal,
                                    "archive_dates": sorted(occurrence_dates(donor, inventory),
                                                            key=lambda d: (d["timestamp"], d["path"]))})
            lineage.append({**summary, "appearances": entries,
                            "literal_window": time_window(entries, "literal_bytes"),
                            "relocation_aware_window": time_window(entries, "relocation_aware_bytes")})
        if name in {"ksqrt", "getmousevalues"}:
            positive = next(((right, row) for right in groups for row in right["occurrences"]
                             if name in map(symbol, row["names"])
                             and any("/buil0801/engine.obj" in p["path"].lower()
                                     for p in inventory[row["artefact"]]["paths"])), None)
            if positive:
                right, row = positive
                score, signals = compare(target, right)
                control_key = "positive" if name == "getmousevalues" else "relocated_positive"
                controls[control_key] = {**occurrence_label(row, inventory), "target": target["name"],
                                         "target_status": target["status"], "score": round(score, 4), "signals": signals,
                                         "literal_bytes": row["sha256"] == occurrence["sha256"],
                                         "relocation_aware_bytes": right["id"] == occurrence["relocation_sha256"],
                                         "rank": next((j for j, entry in enumerate(candidates, 1)
                                                       if groups[entry[2]] is right), None)}
        if name in {"processinput", "getinput"}:
            hits = [(right, row) for right in groups if right["bits"] == 32 for row in right["occurrences"]
                    if {"memset", "printf", "malloc", "vprintf", "scanf", "fscanf"}.intersection(map(symbol, row["names"]))
                    and any("/wat95" in p["path"].lower() for p in inventory[row["artefact"]]["paths"])]
            negative = max(hits, key=lambda pair: ratio(occurrence["size"], pair[1]["size"]), default=None)
            if negative:
                right, row = negative
                score, signals = compare(target, right)
                key = "negative" if name == "getinput" else "large_target_negative"
                controls[key] = {**occurrence_label(row, inventory), "target": target["name"],
                                 "target_size": occurrence["size"], "score": round(score, 4), "signals": signals}
        if index % 100 == 0:
            print(f"Ranked {index}/{len(targets)} targets", flush=True)
    return results, lineage, controls


def time_window(entries, field):
    dates = [date for entry in entries if entry[field] for date in entry["archive_dates"]]
    if not dates:
        return {"first": None, "last": None}
    return {"first": min(dates, key=lambda x: x["timestamp"]), "last": max(dates, key=lambda x: x["timestamp"])}


def oracle_input_hashes():
    inputs = ("build/verify.json", "config/symbols.json", "config/library_symbols.json",
              "build/oracles/d3d/libsyms.json")
    return {path: digest((ROOT / path).read_bytes()) for path in inputs if (ROOT / path).is_file()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--add-dir", type=Path, action="append", default=[], help="Add a read-only input directory")
    parser.add_argument("--top", type=int, default=12, help="Ranked variants retained per oracle binding")
    parser.add_argument("--pool", type=int, default=180, help="Maximum n-gram retrieval candidates for full scoring")
    args = parser.parse_args()
    if args.top < 1 or args.pool < 1 or any(not p.is_dir() for p in args.add_dir):
        parser.error("--top and --pool must be positive; --add-dir must exist")
    started = time.monotonic()
    roots = tuple(dict.fromkeys(p.resolve() for p in (*DEFAULT_ROOTS, *args.add_dir)))
    if any(p == WORK or WORK.is_relative_to(p) or p.is_relative_to(WORK) for p in roots):
        parser.error("input directories must exclude the worker output directory")
    oracle = (ROOT / "assets/D3D.EXE").read_bytes()
    inventory, groups, hits, read_errors, rejected_maps = roster(roots, digest(oracle))
    runtime_fingerprints(inventory, groups)
    counts = {"paths": sum(len(row["paths"]) for row in inventory.values()), "unique_artefacts": len(inventory),
              "unique_functions": len(groups), "function_occurrences": sum(len(row["occurrences"]) for row in groups),
              "containers": dict(Counter(row["container"] for row in inventory.values())),
              "census_status": dict(Counter(row["census"] for row in inventory.values()))}
    write_json(OUT / "inventory.json", {"roots": list(map(display, roots)), "oracle_sha256": digest(oracle),
                                       "summary": counts, "read_errors": read_errors, "rejected_maps": rejected_maps,
                                       "artefacts": list(inventory.values())})
    write_json(OUT / "corpus.json", {"summary": counts, "functions": [
        {"id": row["id"], "size": row["size"], "bits": row["bits"], "names": sorted(row["names"]),
         "features": row["stats"], "feature_occurrence": row["rep"], "occurrences": row["occurrences"]}
        for row in groups]})
    for _ in range(3):
        input_hashes = oracle_input_hashes()
        targets = oracle_targets()
        if input_hashes == oracle_input_hashes():
            break
    else:
        raise RuntimeError("oracle bindings or verification snapshot changed during all three reads")
    results, lineage, controls = rank_all(targets, groups, inventory, args.top, args.pool)
    non_strict = sorted((row for row in results if row["status"] != "STRICT" and row["verify_size"]),
                        key=lambda row: (-row["verify_size"], row["address"],
                                         row["name"] != row["verified_name"], row["name"]))
    top30, top_bindings, names = [], [], set()
    for row in non_strict:
        if row["address"] in names:
            continue
        top30.append(row["address"])
        top_bindings.append(row["name"])
        names.add(row["address"])
        if len(top30) == 30:
            break
    write_json(OUT / "lineage.json", {"oracle_sha256": digest(oracle), "oracle_inputs": input_hashes,
                                     "strict_targets": len(lineage),
                                     "functions": lineage})
    write_json(OUT / "top_donors.json", {"oracle_sha256": digest(oracle), "oracle_inputs": input_hashes,
                                        "weights": WEIGHTS,
                                        "shortlist_pool": args.pool, "controls": controls,
                                        "largest_non_strict": top30, "largest_non_strict_names": top_bindings,
                                        "supplemental_names": sorted(SUPPLEMENT),
                                        "targets": results})
    print(json.dumps({**counts, "ranked_bindings": len(results), "strict_bindings": len(lineage),
                      "controls": controls, "cache_hits": hits, "elapsed_seconds": round(time.monotonic() - started)}))


if __name__ == "__main__":
    main()
