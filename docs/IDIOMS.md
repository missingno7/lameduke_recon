# Searching the STRICT code corpus

Run these commands from `D:/Prog/lameduke_recon` with the repository’s Python and Capstone environment.

```powershell
$w = 'build/idioms'
python -B "tools/idioms/idioms.py" query --asm 'mov ax, word ptr [totalclock]; sub ax, word ptr [lockclock]'
python -B "tools/idioms/idioms.py" c --pattern '^reccnt--;'
python -B "tools/idioms/idioms.py" near game_game gamenumber
python -B "tools/idioms/idioms.py" near game_actors moveexplosions --block 1:2993a
```

- `query` accepts semicolon- or newline-separated Intel instructions and `blockdiff` tokens. For example, `mov R,m2[D:totalclock]; sub R,m2[D:lockclock]` matches the same sequence as the first command. Copy symbolic tokens for relocated addresses with addends, literals, and unbound references. Each hit identifies an exact normalized statement sequence or a contiguous subsequence inside a line-attributed span.
- `c` takes a Python regex and returns each indexed C span that matches, including spans with no attributed machine instructions. A multiline construct is searchable as one span. The index uses physical source text and keeps lexical alternatives under conditional preprocessing. An empty sequence can mean folding, an unselected preprocessor branch, or lack of a line anchor.
- `near` uses `blockdiff.pair_blocks` to identify unmatched oracle blocks, then ranks STRICT C blocks by sequence similarity. Equal scores favor shared symbolic references, then the same profile and unit. `--block` accepts `1:offset`, a hex offset, or `+offset` relative to the verifier-owned extent. `--limit` controls the number of hits per block. `--obj` accepts a worker candidate to compare. All three commands accept `--json`.

The tokens are those from `blockdiff.py`. General-purpose registers, frame displacements, local branch destinations and paired branch polarities are abstracted. Memory operand widths, immediates and symbolic references are retained. Registers and branch polarity are available in the symbolic rendering. An exact normalized hit is a lead for reconstruction. A fresh build and the repository’s strict verifier are still required to accept a source change.

## Build and verification

```powershell
$w = 'build/idioms'
python -B "tools/idioms/neutrality.py"
python -B "tools/idioms/asm_listing.py"
python -B "tools/idioms/build_index.py"
python -B "tools/idioms/probe_quirks.py"
python -B "tools/idioms/line_controls.py"
python -B "tools/idioms/catalogue.py"
python -B "tools/idioms/validate_idioms.py"
```

`neutrality.py` made 70 fresh, serial DOS builds for the 35 C units. It used the registered includes, flag overrides and installs through `tools/wcc.py`. The Watcom 10.0 audio profile already includes `-d1` for most units. Its comparisons removed that switch from the control, with other flags in their original order. GUS retains its registered `-d2` in both experiments. All 35 pairs have identical segment definitions, bytes, coverage and ordered decoded fixups, including location type, frame, target and addend. Publics, externs, groups and COMDEFs also agree. Object hashes and OMF record counts are in [neutrality.json](../neutrality.json).

A.ASM uses MASM 5.10. The pinned listing build (`a.asm,,a.lst;`) emitted the same complete object as the canonical `a.asm;` build. Listing offsets and source text establish the original assembly line. Initializer bytes in A.ASM and seven audio CODE-segment data publics are recorded separately from instructions.

The builder ran the unmodified `match.verify_object` against the line-enabled object for every C unit and against the canonical MASM object. The result exactly matches the registered STRICT set. All 150,963 verified extent bytes are accounted for by instructions, switch-table/padding ranges, or declared CODE data. [idioms.json.gz](../idioms.json.gz) contains 581 ledger entries, including 554 C procedures, 20 assembly procedures and 7 CODE data publics, with 42,873 disassembled instructions, 6,942 blocks and 17,465 source spans. Statement spans link back to instruction indexes and each instruction links to its candidate spans.

Watcom emits line and segment offsets through OMF LINNUM records. There are 42,761 instruction locations from LINNUM or the MASM listing. For 112 instructions with no in-extent line anchor, the index records the function-definition line and labels it `function_only`; these entries have no statement assignment. This affects four nonempty audio procedure regions plus empty stubs and their padding. `-d2` altered the payload in the fallback controls. Neutral alternate debug configurations did not supply the missing line anchors. [line_controls.json](../line_controls.json)

Line records have no column. The index retains 4,731 instructions with multiple candidate spans on the same line and 22 instructions anchored to a source line outside the lexically recognized function body. The C lexer preserves both candidates on a shared line. LINNUM also attributes scheduled instructions to the nearest preceding anchor and aggregates many inline instructions at the call site. For example, `timerhandler` has six separate locations for `keytimerstuff();`, including the differently allocated copy at `1:008ff`. Treat those locations as compiler associations when tracing individual inner statements.

The generated index, listings, probe objects and validation captures are worker artifacts. Regenerate them against the current source and oracle when integrating the Python tools. The catalogue’s examples point to the actual oracle coordinates and contain no raw object bytes.

## Results on three non-STRICT game functions

For each run, the reported missing block addresses matched `blockdiff.py --worklist` on the same fresh candidate object. All three functions were selected from `build/blockdiff.json`. Complete CLI output is in [validation](../validation).

| Function | Worklist blocks | Sample oracle block | Best STRICT C hit | Score | What it established |
|---|---:|---|---|---:|---|
| `gamenumber` | 6 | `1:029ff` | `gametext` `1:02810` | 0.9333 | Retrieved `overwritesprite(x, y + descender, ac, s, 0, 0)` followed by glyph width lookup and x advance. This is a specific inline-copy lead. Across the six blocks the search also recovered the newline and character-offset paths. |
| `moveexplosions` | 7 | `1:2993a` | `moveplayers` `1:2a66d` | 1.0000 | Same sign-extended sector index, 37-byte sector stride, three relocated field references, and `JGE` for `sprite[i].z < sector[sprite[i].sectnum].floorz`. Four other blocks gave `MOVE_ALONG` or `EGS` leads. A random-offset block had only weak generic matches. |
| `draw_and_probe` | 3 | `1:2c8b1` | `draw_pause_sign` `1:2e08b` | 0.5000 | Recovered the signed `tilesizy[...] / 2L` and `rotatesprite` argument shape, including the fixed-point x origin and zoom offset. The other two blocks yielded generic field-load or structure-copy analogues and did not identify their exact C. |

The Intel assembly query returned the single matched `getinput` cast at `1:20e6e`. The C regex returned both `reccnt--;` occurrences and their different generated shapes. The validator checks the ledger, every indexed byte, statement backlinks, catalogue addresses, all three worklist address lists, and single-block filtering.

## Controlled comparisons

[WATCOM95_QUIRKS.md](WATCOM95_QUIRKS.md) and [quirks.json](quirks.json) contain the catalogue. [probes.json](../probes.json) captures fixtures under all four code-bearing default profiles. The switch matrix varies case count and stride with distinct external callees so the compiler must preserve distinct destinations. Its observed choices apply to that source shape. The matrix gives no universal minimum density or case count for jump tables. Separate tests varied operand width, volatile reads and writes, constant expressions, return and branch forms, literal reuse, and prototype shape. The tests also observed right-to-left evaluation for six effectful integer arguments in the tested register convention.
