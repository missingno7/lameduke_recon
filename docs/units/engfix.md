# December Build engine SOURCE pass

The candidate is in `src/`. The worker copy uses Watcom 9.5b with engine flags `-4r -s -or`. The baseline DOS compile yielded 58 STRICT functions and 28,526 verified bytes. The final DOS compile yielded 64 STRICT of 102 publics and 31,408 verified bytes with the current registry and worker overlay. All 58 original STRICT functions held. Both builds verified the complete 75-byte CONST and 237-byte _DATA segments.

The retained changes below are at different levels of verification. STRICT rows have full byte, extent, fixup-site, and target verification. Other listed source forms follow the cited evidence and still have byte differences. The Ghidra drafts helped locate control flow. The immutable D3D oracle and dated Ken sources determined the changes.

## Source evidence and outcomes

| Original SOURCE function | Retained change and evidence |
| --- | --- |
| `drawrooms` | **STRICT.** Use March's long loop index and closest value, with April's independent tests of the result and winning bunch index. December `1:341bb-3422b` compares the result with the live index and uses separate tests. Both branches to the shared return verify. |
| `scansector` | Restore the explicit sprite-list loop and separate index increment suggested by December `1:342f8-34316` and `1:344b2-344c0`. This emits the 756-byte target extent. The scan and bunch region still diverges. |
| `hline` | Group the coordinate products as in April's renderer and pass planar shade as a shifted argument. December `1:39ad5-39b55` and `1:39da0-39dd8` support those forms. The final byte differences are the eight stack-offset bytes for `xr` and `r`, whose homes are exchanged. All fixup sites and targets agree. |
| `screencapture` | Retained the baseline body after unsuccessful trials with inverse-flag declaration, pointer signedness, and PCX header store order. The role of the 32-bit stack homes and the two pixel-fetch paths at `1:3b61d-3b6fa` remains open. |
| `drawsprite` | Restore 32-bit wall-sprite projection products and arithmetic shifts at `1:3d454-3d4e9`, then swap endpoints before testing the two depth values and the one-sided bit. Move constant floor-sprite palette setup into this caller and pass raw tile size shifts per `1:3f069-3f107`. November 29 through December 6 BUILD.TXT entries date the floor sprites, translucency, and one-sided controls. |
| `ceilspritehline` | **STRICT.** Read half-width from `xdimen`, use the page directly in the planar address, and split the 320- and 640-wide linear paths. December `1:3f2e0-3f3a3` sets increments before testing the mode. Shade resides in `drawsprite`. March/June preserve the general span writer. |
| `overwritesprite` | Revert later stereo and flip paths. December `1:3f733-3f778` subtracts both center offsets directly, and `1:3f920-3fa88` uses the earlier clipping, VGA address, and zero column phase. The August 1 and February 7 revision entries distinguish dated changes to sprite writers. The candidate is three bytes short. |
| `permanentwritesprite` | Use `globalpal` at `1:3faec-3fafd`. The March source uses the later palette argument documented in the January changes. Clipping and writer allocation remain open. |
| `grouvline` | Compute the half-width and planar page address directly, then use 32-bit grid projection and crossing products per `1:400b2-401f6` and `1:402d9-40305`. Watcom still allocates the argument and traversal state differently. |
| `cansee` | Restore the April function body with December's equal-XY fast path. The short compares, ascending wall index, and duplicate-sector loop are corroborated at `1:41023-41056` and `1:41171-411b2`. Only the two bytes of the shared-return displacement at `1:411bf` still differ. |
| `hitscan` | Restore indexed wall traversal, a named second vertex, and the early hit-sector reset. Use foot-centered sprite height with the target's two shifts at `1:41710-4172f`. BUILD.TXT dates the later hitscan cleanup on March 14 and y-centering on February 16. Most remaining differences are in local homes and scheduling. |
| `neartag` | Restore wall-index traversal, tag tests before coordinate loads, foot-centered sprite height, and two separate scaled distance products. December `1:419c0-41a4c` and `1:41b43-41c23` support the wall and distance forms. The candidate has the full 1,644-byte extent. |
| `lintersect` | **STRICT.** Retain April's segment/ray intersection shape, use Ken's existing inline absolute value primitive at `1:4205d-4206f`, and reject a zero determinant before calculating the numerator, as shown at `1:420b1-420c9`. |
| `clipmove` | Use April's pre-tested loop, the six-argument `raytrace` call, signed direction comparisons and ascending sector fallback. December `1:42330`, `1:423bd-423d8`, and `1:42536-42576` support the call, slide shifts and fallback. The candidate still differs in extent and local allocation. |
| `getcliplines` | Restore the doubled wall radius, historical sector-list setup, clip type 2 drop check, endpoint wall IDs, and earlier face/wall/floor sprite bounds and clip construction. December `1:42589-42672`, `1:42934` and later sprite paths support these. BUILD.TXT dates `scaredfallz` and clip type 2 on July 5, and later sprite centering on February 16. The wall and sprite loops remain NEAR. |
| `raytrace` | Restore April's six-argument ABI, initial pointer stores, and indexed forward search, with December's 256-step correction and fallback. December `1:43532-43571` and `1:4367f-43718` support the ABI and retry. This candidate produces the full 613-byte extent. |
| `drawline16` | Restore preliminary and post-clipping rejection, separate vertical and horizontal paths, and December's sequence of 32-bit clipping quotients at `1:43947-43a3f` and `1:43b5f-43d97`. The horizontal span fill and stack allocation remain open. |
| `qsetmode640480` | **STRICT with corrected entry.** The pre-existing binding `1:43f42` identifies the 350-line routine. The actual 480-line routine starts at `1:43ff8`, after the preceding return. BIOS modes `0x10` and `0x12`, their state stores, and June's two source bodies identify both. The June port writes were backported to the ordered byte writes at `1:43f8d-43fbe` and `1:44031-440c7`. |
| `draw2dscreen` | Restore ascending wall indices, the wall color and four-pass highlight outline, and the 4,096-index sprite highlight offset at `1:444a8-44533`, `1:446df-4479b`, and `1:44bbd-44c18`. Its own body ends at `1:450e0`, before four separately identified routines. Compare against 3,202 target bytes. |
| `getzrange` | Restore the immediate sector height stores and doubled wall distance at `1:45670-45702`, clip type 2, the narrower portal test using `walldist`, and earlier face/wall/floor sprite bounds at `1:4598e`, `1:45a26-45b32` and the following sprite phase. The later sector-number guard and y-centering path were absent there. Sprite and wall scheduling remain NEAR. |
| `setview` | Retained the baseline body. Moving the `windowy1` store, permuting locals, and reshaping the final `ytop` and `ybot` assignments failed to reproduce the entry saves and store schedule at `1:462c9-46305` and `1:463df-4640b`. |
| `makepalookup` | Read the base palette pointer before applying the masked remap index. Keep the target's color-before-distance stores. December `1:472dc-4730b`, `1:4734f-47366`, and `1:47437-47441` support those forms. The candidate still has a three-byte deficit and a shared-return displacement difference. |

