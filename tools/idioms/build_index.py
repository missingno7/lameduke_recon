"""Build a source/assembly index only from fresh STRICT verifier results."""
import gzip
import json
import re
import struct
from bisect import bisect_right
from collections import Counter, defaultdict

from neutrality import WORK, ROOT, CONFIG, compare, load_json, omf, records, sha256_file
import blockdiff as bd
import match
from source_spans import Source

INDEX = WORK / 'idioms.json.gz'


def line_numbers(blob):
    """Easy-OMF LINNUM: group index, segment index, then (uint16 line, uint32 offset)."""
    lines = defaultdict(list)
    for kind, body in records(blob):
        if kind not in (0x94, 0x95):
            continue
        reader = omf._R(body, True)
        reader.idx()  # group
        seg = reader.idx()
        lines[seg].extend((off, line) for line, off in struct.iter_unpack('<HI', body[reader.p:]))
    for points in lines.values():
        if points != sorted(set(points)) or len({off for off, _ in points}) != len(points):
            raise ValueError('unordered or ambiguous LINNUM anchors')
    return lines


def listing_rows(path, source):
    """Map emitting MASM listing rows to the original source, retaining their byte span."""
    rows = []
    cursor = 0
    norm = lambda t: re.sub(r'\s+', ' ', t.strip()).lower()
    clean = [norm(line) for line in source.lines]
    for row in path.read_text(encoding='cp437').splitlines():
        found = re.match(r'^ ([0-9A-F]{4})  [^\t]+\t+(.+)$', row)
        if not found:
            continue
        off, fragment = int(found[1], 16), norm(found[2])
        line = next((i for i in range(cursor, len(clean)) if clean[i].startswith(fragment)), None)
        if line is None:
            raise ValueError(f'MASM listing cannot locate {row!r} after line {cursor}')
        cursor = line + 1
        if rows and off <= rows[-1][0]:
            raise ValueError(f'MASM listing offset is not unique: {row!r}')
        data = re.match(r'^(?:\w+:\s*)?(?:db|dw|dd|dq|dt)\b', clean[line])
        rows.append((off, cursor, bool(data)))
    return rows


def decode_slice(code, relocs, branch, lo, hi):
    """Call the repo tokenizer for a code range with declared data outside the range."""
    shifted = {pos - lo: ('L', tok[1] - lo) if isinstance(tok, tuple) else tok
               for pos, tok in relocs.items() if lo <= pos < hi}
    ins, leaders = bd.tokenize(code[lo:hi], shifted, lambda t, call: branch(t + lo, call))
    for item in ins:
        item.addr += lo
        if item.target is not None:
            item.target += lo
    return ins, {x + lo for x in leaders}


def asm_body(code, relocs, branch, rows, start):
    ins, leaders = [], set()
    anchors = {}
    data = []
    for (lo, line, is_data), (hi, _, _) in zip(rows, [*rows[1:], (start + len(code), None, None)]):
        if is_data:
            data.append({'offset': lo - start, 'size': hi - lo, 'kind': 'assembly_data', 'line': line})
            continue
        xs, ls = decode_slice(code, relocs, branch, lo - start, hi - start)
        if len(xs) != 1 or xs[0].size != hi - lo or xs[0].shape == 'db':
            raise ValueError(f'MASM listing line {line} does not decode as one instruction')
        anchors[xs[0].addr] = line
        ins.extend(xs)
        leaders.update(ls)
    return bd.Body(ins, leaders), anchors, data


