# December 1994 Build engine, fourth pass

The candidate is in `src4/`. It started from the integrated third pass and uses the stock staged Watcom 9.5b profile `-4r -s -or`.

| Verifier status | Before functions | Before candidate bytes | After functions | After candidate bytes | After target bytes |
| --- | ---: | ---: | ---: | ---: | ---: |
| STRICT | 53 | 24,339 | 57 | 27,502 | 27,502 |
| CODE_BYTES_MATCH | 0 | 0 | 0 | 0 | 0 |
| NEAR_MATCH | 45 | 54,915 | 41 | 52,056 | 53,493 |
| UNPLACED | 0 | 0 | 0 | 0 | 0 |
| Total | 98 | 79,254 | 98 | 79,558 | 80,995 |

All 53 baseline matches are preserved. Four new matches add 3,163 STRICT bytes.

| New STRICT function | Target start | Bytes |
| --- | --- | ---: |
| `loadpalette` | `1:39647` | 510 |
| `transmaskvline` | `1:39e44` | 636 |
| `initcache` | `1:3a819` | 79 |
| `wallmost` | `1:464ce` | 1,938 |

## Reproduce

```powershell
$p = 'build/workers/engine'
python -B tools/match.py --profile engine --symbols "$p/overlay.json" "-i=$p/src4" "$p/src4/ENGINE.C" --json "$p/results.json"
```

The stock compiler ran with zero warnings and errors. The matcher exited 1 with 41 NEAR results. The verifier's ten unit tests passed. `overlay.json` proposes five data bindings, with their target instructions and site counts. The integrated source and canonical bindings were used read-only.

## Extent and shared returns

**STRONG** ENGINE.OBJ occupies `[1:33e5c,1:47abf)`, 80,995 bytes. The keyboard interrupt handler begins there, `setbrightness` returns at `1:47abe`, and MULTI's six-entry dispatch table starts at `1:47abf`. The 1994-09-15 changelog records the ENGINE/MULTI split. The 98-public sequence is unchanged. The 677-byte block previously considered for `clippoly4` was resolved in pass two as part of `rotatesprite`. The first code-placement drift is still the nine-byte deficit in `scansector`.

**PROVEN** Staging the projected wall coordinates and their stores in target order reduced `addboxlinum` from 1,029 to 1,024 bytes. Its two body differences are displacements at `+0x1cf` and `+0x3fc` to the same shared return in `wallfront`. The candidate displacements are each 98 higher. `calcwalls`, in between, is 98 bytes short. The earlier `wallfront` public is one byte short and that shared return is one byte earlier within its public.

**PROVEN** `loadpalette` and `initcache` now reach their target return paths in `drawmasks` and `setgamemode`. The `hline` span is 965 bytes and `transmaskvline` now owns its original 603-byte body, one-byte alignment NOP at `1:3a09f` and 32-byte switch table at `1:3a0a0`. Across `addboxlinum`, `hline` and `transmaskvline`, the changes from the integrated object are -5, +4 and +1 bytes. The combined span retains the verified distance from `drawalls` to the shared `drawmasks` return. Absolute `_TEXT` bases still conflict.

**OPEN** Fourteen NEAR extents totaling 4,899 bytes have their code differences confined to cross-function jump operands. `setvesamode` additionally has unresolved section-relative RMI fixups. The five sprite-list functions share a -1,046-byte displacement error to `drawmasks+341`. Their intervening code includes `drawsprite` (10,425 candidate bytes, 9,875 target), `grouvline` (2,337 against 2,169), `overwritesprite` (1,323 against 1,063), and `ceilspritehline` (1,044 against 976). The largest remaining span shortfall is `draw2dscreen` (3,169 against 4,585).

## Source changes and controls

Dates below are changelog entries or archive dates. The December disassembly establishes implementation details without an individual changelog entry.

