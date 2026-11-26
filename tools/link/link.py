"""Natural-link experiment (link worker 2026-11-26): build every unit fresh, link with Watcom 9.5 WLINK under
DOSBox-X (system dos4g, explicit FILE order, stock vendor/runtime members), compare with D3D.EXE (compare.py).
Outputs in build/link/. Evidence only; never a match claim.

Build and inspect a source-and-stock-library Watcom 9.5 DOS/4GW link.

Exit 0 for a resolved link, 2 for an UNDEFSOK comparison image, 1 for failure.
"""

import argparse
import os
from pathlib import Path, PureWindowsPath
import re
import shutil
import subprocess
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
WORK = ROOT / "build" / "link"        # all link outputs (git-ignored)
WORK.mkdir(parents=True, exist_ok=True)
RUN = WORK / "run"
DRIVE = RUN / "d"
OBJ = DRIVE / "obj"
sys.path.insert(0, str(ROOT / "tools"))
import doshost
import masm
import omf
import wcc
from common import CONFIG, TOOLS_DIR, dump_json, load_json, sha256_file
from compare import compare

PINS = {
    "BIN/WLINK.EXE": "241fdcb027b49849c95c847bdbe10f2a971f7cd911435729043e29f25d64da32",
    "BIN/WLINK.LNK": "f312c29d7fadd5823c0c64746ebc71845fa83198b591f8f3aae763deec990145",
    "BINB/WLSYSTEM.LNK": "f242f01ebaed309ad79840955e1fd99c4cf75263f67a2898fcb7c9634606bdc4",
    "BINB/WSTUB.EXE": "95aec99b60d20426ad4e79d24df599fc5e40e16c40ea9d60e0566181d116adaf",
    "LIB386/DOS/EMU387.LIB": "04d77b19816163f03165e077455d15b3a4b7012f4e40dd66ab4772021d749ade",
}
VENDOR = ROOT / "external/duke3d-1.3d/reference/audiolib-reconstructions/origlibs/104/AUDIO_WF.LIB"
VENDOR_PIN = "553baedbf2c59dc99070e732ffbf2707cfe449670336d3435f4cd42b67470898"


def setup():
    os.environ["LAMEDUKE_HOST"] = "dos"
    doshost.WORKER = WORK / "dos"
    wcc.STAGE_ROOT = WORK / "cc"
    masm.BUILD = WORK
    OBJ.mkdir(parents=True, exist_ok=True)
    (DRIVE / "tmp").mkdir(exist_ok=True)


def object_name(name, unit):
    if name == "multi_data":
        return "mdata"
    return Path(unit["source"]).stem.lower()


def build_units(units):
    # Same unit flags/include order as verify.build_unit, with worker-local paths.
    for name, unit in units.items():
        source = ROOT / unit["source"]
        prof = wcc.profile(unit["profile"])
        out = OBJ / (object_name(name, unit) + ".obj")
        out.unlink(missing_ok=True)
        if unit.get("kind") == "asm":
            rc, log = masm.assemble(source, out, prof["install"])
        else:
            include = [ROOT / p for p in unit.get("include", [])] + [source.parent]
            flags = list(unit.get("flags_override", prof["flags"])) + unit.get("extra_flags", [])
            rc, log = wcc.compile_c(source, out, flags, prof["install"], include)
        (RUN / (name + ".log")).write_text(log, encoding="utf-8")
        if rc or not out.is_file():
            raise RuntimeError(f"{name}: build failed\n{log}")
        print(f"{name:20s} {out.name:12s} {out.stat().st_size:7d} bytes", flush=True)


def check(path, expected):
    got = sha256_file(path)
    if got != expected:
        raise ValueError(f"{path}: SHA-256 {got} differs from pin {expected}")


def extract_members(base, directive):
    archives = {}
    for line in directive.splitlines():
        file, marker, member = line.partition("# stock ")
        if not marker:
            continue
        archive, name = member.split()
        if archive not in archives:
            path = VENDOR if archive == "AUDIO" else base / "LIB386" / archive
            if archive == "AUDIO":
                check(path, VENDOR_PIN)
            data = path.read_bytes()
            members = {}
            for offset, mod in omf.parse_library(data):
                stem = PureWindowsPath(mod.name).stem.lower()
                if stem in members:
                    raise ValueError(f"{path}: duplicate member name {stem}")
                # Copy the original module records from THEADR through MODEND.
                _, end = omf.parse_module(data, offset)
                members[stem] = data[offset:end]
            archives[archive] = members
        dest = DRIVE / PureWindowsPath(file.split()[1])
        dest.parent.mkdir(exist_ok=True)
        dest.write_bytes(archives[archive][name])


