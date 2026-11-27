# Watcom 9.5 GA rules for diagnosing a first difference

## SECTOR jump-table extent and allocator controls

**PROVEN for checkhitceiling and checksectors.** The 264 B immediately preceding `checksectors` at 1:1c5e0 contain 66 jump-table entries. The largest oracle target, at table+4, is PUBLIC+0x2b9c. When the candidate retains this table and the next PUBLIC binding, the verifier requires each oracle target to fall strictly below the end of the candidate public span. The minimum span is 11,165 B. The baseline span of 11,184 B has 19 B of slack. This condition proves the previous function's end at 1:1c4d8. Identity of the individual candidate destinations is a separate test when verifying `checksectors` itself.

| Stock DOS control | Public span B | Leading table B | checkhitceiling | checksectors differing bytes |
| --- | ---: | ---: | --- | ---: |
| Canonical | 11,184 | 264 | STRICT | 7,670 |
| Stage current sector in under_tag and permute locals, csdecl-4cur | 11,168 | 264 | STRICT | 8,137 |
| Same operation, different declaration order, csdecl-10cur | 11,152 | 264 | CODE_BYTES_MATCH | 8,082 |
| Reuse w and i for current-lift arguments | 11,056 | 264 | CODE_BYTES_MATCH | 7,484 |
| Same argument reuse, also cache the initial current sector in w | 11,216 | 264 | STRICT | 8,129 |

The last two controls both emit the oracle's 168-byte frame and incoming MOV EDI,EAX. Caching the initial sector in w recovers the ceiling boundary but moves checksectors' own end 32 bytes beyond its canonical boundary. These tests distinguish the entry register, the preceding extent and the current extent.

**PROVEN for the captured game_oe candidate sorts.** Replaying the optimized IR reads and definitions reproduced all 1,196 measured savings across seven passes. Each qualifying operand and result slot contributes 20 to the power of its loop depth, capped at depth five. The replay follows indexed operands and alias bases to their conflict and sums modulo 2^32. The measured weights are 1, 20, 400, 8,000, 160,000 and 3,200,000. Counts per pass were 204/242/242 for `checksectors`, 147/154 for `cheatkeys`, and 100/107 for `operateactivators`. The flags 0x100 class sorts first when the bit is clear. Within each class conflicts come first and sort by decreasing unsigned savings. Entries without a conflict sort by decreasing signed temporary ID. Equal keys can change relative order in the unstable sort. The later conflict-priority list and the register choice have their own scores.

| Canonical trace and replay decision | Isolated result |
| --- | --- |
| checksectors, candidate t115/t1_snum, pass 3, savings 260 each | 7,670 → 7,669, same extent and first byte. EDI still requires a change in the player number's interference. |
| cheatkeys, candidate t364_dy/t365_dx, pass 1, savings 220 each | 3,332 → 3,331, same extent and frame. These are the inlined door deltas. On pass 2 each has savings 60. |
| operateactivators, home order t1_low/t2_i alone | 3,157 → 3,149. The two homes exchange and the frame stays 100 B. |
| operateactivators, candidate t100/t98, pass 1, savings 140 each, plus the home exchange | 3,157 → 2,760 and 281 → 237 fixup diagnostics. The extent grows 16 B to 5,012 and lacks a boundary proof. |
| checkhitwall, register choice t179 → EAX | 78 → 76. The two changed bytes are in the signed list index conversion and following nextspritestat load at PUBLIC+0x512 and +0x515. EDI scored 2 through copy preference and eligible EAX scored 0. |

In `operateactivators` block 155, t98 receives the first wall index scaled by the 35-byte record size. Its point2 field supplies the index for the same calculation into t100. The optimized IR places their final copies at instructions 7 and 15. Changing declaration order in the shared callee can improve this inline copy while changing its callers' extents. All debugger-forced results above are diagnostic. The source result and stock controls have separate DOS receipts. [SECTOR census](BLOCKER_CENSUS.md#sector-dos-verification-at-12e1107)

**PROVEN for the stock list advance.** With a block-local long byte offset, block 40 splits the signed conversion (t180) from its use in forming link_offset (t167). For the conversion, EAX and EDI both score zero. For link_offset both score two. EAX wins both decisions. In the baseline the conversion t179 prefers EDI two to zero. The DOS source control at canonical case order reproduces the isolated forced-EAX function bytes and verifier references exactly. In the proposed checksectors trace, t1_snum still has no eligible register and spans blocks 0 through 395. The improved table targets arose with the player number still on the stack.

