"""Build the xscan donor matrix with the pinned Watcom wrapper, one compile at a time."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

sys.dont_write_bytecode = True
ROOT = next(p for p in Path(__file__).resolve().parents if (p / "config/units.json").is_file())
WORK = ROOT / "build/workers/xscan"
EXT = ROOT / "external"
KEN = EXT / "x"
DUKE = EXT / "duke3d-1.3d"
V13 = DUKE / "src"
V15 = DUKE / "dukenukem3d"
STUB = WORK / "stubs"
PROFILES = json.loads((ROOT / "config/toolchain.json").read_text())["profiles"]
PROFILES["engine_oe"] = {
    "install": PROFILES["engine"]["install"],
    "flags": [*PROFILES["engine"]["flags"], "-oe=3000"],
}


def files_in(path):
    return sorted((p for p in path.rglob("*") if p.is_file() and p.suffix.lower() == ".c"),
                  key=lambda p: p.as_posix().lower())


def corpus():
    for name in ("kenbuild-20241213", "kenbuild-20260331", "buildsrc-20190520"):
        for path in files_in(KEN / name):
            stem = path.stem.lower()
            if name == "kenbuild-20241213" and (
                    stem not in {"game", "build", "engine"} or len(path.relative_to(KEN / name).parts) != 3):
                continue
            if stem in {"game", "bstub"}:
                profiles = ["game", "game_oe"]
            elif stem in {"build", "engine"}:
                profiles = ["engine", "engine_oe"]
            elif stem in {"multi", "mmulti"}:
                profiles = ["multi"]
            else:
                profiles = ["engine"]
            yield path, name, profiles
    for directory, label in ((V13 / "game", "duke-1.3d-tree"), (V15 / "source", "duke-1.5")):
        for path in files_in(directory):
            if path.parent == directory:
                yield path, label, ["game", "game_oe"]


def includes(src, label):
    if label == "duke-1.3d-tree":
        dirs = ["game/include", "engine/include", "include", "runtime/include", "mact/include",
                "mact/control/include", "mact/keyboard", "mact/scriplib", "mact/lbmlib/include",
                "mact/mipmap/include", "mact/penttime", "mact/tlibdos", "mact/time/include",
                "mact/control_complete/C-counterpart/include"]
        return [V13 / d for d in dirs] + [STUB / "v13"]
    if label == "duke-1.5":
        return [V15 / "audiolib/SOURCE", STUB / "v15"]
    return []


def compile_child():
    # Set scratch roots before importing the wrapper. The wrapper retains hash checking
    # and compiler environment isolation. Clamp its direct NT child timeout as well.
    from types import SimpleNamespace
    sys.path.insert(0, str(ROOT / "tools"))
    os.environ["LAMEDUKE_STAGE"] = str(WORK / "cc")
    import wcc
    import doshost
    doshost.WORKER = WORK / "dos"
    original_run = subprocess.run

    def timed_run(*args, **kwargs):
        kwargs["timeout"] = min(kwargs.get("timeout", 15), 15)
        return original_run(*args, **kwargs)

    wcc.subprocess = SimpleNamespace(run=timed_run)
    try:
        return wcc.main(["wcc.py", *sys.argv[2:]])
    except subprocess.TimeoutExpired:
        print("Watcom NT host timed out after 15 seconds and was terminated", file=sys.stderr)
        return 124


def build(src, profile, out, host, include_dirs, extra=()):
    env = dict(os.environ, LAMEDUKE_HOST=host, PYTHONDONTWRITEBYTECODE="1")
    flags = [*PROFILES[profile]["flags"], *extra]
    args = [sys.executable, "-B", str(Path(__file__).resolve()), "_wcc",
            "--install", PROFILES[profile]["install"], str(src), f"-fo={out}",
            *(f"-i={p}" for p in include_dirs if p.is_dir()), *flags]
    # Canonical DOS wrapper terminates its child at 45 s. Allow the wrapper to clean up.
    try:
        result = subprocess.run(args, cwd=ROOT, env=env, capture_output=True, timeout=65)
        return result.returncode, (result.stdout + result.stderr).decode(errors="replace")
    except subprocess.TimeoutExpired:
        return 124, "Pinned compiler wrapper timed out after 65 seconds\n"


def build_baseline():
    units = json.loads((ROOT / "config/units.json").read_text(encoding="utf-8"))["units"]
    directory = WORK / "baseline"
    directory.mkdir(exist_ok=True)
    rows = {}
    for unit, settings in units.items():
        src = ROOT / settings["source"]
        if unit != "engine" and not src.is_relative_to(ROOT / "src/game"):
            continue
        profile = settings["profile"]
        expected_flags = PROFILES[profile]["flags"]
        flags = settings.get("flags_override", expected_flags) + settings.get("extra_flags", [])
        if flags[:len(expected_flags)] != expected_flags:
            raise ValueError(f"{unit}: baseline flags override its canonical profile")
        inc = [ROOT / p for p in settings.get("include", [])] + [src.parent]
        out = directory / f"{unit}.obj"
        rc, detail = build(src, profile, out, "dos", inc, flags[len(expected_flags):])
        (directory / f"{unit}.txt").write_text(detail, encoding="utf-8")
        if rc or not out.is_file():
            raise RuntimeError(f"Baseline compile failed for {unit}: see {directory / (unit + '.txt')}")
        rows[unit] = {
            "source": settings["source"], "profile": profile, "flags": flags, "host": "dos",
            "object": out.relative_to(ROOT).as_posix(),
            "source_sha256": hashlib.sha256(src.read_bytes()).hexdigest(),
            "object_sha256": hashlib.sha256(out.read_bytes()).hexdigest(),
        }
        print(f"Baseline {unit}: {out.name}", flush=True)
    (directory / "manifest.json").write_text(json.dumps(rows, indent=2) + "\n", encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline", action="store_true", help="Rebuild the nine current source units on DOS")
    parser.add_argument("--host", choices=("dos", "nt"), default="dos")
    parser.add_argument("--label", action="append", help="Select one or more corpus labels")
    parser.add_argument("--retry", action="store_true", help="Retry failed builds with selected compatibility inputs")
    parser.add_argument("--probe", action="append", help="Build files matching any path substring")
    parser.add_argument("--profile", action="append", help="Select build profiles")
    parser.add_argument("--replay-nt", action="store_true", help="Reuse NT compile input selections on DOS")
    parser.add_argument("--refresh-target", action="store_true",
                        help="Rebuild legacy NT manifest rows with the explicit DOS target")
    parser.add_argument("--timeouts-only", action="store_true",
                        help="Replay NT timeouts instead of successful builds")
    args = parser.parse_args()
    if args.baseline:
        build_baseline()
        return
    host = args.host
    obj_dir = WORK / f"obj-{host}"
    log_dir = WORK / f"logs-{host}"
    obj_dir.mkdir(parents=True, exist_ok=True)
    log_dir.mkdir(parents=True, exist_ok=True)
    manifest = WORK / f"corpus-{host}.json"
    rows = json.loads(manifest.read_text(encoding="utf-8")) if manifest.is_file() else {}
    if args.refresh_target and host != "nt":
        parser.error("--refresh-target requires NT")
    if (args.replay_nt and host != "dos") or (args.timeouts_only and not args.replay_nt):
        parser.error("--replay-nt requires DOS, and --timeouts-only requires --replay-nt")
    nt = json.loads((WORK / "corpus-nt.json").read_text(encoding="utf-8")) if args.replay_nt else {}
    for src, label, profiles in corpus():
        path = src.relative_to(ROOT).as_posix()
        if args.label and label not in args.label:
            continue
        if args.probe and not any(p.lower() in path.lower() for p in args.probe):
            continue
        for profile in profiles:
            if args.profile and profile not in args.profile:
                continue
            key = f"{path}:{profile}"
            token = hashlib.sha256(key.encode()).hexdigest()[:12]
            out = obj_dir / f"{token}.obj"
            previous = rows.get(key)
            refresh = (args.refresh_target and previous and
                       "-bt=dos" not in previous["attempts"][-1]["flags"])
            if previous and not refresh and (previous["returncode"] == 0 and out.is_file() or not args.retry):
                continue
            log = log_dir / f"{token}-{len(rows.get(key, {}).get('attempts', [])) + 1}.txt"
            inc = includes(src, label)
            flags = []
            if refresh:
                prior = previous["attempts"][-1]
                src_used = ROOT / prior["source_used"]
                inc = [ROOT / p for p in prior["includes"]]
                flags = prior["flags"][len(PROFILES[profile]["flags"]):]
            elif args.replay_nt:
                wanted = 124 if args.timeouts_only else 0
                if key not in nt or nt[key]["returncode"] != wanted:
                    continue
                prior = nt[key]["attempts"][-1]
                src_used = ROOT / prior["source_used"]
                inc = [ROOT / p for p in prior["includes"]]
                flags = [f for f in prior["flags"][len(PROFILES[profile]["flags"]):] if f != "-bt=dos"]
            elif args.retry:
                from xscan_compat import compatibility
                src_used, inc, flags = compatibility(src, label, inc)
            else:
                src_used = src
            if host == "nt":
                flags.append("-bt=dos")
            rc, detail = build(src_used, profile, out, host, inc, flags)
            if rc == 0 and not out.is_file():
                rc, detail = 1, detail + "\nNo object produced.\n"
            if rc:
                out.unlink(missing_ok=True)
            log.write_text(detail, encoding="utf-8")
            row = rows.setdefault(key, {"source": path, "label": label, "profile": profile,
                                        "install": PROFILES[profile]["install"], "host": host, "attempts": []})
            row.update(returncode=rc, object=out.relative_to(ROOT).as_posix() if not rc else None)
            row["attempts"].append({
                "returncode": rc, "source_used": src_used.relative_to(ROOT).as_posix(),
                "source_sha256": hashlib.sha256(src_used.read_bytes()).hexdigest(),
                "object_sha256": hashlib.sha256(out.read_bytes()).hexdigest() if not rc else None,
                "includes": [p.relative_to(ROOT).as_posix() for p in inc if p.is_dir()],
                "flags": PROFILES[profile]["flags"] + flags,
                "diagnostics": log.relative_to(ROOT).as_posix(),
            })
            temp = manifest.with_suffix(".tmp")
            temp.write_text(json.dumps(rows, indent=2) + "\n", encoding="utf-8")
            temp.replace(manifest)
            print(f"{rc:3} {token} {profile:8} {path}", flush=True)


if __name__ == "__main__":
    if sys.argv[1:2] == ["_wcc"]:
        raise SystemExit(compile_child())
    main()
