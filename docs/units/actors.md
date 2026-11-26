# ACTORS reconstruction

The candidate builds with pinned Watcom 9.5b and the `game` flags `-4r -s -otexan` with zero warnings and errors. It emits all 18 mapped publics in oracle order. The verifier reports **7 STRICT functions and 1,552 bytes**, **0 CODE_BYTES_MATCH functions**, and **11 NEAR_MATCH functions covering 25,239 oracle bytes**. The NEAR_MATCH candidate extents total 21,777 bytes. The complete candidate `_TEXT` is 23,361 bytes. Its publics imply five placement bases. Contiguous placement and runtime behavior remain open.

The oracle is `assets/D3D.EXE`, SHA-256 `698979d97f0f2a16ffdea20ff0059fe49ef9720a7c8933a61601356ca0462701`.

## Boundaries and tables

- **PROVEN.** The PLAYER return at `1:2515b` precedes four zero bytes. ACTORS uses three initializers at `1:25160`, `1:25163` and `1:25168`. The first is a three-byte status list read at `1:25492`. The other two are the four-byte shark frame list and eight-short slime bite frame list, read at `1:260cc` and `1:26874`. All three references originate in ACTORS. The candidate emits the complete 32-byte prefix through `1:25180`, including the internal alignment byte and eight instruction-fill bytes. All bytes agree and both prefix site sets are empty.
- **PROVEN.** There is a separate 112-byte function starting at `1:256e0` after the aligned end of `nuke`. The census groups it under `nuke`. Its EGS call creates tile 488 with three random terms whose masks recur in the moving gib code. The function verifies STRICT under the descriptive name `spawn_blood`.
- **PROVEN.** `movestandables` returns at `1:2ba26`. Nine zero bytes with no fixups precede the independently matched SOUNDS entry at `1:2ba30`. The return-to-return sequence and prefix evidence support a 26,823-byte ACTORS contribution, plus linker fill. **STRONG.** The placement interval is `1:25160–2ba30` and the historical TU name is ACTORS.
- **PROVEN.** The starts `1:25ee8`, `25ef8` and `2a850` are jump tables. Each contains four LE off32 sites into its consuming function, respectively the shark and bat dispatch in `moveactors` and the four-state selector in `movemusic`. The verifier counts those table slots in the preceding public's extent.

## Function results

Byte extents are half-open and include intra-TU alignment and any following jump tables.

| Function | Oracle extent in obj1 | Oracle / candidate bytes | Status | First difference / differing bytes |
|---|---|---:|---|---:|
| `addweapon` | `25180–25480` | 768 / 768 | STRICT | |
| `hitradius` | `25480–255b0` | 304 / 304 | NEAR_MATCH | `+a` / 7 |
| `spawn_gib` | `255b0–25600` | 80 / 80 | STRICT | |
| `ssp` | `25600–25690` | 144 / 144 | STRICT | |
| `nuke` | `25690–256e0` | 80 / 80 | STRICT | |
| `spawn_blood` | `256e0–25750` | 112 / 112 | STRICT | |
| `movesector` | `25750–258c0` | 368 / 368 | NEAR_MATCH | `+c` / 131 |
| `wakeactors` | `258c0–25a60` | 416 / 416 | NEAR_MATCH | `+20` / 65 |
| `ifsquished` | `25a60–25be0` | 384 / 384 | NEAR_MATCH | `+3e` / 176 |
| `effectinsect` | `25be0–25c30` | 80 / 80 | STRICT | |
| `ifhitbyweapon` | `25c30–25d50` | 288 / 288 | STRICT | |
| `guts` | `25d50–25f10` | 448 / 432 | NEAR_MATCH | `+9c` / 84 |
| `moveactors` | `25f10–28f30` | 12,320 / 10,240 | NEAR_MATCH | `+8` / 8,506 |
| `moveweapons` | `28f30–294e0` | 1,456 / 1,312 | NEAR_MATCH | `+15` / 992 |
| `moveeffects` | `294e0–2a470` | 3,984 / 3,216 | NEAR_MATCH | `+8` / 2,635 |
| `moveplayers` | `2a470–2a860` | 1,008 / 1,008 | NEAR_MATCH | `+2d4` / 59 |
| `movemusic` | `2a860–2ab90` | 816 / 816 | NEAR_MATCH | `+12` / 158 |
| `movestandables` | `2ab90–2ba27` | 3,735 / 3,281 | NEAR_MATCH | `+8` / 2,563 |

