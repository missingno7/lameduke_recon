"""Generate Ghidra analysis drafts (semantic seeds, never matching evidence) for the pinned LameDuke LE executable.

    python tools/decomp/decomp.py            # ~45 s; outputs build/decomp/out/<TU>/<addr>_<name>.c + out/index.json

Applies funcmap starts, config/symbols.json, config/library_symbols.json and raynorpat names, LE relocations and the
Watcom register convention (watcall.xml). Written by the decomp worker 2026-11-26."""

import argparse
from collections import defaultdict
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import time


ROOT = Path(__file__).resolve().parents[2]
WORKER = ROOT / "build" / "decomp"     # all outputs and the Ghidra project (git-ignored)
WORKER.mkdir(parents=True, exist_ok=True)
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "tools"))

from common import dump_json, load_json, oracle_le


REGIONS = (
    (0x00000, "cstart"), (0x00010, "game"), (0x14fb0, "sector"),
    (0x1f280, "player"), (0x25160, "actors"), (0x2ba30, "sounds"),
    (0x2c1c0, "menues"), (0x2e700, "gamedef"), (0x33e5c, "engine"),
    (0x47abf, "multi"), (0x49c84, "creative"), (0x4e168, "gravis"),
    (0x55b30, "apogee"), (0x62d8d, "runtime"), (0x6b685, "a_asm"),
)
CODE_DATA = (
    # Private game tables from docs/GAME_MAP.md.
    (0x00010, 0x00016), (0x1f280, 0x1f2e0), (0x25160, 0x25180),
    # EMU387 table operands at 1:69b18, 69b76 and 6a45d.
    (0x69a53, 0x69ad3), (0x6a2d0, 0x6a32a),
)
# Ghidra's first sweep made one-byte function records for these routines
# without decoding their bodies. Both begin at the instruction after a
# return and have valid prologues in the oracle.
RECOVERED_STARTS = (0x4a91c, 0x63931)
# Only use C declarations with direct support in the checked-in sources.
SAMPLES = ("bootleg_cache_pop", "loadsounds", "xyzsound", "getincangle_shift", "getlabel")
STRUCTURES = {
    "sectortype": ("src/engine/BUILD.H", 37),
    "walltype": ("src/engine/BUILD.H", 35),
    "spritetype": ("src/engine/BUILD.H", 38),
    "extratype": ("src/engine/BUILD.H", 10),
    "sound_file": ("src/game/sounds.h", 8),
    "sound_voice": ("src/game/sounds.h", 19),
    "sound_player": ("src/game/sounds.h", 176),
    "actorstate": ("src/game/gamedef.h", 20),
}


def scratch_directory(path):
    """Create one owned directory, or verify an existing owned directory."""
    if path.is_symlink() or path.resolve() != path:
        raise ValueError(f"Worker output is redirected outside its path: {path}")
    path.mkdir(exist_ok=True)
    return path


def rebuild_maps(stage):
    fmap = stage / "funcmap.json"
    names = stage / "raynorpat_functions.json"
    subprocess.run([sys.executable, "-B", str(ROOT / "tools/funcmap.py"),
                    "--out", str(fmap)], cwd=ROOT, check=True)
    # refnames.py has no path options. Run the same extractor with its two
    # literal map paths redirected so the canonical build/map is read-only.
    script = ROOT / "tools/refnames.py"
    source = script.read_text(encoding="utf-8")
    for original, path in [("build/map/funcmap.json", fmap),
                           ("build/map/raynorpat_functions.json", names)]:
        literal = repr(original)
        if source.count(literal) != 1:
            raise ValueError(f"{script.name} no longer uses the expected {original} literal")
        source = source.replace(literal, repr(path.as_posix()))
    source = source.replace("'external/lameduke-raynorpat/source'",
                            repr((ROOT / "external/lameduke-raynorpat/source").as_posix()))
    exec(compile(source, str(script), "exec"), {"__name__": "__main__"})
    return load_json(fmap), load_json(names)


def uncomment(source):
    return re.sub(r"/\*.*?\*/|//[^\n]*", "", source, flags=re.S)


def structure_specs():
    specs = {}
    for name, (path, size) in STRUCTURES.items():
        source = uncomment((ROOT / path).read_text(encoding="latin-1"))
        body = re.search(rf"\bstruct {name}\s*\{{([^}}]+)}}", source)
        fields = []
        for declaration in body[1].split(";"):
            if not declaration.strip():
                continue
            base, declarators = re.fullmatch(
                r"\s*((?:(?:signed|unsigned)\s+)?(?:char|short|long)(?:\s+unsigned\s+int)?)\s*(.+?)\s*",
                declaration, re.S).groups()
            for field in declarators.split(","):
                pointer, field_name, count = re.fullmatch(r"\s*(\*?)(\w+)(?:\[(\d+)\])?\s*", field).groups()
                fields.append([field_name, base + (" *" if pointer else ""), int(count or 1)])
        specs[name] = {"size": size, "fields": fields}
    return specs