def c_body(code, relocs, branch, lead, start, points):
    ins, leaders = decode_slice(code, relocs, branch, lead, len(code))
    points = [(off - start, line) for off, line in points if start + lead <= off < start + len(code)]
    anchors = dict(points)
    starts = {i.addr for i in ins}
    if set(anchors) - starts:
        raise ValueError(f'line anchor mid-instruction at {set(anchors) - starts}')
    data = [{'offset': 0, 'size': lead, 'kind': 'leading_jump_table_and_padding'}] if lead else []
    # Some dispatch tables retained in a previous public's verified extent (e.g.
    # FX_GetVolume) mix external tail-call addresses with local labels. The
    # blockdiff tokenizer already recognizes the relocated entries as data.
    code_items = []
    for item in ins:
        if item.addr in relocs and item.size == 4 and item.shape is None:
            data.append({'offset': item.addr, 'size': 4, 'kind': 'relocated_code_segment_data'})
        else:
            code_items.append(item)
    return bd.Body(code_items, leaders), anchors, data


def build_function(az, source, row, points, assembly):
    oracle = int(row['oracle'].split(':')[1], 16)
    lead = row.get('leading_table', 0)
    start = row['seg_offset'] - lead
    addr = oracle - lead
    code = az.img.obj[1][addr:addr + row['size']]
    relocs, branch = az.oracle_side(addr, code)
    if assembly:
        rows = [(off, line, data) for off, line, data in points if start <= off < start + len(code)]
        body, anchors, data = asm_body(code, relocs, branch, rows, start)
        spans = [{'line': line, 'end_line': line, 'column': 1, 'kind': 'assembly',
                  'text': source.lines[line - 1].strip()} for _, line, _ in rows]
    elif spans := source.initializer(row['function']):
        # The ledger includes Apogee constants explicitly placed in CODE.
        body, anchors = bd.Body([], set()), {}
        data = [{'offset': 0, 'size': len(code), 'line': spans[0]['line'], 'kind': 'code_segment_initializer'}]
    else:
        body, anchors, data = c_body(code, relocs, branch, lead, start, points)
        first = next(iter(anchors.values()), 0)
        spelling = row['symbol'].removesuffix('_')
        spans = source.function(spelling, first) or source.function(row['function'], first)
    by_line = defaultdict(list)
    for sid, span in enumerate(spans):
        for line in range(span['line'], span['end_line'] + 1):
            by_line[line].append(sid)
        span['instructions'] = []
    seq = []
    starts = list(anchors)
    for item in body.ins:
        anchor = bisect_right(starts, item.addr) - 1
        line = anchors[starts[anchor]] if anchor >= 0 else None
        if line is not None and not 1 <= line <= len(source.lines):
            raise ValueError(f"{row['function']}: invalid source line {line}")
        if line and line not in by_line:
            by_line[line] = [len(spans)]
            spans.append({'line': line, 'end_line': line, 'column': 1, 'kind': 'unscoped_line',
                          'text': source.lines[line - 1].strip(), 'instructions': []})
        ids = by_line.get(line, [])
        location = 'masm_listing' if assembly else 'watcom_linnum'
        if line is None and spans and spans[0]['kind'] == 'signature':
            line = spans[0]['line']
            location = 'function_only'
        elif line is None:
            location = 'unavailable'
        for sid in ids:
            spans[sid]['instructions'].append(len(seq))
        raw = next(bd.MD.disasm(code[item.addr:item.addr + item.size], addr + item.addr, count=1), None)
        asm = ' '.join([raw.mnemonic, raw.op_str]).strip() if raw and raw.size == item.size else None
        kind = 'undecoded' if item.shape == 'db' or asm is None else 'instruction'
        # Use the fixture's real code when recording encodings; no raw bytes are persisted.
        seq.append({'offset': item.addr, 'size': item.size, 'line': line, 'anchor': item.addr in anchors,
                    'asm': asm, 'shape': item.shape.strip() if item.shape else None, 'location': location,
                    'symbolic': item.exact.strip() if item.exact else None, 'statements': ids, 'kind': kind})
    blocks = []
    index_at = {i['offset']: n for n, i in enumerate(seq)}
    for block_start, items in body.blocks:
        indexes = [index_at[i.addr] for i in items]
        if any(seq[i]['shape'] for i in indexes):
            blocks.append({'offset': block_start, 'instructions': indexes})
    coverage = Counter()
    for item in [*seq, *data]:
        coverage.update(range(item['offset'], item['offset'] + item['size']))
    if len(coverage) != len(code) or set(coverage.values()) != {1}:
        raise ValueError(f"{row['function']}: index does not partition the complete STRICT extent")
    return {'function': row['function'], 'oracle': row['oracle'], 'extent_start': addr, 'size': len(code),
            'statements': spans, 'instructions': seq, 'blocks': blocks, 'data': data,
            'kind': 'function' if seq else 'code_segment_data'}


