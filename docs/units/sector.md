# LameDuke SECTOR reconstruction

Watcom 9.5b compiles the candidate with `-4r -s -otexan` and reports zero warnings and errors. The unchanged verifier finds **9 STRICT functions / 2,462 bytes** and **9 NEAR_MATCH functions / 39,216 oracle bytes**. The NEAR_MATCH candidate extents total 38,032 bytes. All 18 mapped publics appear in order. Their combined candidate `_TEXT` is 40,494 bytes. The publics imply seven placement bases. Runtime behavior remains untested.

The oracle is `assets/D3D.EXE` with SHA-256 `698979d97f0f2a16ffdea20ff0059fe49ef9720a7c8933a61601356ca0462701`.

## Boundaries and ownership

**PROVEN.** The preceding GAME routine returns at `1:14fac`. Three zero bytes with no fixups precede `checkcursectnums` at `1:14fb0`. The next seven candidate publics fit one base and verify consecutively through `1:15810`.

**PROVEN.** `allignwarpelevators` returns at `1:1f27d`. Its 238-byte extent verifies through `1:1f27e`, followed by two zero bytes with no fixups. The private frame table at `1:1f280` is referenced from `1:1fd33`. The following four tables likewise have PLAYER references at `2031a`, `20722`, `20c14` and `20d5e`. PLAYER's first public code starts at `1:1f2e0`.

**STRONG.** The alignment transition, call clusters and the packed wall and sector references support the proposed SECTOR placement range `1:14fb0–1f280`. The code and table extent before inter-object fill is 41,678 bytes. The historical linker map and exact TU filename remain open.

Watcom places code dispatch tables before the publics that consume them. The verifier includes these ranges in the preceding public's extent. The target fixups distinguish the table ranges from code and from the 12 bytes of alignment at `1:1b4a4–1b4b0`.

| Consuming public | Table range | LE table relocations |
| --- | --- | ---: |
| `operatesectors` | `1:1598c–159e0` | 21 |
| `operateactivators` | `1:181bc–18210` | 21 |
| `playerkeys` | `1:19690–19720` | 36 |
| `checkhitswitch` | `1:1b444–1b4a4` | 24 |
| `checksectors` | `1:1c4d8–1c5e0` | 66 |

These boundaries can be inspected with `python -B build/workers/game/survey.py funcs 14fb0 1f2e0` and `python -B build/workers/game/survey.py dis 14fa9-14ffe 1f190-1f280`. The LE fixup counts come from `Image.sites_in` over the indicated ranges. The census contains data-only starts inside the dispatch tables.

## Function results

The addresses and extents below include all applicable compiler padding and table bytes. The final two bytes of inter-object fill are outside the last extent.

| Function | Obj1 extent | Oracle / candidate bytes | Verifier result |
| --- | --- | ---: | --- |
| `checkcursectnums` | `14fb0–15000` | 80 / 80 | STRICT |
| `findplayer` | `15000–150a0` | 160 / 160 | STRICT |
| `doanimations` | `150a0–15340` | 672 / 672 | STRICT |
| `getanimationgoal` | `15340–15380` | 64 / 64 | STRICT |
| `setanimation` | `15380–15410` | 144 / 144 | STRICT |
| `animatecamsprite` | `15410–15550` | 320 / 320 | STRICT |
| `animatewalls` | `15550–15810` | 704 / 704 | STRICT |
| `activatewarpelevators` | `15810–159e0` | 464 / 464 | NEAR_MATCH at `+38`. 31 instruction bytes differ. All 21 following table targets differ. |
| `operatesectors` | `159e0–18210` | 10,288 / 10,000 | NEAR_MATCH at `+7`. Frame size, register assignment and the following dispatcher table remain open. |
| `operateactivators` | `18210–19540` | 4,912 / 4,624 | NEAR_MATCH at `+c`. The status-list register differs in the prologue. |
| `operatemasterswitches` | `19540–19590` | 80 / 80 | STRICT |
| `operateforcefields` | `19590–19720` | 400 / 400 | NEAR_MATCH at `+15`. 59 instruction bytes differ. The palette store has a different address expression and the following tables differ. |
| `playerkeys` | `19720–1b4b0` | 7,568 / 7,344 | NEAR_MATCH at `+8`. Frame, switch paths and the following wall dispatch tables differ. |
| `checkhitswitch` | `1b4b0–1bb10` | 1,632 / 1,728 | NEAR_MATCH at `+7`. Parameter register and repeated dispatch paths differ. |
| `checkhitwall` | `1bb10–1c180` | 1,648 / 1,648 | NEAR_MATCH at `+7`. 528 instruction bytes differ. The compiler exchanges ESI and EBP for the wall index and impact z. |
| `checkhitceiling` | `1c180–1c5e0` | 1,120 / 1,088 | NEAR_MATCH at `+2a`. 289 instruction bytes differ. The miss branch, frame allocation, light loops and following tables differ. |
| `checksectors` | `1c5e0–1f190` | 11,184 / 10,736 | NEAR_MATCH at `+8`. Frame and both copies of sector operation differ. |
| `allignwarpelevators` | `1f190–1f27e` | 238 / 238 | STRICT |

## Data and independent bindings