def make_metadata(stage, fmap, raynorpat):
    le = oracle_le()
    code, data = le.objects
    image = bytearray(data.base + data.vsize)
    for obj in le.objects:
        image[obj.base:obj.base + obj.vsize] = obj.data
    (stage / "d3d.raw").write_bytes(image)
    fixups = []
    for (obj, off), fx in sorted(le.fixup_map().items()):
        source = le.linear(obj, off)
        stored = struct.unpack_from("<I", image, source)[0]
        if fx.src_type != 7 or stored != fx.target_off:
            raise ValueError(f"Unexpected LE fixup at {obj}:{off:05x}")
        fixups.append([source, le.linear(fx.target_obj, fx.target_off), fx.src_type])

    def linear(coordinate):
        obj, off = coordinate.split(":")
        return le.linear(int(obj), int(off, 16))

    symbols = defaultdict(list)
    starts = {code.base + f["start"]: f for f in fmap["functions"]}
    names_by_address = defaultdict(list)
    for file, origin in [("symbols.json", "canonical"), ("library_symbols.json", "library")]:
        bindings = load_json(ROOT / "config" / file)["d3d"]
        for kind, entries in bindings.items():
            for name, binding in entries.items():
                # These canonical labels bound code extents in match.py.
                # They point to the byte immediately after the terminating ret.
                label_kind = "boundary" if name.endswith("_text_end") else kind
                names_by_address[linear(binding["addr"])].append((name, origin, label_kind))
                symbols[name, label_kind].append(linear(binding["addr"]))
    for coordinate, entries in raynorpat.items():
        for name, _source in entries:
            names_by_address[linear(coordinate)].append((name, "raynorpat", "functions"))

    units = load_json(ROOT / "config/units.json")["units"]
    strict = {}
    for unit in units.values():
        if unit["source"] in ("src/game/sounds.c", "src/game/gamedef.c"):
            for name, status in unit["functions"].items():
                if status["status"] == "STRICT":
                    strict[name] = unit["source"]
    samples = {str(symbols[name, "functions"][0]): name for name in SAMPLES if name in strict}
    if len(samples) != len(SAMPLES):
        raise ValueError("A quality sample is no longer registered STRICT")

    recovered = {code.base + offset for offset in RECOVERED_STARTS}
    starts_to_export = sorted(set(starts) | recovered | {
        address for address, entries in names_by_address.items()
        if code.base <= address < code.base + code.vsize
        and any(kind == "functions" and origin != "raynorpat" for _, origin, kind in entries)})
    functions = []
    for address, end in zip(starts_to_export, starts_to_export[1:] + [code.base + code.vsize]):
        offset = address - code.base
        tu = next(name for begin, name in reversed(REGIONS) if begin <= offset)
        names = [(name, origin) for name, origin, kind in names_by_address[address] if kind == "functions"]
        reasons = starts.get(address, {}).get("why", [])
        if address in recovered:
            reasons = [*reasons, "ghidra_oracle_decode"]
        # The original records may tag a jump table or other private constant
        # as a function. Well-supported starts get first choice of instruction
        # boundaries when a guessed start conflicts with them.
        priority = (0 if names and names[0][1] != "raynorpat" else
                    1 if names or address in recovered or "entry" in reasons or "call" in reasons else 2)
        code_data = any(lo <= offset < hi for lo, hi in CODE_DATA)
        functions.append({
            "linear": address, "limit": end, "addr": f"1:{offset:05x}",
            "name": names[0][0] if names else f"FUN_{address:08x}", "TU": tu,
            "size": starts.get(address, {}).get("size", end - address),
            "funcmap": address in starts, "evidence": reasons,
            "priority": priority, "code_data": code_data,
            "convention": "__watcall" if tu not in ("creative", "gravis", "a_asm", "cstart")
            else "__cdecl" if tu == "creative" else "unknown",
        })

    # The small scalar and array declarations come from BUILD.H, sounds.c/h
    # and gamedef.c/h. Plain Watcom char is unsigned in these profiles.
    data_specs = {
        "sector": ("sectortype", 1024), "wall": ("walltype", 8192),
        "sprite": ("spritetype", 4096), "extra": ("extratype", 4096),
        "sintable": ("short", 2048), "numsectors": ("short", 1), "numwalls": ("short", 1),
        "headspritestat": ("short", 1025),
        "headspritesect": ("short", 1025), "nextspritesect": ("short", 4096),
        "nextspritestat": ("short", 4096), "prevspritesect": ("short", 4096),
        "prevspritestat": ("short", 4096),
        "sound_voices": ("sound_voice", 39), "sound_file_table": ("sound_file", 39),
        "ps": ("sound_player", 17), "hittype": ("actorstate", 4096),
        "SoundToggle": ("char", 1), "MusicToggle": ("char", 1),
        "camerasprite": ("short", 1), "screenpeek": ("short", 1),
        "sound_names": ("char *", 39), "sound_device_names": ("char *", 9),
        "sound_device_map": ("int", 9), "cache_sizes": ("long", 128),
        "cache_depth": ("long", 1), "cache_remaining": ("long", 1),
        "cache_cursor": ("char *", 1), "bootleg_cache": ("char", 300000),
        "music_buffer": ("unsigned char", 40000),
        "keyw": ("char *", 60), "textptr": ("char *", 1), "label": ("char *", 1),
        "labelcnt": ("long", 1), "labelcode": ("long *", 1), "scriptptr": ("long *", 1),
        "actorscrptr": ("long *", 4096), "line_number": ("short", 1),
        "total_lines": ("short", 1), "last_used_text": ("char *", 1),
    }
    types = [[symbols[name, "data"][0], name, typ, count]
             for name, (typ, count) in data_specs.items() if (name, "data") in symbols]
    # Argument order follows the matched signatures. Avoid harvesting
    # provisional C declarations from functions still under reconstruction.
    signatures = {
        "bootleg_cache_pop": ("void", []),
        "loadsounds": ("void", []),
        "xyzsound": ("void", [("num", "short"), ("i", "short"), ("x", "long"),
                              ("y", "long"), ("z", "long")]),
        "getincangle_shift": ("short", [("a", "short"), ("na", "short"), ("shift", "short")]),
        "getlabel": ("void", []),
        "ispecial": ("char", [("c", "char")]),
        "isaltok": ("char", [("c", "char")]),
        "bootleg_cache_alloc": ("char *", [("size", "long")]),
        "getangle": ("int", [("x", "long"), ("y", "long")]),
        "FX_PlayVOC3D": ("int", [("data", "char *"), ("pitch", "int"), ("angle", "int"),
                                ("distance", "int"), ("priority", "int"),
                                ("callback", "unsigned long")]),
        "gameexit": ("void", [("message", "char *")]),
        "filelength": ("long", [("fd", "int")]),
        "close": ("int", [("fd", "int")]),
        "read": ("int", [("fd", "int"), ("buffer", "void *"), ("count", "unsigned int")]),
        "open": ("int", [("path", "char *"), ("flags", "int")]),
        "sopen": ("int", [("path", "char *"), ("flags", "int"), ("share", "int")]),
    }
    stack_varargs = {"open", "sopen"}
    typed_functions = [[symbols[name, "functions"][0], name, ret, args, name in stack_varargs]
                       for name, (ret, args) in signatures.items() if (name, "functions") in symbols]
    jt = set(code.base + off for off in fmap["jump_table_entries"])
    code_sites = {source for source, target, _ in fixups if code.base <= source < code.base + code.vsize
                  and code.base <= target < code.base + code.vsize}
    examined_jump_table = sorted(site for site in jt if site - 4 in code_sites or site + 4 in code_sites)
    dump_json(stage / "input.json", {
        "code": [code.base, code.vsize], "data": [data.base, data.vsize],
        "regions": [[code.base + offset, name] for offset, name in REGIONS],
        "code_data": [[code.base + lo, hi - lo] for lo, hi in CODE_DATA],
        "bss": data.base + sum(le.page_file_range(data.page_index + k)[1]
                              for k in range(data.page_count)),
        "entry": le.linear(*le.entry()), "fixups": fixups,
        "jump_table": examined_jump_table,
        "names": names_by_address, "functions": functions,
        "structures": structure_specs(), "types": types, "signatures": typed_functions,
        "samples": samples,
    })
    print(f"Image {len(image):,} bytes, {len(fixups):,} relocation sites, "
          f"{len(starts)} census starts and {len(functions) - len(starts)} additional seeds", flush=True)


