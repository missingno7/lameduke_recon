"""Additional single mutations over checked C bodies."""
from itertools import combinations

from pycparser import c_ast as C
from body_parser import ast_walk

from allocation import (address_sites, at_child, fresh, names,
                        nonlocal_control, stable, statements)
from c_subset import Expressions, bare, expr_walk, walk
from transforms import Edit, declaration_insert, replacement, unconditional


def use_edits(fun, enabled):
    if "##" in fun.hidden_names:
        return
    if "save" in enabled and "<statement tokens>" not in fun.hidden_names:
        yield from parameter_copies(fun)
    if "branch" in enabled:
        yield from branches(fun)
        yield from invert_ifs(fun)
    if "loopuses" in enabled and "<statement tokens>" not in fun.hidden_names:
        yield from loop_copies(fun)
    if "temp" in enabled:
        yield from stage_arguments(fun)
    if "temp" in enabled or "narrow" in enabled:
        yield from stages(fun, enabled)
    if "narrow" in enabled:
        yield from masked_short(fun)


def parameter_copies(fun):
    src = fun.src
    if nonlocal_control(fun):
        return
    for name, (typ, _) in fun.parameters.items():
        if name in fun.shadowed_parameters or typ.unsigned is None or not fun.editable_name(name):
            continue
        uses = fun.uses.get(name, ())
        if not uses or any(address_sites(fun, i) or src.values[i-1] in (".", "->", "struct", "union", "enum")
                           or src.values[i+1] == ":" for i in uses):
            continue
        temp = fresh(fun, "saved")
        decl = declaration_insert(src, fun.body, f"{typ.spelling} {temp} = {name};")
        changes = tuple(replacement(src, i, i+1, temp) for i in sorted(uses))
        yield Edit("save", f"copy parameter {name} within its body", frozenset([name]), (decl, *changes))


def outer_select(expr):
    """Find one unconditional ternary in a side-effect-free arithmetic spine."""
    expr = bare(expr)
    if expr.op == "conditional":
        return expr
    if expr.op not in {"+", "-", "*", "&", "|", "^"}:
        return None
    hits = [hit for child in expr.children if (hit := outer_select(child))]
    if len(hits) == 1:
        return hits[0]


def common_term(left, right, src):
    left, right = bare(left), bare(right)
    if left.op != right.op or left.op not in {"*", "+", "-", "&", "|", "^"}:
        return None
    for x, y in zip(left.children, right.children):
        a, b = bare(x), bare(y)
        if a.children and src.values[a.a:a.b] == src.values[b.a:b.b]:
            return a, b


def branches(fun):
    src = fun.src
    for node, root in statements(fun):
        if node.kind != "simple" or node.parent.kind != "{" or root.op != "=":
            continue
        lhs, rhs = root.children
        if bare(lhs).op not in ("id", "object") or not stable(fun, root.children[1]):
            continue
        select = outer_select(rhs)
        if select is None or not src.uncommented(node.a, node.b):
            continue
        condition, yes, no = select.children
        target = lhs
        end = node.b
        # The following copy can use a direct destination if the scratch is
        # overwritten before its next read and its address never escapes.
        block = node.parent
        siblings = block.children
        i = siblings.index(node)
        if (block is fun.body and bare(lhs).op == "id" and i+1 < len(siblings)
                and "<statement tokens>" not in fun.hidden_names):
            value = src.values[lhs.a]
            follow = siblings[i+1]
            pair = Expressions(fun, objects=True).parse(follow.parts[0]) if follow.kind == "simple" else None
            later = sorted(j for j in fun.uses[value] if j >= follow.b)
            dead = not later or (src.values[later[0]+1] == "=" and any(
                n.a == later[0] and n.parent is block and not any(
                    n.a < k < n.b for k in later[1:]) for n in siblings))
            if (pair and pair.op == "=" and bare(pair.children[1]).op == "id"
                    and src.values[bare(pair.children[1]).a] == value
                    and bare(pair.children[0]).op in ("id", "object")
                    and pair.children[0].ctype == lhs.ctype and dead
                    and not nonlocal_control(fun) and fun.editable_name(value)
                    and not any(address_sites(fun, j) for j in fun.uses[value])
                    and src.uncommented(node.a, follow.b)):
                target, end = pair.children[0], follow.b

        def branch_text(prefix="", term=None, name=None, target=target):
            values = []
            for arm, part in zip((yes, no), term or (None, None)):
                value = src.raw(arm.a, arm.b)
                if part is not None:
                    start, stop = src.span(part.a, part.b)
                    arm_start, _ = src.span(arm.a, arm.b)
                    value = value[:start-arm_start] + name + value[stop-arm_start:]
                # Keep the arm's precedence and the conditional expression's type.
                if arm.ctype != select.ctype:
                    value = f"({select.ctype.spelling})({value})"
                values.append(f"{src.raw(target.a, target.b)} = "
                              f"{src.text[src.span(rhs.a, select.a)[0]:src.span(select.a, select.b)[0]]}"
                              f"{value}{src.text[src.span(select.a, select.b)[1]:src.span(rhs.a, rhs.b)[1]]};")
            body = f"if ({src.raw(condition.a, condition.b)}) {values[0]} else {values[1]}"
            return "{ " + prefix + body + " }" if prefix else body

        change = replacement(src, node.a, end, branch_text())
        yield Edit("branch", f"branch the conditional at token {select.a}", names(fun, root), (change,))
        term = common_term(yes, no, src)
        if term and stable(fun, condition) and all(stable(fun, x) for x in term):
            name = fresh(fun, "shared")
            value = src.raw(term[0].a, term[0].b)
            text = branch_text(f"{term[0].ctype.spelling} {name} = {value}; ", term, name)
            yield Edit("branch", f"stage shared branch operand at token {term[0].a}", names(fun, root),
                       (replacement(src, node.a, end, text),))