def run_dos(base, directive, label):
    runner = TOOLS_DIR / doshost.RUNNER
    doshost.check_file(runner, doshost.RUNNER)
    command = rf"C:\BIN\WLINK.EXE @D:\{directive}"
    batch = [
        "@echo off", "set WATCOM=C:", r"set PATH=C:\BIN;C:\BINB",
        r"set LIB=C:\LIB386;C:\LIB386\DOS", r"set TMP=D:\TMP",
        r"set TEMP=D:\TMP",
        command + r" > D:\LINK.LOG", "if errorlevel 1 goto fail",
        r"echo ok > D:\DONE.TXT", "goto end", ":fail",
        r"echo failed > D:\DONE.TXT", ":end",
    ]
    for name in ("link.log", "done.txt"):
        (DRIVE / name).unlink(missing_ok=True)
    (DRIVE / "run.bat").write_bytes(("\r\n".join(batch) + "\r\n").encode("ascii"))
    conf = RUN / "dosbox.conf"
    conf.write_text(f"""[sdl]
showmenu=false
[dosbox]
working directory option=default
memsize=32
[cpu]
core=dynamic
cycles=max
[dos]
automount=false
automountall=false
lfn=true
[autoexec]
@echo off
mount c "{base}" -ro
mount d "{DRIVE}"
d:
call run.bat
exit
""", encoding="utf-8")
    env = dict(os.environ, TMP=str(DRIVE / "tmp"), TEMP=str(DRIVE / "tmp"), SDL_AUDIODRIVER="dummy")
    result = subprocess.run(
        [str(runner), "-conf", str(conf), "-silent", "-noconsole", "-exit"],
        cwd=RUN, env=env, capture_output=True, creationflags=subprocess.CREATE_NO_WINDOW, timeout=45,
    )
    (RUN / (label + ".dosbox.log")).write_bytes(result.stdout + result.stderr)
    status = DRIVE / "done.txt"
    log = DRIVE / "link.log"
    output = log.read_text(encoding="cp437") if log.is_file() else ""
    (RUN / (label + ".log")).write_text(output, encoding="utf-8")
    ok = result.returncode == 0 and status.is_file() and status.read_text().strip() == "ok"
    return ok, output


def build_inputs():
    paths = [*CONFIG.glob("*.json"), *(ROOT / "src").rglob("*")]
    return {p.relative_to(ROOT).as_posix(): sha256_file(p) for p in sorted(paths)
            if p.suffix.lower() in {".c", ".h", ".asm", ".inc", ".json"} and p.is_file()}


def attempt(base, directive, label):
    for file in ("game.exe", "game.map"):
        (DRIVE / file).unlink(missing_ok=True)
    ok, log = run_dos(base, directive, label)
    if (DRIVE / "game.map").is_file():
        shutil.copyfile(DRIVE / "game.map", RUN / (label + ".map"))
    unresolved = sorted(set(re.findall(r"Warning\(1028\): (\S+) is an undefined reference", log)))
    print(f"{label}: guest status {'ok' if ok else 'failed'}, {len(unresolved)} undefined symbols")
    return ok, unresolved


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--build-only", action="store_true", help="fresh registered objects only")
    mode.add_argument("--link-only", action="store_true", help="reuse the worker's most recent objects")
    args = parser.parse_args()
    setup()
    units = load_json(CONFIG / "units.json")["units"]
    stems = {name: object_name(name, unit) for name, unit in units.items()}
    if not args.link_only:
        before = build_inputs()
        build_units(units)
        changed = sorted(name for name, digest in build_inputs().items() if before.get(name) != digest)
        build = {name: sha256_file(OBJ / (stem + ".obj")) for name, stem in stems.items()}
        dump_json(RUN / "inputs.json", {"source_and_config_sha256_before_build": before,
                                       "changed_during_build": changed, "object_sha256": build})
        if changed:
            print(f"Inputs changed during compilation. See {RUN / 'inputs.json'}.")
    if args.build_only:
        return 0
    _, base = wcc.install("wc95b")
    for rel, pin in PINS.items():
        check(base / rel, pin)
    template = HERE / "game.lnk"
    directives = template.read_text(encoding="ascii")
    listed = re.findall(r"^file obj\\(\w+)\.obj", directives, re.M)
    if sorted(listed) != sorted(stems.values()):
        raise ValueError("game.lnk must list every registered object once")
    extract_members(base, directives)
    for name in ("game.lnk", "diag.lnk"):
        contents = (HERE / name).read_text(encoding="ascii")
        (DRIVE / name).write_bytes(contents.replace("\n", "\r\n").encode("ascii"))

    (RUN / "comparison.json").unlink(missing_ok=True)
    ok, unresolved = attempt(base, "game.lnk", "strict")
    if unresolved:
        ok, unresolved = attempt(base, "diag.lnk", "diagnostic")
    exe = DRIVE / "game.exe"
    if not ok or not exe.is_file():
        return 1
    results = compare(exe, DRIVE / "game.map", stems, OBJ, unresolved)
    dump_json(RUN / "comparison.json", results)
    for row in results["objects"]:
        print(f"obj{row['index']}: {row['virtual']['agreement_pct']:.2f}% virtual, "
              f"{row['file_backed']['agreement_pct']:.2f}% file-backed")
    print(RUN / "comparison.json")
    return 2 if unresolved else 0


if __name__ == "__main__":
    sys.exit(main())
