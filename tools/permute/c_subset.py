"""Source spans and effect checks for the Watcom C subset used by permute.

Preprocessor tokens are never expanded into editable text. Statements with
unmodelled expressions remain structural nodes, with no effect information.
"""
import re
from bisect import bisect_right
from dataclasses import dataclass, field


class Unsupported(ValueError):
    pass


IDENT = re.compile(r"[A-Za-z_][A-Za-z_0-9]*\Z")
LEX = re.compile(
    r"(?P<directive>^[ \t]*\#(?:\\\r?\n|[^\\\r\n]|\\[^\r\n])*)"
    r"|(?P<comment>/\*(?:[^*]|\*(?!/))*\*/|//(?:\\\r?\n|[^\\\r\n]|\\[^\r\n])*)"
    r"""|(?P<literal>L?"(?:[^"\\\r\n]|\\(?:\r?\n|[^\r\n]))*"|L?'(?:[^'\\\r\n]|\\(?:\r?\n|[^\r\n]))*')"""
    r"|(?P<word>[A-Za-z_][A-Za-z_0-9]*)"
    r"|(?P<number>(?:[0-9]|\.[0-9])(?:[eEpP][+-]|[A-Za-z_0-9.])*)"
    r"|(?P<space>\r?\n|[^\S\n]+)"
    r"|(?P<op>>>=|<<=|->|\+\+|--|&&|\|\||[+*/%&|^=!<>-]=|<<|>>|\.\.\.|[{}()\[\];?:,.~!+*/%&|^=<>-])"
    r"|(?P<invalid>.)",
    re.M,
)
OPEN = {"(": ")", "[": "]", "{": "}"}
CLOSE = set(OPEN.values())


@dataclass(frozen=True)
class Token:
    value: str
    start: int
    end: int
    kind: str


def lex(text):
    if re.search(r"\?\?[=/\x27()!<>-]", text):
        raise Unsupported("trigraphs require preprocessing")
    tokens, comments = [], []
    for m in LEX.finditer(text):
        t = Token(m[0], m.start(), m.end(), m.lastgroup)
        if t.kind == "invalid":
            raise Unsupported(f"unrecognised source token {t.value!r} at offset {t.start}")
        if t.kind == "comment":
            comments.append(t)
        elif t.kind != "space":
            tokens.append(t)
    return tokens, comments


@dataclass(eq=False)
class Stmt:
    kind: str
    a: int
    b: int
    parts: tuple = ()
    children: list = field(default_factory=list)
    parent: "Stmt | None" = None
    decl: "Declaration | None" = None


@dataclass(frozen=True)
class CType:
    rank: str
    unsigned: bool = False

    @property
    def bounds(self):
        bits = {"char": 8, "short": 16, "int": 32, "long": 32}[self.rank]
        return (0, (1 << bits) - 1) if self.unsigned else (-(1 << (bits - 1)), (1 << (bits - 1)) - 1)

    @property
    def spelling(self):
        return ("unsigned " if self.unsigned else "signed " if self.rank == "char" else "") + self.rank

    @property
    def promoted(self):
        return INT if self.rank in ("short", "char") else self


INT = CType("int")


def scalar_type(words):
    words = list(words)
    if words.count("int") == 1 and len(words) > 1:
        words.remove("int")
    if len(words) != len(set(words)):
        return None
    if "char" in words and not ({"signed", "unsigned"} & set(words)):
        return None  # plain char depends on flags
    unsigned = "unsigned" in words
    if unsigned and "signed" in words:
        return None
    ranks = [w for w in words if w not in ("signed", "unsigned")]
    rank = ranks[0] if ranks else "int"
    if words and len(ranks) <= 1 and rank in ("char", "short", "int", "long"):
        return CType(rank, unsigned)
    return None


SPEC = {"register", "signed", "unsigned", "char", "short", "int", "long"}
KEYWORDS = SPEC | {
    "auto", "break", "case", "const", "continue", "default", "do", "double", "else", "enum",
    "extern", "float", "for", "goto", "if", "return", "sizeof", "static", "struct", "switch",
    "typedef", "union", "void", "volatile", "while",
}


@dataclass(eq=False)
class Declaration:
    spec: str
    names: tuple
    ctype: CType | None
    stmt: Stmt


