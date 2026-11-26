# December 1994 Build engine, fifth pass

`src5/` starts with the integrated fourth pass. It uses the stock Watcom 9.5b engine profile `-4r -s -or`.

| Verifier status | Before functions | Before candidate bytes | After functions | After candidate bytes | After target bytes |
| --- | ---: | ---: | ---: | ---: | ---: |
| STRICT | 57 | 27,502 | 58 | 28,526 | 28,526 |
| CODE_BYTES_MATCH | 0 | 0 | 0 | 0 | 0 |
| NEAR_MATCH | 41 | 52,056 | 40 | 51,134 | 52,469 |
| UNPLACED | 0 | 0 | 0 | 0 | 0 |
| Total | 98 | 79,558 | 98 | 79,660 | 80,995 |

The new match is `addboxlinum`, 1,024 bytes at `1:36bec`. All 57 baseline STRICT functions held.

## Reproduce

```powershell
$p = 'build/workers/engine'
python -B tools/match.py --profile engine --symbols "$p/overlay.json" "-i=$p/src5" "$p/src5/ENGINE.C" --json "$p/results.json"
```

The fresh result used the stock DOS host and reported zero compile warnings or errors. The matcher exited 1 with the 40 NEAR results. All ten verifier unit tests passed. The overlay proposes eight data bindings with their LE instruction evidence. The integration gate was green for the supplied baseline.

## Code extent and displacement dependencies

**STRONG** ENGINE.OBJ occupies `[1:33e5c,1:47abf)`, 80,995 bytes across 98 publics. The keyboard handler starts at `1:33e5c`. `setbrightness` returns at `1:47abe`, before MULTI's dispatch table. The 1994-09-15 changelog records the split. Prior analysis placed the disputed 677-byte block inside `rotatesprite`.

**PROVEN** `calcwalls` grew from 4,970 to 5,068 bytes, its full oracle extent. All relocation sites and targets match. Ten instruction bytes differ across two local stack homes. At `+0x20d`, the wall texture numerator step is stored at `[esp+0xc]` and the oracle uses `[esp+0x10]`. The resolved sky tile used across a `wallscan` call occupies the other home. Each appears consistently in the intended body. Trying explicit local scopes, equivalent integer widths, and expression staging regressed these allocations. The selected picking-store order removed the eight other byte differences and four fixup problems in the 18-byte-diff control.

**PROVEN** The restored `calcwalls` extent closes both shared-return jumps in `addboxlinum`. Changes to `scansector` and `wallfront` retain the target switch-table alignment. Their 748- and 562-byte candidate spans respectively are eight bytes short and eight long. Their combined span is exactly 1,310 bytes. The selected `wallfront` product order has more matching normalized instructions than the integrated version. A 554-byte pointer variant also yielded 58 STRICT but left the subsequent run eight bytes earlier than its target offset. At base `1:33e5c`, the selected source places all 37 publics from `calcwalls` through `drawmaskwall` at their oracle offsets. Whole-object placement remains open.

**OPEN** Thirteen NEAR extents covering 3,875 bytes have their code differences confined to cross-function jump operands. `setvesamode` also has unresolved anonymous BSS references. Correcting `qinterpolate` in `drawmaskwall` and `drawsprite` accounts for the six-byte change in several downstream displacements. The five sprite-list functions still have the same ?1,040-byte displacement error to `drawmasks+341`. The largest contributors are `drawsprite`, `overwritesprite`, `grouvline`, and `ceilspritehline`. The jump in `ceilspritescan` reaches back across the recently expanded `calcwalls`, plus the subsequent sprite drawing code.

| Affected functions | Oracle return path | Candidate displacement minus oracle |
| --- | --- | ---: |
| `ceilspritescan` | `wallfront+224` | ?544 |
| `initspritelists`, `insertspritesect`, `insertspritestat`, `deletespritesect`, `deletespritestat` | `drawmasks+341` | ?1,040 |
| `keepaway` | `ceilscan+4b1` | ?844 |
| `rotatepoint` | `clipinsideboxline+171` | ?755 |
| `draw2dgrid` | `printext+2aa` | ?163 |
| `makeradarang` | `drawmasks+33d` | +1,372 |
| `setvesamode`, `setredbluepalette` | `drawmasks+341` | +1,335 |
| `setverts` | `wallscan+b1f` | +1,335 |