## Data and bindings

- **PROVEN.** `CONST`, `CONST2` and `_DATA` are empty in the candidate. The oracle range has just one relocation below `2:0b000`, to `music_files` at `2:0ac70` from `movemusic`. GAMEDEF independently populates that zero-filled array. No obj2 relocation targets the ACTORS code range. These facts support an empty ACTORS initialized-data contribution apart from the verified `_TEXT` prefix and the three dispatch tables.
- **PROVEN.** The candidate's 8,192-byte `_BSS` verifies at `2:a2830–a4830`. The 4,096-byte arrays `msx` and `msy` start at `2:a2830` and `2:a3830`. GAME initialization and SECTOR movement independently corroborate their use and bases. The Watcom 9.5b predictor gives offsets 0 and 4096 when both identifiers first appear on the same symbol page. Header declaration order matters when the page boundary falls between them.
- **NEGATIVE.** Defining `hittype` in ACTORS alongside the wall arrays yielded a 90,112-byte candidate BSS and incompatible bases under `wc95b -4r -s -otexan`. The shared hit records at `2:893e0` are external in the final candidate. Historical ownership of that shared block remains open.
- **PROVEN.** The sprite, sector and player record strides are 38, 37 and 176 bytes and the hit records have five dwords per sprite. Unknown player fields remain named by offset. The `overlay.json` entries use independent GAME callers, initializer references or distinct routine boundaries. Final `bindprop.py --min-funcs 2` produced two function proposals and three data proposals with zero conflicts.

## Source and compiler findings

- **NATURAL.** The core matches use fixed-width fields, status-list loops, and unsigned tile dispatch where indicated by the oracle. The small routines are BINARY_DERIVED. The near dispatchers were drafted with the prototype behavioral reference, then revised against the oracle and Ghidra drafts. The tile macros follow the distributed `DEFS.CON`. The Build `klabs` intrinsic is ORIGINAL_ASM from `BUIL0615/PRAGMAS.H`.
- **COMPILER_STEERED.** Promoting the three EGS motion arguments to long reproduces `spawn_gib` and `spawn_blood`. The short motion prototype produced 49 and 77 byte differences in those routines. The two sprite-to-angle indices in `ssp` preserve the different observed narrowing. Expanded macros model the repeated hit scan, motion and gravity sequences in the oracle. The standalone `guts` emits direct EGS calls and the slime dispatcher uses the five independently observed calls to `spawn_gib`.
- **COMPILER_STEERED.** Staging the repeat-height term and placing the tile-60 quote last gives `wakeactors` the 416-byte oracle extent. Staging the masked angle and reading the tick byte before the holoduke frame advance reduced `moveplayers` to 59 byte differences. Its extent also includes the `movemusic` dispatch table.
- **NEGATIVE.** A tile enum changed Watcom's operand typing. The otherwise equivalent enum build lost STRICT for `effectinsect` and `ifhitbyweapon`. Integer macros restored both. Direct branch, typed local and expression-order probes for `hitradius` retained the best seven differences, which are the reversed two spill slots, the Y square's input register, and the next-sprite index register. The fixup sites and targets agree at its bound start.
- **NEGATIVE.** `wc95b -4r -s` produced zero STRICT on the same small routines. `-4r -s -otexa` and `-4r -otexan` each retained seven STRICT and 1,552 bytes. These controls calibrate the optimizing profile. Historical use of the `s` and `n` options remains open.
- **OPEN.** `spawn_gib`, `spawn_blood`, `effectinsect`, several player fields and the `actors_text_end` boundary label use descriptive spellings. The larger updates still have frame and control-flow differences. The most direct remaining compiler question is what source lifetime gives the two `hitradius` arguments their oracle spill slots.

## Reproduce

From the repository root in PowerShell, using the stock compiler staging and worker-local object output.

```powershell
$w = 'build/workers/actors'
python -B tools/wcc.py --profile game "$w/src/actors.c" "-fo=$w/actors.obj" "-i=$w/src"
python -B tools/match.py --obj "$w/actors.obj" --symbols "$w/overlay.json" --json "$w/results.json"
python -B tools/bindprop.py "$w/results.json" --min-funcs 2
```

The compile-and-match equivalent is `python -B tools/match.py --profile game --symbols "$w/overlay.json" "-i=$w/src" "$w/src/actors.c" --json "$w/results.json"`. The verifier exits 1 for the partial result.