def invert_ifs(fun):
    src = fun.src
    starts = {fun.ast_tokens.get(id(n)) for n in ast_walk(fun.ast) if isinstance(n, C.If)}
    for node in fun.nodes:
        if node.kind != "if" or node.a not in starts:
            continue
        if any(n.kind in ("goto", "label") for n in walk(node)):
            continue
        yes, *other = node.children
        lo, hi = node.parts[0]
        test = src.raw(lo, hi)
        otherwise = src.raw(other[0].a, other[0].b) if other else ""
        value = f"if (!({test})) {{ {otherwise} }} else {{ {src.raw(yes.a, yes.b)} }}"
        variables = frozenset(t.value for t in src.tokens[node.a:node.b] if t.kind == "word")
        yield Edit("branch", f"invert conditional and exchange branches at token {node.a}",
                   variables, (replacement(src, node.a, node.b, value),))


def loop_copies(fun):
    if nonlocal_control(fun):
        return
    src = fun.src
    entries = {node: expr for node, expr in statements(fun) if expr.op == "="}
    writes = {}
    for tree in ast_walk(fun.ast):
        target = (tree.lvalue if isinstance(tree, C.Assignment) else tree.expr
                  if isinstance(tree, C.UnaryOp) and tree.op in ("++", "--", "p++", "p--") else None)
        if isinstance(target, C.ID):
            writes.setdefault(target.name, set()).add(fun.ast_tokens.get(id(target)))
    for loop in fun.nodes:
        if loop.kind != "for" or loop.parent is None or loop.parent.kind != "{":
            continue
        body, = loop.children
        if body.kind != "{" or not body.children or any(n.kind == "label" for n in walk(body)):
            continue
        before = loop.parent.children[:loop.parent.children.index(loop)]
        inner = body.children[0]
        for node, inward in [(inner, False), *([(before[-1], True)] if before else [])]:
            expr = entries.get(node)
            if not expr or node.kind != "simple":
                continue
            dest, value = map(bare, expr.children)
            if dest.op != "id" or value.op != "id" or dest.a != node.a:
                continue
            dst, srcname = src.values[dest.a], src.values[value.a]
            if dst == srcname or not {dst, srcname} <= fun.parameters.keys() | fun.locals.keys():
                continue
            if not all(fun.editable_name(n) and not any(address_sites(fun, j) for j in fun.uses[n])
                       for n in (dst, srcname)):
                continue
            if any(body.a <= j < body.b and j != value.a for j in fun.uses[srcname]):
                continue
            header = range(loop.a, body.a)
            if any(j in header for n in (dst, srcname) for j in fun.uses[n]):
                continue
            if writes.get(dst) != {dest.a}:
                continue
            if dst not in fun.locals or fun.locals[dst][1] >= loop.a:
                continue
            if any(j != dest.a and not body.a < j < body.b and j > dest.a for j in fun.uses[dst]):
                continue
            # Outward movement can speculate a read when the loop is empty.
            if not inward and srcname not in fun.parameters:
                continue
            if not src.uncommented(node.a, node.b):
                continue
            placement = inner if inward else loop
            if placement.decl:
                continue
            yield Edit("loopuses", f"move invariant copy {'into' if inward else 'out of'} loop at token {loop.a}",
                       frozenset((dst, srcname)),
                       (replacement(src, node.a, node.b, ""), at_child(src, placement, src.raw(node.a, node.b))))