| Evidence | Reconstruction |
| --- | --- |
| December `addboxlinum` at `1:36cf2-36dbf` and `1:36f9c-36fe2` | Project horizontal and depth components together for each endpoint. Test facing before rejecting the two nonpositive depths. Store the projected depths, sector, wall and horizontal coordinates in the observed order. |
| 1994-04-22 `ENGINE.C` horizontal rasterizer, December `1:39ade-39b57` | The December distance and projection use the April two-product form, adjusted for dynamic screen width and global position. Selected independent X staging and a combined Y expression restore the extent. This candidate still chooses different stack slots and registers. The alternative 963-byte candidate has closer normalized instructions through the rest of the body. It moves the `transmaskvline` entry by two bytes and changes its switch-table padding from one to three bytes. |
| December `wallfront` and `spritewallfront` | Reuse the decoded wall and point2 indices. Evaluate the corresponding Y product first for the wall cross product. The sprite test uses the two endpoint and sprite deltas directly. Local pointer aliases, short wall indices and different product orders left allocation differences. The selected `wallfront` extent is 553 bytes, within one byte of the target. |
| 1994-11-23 tiled parallax sky entry and December `calcwalls` at `1:34b24-34c57` | Cache wall X repeat for the projection numerator, reread the horizontal span from `xb1/xb2`, and derive odd columns using the neighboring dword samples. This region is closer to the oracle. The sky path remains open. Two independent sky-array bindings follow the November feature's tile and scale operands. |
| 1994-12-28 wall/sprite palette proposal, 1995-01-01 map v6, 1995-01-03 column-width change, December `drawmaskwall` | Recover the unscaled reciprocal depths and scale the initial depth and step for nondefault widths. The wall-front test compares a wall index with the returned foreground index. Closed spans assign dword clip pairs in loops. The 1,890-byte extent is recovered. Alternate X-repeat staging and shift comparisons changed allocation without matching the whole function. |
| 1994-04-22 inline interpolation precedent, 1995-01-03 clip-column change, December `wallmost` | Compare against top and bottom screen extents relative to the horizon, clip at `ytop/ybot`, and divide the original Z by each endpoint depth before applying the viewport scale. The dword `qinterpolatedown16` uses `jecxz` and `loop` at all five target sites. The two uninitialized clipping globals disappear. |
| 1994-10-06 and June 1995 `BUILD.H` | The older header has no `vidoption`. The June header defines it, and the integrated December source has no users. Removing that byte aligns the complete named byte tail with the oracle. |

The target also has the earlier `jecxz`/`loop` form of plain `qinterpolate` at `1:34d8c` in `calcwalls` and `1:3c6bd` in `drawmaskwall`. A stock compile with that form changed `calcwalls` by three bytes and shifted the switch-table padding. It regressed `drawalls`, `loadpalette` and `initcache`. The plain helper retains the integrated form until the upstream placement is resolved. The dword-output `qinterpolatedown16` used by `wallmost` retains its recovered form.

## Data ownership and order

- **PROVEN initialized spans.** The stock compiler with fixed staging verifies all 75 `CONST` bytes at `2:1184` and all 237 `_DATA` bytes at `2:2c00`.
- **STRONG uninitialized extent.** The candidate emits 1,097,937 BSS bytes, down nine from the integrated baseline. That equals the span inferred from `localtilestart` at `2:b1e00` to the exclusive end of the 50-byte anonymous real-mode register block at `2:1bded1`. The RMI starts at `2:1bde9f`, the byte immediately following `visibility`. Its position agrees with the candidate at relative `0x10c09f`. The next contribution's ownership remains open.
- **PROVEN named tail placement against that base.** The ten one-byte globals from `oldreadch` at `2:1bde95` through `visibility` at `2:1bde9e` all agree. The stock `bssrule.predict` on the same-page byte-symbol slice emits `automapping, vidoption, parallaxtype, visibility` at slice offsets 0, 1, 2, 3 (hashes 71, 94, 101, 185). Removing the unused declaration predicts the latter two offsets moving back by one, as observed in the OMF. This small slice tests the local ordering without assigning pages to the entire engine.
- **OPEN full order.** There are 203 named BSS publics. The 187 bound ones imply 41 candidate bases. Fifty-four agree with `2:b1e00`. With the stock `size_bucket` and `name_hash`, the target run from `clipobjectval` (bucket 6, hash 31) to `hitwalls` (bucket 6, hash 186) precedes `lastx` (bucket 6, hash 1), requiring a symbol-page break. The candidate puts `lastx` and `palette` ahead of the clip run. The historical first-declaration stream remains unresolved. Retain the distinction between total length, named placements, and the matcher's sectional verification.

