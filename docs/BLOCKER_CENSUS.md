# First blockers in the canonical DOS build

## SECTOR wall-index stores at 553678e

checksectors remains NEAR_MATCH. The pinned DOS game_oe build verifies 13 of 18 SECTOR functions STRICT for 5,602 bytes, including checkhitceiling (856 B) and checkhitwall (1,648 B). The five remaining functions are NEAR_MATCH. All builds in the table keep the first near-lift jump at PUBLIC+0x339, the first destination at +0x3ad and the first wall store at +0x3d1. Extents include the 264-byte leading table.

| Whole-unit DOS build | Frame B | First-wall homes (near / underfoot), ESP+ | Underfoot store, PUBLIC-relative | Extent B | Byte differences | Fixup diagnostics | Verified table entries / 66 | checkhitceiling |
| --- | ---: | --- | --- | ---: | ---: | ---: | ---: | --- |
| Canonical | 176 | 0x78 / 0x78 | +0x1760 | 11,448 | 7,135 | 556 | 38 | STRICT |
| First and last at outer function scope | 172 | 0x6c / 0x70 | +0x1763 | 11,352 | 7,487 | 622 | 2 | CODE_BYTES_MATCH |
| Outer bounds and counters, short-frame caller | 168 | 0x80 / 0x7c | +0x16f6 | 11,176 | 7,242 | 622 | 2 | CODE_BYTES_MATCH |
| Exact frame and extent | 168 | 0x5c / 0x6c | +0x1728 | 11,448 | 7,483 | 620 | 3 | STRICT |
| Long return with a narrow case-26 comparison | 168 | 0x88 / 0x88 | +0x1773 | 11,464 | 7,459 | 616 | 2 | STRICT |
| Exact homes, diagnostic source | 176 | 0x90 / 0x8c | +0x17b9 | 11,624 | 7,638 | 632 | 2 | STRICT |
| Closest extent with both exact homes | 176 | 0x90 / 0x8c | +0x1777 | 11,544 | 7,674 | 627 | 2 | STRICT |
| Oracle | 168 | 0x90 / 0x8c | +0x173f | 11,448 | 0 | 0 | 66 | STRICT |

Canonical source retained. The integration source.patch is empty. Canonical retains its confirmed 11,448-byte extent, 139 proven reference records and 38/66 proven table entries. Its first differing byte is extent+0x110 (PUBLIC+0x8). The 176-byte frame and the four-byte store to ESP+0x78 remain open.

The earlier 168-byte control ended 16 B early. The new exact-frame, exact-extent control puts the near wall at ESP+0x5c and regresses operatesectors, operateactivators, cheatkeys and checkhitswitch. The long-return control that ends 16 B late preserves all 13 STRICT proofs, but both inlined walls use ESP+0x88 and three other NEAR functions regress. The exact-homes controls also preserve checkhitceiling and checkhitwall. None recovers the canonical target proofs and peer scores together. A separate caller-only comparison reorder reduced the raw mismatch count by one through an immediate overlapping an oracle branch displacement. It left both wall homes unchanged and was rejected.

A DOS control with just the 32-bit return and reordered case-9 declarations reaches the 168-byte frame and the seven-byte store to ESP+0x90 together. The dispatch remains at +0x339, but entry zero and the store are one byte early (+0x3ac and +0x3d0). The underfoot store also uses ESP+0x90, and the extent ends 48 B late. All 13 STRICT publics survive. Four NEAR peers regress. The independently applicable diagnostic source is prototype/one-byte-early/control.patch.

The full per-public verdicts, allocator traces and standalone control patches are under build/workers/blockers/wallhome. prototype/exact-homes/control.patch reproduces the closest DOS extent with both wall homes correct.

## First near-lift dispatch and glass hits at 3dab741

The stock DOS wc95b builds use the registered game_oe profile. Extent-relative first-byte offsets include the 264-byte leading table of checksectors. The jump and destination measurements below start at the PUBLIC.

| Target | Result | Extent B | Differing bytes | First difference | Fixup diagnostics | Proven reference records |
| --- | --- | ---: | ---: | --- | ---: | ---: |
| checksectors | NEAR_MATCH | 11,448 | 7,522 → 7,135 | 0x110 → 0x110 | 601 → 556 | 96 → 139 |
| checkhitwall | STRICT | 1,648 | 46 → 0 | 0x1e4 → equal | 3 → 0 | 83 → 85 |

**checksectors.** The proposed source tests the byte latch directly and stages the arguments to the two lifts in short locals. The player number now stays in EDI. The first near-lift dispatch moves from PUBLIC+0x366 to its oracle position at +0x339. Entry zero moves from +0x3da to +0x3ad and verifies independently. The full 11,448-byte extent and the preceding ceiling proof both hold. The verifier confirms 38/66 leading table entries, compared with 37 at baseline. The frame is 176 B against the oracle's 168 B. The first instruction-length difference after entry zero occurs at +0x3d1 when saving the first sliding-door wall index. The oracle uses ESP+0x90 (7 B) and this build uses ESP+0x78 (4 B). The next displaced distinct destination is +0x9ad in the oracle and +0x9b6 in the candidate. A DOS control loading the initiator first selected the six-byte absolute word load into AX and put both dispatch coordinates one byte early. The proposal loads neartagsector into BX at +0x2c2 using the oracle’s seven-byte encoding.

**checkhitwall.** Two separate statements write 573, first to the hit wall and then to its linked wall. Ordering the four glass groups by first tile (819, 823, 860, 864) restores their destinations. The stock verifier confirms 1,648 bytes and all 85 reference records, including the signed effector-list advance. Each edit on its own increases the whole-function difference count.

The SECTOR unit has 13/18 STRICT publics covering 5,602 bytes. The 856-byte checkhitceiling remains STRICT. The two complete DOS objects are byte-identical. [Worker evidence](../build/workers/blockers/lift/predictions.md)

## SECTOR DOS verification at 12e1107

The baseline and proposed source use the stock DOS wc95b `game_oe` profile (`-4r -s -otexan -oe=3000`). Differences count unequal bytes at the bound extent. First-byte offsets below include each function's leading table. Fixup diagnostics count verifier messages. One message can cover several sites.

| Function | Extent B | Unequal bytes before → after | First byte before → after | Fixup diagnostics before → after | Proven references before → after |
| --- | ---: | ---: | ---: | ---: | ---: |
| `checksectors` | 11,448 | 7,670 → 7,522 | +0x115 → +0x110 | 633 → 601 | 66 → 96 |
| `cheatkeys` | 7,604 | 3,332 → 3,332 | +0x98 → +0x98 | 284 → 284 | 185 → 185 |
| `operateactivators` | 4,996 | 3,157 → 3,157 | +0x5c → +0x5c | 281 → 281 | 31 → 31 |
| `checkhitwall` | 1,648 | 78 → 46 | +0x213 → +0x1e4 | 12 → 3 | 74 → 83 |

**checksectors.** The baseline's first differing instruction starts at PUBLIC+0xc. Watcom spills the incoming player number at ESP+0x80 where the oracle copies EAX to EDI. In the captured graph every register interferes with `t1_snum` from block 0 through block 391. Reusing `w` for the current sector and the wide `i` for its initiator restores the entry register, moves the first unequal byte to extent+0x17a and reduces differences to 7,484. Its public span shrinks by 128 B and `checkhitceiling` becomes CODE_BYTES_MATCH. It also has five more fixup diagnostics. The smaller result was excluded. The proposed C tests the same eight underfoot sector tags in descending groups. At the original extent it gives 7,522 differing bytes and 601 fixup diagnostics, with 96 proven reference records against 66 at baseline. The number of individually confirmed leading-table entries is 37 of 66, compared with 3 at baseline. The first mismatch moves earlier to extent+0x110. The frame instruction is `sub esp,0xa4`, and the incoming player number is saved by `mov m4[esp+0x8c],eax` at PUBLIC+0xc. Testing all eight tags as separate ascending equalities gave 7,611 differing bytes and 630 fixup diagnostics at the same extent. A separate control restores MOV EDI,EAX and keeps the ceiling STRICT by caching the initial current sector in w before reusing w for the last lift call. That control has 8,129 differing bytes and loses checksectors' own extent proof. The 37 matching entries cover eight distinct destinations. Twenty now reach the oracle’s highest target at PUBLIC+0x2b9c exactly. Three different entries matched on the baseline. The first near-lift indirect jump remains 45 B late at PUBLIC+0x366 against +0x339, and entry zero reaches +0x3da against +0x3ad. The displacement already exists at dispatch.

**cheatkeys.** The first unequal byte is the frame immediate at PUBLIC+8. Watcom reserves 100 B and the oracle reserves 96 B. At PUBLIC+0x9fc, the inline debug-door dispatch reloads a word where the oracle reloads a dword. Reversing the first pair of sliding-door midpoint calculations in the shared callee gives 3,329 differences and 282 fixup diagnostics at 7,604 B. A second DOS control, reusing menu_flags for weapon_pos before the weapon tests, produces the oracle's 96-byte frame. The incoming player number then goes to ESP+0x44 instead of ESP+0x4c. The function grows eight bytes and reaches 4,877 differences. Whole-unit DOS verification of the midpoint change also gives 7,550 differences in `operatesectors` and 8,405 in `checksectors`.

**operateactivators.** The frame is 100 B against 92 B, with the incoming channel at ESP+0x48 against ESP+0x4c. Moving the case-9 declarations in the shared callee gives 2,705 differences and 228 fixup diagnostics at the original 4,996 B. The same DOS build puts `cheatkeys` at 5,403 differences, `checkhitswitch` at eight and `checksectors` at 8,183. It loses the adjacent ceiling extent proof. Using a wide list cursor and index with a short sector argument restores the 92-byte frame. The channel still goes to ESP+0x48, the function shrinks 16 B and has 3,529 differences. Neither frame control qualified for integration.

**checkhitwall.** Moving the 860–863 glass case group ahead of 819–822 gives 48 differences and three fixup diagnostics. The new first mismatch is an earlier branch at +0x1e4. Watcom emits a near JBE to +0x29f where the oracle has a short JBE to +0x257. The case values still select the same wall replacements in C. An independent source change stages the signed effector list index as a byte offset. With the original case order this reproduces the forced t179 → EAX result across all 1,648 unrelocated function bytes and all verifier reference records. Only the ModR/M byte at +0x514 and the SIB byte at +0x518 change from baseline. Whole-unit DOS results for the four combinations are 78 differences unchanged, 76 for the list advance alone, 48 for case order alone and 46 for both. The combined build proves 83 references and leaves the unmatched fixups at +0x26d, +0x26e and +0x275. The individual 76-byte result retains the original first difference at +0x213.

The proposed whole unit has 12 STRICT functions covering 3,954 B and six NEAR functions covering 37,724 B. Every previous STRICT public survives, including all 856 B of `checkhitceiling`. Both DOS compiles produced the same object. [Worker evidence](../build/workers/blockers/sector/predictions.md)

## Source tests of the allocator witnesses at 787292d

The five canonical DOS units contained 140 STRICT functions (63,900 bytes), one CODE_BYTES_MATCH function (292 bytes) and 29 NEAR functions. The six requested NEAR targets account for 16,327 bytes of extent. The proposed MULTI, SOUNDS and ENGINE changes remove 32 positional differences. All three complete objects reproduced byte for byte on a second stock DOS build. Every other public retained its status, difference count, fixup problems and unresolved references. Previously verified segments and the 856-byte checkhitceiling boundary hold. No overlay was used.

| Function | Oracle extent | Baseline differences | Proposed differences | First differing byte in the proposed extent |
|---|---:|---:|---:|---|
| operatesectors | 10,288 | 7,404 | 7,404 | +0x91 |
| getzrange | 2,938 | 2,288 | 2,270 | +0x26 |
| checkhitswitch | 1,740 | 2 | 2 | +0x374 |
| scansector | 756 | 7 | 7 | +0x12a |
| spritesound | 464 | 56 | 43 | +0x9 |
| comsend | 141 | 5 | 4 | +0x5e |

The original three register flips replayed to exact diagnostic objects on this snapshot. The table counts only source compiled by the stock DOS compiler. SECTOR instruction offsets below are relative to the PUBLIC. Its leading tables occupy 84 bytes in operatesectors and 108 in checkhitswitch.

- **SECTOR.** Keeping both the narrow sector index and owner across the switch recovered the case-26 EBX calculation at +0x97 and the destinations +0xfe and +0x10a. The owner load still uses ESP+0x1cc instead of +0x1a0. The epilogue releases 0x1d4 instead of 0x1c4. The whole function grows 112 bytes, checksectors adds 544 differences and checkhitceiling loses its extent proof. A separate sector-index control retained the 10,288-byte extent and reduced 7,404 differences to 6,903, but lost the same boundary and worsened four callers or peers.
- **ENGINE.** The March 1995 scansector predicate produces both requested choices in a stock trace. Its whole function has 64 differences and spans 757 bytes. The first displacement changes at +0x10. The first instruction-width change occurs at +0x29f on the spritesortcnt load. Eleven STRICT publics lose proof in this control. In getzrange, three diagnostic home exchanges reproduce the bound stores and advance the first mismatch to +0x85. Giving floor_x2 function scope instead gives a 2,938-byte stock function with 2,270 differences and 68 fixup diagnostics. It restores the _scaredfallz and _sector references at +0x329 and +0x32f, with all ten previous reference proofs retained. ymin stays at ESP+0x48, and +0x26 remains the first unequal byte. A separate control reusing the input coordinates grew the frame from 120 to 132 bytes and lost three STRICT publics.
- **checkhitswitch.** Prefetching the case-406 link before the state store restores both case-405 encoding bytes. It leaves 299 differences, loses checkhitceiling and adds 448 differences to checksectors. In this natural trace the case-405 conversion t116 moves to rank 83 and the scale t117 to rank 82. Both choose ECX. The canonical function retains its two differences.
- **MULTI and SOUNDS.** Holding the producer cursor across the ESC2 store gives comsend 4 differences at its original 141 bytes. Two fixup diagnostics remain for one displaced site. Widening the signed sprite index and naming the listener-coordinate difference in xyzsound gives 43 differences in 464 bytes. Its first mismatch moves from +0x32 to +0x9. All 25 references verify and standalone xyzsound remains STRICT.

