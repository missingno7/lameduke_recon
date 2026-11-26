"""LE census and byte agreement for the worker's WLINK experiment."""

from bisect import bisect_right
from collections import Counter, defaultdict
from pathlib import PureWindowsPath
import re

import funcmap
import match
import omf
from common import ROOT, sha256_bytes
from image import Image, fmt_addr, parse_addr
from le import LE

# Code-map starts, including each unit's private prefix and following alignment.
CODE_REGIONS = [
    ("BEGTEXT", 0x0, None),
    ("GAME", 0x10, "game"),
    ("SECTOR", 0x14fb0, "sector"),
    ("PLAYER", 0x1f280, "player"),
    ("ACTORS", 0x25160, "actors"),
    ("SOUNDS", 0x2ba30, "sounds"),
    ("MENUES", 0x2c1c0, "menues"),
    ("GAMEDEF", 0x2e700, "gamedef"),
    ("ENGINE", 0x33e5c, "engine"),
    ("MULTI", 0x47abf, "multi"),
    ("Creative/Gravis", 0x49c84, "nrpn"),
    ("Apogee C", 0x55b30, "user"),
    ("Watcom runtime", 0x62d8d, "outp"),
    ("A.ASM", 0x6b685, "a"),
]

# Layout anchors from GAME_MAP and RUNTIME_CENSUS. BSS begins at _edata.
DATA_REGIONS = [
    ("_NULL", 0x0, "_NULL"),
    ("CONST", 0x4, "CONST"),
    ("CONST2", 0x2164, "CONST2"),
    ("_DATA", 0x2304, "_DATA"),
    ("_GF1INIT", 0xabdc, "_GF1INIT"),
    ("XI", 0xac0c, "XI"),
    ("YI", 0xac36, "YI"),
    ("Alignment before _BSS", 0xac42, "YI end"),
    ("_BSS", 0xac50, "_BSS"),
    ("STACK (4 KiB hypothesis)", 0x1c89d0, "STACK"),
]


def measure(oracle, candidate):
    equal = 0
    first = None
    for offset, (exp, got) in enumerate(zip(oracle, candidate)):
        if exp == got:
            equal += 1
        elif first is None:
            first = offset
    common = min(len(oracle), len(candidate))
    size = max(len(oracle), len(candidate))
    if first is None and common < size:
        first = common
    return {
        "oracle": len(oracle), "candidate": len(candidate), "equal": equal,
        "different_in_overlap": common - equal, "first_difference": first,
        "agreement_pct": round(100 * equal / size, 4) if size else None,
    }


def by_region(oracle, candidate, regions):
    rows = []
    for (name, exp, got), (_, exp_end, got_end) in zip(regions, regions[1:]):
        row = {"region": name, "oracle_start": exp, "candidate_start": got}
        row["fixed"] = measure(oracle[exp:exp_end], candidate[exp:exp_end])
        if got is not None and got_end is not None:
            row["aligned"] = measure(oracle[exp:exp_end], candidate[got:got_end])
        rows.append(row)
    return rows


def map_symbols(path):
    segments, modules = {}, defaultdict(lambda: defaultdict(set))
    module = None
    for line in path.read_text(encoding="cp437").splitlines():
        if m := re.match(r"^Module: (\S+\.obj)\(", line, re.I):
            module = PureWindowsPath(m[1]).stem.lower()
        if m := re.fullmatch(r"(\S+)\s+(\S+)\s+(?:(DGROUP)\s+)?(\d{4}):([0-9a-f]{8})\s+([0-9a-f]{8})", line, re.I):
            seg, cls, group, obj, start, size = m.groups()
            segments[seg] = {"class": cls, "group": group, "object": int(obj, 16),
                             "start": int(start, 16), "size": int(size, 16)}
        if module and (m := re.fullmatch(r"([0-9a-f]{4}):([0-9a-f]{8})[+*s ]\s+(\S+)", line, re.I)):
            modules[module][m[3].lower()].add((int(m[1], 16), int(m[2], 16)))
    return segments, modules


def text_base(module, symbols):
    text = {i for i, seg in enumerate(module.segments)
            if seg and seg.cls == "CODE" and seg.name != "BEGTEXT" and seg.size}
    bases = {addr - off for name, seg, off, _ in module.publics if seg in text
             for obj, addr in symbols.get(name.lower(), ()) if obj == 1}
    return next(iter(bases)) if len(bases) == 1 else None


