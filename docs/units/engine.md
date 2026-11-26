# December 1994 Build engine

Follow-up candidate for the 1994-12-30 executable, built with the repository's Watcom 9.5b engine profile `-4r -s -or`.

| Verifier status | Functions | Candidate bytes | Target bytes |
| --- | ---: | ---: | ---: |
| STRICT | 42 | 10,392 | 10,392 |
| CODE_BYTES_MATCH | 0 | 0 | 0 |
| NEAR_MATCH | 56 | 68,074 | 70,603 |
| UNPLACED | 0 | 0 | 0 |
| Total | 98 | 78,466 | 80,995 |

The three new STRICT functions are `loadpics` (773 bytes), `nextsectorneighborz` (233), and `updatesector` (202). All 39 integrated STRICT functions still verify. `overlay.json` proposes 38 data addresses with oracle operand evidence. The function addresses are already in the integrated registry.

## Reproduce

From the repository root, use the current matcher and profile.

```powershell
$p = 'build/workers/engine'
python -B tools/match.py --profile engine --symbols "$p/overlay.json" "-i=$p/src2" "$p/src2/ENGINE.C" --json "$p/results.json"
```

The compile produced 0 warnings and 0 errors. The verifier exited 1 with 56 NEAR functions. It reported 75 VERIFIED `CONST` bytes at `2:1184` and 237 VERIFIED `_DATA` bytes at `2:2c00`. The integration gate `python -B tools/gate.py` passed all nine tests and the zero-regression rebuild of the integrated units. Its engine entry used the integrated baseline.

## Whole-object code layout

**PROVEN anchors.** The IRQ 9 install in `initengine` points to `keyhandler` at `1:33e5c`. The preceding byte at `1:33e5b` ends the previous function. The last strict engine function, `setbrightness`, returns at `1:47abe`. The next 24 bytes starting at `1:47abf` are the six-entry switch table for the networking initializer at `1:47ad7`.

**STRONG extent and order.** The engine owns `[1:33e5c,1:47abf)`, or 80,995 bytes. The 1994-09-15 entry moved COM and networking into MULTI.OBJ. The August `netoff`, `comsend`, `convalloc32`, `simulateint`, `getcrc` and `uninstallbicomhandlers` fingerprints all land in that next unit. The table below follows the 98 oracle starts. Watcom emits all 98 candidate publics in this order, with 66 equal-length extents. The translation of `_TEXT` offset zero to `1:33e5c` holds for the first three starts. The first layout drift is the nine-byte `scansector` deficit. At `wallscan` the candidate is 814 bytes early, and it ends 523 bytes early. The candidate ends 2,529 bytes short of the target object.

**PROVEN rotate-sprite boundary.** `rotatesprite` owns `[1:46c60,1:47239)`, or 1,497 bytes. Its internal clipping goes from `1:46f97` through `1:470ae`, followed by the column setup and `spritevline` call. Direct engine calls in this extent are to `animateoffs` and `loadtile`. This accounts for the unplaced 677-byte `clippoly4` introduced with the later implementation. It was removed and `rotatesprite` was rebuilt around the target's five-argument entry and direct clipping. The replacement is 1,538 bytes and remains NEAR.

**PROVEN shared epilogues.** The map I/O bodies have their full lengths and verified relocation targets. The last jump in each currently goes to `setverts+0x222` in the candidate. The oracle goes to `wallscan+0xb1f` at `1:3848e`. `drawrooms` and `setverts` also use this target and still have work in their own extents. Simply placing callers at their oracle offsets leaves the compiler's choice of epilogue unresolved. Restoring the full `wallscan` prologue, paths and tail is needed.

The oracle's `drawmasks` owns several entries into a shared return sequence. `1:3c318` drops twelve stack bytes before six pops. `1:3c31b` starts with the six pops and `1:3c31c` skips the first pop. `makeradarang` uses `1:3c318`. `initengine` and `nextpage` use `1:3c31b`. `setvesamode`, `loadpalette`, and the sprite-list helpers use `1:3c31c`. The same entries exist 7 bytes earlier relative to the start of the candidate `drawmasks`. `setvesamode` also needs the section-relative real-mode call block to verify.

