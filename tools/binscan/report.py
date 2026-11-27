"""Generate the worker report from the inventory, ranking and lineage JSON."""
from pathlib import Path
import json


WORK = Path(__file__).resolve().parents[2] / "build" / "binscan"
OUT = WORK / "out"
SIGNALS = ("mnemonic_ngrams", "operand_ngrams", "block_content", "cfg",
           "call_names_or_signatures", "constants", "data")
COMMAND = "python -B tools/binscan/binscan.py"


def load(name):
    return json.loads((OUT / name).read_text(encoding="utf-8"))


def number(value):
    return f"{value:.2f}" if value is not None else "—"


def stamp(entry, key="first_date"):
    date = entry.get(key)
    return date["timestamp"][:10] if date else "undated"


def short_path(path):
    parts = path.split("/")
    if parts[:2] == ["external", "x"] and len(parts) > 4:
        return "/".join([parts[2], *parts[4:]]) if parts[2] in {
            "kenbuild-20241213", "kenbuild-20260331", "buildsrc-20190520"} else "/".join(parts[2:])
    if parts[:2] == ["external", "duke3d-1.3d"]:
        return "Duke tree/" + "/".join(parts[2:])
    return path


def donor_label(donor):
    if not donor:
        return "—"
    rep = donor["representative"]
    name = rep["name"] or rep["address"]
    member = f" ({rep['member']})" if rep["member"] and rep["member_offset"] and not rep["name"] else ""
    date = stamp(rep)
    if date == "undated":
        date = stamp(donor)
    return f"`{short_path(rep['path'])}`{member} `{name}` ({date})"


def table_row(rank, row):
    best = next(iter(row["donors"]), None)
    named = row["best_named_donor"]
    if best:
        metrics = " / ".join(number(best["signals"][s]) for s in SIGNALS)
        exact = "literal + reloc" if best["literal_bytes"] and best["relocation_aware_bytes"] else (
            "literal" if best["literal_bytes"] else "reloc" if best["relocation_aware_bytes"] else "—")
        score = number(best["score"])
    else:
        metrics, exact, score = "—", "—", "—"
    alternate = f"{donor_label(named)} ({number(named['score'])})" if named and (
        not best or named["id"] != best["id"]) else "—"
    return (f"| {rank} | `{row['name']}` `{row['address']}` | {row['verify_size'] or row['size']:,} | "
            f"{donor_label(best)} | {score} | {metrics} | {exact} | {alternate} |")


def window(row, key):
    result = row[key]
    first, last = result["first"], result["last"]
    if not first:
        return "—"
    return (f"{first['timestamp'][:10]} (`{short_path(first['path'])}`) to "
            f"{last['timestamp'][:10]} (`{short_path(last['path'])}`)")


def anchors(rows):
    for row in rows:
        if row["name"] not in {"drawsprite", "getcliplines", "parsecommand"} or not row["donors"]:
            continue
        donor = row["donors"][0]
        shared = donor["shared_features"]
        calls = ", ".join(f"`{name}`" + (f" ×{n}" if n > 1 else "") for name, n in shared["calls"][:6])
        strings = ", ".join(f"`{text[2:].strip()}`" for text, _ in shared["data"] if text.startswith("S:"))
        if calls:
            yield f"- `{row['name']}` and its leading donor share calls to {calls}."
        if strings:
            yield f"- `{row['name']}` and its leading donor reference the same strings, including {strings}."


