"""Apply the workspace's reviewed game-literal names to a directory of C sources."""
import argparse
from collections import Counter, defaultdict
import json
from pathlib import Path
import re

CATALOG = Path(__file__).with_name("names.json")
HEADER = "records.h"
BEGIN = "/* Named game constants (build/workers/names/names.json). */"
END = "/* End named game constants. */"
TOKENS = re.compile(
    r'/\*[\s\S]*?\*/|//[^\r\n]*|"(?:\\[\s\S]|[^"\\])*"|\'(?:\\[\s\S]|[^\'\\])*\'|'
    r'[A-Za-z_]\w*|0[xX][\da-fA-F]+(?:[uUlL]+)?|\d+(?:[uUlL]+)?|\+\+|--|==|!=|<=|>=|<<|>>|'
    r'\+=|-=|&=|\|=|\^=|->|&&|\|\||[^\s]',
    re.ASCII,
)
NUMBER = re.compile(r'(?:0[xX][\da-fA-F]+|[0-9]+)$')
DEFINITION = re.compile(r'^[ \t]*#[ \t]*define[ \t]+(\w+)([^\r\n]*)', re.M)
LAST_ENDIF = re.compile(r'^[ \t]*#[ \t]*endif\b.*$', re.M)
RADIUS = 8
UNARY_AFTER = {"case", "=", "==", "!=", "<", ">", "<=", ">=", "(", "[", "{", ",", ":", "?", "return"}

def tokens(source):
    previous = None
    last = None
    for hit in TOKENS.finditer(source):
        word = hit.group()
        if word.startswith(("/*", "//")):
            continue
        if word[0] in "\"'":
            word = "<quoted>"
        if previous is not None:
            # Treat a unary signed decimal as one target. An arithmetic minus
            # remains a token of its own.
            old, lo, hi = previous
            if (old == "-" and last in UNARY_AFTER and NUMBER.fullmatch(word)
                    and not source[hi:hit.start()].strip()):
                previous = ("-" + word, lo, hit.end())
                continue
            yield previous
            last = old
        previous = word, hit.start(), hit.end()
    if previous is not None:
        yield previous

def normalize(word, values):
    if word in values:
        return str(values[word])
    digits = word.lstrip("-")
    if NUMBER.fullmatch(digits):
        return str(int(word, 16 if "x" in digits.lower() else 10))
    return word

def anchor(words, index, radius=RADIUS):
    return words[max(0, index - radius):index] + ["<target>"] + words[index+1:index+radius+1]

def definitions(source):
    result = defaultdict(list)
    for define in DEFINITION.finditer(source):
        replacement = define[2].strip()
        replacement = re.split(r'/\*|//', replacement, maxsplit=1)[0].strip()
        result[define[1]].append(replacement)
    return result

def insert_block(source, macros):
    if not macros:
        return source
    guards = list(LAST_ENDIF.finditer(source))
    if not guards:
        raise ValueError("records.h needs a closing #endif")
    newline = "\r\n" if "\r\n" in source else "\n"
    lines = [BEGIN]
    for name, spec in sorted(macros.items(), key=lambda pair: (pair[1]["kind"], pair[1]["value"], pair[0])):
        comment = spec.get("comment")
        tail = "  /* " + comment + " */" if comment else ""
        lines.append("#define " + name + " " + spec["definition"] + tail)
    lines += [END, "", ""]
    index = guards[-1].start()
    return source[:index] + newline.join(lines) + source[index:]

def run(directory, dry_run):
    catalogue = json.loads(CATALOG.read_text(encoding="utf8"))
    records = catalogue["records"]
    pending = [(entry, site) for entry in records for site in entry["locations"] if site["action"] == "replace"]
    replacements = defaultdict(list)
    for entry, site in pending:
        replacements[Path(site["path"]).name].append((entry, site))
    paths = sorted(p for p in directory.iterdir() if p.is_file() and p.suffix.lower() in {".c", ".h"})
    originals = {p.name: p.read_bytes().decode("latin1") for p in paths}
    if HEADER not in originals:
        raise ValueError("missing " + str(directory / HEADER))
    all_defs = {p: definitions(s) for p, s in originals.items()}
    applicable = {name: spec for spec in records if (name := spec["proposed_name"])}
    values = {name: spec["value"] for name, spec in applicable.items()}
    for name, v in catalogue["existing_values"].items():
        values.setdefault(name, v)
    identifiers = {word for source in originals.values() for word, _, _ in tokens(source) if word in applicable}
    planned = {}
    macros = {}
    changed = Counter()
    placed = Counter()
    missing = []
    for file, sites in sorted(replacements.items()):
        if file not in originals:
            missing.extend((site, entry, "source file missing") for entry, site in sites)
            continue
        source = originals[file]
        source_tokens = list(tokens(source))
        words = [normalize(token, values) for token, _, _ in source_tokens]
        by_key = defaultdict(list)
        for index, word in enumerate(words):
            if NUMBER.fullmatch(word.lstrip("-")):
                by_key[int(word), tuple(anchor(words, index))].append(index)

        by_site = defaultdict(list)
        for spec, site in sites:
            by_site[spec["value"], tuple(site["anchor"])].append((spec, site))
        edits = []
        for key, targets in by_site.items():
            candidates = by_key[key]
            if len(candidates) != len(targets):
                reason = f"context occurrences {len(candidates)}, expected {len(targets)}"
                missing.extend((site, spec, reason) for spec, site in targets)
                continue
            for index, (spec, site) in zip(candidates, targets):
                name = spec["proposed_name"]
                original, lo, hi = source_tokens[index]
                definitions_here = all_defs[file].get(name, [])
                definitions_shared = all_defs[HEADER].get(name, [])
                definitions_other = [body for f, defs in all_defs.items() if f not in (file, HEADER)
                                     for body in defs.get(name, [])]
                bodies = definitions_here + definitions_shared + definitions_other
                if any(body != spec["definition"] for body in bodies):
                    missing.append((site, spec, "conflicting #define"))
                    continue
                if not bodies and name in identifiers:
                    missing.append((site, spec, "identifier already present without a recognized #define"))
                    continue
                if original != name and normalize(original, {}) != str(spec["value"]):
                    missing.append((site, spec, "site already uses another identifier"))
                    continue
                if not definitions_here and not definitions_shared:
                    macros[name] = spec
                placed[file] += 1
                if original != name:
                    edits.append((lo, hi, name))
                    changed[file] += 1
        for lo, hi, text in sorted(edits, reverse=True):
            source = source[:lo] + text + source[hi:]
        planned[file] = source
    planned[HEADER] = insert_block(originals[HEADER], macros)
    for file, source in planned.items():
        if source != originals[file] and not dry_run:
            path = directory / file
            temporary = path.with_name(path.name + ".names-tmp")
            temporary.write_bytes(source.encode("latin1"))
            temporary.replace(path)
    for file in sorted(replacements):
        print(f"{file}: {changed[file]} replaced, {placed[file] - changed[file]} already named")
    print(f"{HEADER}: {len(macros)} definitions added")
    for site, spec, reason in missing:
        print(f"Skipped {site['path']}:{site['line']} ({site['function']}, {site['context']}), "
              f"{spec['kind']} {spec['value']} -> {spec['proposed_name']}: {reason}")
    print(f"{sum(changed.values())} replaced, {sum(placed.values()) - sum(changed.values())} already named, "
          f"{len(missing)} unplaced")
    return int(bool(missing))

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    try:
        raise SystemExit(run(args.directory, args.dry_run))
    except (OSError, ValueError) as error:
        parser.exit(2, str(error) + "\n")
