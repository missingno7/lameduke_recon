"""Find C spellings in the verified Watcom index, or triage missing oracle blocks."""
import argparse
import difflib
import gzip
import json
import re
from collections import defaultdict
from pathlib import Path

from build_index import INDEX, WORK, bd, sha256_file, load_json

SIZES = {'byte': 1, 'word': 2, 'dword': 4, 'fword': 6, 'qword': 8, 'tbyte': 10, 'xmmword': 16}
R8 = {'al', 'ah', 'bl', 'bh', 'cl', 'ch', 'dl', 'dh'}
R16 = {'ax', 'bx', 'cx', 'dx', 'di', 'si', 'sp', 'bp', 'cs', 'ds', 'es', 'ss', 'fs', 'gs'}
REG = bd.GP | R16 | {'esp'}
REFERENCE = re.compile(r"[CDFKS]:[^ ,\[\]+]+|[FD]@\d:[0-9a-f]+")


def open_index(path=INDEX):
    with gzip.open(path, 'rt', encoding='utf-8') as src:
        return json.load(src)


def number(value):
    try:
        return int(value[:-1], 16) if re.fullmatch(r'[0-9a-f]+h', value, flags=re.I) else int(value, 0)
    except ValueError:
        return None


def split_quoted(text, separators):
    return [part.strip() for part in re.split(rf'''[{separators}](?=(?:[^'"]|"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*')*$)''',
                                                text) if part.strip()]


def intel_memory(operand, width):
    m = re.fullmatch(r'(?:(\w+)\s+(?:ptr\s+)?)?(?:([cdefgs]s):)?([^\[]*)\[([^\]]+)\]', operand, re.I)
    if not m:
        return None
    size, segment, prefix, expression = m.groups()
    width = SIZES.get((size or '').lower(), width)
    if width is None:
        raise ValueError(f'give an Intel size qualifier for {operand!r}, or paste a normalized operand')
    expr = re.sub(r'\s+', '', expression)
    base = index = ''
    displacement = 0
    ref = prefix.strip()
    for term in re.findall(r'[+-]?[^+-]+', expr):
        sign, term = (-1, term[1:]) if term.startswith('-') else (1, term.lstrip('+'))
        value = number(term)
        if value is not None:
            displacement += sign * value
        elif '*' in term:
            reg, scale = term.split('*')
            if reg.lower() not in REG:
                raise ValueError(f'invalid index operand {term!r}')
            index = f'+R*{int(scale)}'
        elif term.lower() in REG:
            if base:
                index = '+R*1'
            else:
                base = term.lower()
        elif not ref and sign == 1:
            ref = term
        else:
            raise ValueError(f'unsupported symbolic expression {operand!r}')
    seg = f'{segment.lower()}:' if segment else ''
    if ref:
        ref = bd.match.demangle(ref)
        addend = f'{displacement:+#x}' if displacement else ''
        if addend:
            raise ValueError(f'paste the normalized reference for a symbol with addend: {operand!r}')
        address = f'D:{ref}{"+R" if base else ""}{index}'
    elif base in ('esp', 'ebp') and not index:
        address = base + '+S'
    else:
        address = ('R' if base else '') + index + f'{displacement:+#x}'
    return f'{seg}m{width}[{address}]'