## Source evidence and controls

| Date or target evidence | Reconstruction |
| --- | --- |
| 1994-11-23 tiled skies, 1994-12-28 drawing and palettes, and `1:35052-35682` | `calcwalls` compares the resolved sky tile IDs at segment endpoints, including when two sky positions use the same tile. Both sky passes traverse the `p2` chain to the -1 sentinel. The type-2 sky uses `divscale(16384L,(long)sintable[...],16)`. The direct signed lookup and shift/divide sequence are visible at `1:35381-3539f`. The ceiling orientation assignment uses the earlier source order. |
| December `1:34c84-34cb9`, `1:34da6-34fdd` | The right edge compares the shifted repeat against the live last column. Picking selects ceiling, floor and solid walls in order. On portals it can test both edges, then tests a one-way wall only while no hit was selected. Wall, status and hit stores follow the oracle order. The changelog does not date these implementation details. |
| 1994-04-22 interpolation precedent and `1:34d8c-34d9a`, `1:3c6bd-3c6cb` | Plain dword `qinterpolate` uses `jecxz` and `loop`, now restored in `PRAGMAS.H`. The same template applies to the existing calls in `drawmaskwall` and `drawsprite`. With the earlier intermediate layout this change perturbed four existing strict spans. The selected source restores their alignment. |
| 1994-11-09 `gotsector`, and target `1:34334-34358`, `1:34582-3459f` | `scansector` takes both copies of the scan count from the same value after reading the wall range. It uses the positive ceiling-bit test used in the sprite palette path. `wallfront` uses ordinary two-endpoint products with the original subtraction operand order. Both remain NEAR. |
| 1994-10-06 map-v5 `BUILD.H`, 1994-11-09 `gotsector` | Restore tagged structs without the four typedef aliases. Keep `gotsector` beside `gotpic`, both discovery bitsets. The rule and oracle determine the symbol page, with several placements inside that page giving the same offsets. |

## Bounded register-allocation permuter

`permuter/` adapts the existing Rayman 10.6 source mutator to the pinned 9.5b preprocessor, CP437/CRLF and the serial compiler wrapper. Existing target-body, round-trip, macro, data shape and regression checks remain active. The scorer only prioritized variants. The ordinary verifier decided status. The pilot used seed 942, conservative mutations, a maximum of 20 compiler invocations and 32 seconds of admission time per run. Runs ending on the timer let their last compile finish.

| Function | Compiler invocations | Variants compiled | Ineligible | Best admissible byte diffs | Accepted shape |
| --- | ---: | ---: | ---: | ---: | --- |
| `spritewallfront` | 20 | 18 | 0 | 7 ? 7 | Baseline |
| `setview` | 20 | 18 | 0 | 221 ? 221 | Baseline |
| `makepalookup` | 2 | 0 | 0 | Guard refused `unsigned` | Baseline |
| `lintersect` | 9 | 7 | 0 | 339 ? 339 | Baseline |
| `calcwalls` | 4 | 2 | 2 | 18 ? 18 | Baseline |

The pilots on `spritewallfront`, `setview`, `lintersect`, and `makepalookup` used the integrated source. The `calcwalls` pilot used the 18-byte-diff source before the final picking-store correction. The stock wrapper switched from native Windows to the canonical DOS host during this pass, shortening the number of compiles that fit the later time budgets. The `makepalookup` refusal came from a residual keyword shared with a dormant SDK definition. The program retained that guard. Declaration permutations produced only one diagnostic score for all 18 `spritewallfront` variants. This pilot found no compiler-steered shape to adopt. The manually reconstructed `calcwalls` changes are ordinary rendering expressions and branches.

## Data ownership and order