For 21 functions (7,440 bytes), every differing non-fixup byte is in the displacement of a cross-function jump. Each has its full target extent. This diagnostic provides a layout priority. The status remains NEAR.

## Follow-up source changes and evidence

Dates refer to the archived `BUILD.TXT` unless a distribution is named. For details that the changelog does not date, the table gives the oracle evidence and any dated reference source.

| Date or evidence | Follow-up |
| --- | --- |
| 1994-08-11, 1995-03-08, 1995-04-18 | Restored five-argument `rotatesprite`, with the target's fixed-point bounds and per-column clip and draw path. The March entry records Editart centering and shade/palette arguments. The April entry records the optimized implementation and the transparency argument. |
| 1994-04-22 distribution, 1994-09-16 map v5 | Reused the earlier `nextsectorneighborz` loop, with short wall indices and `nextsector1` links. The December function verifies across the whole extent at `1:40ee9`. |
| 1994-04-22 distribution and December code | Restored the ascending fallback search and short wall indices in `updatesector` at `1:43797`. This follows the early source's global fallback direction and the December source's call sequence as recovered from the oracle. |
| 1994-12-28 proposal, 1995-01-01 map v6 | `drawalls` and `drawmaskwall` set the wall palette from the sector's `floorpal`. `scansector` records the per-scan sprite palette, and `drawmasks` swaps, compacts and passes it to the fifth argument of `drawsprite`. The December entry describes this palette arrangement. The January entry records the new per-wall and per-sprite palette fields. |
| 1994-11-09 and December code | Kept the got-sector test. Restored early empty-sector exit, sprite-list termination at -1, and the scan/bunch loop shape in `scansector`. December anchors include the bunch counter accesses at `1:343c0`, `1:34409` and `1:3442c`. The changelog dates got-sector tracking. The remaining control-flow details come from the oracle and the early C. |
| 1994-12-08 and December code | Moved the 13-byte capture literal to `CONST` via Watcom's intrinsic string copy. `screencapture` references `2:11c2`. Its copy is three MOVSD followed by MOVSB, the target prefix through the copy. The capture count belongs to BSS and `initengine` clears it. |
| 1994-04-04 ART format and December code | Restored the cache allocation retry in `loadpics`. The loop at `1:3ae24` decrements the request by 64 KiB after a failed allocation. `artfil` gets its -1 sentinel in `loadpics` and lives in BSS. The ART loader calls the library's `strcpy_` directly. The candidate uses a Watcom name alias to retain that call while enabling the intrinsic for capture. Mark this function's copy-site spelling COMPILER_STEERED. |
| 1994-04-22 distribution, 1994-11-29 floor sprites and December code | Restored one 1,024-entry horizon reciprocal table for the `ceilspritehline` path. `hline` uses the earlier division, separate multiplicative terms, and floor-Y reversal, corroborated at `1:39a97` and `1:39cfb`. The March source has a second table and different arithmetic. The changeover date between these implementations is open. |
| December code | Reordered the polynomial accumulator initialization in `makeradarang`. Its register choices now align with the oracle through the final jump to the `drawmasks` epilogue. No changelog date was found for this code-generation detail. |

## Engine data and Watcom 9.5 BSS

**PROVEN initialized spans.** The 75-byte `CONST` span begins at `2:1184` and ends before `2:11cf`. The 237-byte initialized `_DATA` span is `[2:2c00,2:2ced)`. It contains the ART cache values and flags, then the 128-byte PCX header at `2:2c24`, the 48-byte VGA palette at `2:2ca4`, setup defaults at `2:2cd4`, and `editstatus` at `2:2ceb`. Both whole segments pass the verifier.

**STRONG candidate BSS extent.** The ART loader anchors `localtilestart` and `localtileend` at `2:b1e00` and `2:b2200`. The next anchor is `clipobjectval` at `2:b2600`. Independent clip operands place `hitwalls` at `2:b4e00`, with `lastx` at `2:b4e08`. The anonymous 50-byte `setvesamode` real-mode register block begins at `2:1bde9f`, immediately after `visibility`. DPMI setup and field writes in `setvesamode` independently place it. These anchors imply a provisional span of `[2:b1e00,2:1bded1)` or 1,097,937 bytes. Ownership outside these anchors and the adjacent unit boundary need confirmation. The candidate's OMF `_BSS` is 1,100,018 bytes, 2,081 bytes longer than this span. The verifier reports unbound publics and inconsistent segment placement remains.

