# Permute

`permute.py` searches for C spellings that reproduce Watcom 9.5 GA code generation. It compiles the full translation unit with `tools/wcc.py` and scores independently bound functions with the repository's `match.py` and `blockdiff.py`. Install the Python files together under `tools/permute/`. The runtime dependencies are the repository's Capstone dependency and, for tracing, 64-bit Python with WOW64.

Run from the repository root on Windows. Supply a new output directory below `PERMUTE_WORK` for each search. The default is the worker directory for this proposal and `build/permute/` when installed.

```powershell
$w = 'build/workers/permute2'
$p = "$w/tools_proposed/permute.py"
python -B $p game_actors hitradius --cg95 --budget 150 --out "$w/hitradius-trial"
python -B $p game_sector checkhitswitch --cg95 --focus i,nextspritestat,w --out "$w/switch-trial"
python -B tools/match.py --obj "$w/hitradius-trial/best.obj" hitradius
```

Pass more function names after the input to search several targets together. For a standalone source, `--profile` is required and `-i DIR` is repeatable. `--symbols` accepts the verifier's usual binding overlay. For a current NEAR function, run `semir.py` first to check the source and allocation classification.

`--cg95` runs the repository tracer and explainer for each target using the search's input, include order, flags, and binding overlay. It checks the traced code against ordinary NT and DOS builds. The search takes the first attributed decision, its reported competitors, and source names resolved from their defining operands. Edits must involve at least one focused name in that function. The trace and explanation are saved alongside the search, and `run.json` identifies the decision, the first aligned machine difference, and the source of each focus.

`--focus name1,name2` sets a manual focus when no trace is requested and supplies a fallback when cg95 cannot map a decision to source. Guided targets with neither a mapped nor an explicit focus generate zero variants. `--cg95-explain PATH` uses the named evidence in an existing explanation without running a fresh trace.

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
| `scope` | Move an uninitialised scalar to the innermost block containing all its references or lift it into an enclosing block. Try positions among the destination's independent declarations. Crossing a loop requires a direct first assignment on each entry. |
| `types` | Change width or signedness when the interval of every assigned value fits both types. Preserve the old value type at consumers when integer promotions would change. |
| `ranges` | Split a live range at a direct assignment that dominates the new uses, or merge disjoint same-type live ranges in the same block. |
| `address` | Add or remove immediate `*(&x)` for a scalar lvalue, or capture a call argument's existing address in a function-scope pointer local. The callee receives the original address. |
| `temp` | Stage an unconditionally evaluated pure expression, including repeated subexpressions, in a block or function temporary. Inline an adjacent definition where the proof accounts for every use. |
| `narrow` | Stage an existing cast or assignment conversion, or use a smaller slot if interval bounds preserve the value. A dedicated direct-store rule stages a single volatile index read while retaining the final store. |
| `save` | Copy an incoming scalar parameter into a local or preserve its original value in a separate local before the first narrowing store. |
| `commute` | Swap operands of pure integer operations, including reversing the predicate for an ordered comparison. Preserve grouping. |
| `statements` | Swap or rotate independent direct stores, including fields of a known struct. Check destination and read dependencies against every crossed store. |
| `loops` | Exchange top-tested `while` and `for` headers. Moving an initializer or update requires additional control-flow and scope checks. |
| `chain` | Split discarded assignments to distinct scalar locals. Commit a same-type global update in a local initializer if intervening initializers cannot observe it. |
| `updates` | Exchange prefix, postfix and compound spellings for discarded unit increments and decrements. |

The parser keeps source spans and edits the original text. It refuses in-body directives, inline assembly, and ambiguous function definitions. Its proof rules cover integer locals and parameters, constants, and direct objects and named struct fields with recognized declarations. Opaque calls, pointer aliases, volatile access, macro expansion, and control flow can prevent an edit. Substituting a short's address for a callee's long output is outside the model. The tool guards lines observed by location-sensitive macros. Only the repository verifier can certify a DOS result as STRICT.
