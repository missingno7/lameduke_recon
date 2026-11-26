# LameDuke Ghidra drafts

Run from the repository root with Python 3, Capstone, Java on PATH, and
`C:/tools/ghidra_12.1_PUBLIC` installed.

```powershell
python -B build/workers/decomp/tools_proposed/decomp.py
```

The command rebuilds the raw image, project program, index, TU directories
and quality samples. It replaces the generated `d3d.raw` program in the
worker's `ghidra/LameDuke.gpr` on each run. Save any manual Ghidra edits in
a different program before rebuilding. Use `--ghidra` to choose an
installation. The tested combination was Ghidra 12.1 and Temurin 25.0.4.

## Verified run

The final rebuild took **42.12 seconds**, including 1.24 seconds of input
preparation and 40.69 seconds in the headless process. Ghidra reported about
12 seconds for its main analyzer pass. The project saved and the export
check found one file per index entry.

| Coverage | Entries |
|---|---:|
| Census starts with decompiled C | 1,075 |
| Additional starts with decompiled C | 205 |
| Census starts on data | 42 |
| Census starts inside instructions | 167 |
| Total indexed entries | 1,489 |

The 205 additional starts include 184 independently bound addresses, two
oracle-decoded starts, and 19 found during Ghidra analysis. The 1,280 drafts
cover the 1,280 function records in the saved project. The 209 diagnostic
files account for the other census starts. The import retained 1,828
address/name bindings and 21,029 relocation sites. The raw object slices
were checked against the parser output, including the zero-filled BSS.

Ghidra emitted `halt_baddata` in 26 drafts and reported 10 unrecovered
jump tables. Those drafts can have incomplete control flow. The logs and
inline warnings also expose writes to .text in the assembly rasterizer.

## Inputs and outputs

- `work/d3d.raw` contains both LE objects at their documented linear
  bases. Its bytes come from the hash-checked parser in `tools/le.py`. It
  includes the zero-filled BSS. Ghidra maps `.text`, `.data` and `.bss`
  and applies the LE relocations to its own memory image. Each relocation
  retains its source address, original bytes and resolved target in the
  project. The 21,046 LE records resolve to 21,029 distinct source sites.
- The run invokes `tools/funcmap.py --out` and executes the repository's
  `refnames.py` extractor with its hard-coded map paths redirected into
  `work/`. Names from `symbols.json` have priority, followed by
  `library_symbols.json` and Raynorpat. Alternate names remain labels. The
  project also keeps labels outside the export starts.
- `out/index.json` contains LE coordinates, names, TU, size, body byte
  counts, call targets, and data references. For a census entry, `size` is
  the distance to the next original census start. For a bound addition it
  is the distance to the next merged seed. For a function discovered only
  by Ghidra it is the span from the entry to the last byte of its body.
  These sizes can include alignment or intervening code. `body_bytes` is
  Ghidra's current body size. `evidence` identifies entries added by Ghidra
  and preserves the original reason tags for census entries.
  `callees` includes resolved flow references to entry points, including
  direct tail jumps. `globals` combines LE targets with Ghidra data
  references. Those lists can be incomplete for computed references.
- `out/<TU>/<offset>_<name>.c` holds the decompiler result. Census candidates
  on data or inside another instruction have comment files with the
  conflicting location recorded in the index. Game TUs follow
  `docs/GAME_MAP.md`, and the engine/MULTI/assembly boundaries follow
  `docs/STATUS.md`. Audio uses the Creative, Gravis and Apogee boundaries
  in `docs/AUDIOLIB.md`.
- `samples/` holds paired `__cdecl` and `__watcall` drafts for five game
  routines. The comparison changes the selected function's convention
  while preserving the same analysis, inferred callees, and global types.
  The final TU drafts also apply the checked C signatures. `work/` keeps
  the generated input, analysis logs, coverage counts and wall timings.