Large observed BSS anchors include the short `smost` samples at `2:bc590`, camera-space sprite arrays at `2:d0590` and `2:d4590`, and the v5 map arrays `extra`, `sprite`, `sector`, and `wall` at `2:13cf10`, `2:146f10`, `2:16cf10`, and `2:176310`. The target map-array order is `extra`, `sprite`, `sector`, `wall`. The candidate emits `extra` after the three map arrays. The clip arrays also diverge in order. These disagreements persist after the two capture/ART globals move to BSS and the extra horizon table and unused chain-mode globals are removed.

**PROVEN Watcom 9.5b ordering experiment.** Each variant used the engine profile and was read through `tools/omf.py`. A defined 32 byte arrays with sizes cycling from 1 through 8, then eight scalar globals. B reversed the array definitions, keeping sizes and scalar definitions fixed. C kept B's definitions and added the extern declarations in A's order. All 40 BSS public offsets in A and C were identical. A and B differed. The four-byte-multiple entries came ahead of the remaining even sizes, then the larger odd sizes and the one-byte entries in this fixture. First declaration matters. A flat PJW-modulo-241 name sort failed for both plain and OMF-decorated spellings. The full Watcom 10.x bucket/page/hash rule has yet to be established for 9.5b.

Reproduce the three inputs from the repo root. Each is compiled separately with `python -B tools/wcc.py --profile engine SRC "-fo=OUT"` and inspected with `python -B tools/omf.py OUT`. Variant A opens with `tables@0 nodes@8 parallax@10 scanbuf@18`. Variant B opens with `nodes@0 parallax@8 scanbuf@10 tables@18`. Offsets are hexadecimal.

```python
from pathlib import Path

out = Path("build/workers/engine")
names = (
    "tileoff spritelist sectorlist doorbits tiles names field scanbuf "
    "slope vect counts hlines numsprites clipsects front parallax "
    "spritelist2 counters vlines items lastnode floorstat points nodes "
    "windowbuf windowlst palette caches offsets other frames tables"
).split()
scalars = "zflags bflags state ang xpos ypos zpos numitems".split()
types = ("short", "char", "long")

for variant in "abc":
    declarations = [f"char {name}[{i % 8 + 1}];" for i, name in enumerate(names)]
    scalar_defs = [f"{types[i % 3]} {name};" for i, name in enumerate(scalars)]
    if variant == "a":
        lines = declarations + scalar_defs
    elif variant == "b":
        lines = declarations[::-1] + scalar_defs
    else:
        lines = ["extern " + decl for decl in declarations] + declarations[::-1] + scalar_defs
    (out / f"bss{variant}.c").write_text("\n".join(lines) + "\n")
```

## Negative results and remaining dependencies

- The aggregate local PCX filename initializer emitted 13 bytes ahead of the first public in `_TEXT`. An explicit intrinsic `memcpy` placed the literal correctly with a different copy sequence. The intrinsic `strcpy` matches the oracle's copy. Placing `#pragma intrinsic(strcpy)` next to capture or `#pragma function(strcpy)` before the loader still inlined the loader's variable copy across the translation unit. The named library entry restores the oracle's loader call.
- The first hline backport allocated a 44-byte effective frame and caused Watcom to share the otherwise STRICT `clipinsidebox` tail with it. Completing the observed per-term arithmetic, floor reversal and mode addressing restored the standalone 470-byte `clipinsidebox` extent. This is an observed change in optimizer tail selection caused by a different function.
- Rewriting `spritewallfront` as two temporary cross products produced a 116-byte body. The candidate has the full 114-byte extent and 12 register/instruction-order byte differences.
- Reversing the written sort comparison assigned the correct depth operand to ECX but left the two address registers swapped. Reusing the short swap slot and keeping an explicit local for the finishing sprite loop shortened `drawmasks` to 820 bytes. The retained form is 826 bytes against the 839-byte oracle.
- The first pass's `-4r`-only control introduced `__CHK` without stack-check suppression and did not reproduce scheduling. `-orb` failed on option `b` under Watcom 9.5. The current engine profile uses the historical makefile flags. Keep the profile and verifier as integrated.
- The highest-leverage code work is `wallscan` and `drawmasks`, followed by the span adjustments in `scansector`, `calcwalls`, and `drawalls`. A consistent BSS segment base and definitive ownership boundary are still open.

