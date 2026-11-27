"""Build a separate, line-attributed C index from the external compile manifest."""
import argparse
from bisect import bisect_right
from collections import Counter, defaultdict
import gzip
import json
import re
import struct
import sys

sys.dont_write_bytecode = True
from build_external import ROOT, WORK, digest
sys.path.insert(0, str(ROOT / 'tools/idioms'))
from build_index import bd, decode_slice, load_json, omf, records
from source_spans import Source

MASK = 0xffffffff


class ExternalSource(Source):
    """Recognize ordinary definitions and old-style parameter declarations."""

    def __init__(self, text):
        super().__init__(text)
        self.positions = defaultdict(list)
        for i, value in enumerate(self.vals):
            self.positions[value].append(i)

    def function(self, name, anchor):
        defs = []
        for i in self.positions.get(name, ()):
            end = self.pairs.get(i + 1)
            if self.vals[i + 1:i + 2] != ['('] or end is None:
                continue
            body = end + 1
            if self.vals[body:body + 1] != ['{']:
                params = self.vals[i + 2:end]
                if not params or not all(re.fullmatch(r'[A-Za-z_]\w*', p) for p in params[::2]) or any(
                        p != ',' for p in params[1::2]):
                    continue
                for j in range(body, min(body + 100, len(self.vals))):
                    if self.vals[j] in ('=', '(', ')', '}', 'return'):
                        break
                    if self.vals[j] == '{':
                        if j > body + 1 and self.vals[j - 1] == ';':
                            body = j
                        break
            if self.vals[body:body + 1] == ['{'] and body in self.pairs:
                defs.append((abs(self.line(self.tokens[body].start()) - anchor), i, body))
        if not defs:
            return []
        _, name_at, body = min(defs)
        sig = name_at
        while sig and name_at - sig < 20 and self.vals[sig - 1] not in (';', '{', '}'):
            sig -= 1
        return [self.span(sig, body - 1, 'signature'), *self.statements(body, self.pairs[body] + 1)]


def line_points(blob, stats):
    anchors = defaultdict(lambda: defaultdict(set))
    for kind, body in records(blob):
        if kind not in (0x94, 0x95):
            continue
        reader = omf._R(body, True)
        reader.idx()
        sid = reader.idx()
        for line, off in struct.iter_unpack('<HI', body[reader.p:]):
            anchors[sid][off].add(line)
    rows = {}
    for sid, points in anchors.items():
        stats['ambiguous_line_anchors'] += sum(len(vals) != 1 for vals in points.values())
        rows[sid] = sorted((off, next(iter(vals))) for off, vals in points.items() if len(vals) == 1)
    return rows


def segment_relocations(mod, sid, stats):
    seg = mod.segments[sid]
    relocations = {}
    unsupported = []
    for f in mod.fixups:
        if f.seg != sid:
            continue
        # Far pointer fixups have a selector as well as an offset. The existing
        # tokenizer accepts only four-byte fields, so omit the affected function.
        if f.size() != 4:
            unsupported.append(f.off)
            stats['unsupported_fixup_' + f.kind()] += 1
            continue
        field = struct.unpack_from('<i', seg.data, f.off)[0]
        method, dest = f.target
        addend = f.disp + field
        method &= 3
        if method == 2:
            name = mod.externs[dest]
            kind = 'C' if f.self_rel or name.endswith('_') else 'D'
            token = f'{kind}:{bd.match.demangle(name)}'
            if addend:
                token += f'{addend:+#x}'
            relocations[f.off] = token
        elif method == 0:
            relocations[f.off] = (dest, addend & MASK)
        else:
            stats['unresolved_code_relocations'] += 1
            relocations[f.off] = '?'
    return relocations, sorted(unsupported)


def object_symbol(mod, names, sid, off):
    seg = mod.segments[sid]
    code = seg.cls.upper() == 'CODE'
    label = names[sid].get(off)
    if label is not None:
        return ('C:' if code else 'D:') + label
    if code:
        return f'F:{seg.name}+{off:#x}'
    if seg.cls.upper() != 'BSS' and 0 <= off < len(seg.data) and seg.covered[off]:
        return bd.render(bytes(seg.data[off:off + 48]))
    return f'D:{seg.name}+{off:#x}'