## Natural C controls for conflict order, register choice and stack homes

**PROVEN for scansector on the engine profile.** In the March 1995 spelling, the short-circuit condition reads the wall's point2 and both screen bounds directly. In backend block 26 the stock compiler assigns t92 (scale) to EDX and t91 (signed p2 link) to EDI. AX holds point2. The following load of xb2 at +0x141 and its comparison at +0x148 use EAX where the oracle uses EDX. Farther on, loading spritesortcnt into EDX costs six bytes, while the oracle's EAX load costs five. The diagnostic choices were sufficient for the canonical instruction stream. Restoring the donor expression also changes other allocations and store scheduling.

**PROVEN for comsend on the multi profile.** Retaining the pointer to the volatile producer index from the first ESC2 advance through the common final advance gives the signed index conversion EDX, as predicted. Its result feeds the address addition. In the proposed stock trace those two operations are t25 and t26, and both choose EDX. In the canonical trace the corresponding conversion is t27. At +0x5e the stock candidate adds the ring base into EDX and stores through EDX. The oracle loads the base into EAX and stores through EDX+EAX. This changes the off32 site from +0x5f to +0x60. An independent source control using a widened index for the ESC1 advance and ordinary ESC2 indexing reproduces the complete 12-byte ESC2 sequence including the fixup. It introduces 21 whole-function differences.

**NEGATIVE for checkhitswitch on game_oe.** In 33 stock traces of source forms with two remaining differences, the case-405 conversion and scale both retained EAX. Thirty-two retained conflict ranks 97 and 110 among 215 conflicts. Reusing the wall displacement for the scaled link in case 406 moved the ranks to 81 and 97, still in the same relative order. The replay that exchanges the canonical equal-savings pair (564 each) gives both ECX. Prefetching the link in case 406 provides a positive stock control with the same 215 conflicts. Ranks 83 and 82 put the scale before the conversion. Both choose ECX and the case-405 advance matches, with regressions elsewhere. Absolute rank movement alone did not predict an exchange.

**PROVEN for the diagnostic stack homes.** In canonical getzrange, exchanging ymin with floor_x2, xmax with t185, and ymax with day advances the first difference from +0x26 to +0x85 at the same 120-byte frame and 2,938-byte extent. Positional differences fall from 2,288 to 2,284 and all 70 fixup diagnostics remain. In canonical spritesound a single exchange of i and t47 advances the first difference from +0x32 to +0x86 and reduces 56 differences to 51. These are controlled debugger experiments. The signed-index source change in SOUNDS has a different register assignment at entry and an earlier first difference.

The worker records each stock control, its whole-unit regressions, the replay selectors and the accepted source forms. [Prediction receipts](../build/workers/blockers/natural/predictions.md)

**NEGATIVE for getzrange declaration order.** Moving floor_x2 alone from the floor-sprite block to the function declaration beside xmin leaves xmin at ESP+0x54 and ymin at ESP+0x48. In the stock trace the anonymous t186 now occupies the desired home at ESP+0x5c. The original occupant was floor_x2. In the accepted whole DOS object this change restores the two fixup sites for scaredfallz and the next sector floor at +0x329 and +0x32f. The early ymin home remains open. [Stock home trace](../build/workers/blockers/natural/traces/floor-endpoint.json)

**PROVEN for SOUNDS on game_oe.** The signed short owner widened to a long sprite index gives 47 differing bytes on its own. Retaining cy - y in a block-local long for both getangle and the Manhattan distance gives 43. The combined source retains the independent xyzsound STRICT proof, all 25 wrapper references, and both original extents. Staging the angle passed to FX_PlayVOC3D instead yields a lower wrapper count of 30 but costs 140 differences in standalone xyzsound.

## Whole-unit controls for stack homes, inlining and branch destinations

**PROVEN for ENGINE setview.** With the engine profile, naming the row span before the option test and assigning viewoffset in each arm yields the oracle’s EDX span, AH option byte and viewoffset store order. Keeping the same arms but using the pre-existing loop variable i for the span leaves six positional differences. Keeping j leaves nine. The independent block-local long produces a 334-byte STRICT result with all 26 relocations verified. The evidence establishes these source forms for this function.