## Prior backports retained

| Date or evidence | Candidate change |
| --- | --- |
| 1994-09-13 | `drawrooms` takes the current sector by value. |
| 1994-09-15 | MULTI stays outside this unit, consistent with the target boundary. |
| 1994-09-16 and October BUILD.H | Map v5 sector, wall, sprite and extras storage. Target map I/O uses 37-, 35-, 38- and 10-byte records respectively. |
| 1994-10-02 and 1994-11-15 | Engine timer and animation passes are absent in this unit. IRQ 9 keyboard handling remains. |
| 1994-11-09 | Retained per-frame `gotsector` tracking. |
| 1994-11-16 and 1994-11-23 | Retained the brightness table and variable-width parallax tiles with `pskyoff` and `pskybits`. |
| 1994-11-17 | Kept the newer wall-index `clipinsidebox` API. Both box intersection functions now verify. |
| 1994-11-29 through 1994-12-06 | Kept floor sprite drawing, clipping and one-sided flags. The drawing and clipping bodies remain near. |
| 1994-12-08 | Rebuilt the capture name as a local `captxxxx.pcx` buffer. The oracle initializes a 13-byte local name from this literal. |
| 1994-12-23 | Raised `MAXWALLS` to 8192, confirmed by the immediate in `lastwall` and the 35-byte wall stride. |
| 1995-01-01 | Reversed v6 wall links and sprite palette, offset and clip-distance fields. Restored `nextsector1/2`, `nextwall1/2`, `spritepal`, and the `movesprite` wall-distance argument. |
| 1995-01-03 | Restored 32-bit starting, active and rendering column arrays from the target addressing modes. `smost` retains 16-bit entries, as shown by the signed word loads in `drawmaskwall`. |
| 1995-01-04 | Restored one-argument `animateoffs`. |
| 1995-01-06 | Reversed bulk map I/O to per-record reads and writes. The loader and saver have the right bytes and sites apart from their cross-function return jumps. |
| 1995-01-16 | Restored 17 setup keys and the earlier `permanentwritesprite` API. The key defaults use the oracle's 17-byte initializer. |
| 1995-02-16 and 1995-02-26 | Restored foot-centered movement in `movesprite` and removed the later `faketimerhandler` calls. |
| 1995-03-10, 1995-03-14 and 1995-03-28 | Removed later 3D line drawing and the two-column translucent wall path. Restored the shared segment/ray argument to `lintersect` using April source and the December call sites. |
| Oracle fixed-point code | All decoded engine SHRD and SHLD instructions use CL. Replaced later immediate-shift calls with the generic historical pragma forms. The target VGA vertical loops use LOOP. Both facts improved body matches. |
| Oracle cache and video code | Restored the -1 tile sentinel, file-number tracking and unconditional seek on a cache miss. Recovered the separate red/blue frame blit and palette setup. Restored five-argument fog palette generation using the target's sum of absolute component distances. |
| April C and August OMF | Used earlier source for `inside`, `bunchfront`, `lastwall`, and the initial intersection/sorting helpers. Kept the historically corroborated inline pragma vocabulary. |

The March 1995 ENGINE.C ends partway through `dragpoint`, so its missing tail was seeded from the June source and backported using the earlier source, public ordering and target disassembly. `BUILD.H` starts with the October map-v5 header and includes dated additions and types needed by the candidate. `PRAGMAS.H` starts with the June header and retains the historical inline assembly forms corroborated in the target. This header is a reconstruction aid and its complete historical form is open.

## Target function order

The target extent ends at the next row. The final extent ends at `1:47abf`. The switch tables for `initengine` and `nextpage` at `1:3a0a0` and `1:3a514` lie immediately before their entry points and are included in the preceding public extents.

