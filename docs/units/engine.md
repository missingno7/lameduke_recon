# December 1994 Build engine, third pass

Full Watcom 9.5b engine profile `-4r -s -or`. The baseline is the integrated second-pass source. The candidate is in `src3/`.

| Verifier status | Before functions | Before candidate bytes | After functions | After candidate bytes | After target bytes |
| --- | ---: | ---: | ---: | ---: | ---: |
| STRICT | 42 | 10,392 | 53 | 24,339 | 24,339 |
| CODE_BYTES_MATCH | 0 | 0 | 0 | 0 | 0 |
| NEAR_MATCH | 56 | 68,074 | 45 | 54,915 | 56,656 |
| UNPLACED | 0 | 0 | 0 | 0 | 0 |
| Total | 98 | 78,466 | 98 | 79,254 | 80,995 |

All 42 baseline STRICT results are preserved. The 11 new STRICT functions account for 13,947 bytes.

| New STRICT function | Target start | Bytes |
| --- | --- | ---: |
| `drawalls` | `1:35d09` | 3,811 |
| `wallscan` | `1:3796f` | 2,853 |
| `maskwallscan` | `1:38494` | 2,917 |
| `loadboard` | `1:39127` | 528 |
| `saveboard` | `1:39337` | 481 |
| `initengine` | `1:3a0c0` | 921 |
| `uninitengine` | `1:3a459` | 219 |
| `nextpage` | `1:3a534` | 741 |
| `allocache` | `1:3a868` | 411 |
| `getpixel16` | `1:3b2ae` | 226 |
| `drawmasks` | `1:3bfdb` | 839 |

## Reproduce

```powershell
$p = 'build/workers/engine'
python -B tools/match.py --profile engine --symbols "$p/overlay.json" "-i=$p/src3" "$p/src3/ENGINE.C" --json "$p/results.json"
```

The fresh compile had zero warnings and errors. The verifier exited 1 with 45 NEAR functions and verified all 75 `CONST` bytes at `2:1184` and 237 `_DATA` bytes at `2:2c00`. The verifier's ten unit tests passed. The integration gate was green at the integrator's baseline. The worker result uses the fresh matcher run above.

## Code ownership and shared returns

**PROVEN anchors.** `initengine` installs the keyboard interrupt handler at `1:33e5c`, immediately after the preceding function's return at `1:33e5b`. `setbrightness` returns at `1:47abe`. The six-entry networking switch table at `1:47abf` precedes its initializer at `1:47ad7`. The 1994-09-15 changelog moved COM and networking into MULTI.OBJ.

**STRONG code span.** The engine occupies `[1:33e5c,1:47abf)`, 80,995 bytes. All 98 candidate functions are in oracle order and 72 have their full oracle extent. The first three starts share one `_TEXT` base. `scansector` is still nine bytes short and starts the first drift. The second pass established that `rotatesprite` occupies `[1:46c60,1:47239)`, including its own column clipping and draw call. The candidate contains the 98 December publics with the late `clippoly4` helper removed.

**PROVEN wallscan return.** `wallscan` now reproduces `[1:3796f,1:38494)` and the shared return at `1:3848e`. Reproducing the immediately following `maskwallscan` restored the byte distances from `loadboard` and `saveboard` to that return. `setverts` now has its full 551-byte extent and uses the corresponding `wallscan+0xb1f` tail. Its displacement still differs because of the code between those two functions. `drawrooms` still has body and layout differences.

**PROVEN drawmasks return.** Its exact `[1:3bfdb,1:3c322)` extent restores the entries at `1:3c318` (12-byte stack adjustment), `1:3c31b` (six registers), and `1:3c31c` (five registers). The full-length `screencapture` let `initengine`, `uninitengine`, `nextpage`, `allocache`, and `getpixel16` use their original displacements. `drawalls` also verifies to the shared return. Its verified distance spans compensating size differences in `addboxlinum` (+5), `hline` (-4) and `transmaskvline` (-1). The verifier proves the whole function extent at its bound oracle address. Whole-object placement remains open.