**PROVEN for game_oe MENUES.** An unsigned-byte formal can remove the caller’s early widening and simultaneously change stack placement in the standalone callee. Changing only menutext’s shade formal from long to unsigned char brings draw_and_probe from 395 positional differences to STRICT, leaves menus STRICT and changes six standalone menutext displacement bytes. Its 16-byte reservation stays fixed. CG95 identifies start_x at ESP+4 rather than +12, trading homes with ac. Both have equal home priority in the candidate. The allocator trace does not determine an original source declaration order.

**PROVEN for SOUNDS.** With a short sound-number formal, an int source formal narrowed to short i and the five-argument xyzsound call, Watcom emits a 464-byte inlined spritesound with 25 verified references. The standalone xyzsound stays STRICT. Changing only the wrapper’s source formal to short, retaining the same local i and call, changes the emitted code again. At +0x2f in the accepted wrapper CG95 attributes the first unequal instruction to i’s home. ESP+4 and ESP+8 exchange i and another equal-priority temporary. The first differing encoded byte is +0x32.

**NEGATIVE for cascade diagnosis.** Correcting a table destination can conceal a different instruction sequence. The two SECTOR controls restore the case-26 return and following case start while retaining the wrong index calculation at +0x97. Separating case 9’s sp also recovers a neighbouring extent proof without recovering all the inlined copies. In grouvline the page-scratch control reduces the overall differences from 1,478 to 1,441 while moving the JNE destination from +0x11c to +0x11b. The oracle destination is +0x11d. In the accepted expression the row term precedes the column term, and the page offset is added after the VGA sequencer port write. The branch still reaches +0x11c. The first unequal symbolic instruction inside the arm advances from +0xcc to +0xee, and 12 of 14 references verify versus 9 of 13 on the control. Check the actual target, extent and callers for each claimed cascade repair.

## Full-unit evidence from MENUES and ENGINE

**PROVEN on game_oe.** The shape of a later switch can change the stack homes and instruction lengths in an earlier page whose C text is identical. With the unsigned high-scores range test, the first main-page descriptor copy lands at ESP+0xa0 and the second at ESP. With a two-label switch on the same choice, they land at ESP+0x90 and ESP+0x30. The second destination setup grows from 2 B to 4 B. Both builds reserve 252 B. The switch restores every compared code byte and 57 of 60 leading table targets.

| High-scores choice | Pause labels | Whole-function differing bytes | Correct table targets | Formal DOS status |
|---|---|---:|---:|---|
| Unsigned range test | Original reconstruction | 3,469 | 0/60 | NEAR_MATCH |
| Unsigned range test | Corrected | 3,469 | 0/60 | NEAR_MATCH |
| Two-label switch | Original reconstruction | 0 | 57/60 | NEAR_MATCH |
| Two-label switch | Corrected | 0 | 60/60 | STRICT |

The pause dispatcher at 1:2df90 adds two to the choice. The oracle entries at extent +0xcc and +0xd8 lead to the MODE_GAME store at 1:2dfcc for choices −2 and 1. Entry +0xd0 leads to the MODE_END store at 1:2dfa4 for choice −1. The three residual relocations exposed a gameplay-relevant label error after the layout matched. This experiment establishes the generated code and the cross-page effect. It does not identify a particular allocator tie.

**PROVEN on engine, grouvline.** Both controls start from the same translation unit. Reusing the spent endpoint y1 for the increment swap restores the 112 B frame. Reusing the spent projection temporary tempy for screen y in both traversal loops then restores the exact extent. The first differing byte moves from +5 to +0xc3.

| Swap uses y1 | Raster ordinate uses tempy | Reserved frame | Extent | Differing bytes |
|---|---|---:|---:|---:|
| No | No | 116 | 2,169 | 1,626 |
| No | Yes | 116 | 2,163 | 1,656 |
| Yes | No | 112 | 2,170 | 1,569 |
| Yes | Yes | 112 | 2,169 | 1,478 |

**PROVEN on engine, getzrange.** With the accepted bounds order and queue code fixed, splitting the first four floor-sprite corner values from wall-loop values and splitting the floor y offset have interacting frame costs. The four corners add 12 B on their own. The separate offset adds 4 B on its own. Together they add 20 B. Counting declared longs alone would mispredict the combined reservation.

| Private floor corners | Private floor y offset | Reserved frame | Extent | Differing bytes |
|---|---|---:|---:|---:|
| No | No | 100 | 2,954 | 2,350 |
| Yes | No | 112 | 2,925 | 2,354 |
| No | Yes | 104 | 2,966 | 2,405 |
| Yes | Yes | 120 | 2,938 | 2,288 |