## Dependencies and additional entries

- The 554-byte pointer form of `wallfront` from the previous worker's `work5/combined-554.c` balances the eight restored bytes in `scansector`. The two `drawrooms` return displacements are STRICT with this pair. `wallfront` itself is still NEAR.
- The existing inline assembler primitive `drawpixel16` in `PRAGMAS.H` now uses the older mask and latch operation repeated at `1:43e48-43e66`, `1:44892-448b0` and `1:44cc4-44ce2`. June's header establishes the inline-pragma form. The target instruction sequence clobbers EAX, ECX, and EDX in addition to the EDI argument.
- The separate `qsetmode640350` is 182 bytes and STRICT. The proposed canonical address update for `qsetmode640480` is `1:43ff8`. The current registry cannot override an existing address via an overlay. The worker overlay provides a temporary `qsetmode640480_dec94` diagnostic public at the correct address. The separate DOS control changes only that public's name. Its segment bytes, coverage and OMF fixups all equal the worker candidate's.
- The end of `draw2dscreen` is the jump at `1:450db`. June ENGINE.C establishes `printext16` and `printext256`, and April BUILD.C establishes `printmessage16` with 56 columns and the arguments `184, 8, 0, 6`. BUILD.TXT documents `getspritescreencoord` and its three-argument ABI. The overlay records their independent addresses. The 87-byte message wrapper and 164-byte coordinate function are STRICT. The backported 384-byte `printext256` differs by two bytes at its shared return. `printext16` remains an external reference for the message wrapper.
- The June `printext16` backport produced 751 candidate bytes and 565 differences. Adding it regressed seven baseline functions, including `wallscan` and `loadboard`. That trial was discarded. The NT probe did independently verify both new companion functions, and their DOS results are in the final report.
- NT controls with separate late `hline` address addition regressed up to four previously STRICT functions. The selected arithmetic grouping and shade argument recovered their spans. The same-size stack-home experiments did not eliminate the last eight offsets. Changing the makepalookup remap pointer to signed char moved farther from the oracle.

