# LameDuke matching decompilation — working rules (humans, Claude, Codex workers)

Goal: readable C (and assembly only where the original was assembly) that the original historical toolchain
compiles and links back into the exact bytes of `assets/D3D.EXE` — LameDuke, the 1994-12-30 Duke Nukem 3D
prototype (MZ stub + LE, DOS/4GW). Matching decompilation only: no remakes, ports or modernisation until the
original program is reconstructed. Read `docs/STATUS.md` (current numbers and plan), `docs/FINDINGS.md` (evidence,
PROVEN/STRONG/HYPOTHESIS/NEGATIVE) and `docs/TOOLCHAIN.md` before working.

## Hard rules
- `assets/` (original binaries/data) and `external/` (reference clones, downloads, extracted archives) are
  read-only and git-ignored. Never modify them; never commit their contents or anything mechanically derived from
  them (disassembly dumps, extracted objects). Inventory: `config/oracles.json`, `config/references.json`.
- Historical tools live under `C:\tools` and are immutable. Compile only through `tools/wcc.py` / `tools/match.py`
  (hash-checked, isolated environment, TMP under `build/tmp`). Never put them on the global PATH.
- The verifier decides. STRICT = every byte of the function extent equal at its bound address, identical fixup-site
  sets, every relocation target verified through an independently established binding. Masked/approximate equality
  is never STRICT. Never weaken `tools/match.py` to fit a candidate; if a check looks wrong, prove it with
  independent evidence and report it.
- Matching happens only through source processed by the historical toolchain. Never patch objects, executables,
  fixups or layouts; no `_emit`, raw opcode arrays, copied code bytes, `db` blobs of compiled code, INCBIN, or
  comparison masks. Historical .OBJ files are evidence and references; they are never substituted for source.