The combined ENGINE build retains every previous STRICT public, both VERIFIED data segments and the BSS report. Its getzrange code first differs at +0x26. The oracle stores ymin at ESP+0x5c and the candidate stores it at ESP+0x48. In SECTOR, sharing the initiator and current-sector locals restored checksectors’ entry MOV EDI,EAX while shortening the function by 128 B. That change lost checkhitceiling’s boundary proof. The controls and stock verifier records are under `build/workers/blockers/cascade/`.

The compiler in the installation named `wc95b` is the unpatched 9.5 GA compiler. The engine controls used `-4r -s -or`. The other controls used `game_oe`, with `-4r -s -otexan -oe=3000`. The seven synthetic fixture families produced sixteen source configurations. Every trace passed the stock observer’s ordinary NT and DOS code/public/fixup checks. Each captured sort was replayed against its recorded input. The assembly observations below use function-relative addresses unless a segment offset is explicitly identified. [Measurements and C fixtures](exp/summary.json) · [Control checks and case-405 conflict ranks](exp/controls.json)

## A case address can fail before the first changed instruction

**PROVEN for the six-arm game_oe switch.** With the same case order, changing one immediate from 3 to 257 widened one memory ADD by three bytes. Both builds put six relocated dwords at `_TEXT+00h…17h` and the public at `_TEXT+20h`. The case destinations in segment coordinates were `2d,36,3f,48,51,5a` and `2d,39,42,4b,54,5d`. The changed arm starts at exactly the same address in both builds. Each subsequent case shifts by three bytes. The first changed table field is at `_TEXT+4`, well before the changed ADD at `_TEXT+2dh`. The named selector remains EAX, savings 16. [Short immediate](exp/table/a/probe.c) · [Wide immediate](exp/table/b/probe.c)

For a fixed sequence of blocks, the destination delta is the cumulative change in intervening encoded lengths, including alignment. Consecutive destination deltas can isolate a changed interval when cases share the same physical order. Compare unique destinations in physical address order first. Duplicated targets, distinct table owners, extra blocks and jump relaxation make naive case-number subtraction ambiguous. The control changes a constant specifically to generate a measured length difference. A table mismatch in a real function supplies no evidence for changing a gameplay constant.

**PROVEN for the single forward branch.** The same immediate change in an ordinary conditional shifts the destination of JE from `+10h` to `+13h`. The JE instruction stays at `+2`, its distance byte changes first, and its condition is identical. The only changed-length instruction starts at `+4`. The effective distance equals destination minus the end of the encoded branch. Preserve a matching predicate while testing the code inside that interval. This rule fits the eight branch-first rows and the eleven table-first rows in the supplied census. [Control](exp/branch/a/probe.c) · [Changed span](exp/branch/b/probe.c)

## Two equal-priority homes follow their actual sorter order

**PROVEN for two dword outputs passed by address in game_oe.** With `long x, y;`, the home input contains x at zero-based rank 3 and y at rank 4. Both survive in that order. The finalized homes are x at frame-relative −4 and y at −8. After `sub esp,8` these are `[esp+4]` and `[esp]`. With only the declaration order reversed, y takes rank 3 and x takes rank 4. The pointer registers at the external call are still EAX for x and EDX for y. Both displacements exchange, the eight-byte reserve and 27-byte extent remain fixed, and colour choices are unchanged. [x then y](exp/home/a/probe.c) · [y then x](exp/home/b/probe.c)

The observed comparator first tests the home-priority flag and then compares sizes. It orders alias offsets within a single backend temp ID. Equal priority between different IDs supplies no name- or declaration-based tie break. The gapped sort can displace equal records when other records cross them. A declaration swap is predictive only if it exchanges the relevant surviving records or changes their relative placement through that sort. Inspect both original input ranks and final ranks. [Sorter replay](../../../tools/cg95/cgtrace95.py) · [Two real equal-priority i/hp homes](traces/hitradius/explain.json) · [Two minitext origins](traces/operatefta/explain.json)

## Object identity, lexical scope and register pressure have distinct frame effects