def normalize_asm(text):
    """Accept blockdiff tokens and common Intel syntax without running an assembler."""
    output = []
    for line in split_quoted(text, r';\n'):
        op, *rest = line.split(None, 1)
        operands = rest[0] if rest else ''
        if op.endswith(':'):
            if not operands:
                continue
            op, *rest = operands.split(None, 1)
            operands = rest[0] if rest else ''
        if op in bd.POLARITY.values():
            output.append(line)
            continue
        op = op.lower()
        operands = operands.strip()
        if not operands:
            if op not in ('nop', 'jmp'):
                output.append(op)
            continue
        args = split_quoted(operands, ',')
        if op in (*bd.POLARITY, 'jmp', 'call') and '[' not in operands and operands.lower() not in REG:
            target = re.sub(r'^(?:short|near|far)\s+(?:ptr\s+)?', '', operands, flags=re.I)
            if target.startswith(('C:', 'F@')):
                output.append(f'{op} {target}')
            elif op in bd.POLARITY:
                output.append(bd.POLARITY[op])
            elif op == 'call':
                off = number(target)
                addr = target if target.startswith('1:') else f'1:{off:05x}' if off is not None else None
                output.append(f'{op} F@{addr}' if addr else f'{op} C:{bd.match.demangle(target)}')
            continue
        if op in ('mov', 'xchg') and len(args) == 2 and args[0].lower() == args[1].lower() in REG:
            continue
        if op == 'lea' and len(args) == 2 and args[1].lower() in (f'[{args[0].lower()}]', f'[{args[0].lower()}+0]'):
            continue
        width = None
        regs = [arg.lower() for arg in args if arg.lower() in REG]
        if regs and op not in ('movsx', 'movzx'):
            width = 1 if regs[0] in R8 else 2 if regs[0] in R16 else 4
        if op in ('lea', 'call', 'jmp'):
            width = width or 4
        normalized = []
        for arg in args:
            low = arg.lower()
            imm = number(low)
            if arg == 'R' or arg.startswith('&') or re.match(r'^(?:\w+:)?m\d+\[', arg):
                normalized.append(arg)
            elif low in REG:
                normalized.append('R' if low in bd.GP else low)
            elif imm is not None:
                normalized.append(f'{imm & 0xffffffff:#x}')
            elif '[' in arg:
                normalized.append(intel_memory(arg, width) or arg)
            elif re.fullmatch(r'(?:offset\s+)?\w+', arg, re.I):
                normalized.append('&D:' + bd.match.demangle(arg.split()[-1]))
            else:
                normalized.append(arg)
        output.append(f'{op} {",".join(normalized)}')
    return tuple(output)


def code(fn, indexes, field):
    return tuple(fn['instructions'][i][field] for i in indexes if fn['instructions'][i][field])


def location(fn, first=None):
    address = fn['oracle'] if first is None else f"1:{fn['extent_start'] + fn['instructions'][first]['offset']:05x}"
    return {'unit': fn['unit'], 'function': fn['function'], 'profile': fn['profile'],
            'source': fn['source'], 'address': address}


def statement_hit(fn, span):
    indexes = span['instructions']
    row = location(fn, indexes[0] if indexes else None)
    row.update(line=span['line'], end_line=span['end_line'], column=span['column'], text=span['text'],
               kind=span['kind'], normalized=code(fn, indexes, 'shape'), asm=code(fn, indexes, 'asm'),
               symbolic=code(fn, indexes, 'symbolic'),
               ambiguous=any(len(fn['instructions'][i]['statements']) > 1 for i in indexes))
    return row


def query(index, pattern):
    for fn in index['functions']:
        if fn['language'] != 'c':
            continue
        for span in fn['statements']:
            idx = [i for i in span['instructions'] if fn['instructions'][i]['shape']]
            key = code(fn, idx, 'shape')
            for n in range(len(key) - len(pattern) + 1):
                if key[n:n + len(pattern)] != pattern:
                    continue
                matches = idx[n:n + len(pattern)]
                first, last = matches[0], matches[-1]
                # A source span can recur in disjoint areas after scheduling.
                if any(fn['instructions'][i]['shape'] and i not in matches for i in range(first, last + 1)):
                    continue
                row = statement_hit(fn, span)
                row.update(address=location(fn, first)['address'], matched_normalized=pattern,
                           match='statement' if key == pattern else 'sequence within line-attributed statement')
                yield row


