# ENGINE.C matching pass

This worker starts from the integrated 102-public engine candidate and uses the pinned Watcom 9.5b engine profile (`-4r -s -or`). The canonical verification is a fresh DOS-hosted compile. NT compiles were used for serial source probes. The worker launcher routes the unchanged repository tools' scratch and output into this directory. Existing symbol bindings suffice.

```powershell
$w = 'build/workers/engfix2'
Remove-Item Env:LAMEDUKE_HOST -ErrorAction SilentlyContinue
python -B "$w/run.py" match --profile engine "-i=$w/src" "$w/src/ENGINE.C" --json "$w/results.json"
python -B "$w/summary.py" results.json
```

The matcher exits 1 while any public is NEAR. See `verification.log` for full DOS diagnostics. `baseline.json` and `baseline.log` came from a fresh build of a byte-for-byte copy of the integrated source.

| DOS verifier result | Baseline | Candidate |
| --- | ---: | ---: |
| STRICT functions | 65 | 72 |
| STRICT bytes | 31,720 | 33,220 |
| CODE_BYTES_MATCH functions | 0 | 0 |
| NEAR_MATCH functions | 37 | 30 |
| NEAR_MATCH candidate bytes | 47,840 | 46,177 |

All 65 baseline STRICT functions passed in the final candidate. The seven additions are `spritewallfront` (114 bytes), `initspritelists` (206), `insertspritesect` (158), `insertspritestat` (156), `deletespritesect` (219), `deletespritestat` (217) and `cansee` (430).

## Proven changes

- `spritewallfront` at `1:347d6` verifies for 114 bytes. The preceding cg95 candidate gave the correct seven fixups and two differing comparison bytes. Named X and Y deltas preserve the allocator's correct X/Y conflict order. Defining the sprite Y delta before the wall X delta and comparing `sdy*wdx >= wdy*sdx` produces the oracle's `cmp eax,esi` and `setge al` at `1:34839-3483d`. The whole-unit DOS verifier independently confirmed the result.
- Five sprite-list functions and `cansee` verify their original shared-return instructions after source changes in the renderer. The list routines jump into `drawmasks` at `1:3c31c`. `cansee` jumps into `copytilepiece` at `1:3bd25`. Their bodies did not require edits. Multiple intervening body extents still differ and their signed size differences sum to zero across these intervals. These matches establish their function bytes, extents and fixup targets.
- The 75-byte CONST and 237-byte _DATA segments are covered by the ordinary DOS verification. The verifier still reports unbound _BSS publics.

## Source evidence retained

- `scansector` now assigns `numscansbefore` before copying the same value into `scanfirst`. The oracle loads AX, spills the original count and copies it to BX at `1:3434d-34358`. Naming the wall endpoint and the right-screen bound also restores the order of the sprite picture and depth stores at `1:34563-3456f`. The remaining differences are in the bunch loop.
- `drawmaskwall` uses one named X repeat and a separately assigned texture numerator. The oracle loads the repeat into EBP at `1:3c3f8` and uses it at `1:3c408` and `1:3c453`. The t37 spill in the original trace overlapped an earlier temporary in EBP. The new trace exposed the exchanged stack homes for `z1` and `z2`. Reversing their declaration order moved the verifier's first difference from `+0x6f` to `+0xe4` and reduced differences from 300 to 288. The candidate has the full 1,890-byte extent.
- `drawsprite` restores the foot-centered Z projection for the face and wall paths. `1:3ccc6-3cd31` subtracts the full projected height without the half-height adjustment. `1:3d936-3d96e` subtracts the tile Y offset and uses the resulting Z with the full-height span. The separated face screen origin and wall Z offset follow those operations.
- `overwritesprite` splits the right-bound assignment and decrement. DOS tracing located the first aligned body difference in the palette-offset stack home at `1:3f6d7`. The changed right-bound lifetime reduces body differences. The function size is still short.
- `grouvline` separates the rotations from the world-space additions and reuses the scaled world origin between the two endpoints. The oracle reuses the values in `[esp+0x30]` and `[esp+0x34]` at `1:401c2-40242`. This version has three more differing bytes and three fewer fixup problems. Its ten additional bytes close the six shared-return displacements.
- `clipmove` initializes the original direction from the scalar arguments and visits the same saved wall indices with an explicit ascending loop. The trace anchored an allocation difference on the first wall-coordinate load at `1:42343`. The new source reduced 645 differing bytes to 589 and closed three bytes of its target extent deficit.
- `getzrange` compares the signed wall-box product difference with zero, following the SUB/TEST sequence at `1:45849-45866`. The direct-index and alternate product ordering probes remain unmerged.
- `makepalookup` initializes the copy-loop counter outside its induction statement in the palette-remap pass. The oracle initializes it during pointer setup at `1:472ef-472f7`. The remap loop still has a three-byte deficit and outstanding register and fixup differences.

