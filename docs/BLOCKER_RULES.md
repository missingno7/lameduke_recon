# Watcom 9.5 GA rules for diagnosing a first difference

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