def c_search(index, pattern):
    regex = re.compile(pattern, re.M)
    for fn in index['functions']:
        if fn['language'] != 'c':
            continue
        for span in fn['statements']:
            if regex.search(span['text']):
                yield statement_hit(fn, span)


def selection(arg, extent):
    if arg.startswith('1:'):
        return int(arg[2:], 16)
    if arg.startswith('+'):
        return extent + int(arg[1:], 16)
    return int(arg, 16)


def strict_blocks(index):
    by_token, candidates = defaultdict(set), []
    for fn in index['functions']:
        if fn['language'] != 'c':
            continue
        for block in fn['blocks']:
            indexes = block['instructions']
            if not any(fn['instructions'][i]['statements'] for i in indexes):
                continue
            key = code(fn, indexes, 'shape')
            for tok in set(key):
                by_token[tok].add(len(candidates))
            candidates.append((fn, block, key))
    return by_token, candidates


def rank_blocks(key, token_index, candidates, profile, unit):
    pool = set().union(*(token_index[tok] for tok in key))
    sm = difflib.SequenceMatcher(None, autojunk=False)
    sm.set_seq2(key)
    refs = set(REFERENCE.findall(' '.join(key)))
    ranking = []
    for i in sorted(pool):
        fn, block, other = candidates[i]
        sm.set_seq1(other)
        score = sm.ratio()
        if score:
            common = sorted(refs & set(REFERENCE.findall(' '.join(other))))
            ranking.append((score, bool(common), fn['profile'] == profile, fn['unit'] == unit, -i, common))
    ranking.sort(reverse=True)
    return ranking


def near(index, unit, function, block_addr, limit, obj_path=None):
    az = bd.Analyzer()
    if obj_path is None:
        plain = WORK / 'objects' / f'{unit}.plain.obj'
        report = load_json(WORK / 'neutrality.json')['units'] if plain.is_file() else {}
        if unit in report:
            row = report[unit]
            obj_path = plain.with_name(row[row['canonical']]['file'])
            if sha256_file(obj_path) != row[row['canonical']]['sha256']:
                raise ValueError(f'canonical worker object changed: {obj_path}')
        else:
            obj_path = bd.unit_obj(unit)
    pairs = az.object_pairs(obj_path, {function}, all_funcs=True)
    if len(pairs) != 1:
        raise ValueError(f'{unit}/{function}: expected one bound function')
    pair = pairs[0]
    if pair.status == 'STRICT':
        raise ValueError(f'{unit}/{function} is STRICT in {obj_path}')
    profile = next((f['profile'] for f in index['functions'] if f['unit'] == unit), None)
    by_token, candidates = strict_blocks(index)
    matches = bd.pair_blocks(pair.orig, pair.cand)
    want = selection(block_addr, pair.addr) if block_addr else None
    result = []
    for i, (offset, block) in enumerate(pair.orig.blocks):
        at = pair.addr + offset
        if (want is not None and at != want) or matches[i] is not None or not pair.orig.keys[i]:
            continue
        shape = tuple(k.strip() for k in pair.orig.keys[i])
        ranked = rank_blocks(shape, by_token, candidates, profile, unit)
        hits = []
        for score, _, _, _, neg, refs in ranked[:limit]:
            fn, hit, key = candidates[-neg]
            idx = hit['instructions']
            sources = sorted({s for ins in idx for s in fn['instructions'][ins]['statements']},
                             key=lambda s: (fn['statements'][s]['line'], fn['statements'][s]['column']))
            hits.append({**location(fn, idx[0]), 'score': round(score, 4), 'shared_references': refs,
                         'normalized': key, 'asm': code(fn, idx, 'symbolic'),
                         'sources': [{k: fn['statements'][s][k] for k in ('line', 'end_line', 'text', 'kind')}
                                     for s in sources]})
        result.append({'address': f'1:{at:05x}', 'size': pair.orig.sizes[i], 'normalized': shape,
                       'asm': tuple(x.exact.strip() for x in block if x.exact), 'hits': hits})
    if want is not None and not result:
        raise ValueError(f'1:{want:05x} is not an unmatched block start in {unit}/{function}')
    return {'unit': unit, 'function': function, 'profile': profile, 'status': pair.status,
            'oracle_extent_start': f'1:{pair.addr:05x}', 'unmatched_blocks': result,
            'worklist_blocks': sum(bool(k) and m is None for k, m in zip(pair.orig.keys, matches))}