| Target start | Target bytes | Candidate offset | Candidate bytes | Function | Status |
| --- | ---: | --- | ---: | --- | --- |
| `1:33e5c` | 188 | `00000` | 188 | `keyhandler` | STRICT |
| `1:33f18` | 928 | `000bc` | 928 | `drawrooms` | NEAR_MATCH |
| `1:342b8` | 756 | `0045c` | 747 | `scansector` | NEAR_MATCH |
| `1:345ac` | 554 | `00747` | 549 | `wallfront` | NEAR_MATCH |
| `1:347d6` | 114 | `0096c` | 114 | `spritewallfront` | NEAR_MATCH |
| `1:34848` | 245 | `009de` | 245 | `bunchfront` | STRICT |
| `1:3493d` | 5,068 | `00ad3` | 4,841 | `calcwalls` | NEAR_MATCH |
| `1:35d09` | 3,811 | `01dbc` | 3,240 | `drawalls` | NEAR_MATCH |
| `1:36bec` | 1,024 | `02a64` | 1,022 | `addboxlinum` | NEAR_MATCH |
| `1:36fec` | 1,209 | `02e62` | 1,209 | `ceilscan` | STRICT |
| `1:374a5` | 1,226 | `0331b` | 1,226 | `florscan` | STRICT |
| `1:3796f` | 2,853 | `037e5` | 3,144 | `wallscan` | NEAR_MATCH |
| `1:38494` | 2,917 | `0442d` | 2,887 | `maskwallscan` | NEAR_MATCH |
| `1:38ff9` | 302 | `04f74` | 302 | `transmaskwallscan` | STRICT |
| `1:39127` | 528 | `050a2` | 528 | `loadboard` | NEAR_MATCH |
| `1:39337` | 481 | `052b2` | 481 | `saveboard` | NEAR_MATCH |
| `1:39518` | 303 | `05493` | 303 | `loadtables` | STRICT |
| `1:39647` | 510 | `055c2` | 510 | `loadpalette` | NEAR_MATCH |
| `1:39845` | 570 | `057c0` | 570 | `setgamemode` | STRICT |
| `1:39a7f` | 965 | `059fa` | 1,016 | `hline` | NEAR_MATCH |
| `1:39e44` | 636 | `05df2` | 626 | `transmaskvline` | NEAR_MATCH |
| `1:3a0c0` | 921 | `06064` | 921 | `initengine` | NEAR_MATCH |
| `1:3a459` | 219 | `063fd` | 219 | `uninitengine` | NEAR_MATCH |
| `1:3a534` | 741 | `064d8` | 741 | `nextpage` | NEAR_MATCH |
| `1:3a819` | 79 | `067bd` | 79 | `initcache` | NEAR_MATCH |
| `1:3a868` | 411 | `0680c` | 411 | `allocache` | NEAR_MATCH |
| `1:3aa03` | 198 | `069a7` | 198 | `allocatepermanenttile` | STRICT |
| `1:3aac9` | 258 | `06a6d` | 258 | `loadtile` | STRICT |
| `1:3abcb` | 773 | `06b6f` | 773 | `loadpics` | STRICT |
| `1:3aed0` | 122 | `06e74` | 122 | `showengineinfo` | STRICT |
| `1:3af4a` | 23 | `06eee` | 23 | `resettiming` | STRICT |
| `1:3af61` | 470 | `06f05` | 470 | `clipinsidebox` | STRICT |
| `1:3b137` | 375 | `070db` | 375 | `clipinsideboxline` | STRICT |
| `1:3b2ae` | 226 | `07252` | 226 | `getpixel16` | NEAR_MATCH |
| `1:3b390` | 1,511 | `07334` | 1,408 | `screencapture` | NEAR_MATCH |
| `1:3b977` | 271 | `078b4` | 271 | `inside` | STRICT |
| `1:3ba86` | 230 | `079c3` | 230 | `getangle` | STRICT |
| `1:3bb6c` | 60 | `07aa9` | 60 | `ksqrt` | STRICT |
| `1:3bba8` | 62 | `07ae5` | 62 | `calcksqrt` | STRICT |
| `1:3bbe6` | 325 | `07b23` | 325 | `copytilepiece` | STRICT |
| `1:3bd2b` | 688 | `07c68` | 688 | `printext` | STRICT |
| `1:3bfdb` | 839 | `07f18` | 826 | `drawmasks` | NEAR_MATCH |
| `1:3c322` | 1,890 | `08252` | 1,987 | `drawmaskwall` | NEAR_MATCH |
| `1:3ca84` | 9,875 | `08a15` | 10,176 | `drawsprite` | NEAR_MATCH |
| `1:3f117` | 402 | `0b1d5` | 402 | `ceilspritescan` | NEAR_MATCH |
| `1:3f2a9` | 976 | `0b367` | 1,044 | `ceilspritehline` | NEAR_MATCH |
| `1:3f679` | 1,063 | `0b77b` | 1,323 | `overwritesprite` | NEAR_MATCH |
| `1:3faa0` | 1,358 | `0bca6` | 1,358 | `permanentwritesprite` | NEAR_MATCH |
| `1:3ffee` | 2,169 | `0c1f4` | 2,337 | `grouvline` | NEAR_MATCH |
| `1:40867` | 98 | `0cb15` | 98 | `setsprite` | STRICT |
| `1:408c9` | 281 | `0cb77` | 281 | `movesprite` | STRICT |
| `1:409e2` | 144 | `0cc90` | 144 | `animateoffs` | STRICT |
| `1:40a72` | 206 | `0cd20` | 206 | `initspritelists` | NEAR_MATCH |
| `1:40b40` | 21 | `0cdee` | 21 | `insertsprite` | STRICT |
| `1:40b55` | 158 | `0ce03` | 158 | `insertspritesect` | NEAR_MATCH |
| `1:40bf3` | 156 | `0cea1` | 156 | `insertspritestat` | NEAR_MATCH |
| `1:40c8f` | 20 | `0cf3d` | 20 | `deletesprite` | STRICT |
| `1:40ca3` | 219 | `0cf51` | 219 | `deletespritesect` | NEAR_MATCH |
| `1:40d7e` | 217 | `0d02c` | 217 | `deletespritestat` | NEAR_MATCH |
| `1:40e57` | 73 | `0d105` | 73 | `changespritesect` | STRICT |
| `1:40ea0` | 73 | `0d14e` | 73 | `changespritestat` | STRICT |
| `1:40ee9` | 233 | `0d197` | 233 | `nextsectorneighborz` | STRICT |
| `1:40fd2` | 68 | `0d280` | 68 | `zerohandler` | STRICT |
| `1:41016` | 430 | `0d2c4` | 388 | `cansee` | NEAR_MATCH |
| `1:411c4` | 1,745 | `0d448` | 1,659 | `hitscan` | NEAR_MATCH |
| `1:41895` | 1,644 | `0dac3` | 1,583 | `neartag` | NEAR_MATCH |
| `1:41f01` | 545 | `0e0f2` | 548 | `lintersect` | NEAR_MATCH |
| `1:42122` | 252 | `0e316` | 252 | `dragpoint` | STRICT |
| `1:4221e` | 62 | `0e412` | 62 | `lastwall` | STRICT |
| `1:4225c` | 813 | `0e450` | 807 | `clipmove` | NEAR_MATCH |
| `1:42589` | 3,589 | `0e777` | 3,585 | `getcliplines` | NEAR_MATCH |
| `1:4338e` | 65 | `0f578` | 65 | `addclipline` | STRICT |
| `1:433cf` | 355 | `0f5b9` | 355 | `keepaway` | NEAR_MATCH |
| `1:43532` | 613 | `0f71c` | 524 | `raytrace` | NEAR_MATCH |
| `1:43797` | 202 | `0f928` | 202 | `updatesector` | STRICT |
| `1:43861` | 179 | `0f9f2` | 179 | `rotatepoint` | NEAR_MATCH |
| `1:43914` | 38 | `0faa5` | 38 | `getmousevalues` | STRICT |
| `1:4393a` | 3 | `0facb` | 3 | `printscreeninterrupt` | STRICT |
| `1:4393d` | 1,541 | `0face` | 1,171 | `drawline16` | NEAR_MATCH |
| `1:43f42` | 494 | `0ff61` | 272 | `qsetmode640480` | NEAR_MATCH |
| `1:44130` | 79 | `10071` | 79 | `clear2dscreen` | STRICT |
| `1:4417f` | 735 | `100c0` | 735 | `draw2dgrid` | NEAR_MATCH |
| `1:4445e` | 4,585 | `1039f` | 3,169 | `draw2dscreen` | NEAR_MATCH |
| `1:45647` | 32 | `11000` | 32 | `krand` | STRICT |
| `1:45667` | 2,938 | `11020` | 2,819 | `getzrange` | NEAR_MATCH |
| `1:461e1` | 229 | `11b23` | 229 | `makeradarang` | NEAR_MATCH |
| `1:462c6` | 334 | `11c08` | 336 | `setview` | NEAR_MATCH |
| `1:46414` | 186 | `11d58` | 186 | `precache` | STRICT |
| `1:464ce` | 1,938 | `11e12` | 1,517 | `wallmost` | NEAR_MATCH |
| `1:46c60` | 1,497 | `123ff` | 1,538 | `rotatesprite` | NEAR_MATCH |
| `1:47239` | 599 | `12a01` | 593 | `makepalookup` | NEAR_MATCH |
| `1:47490` | 292 | `12c52` | 292 | `setvesamode` | NEAR_MATCH |
| `1:475b4` | 229 | `12d76` | 229 | `stereoinit` | STRICT |
| `1:47699` | 551 | `12e5b` | 552 | `setverts` | NEAR_MATCH |
| `1:478c0` | 177 | `13083` | 177 | `stereoblit` | STRICT |
| `1:47971` | 57 | `13134` | 57 | `stereoredblueblit` | STRICT |
| `1:479aa` | 176 | `1316d` | 176 | `setredbluepalette` | NEAR_MATCH |
| `1:47a5a` | 101 | `1321d` | 101 | `setbrightness` | STRICT |

