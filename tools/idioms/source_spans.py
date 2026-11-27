"""Lexical C statement candidates for line-only Watcom debug records."""
import bisect
import re

TOKEN = re.compile(r'\s+|/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|\w+|.', re.S)
DIRECTIVE = re.compile(r'^[ \t]*\#(?:[^\n]*?\\\n)*[^\n]*', re.M)
OPEN = {'(': ')', '[': ']', '{': '}'}


class Source:
    def __init__(self, text):
        self.text = text
        blanked = DIRECTIVE.sub(lambda m: re.sub(r'[^\n]', ' ', m[0]), text)
        self.lines = text.splitlines()
        self.breaks = [-1] + [m.start() for m in re.finditer('\n', text)]
        self.tokens = [m for m in TOKEN.finditer(blanked)
                       if not m[0].isspace() and not m[0].startswith(('/*', '//'))]
        self.vals = [m[0] for m in self.tokens]
        self.pairs, stack = {}, []
        for i, token in enumerate(self.vals):
            if token in OPEN:
                stack.append(i)
            elif stack and token == OPEN[self.vals[stack[-1]]]:
                self.pairs[stack.pop()] = i

    def line(self, position):
        return bisect.bisect_right(self.breaks, position - 1)

    def span(self, first, last, kind):
        start = self.tokens[first].start()
        end = self.tokens[last].end()
        line = self.line(start)
        return {'line': line, 'end_line': self.line(end - 1), 'column': start - self.breaks[line - 1],
                'kind': kind, 'text': self.text[start:end]}

    def function(self, name, anchor):
        candidates = []
        for i, t in enumerate(self.vals[:-2]):
            if t != name or self.vals[i+1] != '(' or i+1 not in self.pairs:
                continue
            body = self.pairs[i+1] + 1
            if body in self.pairs and self.vals[body] == '{':
                candidates.append((abs(self.line(self.tokens[body].start()) - anchor), i, body))
        if not candidates:
            return []
        _, name_at, body = min(candidates)
        # Include a multiline return type and name, bounded by the preceding declaration.
        sig = name_at
        while sig and self.vals[sig - 1] not in (';', '{', '}'):
            sig -= 1
        # A preprocessor branch may leave tokens from an alternate signature before the name.
        if name_at - sig > 20:
            sig = name_at
        return [self.span(sig, body - 1, 'signature'), *self.statements(body, self.pairs[body] + 1)]

    def initializer(self, name):
        for i, token in enumerate(self.vals):
            j = self.pairs.get(i + 1)
            if token != name or self.vals[i+1:i+2] != ['['] or j is None:
                continue
            while self.vals[j + 1] == '[':
                j = self.pairs[j + 1]
            if self.vals[j+1:j+3] != ['=', '{'] or j+2 not in self.pairs:
                continue
            start = i
            while start and self.vals[start - 1] not in (';', '{', '}'):
                start -= 1
            return [self.span(start, self.pairs[j + 2] + 1, 'initializer')]
        return []

    def statements(self, i, end):
        while i < end:
            t = self.vals[i]
            j = i
            kind = 'statement'
            if t in ('{', '}'):
                kind = 'brace'
            elif t in ('if', 'for', 'while', 'switch') and i+1 in self.pairs:
                j = self.pairs[i+1]
                kind = t
            elif t in ('else', 'do'):
                kind = t
            elif t in ('case', 'default') or self.vals[i+1:i+2] == [':']:
                kind = 'label'
                ternary = 0
                while j + 1 < end:
                    j += 1
                    v = self.vals[j]
                    if v == ':' and not ternary:
                        break
                    if v == '?':
                        ternary += 1
                    elif v == ':':
                        ternary -= 1
                    j = self.pairs.get(j, j)
            else:
                while j + 1 < end and self.vals[j] not in (';', '}'):
                    j = self.pairs.get(j, j) + 1
                if j > i and self.vals[j] == '}':
                    j -= 1
            yield self.span(i, j, kind)
            i = j + 1