**PROVEN for two address-taken scalar outputs.** The fixture passes its first long to an external fill operation, saves its value, then uses a second long for a second fill. With sibling scopes the frame is eight bytes, y at −4 and x at −8. Extending x to the outer block changes its backend ID from 3 to 2. The home displacements, eight-byte frame and 38-byte body stay identical. Reusing that same x object for the second output leaves one home at −4, a four-byte frame and a 35-byte body. Reuse is a valid source hypothesis when the values do not overlap and the callees’ address semantics permit it. The C backend’s treatment of these traces does not establish a general lifetime-sharing policy for arbitrary escaped addresses. [Sibling scopes](exp/frame/a/probe.c) · [Extended scope](exp/frame/b/probe.c) · [Same object twice](exp/frame/reuse/probe.c)

**PROVEN counterexample to predicting frame size from the number of declared locals.** In a two-pass row-copy loop with ordinary external calls, four separate named short counters map to BX/CX in both phases. Reusing a single pair leaves two named conflicts and the same two colours, zero local bytes and the same 105-byte function extent. In the full PREMAP camera renderer, reusing the pair *increases* the standalone reserve from 20 to the oracle’s 24 bytes. The stronger conclusion is that reusing a C identity changes the candidate/conflict population, and the resulting spills determine the frame. The camera renderer also has incoming coordinates, the historical copybuf pragma and several other calls. The fixture isolates identity without that pressure. [Four counters](exp/counters/a/probe.c) · [Two counters](exp/counters/b/probe.c) · [Full-unit DOS result](real/camera-reuse/comparison.json)

The full-unit follow-up provides a stronger control for the camera case. With declaration order `dest, row`, advancing dest before row and resetting both after rotatesprite, both complete renderers are STRICT. In the standalone trace the four original named counter conflicts had savings 1,446 for each row and 1,406 for each dest. The matching source has one named conflict of each savings, plus anonymous byte/word homes t253 and t254. Four counter-related word homes still appear. All three coordinate conflicts have savings 1,300 in both builds. Initially y takes EDI, z takes EBP and x spills. In the matching build z takes EDI, x takes EBP and y spills. An additional anonymous dword home at −20 and y at −24 complete the 24-byte reserve. In the sprite caller the same edit reduces the frame from 32 to 28 bytes, and in the mirror it remains 40 bytes. These traces establish the final allocation; the individual source-factor controls are tabulated in predictions.md. [Standalone trace comparison](refresh/r10/real/camera-swapboth/cg/render_camera_tile/delta.json) · [Sprite caller](refresh/r10/real/camera-swapboth/cg/render_sprite_camera/delta.json) · [Mirror caller](refresh/r10/real/camera-swapboth/cg/render_mirror/delta.json) · [Independent DOS rebuild](refresh/r10/real/camera-swapboth/independent/receipt.json)

## Equal savings and expression evaluation order

**PROVEN for two live values across a call on the engine profile.** Both named values have savings 8. The return temporary takes EAX. The allocator visits y first and assigns EDX, keeping the first eligible zero-score register when EBX/ECX/ESI/EDI/EBP also score zero and have never been used. It then gives x the first remaining eligible register, EBX. Reversing two independent source loads switches their emitted order and exchanges the colours of the anonymous load temporaries, while y still takes EDX and x EBX. Reversing only the declaration exchanges their backend IDs, while their visit order and final colours again stay fixed. Neither lexical order is a universal tie breaker. [Original](exp/colour/a/probe.c) · [Definitions swapped](exp/colour/b/probe.c) · [Declarations swapped](exp/colour/decl/probe.c)

The conflict sorter uses descending savings and the leaf colour loop compares scores among eligible, non-interfering masks. On equal scores it keeps its incumbent unless only the challenger is already used. Matching a competitor’s savings does not establish which register it blocked, or whether two tied conflicts interfered. The case-405 checkhitswitch trace illustrates this distinction: the manual 564/564 conflict identification and two EAX-versus-ECX operand differences have direct evidence. The automated explainer locates the instruction but does not uniquely identify the pair. [Fresh explanation](traces/checkhitswitch/explain.json) · [Original conflict experiment](../r8astra-sector/NOTES.md)

## Caller liveness changes the allocation of an expanded function

**PROVEN for a same-unit game_oe expansion.** The callee computes y from the global a and x, calls touch and subtracts b. The caller retains one value across the expansion. With `keep = b`, the trace records keep at savings 8 in EDX and the incoming x at savings 12 in EAX. With the single edit `keep = x`, keep disappears as a separate colour, and x lives through the call at savings 16 in EDX. The expanded y stays in EBX. Both caller bodies contain the touch call and have zero local bytes. Their extents are 38 and 34 bytes. The standalone callee’s before/after payload was checked separately. [Retain global](exp/inline/a/probe.c) · [Retain parameter](exp/inline/b/probe.c)

