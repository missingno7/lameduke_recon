# Engine worker pass 3

## DOS verification

| Engine profile | Integrated baseline | Worker src3 |
| --- | ---: | ---: |
| STRICT functions | 75 | 78 |
| STRICT bytes | 35,793 | 37,062 |
| CODE_BYTES_MATCH functions | 0 | 0 |
| NEAR_MATCH functions | 27 | 24 |
| Existing STRICT functions lost | 0 | 0 |

Both runs verified the complete 75-byte CONST and 237-byte _DATA segments. The BSS report lists the same eight unbound publics. The worker used the canonical symbol bindings. The changed files are [ENGINE.C](src3/ENGINE.C) and [PRAGMAS.H](src3/PRAGMAS.H). The other three files were copied from the integrated engine.

From the repository root, run the stock matcher through the worker launcher, which redirects compiler scratch and verifier artifacts to the worker directory.

```powershell
$w = 'build/workers/engfix2'
$env:LAMEDUKE_HOST = 'dos'
python -B "$w/run.py" match --profile engine "-i=$w/src3" "$w/src3/ENGINE.C" --json "$w/results3.json"
```

The pinned `wc95b` install is unpatched Watcom 9.5 GA, as established in [FINDINGS.md](../../../docs/FINDINGS.md). The engine profile is `-4r -s -or`. NT trials compiled and checked the entire unit serially. Retained candidates were verified again on DOS. The final matcher line is `# STRICT 78/102 functions, 37062 bytes`. Exit status 1 indicates remaining NEAR matches. [Baseline results](pass3/baseline.json) · [Final results](results3.json) · [Final verifier log](verification3.log)

## New STRICT functions

- **PROVEN — keepaway, 355 bytes, and rotatepoint, 179 bytes.** `getcliplines` now occupies exactly 3,589 bytes from `1:42589` through the byte before `addclipline`. The source computes the bounds and initializes the sector counters in the order emitted at `1:42615–42672`, decrements the wall bound separately as at `1:426af–426b7`, and compares the clip flag to 1 as at `1:4294a`. The floor-corner comparison uses the equivalent operand order selected by the verifier probes. With this extent, the unchanged shared-return branches at `1:434d5` and `1:4390f` match the oracle. `getcliplines` remains NEAR. [Combination probes](pass3/getcliplines-combination-trials.json)
- **PROVEN — draw2dgrid, 735 bytes.** `drawline16` now emits its own 1,541-byte oracle extent. Ken's `BUIL0422/BUILD.C` contains the inline `hlin16` implementation, with the same dword fill, byte remainder and updated pointer return as the oracle at `1:43b40–43b4e`. The historical pragma is restored in the header alongside the other planar primitives. `drawline16` names both aligned byte endpoints and adds the VGA base after the mask index write, consistent with `1:43ab8–43ae7`. The unchanged shared return at `1:44459` now verifies. `drawline16` remains NEAR. [Primitive probes](pass3/drawline16-hlin16-trials.json) · [Span setup probes](pass3/drawline16-byte-spans-trials.json)

## Remaining body differences

| Function | Oracle size | Emitted size before → after | Differing bytes before → after | First after |
| --- | ---: | ---: | ---: | --- |
| scansector | 756 | 756 → 756 | 7 → 7 | `+0x12a` |
| drawmaskwall | 1,890 | 1,890 → 1,890 | 288 → 288 | `+0xe4` |
| drawsprite | 9,875 | 9,875 → 9,875 | 7,056 → 7,056 | `+0x70` |
| hitscan | 1,745 | 1,745 → 1,745 | 1,145 → 1,074 | `+0x17` |
| getcliplines | 3,589 | 3,590 → 3,589 | 2,962 → 3,017 | `+0x3` |
| keepaway | 355 | 355 → 355 | 1 → 0 | — |
| rotatepoint | 179 | 179 → 179 | 1 → 0 | — |
| drawline16 | 1,541 | 1,543 → 1,541 | 1,301 → 935 | `+0x78` |
| draw2dgrid | 735 | 735 → 735 | 1 → 0 | — |
| getzrange | 2,938 | 2,628 → 2,938 | 2,285 → 2,492 | `+0x5` |
| setview | 334 | 335 → 334 | 221 → 213 | `+0x3` |
| rotatesprite | 1,497 | 1,538 → 1,532 | 1,315 → 1,308 | `+0x5` |