The worker census includes counts by blocker class, individual DOS controls, and source and object receipts. [Worker census](../build/workers/blockers/natural/census.json)


| First-blocker class | Functions | Oracle extent | Positional differences before / after |
|---|---:|---:|---:|
| switch table and register allocation | 1 | 10,288 | 7,404 / 7,404 |
| stack home | 2 | 3,402 | 2,344 / 2,313 |
| equal-savings conflict order | 1 | 1,740 | 2 / 2 |
| register preference and coalescing | 2 | 897 | 12 / 11 |

## Follow-up on case 26 and the remaining frame blockers

At 2454e79 the five units had 30 NEAR functions. Across those 30 the largest census classes by oracle extent were switch tables (4 functions, 34,336 bytes), stack homes (5, 15,374) and frame reservations (5, 13,395). The original classifications describe the first discrepancy in the census snapshot. Both getzrange and grouvline already reserve their original frames in this control.

| Function | Baseline differing bytes | Candidate differing bytes | First differing byte, before → after |
|---|---:|---:|---|
| ENGINE setview | 9 | 0 | +0xb5 → STRICT, 334 bytes and 26 verified references |
| ENGINE hitscan | 1,056 | 993 | +0x17 → +0x5d |
| ENGINE drawsprite | 7,056 | 7,054 | +0x70 → +0x77 |
| ENGINE grouvline | 1,478 | 1,459 | +0xc3 → +0xc3, verified references 9 → 12 |
| SOUNDS spritesound | 149 | 56 | +0x0a → +0x32, verified references 12 → 25 |

The five changes remove 186 positional byte differences. A fresh whole-unit DOS build and repeat produced identical objects for each changed unit. ENGINE gains one STRICT and retains all 85 prior STRICT functions. SOUNDS retains all 12. The other function results and previously verified segments hold.

**SECTOR case 26.** In operatesectors the factored index for the switch changes register at PUBLIC+0x58. In the recursive arm the first unequal-width instruction pair begins at +0xa0. The oracle interleaves the index calculation in EBX with copying ECX into EAX and clearing AL. The candidate calculates the scaled index in EAX first. Its shared return starts at +0xfd and case 9 at +0x109. The corresponding oracle destinations are +0xfe and +0x10a.

A temporary sector pointer for the tag updates to 22 restores both table destinations with a different sequence from +0xaa onward. The function grows 12 bytes and checkhitceiling falls to CODE_BYTES_MATCH on extent proof. Keeping the same pointer and declaring case 9’s sp separately gives a 10,292-byte operatesectors against 10,288 in the oracle. This version restores the 856-byte checkhitceiling STRICT but adds 241 positional differences in operateactivators, 273 in cheatkeys, 3 in checkhitswitch and 560 in checksectors. Both controls were rejected.

**ENGINE and MENUES negatives.** getzrange writes xmin to the correct ESP+0x54. Its following ymin store uses ESP+0x48 where the oracle uses +0x5c. Changing the wall endpoint declarations lowers the score by six at the same extent but gains no exact symbolic instruction and loses one. Pairing the bounds, changing their visit order and reusing floor endpoints did not advance the first difference. In draw_and_probe an unsigned-byte menutext shade formal restores all 896 bytes, and menus remains STRICT. Standalone menutext loses six displacement bytes out of 352. The proposal leaves MENUES at its control result.

The worker records the other functions, unsuccessful predictions and verification receipts in [the follow-up census](../build/workers/blockers/case26/CENSUS.md) and [the prediction results](../build/workers/blockers/case26/predictions.md).

## Follow-up on switch cascades and frame reservations

The five eligible units at 935dabb contain ten functions in these classes, covering 52,947 oracle bytes. SECTOR and MENUES have five table-first functions. ENGINE has five frame-first functions. SOUNDS and MULTI contributed none. Fresh DOS control builds covered all five units. The historical full-project census below retains its original snapshot.

Three source changes survived whole-unit DOS verification. **MENUES menus is STRICT for 5,216 bytes and 319 verified references.** ENGINE grouvline and getzrange regained their original frame and extent. Their combined positional differences fell by 272 bytes. All prior STRICT functions, verified data segments and other function comparisons held in the two proposed units. The other seven targets remain open.

| Function in descending oracle extent | Bytes | Frame, candidate / oracle | First unequal-length instruction after the public | First displaced case in address order, oracle → candidate | DOS result of the tested prediction |
|---|---:|---:|---|---|---|
| checksectors | 11,448 | 168 / 168 | +0xc, 2 B oracle / 7 B candidate | +0x341 → +0x36e (+45 B) | 168 B frame, 7,670 → 7,484 differences, rejected |
| operatesectors | 10,288 | 452 / 452 | +0xa0, 2 B oracle / 7 B candidate | +0xfe → +0xfd (-1 B) | 460 B frame, 7,404 → 7,180 differences, rejected |
| cheatkeys | 7,604 | 100 / 96 | +0x9fc, 4 B oracle / 5 B candidate | +0xa48 → +0xa57 (+15 B) | 96 B frame, 3,332 → 5,273 differences, rejected |
| menus | 5,216 | 252 / 252 | +0x134, 4 B oracle / 2 B candidate | +0x206 → +0x204 (-2 B) | STRICT, retained |
| operateactivators | 4,996 | 100 / 92 | +0xaf, 7 B oracle / 5 B candidate | +0x6f8 → +0x707 (+15 B) | 92 B frame, 3,157 → 3,530 differences, rejected |
| getcliplines | 3,589 | 136 / 124 | +0x3, 3 B oracle / 6 B candidate | — | 124 B frame, 2,900 → 2,935 differences, rejected |
| draw2dscreen | 3,202 | 56 / 52 | +0x9, 1 B oracle / 2 B candidate | — | 52 B frame, 2,726 → 2,810 differences, rejected |
| getzrange | 2,938 | 100 / 120 | +0x12, 4 B oracle / 1 B candidate | — | 120 B frame, 2,412 → 2,288 differences, retained |
| grouvline | 2,169 | 116 / 112 | +0xd8, 1 B oracle / 2 B candidate | — | 112 B frame, 1,626 → 1,478 differences, retained |
| rotatesprite | 1,497 | 92 / 96 | +0x66, 3 B oracle / 1 B candidate | — | 96 B frame, 1,225 → 1,296 differences, rejected |

Frame immediates of the same encoding length can differ before the first unequal-length instruction. The table identifies the first local width divergence and the first displaced physical destination independently. An intervening instruction can cancel an earlier length change. A single displaced target does not identify a unique causal statement.

- **checksectors**. The player argument occupies a stack home at entry. Reusing i for the initiator and w for the current sector on the second lift path restores MOV EDI,EAX at +0xc. The first unequal-length instruction moves to +0xed. The candidate is 128 B short and checkhitceiling loses its boundary proof.
- **operatesectors**. At +0x58 the sector index scale goes to ECX instead of EDI. The first unequal-length instruction at +0xa0 is in the case-26 tag update before the recursive call. Capturing case-19 speed as long reduces differences by 224, increases the frame by 8 B and worsens three inline callers. Reusing j for the door index reduces differences by 1,074 but loses checkhitceiling and worsens four callers. The original source cause of the case-26 schedule remains open.
- **cheatkeys**. The canonical source already reuses the decoy index as the debug-door loop counter. The next size change at +0x9fc is a word reload of the inlined sector argument where the oracle reloads a dword. Capturing a short sector index and a separate long initiator restores the 96 B frame and 7,604 B extent, but increases byte differences from 3,332 to 5,273.
- **menus**. By-value menu_main first lands at +0xa0 instead of +0x90. The second destination is ESP instead of ESP+0x30. Its pointer setup is 2 B shorter at +0x134. A switch in the later scores page corrects the earlier homes and removes all 3,469 positional differences. The last three pause-page targets required correcting the Q, Escape and choice-1 case labels.
- **operateactivators**. The 100 B frame exchanges the low and i homes relative to the oracle. At +0xaf the inlined sector dispatch loads the sector base while the oracle scales the index. A long traversal index plus reuse of j for case-9 speed produces the oracle 92 B frame, but is 4 B short and loses checkhitceiling. Reusing middle for cy alone saves 16 differences in this caller but loses the same peer and worsens four other functions.
- **getcliplines**. The 136 B reservation uses an imm32 at +3. The oracle reserves 124 B using imm8. Scoping the floor-sprite spans alone leaves the 136 B frame. Staging the x coordinates of both edge endpoints and limiting the floor x-offset to that case produces the 124 B frame, a function 15 B short, and eight lost STRICT peers. SemIR stops at the BSR range approximation.
- **draw2dscreen**. After the frame reservation, the oracle pushes the incoming zoom (ECX) at +9. The candidate moves it into EBP. Reusing yp1 for the translated sprite ordinate, changing the independent projection order and spelling the nonpositive visibility test explicitly gives the 52 B frame at the exact extent. The first difference moves to +9, but the function worsens by 84 B.
- **getzrange**. Separate floor-sprite corners and a separate y offset, together with reuse of dx for radius, restore the 120 B frame. Pair the x and y endpoint reads, initialize the sector queue and advance the append index separately. Ordering xmin, ymin, xmax, ymax moves the first differing byte to +0x26. The remaining instruction uses stack displacement 0x48 where the oracle uses 0x5c.
- **grouvline**. Reusing y1 for the increment swap restores the 112 B frame. Reusing tempy for the raster y value in both traversal loops restores the 2,169 B extent as well. First residual byte +0xc3. The first unequal symbolic instruction at +0xcc loads viewoffset before dmost instead of the oracle order.
- **rotatesprite**. The first baseline width change at +0x66 follows the tile-dimension loads. The oracle sign-extends the low width from DX, while the candidate uses CWDE for height. Giving ysiz its own long declaration below tileheight restores the 96 B frame, but the extent grows by 1 B and two STRICT peers fail. Splitting or reusing the per-column count also failed the unit guard.

Full switch vectors, source line anchors, SemIR diagnostics, input hashes and whole-unit rejection receipts are in `build/workers/blockers/cascade/census.json`. The DOS builds used the registered profiles and existing bindings.

This census is pinned to the canonical sources at 8958ccf. Two concurrent integrations made hitradius (304 bytes) and gamenumber (272 bytes) STRICT. Each affected unit was rebuilt and the latest source hashes were checked against the snapshot. The 85-function starting census at 87ab75f remains available with its own ranks, traces and first differences. [Starting census](SUPPLIED.md) · [Starting JSON](supplied.json) · [Results of source tests](predictions.md)

The 83 non-STRICT publics cover 207,256 oracle bytes. The 68 NEAR functions account for 104,874 positional byte differences reported by match.py. The other 15 have exact code and sites. Fourteen depend on unverified CONST contents in nine Watcom 10.0 audio units. setvesamode depends on the engine’s private BSS placement.

The biggest observable class is a relocated switch-table destination. For these functions the compiler placed the table before the public label. Its targets describe the lengths and layout of later code. In several entries the body first diverges at a frame reservation or a stack access. The table position alone establishes neither reordered cases nor missing C.

## Rankings

| First observable failure | Functions | Rank by count¹ | Oracle extent bytes | Verifier byte differences |
|---|---:|---:|---:|---:|
| Switch-table target | 11 | 3 | 73,520 | 42,733 |
| Stack-slot displacement | 15 | 1 | 53,054 | 28,386 |
| Frame reservation | 11 | 3 | 34,325 | 24,057 |
| Instruction or access order | 11 | 3 | 20,669 | 3,467 |
| Branch displacement | 7 | 7 | 13,342 | 3,218 |
| Register choice | 9 | 6 | 5,927 | 2,408 |
| CONST contents | 14 | 2 | 3,084 | 0 |
| Equal-savings conflict | 1 | 9 | 1,740 | 2 |
| Saved-register set | 3 | 8 | 1,303 | 603 |
| BSS target verification | 1 | 9 | 292 | 0 |

¹ Equal counts share a rank. Rows are ordered by total oracle extent. Each function contributes once. Extent measures the amount of code awaiting proof. The last column counts all positional differences across the functions in each row. One changed instruction can shift hundreds of later bytes. Relocation target failures and proof obligations are counted separately from differing bytes.

## What changes inside the body

For the 18 table- or branch-first functions, stepping past a relocated table and ignoring a changed distance on the same branch form gives a second ranking. The scan begins at the public and stops at the first difference in instruction length or symbolic operands. A moved instruction can reflect scheduling, an added expression, or a different spill. The ranking describes the machine evidence. Exact historical source is generally underdetermined.

| First local body difference or target obligation | Functions | Oracle extent bytes |
|---|---:|---:|
| Stack-slot displacement | 17 | 70,630 |
| Frame reservation | 16 | 58,053 |
| Instruction or access order | 15 | 28,155 |
| Register choice | 12 | 26,695 |
| Stack versus register | 1 | 11,448 |
| Operand selection or order | 1 | 3,984 |
| CONST contents | 14 | 3,084 |
| Equal-savings conflict | 1 | 1,740 |
| Address formation | 1 | 1,424 |
| Saved-register set | 3 | 1,303 |
| Branch shape | 1 | 448 |
| BSS target verification | 1 | 292 |

No whole-function first cause was assigned solely from a missing normalized block, an inline window, or a discrepancy later in SemIR traversal. Each remains independently useful as a falsifiable local lead.

## Which compiler rule applies

