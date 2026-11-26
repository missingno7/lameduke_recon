# oeunits: is `-oe=N` inlining also the model for the other game units?

Toolchain: Watcom 9.5 GA (`wc95b`), `-4r -s -otexan -oe=N`. The sweeps ran on the NT host. The STRICT and data
results come from the canonical DOS host. Stock `tools/match.py`, no overlay. The candidates were rebased on the
`src/game` sources of 2026-11-26 20:57 (movelava/initlava rename).

## New toolchain facts (PROVEN by mini tests in `m/`)

- **Recursive functions ARE inlined, at depth 1.** The inlined copy keeps its self-calls as calls.
  - `m/r3.c`: `rec(8)` becomes `rec(7); a[8]=3` inline.
  - `m/r4.c`: a constant argument folds the copy away completely.
  - oeprobe's t13 (`m/t13.c`, re-run here) shows the same thing: caller has `mov eax,7; call rec; mov [a+0x20],3`.
  - The oeprobe/FINDINGS rule "recursive callees are not inlined" is wrong. It was read from the surviving
    `call rec`, which is the inner self-call of the inlined copy.
- **oeprobe's `refs.py` dropped self-calls** (`t != s`). Every "the oracle has no self-calls" statement is an
  artefact of that:
  - The oracle's operatesectors calls itself 2x.
  - The oracle's parse calls itself 40x.
  - The oracle's parsecommand calls itself 5x.
  - `refs.py` here counts self-calls.
- A static function whose every call is inlined is still emitted (`m/st1.c`). Hidden, fully inlined helpers would
  therefore leave a standalone body in the image.
- A compact `switch` (fallthrough or `goto` into a shared tail) does not get its inlined tails duplicated
  (`m/d1.c`).
