"""Try three donor-derived spellings in a disposable copy of ENGINE.C."""
import os
import sys

from build_external import ROOT, WORK, dump
sys.path.insert(0, str(ROOT / 'tools/idioms'))
from build_index import bd, load_json
from reverse_search import binary_for
import doshost
import wcc


EDITS = {
    'overwritesprite': (
        0x3f6c2,
        'palookupoffs = ((long)shade)<<8;\n\tpalookupoffs += FP_OFF(palookup[dapalnum]);',
        'palookupoffs = FP_OFF(palookup[dapalnum]) + (((long)shade)<<8);',
    ),
    'hitscan': (
        0x41721,
        'if ((z1 >= intz) && (z1-((daysiz<<8)>>6) <= intz))',
        'if ((intz <= z1) && (intz >= z1-((daysiz<<8)>>6)))',
    ),
    'draw2dscreen': (
        0x444c2,
        'if ((show2dwall[i>>3]&(1<<(i&7))) == 0) continue;',
        'if ((show2dwall[i>>3]&(1<<(i&7))) <= 0) continue;',
    ),
}


def metrics(pair, target):
    matches = bd.pair_blocks(pair.orig, pair.cand)
    i = next(i for i, (off, _) in enumerate(pair.orig.blocks) if pair.addr + off == target)
    chosen = matches[i]
    return dict(verifier_status=pair.status, target_present=chosen is not None,
                target_candidate_offset=pair.cand.blocks[chosen][0] if chosen is not None else None,
                missing_blocks=sum(k and m is None for k, m in zip(pair.orig.keys, matches) if k),
                missing_bytes=sum(sz for sz, m in zip(pair.orig.sizes, matches) if m is None),
                candidate_size=pair.cand_size)


def main():
    unit = load_json(ROOT / 'config/units.json')['units']['engine']
    original = ROOT / unit['source']
    base = original.read_text(encoding='latin-1')
    variant = base
    for _, old, new in EDITS.values():
        if variant.count(old) != 1:
            raise ValueError(f'expected one occurrence of {old!r}')
        variant = variant.replace(old, new)
    scratch = WORK / 'lead-probe'
    scratch.mkdir(exist_ok=True)
    candidate = scratch / original.name
    candidate.write_bytes(variant.replace('\n', '\r\n').encode('latin-1'))
    out = scratch / 'engine.obj'
    os.environ['LAMEDUKE_HOST'] = 'dos'
    doshost.WORKER = scratch / 'dos'
    profile = wcc.profile('engine')
    rc, detail = wcc.compile_c(candidate, out, profile['flags'], profile['install'],
                               [ROOT / p for p in unit.get('include', [])] + [original.parent])
    (scratch / 'compile.log').write_text(detail, encoding='utf-8')
    if rc:
        raise ValueError(f'candidate compile failed with {rc}')
    az = bd.Analyzer()
    pairs = {label: {p.name: p for p in az.object_pairs(obj, set(EDITS), all_funcs=True)}
             for label, obj in [('registered', binary_for('engine')), ('probe', out)]}
    result = {fn: dict(address=f'1:{address:05x}', **{
        label: metrics(rows[fn], address) for label, rows in pairs.items()})
              for fn, (address, _, _) in EDITS.items()}
    dump(scratch / 'result.json', result)
    print(result, flush=True)


if __name__ == '__main__':
    main()
