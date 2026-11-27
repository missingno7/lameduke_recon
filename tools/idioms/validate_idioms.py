"""Verify index coverage and reproduce the three game block-search examples."""
import json
import os
import re
import subprocess
import sys
from collections import Counter

from idioms import c_search, normalize_asm, open_index, query
from neutrality import ROOT, WORK, dump_json, load_json, sha256_file

CASES = (
    ('game_game', 'gamenumber', '1:029ff', 'gametext', '1:02810', 0.9333, 'overwritesprite'),
    ('game_actors', 'moveexplosions', '1:2993a', 'moveplayers', '1:2a66d', 1.0, 'sector[sprite[i].sectnum].floorz'),
    ('game_menues', 'draw_and_probe', '1:2c8b1', 'draw_pause_sign', '1:2e08b', 0.5, 'tilesizy[1139] / 2L'),
)


def validate_index(index):
    ledger = load_json(ROOT / 'config' / 'units.json')['units']
    expected = {(u, f) for u, v in ledger.items() for f, meta in v['functions'].items() if meta['status'] == 'STRICT'}
    got = {(f['unit'], f['function']) for f in index['functions']}
    assert got == expected and len(index['functions']) == len(expected)
    for fn in index['functions']:
        spans, ins = fn['statements'], fn['instructions']
        coverage = Counter(b for row in [*ins, *fn['data']]
                           for b in range(row['offset'], row['offset'] + row['size']))
        assert coverage == Counter(range(fn['size'])), fn['function']
        for i, row in enumerate(ins):
            assert row['line'] > 0 and row['kind'] == 'instruction', (fn['function'], row)
            assert all(i in spans[s]['instructions'] for s in row['statements'])
        for sid, span in enumerate(spans):
            assert all(sid in ins[i]['statements'] for i in span['instructions'])
    exact = 'mov R,m2[D:totalclock]; sub R,m2[D:lockclock]'
    intel = 'mov ax, word ptr [totalclock]; sub ax, word ptr [lockclock]'
    assert normalize_asm(exact) == normalize_asm(intel)
    rows = list(query(index, normalize_asm(intel)))
    assert [(r['function'], r['address'], r['match']) for r in rows] == [('getinput', '1:20e6e', 'statement')]
    rows = list(c_search(index, r'^reccnt--;'))
    assert [(r['function'], r['address']) for r in rows] == [('playback', '1:081f8'), ('closedemowrite', '1:0b937')]


def cli(script, *args):
    env = dict(os.environ, PYTHONIOENCODING='utf-8')
    return subprocess.run([sys.executable, '-B', str(script), *args], cwd=ROOT, text=True, encoding='utf-8',
                          env=env, capture_output=True, check=True, timeout=40).stdout


def main():
    index = open_index()
    validate_index(index)
    catalogue = load_json(ROOT / 'docs' / 'quirks.json')['rules']
    funcs = {(fn['unit'], fn['function']): fn for fn in index['functions']}
    for rule in catalogue:
        for ex in rule['examples']:
            fn = funcs[ex['unit'], ex['function']]
            address = int(ex['address'].split(':')[1], 16)
            assert fn['oracle'] == ex['entry'] and fn['extent_start'] <= address < fn['extent_start'] + fn['size']
    registry = {(r['unit'], r['function']): r for r in load_json(ROOT / 'build' / 'blockdiff.json')['functions']}
    neutrality = load_json(WORK / 'neutrality.json')['units']
    assert all(r['whole_object_payload_equal'] for r in neutrality.values())
    dest = WORK / 'validation'
    dest.mkdir(exist_ok=True)
    summary = []
    for unit, function, address, example, example_addr, score, source in CASES:
        assert registry[unit, function]['status'] != 'STRICT'
        idioms = ROOT / 'tools' / 'idioms' / 'idioms.py'
        full = json.loads(cli(idioms, 'near', unit, function, '--json'))
        subset = json.loads(cli(idioms, 'near', unit, function, '--block', address, '--json'))
        obj_info = neutrality[unit][neutrality[unit]['canonical']]
        obj = WORK / 'objects' / obj_info['file']
        assert sha256_file(obj) == obj_info['sha256']
        worklist = cli(ROOT / 'tools' / 'blockdiff.py', '--obj', str(obj), function, '--worklist')
        missing = worklist.split('missing (oracle blocks', 1)[1].split('extra (our blocks', 1)[0]
        offsets = re.findall(r'^  oracle \+[0-9a-fx]+ \((1:[0-9a-f]+)\)', missing, flags=re.M)
        assert offsets == [b['address'] for b in full['unmatched_blocks']]
        selected, = subset['unmatched_blocks']
        hit = selected['hits'][0]
        assert (hit['function'], hit['address'], hit['score']) == (example, example_addr, score)
        assert any(source in s['text'] for s in hit['sources'])
        assert selected == next(b for b in full['unmatched_blocks'] if b['address'] == address)
        dest.joinpath(f'{function}.worklist.txt').write_text(worklist, encoding='utf-8')
        dump_json(dest / f'{function}.near.json', full)
        summary.append({'unit': unit, 'function': function, 'blocks': full['worklist_blocks'],
                        'sample': address, 'best_match': example, 'match_address': example_addr, 'score': score})
        print(unit, function, full['worklist_blocks'], address, '->', example, example_addr, score)
    dump_json(dest / 'summary.json', summary)


if __name__ == '__main__':
    main()