- The reverse does happen. Identical per-case blocks made of plain calls are merged by the optimiser when they
  sit inside an inlined copy (checksectors' copy of checkhitswitch, below).
- The GAMEDEF crash at N >= 1500 does not happen with the natural source. That source compiles at N = 1500,
  3000 and 2e9 on NT, and at 3000 on DOS.

## Per unit (natural source, census = intra-unit caller/callee call counts vs oracle; `sw.py`, `res/sweep_final.txt`)

| unit | N that fits | residual census diffs | STRICT DOS before -> after | data segs |
|---|---|---|---|---|
| MENUES | >= 90 (window 80-90) | 0 | 8/11 -> 8/11 (same set) | CONST/_DATA/_BSS VERIFIED |
| ACTORS | >= 101 with one comma steer (else only 101..168) | 0 | 8/18 -> 8/18 | none |
| PLAYER | >= 485 (window 480-485) | 0 | 2/8 -> 2/8 | _DATA VERIFIED |
| SECTOR | >= 1520 (window 1500-1520) | 3 (+-1 setanimation per copy) | 9/18 -> 9/18 | none |
| GAMEDEF | >= 1330 (window 1320-1330) | 2 (parse self 21 vs 40, same as baseline) | 9/14 -> 9/14 | CONST/_DATA/_BSS VERIFIED |
| SOUNDS | any N | 0 | 12/13 -> 12/13 | CONST/_DATA/_BSS VERIFIED |

All six units fit together with GAME's N >= 2500: one `-oe=3000` profile works for every game unit. No STRICT
function is lost; the STRICT sets are identical (`res/dos_<unit>_{base,oe3000}.txt`).

NEAR byte diffs on DOS, baseline -> candidate (positional, so noisy):

| unit | total | function changes |
|---|---|---|
| MENUES | 3877 -> 4090 | draw_and_probe 635->395, menus 2989->3442 |
| ACTORS | 15480 -> 15602 | guts 42->0 (now only the jump-table entries of moveactors in its extent differ), moveactors 8698->8856 |
| PLAYER | 16751 -> 16622 | processinput 11762->11633 |
| SECTOR | 25666 -> 25532 | checkhitswitch 1085->1026, checksectors 7631->7526 |
| GAMEDEF | 12992 -> 12790 | execute 4911->4697 |
| SOUNDS | 149 -> 149 | unchanged |

### MENUES (`mk_menues.py`)

- **Inline source.** draw_and_probe(item, unsigned char color) is the title rotatesprite +
  `drawmenu(item.x, item.y, item.text, color, menu_y)` + `return probe(item)`.
  - The standalone inlines drawmenu and probe.
  - menus calls `action = draw_and_probe(menu_X, 4)` 10x. Each of those copies is inlined and keeps the
    drawmenu/probe calls (depth 1).
  - The 3 slider copies become `draw_menu_slider(...)` calls.
- **Removed.** MENU_ACTION, DRAW_SLIDER and the by-value struct copies. The single-use body macros are written
  into their functions.
- **Lead (not adopted).** With drawmenu's `shade0` declared `unsigned char`, menus improves from NT ibad 408 to
  169: the oracle passes the colour as `xor ecx,ecx; mov cl,..`. But drawmenu then shows a 2-slot stack swap and
  loses STRICT. drawmenu is kept as `short shade0`.

### ACTORS (`mk_actors.py`)

- **Natural calls.**
  - The 8 HIT_BY_WEAPON copies become `x = ifhitbyweapon(i)`.
  - The 3 THROW_BLOOD copies become `spawn_blood(...)`. spawn_blood is the uncalled inline source.
  - moveactors' gib block becomes `guts(i)`, and guts is written as 5 spawn_gib calls. The standalone inlines
    them: guts' own bytes are now exact. The depth-1 copy in moveactors keeps the 5 spawn_gib calls, as the
    oracle does.
- **The one conflict is `addweapon(p, 721, 1)` in moveactors, which the oracle keeps.** As a plain statement it
  is inlined at N >= 169, so natural ACTORS fits only N in [101, 168]. That contradicts PLAYER (>= 485), SECTOR
  (>= 1520) and GAME (>= 2500). Three explanations remain:
  1. **Comma expression (the candidate).** `ps[p].remote_ammo++, addweapon(p, 721, 1);` keeps the call by the
     call-site rule. Without `-oe` it gives the same code as the two-statement form (`v_comma` test). Census 0
     at every N >= 101.
  2. **ACTORS built with a different N** (101..168). Census 0. This would mean per-file flags.
  3. **Two objects** (`src/actors2/`, `mk_actors2.py`). The split is at 1:25600: addweapon, hitradius and
     spawn_gib in one object, the rest in another. Census 0 at N >= 200.
     - This hypothesis needs guts written with direct EGS calls, since spawn_gib would be cross-object. But the
       natural guts (spawn_gib calls inlined) matches the oracle bytes exactly, which argues for spawn_gib and
       guts sharing an object.
     - A split at 25f10 instead would need hand copies of ifhitbyweapon and spawn_blood again.
- **Remaining macros.** MOVE_ALONG (direct movesprite), FALL, HURT and FIND_IN_SECTOR are plain code helpers; no
  uncalled function corresponds to them.
- **Lead.** Writing moveeffects' two `MOVE_ALONG(..) == 0` sites as `ssp(i, 1)` drops moveeffects' NT ibad from
  1229 to 468. ssp is called only from GAMEDEF in the oracle; ACTORS calls of it would be inlined. Not adopted.

### PLAYER (`mk_player.py`)

- SHOOT_BODY becomes shoot()'s body: the macro's do/while `break`s become `return`.
- The 5 copies in processinput become `shoot(ps[snum].i, TILE, 0)`.
- The update_air and spawn_pistol_tracer copies become calls.
- Thresholds:
  - update_air inlines at N in (100, 120].
  - spawn_pistol_tracer inlines at N in (180, 200].
  - shoot inlines at N in (480, 485].
- displayraise stays a call. It owns an initialised local array, `frames[11]`.

### SECTOR (`mk_sector.py`, `clean_sector.py`)

- **operatesectors** is written once, with plain calls to setanimation, getanimationgoal, checkcursectnums,
  activatewarpelevators and itself. That removes:
  - the SET_ANIMATION_INLINE and WARP_ELEVATORS_INLINE copies;
  - the `query_animation` and `player_in_sector` `#pragma aux` aliases;
  - the SECTOR_INLINE_HELPERS machinery;
  - operate_helpers.h.
- **The 4 hand copies** in operateactivators, playerkeys and checksectors (x2) become `operatesectors(sn, source)`.
  These copies are inlined at N >= ~1510 because recursion does not block inlining. They keep the helper calls
  and the 2 self-calls, exactly the oracle's pattern.
- **checkhitswitch** is written with one complete block per switch position: FTA, xyzsound, operateactivators,
  and then operatemasterswitches or operateforcefields.
  - The standalone inlines the 4 operateactivators, giving 4 operatesectors calls.
  - checksectors ends with `checkhitswitch(w)`. Its inlined copy is merged by the optimiser into the compact
    `FTA xyzsound operateactivators operatemasterswitches xyzsound operateactivators operateforcefields`, as in
    the oracle.
  - This replaces SWITCH_EFFECTORS_INLINED and the written-out tail of checksectors.
- **Residual.** Each operatesectors copy has 19 or 20 setanimation calls depending on the caller, in a different
  per-copy pattern than the oracle's. This is tail merging and layout in the lift cases (15-23,
  lift_up/elevator_up), the same kind of difference the old hand copies tuned with for/while variants.

