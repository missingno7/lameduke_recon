"""Exhaustive STRICT block lookup and diagnostic inline-copy comparison."""
from bisect import bisect_right
from collections import Counter, defaultdict
from difflib import SequenceMatcher
from pathlib import Path
import re

from build_index import line_numbers
from neutrality import ROOT, WORK, dump_json, load_json, omf, sha256_file
from source_spans import Source


def key(ins):
    return tuple(i.shape.strip() for i in ins if i.shape)


def locate(address):
    return f'1:{address:05x}'


def strict_tables(index):
    groups = {}
    functions = {}
    for fn in index['functions']:
        if fn['language'] != 'c' or fn['kind'] != 'function':
            continue
        functions[fn['unit'], fn['function']] = fn
        for block in fn['blocks']:
            ins = [fn['instructions'][i] for i in block['instructions']]
            shape = tuple(i['shape'] for i in ins if i['shape'])
            if not shape:
                continue
            if shape not in groups:
                groups[shape] = {'id': len(groups), 'normalized': shape, 'donors': []}
            ids = sorted({s for i in ins for s in i['statements']}, key=lambda s: (
                fn['statements'][s]['line'], fn['statements'][s]['column']))
            spans = [{k: fn['statements'][s][k] for k in ('line', 'end_line', 'text', 'kind')} for s in ids]
            groups[shape]['donors'].append({'unit': fn['unit'], 'function': fn['function'], 'profile': fn['profile'],
                                          'source': fn['source'], 'address': locate(fn['extent_start'] + block['offset']),
                                          'lines': spans, 'instruction_lines': sorted({i['line'] for i in ins if i['line']}),
                                          'locations': sorted({i['location'] for i in ins}),
                                          'symbolic': [i['symbolic'] for i in ins if i['symbolic']]})
    return groups, functions


def block_rows(pair, groups, pair_blocks):
    matches = pair_blocks(pair.orig, pair.cand)
    paired = {b for b in matches if b is not None}
    oracle_keys = set(pair.orig.keys)
    oracle, candidate = [], []
    for i, (off, instructions) in enumerate(pair.orig.blocks):
        group = groups.get(key(instructions))
        if group is not None:
            oracle.append({'address': locate(pair.addr + off), 'size': pair.orig.sizes[i], 'group': group['id'],
                           'candidate_offset': pair.cand.blocks[matches[i]][0] if matches[i] is not None else None})
    for i, (off, instructions) in enumerate(pair.cand.blocks):
        group = groups.get(key(instructions))
        if group is not None and i not in paired:
            candidate.append({'offset': off, 'size': pair.cand.sizes[i], 'group': group['id'],
                              'reason': 'surplus_multiplicity' if pair.cand.keys[i] in oracle_keys else 'absent_in_oracle'})
    return oracle, candidate


def line_runs(ins, points, start):
    offsets, lines = zip(*points) if points else ((), ())
    runs = []
    for n, i in enumerate(ins):
        at = bisect_right(offsets, i.addr + start) - 1
        line = lines[at] if at >= 0 and offsets[at] >= start else None
        if runs and line == runs[-1][0]:
            runs[-1][2] = n + 1
        else:
            runs.append([line, n, n + 1])
    return runs


def common_anchors(body, other):
    """Long exact normalized runs, checked for a named access to give them meaning."""
    sm = SequenceMatcher(None, body, other, autojunk=False)
    blocks = [m for m in sm.get_matching_blocks() if m.size >= 3]
    refs = sorted({t for m in blocks for t in body[m.a:m.a + m.size]
                   if 'C:' in t or 'D:' in t or 'D@2:' in t or 'S:' in t})
    return sum(m.size for m in blocks), refs