**OPEN layout dependencies.** Fifteen remaining functions occupy 4,464 bytes and differ in code bytes only in cross-function jump displacements. `loadpalette` and `initcache` are five bytes from their correct displacement. `transmaskvline` is an additional case whose body has the correct 604-byte extent and whose sole differing code byte is in its shared jump. Its public extent also owns the 32-byte `initengine` switch table. The target has a one-byte alignment NOP at `1:3a09f` before the table. The candidate has no pad at its current section offset, leaving a 635-byte public extent against 636. `setvesamode` also has independently unresolved section-relative BSS operands.

## Third-pass source evidence

Dates reference the archived `BUILD.TXT` unless given as a distribution. The December oracle establishes the implementation details where the changelog does not date them.

| Evidence | Source restored |
| --- | --- |
| 1994-04-22 source and December `wallscan` | The April source already separates planar and linear writers. The target checks live tile dimensions, writes inclusive bottom rows before the clip comparison, and reads the live palette pointer per column. The four-column top loop selects the planar, 320-byte or 640-byte address inside the loop. The exact spans around `1:37c77`, `1:37f7a` and `1:38380` establish these details. The two divisions use the existing generic `divscale(65536L, sw, 16)` form. |
| 1995-01-03 column-width change and later rasterizer optimizations | Both wall scans use dword clip columns in December. `maskwallscan` restores the earlier generic division and left-associated planar offset seen at `1:38788`, `1:38af1` and `1:38ee2`. The later specialized division and the grouped planar macro generated different bodies. `transmaskvline` uses the corresponding earlier forms at `1:39ed6` and `1:39fcd`. |
| 1994-12-28 proposal, 1995-01-01 map v6 | The former notes established sector-derived palettes and the `spritepal` table. `drawmasks` now uses one short swap temporary for both short arrays and evaluates the depth comparison in oracle order. The compaction stores `spritepal` before `thesprite`. Those assignments reproduce `1:3c04e–3c0ac` and `1:3c22e–3c272` in the original compiler. |
| 1994-12-28 proposal and December `drawalls` | Moved the wall palette setup to the start of the draw pass, where `1:35d4a–35d59` reads the sector floor palette. Removed the later zero-height portal flag and per-wall zero palette stores. The target enters on the first scan and stops at the -1 link. Each clip save advances `smostcnt` inside the copy loop, as at `1:360d8–360fc`. Direct v5 wall and sector array indices recover the target's address expressions. |
| 1994-12-08 capture entry and December `screencapture` | A 256-byte write uses the current buffer count, then clears it only after the write succeeds. Short writes decrement the capture number and return -1. The target's 16-color palette branch advances the index in each branch before one byte store at `1:3b8fe`. This restores the 1,511-byte extent. The capture loop register assignment still differs. |
| 1994-04-22 source, December `hline` and `addboxlinum` | `hline` preserves the original distance through orientation swaps for the later shade calculation, stages the X product before computing the Y increment, and shifts the increments and coordinates before adding panning. It is now four bytes short. `addboxlinum` reads fields at the point2 index and checks both projected depth bounds after clipping both endpoints, as at `1:36f64–36f96`. Its extent is now five bytes long. |
| 1994-09-16 v5, 1994-11-29 floor sprites and December code | `calcwalls` uses direct v5 struct indices and a first-scan loop with the -1 terminator. It moved from 4,841 to 4,994 bytes against a 5,068-byte target. `drawsprite` restores initial offset flips at `1:3cb44–3cb71`, the fixed-point division forms at `1:3cba0` and `1:3ce48`, and direct v5 array access. It remains 550 bytes long. The extent and allocation still diverge across its face, wall and floor paths. |
| 1995-04-16, 1995-04-26, 1995-05-17 | Removed the later `show2dsector`, variable video dimensions and `ylookup`, and aspect ratio fields from `BUILD.H`. `numpages` is part of the later video interface in the June header. None is used by the reconstructed December C or rasterizer. |

## Engine data

