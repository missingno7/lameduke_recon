"""Compile historical source for the external idioms index, serially through wcc."""
import argparse
from collections import Counter
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
from types import SimpleNamespace

sys.dont_write_bytecode = True
ROOT = next(p for p in Path(__file__).resolve().parents if (p / 'config/toolchain.json').is_file())
WORK = ROOT / 'build/idioms'
EXT = ROOT / 'external'
X = EXT / 'x'
DUKE = EXT / 'duke3d-1.3d'
SW = X / 'idtech-swsource2005'
ADDENDUM = X / 'ny-sw-src-addendum-20190129/sw-src-and-build-addendum-20190129'
ROTT = EXT / 'rott'
PROFILES = json.loads((ROOT / 'config/toolchain.json').read_text())['profiles']
sys.path.insert(0, str(ROOT / 'tools'))

CORE = {'duke-1.5', 'duke-1.3d-tree', 'kenbuild-20241213', 'kenbuild-20260331', 'buildsrc-20190520'}
ENGINE = {'engine', 'build', 'cache1d', 'pragmas', 'vesa'}
GAME = ('game', 'game_oe')
AUDIO = ROTT / 'audiolib/SOURCE'
PROJECT_DATES = {
    'duke-1.5': ('1996', 'Duke 1.5 version'),
    'duke-1.3d-tree': ('1996', 'v1.3D reconstruction target date'),
    'lesbird-whaven': ('1995', 'Capstone development snapshot; archive contains 1994-96 files'),
    'lesbird-tekwar': ('1995-10', 'Capstone development snapshot label'),
    'rott': ('1995', 'ROTT release-era source; GPL publication 2002'),
    'idtech-swsource2005': ('1997', 'Shadow Warrior release-era source; GPL publication 2005'),
}


