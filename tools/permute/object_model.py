"""Recognise direct integer objects and members of named, non-union records."""
from collections import defaultdict
import re

from c_subset import IDENT, ObjectType, Source, Unsupported, scalar_type


def parts(src, a, b, sep):
    for i in src.next_top(a, b):
        if src.values[i] == sep:
            yield a, i
            a = i + 1
    if a < b:
        yield a, b


def declarations(src, a, b, top=False):
    start = a
    for i in src.next_top(a, b):
        token = src.tokens[i]
        if token.value == ";":
            yield start, i
            start = i + 1
        elif token.kind == "directive":
            start = i + 1
        elif top and token.value == "{" and i > start and src.values[i - 1] == ")":
            start = src.pairs[i] + 1


def objects_in(src, a, b, storage=()):
    """Yield only plain object declarators. The caller tracks unknown declarations."""
    v = src.values
    while a < b and v[a] in storage:
        a += 1
    if a >= b:
        return
    first, end = next(parts(src, a, b, ","))
    spec_end = first
    qualifiers = set()
    while spec_end < end and v[spec_end] in {"const", "volatile"}:
        qualifiers.add(v[spec_end])
        spec_end += 1
    type_start = spec_end
    if v[spec_end:spec_end + 1] == ["struct"] and spec_end + 2 < end:
        atom = v[spec_end + 1]
        spec_end += 2
    else:
        while spec_end < end and v[spec_end] in {"signed", "unsigned", "short", "long", "int", "char"}:
            spec_end += 1
        atom = scalar_type(v[type_start:spec_end])
    if atom is None or not src.macros.isdisjoint(v[first:spec_end]):
        return
    for begin, end in parts(src, spec_end, b, ","):
        pointer = v[begin] == "*"
        name = begin + int(pointer)
        if name >= end:
            continue
        if not IDENT.fullmatch(v[name]) or v[name] in src.macros:
            continue
        step, dimensions = name + 1, 0
        while step < end and v[step] == "[":
            step = src.pairs[step] + 1
            dimensions += 1
        if "const" not in qualifiers and (not pointer or not dimensions) and (
                step == end or top_initializer(v, step, end, storage)):
            yield v[name], ObjectType(atom, dimensions, pointer, "volatile" in qualifiers)


def top_initializer(values, start, end, storage):
    return bool(storage) and start < end and values[start] == "="


def object_model(paths, macros, storage=()):
    objects, members = defaultdict(set), defaultdict(set)
    unknown, member_unknown = set(), set()
    for path in paths:
        try:
            src = Source(path.read_bytes().decode("latin1"), macros)
        except Unsupported:
            continue
        for a, b in declarations(src, 0, len(src.tokens), top=True):
            if a >= b:
                continue
            v = src.values
            if v[a] == "struct" and v[a + 2:a + 3] == ["{"] and src.pairs[a + 2] == b - 1:
                tag = v[a + 1]
                for start, end in declarations(src, a + 3, b - 1):
                    accepted = dict(objects_in(src, start, end))
                    member_unknown.update((tag, word) for word in v[start:end] if word not in accepted)
                    for name, typ in accepted.items():
                        members[tag, name].add(typ)
                continue
            # A prototype's parameter names say nothing about the globals.
            # A function-pointer declarator is still an unknown declaration.
            if any(v[i] == "(" and i > a and IDENT.fullmatch(v[i - 1])
                   and v[i + 1:i + 2] != ["*"] and src.pairs[i] == b - 1 for i in src.next_top(a, b)):
                continue
            accepted = dict(objects_in(src, a, b, {"extern", "static", *storage}))
            unknown.update(word for word in v[a:b] if word not in accepted)
            for name, typ in accepted.items():
                objects[name].add(typ)
        for token in src.tokens:
            if token.kind == "directive" and "pragma" in token.value and "aux" in token.value:
                unknown.update(re.findall(r"[A-Za-z_]\w*", token.value))
    return ({name: next(iter(types)) for name, types in objects.items() if len(types) == 1 and name not in unknown},
            {name: next(iter(types)) for name, types in members.items() if len(types) == 1 and name not in member_unknown})