def main():
    az = bd.Analyzer()
    units = load_json(CONFIG / 'units.json')['units']
    neutrality = load_json(WORK / 'neutrality.json')['units']
    totals = Counter()
    indexed = []
    for unit, ent in units.items():
        source_path = ROOT / ent['source']
        source = Source(source_path.read_text(encoding='latin-1'))
        assembly = ent['kind'] == 'asm'
        plain = WORK / 'objects' / f'{unit}.plain.obj'
        if assembly:
            artifact = plain
            lines = {1: listing_rows(WORK / 'objects' / f'{unit}.lst', source)}
        else:
            neutral = neutrality[unit]
            artifact = plain.with_name(f'{unit}.lines.obj')
            for path, label in ((plain, 'plain'), (artifact, 'lines')):
                if sha256_file(path) != neutral[label]['sha256']:
                    raise ValueError(f'{path} changed since the DOS neutrality check')
            if not neutral['whole_object_payload_equal'] or not compare(*[omf.load(p)[0] for p in (plain, artifact)])[
                    'whole_object_payload_equal']:
                raise ValueError(f'{unit}: line-enabled object payload changed')
            lines = line_numbers(artifact.read_bytes())
        mod, = omf.load(artifact)
        segments = {seg.name: i for i, seg in enumerate(mod.segments) if seg}
        verified = {r['function']: r for r in match.verify_object(az.img, mod)}
        expected = {fn for fn, meta in ent['functions'].items() if meta['status'] == 'STRICT'}
        actual = {fn for fn, row in verified.items() if row['status'] == 'STRICT'}
        if expected != actual:
            raise ValueError(f'{unit}: fresh STRICT ledger differs (lost={expected-actual}, new={actual-expected})')
        for fn in (name for name in verified if name in actual):
            row = verified[fn]
            item = build_function(az, source, row, lines[segments[row['segment']]], assembly)
            item.update(unit=unit, profile=ent['profile'], source=ent['source'], language='asm' if assembly else 'c')
            indexed.append(item)
            totals['functions'] += 1
            totals['assembly_functions' if assembly else 'c_functions' if item['instructions'] else 'c_data_publics'] += 1
            totals['strict_bytes'] += item['size']
            totals['blocks'] += len(item['blocks'])
            totals['source_spans'] += len(item['statements'])
            totals['emitting_spans'] += sum(bool(s['instructions']) for s in item['statements'])
            totals['instructions'] += len(item['instructions'])
            totals['instructions_with_line'] += sum(i['line'] is not None for i in item['instructions'])
            totals['function_only_instructions'] += sum(i['location'] == 'function_only' for i in item['instructions'])
            totals['ambiguous_instructions'] += sum(len(i['statements']) > 1 for i in item['instructions'])
            totals['unscoped_instructions'] += sum(any(item['statements'][s]['kind'] == 'unscoped_line'
                                                      for s in i['statements']) for i in item['instructions'])
            totals['undecoded'] += sum(i['kind'] == 'undecoded' for i in item['instructions'])
        print(unit, len(actual), flush=True)
    with gzip.open(INDEX, 'wt', encoding='utf-8', compresslevel=5) as out:
        json.dump({'schema': 1, 'summary': dict(totals), 'functions': indexed}, out, ensure_ascii=False,
                  separators=(',', ':'))
    print(dict(totals))


if __name__ == '__main__':
    main()