def map_function(code, start, relocs, branch, points, source, names, stats):
    ins, leaders = decode_slice(code, relocs, branch, 0, len(code))
    decoded = {i.addr for i in ins}
    anchors = {}
    for off, line in points:
        if off - start not in decoded:
            stats['mid_instruction_line_anchors'] += 1
        elif not 1 <= line <= len(source.lines):
            stats['out_of_file_line_anchors'] += 1
        else:
            anchors[off - start] = line
    first = next(iter(anchors.values()), 0)
    spans = next((s for n in names if (s := source.function(n, first))), [])
    if not spans:
        stats['functions_without_lexical_definition'] += 1
    by_line = defaultdict(list)
    for sid, span in enumerate(spans):
        span['instructions'] = []
        for line in range(span['line'], span['end_line'] + 1):
            by_line[line].append(sid)

    offsets = list(anchors)
    seq = []
    ignored = []
    for item in ins:
        if item.addr in relocs and item.size == 4 and item.shape is None:
            ignored.append(item.addr)
            continue
        at = bisect_right(offsets, item.addr) - 1
        line = anchors[offsets[at]] if at >= 0 else None
        location = 'watcom_linnum' if line is not None else 'unavailable'
        if line is not None and line not in by_line:
            by_line[line].append(len(spans))
            spans.append(dict(line=line, end_line=line, column=1, kind='unscoped_line',
                              text=source.lines[line - 1].strip(), instructions=[]))
            stats['unscoped_source_lines'] += 1
        if line is None and spans and spans[0]['kind'] == 'signature':
            line = spans[0]['line']
            location = 'function_only'
        ids = by_line.get(line, []) if at >= 0 else []
        for si in ids:
            spans[si]['instructions'].append(len(seq))
        raw = next(bd.MD.disasm(code[item.addr:item.addr + item.size], start + item.addr, count=1), None)
        asm = f'{raw.mnemonic} {raw.op_str}'.strip() if raw and raw.size == item.size else None
        seq.append(dict(offset=item.addr, size=item.size, line=line, location=location, statements=ids, asm=asm,
                        shape=item.shape.strip() if item.shape else None,
                        symbolic=item.exact.strip() if item.exact else None,
                        anchor=item.addr in anchors, kind='instruction' if asm else 'undecoded'))
    by_offset = {i['offset']: n for n, i in enumerate(seq)}
    body = bd.Body([i for i in ins if i.addr in by_offset], leaders)
    blocks = []
    for off, items in body.blocks:
        indexes = [by_offset[i.addr] for i in items]
        if not any(seq[n]['shape'] for n in indexes):
            continue
        has_statement = any(spans[s]['kind'] not in ('signature', 'brace', 'label', 'unscoped_line')
                            for n in indexes if seq[n]['shape'] for s in seq[n]['statements'])
        clean = all(seq[n]['kind'] == 'instruction' and seq[n]['shape'] != 'db' for n in indexes)
        blocks.append(dict(offset=off, instructions=indexes, has_statement=has_statement and clean))
    stats.update(functions=1, instructions=len(seq), blocks=len(blocks), source_spans=len(spans),
                 searchable_blocks=sum(b['has_statement'] for b in blocks),
                 emitting_spans=sum(bool(s['instructions']) for s in spans),
                 undecoded=sum(i['kind'] == 'undecoded' for i in seq),
                 omitted_code_data_bytes=len(ignored) * 4)
    if sum(i['size'] for i in seq) + len(ignored) * 4 != len(code):
        raise ValueError(f'{next(iter(names))}+{start:#x}: incomplete code extent accounting')
    return dict(statements=spans, instructions=seq, blocks=blocks, size=len(code))