class Source:
    def __init__(self, text, macros):
        self.text = text
        self.tokens, self.comments = lex(text)
        self.comment_ends = [t.end for t in self.comments]
        self.values = [t.value for t in self.tokens]
        self.macros = macros
        self.nl = "\r\n" if "\r\n" in text else "\n"
        self.pairs = {}
        stack = []
        for i, v in enumerate(self.values):
            if v in OPEN:
                stack.append(i)
            elif v in CLOSE:
                if not stack or OPEN[self.values[stack[-1]]] != v:
                    raise Unsupported(f"unbalanced {v!r} at offset {self.tokens[i].start}")
                self.pairs[stack.pop()] = i
        if stack:
            raise Unsupported("unclosed delimiter")

    def span(self, a, b):
        if a == b:
            return self.tokens[a].start, self.tokens[a].start
        return self.tokens[a].start, self.tokens[b - 1].end

    def raw(self, a, b):
        if a == b:
            return ""
        start, end = self.span(a, b)
        return self.text[start:end]

    def uncommented(self, a, b):
        start, end = self.span(a, b)
        i = bisect_right(self.comment_ends, start)
        return i == len(self.comments) or self.comments[i].start >= end

    def indent(self, i):
        start = self.tokens[i].start
        line = self.text.rfind("\n", 0, start) + 1
        return re.match(r"[ \t]*", self.text[line:start])[0]

    def next_top(self, a, b):
        while a < b:
            yield a
            a = self.pairs.get(a, a) + 1

    def function(self, name):
        if name in self.macros:
            raise Unsupported(f"{name} is also a macro")
        v = self.values
        signatures = []
        start = 0
        for i in self.next_top(0, len(v)):
            if v[i] == "(" and i and v[i - 1] == name:
                close = self.pairs[i]
                if v[close + 1:close + 2] == ["{"] and "=" not in v[start:i]:
                    signatures.append((i, close + 1))
            if v[i] in (";", "{") or self.tokens[i].kind == "directive":
                start = self.pairs.get(i, i) + 1
        if len(signatures) != 1:
            raise Unsupported(f"{name}: expected one direct function definition, found {len(signatures)}")
        params, a = signatures[0]
        b = self.pairs[a] + 1
        for t in self.tokens[a:b]:
            if t.kind == "directive" or t.value in ("asm", "_asm", "__asm", "__asm__", "_emit", "__emit"):
                raise Unsupported(f"{name}: preprocessor directive or inline assembly in body")
        return Function(self, name, params, a, b)


def walk(stmt):
    yield stmt
    for child in stmt.children:
        yield from walk(child)