- Assembly is legitimate only where the original was assembly (Ken's `A.ASM`, runtime members). Unknown bytes in
  assembly-origin source stay explicit, labelled debt.
- Library code (Watcom CLIB/MATH, Apogee Sound System if supplied as a LIB) is identified and tracked separately
  from reconstructed source; it never counts as reconstructed C.
- Workers write only inside `build/workers/<name>/`, propose symbol bindings in an overlay JSON used with
  `match.py --symbols`, make no git commits, start no background processes, and run one compile at a time.
  Only the integrator edits `src/`, `config/symbols.json`, `config/units.json` and `docs/STATUS.md`.
- Keep facts, hypotheses and open questions apart (PROVEN / STRONG / HYPOTHESIS / OPEN). Record negative results
  in `docs/FINDINGS.md` (integrator) or your worker report, with the exact toolchain and context.
- Keep paths short when compiling (Watcom tools can crash on long paths); `build/tmp` is fine.

## Coordinates and status vocabulary
- Addresses are `obj:offset` hex: object 1 = code (base 0x10000, 0x6c445 bytes), object 2 = data
  (base 0x80000, 0x1c99d0 bytes, first 0xb000 file-backed, rest BSS + stack).
- Function status (mutually exclusive): `STRICT`, `CODE_BYTES_MATCH` (bytes + sites equal, some target unbound),
  `NEAR_MATCH`, `EQUIVALENT_TESTED` (behavioural only), `IDENTIFIED` (identity known, no matching source),
  `UNMATCHED`, `UNKNOWN`. Provenance (HISTORICAL_SOURCE, LATER_SOURCE_BACKPORTED, BINARY_DERIVED, LIBRARY_MEMBER,
  ORIGINAL_ASM) and source quality (NATURAL / COMPILER_STEERED / SYNTHETIC) are tracked separately.

## Semantic references for game/engine code
- external/lameduke-raynorpat: a behavioural C rebuild of LameDuke (GPLv2/BUILDLIC). Its comments cite IDA linear
  addresses: obj1 offset = address - 0x10000 (e.g. `/* 0x249B0 */` = 1:149b0); IDA globals: obj2 offset = address
  - 0x80000 (`dword_B56D8` = 2:356d8). build/map/raynorpat_functions.json lists obj1 addresses with its function names.
  Use it to understand semantics and choose names; it is not matching evidence and its code is not original source.
- external/duke3d-1.3d/dukenukem3d: Duke3D 1.5 source (1996; much later). Ken's 1994 GAME.C in buildold_12_30_94.

## Tools (python from repo root; stdlib + capstone)
- `tools/le.py [EXE]` — LE container census; accounts for every file byte.
- `tools/funcmap.py` — recursive-descent function-start census of obj1 -> `build/map/funcmap.json`.
- `tools/libscan.py [--split] [--min N] LIB|OBJ...` — relocation-aware search of OMF code (whole segments, or
  per public with `--split`) in obj1. `--symbols LIB...` writes `build/oracles/d3d/libsyms.json`.
- `tools/objcmp.py A.OBJ B.OBJ --funcs` — compare two objects (compiler calibration).
- `tools/wcc.py --profile engine SRC.c -fo=OUT.obj [-i=DIR]` — pinned compile.
- `tools/match.py [--profile P] [--symbols OVERLAY.json] [-i=DIR] SRC.c [FUNC...]` or `--obj OBJ` — strict verifier
  with first-difference diagnostics. Bindings: `config/symbols.json` + overlay + library bindings.
- `tools/omf.py FILE.OBJ` — JSON summary of an OMF object/library.
- `tools/decomp/decomp.py` — Ghidra headless drafts of every function (~45 s) into build/decomp/out/<TU>/, using our
  names, LE relocations and the Watcom register convention. Seeds for reading only, never matching evidence.
- `tools/bssrule.py` — predicts Watcom 9.5b _BSS/_DATA offsets from declaration order and names (docs/BSS_LAYOUT.md).
- `tools/semir.py [--unit UNIT] FUNC` (or `--src CAND.c FUNC`) — earliest semantic difference of a NEAR function:
  SOURCE (fix the C), ALLOCATION-ONLY (same dataflow, register/stack choice), LAYOUT (cross-function displacement),
  UNKNOWN. Run it before editing a NEAR function; docs/NEAR_CENSUS.md lists every NEAR function's class.
- `tools/cg95/cgtrace95.py` + `tools/cg95/explain95.py` — Watcom 9.5b register-allocator trace (NT compiler under a
  debugger; identical code to the DOS host) and explanation of the first diverging allocation decision with likely
  source levers; for ALLOCATION-ONLY functions (docs/CG95.md has commands and validation).
- `tools/link/link.py` — natural-link experiment: builds every unit, links with Watcom 9.5 WLINK (DOSBox-X,
  tools/link/game.lnk), compares with D3D.EXE (build/link/run/comparison.json; docs/LINK_REPORT.md). Exit 0 = resolved
  link, 2 = diagnostic image with undefined symbols. Evidence only.
- `tools/xscan/xscan.py` — cross-source donor scan (Ken's snapshots, Build archives, Duke3D 1.3D/1.5 compiled with
  Watcom 9.5): nearest compiled donor functions per LameDuke function; results docs/DONORS.md, config/donors.json.
- docs/ANCESTRY.md — Ken-Build GAME.C counterparts (names, locals, comments) for game functions.
- `tools/gate.py` — integration gate (tests + fresh rebuild of every unit); run before every commit.
- `tools/blockdiff.py` — diagnostic (never proof): basic-block comparison of non-STRICT functions with the oracle
  (content/loose/fuzzy/skeleton/layout/regs), `UNIT FUNC --worklist` lists missing/extra blocks, frame, references
  and register renaming; `--census` triages every unit; `--selftest` (all STRICT functions score 1.0).
- `tools/binscan/` — binary donor corpus and lineage scanner over every artefact under external/ (`binscan.py`, then
  `report.py` → build/binscan/); `tools/omfdeps.py` dates Watcom objects (COMENT 0xE9). docs/HISTORICAL_DONOR_RESEARCH.md.
- `tools/names/apply_names.py DIR` — idempotently names magic numbers in game sources from tools/names/names.json
  (DEFS.CON originals, Duke 1.5, neutral names with comments); byte-neutral, run by the integrator at milestones.
- `tools/permute/permute.py UNIT FUNC... --out DIR` — automated search over semantics-neutral C rewrites
  (declaration order, temporaries, commutative operands, independent statements, loop forms) with whole-unit STRICT
  regression checks and DOS confirmation; workers set PERMUTE_WORK to their own directory.
- `tools/donors.py FUNC...` — one-stop donor lookup (source donors compiled with our Watcom 9.5, dated binary donors,
  Ken GAME.C ancestry, Duke 1.5 / extras / raynorpat hits). RULE: run it before writing or rewriting any function;
  start from the closest historical text and record which donor you used.
- `tools/idioms/idioms.py` — source<->code index of every STRICT function (rebuild: `python -B tools/idioms/build_index.py`
  into build/idioms/, not committed): `near UNIT FUNC` (matched C for each unmatched oracle block), `reverse UNIT FUNC [--external[-loose]]` (oracle blocks with
  STRICT twins incl. code we lack, context-effect blocks, differing inline copies), `query --asm`,
  `c --pattern`. docs/WATCOM95_QUIRKS.md + docs/quirks.json: Watcom 9.5 rules with verified examples. RULE: before
  hand-solving a block, check `idioms.py near` and the quirks catalogue.
- `tools/readme.py` — regenerates the README.md progress bars and the docs/STATUS.md unit table from build/verify.json (after `verify.py --update`).
- Watcom 9.5b compiles run on the canonical DOS host (tools/doshost.py: the original DOS compiler under DOSBox-X,
  ~1.7 s per compile) because CONST padding residue depends on the host; LAMEDUKE_HOST=nt gives the same code
  ~40x faster for exploration, but only DOS-host results count. Never bypass tools/wcc.py.
- Do not split a translation unit into private .inc files: the original files' text leaks into CONST padding
  (GAMEDEF proved it); write each original object as one .c file plus genuine headers.

## Worker final answer
First line `STATUS: done | partial | blocked | failed`, then <=15 lines: scope, hypotheses tested, results
(STRICT/CODE_BYTES_MATCH/NEAR counts and bytes, first diffs), candidate paths, overlay bindings, verifier command +
output, new toolchain facts, negative results, blockers, suggested next step.