The program declares packed Build v5 sector, wall, sprite, and extra layouts
from `src/engine/BUILD.H` and applies them at the bound array addresses.
The SOUNDS records and some GAMEDEF data types come from checked-in
declarations. The player array uses the 176-byte partial SOUNDS view of
the record. Only its declared fields have names.

The register model allocates EAX, EDX, EBX and ECX in order, then stack slots.
It models Watcom's preserved integer registers apart from the return
registers and uses Ghidra's per-function stack-purge inference. In the
oracle, `xyzsound` takes the fifth argument on the stack and uses `ret 4`.
`getincangle_shift` reads AX, DX and BL, and `bootleg_cache_pop` saves and
restores EBX, ECX, EDX and ESI. The variadic `open` and `sopen` entries use
the stack convention, verified from their prologues, the `loadsounds` call,
and Watcom 9.5b's `IO.H`. Creative uses the stack convention as a working
model. Gravis and Ken's assembly retain default or custom Ghidra inference.
The register model approximates aggregate, x87, 64-bit, and custom
`#pragma aux` argument passing.

## Comparison with five registered STRICT functions

| Function | What the draft recovers | What a matching worker still needs |
|---|---|---|
| `sounds/2baa0_bootleg_cache_pop.c` | Signed depth test, decrement, cursor subtraction and remaining-space addition. | The compiler folded `cache_depth - 1` into an indexed displacement four bytes before `cache_sizes`. Ghidra prints an anonymous data address and the old depth scaled by four. Reconstruct the indexed load from the displacement and element width. |
| `sounds/2bbd0_loadsounds.c` | The file test, allocator, read, close and pointer store follow the original sequence. With stack varargs and the checked runtime signatures, all five calls carry their arguments. | The two byte-stride induction variables correspond to one source index over 39 files. The numeric mode and flag arguments are `S_IREAD` and `O_RDWR \| O_BINARY`. The read count is unsigned while `filelength` returns signed long. |
| `sounds/2bcb0_xyzsound.c` | The player and sprite camera branches, typed array fields, distance/priority tests and both writes to the voice Y field survive. The Watcom draft finds all five arguments. | The untyped draft calls register inputs `unaff_DX`, `unaff_EBX` and `unaff_ECX`. The source declaration supplies signed word arguments and signed long coordinates. The inline `klabs` tests and select conditions need source-level cleanup. |
| `gamedef/2e700_getincangle_shift.c` | Both branches, the signed delta and arithmetic right shifts. The cdecl control leaves three unexplained register inputs. The Watcom control recovers them as parameters. | The inferred return is `int` and the shift input appears byte-sized. The checked source declares three signed shorts and a signed short return. The five-bit shift-count mask reflects x86 instruction semantics. |
| `gamedef/2e7b0_getlabel.c` | The lexical scan, newline count, early exit and 64-byte label indexing. The counter is a signed short and the cursors are char pointers. | The inlined `isalnum` appears as an `_IsTable` mask, and the inlined `ispecial` appears as explicit newline, space and carriage-return cases. Preserve those tests when recovering the source macros and loop form. |

The first address-ordered decode let weak census entries claim bytes needed
by named code, including `addweapon`, `AWE32INITNRPN` and `hchainasm64`.
Prioritizing independently bound entries recovered those entries. Most
remaining instruction-interior candidates are in the self-modifying
assembly rasterizer. The two initially undecodable runtime entries at
`1:69a53` and `1:6a2d0` are tables in EMU387. Their LE references are
indexed data operands at `1:69b18`, `1:69b76` and `1:6a45d`.

Autoanalysis initially made one-byte function records at `1:4a91c`
(Creative) and `1:63931` (CLIB). The original code has ordinary function
prologues immediately after the preceding returns. The rebuild seeds those
two addresses before the general analysis pass.

The drafts support manual source reconstruction. Use the historical
toolchain and `tools/match.py` to verify matching extents and bytes.
