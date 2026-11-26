# LameDuke GAMEDEF reconstruction

The worker candidate builds with Watcom 9.5b `-4r -s -otexan`. The pinned oracle is `assets/D3D.EXE`, SHA-256 `698979d97f0f2a16ffdea20ff0059fe49ef9720a7c8933a61601356ca0462701`.

The verifier reports **7 STRICT functions, 800 bytes**, **2 CODE_BYTES_MATCH functions, 912 bytes**, and **5 NEAR_MATCH functions, 20,652 oracle bytes**. The five NEAR_MATCH candidate extents total 18,168 bytes. Runtime behavior is untested. The candidate covers the 14 mapped public entries in order, with 19,880 bytes of `_TEXT` against the 22,364-byte oracle contribution. The publics imply multiple object bases, so contiguous placement remains open.

## Boundaries

**PROVEN.** The first function starts at `1:2e700` and its 64-byte extent verifies through the start of `ispecial`. Projectile and CON movement calls corroborate the three-short angle-step signature. The preceding routine returns at `1:2e6f9` and six zero bytes precede the angle step. `execute` ends in a return at `1:33e5b`. Engine initialization independently installs the IRQ1 handler at `1:33e5c`.

**STRONG.** The seven-TU survey, grouped CON cross-references, adjacent keyword and diagnostic pool, and continuous run of aligned game functions support `1:2e700–33e5c` as a single GAMEDEF contribution. The two additional publications are `passone` at `1:301d0`, a loop also seen in the loader, and `move_actor` at `1:304a0`, whose bit tests and sprite updates recur inside `execute`. Their names describe their observed roles. The original linker map and some original spellings remain open.

Watcom places switch tables before the dispatchers. The verifier assigns those bytes to the immediately preceding public in its extent accounting.

| Table | Oracle range | Entries |
| --- | --- | --- |
| `parsecommand` | `1:2ec6c–2ed60` | 61, including token `-1` and opcodes 0–59 |
| `parse` | `1:307a8–308a4` | 58 opcode targets followed by five inventory targets |
| `execute` | `1:3215c–32258` | 58 opcode targets followed by five inventory targets |

## Function results

All extents are half-open and include the compiler padding and trailing tables described above. The byte counts in this table are oracle then candidate.

| Function | Obj1 extent | Bytes | Verifier status and first difference |
| --- | --- | ---: | --- |
| `getincangle_shift` | `2e700–2e740` | 64 / 64 | STRICT. Signed short return and historical Build `klabs` call. |
| `ispecial` | `2e740–2e770` | 48 / 48 | STRICT. LF increments the counter. LF, space and CR terminate the lexical span. |
| `isaltok` | `2e770–2e7b0` | 64 / 64 | STRICT. Watcom character classification and seven punctuation characters. |
| `getlabel` | `2e7b0–2e850` | 160 / 160 | STRICT. Copies into the current 64-byte label slot. |
| `keyword` | `2e850–2e920` | 208 / 208 | STRICT. Lookahead uses a private cursor. |
| `transword` | `2e920–2eae0` | 448 / 448 | CODE_BYTES_MATCH. Diagnostic at `+17a` refers to the partly covered CONST segment. |
| `transnum` | `2eae0–2ed60` | 640 / 640 | NEAR_MATCH. Zero differing instruction bytes. First failed reference at `+18c` is the first compiler switch target. Its own 396-byte body also has a CONST reference at `+12b`. |
| `parsecommand` | `2ed60–301d0` | 5,232 / 3,536 | NEAR_MATCH, `+a`. Entry uses EAX/EDX in the opposite order for the warning/error sum. Six label reads and eleven keyword lookaheads remain calls in this build. |
| `passone` | `301d0–301e0` | 16 / 16 | STRICT. Loops until `parsecommand` reports a terminator. |
| `loadefs` | `301e0–303b0` | 464 / 464 | CODE_BYTES_MATCH. Eight CONST references and the file-end pointer at `+a3` remain unverified. |
| `furthestangle` | `303b0–304a0` | 240 / 240 | STRICT. Equal sector and sprite output addresses use one dummy short. |
| `move_actor` | `304a0–308b0` | 1,040 / 1,024 | NEAR_MATCH, `+6`. Four-byte candidate frame versus eight bytes in the oracle. The movement flags stay in a register before the early return. The following interpreter table also differs. |
| `parse` | `308b0–32260` | 6,576 / 6,144 | NEAR_MATCH, `+8`. Candidate keeps the sprite argument in EDI where the oracle spills it. Branch and call placement, local allocation, and the following execute table differ. |
| `execute` | `32260–33e5c` | 7,164 / 6,824 | NEAR_MATCH, `+6`. Candidate frame differs during actor setup. The dispatch and movement blocks are emitted inside the function. Case layout and register choices remain open. |

