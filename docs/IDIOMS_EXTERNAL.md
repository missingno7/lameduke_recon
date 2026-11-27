# External Watcom 9.5 C corpus

The proposed idioms tools search historical C compiled with the pinned wc95b installation. Use these results as source leads and check any reconstruction with the repository's strict verifier.

## Build inventory

[compile.json](compile.json) records each file and profile, each attempt's flags, selected source and includes, compatibility inputs, return code, object hash and diagnostic log. [index-inventory.json](index-inventory.json) records which successful objects produced searchable C. [d1-controls.json](d1-controls.json) compares six line-enabled builds with otherwise identical NT builds. CODE, CODE fixups and linkage agree in all six controls. The ROTT `RT_VIEW.C` control has three differing bytes in CONST.

| Project | Files compiled / attempted | Successful profile builds |
|---|---:|---:|
| kenbuild-20241213 | 44/45 | 73 |
| kenbuild-20260331 | 11/12 | 17 |
| buildsrc-20190520 | 5/5 | 6 |
| duke-1.3d-tree | 11/11 | 20 |
| duke-1.5 | 12/12 | 22 |
| lesbird-whaven | 14/24 | 27 |
| lesbird-tekwar | 18/18 | 36 |
| rott | 49/53 | 97 |
| idtech-swsource2005 | 74/75 | 146 |
| ny-sw-src-addendum-20190129 | 82/84 | 162 |

The index has **16,124** function variants, **576,035** emitting C spans and **456,655** searchable block occurrences. There are **80,917** distinct exact keys and **63,942** distinct loose keys. Eight objects supplied no indexed functions. Four Witchaven function variants with far-pointer relocations were omitted, and the other functions in those objects were indexed.

The NT builds use `-bt=dos -d1` after the registered profile flags. Duke 1.3D and Duke 1.5 use both game profiles, and Ken's GAME, BUILD, ENGINE and MULTI files use the appropriate game, engine or multi profile. The Duke 1.3D sources include reconstruction-era work. Dates in the index identify the applicable source snapshot, release era, or archived file timestamp, with the basis recorded per entry. Optional coverage includes the ROTT game directory, the 2005 Shadow Warrior release's main source directory, and the addendum's `source/CODE` snapshot and `blddbg/ENGINE.C`.

The builder reuses xscan's recorded source copies, borrowed headers, stubs and flags. It stages vendor `.DEF` headers already present in the archives. The worker supplied a `mem.h` shim that includes Watcom's `string.h`, a Witchaven editor shim using the archive's volatile `keystatus` declaration, and a TekWar header adaptation aligning the declared `flags32` signedness with its definition. Those inputs are recorded in the compile manifest. Remaining failures include older inline assembler, missing game headers or tile constants, incompatible prototypes, and game_oe timeouts. See the per-attempt logs linked from the manifest.

## Index and commands

The separate [external index](external.json.gz) associates Watcom LINNUM offsets with lexical C statements. It preserves multiple candidate spans for shared lines and labels unscoped lines. Inlining and scheduling can associate several statements' instructions with one line. Indexing uses 32-bit CODE segments, named local/public starts and OMF relocations. The block search requires decoded instructions with an attributable C statement. Anonymous data can be identified by literal content or left as a segment-relative symbol. Code samples retain provenance and the actual compiled source path when xscan used a compatibility copy.

`--external` adds full-block twins under the existing normalized token scheme, or contiguous sequences within a source span for `query`. `--external-loose` adds a second tier. It abstracts names and addresses of relocated globals and pointer-relative displacements while retaining memory width, addressing shape and index scale, nonrelocated numeric immediates, instruction order, branch classes and switch-table references. Direct engine and library calls keep canonical names; other direct call targets are abstracted. As in the existing index, general-purpose registers, stack homes, local jump destinations and paired branch polarities are abstracted. Relaxed hits can reflect a common instruction pattern with unrelated semantics.

```powershell
$w = 'build/workers/extcorpus'
python -B "$w/tools_proposed/build_external.py" --optional
python -B "$w/tools_proposed/build_external.py" --optional --retry
python -B "$w/tools_proposed/build_external_index.py"
python -B "$w/tools_proposed/measure_external.py" --worklist "$w/worklist-61.json"
python -B "$w/tools_proposed/validate_external.py"
```