def classify_code(img, candidate, units, objs, modules):
    original = img.obj[1]
    linked = candidate.obj(1).data
    counts = defaultdict(Counter)
    aligned = defaultdict(Counter)
    _, bounds, _, _, _ = funcmap.analyse()
    bindings = match.Bindings(img)
    starts = sorted({len(original)}
                    | {f["start"] for f in bounds if {"entry", "call", "dataref"} & set(f["why"])}
                    | {off for obj, off in bindings.game.values() if obj == 1}
                    | {off for obj, off in bindings.lib.values() if obj == 1})
    labels = bytearray(len(original))
    categories = ("Other code or embedded data", "Known padding", "Known embedded data",
                  "NEAR_MATCH", "CODE_BYTES_MATCH", "STRICT")
    for label, spans in {
        1: [(0x14fad, 0x14fb0), (0x1f27e, 0x1f280), (0x2515c, 0x25160),
            (0x2ba27, 0x2ba30), (0x2c1bb, 0x2c1c0), (0x2e6fa, 0x2e700),
            (0x49c82, 0x49c84), (0x4e168, 0x4e170)],
        2: [(0x10, 0x20), (0x1f280, 0x1f2d4), (0x25160, 0x25177), (0x47abf, 0x47ad7)],
    }.items():
        for lo, hi in spans:
            labels[lo:hi] = bytes([label]) * (hi - lo)
    census = (ROOT / "docs/RUNTIME_CENSUS.md").read_text(encoding="utf-8")
    for start, end in re.findall(r"\| `1:([0-9a-f]+)-([0-9a-f]+)` \| \d+ \| Alignment", census):
        lo, hi = int(start, 16), int(end, 16)
        labels[lo:hi] = bytes([1]) * (hi - lo)

    spans = []
    for unit, stem in units.items():
        module, = omf.load(objs / (stem + ".obj"))
        base = text_base(module, modules[stem])
        for entry in match.verify_object(img, module):
            status = entry["status"]
            if status not in categories or not entry["size"] or not entry.get("oracle"):
                continue
            obj, lo = parse_addr(entry["oracle"])
            if obj != 1:
                continue
            if status == "NEAR_MATCH":
                hi = starts[bisect_right(starts, lo)]
            else:
                hi = lo + entry["size"]
            spans.append((categories.index(status), lo, hi))
            if base is None:
                aligned[status]["unmapped_functions"] += 1
                continue
            dest = base + entry["seg_offset"]
            m = measure(original[lo:hi], linked[dest:dest + entry["size"]])
            aligned[status].update({
                "functions": 1, "oracle": m["oracle"], "candidate": m["candidate"],
                "equal": m["equal"], "span": max(m["oracle"], m["candidate"]),
                "fully_equal": int(m["oracle"] == m["candidate"] == m["equal"]),
            })
    for label, lo, hi in sorted(spans):
        # Keep the independently identified padding/embedded data explicit.
        for off in range(lo, hi):
            if labels[off] not in (1, 2):
                labels[off] = label
    for off, (byte, label) in enumerate(zip(original, labels)):
        row = counts[categories[label]]
        row["oracle"] += 1
        if off < len(linked):
            row["candidate"] += 1
            row["equal"] += int(byte == linked[off])
            row["different_in_overlap"] += int(byte != linked[off])
    for row in counts.values():
        row["agreement_pct"] = round(100 * row["equal"] / row["oracle"], 4)
    for row in aligned.values():
        if row["span"]:
            row["agreement_pct"] = round(100 * row["equal"] / row["span"], 4)
    return counts, aligned


