"""Local proof rules for rewrites that affect allocator lifetimes and use counts."""
from collections import defaultdict
from itertools import combinations

from c_subset import ASSIGN, CType, Expressions, bare, expr_walk, fits, pure
from transforms import (Edit, declaration_insert, declaration_without, descendants, replacement,
                        unconditional)


def fresh(fun, stem="tmp"):
    return next(f"permute_{stem}{i}" for i in range(len(fun.uses) + len(fun.src.macros) + 1)
                if f"permute_{stem}{i}" not in fun.uses and f"permute_{stem}{i}" not in fun.src.macros
                and f"permute_{stem}{i}" not in fun.hidden_names)


def address_sites(fun, i):
    """Conservative: a binary '&' may also be treated as an address site."""
    a, b = i, i + 1
    src = fun.src
    while src.values[a - 1] == "(" and src.pairs[a - 1] == b:
        a, b = a - 1, b + 1
    return (a - 1, b) if src.values[a - 1] == "&" else None


def lexical_uses(fun, name, block, index):
    if not fun.editable_name(name):
        return None
    # Member names and tags live in distinct namespaces. Leave them untouched.
    values = fun.src.values
    uses = sorted(i for i in fun.uses[name] - {index}
                  if values[i - 1] not in (".", "->", "struct", "union", "enum"))
    if any(not index < i < block.b or values[i + 1] == ":" for i in uses):
        return None
    return uses


def first_write(fun, block, site, uses):
    src = fun.src
    node = next((n for n in block.children if n.a == site), None)
    return (node and node.kind == "simple" and src.values[site + 1] == "="
            and not any(site < i < node.b for i in uses))


def nonlocal_control(fun):
    return (any(n.kind == "goto" for n in fun.nodes)
            or any(name.lstrip("_") in ("setjmp", "sigsetjmp", "setjmp3")
                   for name in fun.uses.keys() | fun.hidden_names))


def at_child(src, node, spelling):
    return replacement(src, node.a, node.a, spelling + src.nl + src.indent(node.a))


def move_scope(fun, name, decl, index, uses):
    src = fun.src
    if nonlocal_control(fun):
        return
    block = decl.stmt.parent
    inner = [n for n in descendants(block) if n.kind == "{" and n.a < uses[0] <= uses[-1] < n.b]
    destinations = inner[-1:]
    outer = block.parent
    while outer:
        if outer.kind == "{":
            destinations.append(outer)
        outer = outer.parent
    old = replacement(src, decl.stmt.a, decl.stmt.b, declaration_without(fun, decl, index))
    for dest in destinations:
        child, parent = (dest, block) if dest.a > block.a else (block, dest)
        cursor, loop = child.parent, False
        while cursor != parent:
            loop |= cursor.kind in ("while", "for", "do")
            cursor = cursor.parent
        if loop and (not first_write(fun, child, uses[0], uses)
                     or any(n.kind == "label" for n in descendants(child))):
            continue
        spelling = f"{decl.spec} {name};"
        # Different positions permit crossing the destination's independent
        # declarations as well as changing the local's lexical scope.
        yield Edit("scope", f"move {name} to start of block at token {dest.a}", frozenset([name]),
                   (old, declaration_insert(src, dest, spelling)))
        for node in dest.children:
            if not node.decl or node.decl.initializer or node.a >= uses[0]:
                break
            if node is not decl.stmt:
                pos = src.tokens[node.b - 1].end
                after = (pos, pos, src.nl + src.indent(node.a) + spelling)
                yield Edit("scope", f"move {name} after declaration at token {node.a}", frozenset([name]),
                           (old, after))


def split_range(fun, name, decl, uses):
    src = fun.src
    block = decl.stmt.parent
    if nonlocal_control(fun) or any(n.kind == "label" and n.parent is block for n in fun.nodes):
        return
    for i, cut in enumerate(uses[1:-1], 1):
        if not first_write(fun, block, cut, uses):
            continue
        new = fresh(fun, "range")
        insert = declaration_insert(src, block, f"{decl.spec} {new};")
        rename = tuple(replacement(src, j, j + 1, new) for j in uses[i:])
        yield Edit("ranges", f"split {name} at token {cut}", frozenset([name]), (insert, *rename))


