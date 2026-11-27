"""Test Watcom line-debug neutrality with fresh, serial, pinned DOS builds."""
import os
import sys
from collections import Counter
from pathlib import Path

WORK = Path(os.environ.get('IDIOMS_WORK', str(Path(__file__).resolve().parents[2] / 'build' / 'idioms'))).resolve()
ROOT = next(p for p in WORK.parents if (p / 'config' / 'units.json').is_file())
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / 'tools'))
import doshost
import omf
import wcc
from common import CONFIG, dump_json, load_json, sha256_file


def records(blob):
    pos = 0
    while pos < len(blob):
        size = int.from_bytes(blob[pos + 1:pos + 3], 'little')
        if size < 1 or pos + 3 + size > len(blob):
            raise ValueError(f'invalid OMF record at {pos:#x}')
        yield blob[pos], blob[pos + 3:pos + 2 + size]
        pos += 3 + size


def compare(a, b):
    """Compare full segments (including padding/coverage) and decoded OMF linkage."""
    fields = ('segments', 'fixups', 'publics', 'externs', 'groups', 'comdefs', 'dialect')
    delta = {field: getattr(a, field) == getattr(b, field) for field in fields}
    changes = []
    for i in range(1, max(len(a.segments), len(b.segments))):
        x = a.segments[i] if i < len(a.segments) else None
        y = b.segments[i] if i < len(b.segments) else None
        if x == y:
            continue
        changes.append({'segment': i, 'before': x.name if x else None, 'after': y.name if y else None,
                        'class': (x or y).cls, 'sizes': [s.size if s else None for s in (x, y)],
                        'coverage_equal': bool(x and y and x.covered == y.covered),
                        'byte_diffs': [[j, aa, bb] for j, (aa, bb) in enumerate(zip(x.data, y.data)) if aa != bb]
                        if x and y else None})
    return {'whole_object_payload_equal': all(delta.values()), 'equal': delta, 'changed_segments': changes}


def object_info(path):
    kinds = Counter(f'{kind:02x}' for kind, _ in records(path.read_bytes()))
    return {'file': path.name, 'sha256': sha256_file(path), 'record_counts': dict(kinds)}


def build(unit, label, entry, profile, flags):
    path = WORK / 'objects' / f'{unit}.{label}.obj'
    src = ROOT / entry['source']
    includes = [ROOT / p for p in entry.get('include', [])] + [src.parent]
    rc, diagnostics = wcc.compile_c(src, path, flags, profile['install'], includes)
    if rc:
        raise RuntimeError(f'{unit} {label}: {diagnostics}')
    return path


def main():
    os.environ['LAMEDUKE_HOST'] = 'dos'
    doshost.WORKER = WORK / 'tmp'
    (WORK / 'objects').mkdir(exist_ok=True)
    config = load_json(CONFIG / 'toolchain.json')
    units = load_json(CONFIG / 'units.json')['units']
    out = {'host': 'DOSBox-X', 'units': {}}
    for unit, entry in units.items():
        if entry['kind'] != 'c' or (len(sys.argv) > 1 and unit not in sys.argv[1:]):
            continue
        profile = config['profiles'][entry['profile']]
        effective = list(entry.get('flags_override', profile['flags'])) + entry.get('extra_flags', [])
        clean = [flag for flag in effective if flag != '-d1']
        debug = effective if '-d1' in effective else effective + ['-d1']
        a = build(unit, 'plain', entry, profile, clean)
        b = build(unit, 'lines', entry, profile, debug)
        row = compare(*[omf.load(path)[0] for path in (a, b)])
        row.update({'plain': object_info(a), 'lines': object_info(b), 'profile': entry['profile'],
                    'install': profile['install'], 'plain_flags': clean, 'line_flags': debug,
                    'canonical': 'lines' if '-d1' in effective else 'plain'})
        out['units'][unit] = row
        dump_json(WORK / 'neutrality.json', out)
        print(unit, 'equal' if row['whole_object_payload_equal'] else str([(s['before'], s['sizes'],
              len(s['byte_diffs'] or [])) for s in row['changed_segments']]) + str(row['equal']),
              'lines', {k: v for k, v in row['lines']['record_counts'].items() if k in ('94','95','c4','c5')}, flush=True)


if __name__ == '__main__':
    main()
