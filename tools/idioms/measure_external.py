"""Measure external-only normalized coverage on the blockdiff non-STRICT census."""
import argparse
from collections import Counter, defaultdict
from pathlib import Path

from build_external import ROOT, WORK, digest, dump
from external_corpus import ExternalBlocks, loose_key, open_external
from idioms import bd, open_index
from reverse_search import binary_for, strict_tables


def capture_worklist(census, ledger):
    units = defaultdict(dict)
    for row in census:
        unit = row['unit']
        if unit.startswith('game_') or unit in ('engine', 'multi'):
            units[unit][row['function']] = row

    az = bd.Analyzer()
    profiles = bd.load_json(ROOT / 'config/units.json')['units']
    objects = {}
    functions = []
    for unit, targets in units.items():
        obj = binary_for(unit)
        pairs = az.object_pairs(obj, set(targets), all_funcs=True)
        if {p.name for p in pairs} != set(targets):
            raise ValueError(f'{unit}: current line object does not cover the census')
        source = profiles[unit]['source']
        objects[unit] = {'path': obj.relative_to(ROOT).as_posix(), 'sha256': digest(obj),
                         'source': source, 'source_sha256': digest(ROOT / source)}
        for pair in pairs:
            target = targets[pair.name]
            if pair.status != target['status']:
                raise ValueError(f'{unit}/{pair.name}: status changed from {target["status"]} to {pair.status}')
            matches = bd.pair_blocks(pair.orig, pair.cand)
            blocks = [dict(address=f'1:{pair.addr + offset:05x}', size=pair.orig.sizes[i],
                           normalized=[x.shape.strip() for x in ins if x.shape],
                           symbolic=[x.exact.strip() for x in ins if x.exact])
                      for i, (offset, ins) in enumerate(pair.orig.blocks) if pair.orig.keys[i] and matches[i] is None]
            functions.append(dict(unit=unit, name=pair.name, status=pair.status, profile=profiles[unit]['profile'],
                                  candidate_size=pair.cand_size, oracle_size=pair.orig_size,
                                  address=f'1:{pair.addr:05x}', blocks=blocks))
    return dict(schema=1, census_sha256=digest(ledger), candidate_inputs=objects, functions=functions)


def measure(index, external, snapshot):
    groups, _ = strict_tables(index)
    strict_relaxed = Counter(loose_key(key) for key in groups if 'db' not in key)
    functions = snapshot['functions']
    stats = Counter()
    exact_keys, loose_keys = external.prepare(
        (tuple(row['normalized']) for fn in functions for row in fn['blocks']), count_all=True)
    for fn in functions:
        for row in fn['blocks']:
            key = tuple(row['normalized'])
            size = row['size']
            strict_exact = len(groups[key]['donors']) if key in groups else 0
            relaxed = loose_key(key)
            strict_loose = strict_relaxed[relaxed]
            n_exact = len(external.exact.get(key, ()))
            n_loose = len(external.loose.get(relaxed, ()))
            flags = {
                'unmatched': True,
                'eligible_decoded': 'db' not in key,
                'strict_exact': bool(strict_exact),
                'strict_loose': bool(strict_loose),
                'external_exact': bool(n_exact),
                'external_loose_inclusive': bool(n_loose),
                'external_exact_gain': bool(n_exact and not strict_exact),
                'external_loose_gain_at_same_tier': bool(n_loose and not strict_loose),
                'external_loose_increment_beyond_strict_exact': bool(n_loose and not strict_exact and not n_exact),
                'external_loose_increment_beyond_strict_loose': bool(n_loose and not strict_loose and not n_exact),
                'external_loose_increment_also_found_by_strict_loose': bool(
                    n_loose and not strict_exact and not n_exact and strict_loose),
            }
            for kind, yes in flags.items():
                if yes:
                    stats[kind + '_blocks'] += 1
                    stats[kind + '_bytes'] += size
            row.update(strict_exact_donors=strict_exact, strict_loose_keys=strict_loose,
                       external_exact_donor_variants=n_exact, external_loose_donor_variants=n_loose)
            if flags['external_exact_gain'] or flags['external_loose_increment_beyond_strict_exact']:
                row['external_hits'] = list(external.hits(key, fn['profile'], 5))
    stats['functions'] = len(functions)
    return dict(snapshot, summary=dict(stats), unique_external_exact_keys=exact_keys, unique_external_loose_keys=loose_keys)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--strict-index', type=Path, default=ROOT / 'build/idioms/idioms.json.gz')
    parser.add_argument('--external-index', type=Path, default=WORK / 'external.json.gz')
    parser.add_argument('--out', type=Path, default=WORK / 'coverage.json')
    parser.add_argument('--worklist', type=Path, help='Use a captured worklist instead of the current census')
    args = parser.parse_args()
    path = ROOT / 'build/blockdiff.json'
    provenance = {'strict_index_sha256': digest(args.strict_index),
                  'tokenizer_sha256': digest(ROOT / 'tools/blockdiff.py'),
                  'oracle_sha256': digest(ROOT / 'assets/D3D.EXE')}
    if args.worklist:
        snapshot = bd.load_json(args.worklist)
        if any(snapshot.get(k) != v for k, v in provenance.items()):
            parser.error('frozen worklist disagrees with the oracle, STRICT index or tokenizer')
        snapshot['worklist_sha256'] = digest(args.worklist)
    else:
        snapshot = capture_worklist(bd.load_json(path)['functions'], path)
        snapshot.update(provenance)
    external = ExternalBlocks(open_external(args.external_index), use_loose=True)
    report = measure(open_index(args.strict_index), external, snapshot)
    report['external_index_sha256'] = digest(args.external_index)
    dump(args.out, report)
    print(report['summary'])


if __name__ == '__main__':
    main()