## Target function order

Each target extent ends at the next row. The final extent ends at `1:47abf`. The switch tables before `initengine` and `nextpage` belong to the preceding public extents.

| Target start | Target bytes | Candidate offset | Candidate bytes | Function | Status |
| --- | ---: | ---: | ---: | --- | --- |
| `1:33e5c` | 188 | `00000` | 188 | `keyhandler` | STRICT |
| `1:33f18` | 928 | `000bc` | 928 | `drawrooms` | NEAR_MATCH |
| `1:342b8` | 756 | `0045c` | 747 | `scansector` | NEAR_MATCH |
| `1:345ac` | 554 | `00747` | 553 | `wallfront` | NEAR_MATCH |
| `1:347d6` | 114 | `00970` | 114 | `spritewallfront` | NEAR_MATCH |
| `1:34848` | 245 | `009e2` | 245 | `bunchfront` | STRICT |
| `1:3493d` | 5,068 | `00ad7` | 4,970 | `calcwalls` | NEAR_MATCH |
| `1:35d09` | 3,811 | `01e41` | 3,811 | `drawalls` | STRICT |
| `1:36bec` | 1,024 | `02d24` | 1,024 | `addboxlinum` | NEAR_MATCH |
| `1:36fec` | 1,209 | `03124` | 1,209 | `ceilscan` | STRICT |
| `1:374a5` | 1,226 | `035dd` | 1,226 | `florscan` | STRICT |
| `1:3796f` | 2,853 | `03aa7` | 2,853 | `wallscan` | STRICT |
| `1:38494` | 2,917 | `045cc` | 2,917 | `maskwallscan` | STRICT |
| `1:38ff9` | 302 | `05131` | 302 | `transmaskwallscan` | STRICT |
| `1:39127` | 528 | `0525f` | 528 | `loadboard` | STRICT |
| `1:39337` | 481 | `0546f` | 481 | `saveboard` | STRICT |
| `1:39518` | 303 | `05650` | 303 | `loadtables` | STRICT |
| `1:39647` | 510 | `0577f` | 510 | `loadpalette` | STRICT |
| `1:39845` | 570 | `0597d` | 570 | `setgamemode` | STRICT |
| `1:39a7f` | 965 | `05bb7` | 965 | `hline` | NEAR_MATCH |
| `1:39e44` | 636 | `05f7c` | 636 | `transmaskvline` | STRICT |
| `1:3a0c0` | 921 | `061f8` | 921 | `initengine` | STRICT |
| `1:3a459` | 219 | `06591` | 219 | `uninitengine` | STRICT |
| `1:3a534` | 741 | `0666c` | 741 | `nextpage` | STRICT |
| `1:3a819` | 79 | `06951` | 79 | `initcache` | STRICT |
| `1:3a868` | 411 | `069a0` | 411 | `allocache` | STRICT |
| `1:3aa03` | 198 | `06b3b` | 198 | `allocatepermanenttile` | STRICT |
| `1:3aac9` | 258 | `06c01` | 258 | `loadtile` | STRICT |
| `1:3abcb` | 773 | `06d03` | 773 | `loadpics` | STRICT |
| `1:3aed0` | 122 | `07008` | 122 | `showengineinfo` | STRICT |
| `1:3af4a` | 23 | `07082` | 23 | `resettiming` | STRICT |
| `1:3af61` | 470 | `07099` | 470 | `clipinsidebox` | STRICT |
| `1:3b137` | 375 | `0726f` | 375 | `clipinsideboxline` | STRICT |
| `1:3b2ae` | 226 | `073e6` | 226 | `getpixel16` | STRICT |
| `1:3b390` | 1,511 | `074c8` | 1,511 | `screencapture` | NEAR_MATCH |
| `1:3b977` | 271 | `07aaf` | 271 | `inside` | STRICT |
| `1:3ba86` | 230 | `07bbe` | 230 | `getangle` | STRICT |
| `1:3bb6c` | 60 | `07ca4` | 60 | `ksqrt` | STRICT |
| `1:3bba8` | 62 | `07ce0` | 62 | `calcksqrt` | STRICT |
| `1:3bbe6` | 325 | `07d1e` | 325 | `copytilepiece` | STRICT |
| `1:3bd2b` | 688 | `07e63` | 688 | `printext` | STRICT |
| `1:3bfdb` | 839 | `08113` | 839 | `drawmasks` | STRICT |
| `1:3c322` | 1,890 | `0845a` | 1,890 | `drawmaskwall` | NEAR_MATCH |
| `1:3ca84` | 9,875 | `08bbc` | 10,425 | `drawsprite` | NEAR_MATCH |
| `1:3f117` | 402 | `0b475` | 402 | `ceilspritescan` | NEAR_MATCH |
| `1:3f2a9` | 976 | `0b607` | 1,044 | `ceilspritehline` | NEAR_MATCH |
| `1:3f679` | 1,063 | `0ba1b` | 1,323 | `overwritesprite` | NEAR_MATCH |
| `1:3faa0` | 1,358 | `0bf46` | 1,358 | `permanentwritesprite` | NEAR_MATCH |
| `1:3ffee` | 2,169 | `0c494` | 2,337 | `grouvline` | NEAR_MATCH |
| `1:40867` | 98 | `0cdb5` | 98 | `setsprite` | STRICT |
| `1:408c9` | 281 | `0ce17` | 281 | `movesprite` | STRICT |
| `1:409e2` | 144 | `0cf30` | 144 | `animateoffs` | STRICT |
| `1:40a72` | 206 | `0cfc0` | 206 | `initspritelists` | NEAR_MATCH |
| `1:40b40` | 21 | `0d08e` | 21 | `insertsprite` | STRICT |
| `1:40b55` | 158 | `0d0a3` | 158 | `insertspritesect` | NEAR_MATCH |
| `1:40bf3` | 156 | `0d141` | 156 | `insertspritestat` | NEAR_MATCH |
| `1:40c8f` | 20 | `0d1dd` | 20 | `deletesprite` | STRICT |
| `1:40ca3` | 219 | `0d1f1` | 219 | `deletespritesect` | NEAR_MATCH |
| `1:40d7e` | 217 | `0d2cc` | 217 | `deletespritestat` | NEAR_MATCH |
| `1:40e57` | 73 | `0d3a5` | 73 | `changespritesect` | STRICT |
| `1:40ea0` | 73 | `0d3ee` | 73 | `changespritestat` | STRICT |
| `1:40ee9` | 233 | `0d437` | 233 | `nextsectorneighborz` | STRICT |
| `1:40fd2` | 68 | `0d520` | 68 | `zerohandler` | STRICT |
| `1:41016` | 430 | `0d564` | 388 | `cansee` | NEAR_MATCH |
| `1:411c4` | 1,745 | `0d6e8` | 1,659 | `hitscan` | NEAR_MATCH |
| `1:41895` | 1,644 | `0dd63` | 1,583 | `neartag` | NEAR_MATCH |
| `1:41f01` | 545 | `0e392` | 548 | `lintersect` | NEAR_MATCH |
| `1:42122` | 252 | `0e5b6` | 252 | `dragpoint` | STRICT |
| `1:4221e` | 62 | `0e6b2` | 62 | `lastwall` | STRICT |
| `1:4225c` | 813 | `0e6f0` | 807 | `clipmove` | NEAR_MATCH |
| `1:42589` | 3,589 | `0ea17` | 3,585 | `getcliplines` | NEAR_MATCH |
| `1:4338e` | 65 | `0f818` | 65 | `addclipline` | STRICT |
| `1:433cf` | 355 | `0f859` | 355 | `keepaway` | NEAR_MATCH |
| `1:43532` | 613 | `0f9bc` | 524 | `raytrace` | NEAR_MATCH |
| `1:43797` | 202 | `0fbc8` | 202 | `updatesector` | STRICT |
| `1:43861` | 179 | `0fc92` | 179 | `rotatepoint` | NEAR_MATCH |
| `1:43914` | 38 | `0fd45` | 38 | `getmousevalues` | STRICT |
| `1:4393a` | 3 | `0fd6b` | 3 | `printscreeninterrupt` | STRICT |
| `1:4393d` | 1,541 | `0fd6e` | 1,171 | `drawline16` | NEAR_MATCH |
| `1:43f42` | 494 | `10201` | 272 | `qsetmode640480` | NEAR_MATCH |
| `1:44130` | 79 | `10311` | 79 | `clear2dscreen` | STRICT |
| `1:4417f` | 735 | `10360` | 735 | `draw2dgrid` | NEAR_MATCH |
| `1:4445e` | 4,585 | `1063f` | 3,169 | `draw2dscreen` | NEAR_MATCH |
| `1:45647` | 32 | `112a0` | 32 | `krand` | STRICT |
| `1:45667` | 2,938 | `112c0` | 2,819 | `getzrange` | NEAR_MATCH |
| `1:461e1` | 229 | `11dc3` | 229 | `makeradarang` | NEAR_MATCH |
| `1:462c6` | 334 | `11ea8` | 336 | `setview` | NEAR_MATCH |
| `1:46414` | 186 | `11ff8` | 186 | `precache` | STRICT |
| `1:464ce` | 1,938 | `120b2` | 1,938 | `wallmost` | STRICT |
| `1:46c60` | 1,497 | `12844` | 1,538 | `rotatesprite` | NEAR_MATCH |
| `1:47239` | 599 | `12e46` | 593 | `makepalookup` | NEAR_MATCH |
| `1:47490` | 292 | `13097` | 292 | `setvesamode` | NEAR_MATCH |
| `1:475b4` | 229 | `131bb` | 229 | `stereoinit` | STRICT |
| `1:47699` | 551 | `132a0` | 551 | `setverts` | NEAR_MATCH |
| `1:478c0` | 177 | `134c7` | 177 | `stereoblit` | STRICT |
| `1:47971` | 57 | `13578` | 57 | `stereoredblueblit` | STRICT |
| `1:479aa` | 176 | `135b1` | 176 | `setredbluepalette` | NEAR_MATCH |
| `1:47a5a` | 101 | `13661` | 101 | `setbrightness` | STRICT |