| Observed difference | Rule and evidentiary limit |
|---|---|
| Switch target or forward branch | A table field holds a relocated destination. A relative branch encodes the distance from its end. Either can change before the instruction whose length changed. Compare physical case spans before inferring a switch-order or predicate change. |
| Local reservation or slot displacement | The allocator selects eligible homes, tries sharing and lays out surviving homes by priority and size. The finalized frame fixes the offsets. A frame-size delta alone does not identify the extra local. Swapped displacements at equal frame size can be diagnosed by tracing the two holders. |
| Register, tie or save set | Conflicts are visited in descending savings order. Eligibility, overlapping ranges, register preferences and already-used state constrain the leaf choice. Changing one allocation can change the saved register set. An observed register substitution alone does not distinguish a tie from a higher score. |
| Instruction/access order | Independent loads or arithmetic can be scheduled around another operation. Aliasing, narrowing, evaluation order and live ranges constrain which reorderings are possible. A SemIR memory-order mismatch needs a field/width check before asserting that a C store was omitted. |
| Differing inline copy | The caller contributes live values and operand constraints to the expansion. A STRICT standalone callee is an independent control. The reverse evidence below identifies overlapping windows and shows which caller to remeasure. |
| Exact code with unverified target | One unverified literal pool blocks every referencing public. In the separate engine BSS case the private target still lacks a verified owning layout. The optimizer cannot discharge either proof requirement. |

cg95 produced ordinary-NT/DOS-controlled explanations for 58 Watcom 9.5 NEAR functions. An automatic mechanism lands on the earliest failing instruction in 12, on a separate machine anchor in 29, and has no unique anchor in 17. The case-405 manual tie is called out separately. The ten Watcom 10.0 NEAR functions use the registered audio profiles and their actual DOS output; the 9.5 observer has no hook map for that compiler. [Controlled rule details](RULES.md) · [Full-unit predictions](predictions.md)

## Method

Fresh serial DOS builds of all 22 affected canonical translation units used registered flags and headers through wcc.compile_c. Source/object hashes, all publics and segment reports are under baseline/. The corresponding blockdiff census agrees on all 83 names, statuses and diagnostic extents. Leading tables use verifier ownership. Offsets below start at the owned extent; the public can be later. A reference can fail before the first non-fixup byte. LINNUM source lines are nearby compiler anchors, including after scheduling or inlining. The JSON retains both output streams, independent resolution failures, SemIR results and locations.

The first failing byte or target is observed. A cg95 assignment is attributed to it only when the oracle instruction address agrees. A later traced allocation is labeled separately. SemIR walks the CFG and its first modeled effect can occur much later. Reverse lookup used the repository STRICT index and block pairing on line-enabled objects payload-checked against the fresh objects. An unmatched normalized twin shows a source lead and cannot by itself establish an omitted C statement. The full reverse records include source spans. Historical worker controls can use different sources and appear only as prior evidence.

## Function evidence

### ass_adlibfx / ADLIBFX_ErrorString

**CONST contents** at 1:627a2 (+0x2a from 1:62778, public 1:62778). Oracle extent 84 B, verifier differences 0 B, 0 verifier problems, 5 unverified obligations. mov eax,&S:'Adlib FX ok.'; literal pool 2:01e58; first byte difference at +d (9 bytes differ)

First obligation: `+2a -> segment CONST+0 (oracle 2:1e58; contents not verified)`.
Source anchor: src/audiolib/adlibfx.c:96, `ErrorString = "Adlib FX ok.";`.
Blockdiff pairs 10/10 oracle blocks, with 0 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Historical DOS path, make order, predecessor and header trials reproduced only subsets of padding. See the nine-pool residue solver. [docs, line 787](../../../docs/FINDINGS.md).

### ass_al_midi / AL_SetVoiceTimbre

**Register choice** at 1:60b30 (+0xc from 1:60b24, public 1:60b24). Oracle extent 1,112 B, verifier differences 696 B, 90 verifier problems, 1 unverified obligations. The first LEA scales voice into EBP versus EDI; the later control flow differs as well.

First instruction, candidate / oracle: `lea ebp,m4[+eax*4+0x0]` / `lea edi,m4[+eax*4+0x0]`.
Source anchor: src/audiolib/al_midi.c:256, `channel = Voice[ voice ].channel;`.
Blockdiff pairs 45/50 oracle blocks, with 6 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Fifty timer forms varied rate/flags declarations and loop order. Thirty-three timbre forms varied returns, narrowing and allocation. Neither function reached STRICT; the December ROTT timer body supplies an independent control. [assrott, line 87](../../../build/workers/assrott/NOTES.md).

### ass_al_midi / AL_SetVoiceVolume

**Instruction or access order** at 1:60ff7 (+0x7b from 1:60f7c, public 1:60f7c). Oracle extent 280 B, verifier differences 30 B, 6 verifier problems, 0 unverified obligations. A register copy precedes the VoiceKsl table load. The oracle loads VoiceKsl first.

First instruction, candidate / oracle: `mov ebx,ecx` / `mov eax,m4[D:VoiceKsl+edx]`.
Source anchor: src/audiolib/al_midi.c:395, `volume = level ^ 63;`.
Blockdiff pairs 8/10 oracle blocks, with 2 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: The left stereo arm matched; changing port locals and operands failed to reproduce mono and right-stereo evaluation. [libfix, line 27](../../../build/workers/libfix/NOTES.md).

### ass_al_midi / AL_Init

**Saved-register set** at 1:61c78 (+0x4 from 1:61c74, public 1:61c74). Oracle extent 518 B, verifier differences 256 B, 55 verifier problems, 0 unverified obligations. The entry saves EDI versus EBP before the initialization body.

First instruction, candidate / oracle: `push edi` / `push ebp`.
Source anchor: src/audiolib/al_midi.c:1540, `int AL_Init`.
Blockdiff pairs 13/15 oracle blocks, with 2 extra candidate blocks. [Reverse](baseline/ass_al_midi/reverse.json) finds 1 unmatched twins; earliest 1:61e71 matches PAS_GetCardSettings at 1:575fd.
Prior work: Under the registered wc100 profiles, PAS output-pointer spellings, GUS parser lifetimes, BLASTER-type selectors and task startup variants failed their respective whole-function comparisons. [units, line 15](../../../docs/units/audfix.md).

### ass_blastold / BLASTER_ErrorString

**CONST contents** at 1:5f076 (+0x6a from 1:5f00c, public 1:5f050). Oracle extent 212 B, verifier differences 0 B, 0 verifier problems, 16 unverified obligations. mov eax,&S:'Sound Blaster ok.'; literal pool 2:01b7c; first byte difference at +12 (5 bytes differ)

First obligation: `+6a -> segment CONST+0 (oracle 2:1b7c; contents not verified)`.
Source anchor: src/audiolib/blastold.c:175, `ErrorString = "Sound Blaster ok.";`.
Blockdiff pairs 21/21 oracle blocks, with 0 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Historical DOS path, make order, predecessor and header trials reproduced only subsets of padding. See the nine-pool residue solver. [docs, line 787](../../../docs/FINDINGS.md).

### ass_blastold / BLASTER_GetEnv

**CONST contents** at 1:60050 (+0x3c from 1:60014, public 1:60014). Oracle extent 444 B, verifier differences 0 B, 0 verifier problems, 6 unverified obligations. mov eax,&S:'BLASTER'; literal pool 2:01b7c; first byte difference at +12 (5 bytes differ)

First obligation: `+3c -> segment CONST+2b4 (oracle 2:1e30; contents not verified)`.
Source anchor: src/audiolib/blastold.c:1757, `Blaster = getenv( "BLASTER" );`.
Blockdiff pairs 41/41 oracle blocks, with 0 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Historical DOS path, make order, predecessor and header trials reproduced only subsets of padding. See the nine-pool residue solver. [docs, line 787](../../../docs/FINDINGS.md).

### ass_fx_man / FX_ErrorString

**CONST contents** at 1:5de1f (+0x2f from 1:5ddf0, public 1:5ddf0). Oracle extent 228 B, verifier differences 0 B, 0 verifier problems, 5 unverified obligations. mov edx,&S:'Fx ok.'; literal pool 2:01ab0; first byte difference at +7 (7 bytes differ)

First obligation: `+2f -> segment CONST+0 (oracle 2:1ab0; contents not verified)`.
Source anchor: src/audiolib/fx_man.c:96, `ErrorString = "Fx ok.";`.
Blockdiff pairs 19/19 oracle blocks, with 0 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Historical DOS path, make order, predecessor and header trials reproduced only subsets of padding. See the nine-pool residue solver. [docs, line 787](../../../docs/FINDINGS.md).

### ass_fx_man / FX_SetupCard

**Switch-table target** at 1:5dee0 (+0xc from 1:5ded4, public 1:5df04). Oracle extent 488 B, verifier differences 195 B, 15 verifier problems, 1 unverified obligations. The first mismatching table field points two bytes ahead/behind its expected case. The first body shape difference concerns the address of PAS capabilities.

First obligation: `+c: off32 -> seg:_TEXT+23f = 1:5dfdf, oracle 1:5dfe1`.
The first body operand, order or length change after discounting equal-form branch destinations is at +0xed: `lea edi,m4[edx+0x8]` / `lea eax,m4[edx+0x8]` near src/audiolib/fx_man.c:228.
Blockdiff pairs 18/20 oracle blocks, with 2 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Under the registered wc100 profiles, PAS output-pointer spellings, GUS parser lifetimes, BLASTER-type selectors and task startup variants failed their respective whole-function comparisons. [units, line 15](../../../docs/units/audfix.md).

### ass_gusmidi / GUS_ErrorString

**CONST contents** at 1:5d542 (+0x26 from 1:5d51c, public 1:5d51c). Oracle extent 100 B, verifier differences 0 B, 0 verifier problems, 7 unverified obligations. mov eax,&S:'Ultrasound music ok.'; literal pool 2:01984; first byte difference at +15 (5 bytes differ)

First obligation: `+26 -> segment CONST+0 (oracle 2:1984; contents not verified)`.
Source anchor: src/audiolib/gusmidi.c:124, `case GUS_Ok :`.
Blockdiff pairs 13/13 oracle blocks, with 0 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Historical DOS path, make order, predecessor and header trials reproduced only subsets of padding. See the nine-pool residue solver. [docs, line 787](../../../docs/FINDINGS.md).

### ass_gusmidi / GUS_GetPatchMap

**CONST contents** at 1:5d5ca (+0x4a from 1:5d580, public 1:5d580). Oracle extent 508 B, verifier differences 0 B, 0 verifier problems, 6 unverified obligations. mov eax,&S:'ULTRADIR'; literal pool 2:01984; first byte difference at +15 (5 bytes differ)

First obligation: `+4a -> segment CONST+f0 (oracle 2:1a74; contents not verified)`.
Source anchor: src/audiolib/gusmidi.c:198, ``.
Blockdiff pairs 31/31 oracle blocks, with 0 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Historical DOS path, make order, predecessor and header trials reproduced only subsets of padding. See the nine-pool residue solver. [docs, line 787](../../../docs/FINDINGS.md).

### ass_gusmidi / GUSMIDI_LoadPatch

**CONST contents** at 1:5d8a1 (+0xa1 from 1:5d800, public 1:5d800). Oracle extent 464 B, verifier differences 0 B, 0 verifier problems, 1 unverified obligations. mov esi,&S:'.pat'; literal pool 2:01984; first byte difference at +15 (5 bytes differ)

First obligation: `+a1 -> segment CONST+124 (oracle 2:1aa8; contents not verified)`.
Source anchor: src/audiolib/gusmidi.c:322, `strcat( text, ProgramName[ prog ] );`.
Blockdiff pairs 21/21 oracle blocks, with 0 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Historical DOS path, make order, predecessor and header trials reproduced only subsets of padding. See the nine-pool residue solver. [docs, line 787](../../../docs/FINDINGS.md).

### ass_guswave / GUSWAVE_ErrorString

**CONST contents** at 1:5cc06 (+0x46 from 1:5cbc0, public 1:5cbe0). Oracle extent 116 B, verifier differences 0 B, 0 verifier problems, 6 unverified obligations. mov eax,&S:'GUSWAVE ok.'; literal pool 2:018c0; first byte difference at +23 (7 bytes differ)

First obligation: `+46 -> segment CONST+0 (oracle 2:18c0; contents not verified)`.
Source anchor: src/audiolib/guswave.c:121, `ErrorString = "GUSWAVE ok.";`.
Blockdiff pairs 11/11 oracle blocks, with 0 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Historical DOS path, make order, predecessor and header trials reproduced only subsets of padding. See the nine-pool residue solver. [docs, line 787](../../../docs/FINDINGS.md).

### ass_guswave / GUSWAVE_GetNextVOCBlock

**Switch-table target** at 1:5d090 (+0x8 from 1:5d088, public 1:5d0b0). Oracle extent 436 B, verifier differences 270 B, 8 verifier problems, 1 unverified obligations. A leading fixup selects a shifted VOC block handler. The body reserves 0ch versus 8.

First obligation: `+8: off32 -> seg:_TEXT+622 = 1:5d162, oracle 1:5d164`.
The first body operand, order or length change after discounting equal-form branch destinations is at +0x2e: `sub esp,0xc` / `sub esp,0x8` near src/audiolib/guswave.c:618.
Blockdiff pairs 26/32 oracle blocks, with 5 extra candidate blocks. [Reverse](baseline/ass_guswave/reverse.json) finds 3 unmatched twins; earliest 1:5d1ed matches GUSWAVE_CallBack at 1:5cc84.
Prior work: Under the registered wc100 profiles, PAS output-pointer spellings, GUS parser lifetimes, BLASTER-type selectors and task startup variants failed their respective whole-function comparisons. [units, line 15](../../../docs/units/audfix.md).

### ass_guswave / GUSWAVE_PlayVOC

**Register choice** at 1:5d243 (+0x7 from 1:5d23c, public 1:5d23c). Oracle extent 372 B, verifier differences 5 B, 0 verifier problems, 0 unverified obligations. The incoming sound pointer is copied from EAX to EDI versus EBP.

First instruction, candidate / oracle: `mov edi,eax` / `mov ebp,eax`.
Source anchor: src/audiolib/guswave.c:756, `int GUSWAVE_PlayVOC`.
Blockdiff pairs 10/10 oracle blocks, with 0 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Register qualifiers, parameter aliases and alternate header offsets left EDI and EBP exchanged for the sample and pitch values. [libfix, line 26](../../../build/workers/libfix/NOTES.md).

### ass_multivoc / MV_SetMixMode