## Reproduce

Run from the repository root. The worker launcher directs the stock compiler scratch into this worker folder. It invokes the repository matcher unchanged. The symbol probe uses a separately compiled source copy.

```powershell
$w = 'build/workers/engfix'
Remove-Item Env:LAMEDUKE_HOST -ErrorAction SilentlyContinue
python -B "$w/verify_dos.py"
python -B tools/match.py --obj "$w/final.obj" --symbols "$w/overlay.json" --json "$w/results.json"
```

The matcher exits 1 for these partial objects. See `verification.log` for both compiler and matcher reports. See `near-after.json` for the original NEAR claims rerun with the corrected 480-line entry. The results preserve the source work even where displacement or allocation still prevents STRICT.

## Final DOS semantic census

This table covers the original 98 functions. The after column uses the corrected 480-line entry. The four additional publics are outside these counts.

| Class | Before | After |
| --- | ---: | ---: |
| STRICT | 58 | 62 |
| SOURCE | 22 | 13 |
| ALLOCATION-ONLY | 3 | 5 |
| LAYOUT | 13 | 14 |
| UNKNOWN | 2 | 4 |

`hline` and `raytrace` moved to ALLOCATION-ONLY. `cansee` moved to LAYOUT. `getcliplines` has an unresolved semantic comparison. The `permanentwritesprite` comparison timed out after 12 seconds. Both are NEAR in the strict verifier.

The bounded semantic pass is reproducible with `python -B build/workers/engfix/census_dos.py` after the DOS build.

| Original SOURCE function | Baseline candidate bytes | Final candidate bytes | Final differing bytes | Final first differing byte | Final class |
| --- | ---: | ---: | ---: | --- | --- |
| `drawrooms` | 928 | 928 | 0 | - | EXACT |
| `scansector` | 748 | 756 | 351 | `+0x2a` | SOURCE |
| `hline` | 965 | 965 | 8 | `+0xb` | ALLOCATION-ONLY |
| `screencapture` | 1,511 | 1,511 | 316 | `+0x34` | SOURCE |
| `drawsprite` | 10,422 | 10,021 | 7,338 | `+0x3c` | SOURCE |
| `ceilspritehline` | 1,044 | 976 | 0 | - | EXACT |
| `overwritesprite` | 1,323 | 1,060 | 721 | `+0x19` | SOURCE |
| `permanentwritesprite` | 1,358 | 1,361 | 777 | `+0x19` | UNKNOWN |
| `grouvline` | 2,337 | 2,148 | 1,643 | `+0x1b` | SOURCE |
| `cansee` | 388 | 430 | 2 | `+0x1aa` | LAYOUT |
| `hitscan` | 1,659 | 1,742 | 409 | `+0x28` | SOURCE |
| `neartag` | 1,583 | 1,644 | 222 | `+0x18` | SOURCE |
| `lintersect` | 548 | 545 | 0 | - | EXACT |
| `clipmove` | 807 | 799 | 645 | `+0x1b` | SOURCE |
| `getcliplines` | 3,585 | 3,201 | 2,810 | `+0x6` | UNKNOWN |
| `raytrace` | 524 | 613 | 299 | `+0x5` | ALLOCATION-ONLY |
| `drawline16` | 1,171 | 1,543 | 1,301 | `+0x12` | SOURCE |
| `qsetmode640480` | 272 | 312 | 0 | - | EXACT |
| `draw2dscreen` | 3,169 | 3,167 | 2,716 | `+0x5` | SOURCE |
| `getzrange` | 2,819 | 2,527 | 2,245 | `+0x5` | SOURCE |
| `setview` | 336 | 336 | 221 | `+0x3` | SOURCE |
| `makepalookup` | 593 | 596 | 314 | `+0x19` | SOURCE |
