# Permute

`permute.py` searches for C spellings that reproduce Watcom 9.5 GA code generation. It compiles the full translation unit with `tools/wcc.py` and scores independently bound functions with the repository's `match.py` and `blockdiff.py`. Copy the three Python files together to `tools/permute/` to install them. The only runtime Python dependency beyond the standard library is the repository's existing Capstone dependency.

Run from the repository root on Windows. Supply a new output directory under `$PERMUTE_WORK` (default `build/permute/`; workers set it to their own directory) for each search.

```powershell
$p = 'tools/permute/permute.py'
$w = 'build/permute'
python -B $p game_sector checkhitswitch --transforms decl,loops,commute --budget 60 --out "$w/switch-trial"
python -B $p engine hline --source "$w/historical/engine/ENGINE.C" --transforms decl --out "$w/hline-trial"
python -B $p "$w/historical/game/game.c" movetransports --profile game_oe --transforms decl --out "$w/transport-trial"
python -B tools/match.py --obj "$w/hline-trial/best.obj" hline
```

Pass more function names after the input to search several targets together. For a standalone source, `--profile` is required and `-i DIR` is repeatable. `--symbols` accepts the verifier's usual binding overlay. For a current NEAR function, run `semir.py` first to check the source and allocation classification.

`--focus name1,name2` keeps edits involving those identifiers. `--cg95-explain PATH` takes an existing `explain95.py --out` JSON and uses named values from its first attributed allocation decision. Reports that contain only anonymous compiler temporaries cannot supply a source-level focus.

## Search and verification

The input is built once on the canonical DOS host and once on NT. The search rejects any DOS candidate that loses a STRICT non-target from the input. NT trials can also reject a regression when NT reproduced the corresponding DOS baseline match. The remaining score orders candidates by target STRICT count, positional byte differences from the verifier, block content, block register agreement, and finally unresolved verifier findings. Byte counts alone cannot certify a match.

`auto` chooses bounded breadth-first enumeration for a small initial neighbourhood and a beam for a large one. Use `--strategy exhaustive`, `greedy`, or `beam` to choose explicitly. `--depth` limits chained edits and `--budget` counts NT variant compilations, including duplicates and failures. `--beam-width` controls how many promising source variants lead to another round. Small declaration groups get full permutations. Larger groups get declaration-line permutations where feasible and pair swaps, including swaps across lists. The final shortlist is rebuilt on DOS. Its best improvement over the DOS input is selected. An all-STRICT candidate gets an immediate DOS check and stops the search unless `--keep-going` is set.

| Artifact | Contents |
|---|---|
| `best.diff` | Unified diff against the input, with original line endings in the hunk content. Empty when the input wins. |
| `best/<source>`, `best.obj` | Selected source and its freshly built DOS object. |
| `run.json`, `summary.json` | Input hash, compiler configuration, search options, selected score, baseline and timings. |
| `trajectory.json` | Baseline and improvements on each host. |
| `variants.jsonl` | Attempt ID, source hash and parent edit, compile time, object hash and result. An exact duplicate object points to its first record for that host. Offsets apply to the parent source decoded as Latin-1. |

## Supported C forms

| Transform | Conditions for an edit |
|---|---|
| `decl` | Permute, split and merge uninitialised automatic scalar declarations. A comment between declarations ends a permutation group. |
| `scope` | Move one uninitialised scalar between nested blocks when every reference is accounted for. Jumps, labels and changes across loop lifetimes block the move. |
| `types` | Change width or signedness when the interval of every assigned value fits both types. Preserve the old value type at consumers when integer promotions would change. |
| `temp` | Stage an integer subexpression in a local using its original type. It must execute unconditionally in a modelled assignment, expression statement or return. Inline a single-use local when its definition is the preceding statement. |
| `commute` | Swap operands of pure integer operations, including reversing the predicate for an ordered comparison. Preserve grouping. |
| `statements` | Swap neighbouring expression statements with disjoint local-variable sets and no unmodelled effects. |
| `loops` | Exchange top-tested `while` and `for` headers. Moving an initializer or update requires additional control-flow and scope checks. |
| `chain` | Split a discarded chain of assignments to distinct scalar locals, preserving each intermediate conversion. |
| `updates` | Exchange prefix, postfix and compound spellings for discarded unit increments and decrements. |

The parser keeps source spans and edits the original text. It refuses in-body directives, inline assembly and ambiguous function definitions. The expression model covers local integers and integer constants, with conservative bounds. Calls, accesses through pointers, aggregate accesses, volatile objects, unmodelled declarations, macros and address-taking limit the edits it can prove. For declaration order and simple loop-header swaps, unrelated opaque expressions may stay in the function. The tool guards lines observed by location-sensitive macros. This search can leave an allocation tie unresolved even when a manually proven rewrite exists.