def run_headless(stage, ghidra, options):
    runtime = scratch_directory(WORKER / "java")
    prefs = scratch_directory(runtime / "prefs")
    cache = scratch_directory(runtime / "cache")
    temp = scratch_directory(runtime / "tmp")
    project = scratch_directory(WORKER / "ghidra")
    env = dict(os.environ)
    # The Windows launcher calls LaunchSupport -save before JVM options
    # for the application take effect. Isolate that launcher's AppData too.
    env.update({
        "APPDATA": str(prefs), "LOCALAPPDATA": str(cache),
        "TMP": str(temp), "TEMP": str(temp),
        "GHIDRA_HEADLESS_JAVA_OPTIONS": " ".join(
            f'-D{k}={v}' for k, v in {
                "application.settingsdir": prefs, "application.cachedir": cache,
                "application.tempdir": temp, "java.io.tmpdir": temp,
                "java.util.prefs.userRoot": prefs, "cpu.core.limit": 4,
            }.items()),
    })
    cmd = [str(ghidra / "support/analyzeHeadless.bat"), str(project), "LameDuke",
           "-import", str(stage / "d3d.raw"), "-overwrite", "-loader", "BinaryLoader",
           "-processor", "x86:LE:32:default", "-cspec", "gcc",
           "-scriptPath", str(Path(__file__).resolve().parent),
           "-preScript", "SeedLameDuke.java", "prepare", str(stage),
           "-postScript", "SeedLameDuke.java", "export", str(stage),
           "-analysisTimeoutPerFile", str(options.analysis_timeout),
           "-max-cpu", "4", "-log", str(stage / "ghidra.log"),
           "-scriptlog", str(stage / "scripts.log")]
    # Ghidra may return zero after reporting a script failure. A completed
    # index and coverage validation are also required.
    subprocess.run(cmd, cwd=WORKER, env=env, check=True)