- **PROVEN initialized spans.** `CONST` covers `[2:1184,2:11cf)`. `_DATA` covers `[2:2c00,2:2ced)`. Both verify as complete segments.
- **STRONG BSS span.** The first ART array starts at `2:b1e00`, followed by the second at `2:b2200` and the clip arrays from `2:b2600`. `setvesamode` independently locates the anonymous 50-byte real-mode register block at `2:1bde9f`, immediately after `visibility`. The inferred exclusive end is `2:1bded1`, giving 1,097,937 bytes. The adjacent owner still needs confirmation. Removing the later header fields reduced the candidate's `_BSS` from 1,100,018 to 1,097,946 bytes. Nine bytes remain against the inferred span.
- **PROVEN binding conflict.** The engine buffer is at `2:132d90`. `drawrooms` marks bunches there and `screencapture` passes it to `write`. The canonical GAMEDEF `tempbuf` at `2:a4830` is a separate object. The candidate spells the private engine variable `enginetempbuf` to give the flat registry an unambiguous name. This can affect BSS ordering under Watcom's name hash. A future unit-qualified binding could retain the historical spelling.
- **PROVEN in the worker fixture.** Imported `build/workers/sounds/bssrule.py` read-only and compared its predictions to the three Watcom 9.5b A/B/C probes described below. It predicts 40/40 offsets in each probe, or 120/120 overall. A and C are identical. A and B differ at 34 of 40 publics. This confirms the rule on the independent example from the earlier engine pass.
- **STRONG engine observations from the rule.** With size bucket 6, the target's clip-array series ends in `hitwalls` (hash 186), then drops to `lastx` (hash 1). A page change is required between them. The candidate emits `lastx` and `palette` before these clip arrays. The target also drops from `extra` (hash 202) to `sprite` (hash 165) in the same size bucket. The candidate emits `sprite`, `sector`, and `wall` ahead of `extra`. The later page wins under the validated rule. The exact December declaration stream and page assignments are open.
- **OPEN placement.** The 206 named candidate BSS publics include 182 with bound oracle addresses, which imply 50 different segment bases. Seventeen publics agree with base `2:b1e00`. `lwall` agrees at offset `0x10a5fc` in both layouts. The candidate RMI starts at relative `0x10c0a8` vs the inferred target `0x10c09f`. It is emitted after the named one-byte globals and has no public for direct binding. This is a separate allocation from the named-symbol ordering experiment.

### BSS fixture reproduction

Use the already documented A/B/C inputs reproduced here. Compile the three separately with `python -B tools/wcc.py --profile engine SRC "-fo=OUT"` from the repository root. Read the offsets with `tools/omf.py` and compare against `bssrule.predict` on the same declaration triples. The first declaration for each array in C is an extern with size zero.

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

- In `wallscan`, the first arithmetic backport reduced 3,144 to 2,946 bytes. The slot layout still kept the column counter above offset `0x80`, producing longer displacements. Correcting the single-column staging, planar address association, and byte-local allocation moved it to the target's short-displacement slot. The final object uses the full 2,853-byte body and the target epilogue.
- In `drawmasks`, reversing the comparison alone left register differences. Reusing the short temporary gave the correct 839-byte extent and matched the sort. Switching the order of Y and tile writes in compaction gave 835 bytes. Putting the palette store before the sprite-index store recovered the full body.
- In `screencapture`, declaration order, `register` hints, standalone `open` assignment, and loop syntax left the register allocation open. A single ternary palette write with byte casts produced 1,510 bytes. The per-branch advance and common store match the observed tail shape and the 1,511-byte extent.
- A separate orientation-swap temporary in `hline` corrected the distance lifetime and reduced 1,016 to 989 bytes. Staging the products and panning brought it to 961. Independent panning order and sign-expression trials stopped short of a full match.
- `_BSS` remains an order and ownership problem. The post-1994 header removals reduce the length discrepancy. The target declaration stream remains open. The group analysis above identifies two places where the target requires a different symbol page assignment.
- Remaining high-return work is the five-byte relative deficit shared by `loadpalette` and `initcache`, the unaligned `transmaskvline` table boundary, and the large spans in `drawsprite`, `calcwalls`, `getcliplines`, `draw2dscreen`, and `wallmost`. The 15 branch-only bodies already make their jump destinations identifiable.

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

## Target function order

The target extent ends immediately before the next row and the final extent ends at `1:47abf`. The switch tables immediately before `initengine` at `1:3a0a0` and `nextpage` at `1:3a514` are included in the prior public's extent.