## Remaining first differences

The offsets come from the fresh matcher. Layout entries describe the first differing operand. Source and allocation classifications also note the principal remaining body differences.

| Function | First byte | Candidate / target bytes | Classification |
| --- | ---: | ---: | --- |
| `drawrooms` | `+0x271` | 928 / 928 | Layout first, then source and fixup-site differences in bunch traversal |
| `scansector` | `+0x10` | 747 / 756 | Source/body layout of terminal branch and scan loop |
| `wallfront` | `+0x27` | 553 / 554 | Register allocation of short wall indices and later comparison flow |
| `spritewallfront` | `+0x1d` | 114 / 114 | Register allocation and load order |
| `calcwalls` | `+0x16` | 4,970 / 5,068 | Stack allocation, with source differences in sector paint and sky traversal |
| `addboxlinum` | `+0x1cf` | 1,024 / 1,024 | Layout. Two jumps to the shared zero-return path at `wallfront+221` |
| `hline` | `+0x5` | 965 / 965 | Register and stack allocation in the prologue, projection and raster arguments |
| `screencapture` | `+0x34` | 1,511 / 1,511 | Stack slot for the inverse flag. Register scheduling and palette loop still differ across the full extent |
| `drawmaskwall` | `+0xd6` | 1,890 / 1,890 | Instruction selection and register allocation at the first xrepeat load, with the dword copy-loop form still pending |
| `drawsprite` | `+0x5` | 10,425 / 9,875 | Stack allocation. Early offset and division forms are restored. Projection and clipping shapes still differ |
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