The source is BINARY_DERIVED, cross-checked against the supplied CON files and the semantic analogues in Duke 1.5 `GAMEDEF.C`. The original Build helper `klabs` is from the June 1994 `BUIL0615/PRAGMAS.H` and recurs in March 1995 `buil1995/PRAGMAS.H`. The exact code and repeated math idiom support that use. This helper is ORIGINAL_ASM in otherwise C routines.

**COMPILER_STEERED.** `TOKEN_CHAR` expands the token predicate where the oracle does so under the requested profile. `transword` uses a second `labelcode[i]` expression and the local order required for identical allocation. `furthestangle` keeps the proven short-local order. `loadefs` calls the historical CLIB3R `__STOSD` public through its register ABI and retains the assignment order needed to match. The shared dispatcher and movement includes reproduce the observed in-function copies, including both switch tables. Other bodies are written as natural C. Several runtime argument and player-field names are descriptive and remain provisional.

## Data and bindings

- **PROVEN.** `_DATA` verifies at `2:02b10–02c00`, all 240 bytes and all 60 keyword pointer relocations. The order comes from those LE pointer sites. For example, `weapon` is opcode 24 and `addinventory` is opcode 48. The later Duke table supplies different opcodes and additional vocabulary.
- **PROVEN.** The 1,598-byte candidate CONST segment begins at `2:00b44`. It has no data fixups. Every one of its 1,587 emitted bytes equals the oracle, including emitted alignment bytes. The uncovered ranges are `2:00bee–00bf0`, `00ca1–00ca4`, `00d51–00d54`, and `010bd–010c0`. The image has zeros in these holes and in the two bytes at `2:01182–01184`. The verifier requires emitted coverage for the complete segment, so it reports MISMATCH and leaves CONST references unverified.
- **PROVEN.** `loadefs` clears 4,096 actor-script pointers at `2:0ae78`. The compiler assigns tile entries, and both `spawn` and `execute` read them. `script[0]` at `2:176c8` records the end pointer. Bytecode begins at `2:176cc`.
- **STRONG.** Music stores four groups of ten 13-byte filenames from `2:0ac70` to `2:0ae78`. Volume zero has its own ten-slot table at `2:a5308`. The parser and stage/environment music callers support these addresses. Asset `MUSIC.CON` uses volume zero and volumes one through four.
- **STRONG.** The shared player and hit records have strides of 176 and 20 bytes. The v5 sprite and sector records have strides of 38 and 37. The animation clock is the sprite `lotag`. Movement flags occupy `hitag`, as seen from the actor opcode and both movement blocks.
- **OPEN.** `last_used_text` has one reference at `2:b1df0` and remains outside the overlay. The exact script capacity, natural BSS layout and historical ownership of some externs still require evidence.

The overlay records the engine, game and runtime anchors used by the verifier. The final `bindprop.py --min-funcs 2` run produced three function and ten data proposals with no conflicts, and every overlapping proposal agreed with the overlay. The OMF alias for `__STOSD` resolves through the repository runtime census.

## Opcode table

Operands below describe the compiler. Label operands can resolve to numbers or addresses as dictated by the command. `if` instructions reserve an absolute false-branch pointer after their operands. The compiler patches an `else` target after parsing the controlled command.