| Target start | Target bytes | Candidate offset | Candidate bytes | Function | Status |
| --- | ---: | --- | ---: | --- | --- |
| `1:33e5c` | 188 | `00000` | 188 | `keyhandler` | STRICT |
| `1:33f18` | 928 | `000bc` | 928 | `drawrooms` | NEAR_MATCH |
| `1:342b8` | 756 | `0045c` | 747 | `scansector` | NEAR_MATCH |
| `1:345ac` | 554 | `00747` | 549 | `wallfront` | NEAR_MATCH |
| `1:347d6` | 114 | `0096c` | 114 | `spritewallfront` | NEAR_MATCH |
| `1:34848` | 245 | `009de` | 245 | `bunchfront` | STRICT |
| `1:3493d` | 5,068 | `00ad3` | 4,994 | `calcwalls` | NEAR_MATCH |
| `1:35d09` | 3,811 | `01e55` | 3,811 | `drawalls` | STRICT |
| `1:36bec` | 1,024 | `02d38` | 1,029 | `addboxlinum` | NEAR_MATCH |
| `1:36fec` | 1,209 | `0313d` | 1,209 | `ceilscan` | STRICT |
| `1:374a5` | 1,226 | `035f6` | 1,226 | `florscan` | STRICT |
| `1:3796f` | 2,853 | `03ac0` | 2,853 | `wallscan` | STRICT |
| `1:38494` | 2,917 | `045e5` | 2,917 | `maskwallscan` | STRICT |
| `1:38ff9` | 302 | `0514a` | 302 | `transmaskwallscan` | STRICT |
| `1:39127` | 528 | `05278` | 528 | `loadboard` | STRICT |
| `1:39337` | 481 | `05488` | 481 | `saveboard` | STRICT |
| `1:39518` | 303 | `05669` | 303 | `loadtables` | STRICT |
| `1:39647` | 510 | `05798` | 510 | `loadpalette` | NEAR_MATCH |
| `1:39845` | 570 | `05996` | 570 | `setgamemode` | STRICT |
| `1:39a7f` | 965 | `05bd0` | 961 | `hline` | NEAR_MATCH |
| `1:39e44` | 636 | `05f91` | 635 | `transmaskvline` | NEAR_MATCH |
| `1:3a0c0` | 921 | `0620c` | 921 | `initengine` | STRICT |
| `1:3a459` | 219 | `065a5` | 219 | `uninitengine` | STRICT |
| `1:3a534` | 741 | `06680` | 741 | `nextpage` | STRICT |
| `1:3a819` | 79 | `06965` | 79 | `initcache` | NEAR_MATCH |
| `1:3a868` | 411 | `069b4` | 411 | `allocache` | STRICT |
| `1:3aa03` | 198 | `06b4f` | 198 | `allocatepermanenttile` | STRICT |
| `1:3aac9` | 258 | `06c15` | 258 | `loadtile` | STRICT |
| `1:3abcb` | 773 | `06d17` | 773 | `loadpics` | STRICT |
| `1:3aed0` | 122 | `0701c` | 122 | `showengineinfo` | STRICT |
| `1:3af4a` | 23 | `07096` | 23 | `resettiming` | STRICT |
| `1:3af61` | 470 | `070ad` | 470 | `clipinsidebox` | STRICT |
| `1:3b137` | 375 | `07283` | 375 | `clipinsideboxline` | STRICT |
| `1:3b2ae` | 226 | `073fa` | 226 | `getpixel16` | STRICT |
| `1:3b390` | 1,511 | `074dc` | 1,511 | `screencapture` | NEAR_MATCH |
| `1:3b977` | 271 | `07ac3` | 271 | `inside` | STRICT |
| `1:3ba86` | 230 | `07bd2` | 230 | `getangle` | STRICT |
| `1:3bb6c` | 60 | `07cb8` | 60 | `ksqrt` | STRICT |
| `1:3bba8` | 62 | `07cf4` | 62 | `calcksqrt` | STRICT |
| `1:3bbe6` | 325 | `07d32` | 325 | `copytilepiece` | STRICT |
| `1:3bd2b` | 688 | `07e77` | 688 | `printext` | STRICT |
| `1:3bfdb` | 839 | `08127` | 839 | `drawmasks` | STRICT |
| `1:3c322` | 1,890 | `0846e` | 1,987 | `drawmaskwall` | NEAR_MATCH |
| `1:3ca84` | 9,875 | `08c31` | 10,425 | `drawsprite` | NEAR_MATCH |
| `1:3f117` | 402 | `0b4ea` | 402 | `ceilspritescan` | NEAR_MATCH |
| `1:3f2a9` | 976 | `0b67c` | 1,044 | `ceilspritehline` | NEAR_MATCH |
| `1:3f679` | 1,063 | `0ba90` | 1,323 | `overwritesprite` | NEAR_MATCH |
| `1:3faa0` | 1,358 | `0bfbb` | 1,358 | `permanentwritesprite` | NEAR_MATCH |
| `1:3ffee` | 2,169 | `0c509` | 2,337 | `grouvline` | NEAR_MATCH |
| `1:40867` | 98 | `0ce2a` | 98 | `setsprite` | STRICT |
| `1:408c9` | 281 | `0ce8c` | 281 | `movesprite` | STRICT |
| `1:409e2` | 144 | `0cfa5` | 144 | `animateoffs` | STRICT |
| `1:40a72` | 206 | `0d035` | 206 | `initspritelists` | NEAR_MATCH |
| `1:40b40` | 21 | `0d103` | 21 | `insertsprite` | STRICT |
| `1:40b55` | 158 | `0d118` | 158 | `insertspritesect` | NEAR_MATCH |
| `1:40bf3` | 156 | `0d1b6` | 156 | `insertspritestat` | NEAR_MATCH |
| `1:40c8f` | 20 | `0d252` | 20 | `deletesprite` | STRICT |
| `1:40ca3` | 219 | `0d266` | 219 | `deletespritesect` | NEAR_MATCH |
| `1:40d7e` | 217 | `0d341` | 217 | `deletespritestat` | NEAR_MATCH |
| `1:40e57` | 73 | `0d41a` | 73 | `changespritesect` | STRICT |
| `1:40ea0` | 73 | `0d463` | 73 | `changespritestat` | STRICT |
| `1:40ee9` | 233 | `0d4ac` | 233 | `nextsectorneighborz` | STRICT |
| `1:40fd2` | 68 | `0d595` | 68 | `zerohandler` | STRICT |
| `1:41016` | 430 | `0d5d9` | 388 | `cansee` | NEAR_MATCH |
| `1:411c4` | 1,745 | `0d75d` | 1,659 | `hitscan` | NEAR_MATCH |
| `1:41895` | 1,644 | `0ddd8` | 1,583 | `neartag` | NEAR_MATCH |
| `1:41f01` | 545 | `0e407` | 548 | `lintersect` | NEAR_MATCH |
| `1:42122` | 252 | `0e62b` | 252 | `dragpoint` | STRICT |
| `1:4221e` | 62 | `0e727` | 62 | `lastwall` | STRICT |
| `1:4225c` | 813 | `0e765` | 807 | `clipmove` | NEAR_MATCH |
| `1:42589` | 3,589 | `0ea8c` | 3,585 | `getcliplines` | NEAR_MATCH |
| `1:4338e` | 65 | `0f88d` | 65 | `addclipline` | STRICT |
| `1:433cf` | 355 | `0f8ce` | 355 | `keepaway` | NEAR_MATCH |
| `1:43532` | 613 | `0fa31` | 524 | `raytrace` | NEAR_MATCH |
| `1:43797` | 202 | `0fc3d` | 202 | `updatesector` | STRICT |
| `1:43861` | 179 | `0fd07` | 179 | `rotatepoint` | NEAR_MATCH |
| `1:43914` | 38 | `0fdba` | 38 | `getmousevalues` | STRICT |
| `1:4393a` | 3 | `0fde0` | 3 | `printscreeninterrupt` | STRICT |
| `1:4393d` | 1,541 | `0fde3` | 1,171 | `drawline16` | NEAR_MATCH |
| `1:43f42` | 494 | `10276` | 272 | `qsetmode640480` | NEAR_MATCH |
| `1:44130` | 79 | `10386` | 79 | `clear2dscreen` | STRICT |
| `1:4417f` | 735 | `103d5` | 735 | `draw2dgrid` | NEAR_MATCH |
| `1:4445e` | 4,585 | `106b4` | 3,169 | `draw2dscreen` | NEAR_MATCH |
| `1:45647` | 32 | `11315` | 32 | `krand` | STRICT |
| `1:45667` | 2,938 | `11335` | 2,819 | `getzrange` | NEAR_MATCH |
| `1:461e1` | 229 | `11e38` | 229 | `makeradarang` | NEAR_MATCH |
| `1:462c6` | 334 | `11f1d` | 336 | `setview` | NEAR_MATCH |
| `1:46414` | 186 | `1206d` | 186 | `precache` | STRICT |
| `1:464ce` | 1,938 | `12127` | 1,517 | `wallmost` | NEAR_MATCH |
| `1:46c60` | 1,497 | `12714` | 1,538 | `rotatesprite` | NEAR_MATCH |
| `1:47239` | 599 | `12d16` | 593 | `makepalookup` | NEAR_MATCH |
| `1:47490` | 292 | `12f67` | 292 | `setvesamode` | NEAR_MATCH |
| `1:475b4` | 229 | `1308b` | 229 | `stereoinit` | STRICT |
| `1:47699` | 551 | `13170` | 551 | `setverts` | NEAR_MATCH |
| `1:478c0` | 177 | `13397` | 177 | `stereoblit` | STRICT |
| `1:47971` | 57 | `13448` | 57 | `stereoredblueblit` | STRICT |
| `1:479aa` | 176 | `13481` | 176 | `setredbluepalette` | NEAR_MATCH |
| `1:47a5a` | 101 | `13531` | 101 | `setbrightness` | STRICT |