def stages(fun, enabled):
    """Reuse one narrow scratch for two existing nonvolatile scalar store conversions."""
    if "narrow" not in enabled or nonlocal_control(fun):
        return
    src = fun.src
    choices = []
    for node, expr in statements(fun):
        if node.kind != "simple" or node.parent is None or node.parent.kind != "{" or expr.op != "=":
            continue
        left, right = map(bare, expr.children)
        if left.ctype.rank not in {"char", "short"} or left.ctype.unsigned is None:
            continue
        if right.ctype == left.ctype or not stable(fun, left) or not stable(fun, right):
            continue
        if set(range(right.a, right.b)) & fun.macro_args:
            continue
        choices.append((node, left, right))
        temp = fresh(fun, "converted")
        raw = src.raw(right.a, right.b)
        for block in [node.parent, *([fun.body] if fun.body is not node.parent else [])]:
            decl = declaration_insert(src, block, f"{left.ctype.spelling} {temp};")
            change = (at_child(src, node, f"{temp} = {raw};"), replacement(src, right.a, right.b, temp))
            yield Edit("narrow", f"stage conversion at token {right.a}", names(fun, expr), (decl, *change))
    for (a, l, r), (b, other, s) in combinations(choices, 2):
        if l.ctype != other.ctype or a.parent is b.parent:
            continue
        temp = fresh(fun, "shared")
        decl = declaration_insert(src, fun.body, f"{l.ctype.spelling} {temp};")
        changes = tuple(change for node, rhs in ((a, r), (b, s))
                        for change in (at_child(src, node, f"{temp} = {src.raw(rhs.a, rhs.b)};"),
                                       replacement(src, rhs.a, rhs.b, temp)))
        yield Edit("narrow", f"share existing conversions at tokens {r.a}/{s.a}",
                   names(fun, l) | names(fun, r) | names(fun, other) | names(fun, s), (decl, *changes))


def masked_short(fun):
    """Extract a signed-short load and mask in the original evaluation region.

    A nonnegative, signed int literal within SHRT_MAX makes both forms
    produce the same int after promotion from the short.
    """
    src = fun.src
    placements = {n.a: n for n in fun.nodes if n.kind == "simple" and n.parent and n.parent.kind == "{"}
    scratch = fresh(fun, "mask")
    for node, root in statements(fun):
        if node.a not in placements or root.op != "=" or not stable(fun, root.children[1]):
            continue
        for expr in expr_walk(root.children[1]):
            if expr.op != "&" or not stable(fun, expr):
                continue
            left, right = map(bare, expr.children)
            if left.op not in ("id", "object") or left.ctype.spelling != "short" or right.op != "num":
                continue
            if right.ctype.spelling != "int" or not (0 <= right.bounds[0] <= 32767):
                continue
            # Do not pull a conditionally evaluated operand out of its arm.
            if expr not in unconditional(root.children[1]):
                continue
            load, mask = src.raw(left.a, left.b), src.raw(right.a, right.b)
            for block in [node.parent, *([fun.body] if node.parent is not fun.body else [])]:
                decl = declaration_insert(src, block, f"short {scratch};")
                before = at_child(src, node, f"{scratch} = {load}; {scratch} &= {mask};")
                use = replacement(src, expr.a, expr.b, scratch)
                yield Edit("narrow", f"stage short load and mask at token {expr.a}", names(fun, expr),
                           (decl, before, use))


def stage_arguments(fun):
    """Give a checked integer call argument an explicit temporary use."""
    src = fun.src
    parser = Expressions(fun, objects=True)
    starts = {fun.ast_tokens.get(id(n)) for n in ast_walk(fun.ast)
              if isinstance(n, C.FuncCall) and isinstance(n.name, C.ID)}
    scratch = fresh(fun, "arg")
    for node in fun.nodes:
        a = node.a
        if node.kind != "simple" or a not in starts or src.values[a] in src.macros:
            continue
        if src.values[a+1] != "(" or src.pairs[a+1] != node.b-2:
            continue
        close = node.b-2
        args = [i for i in src.next_top(a+2, close) if src.values[i] == ","]
        spans = zip([a+2, *(i+1 for i in args)], [*args, close])
        for low, high in spans:
            expr = parser.parse((low, high))
            if not expr or not expr.ctype or not expr.children or not stable(fun, expr):
                continue
            ctype, value = expr.ctype.spelling, src.raw(low, high)
            decl = declaration_insert(src, fun.body, f"{ctype} {scratch};")
            inline = replacement(src, low, high, f"(({scratch} = ({value})), {scratch})")
            yield Edit("temp", f"stage {src.values[a]} argument at token {low}",
                       names(fun, expr) | {src.values[a]}, (decl, inline))
