"""Compile with the DOS-hosted Watcom 9.5b compiler (BIN/WCC386.EXE, bound Phar Lap extender) under DOSBox-X,
one fresh emulator session per object. This is the canonical host for wc95b: it reproduces Ken's BUIL0422
ENGINE.OBJ completely (code, data, all fixups, THEADR and dependency paths; only dependency timestamps differ) and
gives environment-independent CONST padding, unlike the NT-hosted build (docs/FINDINGS.md). Written by the doshost
worker 2026-11-26. Set LAMEDUKE_HOST=nt to use the fast NT-hosted compiler for exploration."""
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parent))
from common import BUILD, TOOLS_DIR, sha256_file  # noqa: E402
WORKER = BUILD / 'dos'          # private per-compile scratch (git-ignored)
import wcc  # noqa: E402

# This 9.5b installation puts the DOS compiler in BIN. Its Phar Lap extender
# is bound into the executable. DOS4GW is only used for the calibration control.
COMPILER = 'BIN/WCC386.EXE'
EXTENDER = 'BIN/DOS4GW.EXE'
RUNNER = 'dosbox-x/dosbox-x.exe'
PINS = {
    COMPILER: '3fe098187af3ed4bccbf184b0f0d1a21a18cf976fe72c6178f19b8eea8b13e78',
    EXTENDER: 'b401506365892bd7bcb4362599279504a863f05bf37d323f732fd348bd1ef3c5',
    RUNNER: 'b028a4d328302ea270722dec69f3ec3a10ebf76beba72a51970db33a88df2bf6',
}


def check_file(path, key):
    digest = sha256_file(path)
    if digest != PINS[key]:
        raise ValueError(f'{path}: SHA-256 {digest} does not match the pinned copy')


def stage_sources(source_dir, src, include, mtime):
    # Preserve the canonical wrapper's flat include search and collision order.
    dirs = list(dict.fromkeys([src.parent] + [Path(p).resolve() for p in include]))
    for directory in reversed(dirs):
        for path in directory.iterdir():
            if path.is_file() and path.suffix.lower() in ('.c', '.h', '.inc'):
                dest = source_dir / path.name.lower()
                shutil.copy2(path, dest)
                if mtime is not None:
                    os.utime(dest, (mtime, mtime))


def compile_staged(src, out, flags, inst, include, *, subdir='', mtime=wcc.STAGE_MTIME,
                   via_dos4gw=False):
    src, out = Path(src).resolve(), Path(out).resolve()
    if not src.is_file():
        raise FileNotFoundError(src)
    if inst != 'wc95b':
        raise ValueError(f'DOS hosting is calibrated for wc95b only, got {inst}')
    _, base = wcc.install(inst)
    runner = (TOOLS_DIR / RUNNER).resolve()
    check_file(runner, RUNNER)

    flags = list(flags)
    # These profiles use single-token switches. Keep DOS shell syntax out of flags.
    if any(not re.fullmatch(r'-[A-Za-z0-9_=+.,-]+', f) for f in flags):
        raise ValueError('expected simple Watcom compiler flags')
    if not re.fullmatch(r'[A-Za-z0-9_]+\.c', src.name.lower()):
        raise ValueError(f'unsupported DOS source filename: {src.name}')
    if subdir and not re.fullmatch(r'[A-Za-z0-9_]{1,8}', subdir):
        raise ValueError(f'expected a DOS stage directory name, got {subdir}')

    out.unlink(missing_ok=True)
    WORKER.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='dos-', dir=WORKER) as scratch:
        root = Path(scratch)
        drive_c, drive_d = root / 'c', root / 'd'
        watcom = drive_c / 'watcom'
        (watcom / 'BIN').mkdir(parents=True)
        shutil.copytree(base / 'H', watcom / 'H')  # copy2 preserves dependency mtimes
        check_file(base / COMPILER, COMPILER)
        shutil.copy2(base / COMPILER, watcom / COMPILER)
        if via_dos4gw:
            check_file(base / EXTENDER, EXTENDER)
            shutil.copy2(base / EXTENDER, watcom / EXTENDER)
        source_dir = drive_d / subdir
        source_dir.mkdir(parents=True)
        (drive_d / 'tmp').mkdir()
        stage_sources(source_dir, src, include, mtime)

        compiler = r'C:\WATCOM\BIN\WCC386.EXE'
        if via_dos4gw:
            compiler = r'C:\WATCOM\BIN\DOS4GW.EXE ' + compiler
        line = ' '.join([compiler, src.name.lower(), *flags, r'-fo=D:\OUT.OBJ'])
        if len(line) > 126:
            raise ValueError('DOS command line exceeds 126 characters')
        commands = [
            '@echo off', r'set INCLUDE=c:\watcom\h', r'set WATCOM=C:\WATCOM',
            r'set PATH=C:\WATCOM\BIN', r'set TMP=D:\TMP', r'set TEMP=D:\TMP',
            'set TZ=UTC0',
        ]
        if subdir:
            commands.append(f'cd {subdir}')
        commands += [line + r' > D:\BUILD.LOG', 'if errorlevel 1 goto fail',
                     r'echo ok > D:\DONE.TXT', 'goto end', ':fail',
                     r'echo failed > D:\DONE.TXT', ':end']
        (drive_d / 'run.bat').write_bytes(('\r\n'.join(commands) + '\r\n').encode('ascii'))
        conf = root / 'dosbox.conf'
        conf.write_text(f'''[sdl]
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
mount c "{drive_c}" -ro
mount d "{drive_d}"
d:
call run.bat
exit
''', encoding='utf-8')
        env = dict(os.environ, TEMP=scratch, TMP=scratch, SDL_AUDIODRIVER='dummy')
        args = [str(runner), '-conf', str(conf), '-silent', '-noconsole', '-exit']
        try:
            result = subprocess.run(args, cwd=root, env=env, capture_output=True,
                                    creationflags=subprocess.CREATE_NO_WINDOW, timeout=45)
        except subprocess.TimeoutExpired:
            return 1, 'DOSBox-X timed out after 45 seconds and was terminated\n'

        def read(path):
            return path.read_text(encoding='cp437', errors='replace') if path.is_file() else ''

        diagnostics = result.stdout.decode(errors='replace') + result.stderr.decode(errors='replace')
        diagnostics += read(drive_d / 'build.log')
        diagnostics += ''.join(read(path) for path in source_dir.glob('*.err'))
        sentinel, staged_out = drive_d / 'done.txt', drive_d / 'out.obj'
        if result.returncode or read(sentinel).strip() != 'ok' or not staged_out.is_file():
            diagnostics += f'\nDOSBox-X exit {result.returncode}, guest status {read(sentinel).strip()!r}\n'
            return 1, diagnostics
        shutil.copyfile(staged_out, out)
        return 0, diagnostics


def compile_c(src, out, flags, inst='wc95b', include=()):
    """Build a 9.5b C unit and return (returncode, diagnostic text), as in wcc.py."""
    return compile_staged(src, out, flags, inst, include)