## Remaining first differences

Offsets come from the current verifier. For extents with further mismatches, the classification describes the first difference and the known body differences. Equal-length cross-function displacement cases have verified fixup sites. Some still have unverified targets.

| Function | First byte | Candidate / target bytes | Classification |
| --- | ---: | ---: | --- |
| `drawrooms` | `+0x271` | 928 / 928 | Layout first, then source and fixup-site differences in bunch traversal |
| `scansector` | `+0x10` | 747 / 756 | Source/body layout of terminal branch and scan loop |
| `wallfront` | `+0xa` | 549 / 554 | Register allocation of short wall indices and later comparison flow |
| `spritewallfront` | `+0x5` | 114 / 114 | Register allocation and load order |
| `calcwalls` | `+0x8` | 4,994 / 5,068 | Stack allocation after v5 index restoration, with remaining projection and sky differences |
| `addboxlinum` | `+0x28` | 1,029 / 1,024 | Register allocation at the initial global position loads and later clip/order differences |
| `loadpalette` | `+0xe` | 510 / 510 | Layout. Displacement to `drawmasks+341` |
| `hline` | `+0xb` | 961 / 965 | Stack slot for the right span argument. The rotation, shifts and address setup still differ in allocation |
| `transmaskvline` | `+0x257` | 635 / 636 | Layout. Shared return displacement and a one-byte switch-table alignment difference |
| `initcache` | `+0x4b` | 79 / 79 | Layout. Displacement to `setgamemode+234` |
| `screencapture` | `+0x34` | 1,511 / 1,511 | Stack slot for the inverse flag. Register scheduling and palette loop still differ across the full extent |
| `drawmaskwall` | `+0x8` | 1,987 / 1,890 | Stack allocation, with source differences in occlusion clipping |
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
| `wallmost` | `+0x6` | 1,517 / 1,938 | Stack allocation, with source differences in projected clipping |
| `rotatesprite` | `+0x5` | 1,538 / 1,497 | Stack allocation, with source differences in direct column clipping and draw path |
| `makepalookup` | `+0xb` | 593 / 599 | Stack allocation of color arguments and scheduling |
| `setvesamode` | `+0x120` | 292 / 292 | Layout. Displacement to `drawmasks+341` and unverified section-relative BSS operands |
| `setverts` | `+0x223` | 551 / 551 | Layout. Displacement to `wallscan+b1f` |
| `setredbluepalette` | `+0xac` | 176 / 176 | Layout. Displacement to `drawmasks+341` |