- **STRONG — hitscan.** The December instruction stream loads the second wall's X before the first wall's X and their Y values in the same order at `1:4128d–412e5`. The retained source orders the loads accordingly, keeps the original exact extent, and reduces verifier fixup problems from 26 to 22. [Endpoint probes](pass3/hitscan-endpoints-trials.json)
- **STRONG — getzrange.** The source reloads the packed wall and sector fields directly and runs the narrower box orientation and products after appending a newly reached sector, as at `1:45a26–45b0c`. It uses the signed terminal index for the duplicate search. The extent is now exact. Raw byte differences increased by 207 and fixup problems by 19, so substantial body work remains. [Portal and height probes](pass3/getzrange-portal-and-height-trials.json) · [Queue probes](pass3/getzrange-queue-trials.json)
- **STRONG — setview and rotatesprite.** `setview` computes the two final viewport bounds explicitly in the mode-7 branch and retains the exact-size stride selection. A diagnostic spelling with explicit multiplications by five and shifts preserves the oracle's EBP save and matches farther into the function. It is one byte short at 333 and has 127 differing bytes, so the candidate retains the exact 334-byte form, which uses a shared multiply. `rotatesprite` follows the oracle's sine/cosine load order at `1:46d46–46d4e` and splits its signed whole-texel step from the stride multiplication as at `1:47187–471ae`. Its extent is still 35 bytes long. [Viewport probes](pass3/setview-column-stage-trials.json) · [Viewport register control](pass3/setview-register-control-trials.json) · [Rotation probes](pass3/rotatesprite-geometry-lifetimes-trials.json)

## Constant audit and negative results

- **PROVEN — engine constant audit.** The engine's local `ENGINE.C`, `BUILD.H`, and `PRAGMAS.H` contain zero enum declarations. The compared limits include `MAXSPRITES`, `MAXSTATUS`, `MAXWALLS`, `MAXTILES`, `XDIM`, and `YDIM`, defined as macros in `BUILD.H`. `MAXCLIPNUM` and `MAXWALLSB` are macros in `ENGINE.C`. The available `BUIL0422` and `buil1995` engine source and headers also use macros for their named limits. There is no enum-to-define conversion to apply to the engine.
- **scansector.** The prior validated allocator trace still accounts for the same seven register differences in the branch starting at `1:343e0`. Thirteen new probes named the wall address or chain position and tested early continuation. None improved the verifier result. Existing many-way allocator ties, including those in `hline` and `calcwalls`, were left at their existing trace evidence. [Allocator explanation](pass2/scansector-explain.json) · [Wall-address probes](pass3/scansector-wall-address-trials.json)
- **drawsprite.** The fresh semantic comparison points to the wall-sprite shade load between the two panning stores at `1:3d9a3–3d9c0`. Twenty-two wall shade, face projection, and sector index source forms yielded no accepted improvement at the exact 9,875-byte extent. The integrated body was retained. [Shade and projection probes](pass3/drawsprite-shade-and-projection-trials.json)
- **Other remaining NEAR functions.** The fresh semantic baseline covers all 27 integrated NEAR functions. The PCX raster probes preserved the 316-byte baseline of `screencapture`. The existing `printext16` backport was retested with the corrected preceding extents. It still emits 747 of the 748 oracle bytes, with 152 differing bytes and no fixup problems. Adding it regresses ten STRICT functions, so it remains a diagnostic. `printext256` and the later shared-return group still depend on recovering this missing routine and the remaining size differences. [Semantic evidence](pass3/semir/) · [PCX probes](pass3/screencapture-raster-trials.json) · [Text renderer control](pass3/printext16-control.json)