def inline_rows(pair, unit, mod, points, source, functions, az):
    # Tables can be included before the public's oracle entry.
    public = next((p for p in mod.publics if p[0].removesuffix('_').strip('_') == pair.name.strip('_')), None)
    if public is None or public[1] not in points:
        return []
    symbol, seg, offset, _ = public
    addr, _ = az.binds.lookup(symbol)
    if addr is None:
        return []
    start = offset - (addr[1] - pair.addr)
    first = next((line for off, line in points[seg] if off >= offset), 0)
    statements = source.function(symbol.removesuffix('_'), first) or source.function(pair.name, first)
    named = {name: fn for (u, name), fn in functions.items() if u == unit and name != pair.name}
    sites = []
    for s in statements:
        names = set(re.findall(r'\b([A-Za-z_]\w*)\s*\(', s['text'])) & named.keys()
        for name in sorted(names):
            sites.append((s, name))
    if not sites:
        return []
    ci = [i for i in pair.cand.ins if i.shape]
    oi = [i for i in pair.orig.ins if i.shape]
    cs, oracle_shape = key(ci), key(oi)
    runs = line_runs(ci, points[seg], start)
    alignment = SequenceMatcher(None, cs, oracle_shape, autojunk=False).get_matching_blocks()
    result = []
    for site, callee in sites:
        fn = named[callee]
        fs = tuple(i['shape'] for i in fn['instructions'] if i['shape'])
        for line, lo, hi in runs:
            if line is None or not site['line'] <= line <= site['end_line'] or hi - lo < 5:
                continue
            if any(t in (f'call C:{callee}', f'jmp C:{callee}') for t in cs[lo:hi]):
                continue
            c_count, c_refs = common_anchors(cs[lo:hi], fs)
            if c_count < 5 or not c_refs:
                continue
            aligned = [(m.b + max(lo, m.a) - m.a, m.b + min(hi, m.a+m.size) - m.a)
                       for m in alignment if m.size >= 2 and max(lo, m.a) < min(hi, m.a+m.size)]
            if not aligned:
                continue
            a, b = aligned[0][0], aligned[-1][1]
            o_count, o_refs = common_anchors(oracle_shape[a:b], fs)
            if o_count < 5 or not set(c_refs) & set(o_refs) or b-a > max(3*(hi-lo), 30):
                continue
            if any(t in (f'call C:{callee}', f'jmp C:{callee}') for t in oracle_shape[a:b]):
                continue
            cand_exact = [i.exact.strip() for i in ci[lo:hi]]
            oracle_exact = [i.exact.strip() for i in oi[a:b]]
            if tuple(cand_exact) == tuple(oracle_exact):
                continue
            # A register/operand difference at an aligned instruction is internal
            # to the aligned window. Missing edge instructions alone are weaker.
            interior = sum(ci[m.a + j].exact != oi[m.b + j].exact for m in alignment for j in range(
                max(0, lo-m.a), min(m.size, hi-m.a)))
            if not interior and cs[lo:hi] == oracle_shape[a:b]:
                continue
            distance = sum(end-beg for beg, end in aligned)
            confidence = 'STRONG' if interior and distance >= 8 else 'HYPOTHESIS'
            result.append({'callee': callee, 'callee_entry': fn['oracle'], 'source': fn['source'],
                           'call_line': line, 'call_text': site['text'], 'confidence': confidence,
                           'candidate_offset': ci[lo].addr, 'oracle_start': locate(pair.addr + oi[a].addr),
                           'oracle_end': locate(pair.addr + oi[b-1].addr + oi[b-1].size),
                           'standalone_anchor_tokens': [c_count, o_count], 'shared_anchors': sorted(set(c_refs) & set(o_refs)),
                           'aligned_symbolic_changes': interior, 'candidate_symbolic': cand_exact,
                           'oracle_symbolic': oracle_exact})
    return result