def merge_ranges(fun, candidates):
    src = fun.src
    if nonlocal_control(fun):
        return
    for left, right in combinations(candidates, 2):
        early, late = sorted((left, right), key=lambda row: row[3][0])
        name, decl, index, uses = early
        other, other_decl, other_index, other_uses = late
        block = decl.stmt.parent
        if (block is not other_decl.stmt.parent or decl.spec != other_decl.spec
                or uses[-1] >= other_uses[0] or not first_write(fun, block, other_uses[0], other_uses)
                or any(n.kind == "label" and n.parent is block for n in fun.nodes)):
            continue
        remove = replacement(src, other_decl.stmt.a, other_decl.stmt.b,
                             declaration_without(fun, other_decl, other_index))
        rename = tuple(replacement(src, i, i + 1, name) for i in other_uses)
        yield Edit("ranges", f"merge disjoint {name} and {other}", frozenset((name, other)), (remove, *rename))


def address_edits(fun, name, decl, uses):
    if "register" in decl.spec.split():
        return
    src = fun.src
    for i in uses:
        addr = address_sites(fun, i)
        if addr and src.values[addr[0]:addr[1]] == ["&", name]:
            a, b = addr
            # Existing address uses must retain object identity. Recognise
            # direct *(&x), or stage &x when it is an entire call argument.
            if (src.values[a - 2:a] == ["*", "("] and src.pairs[a - 1] == b
                    and src.uncommented(a - 2, b + 1)):
                yield Edit("address", f"remove immediate & and * for {name}", frozenset([name]),
                           (replacement(src, a - 2, b + 1, f"({name})"),))
            if src.values[b] in (",", ")") and src.values[a - 1] in ("(", ",") and not (
                    src.values[a - 2:a] == ["*", "("]):
                # Confirm an enclosing direct call, excluding sizeof and
                # other non-evaluated or macro contexts.
                callers = [p for p, q in src.pairs.items() if src.values[p] == "(" and p < a < q
                           and a in set(src.next_top(p + 1, q)) and src.values[p - 1].isidentifier()]
                if len(callers) != 1:
                    continue
                callee = src.values[callers[0] - 1]
                if callee in src.macros or callee in {"sizeof", "if", "while", "for", "switch", "return"}:
                    continue
                ptr = fresh(fun, "ptr")
                storage = declaration_insert(src, fun.body, f"{decl.ctype.spelling} *{ptr};")
                yield Edit("address", f"stage the existing address of {name}", frozenset([name]),
                           (storage, replacement(src, a, b, f"(({ptr} = &{name}), {ptr})")))
        elif not addr and src.values[i - 1] not in ("sizeof", "++", "--") and (
                src.values[i + 1] in ("=", "+=", "-=", "*=", "/=", "%=")):
            yield Edit("address", f"use immediate *& for the store to {name}", frozenset([name]),
                       (replacement(src, i, i + 1, f"(*(&{name}))"),))


def names(fun, expr):
    src = fun.src
    return frozenset(src.values[i] for i in range(expr.a, expr.b) if src.tokens[i].kind == "word")


def stable(fun, expr):
    return (pure(expr) and fun.src.uncommented(expr.a, expr.b)
            and not (set(range(expr.a, expr.b)) & fun.macro_args))


def statements(fun):
    parser = Expressions(fun, objects=True)
    for node in fun.nodes:
        if len(node.parts) != 1:
            continue
        expr = parser.parse(node.parts[0])
        if expr and expr.ctype:
            yield node, bare(expr)


def locations(fun, expr):
    """A direct object's identity plus textual array selectors and record members."""
    src = fun.src
    expr = bare(expr)
    if expr.op in ("id", "object"):
        return (src.values[expr.a],)
    if expr.op == "member":
        return (*locations(fun, expr.children[0]), (".", src.values[expr.b - 1]))
    if expr.op == "index":
        base, idx = expr.children
        idx = bare(idx)
        return (*locations(fun, base), ("[", tuple(src.values[idx.a:idx.b])))
    return ()


def reads(fun, expr):
    expr = bare(expr)
    if expr.op in ("id", "object"):
        yield locations(fun, expr)
    elif expr.op == "member":
        yield locations(fun, expr)
        yield from index_reads(fun, expr)
    elif expr.op == "index":
        yield locations(fun, expr)
        yield from index_reads(fun, expr)
    else:
        for child in expr.children:
            yield from reads(fun, child)


def index_reads(fun, expr):
    expr = bare(expr)
    if expr.op in ("index", "member"):
        yield from index_reads(fun, expr.children[0])
        if expr.op == "index":
            yield from reads(fun, expr.children[1])


def disjoint(left, right):
    if left[0] != right[0]:
        return True
    for a, b in zip(left[1:], right[1:]):
        if a != b:
            return a[0] == b[0] == "."
    return False