- **PROVEN** All 75 `CONST` bytes at `2:1184` and all 237 `_DATA` bytes at `2:2c00` verify. The candidate BSS size is 1,097,937 bytes, which agrees with the inferred span from `2:b1e00` to `2:1bded1`.
- **PROVEN** With packed alignment and measured sizes, `bssrule.predict` reproduces the current candidate's 203 BSS starts from its preprocessed ordered declarations. The initial slot is 18. The pre-keyhandler declaration stream has 577 distinct named entries after removing four aliases, down from 581. A modeled four-slot shift alone predicted 172 of the then 187 oracle bindings at the correct offsets. Moving `gotsector` into the later page predicted all 187, confirmed by an independent stock compile. The baseline had 54 agreeing addresses and 41 implied bases.
- **PROVEN** Eight new floor-sprite data bindings follow the oracle's first vertex stores, copies between the two clipping buffers, and screen X/Y projection. The 195 independently bound BSS publics now agree on base `2:b1e00`. The recorded evidence for the eight bindings is in `overlay.json`.
- **OPEN** The verifier still reports eight unbound BSS publics. The projected offsets get no exact-address fixup votes. The candidate contains the three start-position longs, unused zero-handler vector, two unused shorts, start angle and start sector there. The 50-byte anonymous RMI follows the byte tail. Its size and oracle address are independently suggested by the VESA call, but the matcher requires verified BSS references, and anonymous RMI references still carry that debt.

## Target function order

Each target extent ends at the next row. The final extent ends at `1:47abf`. The switch tables before `initengine` and `nextpage` belong to the preceding public extents.

