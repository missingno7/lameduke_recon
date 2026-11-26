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

## Worker final answer
First line `STATUS: done | partial | blocked | failed`, then <=15 lines: scope, hypotheses tested, results
(STRICT/CODE_BYTES_MATCH/NEAR counts and bytes, first diffs), candidate paths, overlay bindings, verifier command +
output, new toolchain facts, negative results, blockers, suggested next step.