def direct_store(fun, expr):
    if expr.op != "=":
        return None
    lhs, rhs = expr.children
    if not stable(fun, lhs) or not stable(fun, rhs):
        return None
    return locations(fun, lhs), [*index_reads(fun, lhs), *reads(fun, rhs)]


def independent_stores(a, b):
    dest, used = a
    other, inputs = b
    return all(disjoint(dest, x) for x in (other, *inputs)) and all(disjoint(other, x) for x in used)


def store_edits(fun, entries):
    src = fun.src
    forms = {node: expr for node, expr in entries if node.kind == "simple"}
    stores = {}
    for node, expr in forms.items():
        access = direct_store(fun, expr)
        if access and src.uncommented(node.a, node.b):
            stores[node] = access
    for block in fun.nodes:
        if block.kind != "{":
            continue
        for i, node in enumerate(block.children):
            if node not in stores:
                continue
            crossed, can_delay = [node], True
            for later in block.children[i + 1:]:
                if later not in stores or not src.uncommented(node.a, later.b):
                    break
                gap = src.nl + src.indent(node.a)
                middle = src.text[src.tokens[node.b - 1].end:src.tokens[later.a].start]
                if all(independent_stores(stores[n], stores[later]) for n in crossed):
                    touched = names(fun, forms[later])
                    yield Edit("statements", f"advance independent store at token {later.a} to {node.a}", touched,
                               (replacement(src, node.a, later.b,
                                            src.raw(later.a, later.b) + gap + src.raw(node.a, node.b) + middle.rstrip()),))
                can_delay &= independent_stores(stores[node], stores[later])
                if can_delay and len(crossed) > 1:
                    yield Edit("statements", f"delay independent store at token {node.a} to {later.a}", names(fun, forms[node]),
                               (replacement(src, node.a, later.b,
                                            src.text[src.tokens[node.b - 1].end:src.tokens[later.b - 1].end].lstrip()
                                            + gap + src.raw(node.a, node.b)),))
                crossed.append(later)


def stage(fun, node, expr, enabled):
    src = fun.src
    if (node.kind not in ("simple", "return", "if") or node.parent.kind != "{"
            or "##" in fun.hidden_names):
        return
    root = expr.children[1] if expr.op == "=" and stable(fun, expr.children[0]) else expr
    if not stable(fun, root):
        return
    groups = defaultdict(list)
    for site in unconditional(root):
        if site.op == "group" or not site.ctype or not site.children:
            continue
        if not stable(fun, site):
            continue
        groups[tuple(src.values[site.a:site.b])].append(site)
    temp = fresh(fun)
    for sites in groups.values():
        site = sites[0]
        widths = []
        if "temp" in enabled:
            widths.append(("temp", site.ctype, sites))
        if "narrow" in enabled and site.op == "cast" and site.ctype.rank in ("char", "short"):
            widths.append(("narrow", site.ctype, sites[:1]))
        if "narrow" in enabled and site.ctype.rank in ("int", "long"):
            for rank in ("short", "char"):
                for unsigned in (False, True):
                    typ = CType(rank, unsigned)
                    if fits(site.bounds, typ):
                        widths.append(("narrow", typ, sites[:1]))
        for kind, typ, targets in widths:
            value = site.children[0] if kind == "narrow" and site.op == "cast" else site
            ref = temp if typ == site.ctype else f"(({site.ctype.spelling}){temp})"
            assign = at_child(src, node, f"{temp} = {src.raw(value.a, value.b)};")
            use = tuple(replacement(src, e.a, e.b, ref) for e in targets)
            blocks = [node.parent] if node.parent == fun.body else [node.parent, fun.body]
            for block in blocks:
                decl = declaration_insert(src, block, f"{typ.spelling} {temp};")
                yield Edit(kind, f"stage {len(targets)} use(s) at token {site.a} as {typ.spelling}",
                           names(fun, site), (decl, assign, *use))
    if "narrow" in enabled and root is not expr:
        typ = expr.children[0].ctype
        if typ.rank in ("char", "short") and root.ctype != typ:
            assign = at_child(src, node, f"{temp} = {src.raw(root.a, root.b)};")
            use = replacement(src, root.a, root.b, temp)
            yield Edit("narrow", f"stage the existing {typ.spelling} store conversion at token {root.a}",
                       names(fun, expr),
                       (declaration_insert(src, node.parent, f"{typ.spelling} {temp};"), assign, use))