| Target start | Target bytes | Candidate offset | Candidate bytes | Function | Status |
| --- | ---: | ---: | ---: | --- | --- |
| `1:33e5c` | 188 | `00000` | 188 | `keyhandler` | STRICT |
| `1:33f18` | 928 | `000bc` | 928 | `drawrooms` | NEAR_MATCH |
| `1:342b8` | 756 | `0045c` | 748 | `scansector` | NEAR_MATCH |
| `1:345ac` | 554 | `00748` | 562 | `wallfront` | NEAR_MATCH |
| `1:347d6` | 114 | `0097a` | 114 | `spritewallfront` | NEAR_MATCH |
| `1:34848` | 245 | `009ec` | 245 | `bunchfront` | STRICT |
| `1:3493d` | 5,068 | `00ae1` | 5,068 | `calcwalls` | NEAR_MATCH |
| `1:35d09` | 3,811 | `01ead` | 3,811 | `drawalls` | STRICT |
| `1:36bec` | 1,024 | `02d90` | 1,024 | `addboxlinum` | STRICT |
| `1:36fec` | 1,209 | `03190` | 1,209 | `ceilscan` | STRICT |
| `1:374a5` | 1,226 | `03649` | 1,226 | `florscan` | STRICT |
| `1:3796f` | 2,853 | `03b13` | 2,853 | `wallscan` | STRICT |
| `1:38494` | 2,917 | `04638` | 2,917 | `maskwallscan` | STRICT |
| `1:38ff9` | 302 | `0519d` | 302 | `transmaskwallscan` | STRICT |
| `1:39127` | 528 | `052cb` | 528 | `loadboard` | STRICT |
| `1:39337` | 481 | `054db` | 481 | `saveboard` | STRICT |
| `1:39518` | 303 | `056bc` | 303 | `loadtables` | STRICT |
| `1:39647` | 510 | `057eb` | 510 | `loadpalette` | STRICT |
| `1:39845` | 570 | `059e9` | 570 | `setgamemode` | STRICT |
| `1:39a7f` | 965 | `05c23` | 965 | `hline` | NEAR_MATCH |
| `1:39e44` | 636 | `05fe8` | 636 | `transmaskvline` | STRICT |
| `1:3a0c0` | 921 | `06264` | 921 | `initengine` | STRICT |
| `1:3a459` | 219 | `065fd` | 219 | `uninitengine` | STRICT |
| `1:3a534` | 741 | `066d8` | 741 | `nextpage` | STRICT |
| `1:3a819` | 79 | `069bd` | 79 | `initcache` | STRICT |
| `1:3a868` | 411 | `06a0c` | 411 | `allocache` | STRICT |
| `1:3aa03` | 198 | `06ba7` | 198 | `allocatepermanenttile` | STRICT |
| `1:3aac9` | 258 | `06c6d` | 258 | `loadtile` | STRICT |
| `1:3abcb` | 773 | `06d6f` | 773 | `loadpics` | STRICT |
| `1:3aed0` | 122 | `07074` | 122 | `showengineinfo` | STRICT |
| `1:3af4a` | 23 | `070ee` | 23 | `resettiming` | STRICT |
| `1:3af61` | 470 | `07105` | 470 | `clipinsidebox` | STRICT |
| `1:3b137` | 375 | `072db` | 375 | `clipinsideboxline` | STRICT |
| `1:3b2ae` | 226 | `07452` | 226 | `getpixel16` | STRICT |
| `1:3b390` | 1,511 | `07534` | 1,511 | `screencapture` | NEAR_MATCH |
| `1:3b977` | 271 | `07b1b` | 271 | `inside` | STRICT |
| `1:3ba86` | 230 | `07c2a` | 230 | `getangle` | STRICT |
| `1:3bb6c` | 60 | `07d10` | 60 | `ksqrt` | STRICT |
| `1:3bba8` | 62 | `07d4c` | 62 | `calcksqrt` | STRICT |
| `1:3bbe6` | 325 | `07d8a` | 325 | `copytilepiece` | STRICT |
| `1:3bd2b` | 688 | `07ecf` | 688 | `printext` | STRICT |
| `1:3bfdb` | 839 | `0817f` | 839 | `drawmasks` | STRICT |
| `1:3c322` | 1,890 | `084c6` | 1,887 | `drawmaskwall` | NEAR_MATCH |
| `1:3ca84` | 9,875 | `08c25` | 10,422 | `drawsprite` | NEAR_MATCH |
| `1:3f117` | 402 | `0b4db` | 402 | `ceilspritescan` | NEAR_MATCH |
| `1:3f2a9` | 976 | `0b66d` | 1,044 | `ceilspritehline` | NEAR_MATCH |
| `1:3f679` | 1,063 | `0ba81` | 1,323 | `overwritesprite` | NEAR_MATCH |
| `1:3faa0` | 1,358 | `0bfac` | 1,358 | `permanentwritesprite` | NEAR_MATCH |
| `1:3ffee` | 2,169 | `0c4fa` | 2,337 | `grouvline` | NEAR_MATCH |
| `1:40867` | 98 | `0ce1b` | 98 | `setsprite` | STRICT |
| `1:408c9` | 281 | `0ce7d` | 281 | `movesprite` | STRICT |
| `1:409e2` | 144 | `0cf96` | 144 | `animateoffs` | STRICT |
| `1:40a72` | 206 | `0d026` | 206 | `initspritelists` | NEAR_MATCH |
| `1:40b40` | 21 | `0d0f4` | 21 | `insertsprite` | STRICT |
| `1:40b55` | 158 | `0d109` | 158 | `insertspritesect` | NEAR_MATCH |
| `1:40bf3` | 156 | `0d1a7` | 156 | `insertspritestat` | NEAR_MATCH |
| `1:40c8f` | 20 | `0d243` | 20 | `deletesprite` | STRICT |
| `1:40ca3` | 219 | `0d257` | 219 | `deletespritesect` | NEAR_MATCH |
| `1:40d7e` | 217 | `0d332` | 217 | `deletespritestat` | NEAR_MATCH |
| `1:40e57` | 73 | `0d40b` | 73 | `changespritesect` | STRICT |
| `1:40ea0` | 73 | `0d454` | 73 | `changespritestat` | STRICT |
| `1:40ee9` | 233 | `0d49d` | 233 | `nextsectorneighborz` | STRICT |
| `1:40fd2` | 68 | `0d586` | 68 | `zerohandler` | STRICT |
| `1:41016` | 430 | `0d5ca` | 388 | `cansee` | NEAR_MATCH |
| `1:411c4` | 1,745 | `0d74e` | 1,659 | `hitscan` | NEAR_MATCH |
| `1:41895` | 1,644 | `0ddc9` | 1,583 | `neartag` | NEAR_MATCH |
| `1:41f01` | 545 | `0e3f8` | 548 | `lintersect` | NEAR_MATCH |
| `1:42122` | 252 | `0e61c` | 252 | `dragpoint` | STRICT |
| `1:4221e` | 62 | `0e718` | 62 | `lastwall` | STRICT |
| `1:4225c` | 813 | `0e756` | 807 | `clipmove` | NEAR_MATCH |
| `1:42589` | 3,589 | `0ea7d` | 3,585 | `getcliplines` | NEAR_MATCH |
| `1:4338e` | 65 | `0f87e` | 65 | `addclipline` | STRICT |
| `1:433cf` | 355 | `0f8bf` | 355 | `keepaway` | NEAR_MATCH |
| `1:43532` | 613 | `0fa22` | 524 | `raytrace` | NEAR_MATCH |
| `1:43797` | 202 | `0fc2e` | 202 | `updatesector` | STRICT |
| `1:43861` | 179 | `0fcf8` | 179 | `rotatepoint` | NEAR_MATCH |
| `1:43914` | 38 | `0fdab` | 38 | `getmousevalues` | STRICT |
| `1:4393a` | 3 | `0fdd1` | 3 | `printscreeninterrupt` | STRICT |
| `1:4393d` | 1,541 | `0fdd4` | 1,171 | `drawline16` | NEAR_MATCH |
| `1:43f42` | 494 | `10267` | 272 | `qsetmode640480` | NEAR_MATCH |
| `1:44130` | 79 | `10377` | 79 | `clear2dscreen` | STRICT |
| `1:4417f` | 735 | `103c6` | 735 | `draw2dgrid` | NEAR_MATCH |
| `1:4445e` | 4,585 | `106a5` | 3,169 | `draw2dscreen` | NEAR_MATCH |
| `1:45647` | 32 | `11306` | 32 | `krand` | STRICT |
| `1:45667` | 2,938 | `11326` | 2,819 | `getzrange` | NEAR_MATCH |
| `1:461e1` | 229 | `11e29` | 229 | `makeradarang` | NEAR_MATCH |
| `1:462c6` | 334 | `11f0e` | 336 | `setview` | NEAR_MATCH |
| `1:46414` | 186 | `1205e` | 186 | `precache` | STRICT |
| `1:464ce` | 1,938 | `12118` | 1,938 | `wallmost` | STRICT |
| `1:46c60` | 1,497 | `128aa` | 1,538 | `rotatesprite` | NEAR_MATCH |
| `1:47239` | 599 | `12eac` | 593 | `makepalookup` | NEAR_MATCH |
| `1:47490` | 292 | `130fd` | 292 | `setvesamode` | NEAR_MATCH |
| `1:475b4` | 229 | `13221` | 229 | `stereoinit` | STRICT |
| `1:47699` | 551 | `13306` | 551 | `setverts` | NEAR_MATCH |
| `1:478c0` | 177 | `1352d` | 177 | `stereoblit` | STRICT |
| `1:47971` | 57 | `135de` | 57 | `stereoredblueblit` | STRICT |
| `1:479aa` | 176 | `13617` | 176 | `setredbluepalette` | NEAR_MATCH |
| `1:47a5a` | 101 | `136c7` | 101 | `setbrightness` | STRICT |

