# gamefix5 — enum/#define, copy-temporary and loop-shape rules across SECTOR/PLAYER/ACTORS/GAMEDEF/MENUES/SOUNDS

Toolchain: wc95b `-4r -s -otexan` (profile `game`), stock `tools/match.py`. Exploration on LAMEDUKE_HOST=nt; the final
`*.results.json` are from the default DOS host (same code and STRICT counts as NT). GAME (`game.c`) was not touched;
`game.c` includes only `build_v5.h` and `game.h`, and neither was changed.

Changed units (copies of `src/game/<unit>.c` + all `src/game/*.h`):
- `src/sector/sector.c`, `src/sector/operate_helpers.h` (only `sector.c` includes this header)
- `src/actors/actors.c`
- `src/gamedef/gamedef.c` (NATURAL), plus an alternative `src/gamedef_alias/gamedef.c` (COMPILER_STEERED, see below)
- PLAYER, MENUES, SOUNDS: no change kept (copies under `src/` are the integrated sources).

Verify (repo root, DOS host), likewise for actors / gamedef / gamedef_alias:
```
python tools/match.py --profile game "-i=build/workers/gamefix5/src/sector" build/workers/gamefix5/src/sector/sector.c --json build/workers/gamefix5/sector.results.json
```

## Metrics
- STRICT count (verifier). No STRICT function was lost in any kept change. No unit gained a STRICT function.
- `un` is aligned unmatched oracle bytes (`aligned.py`, from gamefix3). It is chaotic in 5-12 KB functions, because one
  register swap changes many encodings.
- `sh` is new in `shape.py`. It is a register-blind shape distance: the number of oracle instructions not aligned
  after normalising registers to r8/r16/r32, dropping stack displacements and masking fixup fields. It isolates
  source-shape effects such as widths, branch polarity and block order from allocation noise. Used as a tie-breaker
  and sanity check.
- Sizes: candidate/oracle extent (oracle extent = bound address to next bound address; includes the next public's
  switch table).

| Unit | STRICT before -> after (DOS) | Σun before -> after | Σsh before -> after | notes |
|---|---|---|---|---|
| sector | 9/18 2462 B -> 9/18 2462 B | 21796 -> 21575 | 2886 -> 2843 (4 big functions; the "before" figure already includes the warp-loop header change) | sizes unchanged |
| actors | 8/18 1936 B -> 8/18 1936 B | 14571 -> 14567 | 2462 -> 2260 | sizes unchanged |
| gamedef | 9/14 1712 B -> 9/14 1712 B | 10300 -> 8218 (alias: 7918) | 2544 -> 2128 | parsecommand 3536 -> 5328 (alias 5232 = oracle) |
| player / menues / sounds | 1 / 7 / 12 unchanged | | | nothing kept |

Per function (after): gamedef parsecommand un 2721 -> 1578 (alias 1399), sh 833 -> 472 (alias 420); execute un
3737 -> 2808, sh 632 -> 583, size 6951 -> 6911 (oracle 7164); byte_diffs execute 4922 -> 4730, first diff +6 -> +10.
SECTOR playerkeys un 3533 -> 3455, checksectors 6482 -> 6358, operatesectors 7393 -> 7339 (size stays exact 10288).
ACTORS moveactors sh 1700 -> 1507, moveweapons 48 -> 45, moveeffects 330 -> 324.

## Rule 1 (enum vs #define): audit complete, nothing left to convert
- `grep enum src/game`: the only enum in these six units is GAMEDEF `enum con_opcode`. All tile, flag and state
  constants are already #defines from gamefix3 and integration. The two enums in `game.c` are out of scope.
- con_opcode -> #defines (generated, `v/gd_enum.py`): byte-identical output for all GAMEDEF functions, so it is
  NEUTRAL and was not kept. Its values are only used as case labels or compared with `long` values.
- `scan.py UNIT [--imm]` compares candidate and oracle histograms of cmp/test operand width, immediates, movsx and
  jcc mnemonics per function. No function shows the enum signature: no remaining `cmp r32,imm` in the candidate where
  the oracle has `cmp r16,imm` for the same constant. The width differences that remain come from loop-variable types
  (SECTOR, below), switch-tree shape (moveactors' inlined ifsquished / masterswitch trees use `jb/jbe` in the
  oracle) and branch polarity.
