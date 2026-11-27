# Watcom 9.5 allocator flips

The patched debugger state is diagnostic evidence. The stock verifier read the resulting OMF to measure code, extent, fixup sites and targets at the independently bound oracle address. Only a separate unmodified DOS-host compile can prove that a C spelling matches. No symbol overlays were used.

## Results

| Function | Fresh DOS baseline | Best diagnostic result | Smallest set found |
|---|---:|---:|---|
| `operatefta` | 4 bytes, first +74h | 0 bytes, 560 verified | Exchange the equal-priority homes of `t15_left.0` and `t102.0`. Sort ranks 19 and 13 exchange, giving the two inlined minitext line origins ESP+0Ch and ESP+14h in oracle order. |
| `nonsharedkeys` | 14 bytes, first +4D0h | 0 bytes, 2,080 verified | In sequence, exchange `t190_ac.0` / `t218_ac.0`, `t160_ac.0` / `t190_ac.0`, and `t159_left.0` / `t160_ac.0`. The four occupied ranks 69, 77, 82 and 86 now hold the title's line origin, level glyph, title glyph and episode glyph. Differences fell 14 to 10 to 6 to 0. |
| `checkhitswitch` | 2 bytes, first +374h | 0 bytes, 1,740 verified | Exchange conflicts `t116.0` and `t117.0` at ranks 97 and 110. Both have savings 564. The case-405 advance conversion and scale then both take ECX. |
| `scansector` | 7 bytes, first +12Ah | 0 bytes, 756 verified | In backend block 26, force the `t92.0` scale to EDX and then the `t91.0` sign-extended `p2[z]` load to EDI. Forcing the scale alone assigns EAX to `t91`. The pair also frees AX for `point2` and puts the right-screen-bound load in EDX. |
| `comsend` | 5 bytes, first +5Dh, 2 fixup problems | 0 bytes, 141 verified | In the ESC2 arm force the sign-extended volatile index `t27.0` from EAX (score 2) to EDX (score 0). The backend now emits an EAX load for the ring base and indexed store, restoring the off32 site at +5Fh. |
| `setview` | 9 bytes, first +B5h, 3 fixup problems | 3 bytes, first +DBh, 3 fixup problems | Force the `y1-startumost[0]` temporary `t37.0` from EAX (score 6) to EDX (score 4). Remaining oracle order is the store to `viewoffset` followed by the load from `xdimen`. The candidate has the opposite order, changing the sites even though the next instruction is at the same offset. |

ENGINE changed during the run from a642268 to 2454e79 in getzrange and grouvline. The two targeted diagnostics replayed with the same scores and no collateral regressions on 2454e79. The original ENGINE source snapshot and trace pair are available under `snapshots/a642268/` and `runs/<function>/snapshot.*`. The current code reproduces the nine-byte `setview` baseline. The previous permute2 worker source independently reproduces eight. It also scores the delta for EDX, and its earlier incoming-coordinate allocations differ. The seven-byte `scansector` result has all fixups verified at baseline. The `comsend` and `setview` baselines have the fixup problems listed above.

Every accepted diagnostic result was scored against all publics of its original unit using `tools/match.py`. No other public increased in byte differences, fixup problems or unresolved references. The baseline unit counts were GAME 37 STRICT / 12 NEAR (22,956 bytes), SECTOR 12 / 6 (3,954 bytes), ENGINE 85 STRICT / 1 CODE_BYTES_MATCH / 17 NEAR (45,183 bytes), and MULTI 20 / 5 (4,328 bytes). Each `runs/<function>/diagnostic-verdict.json` holds the target verifier result and the unit regression check.

The one-flip successes have minimum cardinality. The `scansector` pair is minimum within the 21 chosen regional alternatives because none of the singleton trials improved the verifier score. Each member of the pair alone failed, with 325 and 10 differing bytes respectively. For `nonsharedkeys`, all proper subsets of the accepted sequence failed. The sweep included the 18 eligible pairs of finalized homes, followed by re-traced alternatives at each accepted step. Other three-flip sequences may work. The search does not establish a global minimum across other compiler phases, non-greedy paths or alternate source.

## C source controls