def binary_for(unit):
    units = load_json(ROOT / 'config' / 'units.json')['units']
    source_digest = sha256_file(ROOT / units[unit]['source'])
    for root in (WORK, ROOT / 'build' / 'idioms'):
        manifest = root / 'neutrality.json'
        if not manifest.is_file():
            continue
        info = load_json(manifest)['units'].get(unit)
        if info and info.get('source_sha256', source_digest) != source_digest:
            raise ValueError(f'{unit}: source changed since the line-enabled DOS build; rebuild the worker index')
        if info and info['whole_object_payload_equal']:
            art = root / 'objects' / info['lines']['file']
            if art.is_file() and sha256_file(art) == info['lines']['sha256']:
                return art
    raise ValueError(f'{unit}: no verified line-enabled object in the worker or integrated index')


def reverse(index, census, az, only=None, obj=None, external=None, limit=3):
    from blockdiff import pair_blocks

    units = defaultdict(set)
    expected = {}
    for row in census:
        unit, name = row['unit'], row['function']
        if (only and only != (unit, name)) or not (unit.startswith('game_') or unit in ('engine', 'multi')):
            continue
        units[unit].add(name)
        expected[unit, name] = row
    if not units:
        raise ValueError('no selected non-STRICT game/engine function in build/blockdiff.json')
    groups, functions = strict_tables(index)
    profiles = load_json(ROOT / 'config' / 'units.json')['units']
    out, used, build, external_work = [], set(), {}, []
    for unit, names in units.items():
        art = obj or binary_for(unit)
        pairs = az.object_pairs(art, names, all_funcs=True)
        mod, = omf.load(art)
        points = line_numbers(art.read_bytes())
        entry = profiles[unit]
        source_path = ROOT / entry['source']
        source = Source(source_path.read_text(encoding='latin-1'))
        build[unit] = {'object':art.relative_to(ROOT).as_posix() if art.is_relative_to(ROOT) else art.as_posix(),
                       'object_sha256':sha256_file(art),'source_sha256':sha256_file(source_path)}
        if {p.name for p in pairs} != names:
            raise ValueError(f'{unit}: missing current bound census functions {names - {p.name for p in pairs}}')
        for pair in pairs:
            row = expected[unit, pair.name]
            if pair.status != row['status']:
                raise ValueError(f'{unit}/{pair.name}: census status {row["status"]}, object status {pair.status}')
            oracle, candidate = block_rows(pair, groups, pair_blocks)
            for b in [*oracle, *candidate]:
                used.add(b['group'])
            copies = inline_rows(pair, unit, mod, points, source, functions, az) if entry['profile'] == 'game_oe' and obj is None else []
            actionable = sum(b['size'] for b in oracle if b['candidate_offset'] is None)
            out.append({'unit': unit, 'function': pair.name, 'profile': entry['profile'], 'status': pair.status,
                        'extent_start': locate(pair.addr), 'oracle_size': pair.orig_size, 'candidate_size': pair.cand_size,
                        'census_candidate_size': row['candidate_size'],
                        'oracle_blocks_with_strict_donors': sorted(oracle, key=lambda b: (-b['size'], b['address'])),
                        'unmatched_candidate_blocks_with_strict_donors': sorted(candidate, key=lambda b: (-b['size'], b['offset'])),
                        'candidate_absent_in_oracle_bytes': sum(b['size'] for b in candidate if b['reason'] == 'absent_in_oracle'),
                        'all_shared_oracle_bytes': sum(b['size'] for b in oracle), 'unmatched_shared_oracle_bytes': actionable,
                        'differing_inline_windows': copies})
            if external is not None:
                external_work.append((out[-1], pair))
    if external is not None:
        from external_corpus import annotate_reverse
        external.prepare(tuple(t.strip() for t in key) for _, pair in external_work
                         for body in (pair.orig, pair.cand) for key in body.keys if key)
        for row, pair in external_work:
            annotate_reverse(row, pair, groups, external, limit)
    out.sort(key=lambda r: (-r['unmatched_shared_oracle_bytes'], -r['candidate_absent_in_oracle_bytes'],
                           -r['all_shared_oracle_bytes'], r['unit'], r['function']))
    return {'schema': 1, 'functions': out, 'build': build, 'strict_donor_groups': {str(g['id']): g for g in groups.values() if g['id'] in used},
            'candidate_source_anchors': 'registered DOS line objects' if obj is None else 'inline detection unavailable for --obj'}


