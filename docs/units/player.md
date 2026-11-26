# PLAYER reconstruction

Watcom 9.5b `-4r -s -otexan` compiles the eight recovered entries in oracle order. The verifier gives **1 STRICT / 304 bytes**, **0 CODE_BYTES_MATCH**, and **7 NEAR_MATCH / 23,884 oracle bytes**. The seven near candidates occupy 24,280 bytes. The candidate emits 24,680 bytes of `_TEXT` against 24,284 bytes through the terminal return. Publics imply bases `1:1f280`, `1:1f2a0`, and `1:1efe0`. Contiguous placement and runtime behavior remain open.

## Code boundaries

**PROVEN.** The SECTOR routine returns at `1:1f27d`. Two zero bytes precede the private initializers. The PLAYER code starts at `1:1f2e0`. Its final return is at `1:2515b`, followed by four zeros and the ACTORS prefix at `1:25160`. The two zero-filled gaps have no LE fixups. The overlay anchors the end of the last function at `1:2515c`.

**PROVEN.** The prologues at `1:1f410` and `1:21280` start after terminal returns and compiler alignment. The call census had combined them with `kickview` and `getinput`. The laser-sight hitscan and EGS sequence and the underwater air and volume logic appear again inside `processinput`. Independent call sites anchor the other six entries. For example, ACTORS calls `kickview` at `1:28ff9`, and GAME calls `displayweapon` at `1:05f34`, `1:067f2`, and `1:085cf`.

**STRONG.** The boundary gaps, private initializer references and repeated inlined bodies support one PLAYER contribution. The original linker map and module spelling remain open. The separately callable use-key routine `checksectors` starts at `1:1c5e0` in the adjacent SECTOR range.

| Function | Oracle extent | Oracle / candidate bytes | Status and first difference |
| --- | --- | ---: | --- |
| `kickview` | `1f2e0–1f410` | 304 / 304 | STRICT |
| `spawn_pistol_tracer` | `1f410–1f660` | 592 / 592 | NEAR_MATCH at +08. Frame is 28 bytes versus 32. |
| `shoot` | `1f660–1fd20` | 1,728 / 1,696 | NEAR_MATCH at +0a. Entry argument spills and hitscan output slots differ. |
| `displayraise` | `1fd20–1fe40` | 288 / 288 | NEAR_MATCH at +0a. Shade spill and the display call's argument scheduling differ. |
| `displayweapon` | `1fe40–20e60` | 4,128 / 4,832 | NEAR_MATCH at +08. Frame and argument allocation differ. |
| `getinput` | `20e60–21280` | 1,056 / 1,056 | NEAR_MATCH at +4f. The menu-state zeroing block is four bytes shorter, shifting later sites. |
| `update_air` | `21280–21410` | 400 / 400 | NEAR_MATCH at +d4. Four register-encoding bytes differ, with identical relocation sites and resolved targets. |
| `processinput` | `21410–2515c` | 15,692 / 15,416 | NEAR_MATCH at +2e. Same 272-byte frame. The first live-player branch has a different displacement. |

The `displayraise` verifier extent includes the 11-entry knee pull switch table at `1:1fe0c–1fe38` and the following alignment. The source's explicit cases 0 and 1 reproduce the first two entries targeting the common continuation. The candidate table entries move with `displayweapon` and its case blocks.

## Data and binding evidence

- **PROVEN.** `_DATA` verifies at `2:02364–023dc`, all 120 bytes with complete emitted coverage and no fixups. `knee_frames[6][20]` accounts for the initialized sequence bytes and their zero suffixes. Display uses the same indexed table at `1:20039`, `1:200e9`, and `1:2016f`. The input routine loads the frame at `1:23c64` and treats `0xff` as the sequence terminator.
- **PROVEN.** All initializer payloads in the 96-byte `_TEXT` prefix match. The referenced arrays begin at `1f280`, `1f296`, `1f2aa`, `1f2b4`, and `1f2c9`. The unreferenced five shorts at `1f2a0` are consecutive weapon display tiles 882 through 886. An unused local aggregate with those values emits the observed words. The candidate matches 95 of the 96 prefix bytes. At `1:1f29f` the candidate emits `63` where the oracle has `00`, in the alignment byte after the nine-byte recoil initializer. An earlier build with the helper bodies out of line emitted `00`. Varying the compiler stage path on the final source produced `63` or `74` at that byte. The original source text and stage path remain open.
- **PROVEN.** The candidate has empty `CONST`, `CONST2`, and `_BSS` contributions. The observed functions reference no separate PLAYER literal pool. Player state and input buffers use extern declarations. No BSS layout is claimed, so applying `bssrule.py` to this candidate would predict an empty contribution.
- **PROVEN.** The record strides are 176 bytes for players, 38 for map-v5 sprites, and 37 for map-v5 sectors. GAME demo input stores the signed turn byte at `2:a53bb`. Timer IRQ writes independently establish the three ramp globals. The USERDEFS first byte supplies the render visibility copied by PLAYER. The overlay records these cross-references, callee behavior and independently observed extents. `bindprop.py --min-funcs 2` reports zero additional proposals and zero conflicts.