The predictor concerns where the *caller* obtains and last uses a live value. For each differently allocated real copy, check the standalone public, the local caller interval and all other callers independently. The PREMAP changes provide three measurements of one source edit. The caller that computes a mirror midpoint behaves differently from the sprite camera. [Real-unit predictions and outcomes](predictions.md)

## Scope of the evidence

### Catalogue entry for the PREMAP two-pass copy

**PROVEN on Watcom 9.5 GA, game_oe.** Two non-overlapping loops with the same short row and destination counters can retain distinct backend homes after optimization. Reusing the named counters across the second pass changed the interference of incoming coordinate values across calls. In `render_camera_tile`, the three coordinate conflicts kept identical savings of 1,300 while the spilled parameter changed from x to y. Changing either counter alone kept a 20-byte frame. Changing both restored 24 bytes. In the final source the counters are declared `short dest, row`, dest is advanced first, and the second reset follows `rotatesprite`. This reproduces all 816 bytes at `1:11380` and all 864 bytes of the expanded sprite-camera caller at `1:116b0`, including 39 and 45 reference records. The mirror caller remains NEAR at its first frame-size byte. A low-pressure two-loop fixture used BX/CX and zero stack bytes with either spelling. Conditions and counterexamples are therefore part of the rule. Reproduction from repository root uses `build/workers/blockers/camera.patch`, `build/workers/blockers/exp/summary.json` and `build/workers/blockers/refresh/r10/real/camera-swapboth/independent/receipt.json`.

The stock verifier establishes STRICT status. The synthetic runs establish behavior only for these inputs and profiles. The comparator and scoring mechanisms were previously recovered in CG95. The additions here are single-factor source controls, measured boundaries on what source changes can predict, and their cross-check against current full units. Watcom 10.0 audio cases have separate evidence. Fourteen verified-code dependencies refer to nine incomplete literal pools; the source-read-buffer residue model predicts some historical padding bytes and has not reproduced all nine pools. The engine RMI case concerns private BSS placement. Neither requires a new 9.5 optimizer rule. [CG95](../../../docs/CG95.md) · [Prior Watcom catalogue](../../../docs/WATCOM95_QUIRKS.md) · [Census](CENSUS.md)


# Watcom 9.5b second candidate sort

**PROVEN for the pinned 9.5b compiler, game_oe.** Given the optimized IR and the candidate list at entry, the second sort is reproducible. To predict whether one C temporary precedes another, first compute the comparator key. For equal keys, replay the sort on the *entire* input list. A stable sort of the same keys gives incorrect ranks in both GAME callers.

## Comparator and reference weight

The comparator at NT compiler RVA `3ce14` orders by these fields, in sequence.

1. Temporaries with `flags & 0x100` clear come first.
2. Within that class, records with a conflict pointer come first.
3. With two conflicts, larger **unsigned** 32-bit candidate savings come first. With neither, larger **signed** 32-bit backend temporary IDs come first. Equal savings end the comparison. The comparator never consults declaration position, symbol name, first or last use, width, stack offset or insertion address.

The producer at `3ce69` walks the optimizer's blocks immediately before invoking the sorter. For each instruction it visits every operand slot and the result slot. A qualifying reference to a temporary's canonical conflict contributes **one block weight**. An operand that represents indexed memory contributes its index temporary when eligible. The compiler follows alias links to the base temporary. An instruction reading and writing the same temporary contributes twice. Several C uses can share a single conversion in this IR. Aliases with the same conflict see the same accumulator.

| Backend loop depth | 0 | 1 | 2 | 3 | 4 | 5 or greater |
|---|---:|---:|---:|---:|---:|---:|
| Weight per qualifying IR reference | 1 | 20 | 400 | 8,000 | 160,000 | 3,200,000 |

The weight helper at `53e2a` reads the depth byte at block offset `4c` and caps the index at five. The accumulator is a 32-bit addition. These weights describe candidate priority at this pass. A later register conflict can have a different savings value for the same temporary. The read-only observer in [candidate_weights.py](candidate_weights.py) independently sums the optimized operands and results and compares the sum to the live field.

## How ties get their ranks