- The scan found two natural fixes, both kept (ACTORS):
  - `HURT`: `ps[p].armor > 0` instead of `ps[p].armor`. The oracle tests the unsigned byte with `jbe`. Five copies,
    three in moveactors and two in moveweapons, now match that test. sh: moveactors 1700 -> 1507, moveweapons 48 ->
    45. un -2.
  - moveeffects' three gib ternaries are now `xvel > 0 ? xvel - 1 : 0` (oracle `jle` polarity): sh -6, un -2, size
    unchanged.
  - Rejected: moveeffects `if (sprite[i].xvel <= 0) {A} else {B}` -> `if (xvel > 0) {B} else {A}` (oracle polarity,
    sh 330 -> 211). The function shrinks 3984 -> 3968, away from the exact oracle size, and un +44
    (`v/act_pol.py flip_1153`).

## Rule 2 (written-out copies keep parameters in fresh temporaries)
GAMEDEF, both kept:
- **execute: per-iteration parameter copies (STRONG).** The oracle loop head at `execute+0x118` re-copies i, p and x
  (`[esp+0x14]=[esp+0x24]`, `[esp+0x1c]=[esp+0x18]`, `edi=ebp`) and then tests `killit_flag`. Every CON_NEXT is
  `jmp +0x118`. The candidate copied once before `while (!killit_flag)` and duplicated the killit test at all 31
  continue sites; the oracle has one. This is parse's prologue (`if (killit_flag) return 1;`) written out inside
  the loop. New form:
  `for (;;) { short i2 = i, p2 = p; long x2 = x; ...locals...; if (killit_flag) break; switch ... }`.
  Effects: un 3737 -> 2808, sh 632 -> 583, byte_diffs 4922 -> 4730, frame now equal. Size 6951 -> 6911 (oracle
  7164; the remaining gap is the 40 un-merged `call parse` sites). All 24 orders of the four declaration groups were
  tested (`v/gd_exec2.py`); the copies-first order is best.
- **parsecommand: written-out getlabel and keyword copies (STRONG).** The oracle parsecommand has **no** calls to
  getlabel (2e7b0) or keyword (2e850). It has 6 calls to ispecial, 18 calls to isaltok and 14 to strcmp; the
  candidate had 6 getlabel, 11 keyword and 5 strcmp calls. So the oracle contains 6 getlabel bodies and 9 keyword
  bodies (2 isaltok + 1 strcmp loop each). The oracle CON_MOVE copy shows the fresh-temporary shape: p = textptr
  reloaded per iteration, `n` and the result in EBX, and the not-found path jumping straight to transnum. Added
  `GET_LABEL(done)` and `KEYWORD(k_, done_)` statement macros; each early `return` becomes `goto done`. The 9
  keyword sites were restructured: loop conditions became `for (;;) { KEYWORD(k, ..); if (k != -1) break; ... }`,
  and the `i < 2 && keyword() < 0` forms became `for (...) { KEYWORD(..); if (k >= 0) break; transnum(); }`.
  Effects: size 3536 -> 5328 (oracle 5232), un 2721 -> 1578, sh 833 -> 472.
  The variant where the copies break out of a do-while(0) instead of goto (`label_break`) gives 5408 / un 3006.
- **Inlining residue: COMPILER_STEERED alternative `src/gamedef_alias/`.** In the candidate, Watcom `-oe` inlines
  ispecial and isaltok into the written-out copies. In the oracle they stay calls, while standalone getlabel keeps
  ispecial inlined (`cmp bl,0xa/0x20/0xd`). Calling them through `#pragma aux` aliases (`special_char` ->
  `ispecial_`, `token_char` -> `isaltok_`) uses the same technique as SECTOR's `query_animation`. It gives exactly 6
  + 18 calls and **parsecommand 5232 = oracle size**, un 1399, sh 420, transnum un 166 -> 116. CONST and _DATA still
  VERIFIED; 9 STRICT.
  - HYPOTHESIS: these copies are Watcom `-oe` inline expansions of getlabel and keyword in the original build. The
    nested call was not re-inlined and the parameters were copied to temporaries, which would explain Rule 2.
  - Probe on the integrated source with profile flags + `-oe=N` (`oeprobe.py`; exploration only, not a matching
    result): N=20 gives 3536; N=40-80 give 4320; N>=100 gives 5296 with un 1832; STRICT stays 9 for all N.
  - Inlining alone does not reproduce the keyword copies (prior worker: 6 keyword calls remain). Do not change the
    profile on this evidence.
