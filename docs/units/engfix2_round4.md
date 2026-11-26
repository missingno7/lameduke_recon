# Engine worker pass 4

## DOS verification

| Engine profile | Integrated baseline | Worker src4 |
| --- | ---: | ---: |
| STRICT functions | 78 | 78 |
| STRICT bytes | 37,062 | 37,062 |
| CODE_BYTES_MATCH functions | 0 | 0 |
| NEAR_MATCH functions | 24 | 24 |
| Existing STRICT functions lost | 0 | 0 |

The before and after STRICT sets are identical. Both runs verified all 75 CONST bytes at `2:01184` and all 237 _DATA bytes at `2:02c00`. The candidate changes only [src4/ENGINE.C](src4/ENGINE.C). The other files were copied from the integrated engine. No overlay was used. The final DOS check was on the same object used for the BSS audit. [Baseline match](pass4/baseline.json) · [Final match](results4.json) · [Verifier output](verification4.log)

Run from the repository root. The worker launcher calls the stock tools and confines their outputs to the worker folder. The pinned install is Watcom 9.5 GA with engine flags `-4r -s -or`.

```powershell
$w = 'build/workers/engfix2'
$env:LAMEDUKE_HOST = 'dos'
python -B "$w/run.py" wcc --profile engine "$w/src4/ENGINE.C" "-i=$w/src4" "-fo=$w/pass4/final.obj"
python -B "$w/run.py" match --obj "$w/pass4/final.obj" --json "$w/results4.json"
python -B "$w/pass4/check_bss.py" "$w/pass4/final.obj" --name bss-final
```

The stock matcher reports `# STRICT 78/102 functions, 37062 bytes` and exits 1 with 24 NEAR functions. NT probes checked the full unit between changes, and retained changes were confirmed on DOS.

## Retained source changes

| Function | Extent | Byte differences before → after | Fixup problems before → after | First difference |
| --- | ---: | ---: | ---: | --- |
| drawsprite | 9,875 | 7,056 → 7,056 | 528 → 528 | `+0x70` |
| getcliplines | 3,589 | 3,017 → 2,962 | 110 → 108 | `+0x3` |
| getzrange | 2,938 | 2,492 → 2,501 | 79 → 78 | `+0x5` |
| scansector | 756 | 7 → 7 | 0 → 0 | `+0x12a` |
| hitscan | 1,745 | 1,074 → 1,074 | 22 → 22 | `+0x17` |

- **PROVEN — face-sprite shade immediate.** The December face path computes `yp >> (visibility+4)`, as shown by the ADD and SAR at `1:3cd8b–3cd8e`. Updating the source changes exactly one emitted _TEXT byte in the isolated NT control, from 5 to 4 at `drawsprite+0x308`. The function extent and all prior STRICT functions are preserved. The stock comparison still counts the same total byte differences because that instruction is in a shifted region. [Shade control](pass4/shade-proof.json)
- **STRONG — top-tested sprite walks.** Expressing the walks as loops with a separate load of the head and a tail assignment follows the signed loads and tests at `1:42aec–42b07` for `getcliplines` and the analogous sprite phase of `getzrange`. Both functions keep their exact extents. `getcliplines` loses 55 differing bytes and two fixup problems. `getzrange` loses one fixup problem and gains nine differing bytes. The latter is retained for the source shape and fixup improvement. [Clip bounds probes](pass4/getcliplines-bounds-trials.json) · [Z range probes](pass4/getzrange-contact-trials.json)

## BSS declaration order

**PROVEN — 203/203 candidate OMF public offsets, 195/203 independently bound oracle offsets.** Watcom's preprocessed declaration prefix before the first `keyhandler` definition contains 577 distinct names. A separate compile reads each allocation's `sizeof` from an initialized array. Given those sizes and textual order, `tools/bssrule.py` predicts every named offset at first slot 18 and alignment 1. All 195 bound publics give the same LE base `2:b1e00`. The eight remaining names are present in the object, but this engine unit has no active references that independently attach them to the original image.

| Unbound name | Predicted LE location |
| --- | --- |
| startposx | `2:1bc33c` |
| startposy | `2:1bc340` |
| startposz | `2:1bc344` |
| oldzerohandler | `2:1bce30` |
| hitcnt | `2:1bde50` |
| posfil | `2:1bde52` |
| startang | `2:1bde5e` |
| startsectnum | `2:1bde68` |

The named allocations account for 1,097,887 bytes. The anonymous 50-byte VESA real-mode record starts at `2:1bde9f`, bringing the _BSS to 1,097,937 bytes. The diagnostic script checked all eight associated `setvesamode` relocation sites against the LE target and found each at the predicted field address. The stock matcher still lists those anonymous references as unverified through the whole BSS segment. No verifier or symbol registry changes were made. [Ordered declaration input](pass4/bss-final-declarations.json) · [OMF, LE and VESA cross-check](pass4/bss-final-report.json)

## Negative results and remaining work

- **drawsprite.** The face path in the oracle omits the integrated bound clamps before computing the X increment at `1:3cf0a`. The oracle also uses only the new Y origin after vertically flipping the source at `1:3cfab–3cfb6`. The 31 face-shape trials and 36 wall-shape trials tested these and changes to reciprocal setup, screen width and wall-coordinate lifetimes. A 9,945-byte trial reduced diagnostics to 6,377 differing bytes and 509 fixup problems, but its 70-byte excess regressed ten existing STRICT functions. The larger face and wall changes remain unmerged. [Face tests](pass4/drawsprite-face-trials.json) · [Wall tests](pass4/drawsprite-wall-trials.json)
- **scansector and hitscan.** Tested 14 extra scan-sector forms based on the historical wall loop and the branch schedule before the current seven-byte mismatch. None changed the accepted result. The existing trace still attributes an endpoint register conflict around `1:343e0–343ef`. In `hitscan`, 17 traversal and 17 hit-output forms found no accepted improvement at its 1,745-byte extent. [Scan tests](pass4/scansector-traversal-trials.json) · [Hit traversal](pass4/hitscan-traversal-trials.json) · [Hit outputs](pass4/hitscan-output-order-trials.json)
- **Other layout choices.** The wall-product-first and covered floor-sprite height forms in `getzrange` yielded a 2,962-byte comparison candidate with 2,372 differing bytes. The retained source keeps the correct 2,938-byte extent. Viewport, map drawing, sprite overwrite, and groove-origin probes produced no accepted variant. [Viewport](pass4/setview-viewport-trials.json) · [Map drawing](pass4/draw2dscreen-viewport-trials.json) · [Overwrite](pass4/overwritesprite-palette-rows-trials.json) · [Groove](pass4/grouvline-origin-cache-trials.json)
- **Late shared returns.** Inspection of the earlier 747-byte `printext16` control explains why just filling the missing interval is insufficient. Its presence causes `wallscan` to use a different shared epilogue, shortening the body by one byte, and moves the forward shared-return displacement in `drawrooms` by about 45 KiB. The oracle `printext16` jumps to the `wallscan` epilogue at `1:3848e`. The diagnostic control uses a different stack-cleanup path ending in `raytrace`. The exact source for this missing function remains open. [Prior printer control](pass3/printext16-control.json)

Fresh [semir results](pass4/semir/) cover all 24 baseline NEAR functions. They classified 12 SOURCE, five LAYOUT, four ALLOCATION-ONLY, and three UNKNOWN. Previously traced allocation ties were left with their existing evidence.