**Register choice** at 1:59e6a (+0x21 from 1:59e49, public 1:59e59). Oracle extent 525 B, verifier differences 2 B, 0 verifier problems, 0 unverified obligations. The constant 2 is materialized in ECX versus EBP under the registered audio build.

First instruction, candidate / oracle: `mov ecx,0x2` / `mov ebp,0x2`.
Source anchor: src/audiolib/multivoc.c:1081, `MV_SetErrorCode( MV_NotInstalled );`.
Blockdiff pairs 36/36 oracle blocks, with 0 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Clearing DemandFeed and reading the header offset first helped playback. Merged clamps and qualified arguments missed its 16-byte frame. Two register bytes persisted in mix-mode setting. [libfix, line 28](../../../build/workers/libfix/NOTES.md).

### ass_multivoc / MV_PlayVOC

**Frame reservation** at 1:5a66c (+0x7 from 1:5a665, public 1:5a665). Oracle extent 523 B, verifier differences 243 B, 8 verifier problems, 0 unverified obligations. The VOC playback entry reserves 8 versus 10h. The oracle reserves eight additional bytes.

First instruction, candidate / oracle: `sub esp,0x8` / `sub esp,0x10`.
Source anchor: src/audiolib/multivoc.c:1636, `int MV_PlayVOC`.
Blockdiff pairs 40/45 oracle blocks, with 6 extra candidate blocks. [Reverse](baseline/ass_multivoc/reverse.json) finds 2 unmatched twins; earliest 1:5a684 matches MV_Kill at 1:59a72.
Prior work: Clearing DemandFeed and reading the header offset first helped playback. Merged clamps and qualified arguments missed its 16-byte frame. Two register bytes persisted in mix-mode setting. [libfix, line 28](../../../build/workers/libfix/NOTES.md).

### ass_music / MUSIC_ErrorString

**CONST contents** at 1:58bf6 (+0x2e from 1:58bc8, public 1:58bc8). Oracle extent 176 B, verifier differences 0 B, 0 verifier problems, 10 unverified obligations. mov eax,&S:'Music ok.'; literal pool 2:016f0; first byte difference at +a (16 bytes differ)

First obligation: `+2e -> segment CONST+0 (oracle 2:16f0; contents not verified)`.
Source anchor: src/audiolib/music.c:102, `ErrorString = "Music ok.";`.
Blockdiff pairs 21/21 oracle blocks, with 0 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Historical DOS path, make order, predecessor and header trials reproduced only subsets of padding. See the nine-pool residue solver. [docs, line 787](../../../docs/FINDINGS.md).

### ass_pas16 / PAS_ErrorString

**CONST contents** at 1:573e6 (+0x56 from 1:57390, public 1:573c0). Oracle extent 156 B, verifier differences 0 B, 0 verifier problems, 10 unverified obligations. mov eax,&S:'Pro AudioSpectrum ok.'; literal pool 2:015b4; first byte difference at +16 (6 bytes differ)

First obligation: `+56 -> segment CONST+0 (oracle 2:15b4; contents not verified)`.
Source anchor: src/audiolib/pas16.c:155, `ErrorString = "Pro AudioSpectrum ok.";`.
Blockdiff pairs 15/15 oracle blocks, with 0 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Historical DOS path, make order, predecessor and header trials reproduced only subsets of padding. See the nine-pool residue solver. [docs, line 787](../../../docs/FINDINGS.md).

### ass_pcfx / PCFX_ErrorString

**CONST contents** at 1:56e02 (+0x2a from 1:56dd8, public 1:56dd8). Oracle extent 84 B, verifier differences 0 B, 0 verifier problems, 5 unverified obligations. mov eax,&S:'PC FX ok.'; literal pool 2:0152c; first byte difference at +a (6 bytes differ)

First obligation: `+2a -> segment CONST+0 (oracle 2:152c; contents not verified)`.
Source anchor: src/audiolib/pcfx.c:94, `ErrorString = "PC FX ok.";`.
Blockdiff pairs 10/10 oracle blocks, with 0 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Historical DOS path, make order, predecessor and header trials reproduced only subsets of padding. See the nine-pool residue solver. [docs, line 787](../../../docs/FINDINGS.md).

### ass_sndsrc / SS_ErrorString

**CONST contents** at 1:564ca (+0x26 from 1:564a4, public 1:564a4). Oracle extent 80 B, verifier differences 0 B, 0 verifier problems, 5 unverified obligations. mov eax,&S:'Sound Source ok.'; literal pool 2:01480; first byte difference at +11 (9 bytes differ)

First obligation: `+26 -> segment CONST+0 (oracle 2:1480; contents not verified)`.
Source anchor: src/audiolib/sndsrc.c:102, `ErrorString = "Sound Source ok.";`.
Blockdiff pairs 10/10 oracle blocks, with 0 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Historical DOS path, make order, predecessor and header trials reproduced only subsets of padding. See the nine-pool residue solver. [docs, line 787](../../../docs/FINDINGS.md).

### ass_sndsrc / SS_DetectSoundSource

**CONST contents** at 1:567c8 (+0x8 from 1:567c0, public 1:567c0). Oracle extent 212 B, verifier differences 0 B, 0 verifier problems, 3 unverified obligations. mov eax,&S:'ss1'; literal pool 2:01480; first byte difference at +11 (9 bytes differ)

First obligation: `+8 -> segment CONST+84 (oracle 2:1504; contents not verified)`.
Source anchor: src/audiolib/sndsrc.c:451, `if ( USER_CheckParameter( SELECT_SOUNDSOURCE_PORT1 ) )`.
Blockdiff pairs 14/14 oracle blocks, with 0 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Historical DOS path, make order, predecessor and header trials reproduced only subsets of padding. See the nine-pool residue solver. [docs, line 787](../../../docs/FINDINGS.md).

### ass_sndsrc / SS_Init

**CONST contents** at 1:568de (+0x4a from 1:56894, public 1:56894). Oracle extent 220 B, verifier differences 0 B, 0 verifier problems, 1 unverified obligations. mov eax,&S:'sst'; literal pool 2:01480; first byte difference at +11 (9 bytes differ)

First obligation: `+4a -> segment CONST+90 (oracle 2:1510; contents not verified)`.
Source anchor: src/audiolib/sndsrc.c:511, `if ( ( soundcard == TandySoundSource ) ||`.
Blockdiff pairs 12/12 oracle blocks, with 0 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Historical DOS path, make order, predecessor and header trials reproduced only subsets of padding. See the nine-pool residue solver. [docs, line 787](../../../docs/FINDINGS.md).

### ass_task_man / TS_SetTimerToMaxTaskRate

**Instruction or access order** at 1:55c96 (+0x6 from 1:55c90, public 1:55c90). Oracle extent 60 B, verifier differences 7 B, 2 verifier problems, 0 unverified obligations. After CLI, the candidate saves EAX to EBX before the PIT divisor setup. The oracle loads 10000h into EDX first.

First instruction, candidate / oracle: `mov ebx,eax` / `mov edx,0x10000`.
Source anchor: src/audiolib/task_man.c:265, `flags = DisableInterrupts();`.
Blockdiff pairs 6/7 oracle blocks, with 1 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Fifty timer forms varied rate/flags declarations and loop order. Thirty-three timbre forms varied returns, narrowing and allocation. Neither function reached STRICT; the December ROTT timer body supplies an independent control. [assrott, line 87](../../../build/workers/assrott/NOTES.md).

### ass_task_man / TS_Startup

**Saved-register set** at 1:55e2f (+0x3 from 1:55e2c, public 1:55e2c). Oracle extent 192 B, verifier differences 106 B, 22 verifier problems, 1 unverified obligations. The candidate also saves ESI before reading TS_Installed. The exact December ROTT comparison is separate provenance.

First instruction, candidate / oracle: `push esi` / `cmp m1[D:TS_Installed],0x0`.
Source anchor: src/audiolib/task_man.c:499, `static int TS_Startup`.
Blockdiff pairs 3/8 oracle blocks, with 5 extra candidate blocks. [Reverse](baseline/ass_task_man/reverse.json) finds 1 unmatched twins; earliest 1:55ee5 matches keyword at 1:2e896.
Prior work: Under the registered wc100 profiles, PAS output-pointer spellings, GUS parser lifetimes, BLASTER-type selectors and task startup variants failed their respective whole-function comparisons. [units, line 15](../../../docs/units/audfix.md).

### engine / scansector

**Register choice** at 1:343e2 (+0x12a from 1:342b8, public 1:342b8). Oracle extent 756 B, verifier differences 7 B, 0 verifier problems, 0 unverified obligations. The sign-extended p2[z] load uses EDX versus EDI. Seven bytes differ. A later traced interference need not explain the first choice.

First instruction, candidate / oracle: `movsx edx,m2[D:p2+ebx*2]` / `movsx edi,m2[D:p2+ebx*2]`.
Source anchor: src/engine/ENGINE.C:460, `if ((point2 != thewall[p2[z]]) || (screenright = xb2[z], screenright >= xb1[p2[z]]))`.
[cg95](traces/scansector/explain.json) attributes register interference (point2) at 1:343e8; this is a separate machine anchor.
Blockdiff pairs 36/36 oracle blocks, with 0 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Point2 and screen-bound lifetime changes left scansector at seven bytes. Product, shade and smost staging failed the whole-unit drawmaskwall criterion. [r4multi, line 55](../../../build/workers/r4multi/NOTES.md).

### engine / wallfront

**Register choice** at 1:345b6 (+0xa from 1:345ac, public 1:345ac). Oracle extent 554 B, verifier differences 414 B, 10 verifier problems, 0 unverified obligations. The sign extension of l1 uses EAX via CWDE versus EDX; the selected colour has a higher score in this source.

First instruction, candidate / oracle: `cwde ` / `movsx edx,ax`.
Source anchor: src/engine/ENGINE.C:479, `wal = &wall[thewall[l1]]; x1l1 = wal->x; y1l1 = wal->y;`.
[cg95](traces/wallfront/explain.json) attributes colour score (t18 (l1)) at 1:345b6 in the first instruction.
Blockdiff pairs 14/19 oracle blocks, with 5 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: The l1 conversion scored 2 for EAX and 0 for the desired EDX. Changes to wall orientation forms also disturbed neighbouring jumps. [docs, line 49](../../../docs/CG95.md).

### engine / screencapture

**Stack-slot displacement** at 1:3b452 (+0xc2 from 1:3b390, public 1:3b390). Oracle extent 1,511 B, verifier differences 314 B, 8 verifier problems, 0 unverified obligations. An EAX value is written at +20h versus +14h near capturecount increment. LINNUM marks a region rather than identifying its producer.

First instruction, candidate / oracle: `mov m4[esp+0x20],eax` / `mov m4[esp+0x14],eax`.
Source anchor: src/engine/ENGINE.C:2967, `capturecount++;`.
[cg95](traces/screencapture/explain.json) found no unique decision anchor.
Blockdiff pairs 79/81 oracle blocks, with 2 extra candidate blocks. [Reverse](baseline/engine/reverse.json) finds 1 unmatched twins; earliest 1:3b732 matches wallscan at 1:3808f.
Prior work: The earlier trace identified an inverseit slot. Declaration permutations, mode tests and pointer reshaping did not recover both mode branches and their homes. [r4multi, line 55](../../../build/workers/r4multi/NOTES.md).

### engine / drawmaskwall

**Instruction or access order** at 1:3c406 (+0xe4 from 1:3c322, public 1:3c322). Oracle extent 1,890 B, verifier differences 276 B, 28 verifier problems, 0 unverified obligations. The first slot in the stream copies a register where the oracle negates a value for topinc. Later repeat spills are separate observations.

First instruction, candidate / oracle: `mov edx,ebp` / `neg eax`.
Source anchor: src/engine/ENGINE.C:3456, `topinc = xrepeat * -ry1[z];`.
[cg95](traces/drawmaskwall/explain.json) found no unique decision anchor.
Blockdiff pairs 75/79 oracle blocks, with 4 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Point2 and screen-bound lifetime changes left scansector at seven bytes. Product, shade and smost staging failed the whole-unit drawmaskwall criterion. [r4multi, line 55](../../../build/workers/r4multi/NOTES.md).

### engine / drawsprite

**Stack-slot displacement** at 1:3caf4 (+0x70 from 1:3ca84, public 1:3ca84). Oracle extent 9,875 B, verifier differences 7,056 B, 528 verifier problems, 0 unverified obligations. The low byte saved after reading sprite cstat goes to a different stack position.

First instruction, candidate / oracle: `mov m1[esp+0xd0],al` / `mov m1[esp+0xd4],al`.
Source anchor: src/engine/ENGINE.C:3594, `cstat = sprite[spritenum].cstat;`.
[cg95](traces/drawsprite/explain.json) attributes register interference (t1676) at 1:3d26d; this is a separate machine anchor.
Blockdiff pairs 326/377 oracle blocks, with 57 extra candidate blocks. [Reverse](baseline/engine/reverse.json) finds 3 unmatched twins; earliest 1:3e9b7 matches addboxlinum at 1:36e8d.
Prior work: Repeat and shade staging improved tested shapes but crossed the permitted extent or regressed neighbours in the larger controls. [r6cx-engine, line 22](../../../build/workers/r6cx-engine/NOTES.md).

### engine / overwritesprite

**Instruction or access order** at 1:3f6c2 (+0x49 from 1:3f679, public 1:3f679). Oracle extent 1,063 B, verifier differences 687 B, 34 verifier problems, 0 unverified obligations. The candidate finishes sign-extending shade before it clears the destination for a separate byte load. The oracle interleaves these operations.

First instruction, candidate / oracle: `movsx eax,cl` / `xor eax,eax`.
Source anchor: src/engine/ENGINE.C:4452, `palookupoffs = ((long)shade)<<8;`.
[cg95](traces/overwritesprite/explain.json) attributes stack home ordering (palookupoffs, other holder xsiz) at 1:3f6d7; this is a separate machine anchor.
Blockdiff pairs 52/59 oracle blocks, with 6 extra candidate blocks. [Reverse](baseline/engine/reverse.json) finds 1 unmatched twins; earliest 1:3f94f matches ceilscan at 1:373a8.
Prior work: September-donor-guided endpoint/raster edits improved byte counts. Both bodies remained NEAR. [docs, line 519](../../../docs/FINDINGS.md).

