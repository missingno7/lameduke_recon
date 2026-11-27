"""Run a pinned Watcom compiler in an isolated environment.

    python tools/wcc.py [--install wc95b] [--profile engine] [FLAGS...] SRC.c -fo=OUT.obj [-i=DIR ...]

Key files of the install are hash-checked once per process. The native compiler runs with the install's
environment and private staging. DOS compilation uses the isolated DOSBox-X runner in doshost.py.
Adapted from rayman_recon/tools/wcc.py (GPLv2).
"""
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from common import BUILD, CONFIG, TOOLS_DIR, load_json, sha256_file  # noqa: E402

_checked = set()


def install(name):
    ent = load_json(CONFIG / "toolchain.json")["installs"][name]
    base = TOOLS_DIR / ent["dir"]
    if name not in _checked:
        for rel, want in ent.get("key_files", {}).items():
            got = sha256_file(base / rel)
            if got != want:
                raise SystemExit(f"{name}: {rel} sha256 {got} != pinned {want}")
        _checked.add(name)
    return ent, base


def profile(name):
    return load_json(CONFIG / "toolchain.json")["profiles"][name]


def compile_c(src, out, flags, inst="wc95b", include=(), tool="wcc386"):
    """Compile src to out. include: extra include dirs searched before the install's H/.

    wc95b and wc100 compiles run on the canonical DOS host (tools/doshost.py, DOSBox-X) unless LAMEDUKE_HOST=nt
    (fast native host, same code; CONST padding residue can differ). wc100 DOS hosting: assdos worker 2026-11-27."""
    use_dos = inst in ("wc95b", "wc100") and os.environ.get("LAMEDUKE_HOST", "dos") == "dos"
    if tool == "wcc386" and use_dos:
        import doshost
        return doshost.compile_c(src, out, flags, inst, include)
    ent, base = install(inst)
    out = Path(out).resolve()
    if out.exists():
        out.unlink()
    with staged(src, include) as (stage_src, stage_out, tmp):
        inc = [str(stage_src.parent), str(base / "H")]
        env = {"SYSTEMROOT": os.environ.get("SYSTEMROOT", "C:\\Windows"), "WATCOM": str(base),
               "INCLUDE": ";".join(inc), "PATH": str(base / "BINNT"), "TMP": str(tmp), "TEMP": str(tmp)}
        exe = base / ent[tool]["nt"]
        cmd = [str(exe)] + list(flags) + [str(stage_src), f"-fo={stage_out}"]
        r = subprocess.run(cmd, env=env, capture_output=True, text=True, timeout=300, cwd=str(tmp))
        if stage_out.is_file():
            shutil.copyfile(stage_out, out)
    text = r.stdout + r.stderr
    if r.returncode == 0 and not out.is_file():
        return 1, text + "\nno object produced"
    return r.returncode, text


# Watcom 9.5's CONST padding contains residue that depends on the compile path (PROVEN 2026-11-26: the same
# GAMEDEF source gives an oracle-identical, deterministic literal pool from a short directory and 4 differing
# padding bytes from a long one). Every C compile is therefore staged into one fixed short directory, with a fixed
# TMP and a fixed object name, serialised by a lock file so parallel workers stay deterministic.
STAGE_ROOT = Path(os.environ.get("LAMEDUKE_STAGE", str(BUILD / "cc")))
STAGE_MTIME = 788227200  # 1994-12-24 00:00:00 UTC (canonical; the original files' times are unknown)


class staged:
    def __init__(self, src, include=()):
        self.src = Path(src).resolve()
        self.include = [Path(p).resolve() for p in include]

    def __enter__(self):
        import msvcrt
        STAGE_ROOT.mkdir(parents=True, exist_ok=True)
        self.lockf = open(STAGE_ROOT / "lock", "a+")
        while True:
            try:
                msvcrt.locking(self.lockf.fileno(), msvcrt.LK_LOCK, 1)
                break
            except OSError:
                continue
        s, tmp = STAGE_ROOT / "s", STAGE_ROOT / "t"
        for d in (s, tmp):
            if d.exists():
                shutil.rmtree(d)
            d.mkdir()
        # include directories first, then the unit's own directory (local headers, .inc files) wins on a name clash;
        # INCLUDE then holds only the stage and the install's H/, so no host path reaches the compiler
        for d in list(reversed(self.include)) + [self.src.parent]:
            for f in d.iterdir():
                if f.is_file() and f.suffix.lower() in (".c", ".h", ".inc"):
                    shutil.copyfile(f, s / f.name)
                    # Watcom's dependency records carry each file's DOS timestamp and share the record buffer
                    # whose residue fills CONST padding: pin every staged file to one canonical time
                    os.utime(s / f.name, (STAGE_MTIME, STAGE_MTIME))
        return s / self.src.name, s / "out.obj", tmp

    def __exit__(self, *exc):
        import msvcrt
        self.lockf.seek(0)
        msvcrt.locking(self.lockf.fileno(), msvcrt.LK_UNLCK, 1)
        self.lockf.close()
        return False


def main(argv):
    argv = list(argv[1:])
    inst, flags, include = "wc95b", [], []
    if "--install" in argv:
        i = argv.index("--install"); inst = argv[i + 1]; del argv[i:i + 2]
    if "--profile" in argv:
        i = argv.index("--profile"); p = profile(argv[i + 1]); inst, flags = p["install"], list(p["flags"]); del argv[i:i + 2]
    for a in list(argv):
        if a.startswith("-i="):
            include.append(a[3:]); argv.remove(a)
    out = [a for a in argv if a.startswith("-fo=")]
    srcs = [a for a in argv if not a.startswith("-")]
    flags += [a for a in argv if a.startswith("-") and not a.startswith("-fo=")]
    if len(srcs) != 1 or len(out) != 1:
        print(__doc__)
        return 2
    rc, text = compile_c(srcs[0], out[0][4:], flags, inst, include)
    print(text, end="")
    return rc


if __name__ == "__main__":
    sys.exit(main(sys.argv))