### GAMEDEF (`mk_gamedef.py`)

- **parsecommand.** The 6 GET_LABEL copies become `getlabel()`, and the 9 KEYWORD copies become `k = keyword()`.
  keyword now classifies with `isaltok(*p)` calls, which the standalone inlines (still STRICT). The
  `special_char`/`token_char` `#pragma aux` aliases go.
- **loadefs.** It calls passone, which is inlined and keeps its parsecommand call.
- **execute.** It becomes setup + `while (!parse(i, p, x));` + deletesprite/return +
  `move_actor(i, p, x)`.
  - parse is inlined at N >= ~1325. Its copy keeps the self-calls, getincangle_shift and furthestangle.
  - move_actor is inlined. Its copy keeps getincangle_shift (`a[e] += f()`) and ssp.
- **Residual.** parse's self-call count is 21 against the oracle's 40, the same as the baseline. Both PARSE_IF
  branches end in an identical `mov eax,ebx; mov ebx,esi; call parse` tail that the optimiser cross-jumps. In the
  oracle, x lives in edi and each branch has its own call. This is allocation, not structure.
- **Remaining macros.** PARSE_IF, and TOKEN_CHAR in transword/transnum/parsecommand. Writing TOKEN_CHAR as
  isaltok() calls costs transword its STRICT (ibad 4).
- **parsecommand regresses on NT**, ibad 372 -> 1718. The frame is 0x10 against 0xc: the getlabel/keyword
  inline temporaries need tuning.

### SOUNDS

- No intra-unit calls; nothing depends on N.
- `bootleg_cache_reset` as `while (cache_depth > 0) bootleg_cache_pop();` does not match (ibad 11), so it was
  written out originally.

## Recommendation

- **Use `-oe=3000` (game_oe) for MENUES, PLAYER, SECTOR, GAMEDEF and SOUNDS.** Adopt the natural candidates:
  same STRICT, data verified, and the copy macros and aliases are gone.
- **ACTORS.**
  - Adopt with the comma steering marked COMPILER_STEERED. If the integrator prefers zero steering, keep ACTORS
    on profile `game` until the addweapon question is settled.
  - The two-object and per-file-N alternatives are recorded above.
- **Correct docs/FINDINGS.md.**
  - Recursive callees are inlined at depth 1.
  - The oracle's self-calls exist.

## Files

| Group | Files |
|---|---|
| Candidates | `src/<unit>/<unit>.c` plus copied headers; `src/actors2/{acta,actb}.c` (split hypothesis); `src/actors/actors_nocomma.c` |
| Generators | `mk_menues.py`, `mk_actors.py` (applies the comma edit; `COMMA=0` gives the plain form), `mk_player.py`, `mk_sector.py` + `clean_sector.py`, `mk_gamedef.py`, `mk_actors2.py`, `mk_actors_var.py` |
| Tools | `sw.py UNIT SRC N..` (census; `units.py` ranges), `st.py TAG SRC FLAGS` (match.py summary into `res/`), `fd.py SRC FUNC|all [-q] [--noe|-oe=N]` (instruction diff/ibad), `fdw.py` (diff window around a callee), `tv.py` (substitution trials), `unmacro.py`; `mc.py`/`g4.py`/`refs.py`/`ucensus.py` (from oeprobe, with `INC` and self-calls) |
| Results | `res/dos_*` (DOS host match output and json), `res/sweep_final.txt` |

## Commands

DOS-host verifier, as used for the table:

```
python tools/match.py --install wc95b -4r -s -otexan -oe=3000 -i=build/workers/oeunits/src/<unit> build/workers/oeunits/src/<unit>/<unit>.c
```