def save_parameter(fun, entries):
    src = fun.src
    if "##" in fun.hidden_names:
        return
    for name, (typ, index) in fun.parameters.items():
        if (name in fun.shadowed_parameters or not fun.editable_name(name) or not fun.uses.get(name)
                or any(address_sites(fun, i) or src.values[i - 1] in (".", "->", "struct", "union", "enum")
                       or src.values[i + 1] == ":" for i in fun.uses[name])):
            continue
        if (src.values[index - 1] in src.macros or typ.unsigned is None):
            continue
        formal = fresh(fun, "input")
        declaration = f"{typ.spelling} {name} = {formal};"
        signature = replacement(src, index, index + 1, formal)
        yield Edit("save", f"copy incoming {name} to a local", frozenset([name]),
                   (signature, declaration_insert(src, fun.body, declaration)))
    for name, (decl, index) in fun.locals.items():
        if not decl.initializer or decl.stmt.parent is not fun.body or not fun.editable_name(name):
            continue
        a, b = decl.initializer
        old = src.values[a]
        if (b != a + 1 or old not in fun.parameters or old in fun.shadowed_parameters
                or fun.parameters[old][0] != decl.ctype or fun.uses.get(old) != {a}
                or not fun.editable_name(old) or not src.uncommented(decl.stmt.a, decl.stmt.b)):
            continue
        uses = lexical_uses(fun, name, fun.body, index)
        if not uses or any(address_sites(fun, site) for site in uses):
            continue
        for node, expr in entries:
            if node.kind != "simple" or node.parent is not fun.body or expr.op != "=":
                continue
            dest, value = map(bare, expr.children)
            if (value.op != "id" or value.a != uses[0] or dest.ctype.rank not in ("char", "short")
                    or dest.ctype == decl.ctype or not fits(dest.ctype.bounds, decl.ctype)
                    or any(not n.decl for n in fun.body.children[:fun.body.children.index(node)])):
                continue
            new = fresh(fun, "saved")
            move = replacement(src, decl.stmt.a, decl.stmt.b, "")
            insert = at_child(src, node, f"{decl.spec} {new} = {name};")
            _, parameter = fun.parameters[old]
            yield Edit("save", f"keep incoming {name} in {new} before narrowing", frozenset((old, name, *names(fun, dest))),
                       (move, insert, replacement(src, parameter, parameter + 1, name),
                        replacement(src, value.a, value.b, new)))
            break


def collapse_store(fun, entries):
    """Commit an integer update in the initializer if intervening reads cannot see it."""
    src = fun.src
    expressions = dict(entries)
    for block in fun.nodes:
        if block.kind != "{":
            continue
        for i, decl in enumerate(block.children):
            init = expressions.get(decl)
            if not decl.decl or not decl.decl.initializer or not init or init.op not in ("+", "-"):
                continue
            idx, = decl.decl.names
            local = src.values[idx]
            left, right = map(bare, init.children)
            if left.op != "object" or right.op != "num" or decl.decl.ctype != left.ctype:
                continue
            target, = locations(fun, left)
            if not stable(fun, init) or not fun.editable_name(local) or not fun.editable_name(target):
                continue
            for node in block.children[i + 1:]:
                expr = expressions.get(node)
                if not expr:
                    break
                if node.kind == "decl" and stable(fun, expr) and all(
                        disjoint((target,), obj) for obj in reads(fun, expr)):
                    continue
                if node.kind != "simple" or expr.op != "=" or not src.uncommented(node.a, node.b):
                    break
                dst, rhs = map(bare, expr.children)
                if locations(fun, dst) == (target,) and rhs.op == "id" and src.values[rhs.a] == local:
                    yield Edit("chain", f"commit {target} in the initializer of {local}", frozenset((local, target)),
                               (replacement(src, *decl.decl.initializer, f"({target} {init.op}= {src.raw(right.a, right.b)})"),
                                replacement(src, node.a, node.b, "")))
                break


def inline_initialized(fun, entries):
    src = fun.src
    expressions = dict(entries)
    for node, expr in entries:
        if node.kind not in ("return", "simple") or node.parent.kind != "{":
            continue
        siblings = node.parent.children
        position = siblings.index(node)
        if not position:
            continue
        previous = siblings[position - 1]
        value = expressions.get(previous)
        if (not previous.decl or not previous.decl.initializer or not value or not stable(fun, value)
                or not src.uncommented(previous.a, previous.b)):
            continue
        idx, = previous.decl.names
        name = src.values[idx]
        if not fun.editable_name(name):
            continue
        root = expr.children[1] if expr.op == "=" and stable(fun, expr.children[0]) else expr
        if not stable(fun, root):
            continue
        sites = [e for e in unconditional(root) if e.op == "id" and src.values[e.a] == name]
        if not sites or fun.uses[name] != {idx, *(e.a for e in sites)}:
            continue
        typ = previous.decl.ctype
        spelling = f"(({typ.spelling})({src.raw(value.a, value.b)}))"
        yield Edit("temp", f"inline {len(sites)} use(s) of initialized {name}", names(fun, value) | {name},
                   (replacement(src, previous.a, previous.b, ""),
                    *(replacement(src, e.a, e.b, spelling) for e in sites)))