In these fixtures and callers, new records precede the surviving output of pass one, whose relative order is retained at entry to pass two. New records can appear when a live range splits. This is an observed input-list construction, separate from the comparator. The traced array sorter at `4c3c5` starts with gap equal to list length, then halves the gap with alternating additions of 0 and 1. For each gap, it scans forward and exchanges a pair only for a strict priority inversion. It repeats that gap until a scan makes no exchanges and stops after gap one. Two equal candidates can cross when either exchanges with a third record. Equal candidates already in sorted input can also remain in precise input order.

## Small single-factor C controls

The base [probe.c](candidate-sort/base/probe.c) loads two short globals into `left` and `right`, calls an external observer, passes both locals to an external function in a loop and passes each once after the loop. Ranks below are zero-based in pass two. All counts refer to independently predicted candidate savings. Each listed compile has an ordinary NT and a canonical DOS code/public/fixup control.

| Change from the indicated control | Measured effect |
|---|---|
| Base | Each local has one outer definition, one inner read and one outer read. Both score 22. Left and right occupy ranks 1 and 2. The loop counter scores 61, including a read and a write in the same increment. |
| Add a call using left ahead of the external observer | Left becomes 23. Repeating its call adjacent to its existing loop use leaves 22. The argument conversion is shared in the latter optimized IR. |
| Move only left's loop use outside | Left falls from 22 to 3, behind right at 22. Moving it to depths 2, 3, 4, 5 and 6 gives 402, 8,002, 160,002, 3,200,002 and 3,200,002. |
| Exchange the first pair of loop reads, the final pair of reads, or the two initial definitions separately | All three retain 22/22 and ranks 1/2. |
| Move left's declaration into a nested block, with a braces-only control | Both retain 22/22 and ranks 1/2. |
| Change only left from short to long | A 4-byte base and a 2-byte alias share its conflict, both scoring 22. They finish at ranks 1 and 3 with right at 2. |
| Change an external call argument from the address of the global to the address of left | Left loses its conflict, keeps flag 0x100 clear and moves from rank 1 to rank 7. Right stays at 22. Taking an address that an optimizer can eliminate has a different outcome in the GAME tests below. |
| Put the same body in a private, expanded callee | The two locals retain scores 22/22 and ranks 1/2 in the caller. |
| Exchange only the two local declarations | The backend IDs attach to the opposite names and the roles at ranks 1 and 2 reverse. Both scores remain 22. This edit acts through the input list. |

**Small reproduction of a real split.** In [shared-loops/probe.c](candidate-sort/shared-loops/probe.c), the two branches reuse the same two local declarations in disjoint loops. The first sort sees one named line value at 42. The second sees the named value and a new, anonymous one at 21, each with exactly one depth-zero write and one depth-one read. Two records enter between passes. The loop-counter halves, each at 82, enter at ranks 1 (anonymous) and 2 (named) and exit at ranks 1 and 0. Exchanging only the two C declarations eliminates that equal-key crossing. In the separate guarded-call fixture, redirecting just the extra call's argument from left to right exchanges the two locals at ranks 2/3 with unchanged 22/22. That change is inherited from pass one. These fixtures distinguish actual second-pass reordering from a changed input that is already sorted.

**A source edit that exchanges the two halves.** In this same small fixture, [passing left to one additional call](candidate-sort/split-use-first-left/probe.c) before the *first* loop raises the merged score from 42 to 43. After splitting, the first line value scores 22 and reaches rank 2. The second stays at 21 and reaches rank 3. In the base fixture these ranks were reversed. Passing the already loaded global `ga` at the same site has the same effect because the optimizer forwards the local value. Passing the loop counter there keeps the two line values at 21 and in the original order. Applying the extra left use only to the second loop raises only the anonymous half to 22. The extra calls are source controls in a synthetic program.

**Inlining control.** Two calls to the same short renderer can also produce distinct line-origin records in pass one, each at 21. The two-draw fixture gives the same identities and scores whether the static renderer definition precedes the caller or follows a forward declaration. Classify each expansion from both snapshots before assuming that its original C declaration split.

## GAME predictions

**operatefta.** In pass one `t15_left` covers both expanded lines and scores 42. After splitting, `t15_left` and the anonymous `t102` each have one outer definition and one newline-path read at depth one. Both therefore score 21. Pass two has 75 records, including three new ones. The anonymous origin enters at rank 2 and reaches 11 at gap 9 and 12 at gap 1. The named origin stays at rank 13 throughout. The only natural equal-key crossing in that pass is between the two text pointers. The two line origins themselves *retain* their relative order. The previously verified diagnostic exchange of output ranks 12/13 fixes four displacement bytes. A later conflict-sort exchange leaves all four.