- ACTORS macro temporaries, all rejected (`v/act_r2.py`):
  - HURT with `hurt_p`/`hurt_damage` temporaries: un -1858 in moveactors but sh +125, size +32, moveweapons +264.
    The oracle folds the constant at HURT(p,5) (`sub edx,5`), so damage is not a fresh temporary.
  - FIND_IN_SECTOR with a tile temporary: no change. With a sector temporary: movestandables +17 B.
  - FIND_IN_SECTOR as a while loop: no change.
  - THROW_BLOOD with ang/speed temporaries: moveeffects +32 B, un +550.
  - FALL with a pointer temporary: does not compile; the struct tag differs, so not pursued.
- MENUES already has explicit copies (DRAW_SLIDER, MENU_ACTION). gamefix3's color-byte negative still stands; no new
  test.

## Rule 3 (SECTOR sprite-list loops top-tested)
`loopexp.py sector` converts each `for (v = head..; v >= 0; v = next[v])` loop to `v = head; while (v >= 0) {...;
v = next[v]; }` one at a time, then accumulates greedily. A loop is accepted only if the containing function's un
drops, its size does not move away from the oracle, and no STRICT function is lost. Loop numbers are from the
integrated sector.c, in order:
- No-ops: the small STRICT functions compile identically either way (0, 1: connecthead loops; 2, 3; 16; 27-36; 67).
  activatewarpelevators (4, 5) is also unchanged. Their for form was kept.
- Accepted (24): operatesectors 6, 9, 11, 14 (cases 15, 23i, 25i, 28-first). playerkeys copy 17-21, 25, 26.
  checksectors copies 38-43, 47, 49-52, 57, 65.
- Rejected (size moves away by -16/-32): 10, 12, 13, 15 (operatesectors 23j, 25j, 27, 28j); 23, 24; 45, 46, 48, 53,
  55, 56, 58, 63. Rejected (no gain): 7, 8, 22, 44, 54, 59-62, 64, 66.
- `WARP_ELEVATORS_INLINE` first scan loop as while (`v/sec_warp.py`): operatesectors un -29, sh -11, size exact. The
  second loop is rejected (-32 B).
- Most search loops (`... if (match) break;`) are top-tested. Several "apply to all" loops in the cases are not.
  This is consistent with gamefix3's negative for a blanket conversion.

SECTOR loop-variable widths: evidence recorded, change not kept.
- The oracle loads `headspritestat[3]` with these widths, in case order 29, 20, 23i, 23j, 25i, 25j, 27, 28j:
  16, 16, (ecx long), 16, 32, 16, 32, 32.
- The same pattern appears in every copy (public, activators, playerkeys, both checksectors copies). In case 23 the
  found sprite is a long in ECX (`test ecx,ecx; jl`) and the inner loop a short in SI. Case 25 is the same. Case 28's
  second loop is long and compares `edx` with a 32-bit hitag.
- So `i` is long, the inner and case-29/20 loop variable is short, and the getanimationgoal result is short (oracle
  `mov edi,eax; test ax,ax`).
- `long i, goal, middle; short j;` with case 29/20 loops renamed to j and case 28's second loop to i
  (`v/sec_j.py`, `v/sec_types.py`):
  - Copy by copy, un swings by ±600 bytes.
  - sh gets worse overall (2854 -> 3079..3388).
  - operatesectors loses its exact size (-160..-240).
  - Not kept. Register allocation dominates here, so this needs an allocator-level look (cg95), not a source sweep.

## Tools here
`scan.py UNIT [SRC] [--imm]` (width/jcc histograms), `shape.py` (register-blind metric), `vexp.py UNIT FUNCS
VARIANTS.py [BASE]` (variant runner: size, un, sh, STRICT, lost STRICT; SAVE=name), `loopexp.py UNIT [--greedy
N,..|--apply N,..]` (per-loop for->while), `oeprobe.py` (exploration-only -oe probe), `aligned.py`, `dd.py`,
`odis.py`, `sizes.py`, `summ.py` (from gamefix3). Variant files are in `v/`. `base/` holds NT-host baseline JSON of
the integrated sources.
