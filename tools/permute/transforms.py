"""Meaning-preserving edit generators over the checked source subset."""
from dataclasses import dataclass
from itertools import combinations, permutations, product
from math import factorial, prod

from c_subset import ASSIGN, PRECEDENCE, UPDATE, CType, Expressions, bare, expr_walk, fits, pure


NAMES = ("decl", "scope", "types", "ranges", "address", "temp", "narrow", "save",
         "commute", "statements", "loops", "chain", "updates")
REVERSE = {"<": ">", ">": "<", "<=": ">=", ">=": "<="}
COMMUTATIVE = {"+", "*", "&", "|", "^", "==", "!="}


@dataclass(frozen=True)
class Edit:
    kind: str
    detail: str
    variables: frozenset
    changes: tuple

    def apply(self, text):
        # Apply a consuming edit before inserts at the same position. Insert
        # ties in reverse input order so earlier entries appear first.
        ordered = sorted(enumerate(self.changes), key=lambda item: (*item[1][:2], item[0]), reverse=True)
        for _, (start, end, value) in ordered:
            text = text[:start] + value + text[end:]
        return text


def replacement(src, a, b, text):
    return (*src.span(a, b), text)


def decl_runs(fun):
    for block in fun.nodes:
        if block.kind != "{":
            continue
        run = []
        for child in block.children:
            if child.decl and not child.decl.initializer and fun.src.uncommented(child.a, child.b):
                if run and not fun.src.uncommented(run[-1].stmt.a, child.b):
                    yield run
                    run = []
                run.append(child.decl)
            elif run:
                yield run
                run = []
        if run:
            yield run


def render_declarations(groups, sep):
    return sep.join(f"{spec} {', '.join(names)};" for spec, names in groups if names)


def type_runs(pairs):
    groups = []
    for spec, name in pairs:
        if groups and groups[-1][0] == spec:
            groups[-1][1].append(name)
        else:
            groups.append((spec, [name]))
    return groups


def swaps(items):
    for i, j in combinations(range(len(items)), 2):
        choice = list(items)
        choice[i], choice[j] = choice[j], choice[i]
        yield choice


def orders(groups):
    # Previous searches found both declaration-line orders and cross-list
    # transpositions useful. Enumerate the small products in each family.
    items = [(spec, name) for spec, names in groups for name in names]
    variants = [list(permutations(names)) if len(names) <= 3 else [names] for _, names in groups]
    if factorial(len(groups)) * prod(map(len, variants)) <= 720:
        for order in permutations(range(len(groups))):
            for choice in product(*(variants[i] for i in order)):
                yield [(groups[i][0], names) for i, names in zip(order, choice)]
    else:
        yield from swaps(groups)
    yield type_runs(items)
    yield [(spec, [name]) for spec, name in items]
    if factorial(len(items)) <= 120:
        for choice in permutations(items):
            yield type_runs(choice)
            yield [(spec, [name]) for spec, name in choice]
    else:
        for choice in swaps(items):
            # Preserve the original lists if the types still fit their slots.
            if [spec for spec, _ in choice] == [spec for spec, _ in items]:
                rebuilt, start = [], 0
                for spec, names in groups:
                    end = start + len(names)
                    rebuilt.append((spec, [name for _, name in choice[start:end]]))
                    start = end
                yield rebuilt
            yield type_runs(choice)
    for i, (spec, names) in enumerate(groups):
        for cut in range(1, len(names)):
            yield groups[:i] + [(spec, names[:cut]), (spec, names[cut:])] + groups[i + 1:]
        if i and groups[i - 1][0] == spec:
            yield groups[:i - 1] + [(spec, groups[i - 1][1] + names)] + groups[i + 1:]


def declaration_edits(fun):
    src = fun.src
    for run in decl_runs(fun):
        first, last = run[0].stmt, run[-1].stmt
        sep = src.nl + src.indent(first.a)
        groups = [(d.spec, tuple(src.values[i] for i in d.names)) for d in run]
        original = [name for _, names in groups for name in names]
        old = render_declarations(groups, sep)
        # Formatting alone is never an edit.
        seen = {old}
        for group in orders(groups):
            new = render_declarations(group, sep)
            if new in seen:
                continue
            seen.add(new)
            names = [name for _, names in group for name in names]
            moved = {right for left, right in zip(original, names) if left != right}
            detail = " / ".join(", ".join(ns) for _, ns in group)
            yield Edit("decl", detail, frozenset(moved or names), (replacement(src, first.a, last.b, new),))