### engine / grouvline

**Frame reservation** at 1:3fff3 (+0x5 from 1:3ffee, public 1:3ffee). Oracle extent 2,169 B, verifier differences 1,626 B, 63 verifier problems, 0 unverified obligations. 74h versus 70h. The frame contains dir, dist, dinc, incr and grid arrays plus transient row stride.

First instruction, candidate / oracle: `sub esp,0x74` / `sub esp,0x70`.
Source anchor: src/engine/ENGINE.C:4691, `grouvline (long xscr, long d1, long d2, short sectnum)`.
[cg95](traces/grouvline/explain.json) attributes colour score (t342) at 1:400dc; this is a separate machine anchor.
Blockdiff pairs 73/89 oracle blocks, with 16 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Changing the projection sum and placing the stride before the arrays helped grouvline. Reversing intersection-local order improved hitscan. Frames and later register shapes still differed. [r6cx-engine, line 18](../../../build/workers/r6cx-engine/NOTES.md).

### engine / hitscan

**Stack-slot displacement** at 1:411db (+0x17 from 1:411c4, public 1:411c4). Oracle extent 1,745 B, verifier differences 1,056 B, 21 verifier problems, 0 unverified obligations. The incoming x coordinate is stored at +48h versus +4ch.

First instruction, candidate / oracle: `mov m4[esp+0x48],eax` / `mov m4[esp+0x4c],eax`.
Source anchor: src/engine/ENGINE.C:5204, `hitscan(long xs, long ys, long zs, short sectnum, long vx, long vy, long vz,`.
[cg95](traces/hitscan/explain.json) attributes stack home ordering (xs, other holder ys) at 1:411d8 in the first instruction.
Blockdiff pairs 75/82 oracle blocks, with 7 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Changing the projection sum and placing the stride before the arrays helped grouvline. Reversing intersection-local order improved hitscan. Frames and later register shapes still differed. [r6cx-engine, line 18](../../../build/workers/r6cx-engine/NOTES.md).

### engine / neartag

**Stack-slot displacement** at 1:418ad (+0x18 from 1:41895, public 1:41895). Oracle extent 1,644 B, verifier differences 201 B, 8 verifier problems, 0 unverified obligations. The third incoming coordinate is stored at +20h versus +1ch.

First instruction, candidate / oracle: `mov m4[esp+0x20],ebx` / `mov m4[esp+0x1c],ebx`.
Source anchor: src/engine/ENGINE.C:5362, `neartag (long xs, long ys, long zs, short sectnum, short ange, short *neartagsector, short *neartagwall, short…`.
[cg95](traces/neartag/explain.json) attributes stack home ordering (zs, other holder vz) at 1:418aa in the first instruction.
Blockdiff pairs 43/47 oracle blocks, with 4 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Earlier queue index initialization improved neartag. A three-byte palette cursor and additive RGB offset reduced makepalookup’s fixup errors without closing it. [r4multi, line 53](../../../build/workers/r4multi/NOTES.md).

### engine / getcliplines

**Frame reservation** at 1:4258c (+0x3 from 1:42589, public 1:42589). Oracle extent 3,589 B, verifier differences 2,900 B, 128 verifier problems, 0 unverified obligations. The candidate reserves 88h versus 7ch for wall and sprite clipping calculations. The specific three dwords cannot be identified from SUB alone.

First instruction, candidate / oracle: `sub esp,0x88` / `sub esp,0x7c`.
Source anchor: src/engine/ENGINE.C:5694, `getcliplines(long x, long y, long z, long goalx, long goaly, short sectnum, long walldist, long ceildist, long flordist, char…`.
[cg95](traces/getcliplines/explain.json) attributes register interference (y2) at 1:42711; this is a separate machine anchor.
Blockdiff pairs 73/122 oracle blocks, with 48 extra candidate blocks. [Reverse](baseline/engine/reverse.json) finds 8 unmatched twins; earliest 1:427c6 matches MV_Init at 1:5ad5d.
Prior work: Direct indexed sprite fields restored earlier reference patterns. Direct seeding omitted the redundant zero store, while face/wall shared height checks improved getzrange. Different frame totals survived. [r6cx-engine, line 16](../../../build/workers/r6cx-engine/NOTES.md).

### engine / raytrace

**Instruction or access order** at 1:435ad (+0x7b from 1:43532, public 1:43532). Oracle extent 613 B, verifier differences 108 B, 0 verifier problems, 0 unverified obligations. The candidate inserts a register copy before reloading the other clip endpoint. SemIR agrees on the modeled effects.

First instruction, candidate / oracle: `mov ebx,eax` / `mov eax,m4[esp+0x28]`.
Source anchor: src/engine/ENGINE.C:5942, `x2 = clipx2[z]; y2 = clipy2[z];`.
[cg95](traces/raytrace/explain.json) attributes stack home ordering (y4, other holder x1) at 1:435cb; this is a separate machine anchor.
Blockdiff pairs 21/24 oracle blocks, with 3 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: September-donor-guided endpoint/raster edits improved byte counts. Both bodies remained NEAR. [docs, line 519](../../../docs/FINDINGS.md).

### engine / drawline16

**Register choice** at 1:439b5 (+0x78 from 1:4393d, public 1:4393d). Oracle extent 1,541 B, verifier differences 935 B, 7 verifier problems, 0 unverified obligations. The first loaded endpoint home agrees; the destination is EBP versus ESI for the y-endpoint swap.

First instruction, candidate / oracle: `mov ebp,m4[esp+0x8]` / `mov esi,m4[esp+0x8]`.
Source anchor: src/engine/ENGINE.C:6053, `if (y2 < y1) i = y1, y1 = y2, y2 = i;`.
[cg95](traces/drawline16/explain.json) found no unique decision anchor.
Blockdiff pairs 81/91 oracle blocks, with 9 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Two planar-fill expressions failed to improve the dword-plus-byte fill. The displaced neighbour was a separate layout effect. [units, line 63](../../../docs/units/engfix2_round2.md).

### engine / draw2dscreen

**Frame reservation** at 1:44463 (+0x5 from 1:4445e, public 1:4445e). Oracle extent 3,202 B, verifier differences 2,726 B, 62 verifier problems, 0 unverified obligations. 38h versus 34h for screen endpoints, projected coordinates and colour. Earlier donor substitutions affected neighbouring STRICT bodies.

First instruction, candidate / oracle: `sub esp,0x38` / `sub esp,0x34`.
Source anchor: src/engine/ENGINE.C:6283, `draw2dscreen(long posxe, long posye, short ange, long zoome, short gride)`.
[cg95](traces/draw2dscreen/explain.json) attributes colour score (j) at 1:444e9; this is a separate machine anchor.
Blockdiff pairs 33/65 oracle blocks, with 29 extra candidate blocks. [Reverse](baseline/engine/reverse.json) finds 4 unmatched twins; earliest 1:44533 matches calcwalls at 1:34be4.
Prior work: Donor-guided forms changed extents and regressed STRICT neighbours. The shared returns make full-unit verification material. [docs, line 521](../../../docs/FINDINGS.md).

### engine / getzrange

**Frame reservation** at 1:4566c (+0x5 from 1:45667, public 1:45667). Oracle extent 2,938 B, verifier differences 2,412 B, 71 verifier problems, 0 unverified obligations. 64h versus 78h across wall and sprite clipping; semir first sees a different effect in the sector list.

First instruction, candidate / oracle: `sub esp,0x64` / `sub esp,0x78`.
Source anchor: src/engine/ENGINE.C:6626, `getzrange(long x, long y, long z, short sectnum,`.
[cg95](traces/getzrange/explain.json) attributes register interference (t479) at 1:4577c; this is a separate machine anchor.
Blockdiff pairs 45/107 oracle blocks, with 58 extra candidate blocks. [Reverse](baseline/engine/reverse.json) finds 17 unmatched twins; earliest 1:457aa matches bunchfront at 1:3489e.
Prior work: Direct indexed sprite fields restored earlier reference patterns. Direct seeding omitted the redundant zero store, while face/wall shared height checks improved getzrange. Different frame totals survived. [r6cx-engine, line 16](../../../build/workers/r6cx-engine/NOTES.md).

### engine / setview

**Register choice** at 1:4637b (+0xb5 from 1:462c6, public 1:462c6). Oracle extent 334 B, verifier differences 9 B, 3 verifier problems, 0 unverified obligations. The first differing copy targets EAX versus EDX in the view span/stride calculation. Historical controls changed the colour at the cost of the span.

First instruction, candidate / oracle: `mov eax,esi` / `mov edx,esi`.
Source anchor: src/engine/ENGINE.C:6837, `startdmost[i] = j;`.
[cg95](traces/setview/explain.json) attributes instruction order at 1:463a6; this is a separate machine anchor.
Blockdiff pairs 13/14 oracle blocks, with 1 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Reusing j corrected six stride bytes and introduced nine span differences. Declaration order, use counts, statement order and address-taking failed the whole-unit target. [docs, line 524](../../../docs/FINDINGS.md).

### engine / rotatesprite

**Frame reservation** at 1:46c65 (+0x5 from 1:46c60, public 1:46c60). Oracle extent 1,497 B, verifier differences 1,225 B, 22 verifier problems, 0 unverified obligations. 5ch versus 60h. The raster loop’s count, pitch, height and the known external/shared returns impose additional constraints.

First instruction, candidate / oracle: `sub esp,0x5c` / `sub esp,0x60`.
Source anchor: src/engine/ENGINE.C:7013, `rotatesprite(long sx, long sy, long z, short a, short picnum)`.
[cg95](traces/rotatesprite/explain.json) attributes colour score (t233) at 1:46fab; this is a separate machine anchor.
Blockdiff pairs 53/75 oracle blocks, with 24 extra candidate blocks. [Reverse](baseline/engine/reverse.json) finds 1 unmatched twins; earliest 1:47037 matches addboxlinum at 1:36dbf.
Prior work: Donor-guided forms changed extents and regressed STRICT neighbours. The shared returns make full-unit verification material. [docs, line 521](../../../docs/FINDINGS.md).

### engine / makepalookup

**Stack-slot displacement** at 1:47244 (+0xb from 1:47239, public 1:47239). Oracle extent 599 B, verifier differences 194 B, 7 verifier problems, 0 unverified obligations. Incoming byte r in BL is stored to +1ch versus +24h. The first mismatching byte belongs to its home displacement.

First instruction, candidate / oracle: `mov m1[esp+0x1c],bl` / `mov m1[esp+0x24],bl`.
Source anchor: src/engine/ENGINE.C:7169, `makepalookup(long palnum, char *remapbuf, signed char r, signed char g, signed char b)`.
[cg95](traces/makepalookup/explain.json) attributes equal-score colour choice (t47) at 1:47300; this is a separate machine anchor.
Blockdiff pairs 25/28 oracle blocks, with 3 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Earlier queue index initialization improved neartag. A three-byte palette cursor and additive RGB offset reduced makepalookup’s fixup errors without closing it. [r4multi, line 53](../../../build/workers/r4multi/NOTES.md).

### engine / setvesamode

**BSS target verification** at 1:474d3 (+0x43 from 1:47490, public 1:47490). Oracle extent 292 B, verifier differences 0 B, 0 verifier problems, 8 unverified obligations. The private real-mode RMI buffer has the expected code reference. The owning _BSS region lacks independent verification.

First obligation: `+43 -> segment _BSS+10c09f (oracle 2:1bde9f; contents not verified)`.
Source anchor: src/engine/ENGINE.C:7253, `memset(&RMI,0,sizeof(RMI));    // Set up real-mode call structure`.
Blockdiff pairs 7/8 oracle blocks, with 1 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Giving the private VESA buffer a public name changed BSS placement. No independent proof of its original private layout resulted. [docs, line 315](../../../docs/FINDINGS.md).

### game_actors / ms

**Stack-slot displacement** at 1:2575c (+0xc from 1:25750, public 1:25750). Oracle extent 368 B, verifier differences 106 B, 10 verifier problems, 0 unverified obligations. i and endwall have equal-priority homes at +0ch and +8. The common endpoint value affects the rotatepoint/dragpoint loop.

First instruction, candidate / oracle: `mov m4[esp+0xc],eax` / `mov m4[esp+0x8],eax`.
Source anchor: src/game/actors.c:159, `void ms(short i)`.
[cg95](refresh/r10/traces/ms/explain.json) attributes stack home ordering (i, other holder endwall) at 1:25759 in the first instruction.
Blockdiff pairs 1/4 oracle blocks, with 3 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Moving wall/j increments and changing tx/ty order improved the loop in one trial without producing STRICT. The historical target keeps endwall in EDI across rotatepoint. [r2pam, line 78](../../../build/workers/r2pam/NOTES.md).

### game_actors / moveactors

**Switch-table target** at 1:25ee8 (+0x0 from 1:25ee8, public 1:25f10). Oracle extent 12,360 B, verifier differences 5,889 B, 496 verifier problems, 0 unverified obligations. The first relocated case entry is off by one. The first post-entry operand difference is the address of a stack local near the sector read and findplayer call. The SHARK paths have independently measured case drift.

First obligation: `+0: off32 -> seg:_TEXT+11af = 1:2630f, oracle 1:2630e`.
The first body operand, order or length change after discounting equal-form branch destinations is at +0x54: `lea edx,m4[esp+0x10]` / `lea edx,m4[esp+0x14]` near src/game/actors.c:333.
[cg95](refresh/r10/traces/moveactors/explain.json) attributes stack home ordering (dist) at 1:25f3c; this is a separate machine anchor.
Blockdiff pairs 479/521 oracle blocks, with 42 extra candidate blocks. [Reverse](refresh/r10/baseline/game_actors/reverse.json) finds 1 unmatched twins; earliest 1:26b09 matches checkhitceiling at 1:1c3e5. 3 differing inline windows include ifhitbyweapon at 1:26af1.
Prior work: Local order and narrowing the scope of l improved the body slightly. [r10-actors, line 1](../../../build/workers/r10-actors/NOTES.md).

