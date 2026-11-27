"""Produce a MASM 5.10 listing for the original assembly unit, with a pinned control.

MASM's empty second argument retains the default object name. The third
argument requests a listing during the ordinary assembly, without changing
the assembly source or feeding a listing back into the assembler.
"""
import shutil
import subprocess
import tempfile
from pathlib import Path

from neutrality import WORK, ROOT, CONFIG, compare, load_json, omf, sha256_file
from common import TOOLS_DIR
import masm


def main():
    config = load_json(CONFIG / 'toolchain.json')['installs']['masm510']
    for path, digest in config['key_files'].items():
        if sha256_file(TOOLS_DIR / path) != digest:
            raise SystemExit(f'{path}: pin mismatch')
    src = ROOT / load_json(CONFIG / 'units.json')['units']['a_asm']['source']
    directory = WORK / 'objects'
    directory.mkdir(exist_ok=True)
    masm.BUILD = WORK  # existing wrapper's TMP and staged input stay in this worker
    plain = directory / 'a_asm.plain.obj'
    rc, diagnostics = masm.assemble(src, plain)
    if rc:
        raise SystemExit(diagnostics)
    with tempfile.TemporaryDirectory(prefix='m', dir=WORK / 'tmp') as scratch:
        shutil.copyfile(src, f'{scratch}/A.ASM')
        env = {'SYSTEMROOT': r'C:\Windows', 'PATH': scratch, 'TMP': scratch, 'TEMP': scratch}
        exe = TOOLS_DIR / config['masm']
        runner = TOOLS_DIR / config['runner']
        run = subprocess.run([str(runner), str(exe), 'a.asm,,a.lst;'], cwd=scratch,
                             env=env, capture_output=True, timeout=45, check=True)
        print((run.stdout + run.stderr).decode('cp437', 'replace'))
        alt = Path(scratch) / 'A.OBJ'
        if alt.read_bytes() != plain.read_bytes():
            raise ValueError(compare(omf.load(plain)[0], omf.load(alt)[0]))
        shutil.copyfile(Path(scratch) / 'A.LST', directory / 'a_asm.lst')
    print('MASM listing object identical:', sha256_file(plain))


if __name__ == '__main__':
    main()
