# Engine worker pass 2

## DOS verification

| Engine profile | Integrated source | Worker src2 |
| --- | ---: | ---: |
| STRICT functions | 72 | 75 |
| STRICT bytes | 33,220 | 35,793 |
| CODE_BYTES_MATCH functions | 0 | 0 |
| NEAR_MATCH functions | 30 | 27 |
| Existing STRICT functions lost | 0 | 0 |

The fresh DOS baseline and final reports are [baseline.json](pass2/baseline.json) and [results2.json](results2.json). Both verified all 75 bytes of CONST at `2:01184` and all 237 bytes of _DATA at `2:02c00`. BSS has the same eight unbound publics. The verifier used the canonical bindings throughout. The candidate is [src2/ENGINE.C](src2/ENGINE.C), with the four companion files copied from the integrated engine.

From the repo root, run the pinned Watcom 9.5b DOS compiler with engine flags `-4r -s -or`. The launcher calls the stock matcher and redirects its scratch paths into this worker directory.

```powershell
$w = 'build/workers/engfix2'
$env:LAMEDUKE_HOST = 'dos'
python -B "$w/run.py" match --profile engine "-i=$w/src2" "$w/src2/ENGINE.C" --json "$w/results2.json"
```

The verifier reports `# STRICT 75/102 functions, 35793 bytes`. Exit status 1 reports the 27 remaining NEAR functions. The complete console log is [verification2.log](verification2.log). NT probes ran serially against the full unit. Accepted changes were checked again on DOS.

## Evidence for the new STRICT functions

- **PROVEN — permanentwritesprite, 1,358 bytes.** DOS `1:3fc99–3fcb8` selects the VGA map mask from the loop's plane before adding the left bound to form X. Changing the source to follow that order left a single displacement byte at `+0x3c4` with the old renderer extents. The final grouped sizes put the shared return in `copytilepiece` at the expected address `1:3bd25`.
- **PROVEN — clipmove, 813 bytes.** The April BUILD snapshot uses directly indexed hit wall endpoints in the opposing tangent test. That form and saving the original vectors after the zero-movement guard gave the target extent with seven EBP/EDI byte differences and all fixups verified. Declaring the saved Y vector before the saved X vector gives both vectors their oracle registers. See the [trace explanation](pass2/clipmove-explain.json) and [source trials](pass2/clipmove-last-trials.json).
- **PROVEN — ceilspritescan, 402 bytes.** Its unchanged source emits the oracle's branch at `1:3f2a4` to the shared return in `wallfront` once `drawsprite` has its own exact extent. The six existing shared-return matches still pass after making all four renderer extents individually exact.

## Extents and remaining differences

Sizes use independently bound oracle boundaries. Differing bytes and first offsets are the verifier's diagnostics. Layout was prioritized for source-supported changes, including some candidates that increase the reported body differences.

| Function | Oracle size | Size before → after | Differing bytes before → after | First after |
| --- | ---: | ---: | ---: | --- |
| scansector | 756 | 756 → 756 | 7 → 7 | `+0x12a` |
| drawmaskwall | 1,890 | 1,890 → 1,890 | 288 → 288 | `+0xe4` |
| drawsprite | 9,875 | 9,889 → 9,875 | 6,614 → 7,056 | `+0x70` |
| overwritesprite | 1,063 | 1,057 → 1,063 | 476 → 702 | `+0x49` |
| permanentwritesprite | 1,358 | 1,361 → 1,358 | 777 → 0 | — |
| grouvline | 2,169 | 2,158 → 2,169 | 1,646 → 1,627 | `+0x5` |
| hitscan | 1,745 | 1,742 → 1,745 | 409 → 1,145 | `+0x17` |
| clipmove | 813 | 802 → 813 | 589 → 0 | — |
| getcliplines | 3,589 | 3,201 → 3,590 | 2,810 → 2,962 | `+0x3` |
| keepaway | 355 | 355 → 355 | 2 → 1 | `+0x108` |
| rotatepoint | 179 | 179 → 179 | 2 → 1 | `+0xaf` |
| drawline16 | 1,541 | 1,543 → 1,543 | 1,301 → 1,301 | `+0x12` |
| draw2dgrid | 735 | 735 → 735 | 2 → 1 | `+0x2db` |
| draw2dscreen | 3,202 | 3,167 → 3,202 | 2,716 → 2,727 | `+0x5` |
| getzrange | 2,938 | 2,483 → 2,628 | 2,209 → 2,285 | `+0x5` |
| setview | 334 | 336 → 335 | 221 → 221 | `+0x3` |
| makepalookup | 599 | 596 → 599 | 313 → 312 | `+0x19` |