| Function | Source lever and expectation |
|---|---|
| `operatefta` | Argument staging and moving the line origin's declaration/assignment alter creation order before home sorting. A separate minitext origin declaration and assignment, or a scoped origin at the call site, retained the four original bytes. Moving the local out of the minitext block worsened the allocation. |
| `nonsharedkeys` | The affected stack slots are regenerated at each gametext expansion. Scoped labels/origins and initialized declarations retained the original order. Swapping declaration order or giving the glyph an inner scope changed allocations and lost STRICT gametext/gamenumber. |
| `checkhitswitch` | Target the visit order while retaining the two equal-savings conflicts. Explicit index scaling, moving disjoint declarations and splitting predicates or the advance tested temp creation and lifetime. Nested predicates and prescaled links preserved the two original differing bytes. |
| `scansector` | Stage the narrow/wide next-scan index and the opposite bound to change interference and strength reduction. The reversed bound comparison kept seven bytes and no fixup problems. The historical March 1995 direct expression and named next-scan lifetimes regressed. |
| `comsend` | Change the index conversion's copy preference via subscript vs. integer addition, operand order, and named values across the volatile dereference. Commuting the integer sum or introducing a named index preserved the five-byte result. Preloading the base or reverting to the subscript changed the code shape. |
| `setview` | Shape the delta's copy affinity separately from scheduling the next screen-bound read. Named delta, branch and loop-bound forms regressed or preserved nine bytes. The pre-existing permute2 coordinate alias and height staging drove the equivalent delta to EDX in its eight-byte control. |

None of the 84 completed DOS source builds produced a new STRICT target. The first engine wall-pointer trial failed to compile because ENGINE declares only `struct walltype`. The corrected spelling compiled and regressed, and its receipt is under `runs/scansector/corrected/`. On the current ENGINE, staging the incoming y1 and viewport height together reproduced the prior eight-byte setview result. Either factor alone left nine or more bytes. The eight-byte candidate has the three original fixup problems and no other function worsened. Its diff is under `runs/setview/factors/incoming-y1-and-staged-height/`.

The DOS probes compiled copies of the full registered translation units through the stock wrapper and compared every public with the DOS baseline. Trial directories retain source diffs and summaries, including regressions in rejected forms. The `dos-source-verdict.json` files recheck the saved builds against the DOS baselines for the source snapshot each build used. The matched binary donor for `comsend` is BUIL0801's ENGINE.OBJ and supplies no C text for the allocator choice. The March 1995 engine source supplied the historical scansector expression. For game/font cases the integrated source was the closest code-shape starting point and the K94/Duke 1.5 hits supplied semantic cross-checks. Donor and idiom lookup preceded the source probes.

## Reproduce

From the repo root, use 64-bit Python on Windows. Run one compile at a time.

```powershell
$w = 'build/workers/tieflip'
python -B "$w/baselines.py"
python -B "$w/tools_proposed/flip95.py" checkhitswitch --list "$w/runs/checkhitswitch/baseline.json"
python -B "$w/tools_proposed/flip95.py" checkhitswitch --unit game_sector --flip 'checkhitswitch:conflict:priority-tie:t116.0~t117.0:1' --out "$w/runs/checkhitswitch/replay.json"
python -B tools/match.py --obj "$w/runs/checkhitswitch/replay.obj" checkhitswitch
python -B "$w/tools_proposed/climb95.py" scansector --baseline snapshot.json --tag bridged
python -B "$w/current_engine.py"
python -B "$w/source_probes.py" --engine-baseline current comsend scansector setview operatefta nonsharedkeys checkhitswitch
python -B "$w/followup_source.py" --engine-baseline current checkhitswitch scansector setview operatefta nonsharedkeys comsend
python -B "$w/last_source.py"
```

The pinned NT compiler runs through `wcc.py` with the stock CG95 debugger and a scratch copy of the compiler. The sorter hook exchanges comparator-equal ranks in the in-memory returned list and preserves the relative alias order within each home identity. The register hook switches the provisional colour to an eligible scored alternative before the normal cost check and commit, and records the committed choice. A selector encodes function, phase, decision kind, temp IDs and alias offsets, and occurrence. The same-source selector does not use a debugger address and may need rediscovery after C edits. The runner refuses unresolved selectors and vetoed colours. Neither hook edits a compiler executable or generated object. Unobserved late scheduling decisions are outside these hooks.

Integration candidates are the two Python tools under `tools_proposed/`. They use the repo's current CG95 hook map. The focus specification records the per-function constraints and the one `scansector` pair used to bridge its singleton plateau.
