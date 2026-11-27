"""Check -d1 code and linkage neutrality in representative donor builds."""
import os
import sys

from build_external import ROOT, WORK, compile_one, compatibility, dump, staged_headers
sys.path.insert(0, str(ROOT / 'tools/idioms'))
from neutrality import compare, load_json, omf
import wcc


def code_view(mod):
    return dict(segments={i: s for i, s in enumerate(mod.segments) if s and s.cls == 'CODE'},
                fixups=[f for f in mod.fixups if mod.segments[f.seg].cls == 'CODE'],
                linkage=(mod.publics, mod.externs, mod.groups, mod.comdefs, mod.dialect))


def main():
    manifest = load_json(WORK / 'compile.json')
    variants = {
        ('duke-1.5', 'actors.c', 'game_oe'),
        ('kenbuild-20241213', 'engine.c', 'engine'),
        ('lesbird-whaven', 'whbuild.c', 'game'),
        ('lesbird-tekwar', 'tektag.c', 'game_oe'),
        ('rott', 'rt_view.c', 'game'),
        ('idtech-swsource2005', 'rooms.c', 'game'),
    }
    directory = WORK / 'd1-controls'
    directory.mkdir(exist_ok=True)
    wcc.STAGE_ROOT = WORK / 'cc'
    os.environ['LAMEDUKE_HOST'] = 'nt'
    results = []
    for key, row in manifest.items():
        a = row['attempts'][-1]
        src = ROOT / a['source_used']
        variant = (row['project'], src.name.casefold(), row['profile'])
        if row['returncode'] or variant not in variants:
            continue
        variants.remove(variant)
        includes = [ROOT / i for i in a['includes']]
        staged, _, _ = compatibility(row['project'], src, staged_headers(src, includes))
        flags = [flag for flag in a['flags'] if flag != '-d1']
        dest = directory / f'{row["project"]}.obj'
        rc, log = compile_one(wcc, src, dest, flags, row['install'], includes, 8, staged.items())
        (dest.with_suffix('.txt')).write_text(log, encoding='utf-8')
        if rc:
            raise ValueError(f'{key}: no-line control failed')
        plain, = omf.load(dest)
        lines, = omf.load(ROOT / row['object'])
        details = compare(plain, lines)
        code_equal = code_view(plain) == code_view(lines)
        results.append(dict(file=row['file'], profile=row['profile'], flags=flags, object=row['object'],
                            code_and_linkage_equal=code_equal,
                            whole_object_payload_equal=details['whole_object_payload_equal'],
                            changed_segments=[x['segment'] for x in details['changed_segments']]))
        print(results[-1], flush=True)
    dump(WORK / 'd1-controls.json', results)
    if variants or any(not r['code_and_linkage_equal'] for r in results):
        raise ValueError(f'control coverage or code equality failed: {variants}')


if __name__ == '__main__':
    main()