def render(report, markdown=False):
    lines = []
    if markdown:
        lines += ['# Shared STRICT blocks', '',
                  'Ranked by unmatched oracle bytes that have an exact normalized STRICT C donor. Candidate-side bytes break ties.',
                  'Each row shows a representative donor and a count. The JSON holds every donor and its C lines.',
                  'Normalized equality preserves widths, references and immediates while abstracting general-purpose registers, stack homes and some branch details.',
                  'Inline windows require a source call site and named machine anchors in the standalone callee and both versions of the caller.',
                  'Inline windows are diagnostic and can miss small, unanchored or heavily rescheduled copies.', '']
    for fn in report['functions']:
        mark = '## ' if markdown else ''
        lines += [f"{mark}{fn['unit']}/{fn['function']} ({fn['status']})", '',
                  f"{fn['unmatched_shared_oracle_bytes']} unmatched oracle bytes with donors, {fn['all_shared_oracle_bytes']} total shared oracle bytes, "
                  f"{fn['candidate_absent_in_oracle_bytes']} candidate bytes whose normalized key is absent from the oracle."]
        for block_set in ('oracle_blocks_with_strict_donors', 'unmatched_candidate_blocks_with_strict_donors'):
            if fn[block_set]:
                lines += ['', ('Oracle blocks with STRICT donors' if block_set.startswith('oracle') else
                               'Unmatched candidate blocks with STRICT donors'), '']
            for block in fn[block_set]:
                group = report['strict_donor_groups'][str(block['group'])]
                donors = sorted(group['donors'], key=lambda d: (not bool(d['lines']), d['unit'] != fn['unit'],
                                                                d['profile'] != fn['profile'], d['address']))
                at = block.get('address') or f"candidate +{block['offset']:#x}"
                state = block.get('reason') or ('missing locally' if block['candidate_offset'] is None else 'present locally')
                d = donors[0]
                c = ' / '.join(f"{s['line']} {' '.join(s['text'].split())}" for s in d['lines']) or 'function-level location'
                lines.append(f"- {at}, {block['size']} B, {state.replace('_', ' ')}. "
                             f"{d['function']} {d['address']} ({len(donors)} donors, group {group['id']}) · {d['source']} · {c}")
        if fn['differing_inline_windows']:
            lines += ['', 'Differing inline windows', '']
        for copy in fn['differing_inline_windows']:
            lines.append(f"- Inline {copy['confidence']} · {copy['callee']} {copy['callee_entry']} · "
                         f"{copy['source']}:{copy['call_line']} · {copy['oracle_start']}–{copy['oracle_end']} · "
                         f"{copy['aligned_symbolic_changes']} aligned instruction differences.")
        lines.append('')
    return '\n'.join(lines)


def main():
    from idioms import bd, open_index

    report = reverse(open_index(), load_json(ROOT / 'build' / 'blockdiff.json')['functions'], bd.Analyzer())
    dump_json(WORK / 'shared_blocks.json', report)
    (WORK / 'shared_blocks.md').write_text(render(report, markdown=True), encoding='utf-8')
    print(len(report['functions']), 'functions;', len(report['strict_donor_groups']), 'groups')
    print(Counter(k for f in report['functions'] for k in ('oracle', 'candidate', 'inline') for _ in f[
        {'oracle': 'oracle_blocks_with_strict_donors', 'candidate': 'unmatched_candidate_blocks_with_strict_donors',
         'inline': 'differing_inline_windows'}[k]]))


if __name__ == '__main__':
    main()