## Diagnostics and negative results

The 37-entry baseline census is under `semir-base/`. It yielded 15 LAYOUT, 13 SOURCE, five ALLOCATION-ONLY and four UNKNOWN. The baseline semantic process for `permanentwritesprite` exceeded 18 seconds. The debugger produced its 158-event trace with DOS/NT CODE agreement. The explanation stalled in the same semantic comparison and was stopped after about 57 seconds. The three other UNKNOWN entries are `calcwalls`, `getcliplines` and `rotatesprite`.

Use `traces/*-explain.json` for attribution and the adjacent trace and object for raw evidence. The trace projection checks all CODE bytes, coverage, publics and compiler fixups against a fresh normal NT compile and a fresh DOS compile. The full matcher remains the acceptance criterion.

- `calcwalls` exchanges the homes for `topinc` and the temporary sky tile at `1:34b47`, `1:34c04` and eight later accesses. Alternate uses of existing temporaries and assignment spelling failed to improve the ten differing bytes without a regression. `hline` left `r` and `xr` exchanged at `[esp+4]` and `[esp+8]`; altered denominator staging and shading forms failed to recover the eight bytes.
- `wallfront` starts with a conversion of `l1` in EAX, where the oracle uses EDX. Direct coordinate indexing and named wall/point indices did not improve the integrated candidate while keeping its adjacent callers STRICT. `raytrace` rephrased the source endpoints, products and retry without resolving the 48-byte versus 52-byte stack-frame allocation or improving the verified comparison.
- `setview` parameter preservation and the final paired stores still diverge. Product order, source placement of `windowy1`, early returns and independent `ytop`/ `ybot` forms failed to improve the full verifier result. The candidate remains two bytes longer.
- The existing `printext16` binding at `1:450e0` covers 748 bytes through `1:453cc`. Substituting the existing `mul80` primitive in the June source reproduces the LEA/SHL address computation at `1:45192-45195`. This NT diagnostic emits 747 bytes and has 152 differing bytes, with eight STRICT regressions elsewhere in the unit. It remains in `best-printext16.C`.
- `hitscan` and `neartag` traces exposed exchanges of the `xe`/`ze` and `zs`/`vz` stack homes. Reordering the endpoint assignments, splitting their signed wall indices and shortening the zero-Z setup yielded no accepted improvement. `screencapture` retained its 316 differing bytes after width and lifetime probes for the inverse flag.
- Size changes in `getcliplines`, `getzrange`, `draw2dscreen`, the missing `printext16`, and the sprite renderers still affect the other cross-function transfers. Shared-return displacements remain part of the unmodified byte comparison.

The trial scripts and their logs record the attempted source forms under the same engine profile. Each compiling trial compared the whole unit. Promotion required a DOS rebuild and retention of the accepted STRICT set.

| Changed function | Baseline bytes / differing bytes | Candidate bytes / differing bytes | Status | First difference |
| --- | ---: | ---: | --- | --- |
| `scansector` | 756 / 351 | 756 / 7 | NEAR_MATCH | `+0x12a` |
| `spritewallfront` | 114 / 7 | 114 / 0 | STRICT | - |
| `drawmaskwall` | 1,887 / 1,087 | 1,890 / 288 | NEAR_MATCH | `+0xe4` |
| `drawsprite` | 10,021 / 7,338 | 9,889 / 6,614 | NEAR_MATCH | `+0x3c` |
| `overwritesprite` | 1,060 / 721 | 1,057 / 476 | NEAR_MATCH | `+0x19` |
| `grouvline` | 2,148 / 1,643 | 2,158 / 1,646 | NEAR_MATCH | `+0x1b` |
| `clipmove` | 799 / 645 | 802 / 589 | NEAR_MATCH | `+0xa` |
| `getzrange` | 2,527 / 2,245 | 2,483 / 2,209 | NEAR_MATCH | `+0x5` |
| `makepalookup` | 596 / 314 | 596 / 313 | NEAR_MATCH | `+0x19` |