def verify_exports(stage):
    meta = load_json(stage / "input.json")
    report = load_json(stage / "run.json")
    index = load_json(stage / "out/index.json")
    expected = {item["addr"]: item for item in meta["functions"]}
    addresses = {item["addr"] for item in index}
    if len(index) != len(addresses) or not expected.keys() <= addresses:
        raise ValueError("Export index differs from the input start list")
    paths = set()
    successes = 0
    for item in index:
        seed = expected.get(item["addr"])
        if seed is not None:
            for key in ("name", "size", "TU", "funcmap"):
                if item[key] != seed[key]:
                    raise ValueError(f"Unexpected {key} in the index at {item['addr']}")
            if seed["priority"] == 0 and item["status"] != "decompiled":
                raise ValueError(f"Bound function {seed['name']} at {seed['addr']} failed decompilation")
        elif item["evidence"] != ["ghidra_autoanalysis"]:
            raise ValueError(f"Unexplained addition to the index at {item['addr']}")
        path = stage / "out" / item["file"]
        if path in paths or not path.is_file() or path.stat().st_size == 0:
            raise ValueError(f"Missing or colliding export: {path}")
        successes += item["status"] == "decompiled"
        paths.add(path)
    if sum(item["ghidra_function"] for item in index) != report["ghidra_functions"]:
        raise ValueError("Export index does not cover every Ghidra function")
    if report["relocations"] != len(meta["fixups"]):
        raise ValueError("Ghidra relocation census differs from the LE input")
    labels = {(address, name) for address, aliases in meta["names"].items() for name, _, _ in aliases}
    if report["retained_labels"] != len(labels):
        raise ValueError("Ghidra symbol census differs from the input")
    print(f"Verified {len(index)} files, {successes} with decompiled C", flush=True)
    return successes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ghidra", type=Path, default=Path("C:/tools/ghidra_12.1_PUBLIC"))
    parser.add_argument("--analysis-timeout", type=int, default=600, help="Ghidra analysis limit in seconds")
    options = parser.parse_args()
    started = time.monotonic()
    stage = scratch_directory(WORKER / "work")
    for name in ("out", "samples"):
        path = stage / name
        if path.is_symlink() or path.resolve() != path:
            raise ValueError(f"Worker output is redirected: {path}")
        if path.exists():
            shutil.rmtree(path)
    (stage / "run.json").unlink(missing_ok=True)

    make_metadata(stage, *rebuild_maps(stage))
    prepared = time.monotonic()
    run_headless(stage, options.ghidra.resolve(), options)
    analyzed = time.monotonic()
    verify_exports(stage)
    for name in ("out", "samples"):
        dest = WORKER / name
        if dest.is_symlink() or dest.resolve() != dest:
            raise ValueError(f"Worker output is redirected: {dest}")
        if dest.exists():
            shutil.rmtree(dest)
        (stage / name).replace(dest)
    timing = {"prepare_seconds": round(prepared - started, 2),
              "headless_seconds": round(analyzed - prepared, 2),
              "total_seconds": round(time.monotonic() - started, 2)}
    dump_json(stage / "timing.json", timing)
    print(json.dumps(timing), flush=True)


if __name__ == "__main__":
    main()