def volatile_index_staging(fun):
    """Capture one volatile index read while retaining the original store."""
    src = fun.src
    v = src.values
    parser = Expressions(fun, objects=True)
    for node in fun.nodes:
        if (node.kind != "simple" or node.parent.kind != "{" or not src.uncommented(node.a, node.b)
                or set(range(node.a, node.b)) & fun.macro_args or "##" in fun.hidden_names):
            continue
        pos = next((i for i in src.next_top(node.a, node.b) if v[i] == "="), None)
        if pos is None:
            continue
        rhs = parser.parse((pos + 1, node.b - 1))
        if not rhs or not rhs.ctype or not stable(fun, rhs):
            continue
        a = node.a
        if v[a + 1:pos] and v[a + 1:pos][:2] == ["[", "*"] and v[a + 4:pos] == ["]"]:
            base, index = v[a], a + 2
        elif (v[a:a + 2] == ["*", "("] and src.pairs[a + 1] + 1 < pos):
            k = src.pairs[a + 1] + 1
            form = v[k:pos]
            # Recognise integer pointer arithmetic without interpreting or
            # changing the implementation-defined pointer casts themselves.
            if (len(form) != 12 or form[:4] != ["(", "(", "long", ")"]
                    or form[5:10] != ["+", "(", "long", ")", "*"] or form[-1] != ")"
                    or v[k - 2] != "*"):
                continue
            base, index = form[4], k + 9
        else:
            continue
        value = v[index + 1]
        if any(n in fun.declared_names or n in src.macros or not fun.editable_name(n) for n in (base, value)):
            continue
        ptr, idx = src.objects.get(base), src.objects.get(value)
        if not (ptr and ptr.pointer and idx and idx.pointer and idx.volatile and isinstance(idx.atom, CType)):
            continue
        temp = fresh(fun, "index")
        for typ in (idx.atom, idx.atom.promoted, CType("long")):
            if not fits(idx.atom.bounds, typ):
                continue
            cast = temp if idx.atom == typ else f"(({idx.atom.spelling}){temp})"
            save = at_child(src, node, f"{temp} = *{value};")
            use = replacement(src, index, index + 2, cast)
            for block in dict.fromkeys((node.parent, fun.body)):
                decl = declaration_insert(src, block, f"{typ.spelling} {temp};")
                yield Edit("narrow", f"stage one volatile index read through {typ.spelling} at token {index}",
                           frozenset((base, value)), (decl, save, use))


def allocation_edits(fun, enabled):
    if "<statement tokens>" in fun.hidden_names:
        return
    candidates = []
    for name, (decl, index) in fun.locals.items():
        stmt = decl.stmt
        if decl.initializer or not fun.src.uncommented(stmt.a, stmt.b):
            continue
        uses = lexical_uses(fun, name, stmt.parent, index)
        if not uses:
            continue
        if "address" in enabled:
            yield from address_edits(fun, name, decl, uses)
        if any(address_sites(fun, i) for i in uses):
            continue
        candidates.append((name, decl, index, uses))
        if "scope" in enabled:
            yield from move_scope(fun, name, decl, index, uses)
        if "ranges" in enabled:
            yield from split_range(fun, name, decl, uses)
    if "ranges" in enabled:
        yield from merge_ranges(fun, candidates)
    if "narrow" in enabled:
        yield from volatile_index_staging(fun)
    if enabled & {"temp", "narrow", "save", "statements", "chain"}:
        entries = list(statements(fun))
        if enabled & {"temp", "narrow"}:
            for node, expr in entries:
                yield from stage(fun, node, expr, enabled)
        if "save" in enabled:
            yield from save_parameter(fun, entries)
        if "statements" in enabled:
            yield from store_edits(fun, entries)
        if "chain" in enabled:
            yield from collapse_store(fun, entries)
        if "temp" in enabled:
            yield from inline_initialized(fun, entries)