def loop_edits(fun):
    src = fun.src
    if src.macros & {"for", "while"}:
        return
    for node in fun.nodes:
        if node.kind == "while":
            a, b = node.parts[0]
            if a != b and src.uncommented(node.a, b + 1) and src.macros.isdisjoint(src.values[a:b]):
                yield Edit("loops", "while to for", frozenset(src.values[a:b]),
                           (replacement(src, node.a, b + 1, f"for (; {src.raw(a, b)}; )"),))
        if node.kind != "for" or not src.uncommented(node.a, node.b):
            continue
        init, cond, step = node.parts
        body, = node.children
        values = src.values[node.a:node.b]
        if not src.macros.isdisjoint(values):
            continue
        test = src.raw(*cond) or "1"
        if init[0] == init[1] and step[0] == step[1]:
            yield Edit("loops", "for to while", frozenset(values),
                       (replacement(src, node.a, body.a, f"while ({test})"),))
        elif no_loop_jumps(body):
            parser = Expressions(fun)
            forms = [parser.parse(span) for span in (init, step) if span[0] != span[1]]
            if not all(forms):
                continue
            before = f"{src.raw(*init)}; " if init[0] != init[1] else ""
            after = f" {src.raw(*step)};" if step[0] != step[1] else ""
            # Keep the body as a nested statement, including its declaration scope.
            spelling = f"{{ {before}while ({test}) {{ {src.raw(body.a, body.b)}{after} }} }}"
            yield Edit("loops", "for to while with init and step", frozenset(values),
                       (replacement(src, node.a, node.b, spelling),))
    if not fun.opaque_macro_statement:
        yield from fold_while(fun)


def descendants(stmt):
    for child in stmt.children:
        yield child
        yield from descendants(child)


def no_loop_jumps(body):
    return all(n.kind not in ("continue", "goto", "label") for n in (body, *descendants(body)))


def fold_while(fun):
    src = fun.src
    parser = Expressions(fun)
    for before, loop in adjacent(fun):
        if before.kind != "simple" or loop.kind != "while" or not src.uncommented(before.a, loop.b):
            continue
        body, = loop.children
        if body.kind != "{" or not body.children or not no_loop_jumps(body):
            continue
        step = body.children[-1]
        if step.kind != "simple" or not src.macros.isdisjoint(src.values[before.a:loop.b]):
            continue
        init, update = parser.parse(before.parts[0]), parser.parse(step.parts[0])
        if not init or not update or not discard_assignment(bare(init)):
            continue
        update = bare(update)
        if update.op not in ASSIGN | UPDATE or not independent(update):
            continue
        variables = identifiers(fun, update)
        if any(n in fun.locals and fun.locals[n][0].stmt.a >= body.a for n in variables):
            continue
        header = f"for ({src.raw(*before.parts[0])}; {src.raw(*loop.parts[0])}; {src.raw(*step.parts[0])})"
        yield Edit("loops", "fold preceding init and loop tail into for",
                   frozenset(src.values[before.a:loop.b]), (
                       replacement(src, before.a, body.a, header),
                       replacement(src, step.a, step.b, ""),
                   ))


def identifiers(fun, expr):
    return frozenset(fun.src.values[e.a] for e in expr_walk(expr) if e.op == "id")


def used_as_statement(node, part):
    return node.kind == "simple" or (node.kind == "for" and part in (0, 2))


def adjacent(fun):
    for block in fun.nodes:
        if block.kind == "{":
            yield from zip(block.children, block.children[1:])


def independent(expr):
    if expr.op in ASSIGN:
        return pure(expr.children[1])
    return expr.op in UPDATE or pure(expr)


def discard_assignment(expr):
    return expr.op == "=" and bare(expr.children[0]).op == "id" and pure(expr.children[1])


def declaration_without(fun, decl, removed):
    return render_declarations([(decl.spec, [fun.src.values[i] for i in decl.names if i != removed])], "")


def declaration_insert(src, block, spelling):
    indent = src.indent(block.children[0].a) if block.children else src.indent(block.a) + "    "
    pos = src.tokens[block.a].end
    return pos, pos, f"{src.nl}{indent}{spelling}"