- **PROVEN.** The candidate's `CONST`, `CONST2` and `_DATA` segments are empty. The oracle SECTOR code has no obj2 relocations into the file-backed interval below `2:0b000`. File-backed data also has no relocations back into this SECTOR interval. These checks and the adjacent literal pools support an empty initialized contribution.
- **PROVEN.** The candidate defines seven BSS publics in 8,710 bytes. Watcom places `animatesect`, `animateptr`, `animatevel`, `animategoal`, `animwall`, `animatecnt` and `numanimwalls` at offsets `0, 400, c00, 1400, 1c00, 2200, 2204` of its `_BSS`. Their independently observed oracle addresses are `2:a1430`, `a1830`, `0eec8`, `a2030`, `a4c30`, `a5250` and `a53a8`. The verifier reports MISMATCH for the single-base BSS layout.
- **STRONG.** The positions of the animation arrays and the 511-slot insertion guard support four 512-entry arrays. `prelevel` builds six-byte animated-wall records and their count. The following confirmed allocation at `2:a5230` bounds the animated-wall interval at 256 records. The declarations use these capacities.
- **PROVEN.** `bindprop.py --min-funcs 2` on the final result yields zero function proposals, 13 data proposals and zero conflicts. The overlay records data anchors from independent calls, initialization and indexed consumers. In particular, the neartag callee's ABI establishes the three short outputs and dword distance. The original TU ownership of those externs remains open.

## Source and compiler findings

- The candidate is BINARY_DERIVED from the pinned oracle. The prototype behavioral rebuild supplied semantics and provisional labels. Duke 1.5 `SECTOR.C` and Ken's September 1994 `GAME.C` supply source antecedents. Ken's `operatesector` preserves the two separate calculations of the neighboring walls in the sideways double-door. Restoring those calculations and direct sector indexing materially reduces the extent deficits across the four candidate sector-operation copies.
- **NATURAL.** The small STRICT bodies use typed fields and ordinary loops. The toolchain reproduces the byte stores through the packed v5 wall, sector and sprite records. The player stride is 176 bytes. The original Build `klabs` intrinsic is ORIGINAL_ASM from `BUIL0615/PRAGMAS.H`.
- **COMPILER_STEERED.** `doanimations` stages the player adjustment twice and lists the goal copy before the velocity copy. Starting with the velocity copy yields identical instructions and sites with four swapped relocation destinations. Declaring the lists in this TU preserves the verified accesses and gives the reported BSS mismatch.
- **COMPILER_STEERED.** Loading the monitor owner into a short after the nonnegative test reproduces an extra register copy in `animatecamsprite`. A single combined owner assignment and test gives a 304-byte candidate with first difference at `+14`. The actual renderer at `1:11a10` takes a single wall argument. With that prototype, explicit positive tile-bit tests and the nested overlay-tile switch, `animatewalls` verifies across its full 704 bytes.
- **COMPILER_STEERED.** `operate.inc` carries the recovered dispatcher into its callers. The public expands the animation and lift helper bodies. Aliased C declarations preserve independently observed calls to the short helpers in caller copies. The target public has 99 relocation references to the animation count and no calls to the standalone setter. The target activator copy calls the setter 19 times and has four direct calls to the goal lookup. `wallactivate.inc` and `wallforcefield.inc` represent duplicate shot-switch blocks evidenced by the two table destinations in each pair.
- **NEGATIVE.** The final source gives zero STRICT under the MULTI flags `wc95b -4r -s`. The final controls `-4r -s -otexa` and `-4r -otexan` each keep nine STRICT and 2,462 bytes. Historical use of `-s` and the `n` suboption remains open.
- **NEGATIVE.** Before the explicit helper expansions, `-oe=100` and `-oe=1000` expanded small helpers into the caller bodies as well as into `operatesectors`. On that source the activator grew from 3,824 to 7,296 and 8,976 bytes respectively, against 4,912 oracle bytes. `__inline` on a separate C probe produced Watcom 9.5b error E1009.
- **NEGATIVE.** The warp elevator probe retained the correct size and control-flow shape, with register differences beginning at `+38`. Separating the scan indices or writing the two state stores separately increased the differing bytes. The first-difference column for the final build includes the current state of all other failed experiments.
- **OPEN.** `playerkeys`, `render_camera_tile`, `render_mirror`, `update_ripple_tile` and several player fields carry descriptive names. Early branches in the semantic candidate preserve observed unchecked indexing, as in original elevator and interaction paths. `findplayer` and `checkhitceiling` leave local selections unset on paths that do so in the oracle. The caller and map invariants for those paths remain open.

## Reproduce

From the repository root, with one compile running at a time

```powershell
$w = 'build/workers/sector'
python -B "$w/run.py" --profile game --symbols "$w/overlay.json" "-i=$w/src" "$w/src/sector.c" --json "$w/results.json"
python -B tools/bindprop.py "$w/results.json" --min-funcs 2
```

`run.py` invokes the repository's `match.py` and pinned compiler wrapper with worker-local staging, object and temporary paths. An early compiler run used shared `build/cc` staging before the wrapper accounted for the tool's fixed-path staging setting. The final recipe writes its build scratch inside the worker. The verifier exits 1 for this partial result.

The nearest remaining standalone body is the warp elevator at `1:15810`. Work on its register allocation and the table generated by `operatesectors` can proceed independently. Verify both together before promoting the full warp elevator extent.
