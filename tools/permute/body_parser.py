"""Pycparser supplies body syntax and declaration identity; edits use original spans.

The parse-only mask never expands macros. Unknown or statement-producing macros
are opaque to mutations. The retained v2 model proves scalar types and effects.
"""
from collections import Counter
import re

from pycparser import c_ast as C, c_parser

from c_subset import Source as TokenSource, Unsupported

QUALIFIERS = re.compile(r"_{1,2}(?:far|near|huge|interrupt|cdecl|pascal|stdcall|fastcall|loadds|saveregs|export)_{0,2}$")
ANNOTATIONS = {"__declspec", "__based", "_declspec", "_based"}


def blank(text):
    return re.sub(r"[^\r\n]", " ", text)


def ast_walk(node):
    yield node
    for _, child in node.children():
        yield from ast_walk(child)


def typedef_names(paths):
    """Get declarator names from simple and compound typedefs in the include closure."""
    result = set()
    for path in sorted(paths):
        raw = path.read_bytes().decode("latin1")
        try:
            src = TokenSource(raw, ())
        except Unsupported:
            continue
        for i in src.next_top(0, len(src.values)):
            if src.values[i] != "typedef":
                continue
            words = []
            for j in src.next_top(i + 1, len(src.values)):
                value = src.values[j]
                if value in (";", ","):
                    if words:
                        result.add(words[-1])
                    words.clear()
                    if value == ";":
                        break
                elif src.tokens[j].kind == "word":
                    words.append(value)
                elif value == "(" and src.values[j + 1:j + 2] == ["*"]:
                    for k in range(j + 2, src.pairs[j]):
                        if src.tokens[k].kind == "word" and not QUALIFIERS.fullmatch(src.values[k]):
                            words.append(src.values[k])
    return result


class Source(TokenSource):
    body_directives = True

    def __init__(self, *args, typedefs=(), **kwargs):
        super().__init__(*args, **kwargs)
        self.typedefs = set(typedefs)
        self.functions = {}

    def function(self, name):
        if name not in self.functions:
            self.functions[name] = self.parse_function(super().function(name))
        return self.functions[name]

    def parse_function(self, fun):
        a, b = self.span(fun.a, fun.b)
        masked = list(self.text[a:b])
        protected = []

        def mask(start, end, spelling=None):
            raw = self.text[start:end]
            replacement = blank(raw)
            if spelling is not None:
                replacement = spelling + replacement[len(spelling):]
            masked[start-a:end-a] = replacement
            protected.append((start, end))

        for t in self.comments:
            if a <= t.start < b:
                mask(t.start, t.end)
        standalone = {n.a for n in fun.nodes if n.kind in ("simple", "opaque")}
        for i in range(fun.a + 1, fun.b - 1):
            t = self.tokens[i]
            if any(lo <= t.start < hi for lo, hi in protected):
                continue
            if t.kind == "directive" or QUALIFIERS.fullmatch(t.value):
                mask(t.start, t.end)
            elif t.value in ANNOTATIONS and self.values[i+1] == "(":
                mask(t.start, self.tokens[self.pairs[i+1]].end)
            elif i in standalone and "<statement tokens>" in self.macro_expansion.get(t.value, ()):
                # Macro arguments may contain type names (e.g. LL_AddToTail).
                # Give the parser a null statement without moving the source.
                end = self.pairs[i+1] if self.values[i+1] == "(" else i
                mask(t.start, self.tokens[end].end, None if self.values[end+1] == ";" else ";")
            elif t.value in self.macros and self.values[i+1] == "(":
                close = self.pairs[i+1]
                if close > i+2 and self.typedefs.intersection(self.values[i+2:close]):
                    # Type-valued macro arguments are preprocessor syntax.
                    # Keep the invocation opaque and preserve its raw span.
                    mask(self.tokens[i+2].start, self.tokens[close-1].end, "0")
            elif t.kind == "number" and re.search(r"(?i)(?:ui64|i64)$", t.value):
                mask(t.start, t.end, "0")

        prefix = "".join(f"typedef int {n};\n" for n in sorted(self.typedefs - set(fun.parameters)))
        sig = "void permute_body(void)\n"
        body = "".join(masked).replace("\r", " ")
        try:
            tree = c_parser.CParser().parse(prefix + sig + body)
        except c_parser.ParseError as exc:
            raise Unsupported(f"{fun.name}: pycparser could not parse body: {exc}") from exc
        fun.ast = tree.ext[-1].body
        line_start = [a, *(m.end() + a for m in re.finditer("\n", body))]
        first_line = (prefix + sig).count("\n") + 1
        starts = {tok.start: i for i, tok in enumerate(self.tokens[fun.a:fun.b], fun.a)}
        fun.ast_tokens = {id(n): starts.get(line_start[n.coord.line-first_line] + n.coord.column - 1)
                          for n in ast_walk(fun.ast) if n.coord and n.coord.line >= first_line
                          and n.coord.column is not None}
        fun.protected_spans = protected

        # V2 mistook bare calls and x * y statements for declarations. Clear
        # that ambiguity only where the AST identifies the same source token.
        declarations = Counter(n.name for n in ast_walk(fun.ast)
                               if isinstance(n, (C.Decl, C.Typedef)) and n.name)
        statements = {fun.ast_tokens.get(id(n)) for n in ast_walk(fun.ast)
                      if isinstance(n, (C.FuncCall, C.Assignment, C.BinaryOp, C.UnaryOp))}
        for node in fun.nodes:
            if node.kind == "opaque" and node.a in statements:
                node.kind = "simple"
                node.parts = ((node.a, node.b - 1),)
        for name, decls in _scalar_declarations(fun).items():
            if len(decls) == 1 and declarations[name] == 1 and name not in fun.parameters:
                fun.locals[name] = decls[0]
        fun.shadowed_parameters.update(declarations)
        return fun


def _scalar_declarations(fun):
    result = {}
    for node in fun.nodes:
        if node.decl and node.decl.ctype:
            for i in node.decl.names:
                result.setdefault(fun.src.values[i], []).append((node.decl, i))
    return result


def within_body(fun, edit):
    """Reject nonlocal, overlapping, or parser-masked edits."""
    lo = fun.src.tokens[fun.a].end
    hi = fun.src.tokens[fun.b - 1].start
    end = lo
    for a, b, _ in sorted(edit.changes):
        if not end <= a <= b <= hi:
            return False
        if any(a < right and left < b for left, right in fun.protected_spans):
            return False
        end = b
    return True
