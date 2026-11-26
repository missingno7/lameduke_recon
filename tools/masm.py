"""Assemble with pinned MASM 5.10 under pinned MS-DOS Player (a DOS assembler; never run directly on the host).

    python tools/masm.py SRC.ASM OUT.OBJ

The source is copied as A.ASM into a fresh private directory (short 8.3 names; MS-DOS Player uses the host
filesystem), MASM is invoked exactly as Ken's makefile did (`masm a.asm;`), and the resulting object is copied out.
Calibration: this reproduces BUIL0422/A.OBJ byte-for-byte from BUIL0422/A.ASM (docs/TOOLCHAIN.md).
"""
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from common import BUILD, CONFIG, TOOLS_DIR, load_json, sha256_file  # noqa: E402


def assemble(src, out, inst="masm510"):
    ent = load_json(CONFIG / "toolchain.json")["installs"][inst]
    exe, runner = TOOLS_DIR / ent["masm"], TOOLS_DIR / ent["runner"]
    for rel, want in ent["key_files"].items():
        if sha256_file(TOOLS_DIR / rel) != want:
            raise SystemExit(f"{inst}: {rel} does not match its pinned sha256")
    root = BUILD / "tmp"
    root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="m", dir=root) as tmp:
        tmp = Path(tmp)
        shutil.copyfile(src, tmp / "A.ASM")
        env = {"SYSTEMROOT": "C:\Windows", "PATH": str(tmp), "TMP": str(tmp), "TEMP": str(tmp)}
        r = subprocess.run([str(runner), str(exe), "a.asm;"], cwd=tmp, env=env, capture_output=True, timeout=120)
        text = (r.stdout + r.stderr).decode("cp437", "replace")
        obj = tmp / "A.OBJ"
        if r.returncode or not obj.is_file() or "Severe" in text and " 0 Severe" not in text:
            return 1, text + "\nassembly failed"
        shutil.copyfile(obj, out)
    return 0, text


if __name__ == "__main__":
    rc, t = assemble(sys.argv[1], sys.argv[2])
    print(t)
    sys.exit(rc)