class Effects:
    def __init__(self, fun):
        self.fun, self.src = fun, fun.src
        parser = Expressions(fun)
        self.entries = []
        self.simple = {}
        for node in fun.nodes:
            for index, span in enumerate(node.parts):
                expr = parser.parse(span)
                if expr is None:
                    continue
                expr = bare(expr)
                self.entries.append((node, index, expr))
                if node.kind in ("return", "simple"):
                    self.simple[node] = expr

    def closed_uses(self, name, decl_index):
        # No hidden macro use, address taking, sizeof, unknown declarator or
        # other unmodelled reference may survive a type/scope change.
        if not self.fun.editable_name(name):
            return None
        uses = []
        for node, part, expr in self.entries:
            for atom in expr_walk(expr):
                if atom.op == "id" and self.src.values[atom.a] == name:
                    uses.append((node, part, expr, atom))
        if {decl_index, *(atom.a for _, _, _, atom in uses)} != self.fun.uses[name]:
            return None
        return uses

    def scoped_edits(self, enabled):
        for name, (decl, index) in self.fun.locals.items():
            stmt = decl.stmt
            if not self.src.uncommented(stmt.a, stmt.b):
                continue
            uses = self.closed_uses(name, index)
            if not uses:
                continue
            if "types" in enabled:
                yield from self.change_type(name, decl, index, uses)

    def change_type(self, name, decl, index, uses):
        parser = Expressions(self.fun, objects=True)
        initial = parser.parse(decl.initializer) if decl.initializer else None
        if decl.initializer and (not initial or not initial.bounds):
            return
        writes, reads = ([initial.bounds] if initial else []), []
        for node, part, expr, atom in uses:
            # Match complete value contexts; assignment expressions and updates
            # whose results are consumed are deliberately outside this proof.
            if pure(expr):
                reads.append(atom)
            elif used_as_statement(node, part) and discard_assignment(expr):
                left, right = expr.children
                if bare(left) == atom:
                    writes.append(right.bounds)
                else:
                    reads.append(atom)
            else:
                return
        if not writes:
            return
        old = decl.ctype
        if old.unsigned is None:
            return
        others = declaration_without(self.fun, decl, index)
        sep = self.src.nl + self.src.indent(decl.stmt.a)
        storage = "register " if "register" in decl.spec.split() else ""
        for rank, unsigned in product(("char", "short", "int", "long"), (False, True)):
            typ = CType(rank, unsigned)
            if typ == old or not all(fits(bounds, old) and fits(bounds, typ) for bounds in writes):
                continue
            value = f" = {self.src.raw(*decl.initializer)}" if decl.initializer else ""
            declaration = f"{storage}{typ.spelling} {name}{value};"
            edits = [replacement(self.src, decl.stmt.a, decl.stmt.b,
                                 others + sep + declaration if others else declaration)]
            # A cast back to the old value type preserves the usual arithmetic
            # conversions and the type observed by every modelled consumer.
            if typ != old:
                edits.extend(replacement(self.src, r.a, r.b, f"(({old.spelling}){name})") for r in reads)
            yield Edit("types", f"{name}: {old.spelling} to {typ.spelling}", frozenset([name]), tuple(edits))

    def neighbouring_edits(self, enabled):
        src = self.src
        for before, after in adjacent(self.fun):
            left, right = self.simple.get(before), self.simple.get(after)
            if left is None or right is None or not src.uncommented(before.a, after.b):
                continue
            if "statements" in enabled and before.kind == after.kind == "simple":
                if independent(left) and independent(right):
                    a, b = identifiers(self.fun, left), identifiers(self.fun, right)
                    if a and b and a.isdisjoint(b):
                        gap = src.text[src.tokens[before.b - 1].end:src.tokens[after.a].start]
                        swap = src.raw(after.a, after.b) + gap + src.raw(before.a, before.b)
                        yield Edit("statements", f"swap statements at token {before.a}", a | b,
                                   (replacement(src, before.a, after.b, swap),))
            if "temp" in enabled and before.kind == "simple" and discard_assignment(left) and pure_value(right):
                yield from self.inline_temp(before, left, right)

    def inline_temp(self, before, left, right):
        src = self.src
        dest, value = left.children
        dest = bare(dest)
        name = src.values[dest.a]
        if name not in self.fun.locals:
            return
        decl, index = self.fun.locals[name]
        value_context = right.children[1] if discard_assignment(right) else right
        sites = [e for e in expr_walk(value_context) if e.op == "id" and src.values[e.a] == name]
        if (len(sites) != 1 or decl.initializer or not self.fun.editable_name(name)
                or not src.uncommented(decl.stmt.a, decl.stmt.b)):
            return
        use, = sites
        if self.fun.uses[name] != {index, dest.a, use.a}:
            return
        # Preserve the old conversion performed by the temporary assignment.
        expr = f"(({decl.ctype.spelling})({src.raw(value.a, value.b)}))"
        yield Edit("temp", f"inline single-use {name}", identifiers(self.fun, value) | {name}, (
            replacement(src, decl.stmt.a, decl.stmt.b, declaration_without(self.fun, decl, index)),
            replacement(src, before.a, before.b, ""),
            replacement(src, use.a, use.b, expr),
        ))

    def stage_temp(self, node, expr):
        if self.fun.has_macros or not node.parent or node.parent.kind != "{" or not pure_value(expr):
            return
        src = self.src
        root = expr.children[1] if discard_assignment(expr) else expr
        name = next(f"permute_tmp{i}" for i in range(len(self.fun.uses) + len(src.macros) + 1)
                    if f"permute_tmp{i}" not in self.fun.uses and f"permute_tmp{i}" not in src.macros)
        for site in unconditional(root):
            if not site.children or site.op == "group" or not src.uncommented(site.a, site.b):
                continue
            # Insert at the start of the containing statement, before any
            # other evaluation. Every other evaluation here is pure as well.
            code = f"{name} = {src.raw(site.a, site.b)};{src.nl}{src.indent(node.a)}"
            start = src.tokens[node.a].start
            use = replacement(src, site.a, site.b, name)
            if start == use[0]:
                use = (*use[:2], code + name)
                assignment = ()
            else:
                assignment = ((start, start, code),)
            yield Edit("temp", f"stage expression at token {site.a}", identifiers(self.fun, site),
                       (declaration_insert(src, node.parent, f"{site.ctype.spelling} {name};"), use, *assignment))

    def expr_edits(self, enabled):
        src = self.src
        for node, part, expr in self.entries:
            if "commute" in enabled:
                for site in expr_walk(expr):
                    if site.op not in COMMUTATIVE | REVERSE.keys() or not pure(site) or not src.uncommented(site.a, site.b):
                        continue
                    left, right = site.children
                    op = REVERSE.get(site.op, site.op)
                    spelling = f"{operand(src, right, PRECEDENCE[op])} {op} {operand(src, left, PRECEDENCE[op] + 1)}"
                    yield Edit("commute", f"{site.op} operands at token {site.a}", identifiers(self.fun, site),
                               (replacement(src, site.a, site.b, spelling),))
            if "temp" in enabled and node.kind in ("return", "simple"):
                yield from self.stage_temp(node, expr)
            if used_as_statement(node, part) and src.uncommented(expr.a, expr.b):
                if "updates" in enabled:
                    yield from self.update_edits(expr)
                if "chain" in enabled and node.kind == "simple" and node.parent.kind == "{" and src.uncommented(node.a, node.b):
                    yield from self.chain_edits(node, expr)

    def update_edits(self, expr):
        src = self.src
        if expr.op in UPDATE:
            target, = expr.children
            op = expr.op[-2:]
        elif expr.op in ("+=", "-="):
            target, amount = expr.children
            if bare(amount).op != "num" or amount.bounds != (1, 1) or amount.ctype.unsigned:
                return
            op = expr.op[0] * 2
        else:
            return
        name = src.values[bare(target).a]
        for spelling in (f"{op}{name}", f"{name}{op}", f"{name} {op[0]}= 1"):
            yield Edit("updates", spelling, frozenset([name]), (replacement(src, expr.a, expr.b, spelling),))

    def chain_edits(self, node, expr):
        src = self.src
        names = []
        while expr.op == "=":
            lhs, expr = expr.children
            names.append(src.values[bare(lhs).a])
            expr = bare(expr)
        if len(names) < 2 or len(names) != len(set(names)) or not pure(expr):
            return
        # Each converted inner value becomes the RHS of the next assignment.
        value, lines = src.raw(expr.a, expr.b), []
        for name in reversed(names):
            lines.append(f"{name} = {value};")
            value = name
        sep = src.nl + src.indent(node.a)
        yield Edit("chain", "split " + " = ".join(names), frozenset(names) | identifiers(self.fun, expr),
                   (replacement(src, node.a, node.b, sep.join(lines)),))


def pure_value(expr):
    return pure(expr) or discard_assignment(expr)


def operand(src, expr, minimum):
    expr = bare(expr)
    spelling = src.raw(expr.a, expr.b)
    return f"({spelling})" if PRECEDENCE.get(expr.op, 99) < minimum else spelling


def unconditional(expr):
    yield expr
    for child in expr.children[:1] if expr.op in ("&&", "||", "conditional") else expr.children:
        yield from unconditional(child)


def edits(fun, enabled):
    from allocation import allocation_edits

    if "decl" in enabled:
        yield from declaration_edits(fun)
    if "loops" in enabled:
        yield from loop_edits(fun)
    if enabled & {"scope", "ranges", "address", "temp", "narrow", "save", "statements", "chain"}:
        yield from allocation_edits(fun, enabled)
    if not fun.opaque_macro_statement and enabled & {"types", "temp", "commute", "statements", "chain", "updates"}:
        effects = Effects(fun)
        if "types" in enabled:
            yield from effects.scoped_edits(enabled)
        if enabled & {"temp", "statements"}:
            yield from effects.neighbouring_edits(enabled)
        yield from effects.expr_edits(enabled)