### game_actors / moveexplosions

**Branch displacement** at 1:294f5 (+0x15 from 1:294e0, public 1:294e0). Oracle extent 3,984 B, verifier differences 673 B, 54 verifier problems, 0 unverified obligations. The same status-list JL changes distance. The next non-distance difference is in the ssp expansion, which loads a different sprite field at extent +35ah.

First instruction, candidate / oracle: `jl -> extent +0xf85` / `jl -> extent +0xf7c`.
The first body operand, order or length change after discounting equal-form branch destinations is at +0x35a: `movsx ebx,m2[D@2:146f28+eax]` / `mov bx,m2[D@2:146f22+eax]` near src/game/actors.c:1160.
[cg95](refresh/r10/traces/moveexplosions/explain.json) attributes register interference (t181) at 1:29847; this is a separate machine anchor.
Blockdiff pairs 162/167 oracle blocks, with 5 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin. 5 differing inline windows include ssp at 1:2982c.
Prior work: Moving p/x to function scope, expanding one movement call, staging the low-word blood angle and masking FLAIR cut 2,035 differences to 673. [r10-actors, line 1](../../../build/workers/r10-actors/NOTES.md).

### game_actors / movestandables

**Frame reservation** at 1:2ab98 (+0x8 from 1:2ab90, public 1:2ab90). Oracle extent 3,735 B, verifier differences 2,381 B, 240 verifier problems, 1 unverified obligations. The reserve is 1ch versus 18h for a function with nexti, sect, p and a nearest-player distance.

First instruction, candidate / oracle: `sub esp,0x1c` / `sub esp,0x18`.
Source anchor: src/game/actors.c:1460, `void movestandables(void)`.
[cg95](refresh/r10/traces/movestandables/explain.json) attributes colour score (t149) at 1:2aec4; this is a separate machine anchor.
Blockdiff pairs 128/155 oracle blocks, with 30 extra candidate blocks. [Reverse](refresh/r10/baseline/game_actors/reverse.json) finds 3 unmatched twins; earliest 1:2b9eb matches addweapon at 1:2518e. 4 differing inline windows include ifhitbyweapon at 1:2ac0a.
Prior work: A sector temporary lengthened the body by 17 bytes in that control. Reshaping the sector-list walk as a while loop produced no change. [gamefix5, line 97](../../../build/workers/gamefix5/NOTES.md).

### game_game / waitforeverybody

**Branch displacement** at 1:01171 (+0x51 from 1:01120, public 1:01120). Oracle extent 1,952 B, verifier differences 687 B, 77 verifier problems, 0 unverified obligations. The connecthead split jumps a different distance. In the first expanded getpackets path a register copy precedes the constant 4 that the oracle loads in EBP.

First instruction, candidate / oracle: `jne -> extent +0x41f` / `jne -> extent +0x41b`.
The first body operand, order or length change after discounting equal-form branch destinations is at +0x172: `mov ecx,edx` / `mov ebp,0x4` near src/game/game.c:521.
[cg95](refresh/r10game/traces/waitforeverybody/explain.json) found no unique decision anchor.
Blockdiff pairs 90/94 oracle blocks, with 4 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin. 2 differing inline windows include getpackets at 1:011c9.
Prior work: A separate broadcast-list index reduced 1,100 differences to 687. [r10-game, line 1](../../../build/workers/r10-game/NOTES.md).

### game_game / sync

**Branch displacement** at 1:018d8 (+0x18 from 1:018c0, public 1:018c0). Oracle extent 2,832 B, verifier differences 730 B, 112 verifier problems, 0 unverified obligations. The connecthead branch reaches another offset. A getpackets inline and packet field store order differ deeper in the body.

First instruction, candidate / oracle: `jne -> extent +0x62a` / `jne -> extent +0x627`.
The first body operand, order or length change after discounting equal-form branch destinations is at +0x9c: `mov ecx,edx` / `mov ebp,0x1` near src/game/game.c:546.
[cg95](refresh/r10game/traces/sync/explain.json) attributes colour score (t408) at 1:01de6; this is a separate machine anchor.
Blockdiff pairs 114/125 oracle blocks, with 11 extra candidate blocks. [Reverse](refresh/r10game/baseline/game_game/reverse.json) finds 1 unmatched twins; earliest 1:01de6 matches domovethings at 1:09122. 2 differing inline windows include getpackets at 1:018e8.
Prior work: Restoring the master packet length as a short yielded 730 differences and the 2,832-byte extent. [r10-game, line 1](../../../build/workers/r10-game/NOTES.md).

### game_game / operatefta

**Stack-slot displacement** at 1:02474 (+0x74 from 1:02400, public 1:02400). Oracle extent 560 B, verifier differences 4 B, 0 verifier problems, 0 unverified obligations. The two minitext origins occupy exchanged slots. Fresh explain identifies left and t102 with equal home priority.

First instruction, candidate / oracle: `mov m4[esp+0x14],edi` / `mov m4[esp+0xc],edi`.
Source anchor: src/game/game.c:650, `user_quote_time--;`.
[cg95](refresh/r10game/traces/operatefta/explain.json) attributes stack home ordering (left, other holder t102) at 1:02471 in the first instruction.
Blockdiff pairs 28/28 oracle blocks, with 0 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin. 1 differing inline windows include minitext at 1:02544.
Prior work: Scoping the line origin after the dimension guard reduced the discrepancy to four slot bytes. Width, named coordinates/timers, condition reversals and glyph declaration changes failed to solve both homes. [followup, line 31](../../../build/workers/r9astra-game/followup/NOTES.md).

### game_game / strget

**Stack-slot displacement** at 1:02a49 (+0x9 from 1:02a40, public 1:02a40). Oracle extent 1,024 B, verifier differences 525 B, 22 verifier problems, 0 unverified obligations. The saved incoming x now occupies +18h versus +8h after the caller-specific small-font shade change.

First instruction, candidate / oracle: `mov m4[esp+0x18],eax` / `mov m4[esp+0x8],eax`.
Source anchor: src/game/game.c:773, `unsigned char strget(short x, short y, unsigned char input_shade, unsigned char max_length)`.
[cg95](refresh/r10game/traces/strget/explain.json) attributes stack home ordering (x, other holder y) at 1:02a46 in the first instruction.
Blockdiff pairs 56/65 oracle blocks, with 9 extra candidate blocks. [Reverse](refresh/r10game/baseline/game_game/reverse.json) finds 1 unmatched twins; earliest 1:02dab matches cameratext at 1:0a662. 4 differing inline windows include minitext at 1:02bab.
Prior work: Reinstating a small-font shade word helped strget and initially worsened main. [r10-game, line 1](../../../build/workers/r10-game/NOTES.md).

### game_game / displayrooms

**Switch-table target** at 1:0400c (+0x0 from 1:0400c, public 1:04050). Oracle extent 3,268 B, verifier differences 2,301 B, 155 verifier problems, 1 unverified obligations. The table points to another offset. Its body reserves 18h versus 14h and contains differing animatesprites windows with multiple exact STRICT twins.

First obligation: `+0: off32 -> seg:_TEXT+421e = 1:422e, oracle 1:422b`.
The first body operand, order or length change after discounting equal-form branch destinations is at +0x4a: `sub esp,0x18` / `sub esp,0x14` near src/game/game.c:1328.
[cg95](refresh/r10game/traces/displayrooms/explain.json) attributes register interference (t91) at 1:041c1; this is a separate machine anchor.
Blockdiff pairs 120/154 oracle blocks, with 33 extra candidate blocks. [Reverse](refresh/r10game/baseline/game_game/reverse.json) finds 22 unmatched twins; earliest 1:04103 matches playback at 1:0841b. 3 differing inline windows include animatesprites at 1:04166.
Prior work: A per-iteration animatesprites index reduced its caller extent by 16 bytes. [r10-game, line 1](../../../build/workers/r10-game/NOTES.md).

### game_game / nonsharedkeys

**Stack-slot displacement** at 1:051a0 (+0x4d0 from 1:04cd0, public 1:04cd0). Oracle extent 2,080 B, verifier differences 14 B, 0 verifier problems, 0 unverified obligations. All 111 normalized blocks pair and all fixup obligations pass. The first saved dword in the expanded font path uses +14h versus +1ch.

First instruction, candidate / oracle: `mov m4[esp+0x14],edi` / `mov m4[esp+0x1c],edi`.
Source anchor: src/game/game.c:1453, `ps[0].level &= 15;`.
[cg95](refresh/r10game/traces/nonsharedkeys/explain.json) found no unique decision anchor.
Blockdiff pairs 111/111 oracle blocks, with 0 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin. 2 differing inline windows include gametext at 1:05267.
Prior work: Separating ordinary and extended arrow scancode reads recovered 111/111 blocks and exposed the remaining 14 bytes. [r10-game, line 1](../../../build/workers/r10-game/NOTES.md).

### game_game / main

**Instruction or access order** at 1:05bd3 (+0x73 from 1:05b60, public 1:05b60). Oracle extent 7,472 B, verifier differences 1,338 B, 97 verifier problems, 0 unverified obligations. At the first mismatch the oracle loads the timer mode 36h into EDX ahead of puts, initmenus and initengine. The candidate delays that constant until the inittimer expansion.

First instruction, candidate / oracle: `call C:puts` / `mov edx,0x36`.
Source anchor: src/game/game.c:1607, `puts(main_init_engine);`.
[cg95](refresh/r10game/traces/main/explain.json) attributes register interference (t154) at 1:05ef9; this is a separate machine anchor.
Blockdiff pairs 291/308 oracle blocks, with 17 extra candidate blocks. [Reverse](refresh/r10game/baseline/game_game/reverse.json) finds 3 unmatched twins; earliest 1:067f0 matches playback at 1:085cd. 5 differing inline windows include inittimer at 1:05be7.
Prior work: Restoring separate screenpeek reads for the two HUD calls improved the inlined playback path. [r10-game, line 1](../../../build/workers/r10-game/NOTES.md).

### game_game / dobonus

**Stack-slot displacement** at 1:09667 (+0x17 from 1:09650, public 1:09650). Oracle extent 3,536 B, verifier differences 958 B, 37 verifier problems, 0 unverified obligations. The first store has a different dword stack displacement. All normalized blocks can match while the glyph and timer homes differ.

First instruction, candidate / oracle: `mov m4[esp+0x30],eax` / `mov m4[esp+0x54],eax`.
Source anchor: src/game/game.c:1939, `menu_drag = 0;`.
[cg95](refresh/r10game/traces/dobonus/explain.json) attributes stack home ordering (start, other holder t707) at 1:09664 in the first instruction.
Blockdiff pairs 157/159 oracle blocks, with 2 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin. 11 differing inline windows include gametext at 1:096a9.
Prior work: Changed inline font allocation reduced 1,483 differences to 958 while losing two normalized block pairs. [r10-game, line 1](../../../build/workers/r10-game/NOTES.md).

### game_game / update_menu_cursor

**Branch displacement** at 1:0a74b (+0x5b from 1:0a6f0, public 1:0a6f0). Oracle extent 448 B, verifier differences 227 B, 23 verifier problems, 0 unverified obligations. The menu_drag branch skips a different distance. A later guard reload distinguishes the short-circuit paths.

First instruction, candidate / oracle: `jne -> extent +0x71` / `jne -> extent +0x6f`.
The first body operand, order or length change after discounting equal-form branch destinations is at +0x6f: `jmp` / `mov ax,m2[D:menu_drag]` near src/game/game.c:2025.
[cg95](refresh/r10game/traces/update_menu_cursor/explain.json) attributes register interference (t29) at 1:0a7c8; this is a separate machine anchor.
Blockdiff pairs 16/21 oracle blocks, with 5 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin. 1 differing inline windows include myos at 1:0a847.
Prior work: The GAME search tested cursor shade spellings and EGS/glass source variants without a complete match. Declaration permutations alone also failed. [docs, line 580](../../../docs/FINDINGS.md).

### game_game / displayhighscores

**Stack-slot displacement** at 1:0a8e9 (+0x39 from 1:0a8b0, public 1:0a8b0). Oracle extent 1,024 B, verifier differences 153 B, 8 verifier problems, 0 unverified obligations. The timer/shade path’s first dword store uses +4 versus +8. Inline expansions include independent y homes with the same spelling.

First instruction, candidate / oracle: `mov m4[esp+0x4],edx` / `mov m4[esp+0x8],edx`.
Source anchor: src/game/game.c:2049, `short pulse = next_text_shade(); /* COMPILER_STEERED: save the word before selecting the shade byte. */`.
[cg95](refresh/r10game/traces/displayhighscores/explain.json) attributes stack home ordering (y, other holder y) at 1:0a8e6 in the first instruction.
Blockdiff pairs 49/50 oracle blocks, with 1 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin. 5 differing inline windows include next_text_shade at 1:0a8b9.
Prior work: Naming the shade improved a raw score to 123 but substantially reduced normalized agreement. This variant was discarded. [r10-game, line 1](../../../build/workers/r10-game/NOTES.md).

### game_game / lotsofglass

**Stack-slot displacement** at 1:0acbf (+0xf from 1:0acb0, public 1:0acb0). Oracle extent 480 B, verifier differences 250 B, 31 verifier problems, 0 unverified obligations. The incoming count n occupies +8 versus +10h, in competition with t41. EGS is expanded later.

First instruction, candidate / oracle: `mov m4[esp+0x8],edx` / `mov m4[esp+0x10],edx`.
Source anchor: src/game/game.c:2067, `void lotsofglass(short i, short n)`.
[cg95](refresh/r10game/traces/lotsofglass/explain.json) attributes stack home ordering (n, other holder t41) at 1:0acbc in the first instruction.
Blockdiff pairs 7/9 oracle blocks, with 2 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin. 1 differing inline windows include EGS at 1:0ad04.
Prior work: The GAME search tested cursor shade spellings and EGS/glass source variants without a complete match. Declaration permutations alone also failed. [docs, line 580](../../../docs/FINDINGS.md).

### game_game / moveeffectors