def rel(path):
    return path.relative_to(ROOT).as_posix()


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def dump(path, data):
    temp = path.with_suffix('.tmp')
    temp.write_text(json.dumps(data, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    temp.replace(path)


def cs(path):
    return sorted(path.glob('*.c'), key=lambda p: p.name.lower())


def date_for(path, project):
    if project in PROJECT_DATES:
        return PROJECT_DATES[project]
    # Original archive timestamps are a date lead, with their precision labelled.
    modified = datetime.fromtimestamp(path.stat().st_mtime, timezone.utc).date()
    if 1993 <= modified.year <= 2000:
        return modified.isoformat(), 'archived member mtime'
    return {
        'kenbuild-20241213': ('1993-1996', 'archive snapshot range'),
        'kenbuild-20260331': ('1993-1997', 'archive snapshot range'),
        'buildsrc-20190520': ('1995-1997', 'archive snapshot range'),
        'ny-sw-src-addendum-20190129': ('1995-1997', 'archive snapshot range'),
    }[project]


def input_files(xscan, optional):
    replay = json.loads((xscan / 'corpus-nt.json').read_text(encoding='utf-8'))
    for row in replay.values():
        project, path, profile = row['label'], ROOT / row['source'], row['profile']
        if project not in CORE or profile not in ('engine', 'multi', *GAME):
            continue
        if project not in ('duke-1.5', 'duke-1.3d-tree') and path.stem.lower() not in {'game', 'build', 'engine', 'multi'}:
            continue
        yield project, path, profile, row['attempts']
    wh = X / 'lesbird-whaven/WHAVEN/SRC'
    tek = X / 'lesbird-tekwar/TEKWAR/SRC'
    sw_audio = SW / '_nested/AudioLib.zip/audiolib/SOURCE'
    groups = [
        ('lesbird-whaven', wh, []),
        ('lesbird-tekwar', tek, [wh]),
    ]
    if optional:
        groups += [
            ('rott', ROTT / 'rott', [AUDIO, ROTT / 'audiolib/PUBLIC/INCLUDE']),
            ('idtech-swsource2005', SW / 'source', [sw_audio, SW / 'extras/code.h']),
            ('ny-sw-src-addendum-20190129', ADDENDUM / 'source/CODE',
             [ADDENDUM / 'source/CODE/INCL', ADDENDUM / 'extras/CODE/MACT', SW / 'source', sw_audio]),
            ('ny-sw-src-addendum-20190129', ADDENDUM / 'build/CODE/blddbg', [SW / 'source']),
        ]
    for project, folder, includes in groups:
        if not folder.is_dir():
            raise ValueError(f'missing source directory: {folder}')
        for path in cs(folder):
            stem = path.stem.lower()
            profiles = ('engine',) if stem in ENGINE else ('multi',) if stem in {'multi', 'mmulti'} else GAME
            for profile in profiles:
                yield project, path, profile, [{'source_used': rel(path),
                                               'includes': [rel(p) for p in includes if p.is_dir()],
                                               'flags': PROFILES[profile]['flags']}]


def compile_one(wcc, src, obj, flags, install, includes, timeout, header_patches=()):
    original_run = subprocess.run

    def timed(*args, **kwargs):
        kwargs['timeout'] = timeout
        return original_run(*args, **kwargs)

    original = wcc.subprocess
    original_staged = wcc.staged

    class StagedHeaders(original_staged):
        def __enter__(self):
            source, out, scratch = super().__enter__()
            try:
                for name, data in header_patches:
                    header = source.parent / name
                    header.write_bytes(data)
                    os.utime(header, (wcc.STAGE_MTIME, wcc.STAGE_MTIME))
            except BaseException:
                super().__exit__(*sys.exc_info())
                raise
            return source, out, scratch

    wcc.subprocess = SimpleNamespace(run=timed)
    if header_patches:
        wcc.staged = StagedHeaders
    try:
        return wcc.compile_c(src, obj, flags, install, includes)
    except subprocess.TimeoutExpired:
        return 124, f'Watcom NT host exceeded {timeout:g} seconds and was terminated.'
    finally:
        wcc.subprocess = original
        wcc.staged = original_staged


def staged_headers(source, includes):
    # Enumerate compatibility headers that actually survive flattened staging.
    staged = {}
    for folder in [*reversed(includes), source.parent]:
        for p in folder.iterdir():
            if p.is_file() and p.suffix.lower() in {'.h', '.inc', '.def'}:
                staged[p.name.lower()] = p
    return staged


def compatibility(project, source, headers):
    staged = {p.name: p.read_bytes() for p in headers.values() if p.suffix.lower() == '.def'}
    edits = {}
    stubs = []
    if project in {'rott', 'idtech-swsource2005', 'ny-sw-src-addendum-20190129'} and 'mem.h' not in headers:
        stubs.append(WORK / 'stubs/mem.h')
    if project == 'lesbird-tekwar' and source.name.casefold() == 'tektag.c':
        header = headers['tekwar.h']
        data = header.read_bytes()
        before, after = b'extern    long      flags32[];', b'extern    unsigned long flags32[];'
        if data.count(before) != 1:
            raise ValueError(f'could not locate the original flags32 declaration in {header}')
        staged[header.name] = data.replace(before, after)
        edits[header.name] = 'declare flags32 unsigned to agree with its definition in TEKTAG.C'
    flags = []
    if project == 'lesbird-whaven' and source.name.casefold() in {'bstub.c', 'whbuild.c'}:
        stubs.append(WORK / 'stubs/whkeys.h')
        flags.append('-fi=whkeys.h')
    for stub in stubs:
        headers[stub.name] = stub
        staged[stub.name] = stub.read_bytes()
    return staged, edits, flags


def dependency_headers(obj, paths):
    # Watcom COMENT E9 ends with the exact staged dependency name.
    from neutrality import records
    used = {}
    for kind, body in records(obj.read_bytes()):
        if kind != 0x88 or body[1:2] != b'\xe9' or len(body) <= 7:
            continue
        name = Path(body[7:7 + body[6]].decode('latin-1').rstrip('\0')).name.lower()
        if name in paths:
            used[name] = rel(paths[name])
    return used


def unique_attempts(attempts):
    seen = set()
    for attempt in sorted(attempts, key=lambda a: a.get('returncode', 0) != 0):
        choice = (attempt['source_used'], tuple(attempt['includes']),
                  tuple(flag for flag in attempt['flags'] if flag not in ('-d1', '-bt=dos')))
        if choice not in seen:
            seen.add(choice)
            yield choice


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xscan', type=Path, default=ROOT / 'build/workers/xscan',
                        help='xscan NT manifest and its recorded compatibility inputs')
    parser.add_argument('--optional', action='store_true', help='Also compile ROTT and Shadow Warrior')
    parser.add_argument('--project', action='append')
    parser.add_argument('--retry', action='store_true')
    parser.add_argument('--timeout', type=float, default=8)
    args = parser.parse_args()
    os.environ.update(LAMEDUKE_HOST='nt', LAMEDUKE_STAGE=str(WORK / 'cc'))
    sys.path.insert(0, str(ROOT / 'tools/idioms'))
    import wcc

    objects, logs = WORK / 'objects', WORK / 'logs'
    objects.mkdir(exist_ok=True)
    logs.mkdir(exist_ok=True)
    manifest = WORK / 'compile.json'
    rows = json.loads(manifest.read_text(encoding='utf-8')) if manifest.exists() else {}
    for project, path, profile, attempts in input_files(args.xscan, args.optional):
        if args.project and project not in args.project:
            continue
        key = f'{rel(path)}:{profile}'
        token = hashlib.sha256(key.encode()).hexdigest()[:12]
        obj = objects / (token + '.obj')
        if key in rows and (rows[key]['returncode'] == 0 and obj.is_file() or not args.retry):
            continue
        date, date_basis = date_for(path, project)
        row = rows.setdefault(key, dict(project=project, file=rel(path), date=date, date_basis=date_basis,
                                        profile=profile, host='nt', install=PROFILES[profile]['install'], attempts=[]))
        for used, dirs, prior_flags in unique_attempts(attempts):
            src = ROOT / used
            includes = [ROOT / p for p in dirs if (ROOT / p).is_dir()]
            log = logs / f'{token}-{len(row["attempts"]) + 1}.txt'
            available = staged_headers(src, includes)
            staged, edits, extra = compatibility(project, path, available)
            flags = [*prior_flags, *extra, '-bt=dos', '-d1']
            rc, detail = compile_one(wcc, src, obj, flags, row['install'], includes, args.timeout, staged.items())
            headers = dependency_headers(obj, available) if not rc else {}
            log.write_text(detail, encoding='utf-8')
            row['attempts'].append(dict(source_used=used, source_sha256=digest(src), includes=[rel(i) for i in includes],
                                        flags=flags, diagnostics=rel(log), returncode=rc, dependency_headers=headers,
                                        header_edits=edits,
                                        stubs_offered=sorted(rel(p) for p in available.values()
                                                             if '/stubs/' in rel(p)),
                                        stubs=sorted(p for p in headers.values() if '/stubs/' in p)))
            if not rc:
                break
        row.update(returncode=rc, object=rel(obj) if not rc else None, object_sha256=digest(obj) if not rc else None)
        if rc:
            obj.unlink(missing_ok=True)
        dump(manifest, rows)
        print(f'{rc:3} {profile:8} {rel(path)}', flush=True)
    print(Counter((r['project'], r['returncode'] == 0) for r in rows.values()), flush=True)


if __name__ == '__main__':
    main()
