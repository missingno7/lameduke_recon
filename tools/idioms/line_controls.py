"""Try alternate DOS line metadata for the anchorless Apogee procedures."""
import os
from collections import Counter

from neutrality import WORK, ROOT, CONFIG, doshost, dump_json, load_json, compare, omf, records, wcc
from build_index import line_numbers


def main():
    os.environ['LAMEDUKE_HOST'] = 'dos'
    doshost.WORKER = WORK / 'tmp'
    units = load_json(CONFIG / 'units.json')['units']
    report = {}
    for unit in ('ass_dpmi', 'ass_blastold', 'ass_multivoc'):
        entry = units[unit]
        profile = wcc.profile(entry['profile'])
        base = WORK / 'objects' / f'{unit}.lines.obj'
        src = ROOT / entry['source']
        for label, suffix in (('cv', ['-d1', '-hc']), ('wd', ['-d1', '-hw']), ('d2', ['-d2'])):
            effective = list(entry.get('flags_override', profile['flags'])) + entry.get('extra_flags', [])
            flags = [flag for f in effective for flag in (suffix if f == '-d1' else [f])]
            target = base.with_name(f'{unit}.{label}.obj')
            includes = [ROOT / x for x in entry.get('include', [])] + [src.parent]
            rc, diagnostics = wcc.compile_c(src, target, flags, profile['install'], includes)
            r = {'flags': flags, 'exit': rc}
            if rc:
                r['diagnostics'] = diagnostics
            else:
                a, b = omf.load(base)[0], omf.load(target)[0]
                diff = compare(a, b)
                r.update(record_counts=dict(Counter(f'{t:02x}' for t, _ in records(target.read_bytes()))),
                         line_numbers_equal=line_numbers(target.read_bytes()) == line_numbers(base.read_bytes()),
                         equal=diff['equal'], segments=[{'name': s['before'] or s['after'], 'sizes': s['sizes'],
                                                          'byte_differences': len(s['byte_diffs'] or [])}
                                                         for s in diff['changed_segments']])
            report[f'{unit}:{label}'] = r
            print(unit, label, rc, r.get('equal'), flush=True)
    dump_json(WORK / 'line_controls.json', report)


if __name__ == '__main__':
    main()