def fixup_report(oracle, candidate):
    def site(fx):
        return fx.obj, fx.obj_off, fx.src_type

    def target(fx):
        return fx.target_obj, fx.target_off

    expected = {site(fx): target(fx) for fx in oracle.fixups}
    actual = {site(fx): target(fx) for fx in candidate.fixups}
    shared = expected.keys() & actual.keys()
    output = {}
    for o in oracle.objects:
        n = candidate.obj(o.index)
        left_pages = oracle.page_map[o.page_index - 1:o.page_index - 1 + o.page_count]
        right_pages = candidate.page_map[n.page_index - 1:n.page_index - 1 + n.page_count]
        left = [fx for fx in oracle.fixups if fx.obj == o.index]
        right = [fx for fx in candidate.fixups if fx.obj == n.index]
        common_sites = {s for s in shared if s[0] == o.index}
        left_pages_fx = defaultdict(list)
        right_pages_fx = defaultdict(list)
        for page_lists, obj, fixups in ((left_pages_fx, o, left), (right_pages_fx, n, right)):
            for fx in fixups:
                page_lists[fx.page - obj.page_index].append(fx)
        common_pages = range(min(o.page_count, n.page_count))
        same_sites = sum([site(f) for f in left_pages_fx[p]] == [site(f) for f in right_pages_fx[p]]
                         for p in common_pages)
        same_records = sum([f.raw for f in left_pages_fx[p]] == [f.raw for f in right_pages_fx[p]]
                           for p in common_pages)
        same_shared_order = sum(
            [site(f) for f in left_pages_fx[p] if site(f) in common_sites]
            == [site(f) for f in right_pages_fx[p] if site(f) in common_sites]
            for p in common_pages
        )
        first_page = next((p for p in common_pages
                           if [f.raw for f in left_pages_fx[p]] != [f.raw for f in right_pages_fx[p]]), None)
        output[o.index] = {
            "oracle_records": len(left), "candidate_records": len(right),
            "oracle_unique_sites": len({site(f) for f in left}),
            "candidate_unique_sites": len({site(f) for f in right}),
            "shared_sites_at_fixed_offsets": len(common_sites),
            "shared_sites_with_identical_targets": sum(expected[s] == actual[s] for s in common_sites),
            "page_map_entries_equal": sum(a == b for a, b in zip(left_pages, right_pages)),
            "pages_compared": len(common_pages), "pages_with_identical_record_bytes": same_records,
            "pages_with_identical_site_sequence": same_sites,
            "pages_with_identical_common_site_sequence": same_shared_order,
            "first_different_object_page": first_page,
        }
        if first_page is not None:
            for label, pages in (("oracle", left_pages_fx), ("candidate", right_pages_fx)):
                output[o.index][label + "_first_page_records"] = [
                    {"site": fmt_addr((fx.obj, fx.obj_off)), "src_in_page": fx.src_off,
                     "target": fmt_addr(target(fx)), "flags": fx.flags} for fx in pages[first_page][:10]
                ]
    return output


def backed_size(le, obj):
    return min(obj.vsize, sum(le.page_file_range(page)[1]
                             for page in range(obj.page_index, obj.page_index + obj.page_count)))


def stock_code(oracle, candidate, modules, drive):
    """Compare each documented stock segment at its own map address."""
    rows = []
    audio = (ROOT / "docs/AUDIOLIB.md").read_text(encoding="utf-8")
    for name, start, end in re.findall(r"\| (\w+) \| ([0-9a-f]+)–([0-9a-f]+) \| Whole code\+sites", audio):
        stem = name.lower()
        if (drive / "v" / (stem + ".obj")).is_file():
            rows.append(("v", stem, int(start, 16), int(end, 16)))
    runtime = (ROOT / "docs/RUNTIME_CENSUS.md").read_text(encoding="utf-8")
    for start, end, name in re.findall(r"\| `1:([0-9a-f]+)-([0-9a-f]+)` \| \d+ \| `[\w/.]+\.LIB:([^ /\x60]+)",
                                       runtime):
        rows.append(("r", "atoi" if name == "atol" else name, int(start, 16), int(end, 16)))
    stats = []
    for folder, stem, lo, hi in rows:
        module, = omf.load(drive / folder / (stem + ".obj"))
        start = text_base(module, modules[stem])
        row = {"archive_group": folder, "member": stem, "oracle_start": lo, "candidate_start": start}
        if start is not None:
            size = sum(seg.size for seg in module.segments[1:] if seg.cls == "CODE" and seg.name != "BEGTEXT")
            row.update(measure(oracle.obj(1).data[lo:hi], candidate.obj(1).data[start:start + size]))
        stats.append(row)
    return stats