**Frame reservation** at 1:0ba38 (+0x8 from 1:0ba30, public 1:0ba30). Oracle extent 14,016 B, verifier differences 9,625 B, 757 verifier problems, 1 unverified obligations. The local reserve is 58h versus 60h. The frame also serves wall bounds and promoted sprite indices across the effector switch.

First instruction, candidate / oracle: `sub esp,0x58` / `sub esp,0x60`.
Source anchor: src/game/game.c:2340, `void moveeffectors(void)`.
[cg95](refresh/r10game/traces/moveeffectors/explain.json) attributes register interference (t1128) at 1:0bbfe; this is a separate machine anchor.
Blockdiff pairs 434/492 oracle blocks, with 58 extra candidate blocks. [Reverse](refresh/r10game/baseline/game_game/reverse.json) finds 3 unmatched twins; earliest 1:0db5a matches ifhitbyweapon at 1:25c7e. 1 differing inline windows include FTA at 1:0c5ea.
Prior work: Scoped case-8/9 wall bounds improved normalized block count but grew the body. Widening the case-5 saved angle improved positional differences at a different extent. [followup, line 41](../../../build/workers/r9astra-game/followup/NOTES.md).

### game_gamedef / parse

**Instruction or access order** at 1:309a0 (+0x1f8 from 1:307a8, public 1:308b0). Oracle extent 6,580 B, verifier differences 141 B, 11 verifier problems, 0 unverified obligations. In IFCANSEE, two signed short stack inputs to getincangle are loaded in a different order. All 52 measured switch destinations in the selected source agree.

First instruction, candidate / oracle: `movsx eax,m2[esp+0xc]` / `movsx edx,m2[esp+0x8]`.
Source anchor: src/game/gamedef.c:767, `j = getincangle(sprite[i].ang,`.
[cg95](traces/parse/explain.json) found no unique decision anchor.
Blockdiff pairs 196/202 oracle blocks, with 6 extra candidate blocks. [Reverse](baseline/game_gamedef/reverse.json) finds 1 unmatched twins; earliest 1:31c83 matches parsecommand at 1:2f583.
Prior work: IFCANSEE staging matched 1,085 bytes from the public in an alternative but displaced later cases. Rearranging the selector, angle helper and false-branch snapshots missed the combined target. [round4, line 40](../../../build/workers/r8astra-gamedef/round4/NOTES.md).

### game_gamedef / execute

**Switch-table target** at 1:3215c (+0x0 from 1:3215c, public 1:32260). Oracle extent 7,424 B, verifier differences 2,909 B, 338 verifier problems, 0 unverified obligations. A displaced leading switch entry precedes the public 30h versus 2ch reservation and the i/p argument stores. The standalone parser has a separate extent.

First obligation: `+0: off32 -> seg:_TEXT+4451 = 1:32b51, oracle 1:32b4c`.
The first body operand, order or length change after discounting equal-form branch destinations is at +0x108: `sub esp,0x30` / `sub esp,0x2c` near src/game/gamedef.c:1146.
[cg95](traces/execute/explain.json) found no unique decision anchor.
Blockdiff pairs 213/236 oracle blocks, with 23 extra candidate blocks. [Reverse](baseline/game_gamedef/reverse.json) finds 2 unmatched twins; earliest 1:3369a matches parsecommand at 1:2f583. 1 differing inline windows include move_actor at 1:33b61.
Prior work: Separate controls reproduce a 44-byte frame and both incoming homes. Matching frame and extent together still shifted inline cases. Interpreter-loop rewrites could suppress expansion entirely. [round4, line 13](../../../build/workers/r8astra-gamedef/round4/NOTES.md).

### game_menues / draw_and_probe

**Instruction or access order** at 1:2c8fa (+0x7a from 1:2c880, public 1:2c880). Oracle extent 896 B, verifier differences 395 B, 16 verifier problems, 0 unverified obligations. The menu record load comes before the menu_y load, where the oracle does the reverse. A later register-interference anchor is distinct.

First instruction, candidate / oracle: `mov eax,m4[esp+0x5a]` / `mov dx,m2[D:menu_y]`.
Source anchor: src/game/menues.c:269, `menutext(m.x, m.y, m.text, shade, menu_y);`.
[cg95](traces/draw_and_probe/explain.json) attributes register interference (t25) at 1:2c8fa in the first instruction.
Blockdiff pairs 47/50 oracle blocks, with 3 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin. 2 differing inline windows include menutext at 1:2c8f0.
Prior work: Per-page menus and an unsigned-byte shade could make draw_and_probe STRICT while losing menutext’s six stack-home bytes. That trade violated the whole-unit criterion. [r4units, line 38](../../../build/workers/r4units/NOTES.md).

### game_menues / menus

**Switch-table target** at 1:2cc00 (+0x0 from 1:2cc00, public 1:2ccf0). Oracle extent 5,216 B, verifier differences 3,469 B, 306 verifier problems, 1 unverified obligations. A table target moves in a function with repeated by-value menu records. The first post-entry address formation for draw_and_probe uses another local offset.

First obligation: `+0: off32 -> seg:_TEXT+d34 = 1:2cef4, oracle 1:2cef6`.
The first body operand, order or length change after discounting equal-form branch destinations is at +0x210: `lea edi,m4[esp+0xa0]` / `lea edi,m4[esp+0x90]` near src/game/menues.c:300.
[cg95](traces/menus/explain.json) attributes register interference (t243) at 1:2d5a1; this is a separate machine anchor.
Blockdiff pairs 195/198 oracle blocks, with 3 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin. 3 differing inline windows include draw_menu_slider at 1:2d58a.
Prior work: Per-page menus and an unsigned-byte shade could make draw_and_probe STRICT while losing menutext’s six stack-home bytes. That trade violated the whole-unit criterion. [r4units, line 38](../../../build/workers/r4units/NOTES.md).

### game_player / spawn_pistol_tracer

**Register choice** at 1:1f419 (+0x9 from 1:1f410, public 1:1f410). Oracle extent 592 B, verifier differences 335 B, 23 verifier problems, 0 unverified obligations. Incoming snum is sign-extended into EDX where the oracle uses EAX via CWDE. The X/Y ray origins matter again in the caller.

First instruction, candidate / oracle: `movsx edx,ax` / `cwde `.
Source anchor: src/game/player.c:63, `sx = ps[snum].posx;`.
[cg95](refresh/r10final/traces/spawn_pistol_tracer/explain.json) attributes register interference (t14 (snum)) at 1:1f419 in the first instruction.
Blockdiff pairs 4/5 oracle blocks, with 1 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: The earlier overlapping weapon_pos byte read occupied the register wanted by the opening sign extension. Combined scope changes retained the standalone extent. [r10-player, line 1](../../../build/workers/r10-player/NOTES.md).

### game_player / displayweapon

**Stack-slot displacement** at 1:1fe4c (+0x40 from 1:1fe0c, public 1:1fe40). Oracle extent 4,180 B, verifier differences 2,478 B, 115 verifier problems, 0 unverified obligations. The low-level drawing paths now retain the backward frame-27 pistol branch. The first failure is saving incoming snum to [esp+54h] versus [esp+50h], with the same 60h frame.

First instruction, candidate / oracle: `mov m4[esp+0x54],eax` / `mov m4[esp+0x50],eax`.
Source anchor: src/game/player.c:237, `void displayweapon(short snum)`.
[cg95](refresh/r10final/traces/displayweapon/explain.json) attributes stack home ordering (snum, other holder pistol_x) at 1:1fe49 in the first instruction.
Blockdiff pairs 78/101 oracle blocks, with 23 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Pistol-local scope and the frame-27 backward branch restored additional blocks. An alternative 79-block form had more positional errors. [r10-player, line 1](../../../build/workers/r10-player/NOTES.md).

### game_player / processinput

**Stack-slot displacement** at 1:215f9 (+0x1e9 from 1:21410, public 1:21410). Oracle extent 15,692 B, verifier differences 10,512 B, 621 verifier problems, 0 unverified obligations. The reused divisor of look_ang / 3 is in a different dword home. Keep the four getzrange outputs and both inline findplayer distances distinct in the comparison.

First instruction, candidate / oracle: `mov m4[esp+0x90],edx` / `mov m4[esp+0x88],edx`.
Source anchor: src/game/player.c:592, `ps[snum].look_ang -= ps[snum].look_ang / 3;`.
[cg95](refresh/r10final/traces/processinput/explain.json) attributes register interference (t604) at 1:221a9; this is a separate machine anchor.
Blockdiff pairs 380/422 oracle blocks, with 41 extra candidate blocks. [Reverse](refresh/r10final/baseline/game_player/reverse.json) finds 2 unmatched twins; earliest 1:23e58 matches shoot at 1:1f9a1. 5 differing inline windows include shoot at 1:22d24.
Prior work: The 9,843-difference alternative moved the fifth findplayer home. The selected 10,512-difference form retained the caller constraint. [r10-player, line 1](../../../build/workers/r10-player/NOTES.md).

### game_premap / spawn

**Stack-slot displacement** at 1:0f1af (+0x4f from 1:0f160, public 1:0f160). Oracle extent 8,736 B, verifier differences 4,565 B, 366 verifier problems, 0 unverified obligations. EGS returns the new sprite index i in AX. The store is to +4 instead of +14h. A later crane use also has a colour tie.

First instruction, candidate / oracle: `mov m2[esp+0xc],ax` / `mov m2[esp+0x14],ax`.
Source anchor: src/game/premap.c:249, `i = EGS(sprite[j].sectnum, sprite[j].x, sprite[j].y, sprite[j].z,`.
[cg95](refresh/r10/traces/spawn/explain.json) attributes equal-score colour choice (t1563) at 1:0fc7b; this is a separate machine anchor.
Blockdiff pairs 253/276 oracle blocks, with 24 extra candidate blocks. [Reverse](refresh/r10/baseline/game_premap/reverse.json) finds 2 unmatched twins; earliest 1:10a20 matches copytilepiece at 1:3bcfa.
Prior work: Reordering the short locals and spelling out the case-20 loop preserved both extents and restored 13 entries in the expanded caller. [r10-actors, line 1](../../../build/workers/r10-actors/NOTES.md).

### game_premap / render_camera_tile

**Frame reservation** at 1:11385 (+0x5 from 1:11380, public 1:11380). Oracle extent 816 B, verifier differences 367 B, 19 verifier problems, 0 unverified obligations. 14h versus 18h with two pairs of short framebuffer/tile loop counters. The tested source change restores the reserve.

First instruction, candidate / oracle: `sub esp,0x14` / `sub esp,0x18`.
Source anchor: src/game/premap.c:749, `void render_camera_tile(long x, long y, long z, short ang, short sect, short tilenume)`.
[cg95](refresh/r10/traces/render_camera_tile/explain.json) found no unique decision anchor.
Blockdiff pairs 0/5 oracle blocks, with 5 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Early camera work tried row/dest declaration orders, K&R parameters, seven sprite-field orders and lower inline thresholds. The no-modify-clause copybuf pragma restored the tile reloads. [gamefix7, line 87](../../../build/workers/gamefix7/NOTES.md).
The worker-local camera patch resolves this entire extent, preserves each previously STRICT public, and verifies all references in a fresh full-unit DOS build. [Source change](camera.patch) · [Verifier receipt](refresh/r10/real/camera-swapboth/stock.json)

### game_premap / render_sprite_camera

**Frame reservation** at 1:116b8 (+0x8 from 1:116b0, public 1:116b0). Oracle extent 864 B, verifier differences 347 B, 19 verifier problems, 0 unverified obligations. The inlined renderer reserves 20h versus 1ch. The terminal sprite input fields and copy loops share that frame.

First instruction, candidate / oracle: `sub esp,0x20` / `sub esp,0x1c`.
Source anchor: src/game/premap.c:766, `void render_sprite_camera(short i)`.
[cg95](refresh/r10/traces/render_sprite_camera/explain.json) found no unique decision anchor.
Blockdiff pairs 0/5 oracle blocks, with 5 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Early camera work tried row/dest declaration orders, K&R parameters, seven sprite-field orders and lower inline thresholds. The no-modify-clause copybuf pragma restored the tile reloads. [gamefix7, line 87](../../../build/workers/gamefix7/NOTES.md).
The worker-local camera patch resolves this entire extent, preserves each previously STRICT public, and verifies all references in a fresh full-unit DOS build. [Source change](camera.patch) · [Verifier receipt](refresh/r10/real/camera-swapboth/stock.json)

### game_premap / render_mirror

**Frame reservation** at 1:11a18 (+0x8 from 1:11a10, public 1:11a10). Oracle extent 976 B, verifier differences 205 B, 12 verifier problems, 0 unverified obligations. The expanded camera body reserves 28h versus 24h. The caller also computes the wall midpoint and player-facing angle.

First instruction, candidate / oracle: `sub esp,0x28` / `sub esp,0x24`.
Source anchor: src/game/premap.c:773, `void render_mirror(short w)`.
[cg95](refresh/r10/traces/render_mirror/explain.json) attributes colour score (t304) at 1:11aa9; this is a separate machine anchor.
Blockdiff pairs 0/5 oracle blocks, with 5 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Early camera work tried row/dest declaration orders, K&R parameters, seven sprite-field orders and lower inline thresholds. The no-modify-clause copybuf pragma restored the tile reloads. [gamefix7, line 87](../../../build/workers/gamefix7/NOTES.md).

### game_premap / prelevel

**Switch-table target** at 1:1219c (+0x4 from 1:12198, public 1:12210). Oracle extent 9,992 B, verifier differences 6,137 B, 474 verifier problems, 0 unverified obligations. The prefix points into the expanded body. resetprestat’s first differing signed conversion also precedes much of the inlined spawn.

