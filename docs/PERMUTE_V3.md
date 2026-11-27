# Permute v3 worker report

The drop-in proposal lives in [tools_proposed](tools_proposed/README.md). Search uses Watcom 9.5 through the original wrapper and checks every public with the stock verifier. Coverage and candidate parsing use pycparser 3.00, already installed. No binding overlays or package installs were needed.

## Coverage

| Input | Functions | v2 with an applicable edit | v3 with an applicable edit |
|---|---:|---:|---:|
| Registry and sources at `48c4595` | 85 | 73 | 77 |
| Live registry | 77 | 65 | 69 |
| Archived guided game sweep, hash-matched input sources | 12 | 4 | 12 |

The first two rows enumerate all enabled edit families. The third row uses the original v2 CG95 filters and v3's prioritized, complete enumeration. The archived v2 receipts show four functions with at least one variant and eight with zero. FINDINGS reports three with edits. Coverage in each v3 row requires a body-confined edit that reparses and preserves location-sensitive macro uses. Languages or compilers outside Watcom 9.5 search support were included in parser-only coverage. The eight excluded bodies contain unsupported conditional directives inside audio-library functions.

[Historical census](coverage-48c4595.json), [Live census](coverage.json), [Guided game sweep](coverage-sweep.json)

## Calibration

- Starting from `787292d^`, the automated search found a branch conversion for `setview` at NT variant 10. A fresh DOS build verified all 334 bytes and its relocations, with zero other-public regressions. Variant 5 reduced the EDX preference deficit from 2 to 0 with the chosen register still EAX and all nine byte differences unchanged. The final tool reproduced the automated result in one trial with the branch family enabled. Its unforced trace recorded EDX for the target role. The historical `span` spelling also verified at 334 bytes in a fresh DOS control. [Automated result](calibration/setview/summary.json), [Final DOS-confirmed diff](final-calibration/setview/best.diff), [Historical span control](controls/setview/receipt.json)
- Starting from `3dbf0bc^`, the first automated sweep left `moveexplosions` at 555 bytes. Splicing the historical fixed function into the pre-fix unit yielded 127 differing bytes and zero other-public regressions. The unforced DOS-controlled trace changed the block-143 `randomstash` load from BX to AX for the shared `short angle`. The historical body also changes entry load order and the leg-piece temporary. The automated sweep did not rediscover the shared-angle spelling. [Automated result](calibration/moveexplosions/summary.json), [Historical angle control](controls/moveexplosions/receipt.json)

## Live search

The initial and final sweeps capped NT variant compiles at 22 and 12 per function, respectively. These caps can stop the first neighbourhood before beam expansion. NT score improvements can represent a real change in one allocator decision with an increase in final bytes or relocation problems. The final selector keeps the DOS baseline unless the proposed target preserves or improves its verifier discrepancies and the other publics pass the collateral checks.

| Unit and function | Live baseline | Final DOS result | First differing byte | Diagnostic experiment |
|---|---:|---:|---:|---|
| `game_game operatefta` | 4 | 4 | +0x74 | One second-candidate-sort flip reached the oracle. |
| `game_game nonsharedkeys` | 14 | 14 | +0x4d0 | Two candidate-sort flips and one home flip reached the oracle. |
| `game_sector checkhitswitch` | 2 | 2 | +0x374 | One conflict-sort flip reached the oracle. |
| `engine scansector` | 7 | 7 | +0x12a | Two register flips reached the oracle. An exploratory source variant moved one choice and worsened the byte/fixup result. |
| `multi comsend` | 4 | 4 | +0x5e | The old selector did not resolve on the current source. Current EDX is present and the residual operation is ADD versus a separate base load and indexed store. |
| `game_actors moveexplosions` | 127 | 127 | +0x59c | One conflict-sort flip reached 62 differing bytes and six fixup problems in the paired NT control. |

Zero new live STRICT or CODE_BYTES_MATCH publics were found. The four oracle-reaching live diagnostic sequences are minimal among the tested ordered subsets. Diagnostic results can guide future source edits. Stock DOS builds and the verifier determine match status.

A later workspace update independently lowered the live `moveexplosions` baseline to 74 differing bytes. A focused eight-variant pass using argument staging, narrowing and branch edits retained that DOS baseline. Its paired diagnostic replay reached five differing bytes with every fixup verified. [Focused result](actors-focused/moveexplosions/summary.json), [Five-byte diagnostic plan](actors-focused/moveexplosions/decisions/moveexplosions/plan.json)

The historical six-flip checkpoint independently reproduced the three differing bytes and zero fixup problems. Its remaining mismatches are the load, compare and LEA at +0x860, +0x86c and +0x87b, using EBP for `sector.floorz` where the oracle uses ECX. Its paired unforced debugger control showed zero collateral regressions. [Live moveexplosions plan](final-live/moveexplosions/decisions/moveexplosions/plan.json), [Three-byte checkpoint](controls/floorz/receipt.json)

## Reproduction

```powershell
$w = 'build/workers/permute3'
python -B "$w/coverage.py"
python -B "$w/coverage.py" --revision 48c4595
python -B "$w/coverage.py" --sweep
python -B "$w/controls.py" --floorz
python -B "$w/run.py" another-run operatefta nonsharedkeys checkhitswitch scansector comsend moveexplosions --budget 12 --flip-trials 5
python -B tools/match.py --obj "$w/final-calibration/setview/best.obj" setview
```