def a_relocation_control(oracle, candidate, source, start):
    module, = omf.load(source)
    seg, = (i for i, s in enumerate(module.segments) if s and s.cls == "CODE" and s.size)
    target = 0x6b685
    size = module.segments[seg].size
    original = oracle.obj(1).data[target:target + size]
    linked = candidate.obj(1).data[start:start + size]
    different = {i for i, (a, b) in enumerate(zip(original, linked)) if a != b}
    fields = {i for fx in module.fixups if fx.seg == seg for i in range(fx.off, fx.off + fx.size())}
    expected = {fx.obj_off - target for fx in oracle.fixups if fx.obj == 1 and target <= fx.obj_off < target + size}
    actual = {fx.obj_off - start for fx in candidate.fixups if fx.obj == 1 and start <= fx.obj_off < start + size}
    return {"byte_differences": len(different), "inside_omf_fixup_fields": len(different & fields),
            "oracle_le_absolute_sites": len(expected), "candidate_le_absolute_sites": len(actual),
            "shared_relative_le_absolute_sites": len(actual & expected)}


def compare(exe, map_path, units, objs, unresolved):
    img = Image()
    oracle = img.le
    candidate = LE(exe)
    symbols, modules = map_symbols(map_path)
    code, data = (candidate.obj(i) for i in (1, 2))
    code_regions = []
    for name, expected, stem in CODE_REGIONS:
        path = objs / (stem + ".obj") if stem else None
        if path and not path.is_file():
            folder = "v" if stem == "nrpn" else "r"
            path = exe.parent / folder / path.name
        at = text_base(omf.load(path)[0], modules[stem]) if path else 0
        code_regions.append((name, expected, at))
    code_regions.append(("end", oracle.obj(1).vsize, code.vsize))

    data_regions = []
    for name, expected, seg in DATA_REGIONS:
        row = symbols.get("YI" if seg == "YI end" else seg)
        start = row["start"] if row else None
        if row and seg == "YI end":
            start += row["size"]
        data_regions.append((name, expected, start))
    data_regions.append(("end", oracle.obj(2).vsize, data.vsize))

    fixed, aligned = classify_code(img, candidate, units, objs, modules)
    files = []
    candidate_regions = {name: (lo, hi) for lo, hi, name in candidate.regions()}
    for lo, hi, label in oracle.regions():
        a, b = candidate_regions.get(label, (0, 0))
        files.append({"region": label, "oracle_start": lo, "candidate_start": a,
                      **measure(oracle.file[lo:hi], candidate.file[a:b])})

    data_anchors = {}
    bindings = match.Bindings(img)
    for stem, names in modules.items():
        for name, places in names.items():
            if len(places) != 1:
                continue
            got, = places
            expected, _ = bindings.lookup(name)
            if expected and expected[0] == got[0] == 2:
                data_anchors[f"{stem}:{name}"] = {"oracle": expected[1], "candidate": got[1]}

    return {
        "unresolved": unresolved,
        "oracle_sha256": sha256_bytes(oracle.file), "candidate_sha256": sha256_bytes(candidate.file),
        "file": measure(oracle.file, candidate.file), "file_regions_by_start": files,
        "le_offset": [oracle.h, candidate.h],
        "mz_field_differences": {k: [v, candidate.mz[k]] for k, v in oracle.mz.items() if candidate.mz[k] != v},
        "le_field_differences": {k: [v, candidate.hdr[k]] for k, v in oracle.hdr.items() if candidate.hdr[k] != v},
        "resident_names": [oracle.resident_names(), candidate.resident_names()],
        "objects": [{
            "index": o.index, "base": [o.base, n.base], "flags": [o.flags, n.flags],
            "first_page": [o.page_index, n.page_index], "pages": [o.page_count, n.page_count],
            "virtual": measure(o.data, n.data),
            "file_backed": measure(o.data[:backed_size(oracle, o)], n.data[:backed_size(candidate, n)]),
            "backing_padding_after_vsize": [
                sum(le.page_file_range(p)[1] for p in range(obj.page_index, obj.page_index + obj.page_count))
                - backed_size(le, obj) for le, obj in ((oracle, o), (candidate, n))
            ],
        } for o, n in zip(oracle.objects, candidate.objects)],
        "code_regions": by_region(oracle.obj(1).data, code.data, code_regions),
        "code_at_oracle_offsets": fixed,
        "code_at_function_starts": aligned,
        "data_regions": by_region(oracle.obj(2).data, data.data, data_regions),
        "data_anchors": data_anchors, "stock_code_at_member_starts": stock_code(oracle, candidate, modules, exe.parent),
        "a_relocation_control": a_relocation_control(oracle, candidate, objs / "a.obj", code_regions[-2][2]),
        "candidate_segments": symbols, "fixups": fixup_report(oracle, candidate),
    }