def print_statements(rows):
    for row in rows:
        tag = ' [shared line]' if row['ambiguous'] else ''
        print(f"{row['unit']}/{row['function']} [{row['profile']}] {row['address']} {row['source']}:{row['line']}{tag}")
        print('    ' + row['text'].replace('\n', '\n    '))
        print('    ' + (' ; '.join(row['symbolic']) or '<no normalized instructions attributed>'))
        if row['normalized'] != row['symbolic']:
            print('    normalized  ' + (' ; '.join(row['normalized']) or '<empty>'))
    print(f'{len(rows)} source occurrences')


def print_near(row):
    print(f"{row['unit']}/{row['function']} [{row['profile']}] {row['status']}, {row['worklist_blocks']} unmatched blocks")
    for block in row['unmatched_blocks']:
        print(f"  {block['address']} ({block['size']} B)  " + ' ; '.join(block['asm']))
        for hit in block['hits']:
            print(f"    {hit['score']:.3f}  {hit['unit']}/{hit['function']} [{hit['profile']}] {hit['address']} {hit['source']}")
            print('      ' + ' ; '.join(hit['asm']))
            for src in hit['sources']:
                print(f"      {src['line']:>5}  {' '.join(src['text'].split())}")
        if not block['hits']:
            print('    No STRICT C block shares a normalized instruction.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--index', type=Path, default=INDEX)
    commands = parser.add_subparsers(dest='command', required=True)
    q = commands.add_parser('query', help='match a semicolon- or newline-delimited assembly sequence')
    q.add_argument('--asm', required=True)
    c = commands.add_parser('c', help='list all indexed source spans matching a C regex')
    c.add_argument('--pattern', required=True)
    n = commands.add_parser('near', help='find STRICT C examples for blockdiff worklist gaps')
    n.add_argument('unit')
    n.add_argument('function')
    n.add_argument('--block', help='oracle block start as 1:offset, hex offset, or +offset from extent start')
    n.add_argument('--limit', type=int, default=3, help='hits per block (default 3)')
    n.add_argument('--obj', type=Path, help='compare a different candidate object')
    r = commands.add_parser('reverse', help='find exact normalized oracle and candidate matches in STRICT C')
    r.add_argument('unit')
    r.add_argument('function')
    r.add_argument('--obj', type=Path, help='compare a different object with the same census status')
    for command in (q, c, n, r):
        command.add_argument('--json', action='store_true')
    args = parser.parse_args()
    try:
        index = open_index(args.index)
        if args.command == 'near':
            if args.limit < 1:
                raise ValueError('limit must be positive')
            result = near(index, args.unit, args.function, args.block, args.limit, args.obj)
            show = print_near
        elif args.command == 'reverse':
            from reverse_search import render, reverse
            census = load_json(bd.BUILD / 'blockdiff.json')['functions']
            result = reverse(index, census, bd.Analyzer(), (args.unit, args.function), args.obj)
            show = lambda data: print(render(data))
        else:
            if args.command == 'c':
                result = list(c_search(index, args.pattern))
            else:
                pattern = normalize_asm(args.asm)
                if not pattern:
                    raise ValueError('no searchable instructions after blockdiff normalization')
                result = list(query(index, pattern))
            show = print_statements
    except (OSError, ValueError, re.error) as e:
        parser.error(str(e))
    if args.json:
        print(json.dumps(result, indent=1))
    else:
        show(result)


if __name__ == '__main__':
    main()