First obligation: `+4: off32 -> seg:_TEXT+5355 = 1:14445, oracle 1:1444c`.
The first body operand, order or length change after discounting equal-form branch destinations is at +0xce: `movsx edx,dx` / `movsx ebx,dx` near src/game/premap.c:871.
[cg95](refresh/r10/traces/prelevel/explain.json) attributes register interference (t1950) at 1:12288; this is a separate machine anchor.
Blockdiff pairs 284/320 oracle blocks, with 37 extra candidate blocks. [Reverse](refresh/r10/baseline/game_premap/reverse.json) finds 1 unmatched twins; earliest 1:13cbd matches copytilepiece at 1:3bcfa. 1 differing inline windows include resetprestat at 1:12219.
Prior work: A previously accepted permutation had cost its embedded spawn all 13 matching table entries. The new local order restores them. [r10-actors, line 1](../../../build/workers/r10-actors/NOTES.md).

### game_premap / newgame

**Instruction or access order** at 1:148bb (+0x1b from 1:148a0, public 1:148a0). Oracle extent 272 B, verifier differences 38 B, 17 verifier problems, 0 unverified obligations. The incoming byte stores now agree. The candidate reads ud.debug at +1bh where the oracle first forms the value 1 in BL.

First instruction, candidate / oracle: `mov bx,m2[D@2:a57ad]` / `mov bl,0x1`.
Source anchor: src/game/premap.c:987, `if (ud.debug) {`.
[cg95](refresh/r10/traces/newgame/explain.json) found no unique decision anchor.
Blockdiff pairs 3/4 oracle blocks, with 1 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: A shared short zero for weapon ownership reduced 49 differences to 38. [r10-actors, line 1](../../../build/workers/r10-actors/NOTES.md).

### game_sector / operatesectors

**Switch-table target** at 1:1598c (+0x0 from 1:1598c, public 1:159e0). Oracle extent 10,288 B, verifier differences 7,404 B, 564 verifier problems, 0 unverified obligations. The selector scales the sector index into a different register. The prefix first reports the resulting case layout. Cases 19–22 have separate animation reference evidence.

First obligation: `+0: off32 -> seg:_TEXT+b39 = 1:15ae9, oracle 1:15aea`.
The first body operand, order or length change after discounting equal-form branch destinations is at +0xac: `lea ecx,m4[+ebx*2+0x0]` / `lea edi,m4[+ebx*2+0x0]` near src/game/sector.c:278.
[cg95](traces/operatesectors/explain.json) attributes colour score (t1683) at 1:16c68; this is a separate machine anchor.
Blockdiff pairs 394/447 oracle blocks, with 52 extra candidate blocks. [Reverse](baseline/game_sector/reverse.json) finds 11 unmatched twins; earliest 1:16f98 matches activatewarpelevators at 1:158fb. 18 differing inline windows include setanimation at 1:15dd2.
Prior work: Reusing daz for the case-21 ceiling improved positional bytes but grew the extent to 10,308 and lost two lift/sound calls in checksectors. [followup2, line 32](../../../build/workers/r8astra-sector/followup2/NOTES.md).

### game_sector / operateactivators

**Switch-table target** at 1:181c0 (+0x4 from 1:181bc, public 1:18210). Oracle extent 4,996 B, verifier differences 3,157 B, 281 verifier problems, 0 unverified obligations. An early table entry precedes the 64h versus 5ch reserve for the expanded sector handlers.

First obligation: `+4: off32 -> seg:_TEXT+4574 = 1:19524, oracle 1:19522`.
The first body operand, order or length change after discounting equal-form branch destinations is at +0x5a: `sub esp,0x64` / `sub esp,0x5c` near src/game/sector.c:598.
[cg95](traces/operateactivators/explain.json) found no unique decision anchor.
Blockdiff pairs 121/169 oracle blocks, with 46 extra candidate blocks. [Reverse](baseline/game_sector/reverse.json) finds 3 unmatched twins; earliest 1:1847a matches keytimerstuff at 1:0023e.
Prior work: Case-21/22 post-write goal reads recover the two missing references. Changing the activator walk or dispatch disturbed the frame, embedded operator, or call counts. [followup2, line 30](../../../build/workers/r8astra-sector/followup2/NOTES.md).

### game_sector / cheatkeys

**Switch-table target** at 1:196b4 (+0x24 from 1:19690, public 1:19720). Oracle extent 7,604 B, verifier differences 3,332 B, 284 verifier problems, 0 unverified obligations. One of the leading entries first exposes displaced body lengths. The public prologue reserves 64h versus 60h.

First obligation: `+24: off32 -> seg:_TEXT+5226 = 1:1a1d6, oracle 1:1a1c7`.
The first body operand, order or length change after discounting equal-form branch destinations is at +0x96: `sub esp,0x64` / `sub esp,0x60` near src/game/sector.c:654.
[cg95](traces/cheatkeys/explain.json) found no unique decision anchor.
Blockdiff pairs 240/290 oracle blocks, with 46 extra candidate blocks. [Reverse](baseline/game_sector/reverse.json) finds 3 unmatched twins; earliest 1:1a314 matches keytimerstuff at 1:0023e. 1 differing inline windows include operateforcefields at 1:1b34b.
Prior work: A shared full-width gm recovered four expanded door blocks. Narrowing dimensionmode improved a diagnostic score but emitted the wrong access width. [followup2, line 24](../../../build/workers/r8astra-sector/followup2/NOTES.md).

### game_sector / checkhitswitch

**Equal-savings conflict** at 1:1b7b8 (+0x374 from 1:1b444, public 1:1b4b0). Oracle extent 1,740 B, verifier differences 2 B, 0 verifier problems, 0 unverified obligations. Case 405 advances nextspritestat through EAX versus ECX. Two instructions and two bytes are affected. Manual conflict identification is stronger here than the automated attribution.

First instruction, candidate / oracle: `movsx eax,cx` / `movsx ecx,cx`.
Source anchor: src/game/sector.c:870, `i = nextspritestat[i];`.
[cg95](traces/checkhitswitch/explain.json) found no unique decision anchor.
Blockdiff pairs 83/83 oracle blocks, with 0 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Factoring the predicate, state write, list walk or next-link helper left both selector bytes. Folding the wall multiply moved the two conflicts together but still visited the conversion first. [followup2, line 36](../../../build/workers/r8astra-sector/followup2/NOTES.md).

### game_sector / checkhitwall

**Branch displacement** at 1:1bd23 (+0x213 from 1:1bb10, public 1:1bb10). Oracle extent 1,648 B, verifier differences 78 B, 12 verifier problems, 0 unverified obligations. An unsigned dispatch-range branch has a changed distance to the glass handlers. The early texture write competes with the adjacent-sector load.

First instruction, candidate / oracle: `jbe -> extent +0x29c` / `jbe -> extent +0x29f`.
The first body operand, order or length change after discounting equal-form branch destinations is at +0x257: `mov m2[D@2:17631a+eax],0x49f` / `movsx ebx,m2[D@2:176325+eax]` near src/game/sector.c:926.
[cg95](traces/checkhitwall/explain.json) found no unique decision anchor.
Blockdiff pairs 60/63 oracle blocks, with 3 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Staged glass arguments, full/partial helper expansion and next-link lifetimes failed to beat 78 differing bytes. [followup2, line 34](../../../build/workers/r8astra-sector/followup2/NOTES.md).

### game_sector / checksectors

**Switch-table target** at 1:1c4d8 (+0x0 from 1:1c4d8, public 1:1c5e0). Oracle extent 11,448 B, verifier differences 7,670 B, 633 verifier problems, 0 unverified obligations. The switch already precedes the public. The first table fixup goes to a displaced case. In the body the incoming player index is spilled where the oracle keeps a register.

First obligation: `+0: off32 -> seg:_TEXT+7a0a = 1:1c9ba, oracle 1:1c98d`.
The first body operand, order or length change after discounting equal-form branch destinations is at +0x114: `mov m4[esp+0x80],eax` / `mov edi,eax` near src/game/sector.c:1042.
[cg95](original-traces/checksectors/explain.json) attributes register interference (t2276) at 1:1d9d8; this is a separate machine anchor.
Blockdiff pairs 302/387 oracle blocks, with 81 extra candidate blocks. [Reverse](baseline/game_sector/reverse.json) finds 6 unmatched twins; earliest 1:1d55d matches bunchfront at 1:3492b.
Prior work: Scoping the sector index and staging the latch restored the local reserve and both lift paths. Splitting camera/return logic and a case-21 ceiling control lost block or call agreement. [followup2, line 28](../../../build/workers/r8astra-sector/followup2/NOTES.md).

### game_sounds / spritesound

**Instruction or access order** at 1:2be7a (+0xa from 1:2be70, public 1:2be70). Oracle extent 464 B, verifier differences 149 B, 14 verifier problems, 0 unverified obligations. The candidate spills a register before sign-extending the other argument. SemIR finds equivalent modeled effects.

First instruction, candidate / oracle: `mov m4[esp+0x10],eax` / `movsx eax,dx`.
Source anchor: src/game/sounds.c:202, `short num = number;`.
[cg95](traces/spritesound/explain.json) found no unique decision anchor.
Blockdiff pairs 16/20 oracle blocks, with 4 extra candidate blocks. [Reverse](baseline/game_sounds/reverse.json) finds 2 unmatched twins; earliest 1:2bf89 matches xyzsound at 1:2bdae.
Prior work: The round-four source/loop and store-order trials left these functions unresolved. The C/IR diagnostics here are from a fresh build. [docs, line 597](../../../docs/FINDINGS.md).

### multi / neton

**Instruction or access order** at 1:47d90 (+0x10b from 1:47c85, public 1:47c85). Oracle extent 1,079 B, verifier differences 298 B, 57 verifier problems, 1 unverified obligations. The first access reads the handler buffer pointer where the oracle reads the real-mode segment byte first.

First instruction, candidate / oracle: `mov eax,m4[D:rmnethandler]` / `mov dl,m1[D:rmsegment16]`.
Source anchor: src/multi/multi.c:195, `rmnethandler[7] = rmsegment16;`.
[cg95](traces/neton/explain.json) attributes equal-score colour choice (t82) at 1:47eb2; this is a separate machine anchor.
Blockdiff pairs 9/13 oracle blocks, with 4 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Connection-number association and byte-load order did not recover neton’s extent. netsend could drop the extra EDI without recovering the body. Pointer/index/increment variants failed the comsend escape arm. [r4multi, line 41](../../../build/workers/r4multi/NOTES.md).

### multi / netsend

**Saved-register set** at 1:483c4 (+0x3 from 1:483c1, public 1:483c1). Oracle extent 593 B, verifier differences 241 B, 40 verifier problems, 1 unverified obligations. The candidate pushes EDI in addition to EBX, ECX and ESI. SemIR finds equivalent local effects.

First instruction, candidate / oracle: `push edi` / `mov esi,eax`.
Source anchor: src/multi/multi.c:333, `void netsend(short otherconnectindex, short messleng)`.
[cg95](traces/netsend/explain.json) found no unique decision anchor.
Blockdiff pairs 13/15 oracle blocks, with 2 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Connection-number association and byte-load order did not recover neton’s extent. netsend could drop the extra EDI without recovering the body. Pointer/index/increment variants failed the comsend escape arm. [r4multi, line 41](../../../build/workers/r4multi/NOTES.md).

### multi / comsend

**Register choice** at 1:4866f (+0x5d from 1:48612, public 1:48612). Oracle extent 141 B, verifier differences 5 B, 2 verifier problems, 0 unverified obligations. The buffer-end word is sign-extended to EAX versus EDX. The extent differs at five bytes.

First instruction, candidate / oracle: `movsx eax,m2[eax+0x0]` / `movsx edx,m2[eax+0x0]`.
Source anchor: src/multi/multi.c:386, `*(volatile unsigned char *)((long)outbuf + (long)*outbufend) = 65;`.
[cg95](traces/comsend/explain.json) found no unique decision anchor.
Blockdiff pairs 5/6 oracle blocks, with 1 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: Connection-number association and byte-load order did not recover neton’s extent. netsend could drop the extra EDI without recovering the body. Pointer/index/increment variants failed the comsend escape arm. [r4multi, line 41](../../../build/workers/r4multi/NOTES.md).

### multi / sendpacket

**Branch displacement** at 1:48c71 (+0x1c from 1:48c55, public 1:48c55). Oracle extent 1,054 B, verifier differences 128 B, 18 verifier problems, 0 unverified obligations. The dispatch JAE has a different distance before packet CRC formation.

First instruction, candidate / oracle: `jae -> extent +0x13c` / `jae -> extent +0x13a`.
The first body operand, order or length change after discounting equal-form branch destinations is at +0x97: `mov ebx,eax` / `xor eax,eax` near src/multi/multi.c:585.
[cg95](traces/sendpacket/explain.json) attributes colour score (t31) at 1:48d09; this is a separate machine anchor.
Blockdiff pairs 48/50 oracle blocks, with 2 extra candidate blocks. Reverse lookup finds no unmatched oracle block with an exact normalized STRICT twin.
Prior work: The -5r profile recovered the signed-short promotion. Rewriting CRC emission and packet/ring indices improved the code but left forward branches and byte extraction displaced. [r4multi, line 27](../../../build/workers/r4multi/NOTES.md).

### multi / getpacket

**Branch displacement** at 1:49092 (+0x1f from 1:49073, public 1:49073). Oracle extent 1,424 B, verifier differences 695 B, 52 verifier problems, 0 unverified obligations. The receive-list guard has a different forward distance. The inbuf byte fetch later uses a scaled address folded differently.

First instruction, candidate / oracle: `jae -> extent +0x2f9` / `jae -> extent +0x2fb`.
The first body operand, order or length change after discounting equal-form branch destinations is at +0x53: `add edx,ebx` / `mov dl,m1[edx+ebx*1+0x0]` near src/multi/multi.c:679.
[cg95](traces/getpacket/explain.json) attributes colour score (t238) at 1:4913c; this is a separate machine anchor.
Blockdiff pairs 72/81 oracle blocks, with 9 extra candidate blocks. [Reverse](baseline/multi/reverse.json) finds 1 unmatched twins; earliest 1:4955d matches loadboard at 1:392c6.
Prior work: The -5r profile recovered the signed-short promotion. Rewriting CRC emission and packet/ring indices improved the code but left forward branches and byte extraction displaced. [r4multi, line 27](../../../build/workers/r4multi/NOTES.md).