def object_functions(obj, row, attempt, stats):
    source = ExternalSource((ROOT / attempt['source_used']).read_text(encoding='latin-1'))
    mod, = omf.load(obj)
    points = line_points(obj.read_bytes(), stats)
    publics = defaultdict(lambda: defaultdict(list))
    for raw, sid, off, local in mod.publics:
        if sid and off < mod.segments[sid].size:
            publics[sid][off].append((local, raw))
    names = defaultdict(dict)
    for sid, entries in publics.items():
        names[sid] = {off: bd.match.demangle(min(symbols)[1]) for off, symbols in entries.items()}
    for sid, seg in enumerate(mod.segments):
        if not seg or seg.cls.upper() != 'CODE' or not seg.use32:
            continue
        fixups, unsupported = segment_relocations(mod, sid, stats)
        sites = sorted(fixups)
        anchors = points.get(sid, [])
        line_offsets = [off for off, _ in anchors]
        entries = sorted(names[sid].items())
        tails = [bd.match.jump_table_tail(mod, sid, seg, entries, j) for j in range(len(entries))]
        for j, (start, name) in enumerate(entries):
            end = tails[j][0] if tails[j] else entries[j + 1][0] if j + 1 < len(entries) else seg.size
            lead = tails[j - 1][0] if j and tails[j - 1] else start
            if j == 0 and start:
                # There is no preceding public. Any prefix remains out of C, but same-function
                # references into the prefix need the local-table token at the dispatch instruction.
                lead = 0
            if end <= start:
                continue
            if bisect_right(unsupported, start - 1) != bisect_right(unsupported, end - 1):
                stats['functions_with_unsupported_relocation'] += 1
                continue
            i0, i1 = bisect_right(sites, start - 1), bisect_right(sites, end - 1)
            relocs = {}
            for site in sites[i0:i1]:
                target = fixups[site]
                if isinstance(target, tuple):
                    tsid, off = target
                    target = ('L', off - start) if tsid == sid and lead <= off < end else object_symbol(
                        mod, names, tsid, off)
                relocs[site - start] = target

            def branch(target, is_call):
                if not is_call and lead <= target + start < end:
                    return None
                return object_symbol(mod, names, sid, target + start)

            function_anchors = anchors[bisect_right(line_offsets, start - 1):bisect_right(line_offsets, end - 1)]
            all_names = [name, *(bd.match.demangle(raw) for _, raw in sorted(publics[sid][start]))]
            fn = map_function(bytes(seg.data[start:end]), start, relocs, branch, function_anchors,
                              source, dict.fromkeys(all_names), stats)
            if not fn['instructions']:
                continue
            fn.update({k: row[k] for k in ('project', 'file', 'date', 'date_basis', 'profile', 'host', 'install')},
                      corpus='external', unit=row['project'], source=attempt['source_used'],
                      source_used=attempt['source_used'], segment=seg.name, segment_offset=start,
                      function=name, language='c', kind='function', object=row['object'])
            yield fn


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--project', action='append', help='Index a subset of the compile manifest')
    parser.add_argument('--repair-index-errors', action='store_true',
                        help='Keep previous indexed functions and retry only failed objects')
    args = parser.parse_args()
    manifest = WORK / 'compile.json'
    rows = load_json(manifest)
    index = WORK / 'external.json.gz'
    inventory = WORK / 'index-inventory.json'
    old = load_json(inventory) if args.repair_index_errors else {'summary': {}, 'builds': {}}
    by_file = old['builds']
    totals = Counter(old['summary'])
    if args.repair_index_errors and args.project:
        parser.error('--repair-index-errors uses the existing index inventory')
    tmp = index.with_suffix('.tmp')
    with gzip.open(tmp, 'wt', encoding='utf-8', compresslevel=5) as out:
        out.write('{"schema":1,"corpus":"external","functions":[\n')
        first = True
        if args.repair_index_errors:
            with gzip.open(index, 'rt', encoding='utf-8') as previous:
                if previous.readline().rstrip('\n') != '{"schema":1,"corpus":"external","functions":[':
                    raise ValueError('existing index lacks line framing')
                for line in previous:
                    if line.startswith('],"summary":'):
                        break
                    if not line.strip():
                        continue
                    if not first:
                        out.write(',\n')
                    out.write(line.rstrip('\n').removesuffix(','))
                    first = False
        for key, row in rows.items():
            if row['returncode'] or args.project and row['project'] not in args.project:
                continue
            if args.repair_index_errors:
                if 'error' not in by_file.get(key, {}):
                    continue
                totals['objects_with_index_error'] -= 1
            obj = ROOT / row['object']
            attempt = row['attempts'][-1]
            if digest(obj) != row['object_sha256'] or digest(ROOT / attempt['source_used']) != attempt['source_sha256']:
                raise ValueError(f'{key}: compiled inputs changed')
            stats = Counter()
            entry = by_file[key] = dict(object=row['object'], object_sha256=row['object_sha256'])
            try:
                functions = list(object_functions(obj, row, attempt, stats))
            except (ValueError, struct.error) as e:
                entry['error'] = str(e)
                totals['objects_with_index_error'] += 1
                print(f'{key}: {e}', flush=True)
                continue
            for fn in functions:
                if not first:
                    out.write(',\n')
                first = False
                out.write(json.dumps(fn, ensure_ascii=False, separators=(',', ':')))
            entry['statistics'] = dict(stats)
            totals.update(stats)
            totals['objects_indexed'] += 1
            totals['objects_with_functions'] += bool(functions)
        out.write('\n],"summary":')
        json.dump(dict(totals), out, separators=(',', ':'))
        out.write('}')
    tmp.replace(index)
    from build_external import dump
    dump(inventory, dict(summary=dict(totals), builds=by_file))
    print(json.dumps(dict(totals), indent=2))


if __name__ == '__main__':
    main()
