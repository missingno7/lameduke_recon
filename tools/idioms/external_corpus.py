"""Normalization and lookup for the opt-in historical C corpus."""
from collections import defaultdict
from functools import lru_cache
import gzip
import json
import os
from pathlib import Path
import re
import sys

ROOT = next(p for p in Path(__file__).resolve().parents if (p / 'config/toolchain.json').is_file())
sys.path.append(str(ROOT / 'tools/idioms'))
from build_index import bd, load_json

DEFAULT_WORK = (Path(__file__).resolve().parents[1] if Path(__file__).resolve().parent.name == 'tools_proposed'
                else ROOT / 'build/idioms')
EXTERNAL_INDEX = Path(os.environ.get('IDIOMS_EXTERNAL_INDEX', DEFAULT_WORK / 'external.json.gz'))
MEMORY = re.compile(r"((?:\b[a-z]{2}:)?m\d+\[)((?:S:(?:'(?:\\.|[^'\\])*'|\"(?:\\.|[^\"\\])*\"))?[^\]]*)\]")
FIELD = re.compile(r'((?:R)?(?:\+R\*[1248])?)[+-]0x[0-9a-f]+$')
INDEX = re.compile(r'(\+R)?(\+R\*[1248])?$')
CALL = re.compile(r'^((?:call|jmp|j[a-z]+|loop[a-z]*) )(?:C:([^ ]+)|F(?:@|:)[^ ]+)$')
ADDRESS = re.compile(r'''&(?:D[:@][^, ]+|K:[0-9a-f]+|S:(?:'(?:\\.|[^'\\])*'|"(?:\\.|[^"\\])*"))''')
DATA_PREFIXES = ('D:', 'D@', 'K:', 'S:')


@lru_cache(maxsize=1)
def known_calls():
    """Canonical engine, ASM, MULTI and library names, including known aliases."""
    config = ROOT / 'config'
    units = load_json(config / 'units.json')['units']
    az = bd.Analyzer()
    protected = set(az.binds.lib)
    for unit, entry in units.items():
        if unit in ('engine', 'multi', 'a_asm') or unit.startswith('ass_'):
            protected.update(entry['functions'])
    result = {}
    for name in sorted(protected):
        symbol = bd.match.demangle(name)
        address, _ = az.binds.lookup(name)
        canon = az.refs.names.get(address, symbol)
        if address and address[0] == 1:
            result[symbol.casefold()] = canon
            result[canon.casefold()] = canon
    known = set(result.values())
    for table in (az.binds.lib, az.binds.game):
        for name, address in table.items():
            canon = az.refs.names.get(address)
            if address[0] == 1 and canon in known:
                result[bd.match.demangle(name).casefold()] = canon
    return result


def _memory(match):
    lead, addr = match.groups()
    if addr == 'D:table' or addr.startswith('D:table+'):
        return match[0]
    if addr.startswith(DATA_PREFIXES):
        suffix = INDEX.search(addr)[0]
        return f'{lead}G{suffix}]'
    field = FIELD.fullmatch(addr)
    if field and field[1]:
        return f'{lead}{field[1]}+F]'
    return match[0]


@lru_cache(maxsize=65536)
def loose_token(token):
    """Preserve address shape, operand widths, immediates and branch classes."""
    call = CALL.fullmatch(token)
    if call:
        name = known_calls().get((call[2] or '').casefold(), '?')
        return f'{call[1]}C:{name}'
    token = MEMORY.sub(_memory, token)
    return ADDRESS.sub('&G', token)


def loose_key(tokens):
    return tuple(loose_token(t) for t in tokens)


HEADER = '{"schema":1,"corpus":"external","functions":['


class ExternalFunctions:
    """Iterate one indexed function at a time from the line-framed JSON array."""

    def __init__(self, path):
        self.path = path

    def __iter__(self):
        with gzip.open(self.path, 'rt', encoding='utf-8') as stream:
            if stream.readline().rstrip('\n') != HEADER:
                raise ValueError(f'{self.path}: unsupported external index')
            for line in stream:
                if line.startswith('],"summary":'):
                    return
                if line.strip():
                    yield json.loads(line.rstrip('\n').removesuffix(','))
            raise ValueError(f'{self.path}: truncated external index')


def open_external(path=EXTERNAL_INDEX):
    return {'functions': ExternalFunctions(path)}


def block_key(fn, block):
    return tuple(fn['instructions'][i]['shape'] for i in block['instructions']
                 if fn['instructions'][i]['shape'])


def source_spans(fn, indexes):
    ids = sorted({s for n in indexes for s in fn['instructions'][n]['statements']},
                 key=lambda s: (fn['statements'][s]['line'], fn['statements'][s]['column']))
    return [{k: fn['statements'][s][k] for k in ('line', 'end_line', 'text', 'kind')} for s in ids]