| ID | Keyword | Inputs and handling |
| ---: | --- | --- |
| 0 | `end` | Stops compilation with the signed minimum dword sentinel |
| 1 | `actor` | Tile, optional strength/action/move, optional OR-ed movement flags; emits a four-dword actor header |
| 2 | `addbatteryammo` | Amount |
| 3 | `ifrnd` | Chance and command |
| 4 | `enda` | Ends the actor declaration and checks brace depth |
| 5 | `ifcansee` | Command; facing and cached line-of-sight test |
| 6 | `ifhitweapon` | Command; stores hit sprite for the weapon test |
| 7 | `action` | Label and up to five values at top level, one action operand inside actor/state |
| 8 | `ifpdistl` | Distance and command |
| 9 | `ifpdistg` | Distance and command |
| 10 | `else` | Command and a backpatched skip address |
| 11 | `strength` | New strength |
| 12 | `break` | Terminates a runtime dispatch |
| 13 | `shoot` | Tile |
| 14 | `addscore` | Amount |
| 15 | `sound` | VOC filename mapped to the 39-entry sound table |
| 16 | `fall` | Applies floor-clamped gravity |
| 17 | `state` | Label definition at top level, address operand inside actor/state |
| 18 | `ends` | Ends state scope or the nested runtime dispatch |
| 19 | `define` | Label and value. A duplicate generates a warning. |
| 20 | `//` | Skips through the line comment |
| 21 | `nuke` | Sprite-centered effect |
| 22 | `killit` | Sets actor removal flag |
| 23 | `addweapon` | Weapon operand |
| 24 | `weapon` | Numeric operand consumed by the runtime |
| 25 | `addhealth` | Amount, subject to the observed armor and health checks |
| 26 | `ifdead` | Command |
| 27 | `ifsquished` | Command |
| 28 | `cactor` | Replacement tile |
| 29 | `{` | Starts a recursive command group |
| 30 | `}` | Terminates a command group |
| 31 | `spawn` | Tile |
| 32 | `move` | Label and up to two values at top level, move operand and OR-ed flags inside actor/state |
| 33 | `ifwasweapon` | Tile and command |
| 34 | `ifaction` | Action operand and command |
| 35 | `ifactioncount` | Count and command |
| 36 | `resetactioncount` | Clears action count |
| 37 | `debris` | Count |
| 38 | `ifskill` | Threshold and command, compares player skill below operand |
| 39 | `/*` | Skips block comment |
| 40 | `cstat` | Sprite flags |
| 41 | `ifmove` | Movement operand and command |
| 42 | `resetplayer` | Calls the player initializer |
| 43 | `ifonwater` | Command, with sector tag and sprite height test |
| 44 | `ifinwater` | Command, with sector tag test |
| 45 | `ifplayersweapon` | Weapon operand and command |
| 46 | `ifmovecount` | Count and command |
| 47 | `resetmovecount` | Clears movement count |
| 48 | `addinventory` | Inventory selector 0–4 |
| 49 | `addammo` | Amount |
| 50 | `hitradius` | Radius and damage |
| 51 | `ifp` | OR-ed player-state flags and command |
| 52 | `addrammo` | Amount |
| 53 | `sink` | Sprite z offset and reduced horizontal velocity |
| 54 | `music` | Volume and up to ten filenames |
| 55 | `include` | Filename. Recurses while preserving the outer source cursor, line and else state. |
| 56 | `ifstrength` | Threshold and command |
| 57 | `blood` | Count |
| 58 | `guts` | Calls the sprite effect routine |
| 59 | `ifspawnedby` | Owner operand and command |

## Compiler findings and negatives

- **PROVEN.** `wc95b -4r -s -otexan` with a long return gave the angle step an 80-byte extent using Build `klabs`. A C conditional absolute value gave 96 bytes and a different save set. The short return and historical inline helper give 64 bytes, including the following alignment.
- **PROVEN.** The ordinary `memset` spelling used a different fill entry in the loader. Calling the CLIB3R dword entry and assigning `line_number` before `total_lines` removed every code and fixup-site difference. This establishes a reproducing ABI declaration. The original spelling of the fill expression remains open.
- **PROVEN.** `furthestangle` with three different hit shorts reserved 36 frame bytes and differed at `+7`. The oracle references the same stack slot for two of those outputs. Sharing the dummy gives the observed 32-byte frame.
- **NEGATIVE.** Natural `isaltok` calls in the three lexer loops remain out of line with plain `-otexan`. With `-oe=100` they inline and `keyword` stays STRICT, but `transword` differs at `+47`. The default build uses the recovered token predicate explicitly.
- **NEGATIVE.** With the unexpanded parser sources, `-oe=100` inlines all six parser label reads and some keyword lookaheads, producing a 5,296-byte compiler dispatcher with unresolved case layout. `-oe=300` brings movement into execute. `-oe=2000` also brings in the runtime switch but produced a 6,152-byte execute versus 7,164 in the oracle. Inlining ceilings of 5,000 and 10,000 had the same result on that probe. These flags were diagnostic and the historical inline ceiling remains open.
- **NEGATIVE.** Source pragmas `inline_depth`, `inline_recursion`, `on(inline)`, `optimize("e", on)`, and `option("-oe=2000")` each left the tested object sizes, call counts, and first differences unchanged under `-otexan`. The shared source includes give 6,824 bytes of execute under the requested profile.

## Reproduce

From the repository root

```powershell
$w = 'build/workers/gamedef'
python -B "$w/run.py" match --profile game --symbols "$w/overlay.json" "$w/src/gamedef.c" "-i=$w/src" --json "$w/results.json"
python -B tools/bindprop.py "$w/results.json"
```

The wrapper invokes the repository's `match.py` and `wcc.py` with worker-local object and compiler scratch. The source goes before `-i=` because `match.py` parses the first positional argument as the source. The match exit status is 1 for the reported partial reconstruction.