## Provenance and compiler tests

The reconstruction is **BINARY_DERIVED**. The Raynorpat LameDuke rebuild supplied the gameplay reading and portions of the initial movement and weapon-frame control flow. Case order, exact widths, offsets and call conventions were checked against the executable. Ken's September 1994 `GAME.C` supplies the parallel sync-input and timing idiom, and the August 1994 Build declarations support the volatile key array. The prototype `DEFS.CON` supplies the tile names. The historical `klabs` intrinsic in the header is **ORIGINAL_ASM** from June 1994 Build `PRAGMAS.H`.

`kickview` is **COMPILER_STEERED** for the caller ABI. The local angle is short, but PLAYER consumes the full register from `getangle` and declares the angle-step result full-width before narrowing it to a short. The full-width second angle-step argument suppresses an extra sign extension. The stored halfwords and compare order then match. `displayraise` uses a short tile local and a delayed tile-base addition. `displayweapon` retains the unused five-short initializer and explicit jump-table cases. `processinput` shares C bodies through three `.inc` files to reproduce the in-function copies under the requested profile. These choices are **COMPILER_STEERED**. The air logic, input fields and shot dispatch otherwise follow **NATURAL** C shapes. `kickview`, `displayraise`, `draw_weapon_tile`, `spawn_pistol_tracer`, `update_air`, and `knee_frames` are descriptive names. Several player member spellings and unassigned fields remain provisional.

- **PROVEN.** The default profile produces STRICT `kickview`. A `wc95b -4r -s -otexa` control also matches it. These checks support the profile as a reproducing configuration for PLAYER. The original option string remains open.
- **NEGATIVE.** Declaring both angle APIs as short-return in PLAYER kept the 304-byte extent but left an extra conversion at the call and mismatched compare scheduling. Changing only the second argument type still left 14 differing bytes.
- **NEGATIVE.** With ordinary helper calls and the requested profile, the first complete `processinput` candidate was 6,123 bytes. A diagnostic build of that same source with `-oe=2000` reached 15,271 bytes with the inline copies. The requested profile stays in use. Tests of `__inline`, `__inline__`, `_inline` and `inline` failed to compile under 9.5b. Explicit shared C bodies restore the inline code regions but leave differences in case layout, register allocation and stack slots.
- **NEGATIVE.** Under the requested profile, `update_air` local-index, health-alias, opposite-condition, and separate-volume-call variants failed to remove the register difference. The oracle sign-extends the player index into EBX for the health block, and the candidate sign-extends it into EAX. `getinput` reset chains and volatile motion-accumulator controls left differences. The retained volatile key array preserves the two samples of keypad plus and the observed per-key stores.
- **NEGATIVE.** Shorter include filenames and the tested stage paths failed to reproduce the final prefix alignment byte. The natural nine-byte recoil array is retained.

## Reproduce

From the repository root in PowerShell

```powershell
$w = 'build/workers/player'
python -B "$w/run.py" match --profile game --symbols "$w/overlay.json" "-i=$w/src" "$w/src/player.c" --json "$w/results.json"
python -B tools/bindprop.py "$w/results.json" --min-funcs 2
python -B "$w/scratch/layout.py"
```

The wrapper invokes the unchanged verifier and compiler wrapper while keeping scratch and the match object in the worker directory. The verifier exits with status 1 for this partial result. The oracle is `assets/D3D.EXE`, SHA-256 `698979d97f0f2a16ffdea20ff0059fe49ef9720a7c8933a61601356ca0462701`.
