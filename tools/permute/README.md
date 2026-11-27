# Permute v3

Permute searches equivalent C spellings with the pinned Watcom 9.5 compiler, stock CG95 observer and stock LameDuke verifier. Install all Python files in this directory together under `tools/permute/`. Runtime requirements are Python 3.11+, pycparser and the repository's Capstone dependency. The Watcom NT observer uses Windows WOW64.

Run from the repository root. Output goes to a fresh subdirectory of `PERMUTE_WORK`. The worker proposal defaults to its parent directory. The installed tool defaults to `build/permute/`.

```powershell
$w = 'build/workers/permute3'
$p = "$w/tools_proposed/permute.py"
python -B $p game_game operatefta --cg95 --out "$w/operatefta-example" --flip-focus "$w/focus.json"
python -B tools/match.py --obj "$w/operatefta-example/best.obj" operatefta
```

The original CLI accepts multiple targets, a standalone C path plus `--profile`, and `--source` to use a historical copy with a registered unit's flags and include directories. `-i` and `--symbols` accept the usual include directories and verifier binding overlay. Run `semir.py` first for a current NEAR target when deciding whether the residue is an allocation problem.

## Parsing and source edits

Pycparser reads a synthetic function containing the raw target body. Its context is assembled from typedef names found in the reachable source and header files. A position-preserving mask covers comments, supported Watcom modifiers, argument lists containing type-valued macro syntax, statement-producing macros and supported directives. The parser validates each resulting body. The original token spans and v2 scalar and effect checks constrain the edits. The tool uses the original source text as output and applies replacements inside the target braces. It checks location-sensitive macros against the input before compiling. Conditional preprocessing inside the body, inline assembly, ambiguous declarations and unknown side effects can limit or prevent edits.

The transformation catalogue retains declaration reordering, scoping, local width changes, live-range splits and merges, immediate address identities, integer commutation, independent stores and alternative loop spellings. It adds body-local parameter staging, integer argument staging, existing narrow store conversions through a shared short or byte, separately staged short loads and masks, conversion of a ternary store into branches, branch reversal, and movement of a proven invariant local copy across a `for` loop boundary. Only recognized source situations qualify. Assignments, aliasing, macro scope, unsupported lvalues and control-flow joins can exclude a potential mutation.

## Objective and search

With `--cg95`, the tool records one DOS-controlled baseline trace and explanation per target. The stock flip/climb tools test applicable diagnostic choices. `--flip-focus PATH` supplies optional hints keyed by public name. Supported fields are `replay` for known selector sequences, `names`, `blocks`, `regset`, `homes`, `priority`, `bridges`, and `source_names` to prioritize edits. The runner tests known ordered subsets from shortest to longest, then uses bounded climb if the best replay still has differences. `--flip-trials` bounds each climb round and accepts zero for replay only. A result is minimal only within the tested paths or subsets recorded in the diagnostic receipt.

Each eligible variant uses an ordinary NT compile and an unforced observer compile when a flip plan supplies decisions. Their code, publics and fixups must agree under the stock CG95 projection. Score components, in order, are whole-unit collateral, number of unsolved targets, unresolved decision associations, decisions still off target, comparator tier and negative margin, rank distance, aligned instruction gaps, positional register and stack-home differences, byte differences and verifier reference issues. Sort scoring uses the observed key, including flag 0x100, conflict presence, unsigned savings and signed temp IDs in the candidate sort. Changes in optimized IR uses at a loop depth can affect savings by a factor of 20. Register scoring uses the desired register's eligible score minus the natural incumbent's score, with the actual committed choice and eligibility tracked independently. The saved receipt includes both signed margins and the fields used to associate temps across compiles. When no diagnostic choice is found, aligned operand and byte scoring remain available.

The beam expands the deterministic single-edit neighbourhood of each retained parent, scores candidates and keeps up to `--beam-width` for the next depth. `--budget` caps variant compiles, so a large initial neighbourhood can consume the budget before a second layer. `--strategy auto` selects exhaustive search for small neighbourhoods and beam search for bigger ones. `--strategy greedy` uses a one-parent beam. `--focus` explicitly restricts applicable edits, and inferred source names order edits when `--cg95` is enabled. `--cg95-explain` accepts a saved explanation for name filtering without running a diagnostic climb.

The ordinary DOS baseline protects every other public's verifier status, byte difference count, fixup problems and unresolved references. The shortlist is compiled on DOS and compared with that baseline using the stock verifier. The selected DOS candidate also preserves or reduces the target's verifier discrepancies. A STRICT result triggers immediate DOS confirmation unless the search is configured to continue. Diagnostic objects are experiment receipts. The selected ordinary DOS result is the verification artifact.

## Outputs

| File | Contents |
|---|---|
| `best.diff`, `best/<source>`, `best.obj` | Selected DOS result and its source |
| `best-nt.diff` | Best NT search spelling when it improved the search score |
| `run.json`, `summary.json` | Input and options, baseline and selected verdicts, collateral, and timing |
| `trajectory.json`, `variants.jsonl` | Improvements and variant provenance |
| `decisions/<function>/` | DOS trace control, explanation, diagnostic trials and scoring plan |

The parser mask and body splice follow Rayman's permuter approach using original LameDuke token spans. The aligned allocation objective and deterministic neighbourhood and beam follow the approach in SimAnt's `alloc_search.py`. The v3 implementation uses local repo APIs and independently written code.