**nonsharedkeys.** Pass two has 225 records, with two arrivals and one removal. Each expanded glyph has three definitions and one read at depth two, for 1,600. Each origin has one definition at depth one and one read at depth two, for 420. Glyphs `t160_ac/t218_ac/t190_ac` enter at ranks 12/13/14. At gap 7 they occupy 5/13/7. They finish at 9/11/10. Thus `t190_ac` overtakes `t218_ac` despite having identical priority. Thirteen equal-key pairs cross in this pass. In the earlier diagnostic, two exchanges among the glyphs followed by one home exchange reproduce all 2,080 bytes and fixups.

**Conditional source predictions.** Holding the optimized lists and all other fields fixed, adding one depth-zero reference only to the first operatefta origin, or removing one only from the second, predicts the complete desired order of its six live homes. Both edits together also predict that order. Across 25 combinations of symmetric glyph and origin changes for both expansions, none did. For nonsharedkeys, none of 25 symmetric changes to the three origins and three glyphs predicted the desired complete home order. The tested deltas per group were 0, ±1 and ±20 for operatefta and 0, ±20 and ±400 for nonsharedkeys. These are counterfactuals. Adding a C statement can also alter list membership, frontend identity or other weights. [Prediction replay](candidate_predict.py)

**Natural whole-unit controls.** The unforced GAME builds test both priority and list membership. A nonescaping pointer to minitext's saved origin is eliminated. Its anonymous origin changes ID from 102 to 103 and retains the same 21 and input/output ranks 2/12. The named origin remains at 13/13. operatefta retains all four differences. Naming the pointer to the second quote gives the same ranks but 34 differences elsewhere in the function. In nonsharedkeys, a short glyph copy keeps the 225-entry list, the three role ranks, and all 14 differences. Widening and sharing the glyph for the draw and table index changes the list to 223 entries. All three glyph scores remain 1,600, but the first and third glyphs exchange their final candidate ranks. This gives 37 differences and two relocation problems, and loses two previously STRICT publics. The full reference graph distinguishes a changed temporary ID from a changed priority, input rank, or complete-unit result. [DOS verdicts](candidate-sort/dos-summary.json) · [Rank audit](candidate-sort/rank-audit.json)

## Reproduction

**Validation totals.** The 55 synthetic cases and eight captures of GAME functions give 63 second-sort outputs and 63 first-sort outputs. All 126 complete orders replay exactly. The independent IR count reproduced all 1,044 scored records in pass one and all 1,087 in pass two. The two original GAME traces account for 44 and 126 scored records in pass two respectively. The other six GAME captures use unforced source variants.

Naming the newline height as a short in minitext leaves the two line values at 21/21 and preserves the 75-entry second sort. Sharing a single long x-coordinate between two nonsharedkeys labels produces 226 candidates, while all three glyphs still score 1,600. The whole function then has 741 differences. Across 58 DOS whole-unit source variants, 24 retained every existing STRICT public and the original public set. None improved either target under those conditions. One rejected private-helper layout also changed CONST padding. The final fresh DOS build with baseline sources retained 37/49 STRICT, 22,956 bytes, all three verified data segments and the original 4/14 differences. All work stays in this worker directory.

Run from the repository root. The harness uses `tools/wcc.py` with `game_oe` and private scratch paths. The observer reads optimizer state without forcing allocator decisions.

```powershell
$w = 'build/workers/flip2src'
python -B "$w/candidate_weights.py" --fixtures
python -B "$w/candidate_weights.py" operatefta nonsharedkeys
python -B "$w/candidate_predict.py"
python -B "$w/candidate_real.py"
python -B "$w/candidate_real.py" callers
python -B "$w/candidate_real.py" helpers
python -B "$w/candidate_real.py" literals
python -B "$w/candidate_real.py" minitext-local-address gametext-loop-glyph-short gametext-draw-tile operatefta-quote minitext-height-short-before nonsharedkeys-label-x-long-labels
python -B "$w/candidate_ranks.py"
python -B "$w/candidate_results.py"
python -B tools/match.py --obj "$w/candidate-sort/real/baseline/game.obj"
```