def external_location(fn, offset):
    return {**{k: fn[k] for k in ('corpus', 'project', 'file', 'date', 'date_basis', 'profile', 'function',
                                 'source_used', 'source', 'unit')},
            'address': f"{fn['segment']}+{fn['segment_offset'] + offset:#x}"}


class ExternalBlocks:
    def __init__(self, index, use_loose=False):
        self.index = index
        self.exact = defaultdict(list)
        self.loose = defaultdict(list) if use_loose else None
        self.loaded = False

    def prepare(self, keys, count_all=False):
        if self.loaded:
            raise ValueError('external block keys have already been prepared')
        keys = set(keys)
        relaxed = {loose_key(key) for key in keys} if self.loose is not None else set()
        unique_exact, unique_loose = set(), set()
        for fn in self.index['functions'] if keys or count_all else ():
            for block in fn['blocks']:
                if not block['has_statement']:
                    continue
                key = block_key(fn, block)
                if not key or 'db' in key:
                    continue
                loose = loose_key(key) if self.loose is not None else None
                if count_all:
                    unique_exact.add(key)
                    unique_loose.add(loose)
                if key not in keys and loose not in relaxed:
                    continue
                indexes = block['instructions']
                row = {**external_location(fn, block['offset']), 'normalized': key,
                       'asm': tuple(fn['instructions'][i]['symbolic'] for i in indexes
                                    if fn['instructions'][i]['symbolic']),
                       'sources': source_spans(fn, indexes)}
                if key in keys:
                    self.exact[key].append(row)
                if loose in relaxed:
                    self.loose[loose].append(row)
        self.loaded = True
        return len(unique_exact), len(unique_loose)

    def hits(self, key, profile, limit):
        if not self.loaded:
            raise ValueError('external block lookup needs prepared target keys')
        exact = self.exact.get(key, [])
        relaxed = self.loose.get(loose_key(key), []) if self.loose is not None else []
        for tier, rows in (('exact', exact), ('loose', relaxed)):
            seen = set()
            for row in sorted(rows, key=lambda r: (r['profile'] != profile, r['file'], r['function'], r['address'])):
                if tier == 'loose' and row['normalized'] == key:
                    continue
                identity = row['file'], row['function'], tuple((s['line'], s['text']) for s in row['sources'])
                if identity in seen:
                    continue
                seen.add(identity)
                yield {**row, 'tier': tier}
                if len(seen) == limit:
                    break


def annotate_reverse(report, census_pair, strict, external, limit):
    matches = bd.pair_blocks(census_pair.orig, census_pair.cand)
    profile = report['profile']
    for key_name, body, partners in (
        ('oracle_blocks_with_external_donors', census_pair.orig, matches),
        ('unmatched_candidate_blocks_with_external_donors', census_pair.cand, None),
    ):
        taken = {k for k in matches if k is not None}
        rows = []
        for i, ((offset, _), tokens) in enumerate(zip(body.blocks, body.keys)):
            if not tokens or partners is None and i in taken:
                continue
            key = tuple(t.strip() for t in tokens)
            hits = list(external.hits(key, profile, limit))
            if not hits:
                continue
            position = ({'address': f'1:{census_pair.addr + offset:05x}',
                         'candidate_offset': body_offset(census_pair.cand, partners[i])}
                        if partners is not None else {'offset': offset})
            rows.append({**position, 'size': body.sizes[i], 'strict_exact': key in strict, 'hits': hits})
        report[key_name] = rows


def body_offset(body, index):
    return body.blocks[index][0] if index is not None else None


def render_reverse(report):
    lines = []
    for fn in report['functions']:
        lines.append(f"{fn['unit']}/{fn['function']} external diagnostics")
        for field in ('oracle_blocks_with_external_donors', 'unmatched_candidate_blocks_with_external_donors'):
            for row in fn[field]:
                at = row['address'] if 'address' in row else f"candidate +{row['offset']:#x}"
                state = ('missing locally' if row['candidate_offset'] is None else
                         'present locally') if 'candidate_offset' in row else 'unmatched candidate'
                lines.append(f"  {at} ({row['size']} B, {state})")
                for hit in row['hits']:
                    lines.append('    ' + hit_location(hit))
                    for span in hit['sources']:
                        lines.append(f"      {span['line']:>5}  {' '.join(span['text'].split())}")
    return '\n'.join(lines)


def hit_location(hit):
    return (f"external {hit['tier']} | {hit['project']} ({hit['date']}) | "
            f"{hit['function']} [{hit['profile']}] {hit['address']} {hit['file']}"
            + (f" (compiled from {hit['source_used']})" if hit['file'] != hit['source_used'] else ''))