## Remaining first differences

Offsets refer to the first differing byte in the verifier report. A branch displacement can be the first difference in a function whose body also differs. Frame-size and register changes identify where code generation first diverges. In functions with known body changes, the underlying source discrepancy can affect allocation at entry.

| Function | First byte | Candidate / target bytes | Classification |
| --- | ---: | ---: | --- |
| `drawrooms` | `+0x271` | 928 / 928 | Layout first, then source and fixup-site differences in bunch traversal |
| `scansector` | `+0x10` | 747 / 756 | Source/body layout of terminal branch and scan loop |
| `wallfront` | `+0xa` | 549 / 554 | Register allocation of short wall indices and later comparison flow |
| `spritewallfront` | `+0x5` | 114 / 114 | Register allocation and load order |
| `calcwalls` | `+0x8` | 4,841 / 5,068 | Stack allocation, with source differences in split wall preparation |
| `drawalls` | `+0x8` | 3,240 / 3,811 | Stack allocation, with source differences in wall passes and occlusion saves |
| `addboxlinum` | `+0x19` | 1,022 / 1,024 | Register allocation and scheduling in initial wall projection |
| `wallscan` | `+0x5` | 3,144 / 2,853 | Stack allocation, with source differences in column setup and shared tail |
| `maskwallscan` | `+0x1e` | 2,887 / 2,917 | Layout first, then source differences in masked column loops |
| `loadboard` | `+0x20c` | 528 / 528 | Layout. Displacement to a shared epilogue |
| `saveboard` | `+0x1dd` | 481 / 481 | Layout. Displacement to a shared epilogue |
| `loadpalette` | `+0xe` | 510 / 510 | Layout. Displacement to a shared epilogue |
| `hline` | `+0x5` | 1,016 / 965 | Stack allocation, with projection and draw parameter scheduling |
| `transmaskvline` | `+0xf` | 626 / 636 | Source/body layout of terminal branch and clip bounds |
| `initengine` | `+0x20a` | 921 / 921 | Layout. Displacement to a shared epilogue |
| `uninitengine` | `+0xb4` | 219 / 219 | Layout. Displacement to a shared epilogue |
| `nextpage` | `+0x2e1` | 741 / 741 | Layout. Displacement to a shared epilogue |
| `initcache` | `+0x4b` | 79 / 79 | Layout. Displacement to a shared epilogue |
| `allocache` | `+0x184` | 411 / 411 | Layout. Displacement to a shared epilogue |
| `getpixel16` | `+0xde` | 226 / 226 | Layout. Displacement to a shared epilogue |
| `screencapture` | `+0x34` | 1,408 / 1,511 | Register and stack allocation of the filename and PCX state |
| `drawmasks` | `+0x18` | 826 / 839 | Source/body layout of sorting entry and sprite compaction |
| `drawmaskwall` | `+0x8` | 1,987 / 1,890 | Stack allocation, with source differences in occlusion clipping |
| `drawsprite` | `+0x5` | 10,176 / 9,875 | Stack allocation, with source differences in sprite projection and clipping |
| `ceilspritescan` | `+0x18e` | 402 / 402 | Layout. Displacement to a shared epilogue |
| `ceilspritehline` | `+0x6` | 1,044 / 976 | Stack allocation, with source differences in span setup and writers |
| `overwritesprite` | `+0x5` | 1,323 / 1,063 | Stack allocation, with source differences in column writers |
| `permanentwritesprite` | `+0x19` | 1,358 / 1,358 | Source/body layout of early clip branch and draw path |
| `grouvline` | `+0x8` | 2,337 / 2,169 | Register allocation at entry, with source differences in planar pixel loops |
| `initspritelists` | `+0xca` | 206 / 206 | Layout. Displacement to a shared epilogue |
| `insertspritesect` | `+0x1f` | 158 / 158 | Layout. Displacement to a shared epilogue |
| `insertspritestat` | `+0x1f` | 156 / 156 | Layout. Displacement to a shared epilogue |
| `deletespritesect` | `+0x21` | 219 / 219 | Layout. Displacement to a shared epilogue |
| `deletespritestat` | `+0x21` | 217 / 217 | Layout. Displacement to a shared epilogue |
| `cansee` | `+0x5` | 388 / 430 | Stack allocation, with source differences in sector traversal |
| `hitscan` | `+0x5` | 1,659 / 1,745 | Stack allocation, with source differences in ray and sprite intersection |
| `neartag` | `+0x5` | 1,583 / 1,644 | Stack allocation, with source differences in traversal and filtering |
| `lintersect` | `+0x5` | 548 / 545 | Stack allocation, with source differences in ray/segment intersection |
| `clipmove` | `+0x5` | 807 / 813 | Stack allocation, with source differences in clipping loop |
| `getcliplines` | `+0x5` | 3,585 / 3,589 | Stack allocation, with source differences in wall and sprite clipping |
| `keepaway` | `+0x108` | 355 / 355 | Layout. Displacement to a shared epilogue |
| `raytrace` | `+0x5` | 524 / 613 | Stack allocation, with source differences in line intersection |
| `rotatepoint` | `+0xaf` | 179 / 179 | Layout. Displacement to a shared epilogue |
| `drawline16` | `+0x5` | 1,171 / 1,541 | Stack allocation, with source differences in earlier VGA line algorithm |
| `qsetmode640480` | `+0x4` | 272 / 494 | Source change to video mode transitions and entry saves |
| `draw2dgrid` | `+0x2db` | 735 / 735 | Layout. Displacement to a shared epilogue |
| `draw2dscreen` | `+0x5` | 3,169 / 4,585 | Stack allocation, with source differences in 2D sprite and label drawing |
| `getzrange` | `+0x5` | 2,819 / 2,938 | Stack allocation, with source differences in neighboring sector traversal |
| `makeradarang` | `+0xe1` | 229 / 229 | Layout. Displacement to a shared epilogue |
| `setview` | `+0x3` | 336 / 334 | Register allocation of viewport arguments at entry |
| `wallmost` | `+0x6` | 1,517 / 1,938 | Stack allocation, with source differences in projected clipping |
| `rotatesprite` | `+0x5` | 1,538 / 1,497 | Stack allocation, with source differences in direct column clipping and draw path |
| `makepalookup` | `+0xb` | 593 / 599 | Stack allocation of color arguments and scheduling |
| `setvesamode` | `+0x120` | 292 / 292 | Layout. Displacement to a shared epilogue and unverified section-relative BSS references |
| `setverts` | `+0x222` | 552 / 551 | Layout. Explicit return in candidate, shared wallscan epilogue in oracle |
| `setredbluepalette` | `+0xac` | 176 / 176 | Layout. Displacement to a shared epilogue |