- **STRONG — renderer source.** `drawsprite` writes the full 32-bit `dwall` entries in the fully occluded face-sprite case, restores the floor-sprite depth shade seen at `1:3f004–3f028`, and uses the oracle comparison order for the one-sided floor sprite. `overwritesprite` evaluates the shade offset and palette base separately and computes the right bound as `x2-1`. `grouvline` calculates the planar base before adding the page and adds the world origins as each endpoint coordinate is formed, consistent with `1:400e2–400f3` and `1:401c2–40242`. [Exact renderer combination](pass2/final_renderer.py)
- **STRONG — collision source.** `hitscan` uses the packed 16-bit point number and separately decrements the ending wall index. `getcliplines` accesses indexed portal fields and floor sprite corners directly, tests the signed difference of wall-box products as at `1:427c6–427eb`, and uses the goal coordinate comparisons and foot-centered Z predicates in the oracle. `getzrange` loads both coordinates of each wall endpoint before the box tests and uses the oracle cross-product operands at `1:457ea–45810`. Its sprite phase uses indexed fields.
- **STRONG — map and palette source.** `draw2dscreen` uses a signed short sprite-list index and forms the absolute sprite screen coordinates before the screen tests. `setview` takes the viewport height once in the final mode-7 branch, corresponding to the single load at `1:463e4`. The tinted branch of `makepalookup` loads the destination palette before adding the color index, consistent with `1:4736e–47379`. Its code extent is now exact, with the earlier remap-loop divergence and shared return still visible.

## Negative results and open items

- **scansector.** The fresh semir run labels the distant shared return SOURCE, while the verifier anchors the seven local differences at `1:343e0–34400`, with verified fixups. One refreshed trace says the endpoint index's EDX home overlaps the value wanted in EDI. The following point2 load also wants AX, with t92/t88 live at the relevant decision. Staging the endpoint or point2 in 12 explicit forms failed to improve the retained candidate. [Allocator explanation](pass2/scansector-explain.json) · [Probes](pass2/scansector-trials.json)
- **drawmaskwall.** Semir labels it ALLOCATION-ONLY. One refreshed trace anchors the divergence before the multiply at `1:3c408`. The candidate copies EBP to EDX, multiplies in EDX, and spills the numerator on a different schedule. The trace did not uniquely attribute the first diverging allocator decision. Twenty-four numerator and negation probes yielded no accepted improvement. [Allocator explanation](pass2/drawmaskwall-explain.json) · [Probes](pass2/drawmaskwall-trials.json)
- **Shared returns after the collision block.** The remaining one-byte size error in `getcliplines` causes the single-byte differences in `keepaway` and `rotatepoint`. A shorter `clipmove` candidate cancelled that byte and matched both callers, at the cost of the verified 813-byte `clipmove` body. The candidate keeps the exact `clipmove` extent. `drawline16` is also two bytes long, leaving `draw2dgrid` off by three. The two C expressions tried for the oracle's dword-plus-byte planar fill did not improve `drawline16`. [Final collision probes](pass2/getcliplines-oracle-trials.json)
- **Other remaining differences.** The trace for `clipmove` found no unique allocator anchor, but its seven literal register differences supported the narrow declaration-order trial. Broad equal-priority allocation searches were skipped using the previous [trace evidence](traces/) and the repository's Allocation ties finding. The bounded semir run timed out for `permanentwritesprite`. The DOS byte and fixup check verifies that function independently. [Fresh semir evidence](pass2/semir/)

The highest layout leverage is the one-byte excess in `getcliplines`, followed by the planar fill in `drawline16`. The missing `printext16` extent remains relevant to the late shared returns. The previous backport trial and its regressions are documented in [the first worker report](NOTES.md).