class Function:
    def __init__(self, src, name, params, a, b):
        self.src, self.name, self.a, self.b = src, name, a, b
        self.body = self.statement(a, b)
        self.nodes = list(walk(self.body))
        self.uses = {}
        for i in range(a + 1, b - 1):
            if src.tokens[i].kind == "word":
                self.uses.setdefault(src.values[i], set()).add(i)
        declarations = {}
        for node in self.nodes:
            if node.decl:
                for index in node.decl.names:
                    declarations.setdefault(src.values[index], []).append((node.decl, index))
        # For effect analysis use only unique bare scalar declarations. Treat
        # potential shadowing in opaque declarations as unknown.
        params_set = set(src.values[params + 1:a])
        unknown = set()
        for node in self.nodes:
            if node.kind == "opaque":
                unknown.update(src.values[node.a:node.b])
            if node.kind == "for":
                lo, hi = node.parts[0]
                # A declaration in a C99-style header can shadow an outer
                # local. Leave those names out of the expression model.
                if self.possible_declaration(lo, hi):
                    unknown.update(src.values[lo:hi])
        self.locals = {
            n: refs[0] for n, refs in declarations.items()
            if len(refs) == 1 and n not in params_set and n not in unknown and refs[0][0].ctype
        }
        self.has_macros = not src.macros.isdisjoint(self.uses)
        self.opaque_macro_statement = any(src.values[n.a] in src.macros for n in self.nodes)

    def possible_declaration(self, a, b):
        if b - a < 2:
            return False
        first, after = self.src.values[a:a + 2]
        return first in KEYWORDS or IDENT.fullmatch(first) and (after in ("(", "*") or IDENT.fullmatch(after))

    def declaration(self, node):
        src = self.src
        v = src.values
        i = node.a
        while i < node.b - 1 and v[i] in SPEC:
            i += 1
        if i == node.a or i >= node.b - 1:
            return None
        names = tuple(range(i, node.b - 1, 2))
        if any(not IDENT.fullmatch(v[n]) or v[n] in KEYWORDS or v[n] in src.macros for n in names):
            return None
        if len(names) * 2 - 1 != node.b - 1 - i or any(v[n] != "," for n in range(i + 1, node.b - 1, 2)):
            return None
        # Only ordinary, uninitialised, non-pointer, non-array declarators.
        spec = v[node.a:i]
        if any(s in src.macros for s in spec):
            return None
        typ = scalar_type([s for s in spec if s != "register"])
        if typ is None and sorted(s for s in spec if s != "register") != ["char"]:
            return None
        return Declaration(" ".join(spec), names, typ, node)

    def statement(self, a, limit, parent=None):
        src = self.src
        v = src.values
        if a >= limit:
            raise Unsupported("missing statement")
        node = Stmt(v[a], a, a + 1, parent=parent)
        if v[a] == "{":
            end = src.pairs[a]
            i = a + 1
            while i < end:
                child = self.statement(i, end, node)
                node.children.append(child)
                i = child.b
            node.b = end + 1
        elif v[a] in ("if", "while", "for", "switch"):
            if v[a + 1] != "(":
                raise Unsupported(f"expected a parenthesised {v[a]} header")
            close = src.pairs[a + 1]
            if v[a] == "for":
                cuts = [i for i in src.next_top(a + 2, close) if v[i] == ";"]
                if len(cuts) != 2:
                    raise Unsupported("malformed for header")
                left, right = cuts
                node.parts = ((a + 2, left), (left + 1, right), (right + 1, close))
            else:
                node.parts = ((a + 2, close),)
            child = self.statement(close + 1, limit, node)
            node.children.append(child)
            node.b = child.b
            if v[a] == "if" and node.b < limit and v[node.b] == "else":
                child = self.statement(node.b + 1, limit, node)
                node.children.append(child)
                node.b = child.b
        elif v[a] == "do":
            child = self.statement(a + 1, limit, node)
            tail = child.b
            if v[tail:tail + 2] != ["while", "("]:
                raise Unsupported("malformed do/while tail")
            close = src.pairs[tail + 1]
            if v[close + 1] != ";":
                raise Unsupported("expected semicolon after do/while")
            node.children.append(child)
            node.parts = ((tail + 2, close),)
            node.b = close + 2
        elif v[a] in ("case", "default") or v[a + 1] == ":":
            node.kind = "label"
            depth = 0
            for i in src.next_top(a + 1, limit):
                if v[i] == ":" and depth == 0:
                    break
                depth += (v[i] == "?") - (v[i] == ":")
                if v[i] in (";", "{"):
                    raise Unsupported("malformed switch label")
            else:
                raise Unsupported("missing label colon")
            child = self.statement(i + 1, limit, node)
            node.children.append(child)
            node.b = child.b
        else:
            for i in src.next_top(a, limit):
                if v[i] == ";":
                    break
            else:
                raise Unsupported(f"missing semicolon after {v[a]!r}")
            node.b = i + 1
            if v[a] in ("return", "goto", "break", "continue"):
                if v[a] == "return":
                    node.parts = ((a + 1, i),)
            elif parent and parent.kind == "{":
                node.decl = self.declaration(node)
                node.kind = "decl" if node.decl else "simple"
                if node.kind == "simple" and self.possible_declaration(a, i):
                    node.kind = "opaque"
                if node.kind == "simple":
                    node.parts = ((a, i),)
            else:
                node.kind = "simple"
                node.parts = ((a, i),)
        if node.b > limit:
            raise Unsupported("statement crosses block boundary")
        return node

    def local_type(self, i):
        node = self.locals.get(self.src.values[i])
        if node is None:
            return None
        decl, index = node
        block = decl.stmt.parent
        if index < i < block.b:
            return decl.ctype
        return None


@dataclass(frozen=True)
class Expr:
    op: str
    a: int
    b: int
    children: tuple = ()
    ctype: CType | None = None
    bounds: tuple | None = None


ASSIGN = {"=", "+=", "-=", "*=", "/=", "%=", "<<=", ">>=", "&=", "^=", "|="}
PRECEDENCE = {
    **{op: 1 for op in ASSIGN},
    "||": 2, "&&": 3, "|": 4, "^": 5, "&": 6, "==": 7, "!=": 7,
    "<": 8, "<=": 8, ">": 8, ">=": 8, "<<": 9, ">>": 9, "+": 10, "-": 10,
    "*": 11, "/": 11, "%": 11,
}
COMPARE = {"<", "<=", ">", ">=", "==", "!="}
UPDATE = {"pre++", "pre--", "post++", "post--"}


def fits(bounds, typ):
    lo, hi = typ.bounds
    return lo <= bounds[0] <= bounds[1] <= hi


def usual(a, b):
    a, b = a.promoted, b.promoted
    return CType("long" if "long" in (a.rank, b.rank) else "int", a.unsigned or b.unsigned)


