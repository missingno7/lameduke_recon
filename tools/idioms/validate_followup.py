"""Check reverse coverage, data provenance, and the cross-version audit outputs."""
from collections import Counter
import json
import os
import subprocess
import sys

from idioms import bd, open_index
from import_audit import FILES, extract, verify_controls
from neutrality import ROOT, WORK, load_json, sha256_file


def cli(unit, function):
    result = subprocess.run([sys.executable, '-B', str(ROOT/'tools'/'idioms'/'idioms.py'), 'reverse', unit, function,
                             '--json'], cwd=ROOT, env=dict(os.environ,PYTHONIOENCODING='utf-8'), capture_output=True,
                            text=True, encoding='utf-8', timeout=40, check=True)
    return json.loads(result.stdout)


def validate_reverse(index):
    census = load_json(ROOT/'build'/'blockdiff.json')['functions']
    expected = {(c['unit'],c['function']):c['status'] for c in census if c['unit'].startswith('game_') or c['unit'] in ('engine','multi')}
    report = load_json(WORK/'shared_blocks.json')
    rows = report['functions']
    assert {(x['unit'],x['function']):x['status'] for x in rows} == expected and len(rows) == len(expected)
    assert [(-f['unmatched_shared_oracle_bytes'],-f['candidate_absent_in_oracle_bytes']) for f in rows] == sorted(
        (-f['unmatched_shared_oracle_bytes'],-f['candidate_absent_in_oracle_bytes']) for f in rows)
    az = bd.Analyzer()
    strict = {}
    for f in index['functions']:
        if f['language']!='c' or f['kind']!='function':
            continue
        for b in f['blocks']:
            addr = f"1:{f['extent_start']+b['offset']:05x}"
            k = tuple(f['instructions'][i]['shape'] for i in b['instructions'] if f['instructions'][i]['shape'])
            strict[f['unit'],f['function'],addr] = k
    donors = set(strict.values())
    for g in report['strict_donor_groups'].values():
        for d in g['donors']:
            assert strict[d['unit'],d['function'],d['address']] == tuple(g['normalized'])
    for unit, obj in report['build'].items():
        artifact = ROOT/obj['object']
        assert sha256_file(artifact) == obj['object_sha256']
        targets = {f['function']:f for f in rows if f['unit']==unit}
        for pair in az.object_pairs(artifact,set(targets),all_funcs=True):
            r = targets[pair.name]
            matches = bd.pair_blocks(pair.orig,pair.cand)
            used = {p for p in matches if p is not None}
            comparable = lambda key: tuple(t.strip() for t in key)
            orig = {f'1:{pair.addr+off:05x}' for (off,_),k in zip(pair.orig.blocks,pair.orig.keys) if comparable(k) in donors}
            cand = {off for i,((off,_),k) in enumerate(zip(pair.cand.blocks,pair.cand.keys))
                    if i not in used and comparable(k) in donors}
            assert orig == {b['address'] for b in r['oracle_blocks_with_strict_donors']}
            assert cand == {b['offset'] for b in r['unmatched_candidate_blocks_with_strict_donors']}
    gn, = cli('game_game','gamenumber')['functions']
    assert gn == next(r for r in rows if r['function']=='gamenumber')
    assert any(c['callee']=='gametext' and c['aligned_symbolic_changes']>0 and c['confidence']=='STRONG' for c in gn['differing_inline_windows'])
    assert any(b['reason']=='absent_in_oracle' and b['offset']==0xda for b in gn['unmatched_candidate_blocks_with_strict_donors'])
    mx = next(row for row in rows if row['function']=='moveexplosions')
    block = next(b for b in mx['oracle_blocks_with_strict_donors'] if b['address']=='1:2993a')
    assert block['candidate_offset'] is None
    assert any(d['function']=='moveplayers' for d in report['strict_donor_groups'][str(block['group'])]['donors'])
    return {'functions':len(rows), 'shared_oracle_blocks':sum(len(f['oracle_blocks_with_strict_donors']) for f in rows),
            'actionable_oracle_bytes':sum(f['unmatched_shared_oracle_bytes'] for f in rows),
            'absent_candidate_bytes':sum(f['candidate_absent_in_oracle_bytes'] for f in rows),
            'inline_windows':dict(Counter(i['confidence'] for f in rows for i in f['differing_inline_windows']))}


def validate_imports(index):
    doc = load_json(ROOT/'docs'/'quirks.json')
    rows = doc['imported_rules']
    tracked = {r['id']:r for r in rows}
    basis = {f['function']:f for f in index['functions'] if f['profile'] in ('engine','multi','game_oe')}
    n = 0
    for tag,(file,_) in FILES.items():
        for start,end,_,text in extract(tag,(ROOT/file).read_text(encoding='utf-8').splitlines()):
            r = tracked[f'import-{tag}-{start}']
            assert r['original_claim']==text and r['imported_from']['end_line']==end
            assert r['applies_to_9.5'] in ('CONFIRMED','REFUTED','UNTESTED')
            for example in r['examples']:
                fn = basis[example['function']]
                assert fn['profile']==example['profile'] and fn['source']==example['source']
            if r['applies_to_9.5']!='UNTESTED':
                assert r['examples'] or r['probes'] or r['control_artifact']
            n += 1
    assert n == len(tracked) == len(rows)
    verify_controls(load_json(WORK/'import_probes.json'),load_json(WORK/'probes.json'),
                    load_json(WORK/'version_controls.json'),list(basis.values()))
    assert (ROOT/'docs'/'WATCOM95_QUIRKS.md').read_text(encoding='utf-8').count('<!-- cross-version audit -->') == 1
    return dict(Counter(r['applies_to_9.5'] for r in rows))


if __name__=='__main__':
    index = open_index()
    print(validate_reverse(index))
    print(validate_imports(index))
