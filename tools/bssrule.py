"""Predict near-data offsets emitted by Watcom 9.5b WCC386.

Pass ordered (name, byte size, emitted alignment) tuples to predict().
Use alignment 1 for packed emission (-4r or -os). With -ot use the
largest leaf scalar size, ignoring structure member packing.
A zero size consumes a symbol slot without allocating BSS. Include header
externs, prototypes, typedefs, and any synthetic symbol slots in source
order. A later definition fills an earlier zero-sized slot. A repeated
declaration of the same symbol does not consume a new slot. For shadowed
block-scope symbols use distinct placeholder names.

CLI input is a JSON array of triples. Use --segment data for an ordered
list of initialized objects in one segment, and --speed for -ot alignment.
The resulting offsets are OMF segment offsets, independent of the linker.
"""
import argparse
import json
from pathlib import Path

PAGE_SLOTS = 22
FIRST_USER_SLOT = 18
HASH_BUCKETS = 211


def name_hash(name):
    """Identifier bucket, excluding the OMF decoration on external names."""
    value = 0
    for byte in name.encode('ascii'):
        value = ((value << 4) + byte) & 0xfff
    return value % HASH_BUCKETS


def size_bucket(size):
    """Higher ranks are emitted first."""
    if size < 1:
        raise ValueError('an allocated symbol must have positive size')
    if size % 8 == 0:
        return 6
    if size % 4 == 0:
        return 5
    if size % 2 == 0:
        return 3 if size == 2 else 4
    return 1 if size == 1 else 2


def _check_alignment(alignment):
    if alignment < 1:
        raise ValueError('alignment must be positive')


def _align(offset, alignment):
    return offset + (-offset % alignment)


def predict(declarations, first_slot=FIRST_USER_SLOT):
    """Lay out _BSS from the symbol allocation stream.

    The optional first_slot supplies the first handle in an already
    flattened slice of the stream. Its phase modulo 22 is sufficient.
    """
    if first_slot < 0:
        raise ValueError('first_slot must be nonnegative')
    symbols = {}
    for name, size, alignment in declarations:
        if size < 0:
            raise ValueError(f'negative size for {name}')
        _check_alignment(alignment)
        old = symbols.get(name)
        if old is None:
            symbols[name] = (first_slot + len(symbols), size, alignment)
        elif size:
            if old[1] and old[1:] != (size, alignment):
                raise ValueError(f'conflicting definitions for {name}')
            symbols[name] = (old[0], size, alignment)
    allocated = [(name, slot, size, align)
                 for name, (slot, size, align) in symbols.items() if size]
    allocated.sort(key=lambda s: (-size_bucket(s[2]), -(s[1] // PAGE_SLOTS),
                                  name_hash(s[0]), -s[1]))
    offsets = {}
    offset = 0
    for name, slot, size, align in allocated:
        offset = _align(offset, align)
        offsets[name] = offset
        offset += size
    return offsets


def predict_initialized(definitions, speed=False):
    """Lay out named near initializers, in definition order within one segment."""
    offsets = {}
    offset = 0
    for name, size, alignment in definitions:
        if size < 1 or name in offsets:
            raise ValueError(f'invalid initialized definition for {name}')
        _check_alignment(alignment)
        offset = _align(offset, alignment if speed else 1)
        offsets[name] = offset
        offset += size
    return offsets


def predict_literals(literals, speed=False, code=False):
    """Offsets for the ordinary CONST pool, excluding strings copied into arrays.

    Supply decoded, materialized literal bytes in first-use order with the
    trailing NUL. With -zc, code=True yields offsets within the code pool.
    Watcom's physical alignment bytes can include residue.
    """
    ordered = list(dict.fromkeys(literals))
    if code:
        ordered = sorted(reversed(ordered), key=len, reverse=True)
    offsets = {}
    offset = 0
    for literal in ordered:
        offset = _align(offset, 4 if speed else 1)
        offsets[literal] = offset
        offset += len(literal)
    return offsets


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('declarations', type=Path)
    parser.add_argument('--first-slot', type=int, default=FIRST_USER_SLOT)
    parser.add_argument('--segment', choices=('bss', 'data'), default='bss')
    parser.add_argument('--speed', action='store_true')
    args = parser.parse_args()
    declarations = json.loads(args.declarations.read_text(encoding='utf-8'))
    offsets = (predict(declarations, args.first_slot) if args.segment == 'bss'
               else predict_initialized(declarations, args.speed))
    print(json.dumps(offsets, indent=2))


if __name__ == '__main__':
    main()