def integer(text):
    m = re.fullmatch(r"(0[xX][0-9a-fA-F]+|0[0-7]*|[1-9][0-9]*)([uU][lL]?|[lL][uU]?|)", text)
    if not m:
        return None
    num, suffix = m.groups()
    base = 16 if num.lower().startswith("0x") else 8 if num.startswith("0") else 10
    value = int(num, base)
    ranks = ["long"] if "l" in suffix.lower() else ["int", "long"]
    types = []
    for rank in ranks:
        if "u" not in suffix.lower():
            types.append(CType(rank))
        if base != 10 or rank == "long" or "u" in suffix.lower():
            types.append(CType(rank, True))
    return next(((typ, (value, value)) for typ in types if fits((value, value), typ)), None)


def converted(bounds, typ):
    if fits(bounds, typ):
        return bounds
    if typ.unsigned and bounds[0] == bounds[1]:
        value = bounds[0] & typ.bounds[1]
        return value, value
    return typ.bounds


def binary(op, left, right):
    if op in COMPARE | {"&&", "||"}:
        return INT, (0, 1)
    typ = left.ctype.promoted if op in ("<<", ">>") else usual(left.ctype, right.ctype)
    lo, hi = converted(left.bounds, typ)
    low, high = converted(right.bounds, typ)
    estimate = typ.bounds
    if op == "+":
        estimate = lo + low, hi + high
    elif op == "-":
        estimate = lo - high, hi - low
    elif op == "*":
        corners = [a * b for a in (lo, hi) for b in (low, high)]
        estimate = min(corners), max(corners)
    elif op == "&":
        masks = [b for a, b in ((lo, hi), (low, high)) if a == b and a >= 0]
        if masks:
            estimate = 0, min(masks)
    return typ, converted(estimate, typ)


def bare(expr):
    while expr.op == "group":
        expr = expr.children[0]
    return expr


def expr_walk(expr):
    yield expr
    for child in expr.children:
        yield from expr_walk(child)


def pure(expr):
    return all(e.op not in ASSIGN | UPDATE for e in expr_walk(expr))


class Expressions:
    def __init__(self, fun):
        self.fun = fun
        self.v = fun.src.values

    def parse(self, span):
        self.i, self.end = span
        if self.i == self.end:
            return None
        try:
            expr = self.read(1)
            return expr if self.i == self.end else None
        except Unsupported:
            return None

    def read(self, minimum):
        left = self.prefix()
        while self.i < self.end:
            op = self.v[self.i]
            prec = PRECEDENCE.get(op, 0)
            if prec < minimum:
                break
            self.i += 1
            right = self.read(prec if op in ASSIGN else prec + 1)
            if op in ASSIGN:
                if bare(left).op != "id":
                    raise Unsupported("unmodelled lvalue")
                typ, bounds = left.ctype, left.ctype.bounds
            else:
                typ, bounds = binary(op, left, right)
            left = Expr(op, left.a, right.b, (left, right), typ, bounds)
        return left

    def prefix(self):
        if self.i >= self.end:
            raise Unsupported("missing operand")
        a = self.i
        token = self.v[a]
        self.i += 1
        if token == "(":
            close = self.fun.src.pairs[a]
            words = self.v[a + 1:close]
            cast = scalar_type(words) if not self.fun.src.macros.intersection(words) else None
            if cast is not None:
                self.i = close + 1
                child = self.prefix()
                expr = Expr("cast", a, child.b, (child,), cast, converted(child.bounds, cast))
            else:
                child = self.read(1)
                if self.i != close or self.i >= self.end:
                    raise Unsupported("unmodelled parenthesised expression")
                self.i += 1
                expr = Expr("group", a, self.i, (child,), child.ctype, child.bounds)
        elif token in ("!", "~", "+", "-", "++", "--"):
            child = self.prefix()
            op = "pre" + token
            typ = INT if token == "!" else child.ctype.promoted
            bounds = typ.bounds
            if token in ("++", "--"):
                if bare(child).op != "id":
                    raise Unsupported("unmodelled update target")
                typ, bounds = child.ctype, child.ctype.bounds
            elif token == "!":
                bounds = (0, 1)
            elif token == "+":
                bounds = child.bounds
            elif token == "-":
                bounds = converted((-child.bounds[1], -child.bounds[0]), typ)
            return Expr(op, a, child.b, (child,), typ, bounds)
        else:
            constant = integer(token)
            typ = self.fun.local_type(a)
            if token in self.fun.src.macros or (not constant and not typ):
                raise Unsupported(f"unmodelled operand {token!r}")
            expr = Expr("num", a, self.i, (), *constant) if constant else Expr("id", a, self.i, (), typ, typ.bounds)
        if self.i < self.end and self.v[self.i] in ("++", "--"):
            if bare(expr).op != "id":
                raise Unsupported("unmodelled update target")
            op = "post" + self.v[self.i]
            self.i += 1
            return Expr(op, a, self.i, (expr,), expr.ctype, expr.bounds)
        return expr