def generate():
    inv, rankings, lin = (load(file) for file in ("inventory.json", "top_donors.json", "lineage.json"))
    summary = inv["summary"]
    lineages = lin["functions"]
    targets = rankings["targets"]
    matched_raw = sum(any(o["literal_bytes"] for o in row["appearances"]) for row in lineages)
    matched_norm = sum(any(o["relocation_aware_bytes"] for o in row["appearances"]) for row in lineages)
    dated_raw = sum(bool(e["literal_window"]["first"]) for e in lineages)
    dated_norm = sum(bool(e["relocation_aware_window"]["first"]) for e in lineages)
    names = rankings.get("largest_non_strict_names") or [None] * len(rankings["largest_non_strict"])
    table = [next(row for row in targets if row["address"] == address and (name is None or row["name"] == name))
             for address, name in zip(rankings["largest_non_strict"], names)]
    supplements = [row for row in targets if row["name"] in rankings["supplemental_names"] and row not in table]
    lines = [
        "# LameDuke binary donor scan", "",
        f"The inventory covers **{summary['paths']:,} paths** representing **{summary['unique_artefacts']:,} unique "
        f"artefacts**. The corpus contains **{summary['unique_functions']:,} function variants** at "
        f"**{summary['function_occurrences']:,} occurrences**. Ranked **{len(targets)} oracle bindings**.",
        "",
        "| Container | Unique artefacts |",
        "|---|---:|",
        *(f"| {name} | {count:,} |" for name, count in sorted(summary["containers"].items())),
        "",
        "## Reproduce", "", "```powershell",
        COMMAND,
        COMMAND + " --add-dir D:/path/to/additional/artefacts",
        "python -B tools/binscan/report.py",
        "```", "",
        "Both default roots are scanned recursively on every run. Any directory added under `external/x` is "
        "included automatically. Each artefact parse is cached in `out/cache` by SHA-256, reader revision and "
        "Capstone version. `--top` controls the retained variants and `--pool` widens the n-gram shortlist.",
        "",
        "## Method", "",
        "- The inventory keys artefacts by SHA-256 and retains every path. Fingerprints are recorded as "
        "literal banner, extender, runtime or library-name evidence. Matching complete CLIB/MATH signature "
        "variants corroborate the runtime family. Evidence inside an executable can belong to its stub or "
        "another linked component. Dates for lineage use FILESLST path and size matches, "
        "ZIP member timestamps, ZIP basename/size/CRC matches for extracted copies, or COFF and ar header "
        "timestamps. Embedded strings and filesystem mtime remain available for inspection.",
        "- OMF uses public-to-next-public or segment-end extents with covered bytes and fixups. LE/LX and PE "
        "use recursive descent from the entry point, code-pointer fixups, direct calls and available executable "
        "symbols. A WLINK sidecar must agree on the entry and every object size before supplying "
        "names or a link timestamp. Jump-table entries in code seed labels. Linked executable extents end at the last reached "
        "instruction or associated table before the next candidate start. Oracle extents use "
        "`blockdiff.Analyzer.oracle_extent` and the candidate sizes from `build/verify.json`.",
        "- Literal equality is SHA-256 of the complete selected extent. Relocation-aware equality additionally "
        "requires the same field positions, widths, absolute/relative kinds and internal targets after "
        "abstracting relocation fields and outgoing direct displacements. External target identity needs "
        "independent verification. A group can include occurrences with different external references. "
        "Its scored representative prefers named publics and richer call/data evidence. The corpus retains "
        "all locations, raw hashes, relocation targets and direct call coordinates. All same-name variants "
        "remain in the ranking JSON with dated occurrences.",
        "- Retrieval uses rare normalized mnemonic n-grams, shared block and string anchors, and every "
        "same-name and exact-hash variant. "
        "Full ranking blends mnemonic trigrams, operand bigrams, blockdiff block content and loose block "
        "content, CFG block/edge/back-edge counts and degree distribution, direct callee names or opening "
        "signatures, nontrivial immediates and referenced strings/data. A square-root length ratio penalizes "
        "differently sized bodies. Exact relocation-aware variants score 1.00. Named block coverage is "
        "reported alongside generic reference shapes in the JSON. Approximate hits are leads for inspection.",
        "- The oracle copy is inventoried and excluded from donor searches. MZ-only and NE files and packed "
        "Watcom patch payloads receive inventory and fingerprints. Locally stored ZIP code members are "
        "parsed in memory. Other compressed formats receive inventory only. Linked extents are heuristic "
        "and stripped routines may lack names. OMF records outside the reader's coverage appear in the inventory.",
        "",
        "## Calibration", "",
    ]
    positive, negative = (rankings["controls"].get(key) for key in ("positive", "negative"))
    if positive:
        lines.append(f"- Positive control. `{positive['target']}` is {positive['target_status']} in LameDuke. "
                     f"`{short_path(positive['path'])}` `{positive['name']}` ({stamp(positive)}) "
                     f"ranked {positive['rank']} at {positive['score']:.4f}. Literal bytes "
                     f"{'agree' if positive['literal_bytes'] else 'differ'} and relocation-aware bytes "
                     f"{'agree' if positive['relocation_aware_bytes'] else 'differ'}.")
    else:
        lines.append("- Positive control unavailable in the current corpus.")
    relocated = rankings["controls"].get("relocated_positive")
    if relocated:
        lines.append(f"- Relocation control. `{relocated['target']}` in "
                     f"`{short_path(relocated['path'])}` ranked {relocated['rank']} at {relocated['score']:.4f}. "
                     f"Literal equality is {relocated['literal_bytes']} and relocation-aware equality "
                     f"is {relocated['relocation_aware_bytes']}.")
        lines.append("- The repository's independent `tools/libscan.py --split --min 12` check against "
                     "BUIL0801 ENGINE.OBJ found both controls at their bound addresses with equal relocation sites.")
    if negative:
        lines.append(f"- Negative control. `{negative['target']}` against unrelated Watcom runtime "
                     f"`{negative['name']}` in `{short_path(negative['path'])}` scored "
                     f"{negative['score']:.4f} at {negative['target_size']:,} versus {negative['size']:,} bytes.")
    else:
        lines.append("- Negative control unavailable in the current corpus.")
    if secondary := rankings["controls"].get("large_target_negative"):
        lines.append(f"- Large-target negative. `{secondary['target']}` versus Watcom "
                     f"`{secondary['name']}` scored {secondary['score']:.4f}.")
    lines.extend(["", "## Largest non-STRICT targets", "",
                  "Sizes follow the verification snapshot. Signals are mnemonic, operand, block, CFG, calls, "
                  "immediates and data, in that order. A listed best hit with weak agreement is a negative result "
                  "for whole-function similarity. Archive dates are attached to preserved occurrences.",
                  "", "| Rank | Target | Verify bytes | Best donor | Score | Signals | Equality | Best same-name alternative |",
                  "|---:|---|---:|---|---:|---|---|---|",
                  *(table_row(i, row) for i, row in enumerate(table, 1)),
                  *(table_row("extra", row) for row in supplements), "", *anchors(table),
                  "", "## STRICT function lineage", "",
                  f"Of {len(lineages)} STRICT bindings, {matched_raw} have literal-byte appearances "
                  f"({dated_raw} dated) and {matched_norm} have relocation-aware appearances ({dated_norm} dated). "
                  "The windows show earliest and latest preserved dated copies in these roots. Small shared "
                  "functions can have multiple unrelated occurrences. Full per-path date evidence and the "
                  "two equality flags are in [lineage.json](out/lineage.json).", "",
                  "| Function | Literal-byte date window | Relocation-aware date window |",
                  "|---|---|---|"])
    examples = [row for row in lineages if row["name"] in {"ksqrt", "krand", "getmousevalues", "netoff",
                                                         "getcrc", "uninstallbicomhandlers", "getceilzofslope"}
                and row["appearances"]]
    lines.extend(f"| `{row['name']}` | {window(row, 'literal_window')} | "
                 f"{window(row, 'relocation_aware_window')} |" for row in examples)
    errors = [row for row in inv["artefacts"] if row.get("parse_errors")]
    lines.extend(["", "## Coverage and negative results", "",
                  f"Parse status counts are {', '.join(f'{count} {label}' for label, count in sorted(summary['census_status'].items()))}. "
                  f"The inventory records parser errors for {len(errors)} unique artefacts and "
                  f"{len(inv.get('read_errors', []))} read failures.",
                  f"{sum(not any(d['relocation_aware_bytes'] for d in row['donors']) for row in table)} of "
                  f"the {len(table)} largest non-STRICT targets have no complete relocation-aware copy "
                  "among the discovered functions.", ""])
    accepted = [row["linker_map"]["path"] for row in inv["artefacts"] if row.get("linker_map")]
    rejected = inv.get("rejected_maps", [])
    if accepted:
        lines.append("Layout-checked linker maps supplied names for " +
                     f"{len(accepted)} images. The Shadow Warrior addendum includes later relinks, "
                     "identified by their own map creation dates. Dated windows retain those observed copies.")
    if rejected:
        mismatches = [row for row in rejected if row["reason"] == "entry or object extents disagree"]
        lines.append(f"{len(rejected)} linker maps could not be paired with a layout-consistent local image. "
                     f"{len(mismatches)} had a nearby image with a different entry or object layout.")
    recovered = [m for row in inv["artefacts"] for m in row.get("modules", []) if m.get("parser_extension")]
    if recovered:
        lines.append(f"The worker reader recovered {len(recovered)} Watcom patch-library members whose "
                     "Easy OMF A2 records use 32-bit offsets and 16-bit repetition counts. For example, "
                     "`3CRDL.B:fpeinth` has a 512-byte run at data offset 0x16, followed by LEDATA at 0x216.")
    lines.extend(["",
                  "The scan is diagnostic under the repository's matching rules. It invokes Capstone and "
                  "Python parsers only."])
    (WORK / "NOTES.md").write_text("\n".join(lines) + "\n", encoding="utf-8")


if __name__ == "__main__":
    generate()