## Remaining first differences

The offsets come from the fresh matcher. Layout entries describe the first differing jump operand. Allocation and source rows record the first mismatch and any established body differences.

| Function | First byte | Candidate / target bytes | Classification |
| --- | ---: | ---: | --- |
| `drawrooms` | `+0x283` | 928 / 928 | Layout first, then source and fixup-site differences in bunch traversal |
| `scansector` | `+0x10` | 748 / 756 | Layout first at the initial return branch, followed by allocation and traversal shape |
| `wallfront` | `+0x27` | 562 / 554 | Register allocation in wall endpoint loads and the cross products |
| `spritewallfront` | `+0x1d` | 114 / 114 | Register allocation and load order |
| `calcwalls` | `+0x20d` | 5,068 / 5,068 | Stack allocation. The projection step and sky tile temporary use exchanged stack homes |
| `hline` | `+0x5` | 965 / 965 | Register and stack allocation in the prologue, projection and raster arguments |
| `screencapture` | `+0x34` | 1,511 / 1,511 | Stack slot for the inverse flag. Register scheduling and palette loop still differ across the full extent |
| `drawmaskwall` | `+0xc2` | 1,887 / 1,890 | Instruction selection and register allocation at the first xrepeat load. The historical dword interpolation loop is restored |
| `drawsprite` | `+0x5` | 10,422 / 9,875 | Stack allocation. Early offset and division forms are restored. Projection and clipping shapes still differ |
| `ceilspritescan` | `+0x18e` | 402 / 402 | Layout. Displacement to `wallfront+224` |
| `ceilspritehline` | `+0x6` | 1,044 / 976 | Stack allocation, with source differences in span setup and writers |
| `overwritesprite` | `+0x5` | 1,323 / 1,063 | Stack allocation, with source differences in column writers |
| `permanentwritesprite` | `+0x19` | 1,358 / 1,358 | Source/body layout of early clip branch and draw path |
| `grouvline` | `+0x8` | 2,337 / 2,169 | Register allocation at entry, with source differences in planar pixel loops |
| `initspritelists` | `+0xca` | 206 / 206 | Layout. Displacement to `drawmasks+341` |
| `insertspritesect` | `+0x1f` | 158 / 158 | Layout. Displacement to `drawmasks+341` |
| `insertspritestat` | `+0x1f` | 156 / 156 | Layout. Displacement to `drawmasks+341` |
| `deletespritesect` | `+0x21` | 219 / 219 | Layout. Displacement to `drawmasks+341` |
| `deletespritestat` | `+0x21` | 217 / 217 | Layout. Displacement to `drawmasks+341` |
| `cansee` | `+0x5` | 388 / 430 | Stack allocation, with source differences in sector traversal |
| `hitscan` | `+0x5` | 1,659 / 1,745 | Stack allocation, with source differences in ray and sprite intersection |
| `neartag` | `+0x5` | 1,583 / 1,644 | Stack allocation, with source differences in traversal and filtering |
| `lintersect` | `+0x5` | 548 / 545 | Stack allocation, with source differences in ray/segment intersection |
| `clipmove` | `+0x5` | 807 / 813 | Stack allocation, with source differences in clipping loop |
| `getcliplines` | `+0x5` | 3,585 / 3,589 | Stack allocation, with source differences in wall and sprite clipping |
| `keepaway` | `+0x108` | 355 / 355 | Layout. Displacement to `ceilscan+4b1` |
| `raytrace` | `+0x5` | 524 / 613 | Stack allocation, with source differences in line intersection |
| `rotatepoint` | `+0xaf` | 179 / 179 | Layout. Displacement to `clipinsideboxline+171` |
| `drawline16` | `+0x5` | 1,171 / 1,541 | Stack allocation, with source differences in earlier VGA line algorithm |
| `qsetmode640480` | `+0x4` | 272 / 494 | Source change to video mode transitions and entry saves |
| `draw2dgrid` | `+0x2db` | 735 / 735 | Layout. Displacement to `printext+2aa` |
| `draw2dscreen` | `+0x5` | 3,169 / 4,585 | Stack allocation, with source differences in 2D sprite and label drawing |
| `getzrange` | `+0x5` | 2,819 / 2,938 | Stack allocation, with source differences in neighboring sector traversal |
| `makeradarang` | `+0xe1` | 229 / 229 | Layout. Displacement to `drawmasks+33d` |
| `setview` | `+0x3` | 336 / 334 | Register allocation of viewport arguments at entry |
| `rotatesprite` | `+0x5` | 1,538 / 1,497 | Stack allocation, with source differences in direct column clipping and draw path |
| `makepalookup` | `+0xb` | 593 / 599 | Stack allocation of color arguments and scheduling |
| `setvesamode` | `+0x120` | 292 / 292 | Layout. Displacement to `drawmasks+341` and unverified section-relative BSS operands |
| `setverts` | `+0x223` | 551 / 551 | Layout. Displacement to `wallscan+b1f` |
| `setredbluepalette` | `+0xac` | 176 / 176 | Layout. Displacement to `drawmasks+341` |

## Earlier source evidence retained

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