`reverse`, `near` and `query` list the existing STRICT C results before the opt-in external results. `near` and `reverse` cap displayed external hits per tier and block with `--limit`. Both retain the existing worklist and block-pairing behavior. The proposed CLI defaults to the worker index and supports `--external-index` or `IDIOMS_EXTERNAL_INDEX` for alternate index paths.

## Coverage

[coverage.json](coverage.json) records the hashes of the oracle, census, tokenizer, STRICT index, external index and candidate line objects. The measurement uses the live `build/blockdiff.json` roster for game, ENGINE and MULTI. It checks each candidate status and consumes candidate blocks as a multiset using `blockdiff.pair_blocks`. Counts are oracle block occurrences. Bytes are the opcode-bearing bytes counted by `blockdiff` for each occurrence. The base comparison uses STRICT C blocks, matching the existing reverse search. The same loose transformation is applied to both corpora for the stricter external-only loose comparison.

| Measure | Oracle blocks | Oracle bytes |
|---|---:|---:|
| Unmatched oracle blocks in the 61-function snapshot | 947 | 59,243 |
| Existing STRICT exact twins | 102 | 1,437 |
| STRICT twins after applying the same loose transform | 126 | 1,770 |
| External exact twins, including overlap | 144 | 1,598 |
| External loose twins, including exact and overlap | 181 | 2,365 |
| **New external exact twins beyond STRICT exact** | 73 | 1,109 |
| Extra external loose twins beyond both exact searches | 32 | 648 |
| Of those, also found in STRICT at the loose tier | 15 | 133 |
| **Extra external loose twins beyond both exact searches and loose STRICT** | 17 | 515 |
| Total external-only with both corpora using the loose tier | 85 | 1,571 |

Seven new exact hits have one normalized instruction. Twenty-six of the new exact hits and eight of the 17 additional same-baseline loose hits have at least five normalized instructions.

The live roster changed during the run. It dropped `game_actors/hitradius`, which had zero unmatched oracle blocks in the captured version. The other 60 worklists and all candidate object hashes agreed. [worklist-61.json](worklist-61.json) preserves the requested roster and original keys.

## Source leads

The three changes below were built together in a scratch copy of ENGINE.C with the canonical DOS-hosted `engine` profile. All three target blocks went from absent to present under `blockdiff.pair_blocks`. The verifier still reports NEAR_MATCH for each function. [Probe results](lead-probe/result.json) · [Reproduce the probe](tools_proposed/try_leads.py)

| Target oracle block | Donor C and proposed change | DOS scratch result |
|---|---|---|
| `overwritesprite` `1:3f6c2` (38 B) | [March 1995 ENGINE.C:4530](../../../external/x/kenbuild-20260331/kenbuild-related-archives-20260331/build_1995_03_28_partial/ENGINE.C#L4530) adds `FP_OFF(palookup[dapalnum])` and the shifted signed shade in one expression. Replace the two assignments with that expression, with the palette term first. | Twin at candidate `+0x49`. Total unmatched bytes rose from 258 to 278, so the change repairs this block while shifting other coverage. |
| `hitscan` `1:41721` (22 B) | [BUIL0422 ENGINE.C:4747](../../../external/x/kenbuild-20241213/kenbuild-related-archives-20241213/BUIL0422/ENGINE.C#L4747) puts `intz` on the left of both sprite-height comparisons and keeps the paired shifts in the lower bound. Use that operand order with the existing height expression. | Twin at candidate `+0x55f`. Missing blocks fell from 7 to 5 and missing bytes from 403 to 315. |
| `draw2dscreen` `1:444c2` (39 B) | [SRC_9505 ENGINE.C:6223](../../../external/x/buildsrc-20190520/buildsrc-20190520/SRC_9505/ENGINE.C#L6223) tests the wall visibility mask with `> 0`. In the early-continue form, use the complementary `<= 0` test to recover the signed branch. | Twin at candidate `+0x61`. Missing bytes fell from 2,058 to 2,027. |

## Integration

Copy the proposed `idioms.py`, `reverse_search.py` and `external_corpus.py` into `tools/idioms`. The builder and measurement scripts can live there as well. For an integrated CLI, place the generated index at `build/idioms/external.json.gz` or set `IDIOMS_EXTERNAL_INDEX`. The builders use the extcorpus worker scratch area and read xscan's compatibility inputs by default. The existing STRICT index and verifier are independent of the external index.
