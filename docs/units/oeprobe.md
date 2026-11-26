# oeprobe: was GAME compiled with a large `-oe=N`?

Toolchain: Watcom 9.5 GA (`wc95b` install), profile `game` flags `-4r -s -otexan`, plus `-oe=N`. Exploration ran on
the NT host (`LAMEDUKE_HOST=nt`). The final numbers come from the canonical DOS host. Stock `tools/match.py`; no
overlay.

## Answer (STRONG; every listed rule PROVEN by a mini test or an A/B compile)

Yes. Write GAME as ordinary calls and compile it with any `-oe=N` where N >= 2500. The compiler's inliner then
reproduces the oracle's intra-GAME call and inline pattern exactly: 0 differences over every (caller, callee) pair
(`census.py`). Two conditions apply:

1. The GAME range `1:00010-14fad` is **two objects**, split at `1:0f160` (`spawn`). Object 2 runs from spawn to
   getscorename. Calls between the objects are never inlined. This confirms the dataown boundary hypothesis.
2. GAME's strings are **string literals**, not the provisional extern names.

Under these conditions every written-out copy we model by hand is a compiler inline expansion. That includes
update_timer_input x6, inittimer, uninittimer, FTA, record, move_status9, displayrooms and its
animatesprites/water copies, the text helpers, displaystatus, displayquotes, draw_monitor_overlay, domovethings,
the packet decoder (domovethings_multiplayer), and spawn in prelevel. Some whole regions are inlined functions:

| Region | Inlined function |
|---|---|
| main's start-up wait | `wait_for_players()` |
| main's demo loop | `playback_demo()` |
| main's chat line | `strget(10, 140, next_text_shade(), 79)` |
| main's multiplayer branches | `network_move_loop()` |
| dobonus/displayhighscores `ltoa` + minitext | `numtext()` |
| nonsharedkeys `ltoa` + gametext | `numtext_relative()` |
| moveeffectors case 128 | `spawn_random_effects(i, 15)` |
| update_menu_cursor | `next_text_shade()` + `draw_weapon_tile()` |
| prelevel head | `reset_player_motion(0)` |
| render_mirror, render_sprite_camera | `render_camera_tile(...)` |

This explains why playback_demo, network_move_loop, wait_for_players, numtext_relative, draw_meter,
spawn_random_effects, render_sprite_camera and reset_player_motion have no callers.

## The 9.5 inliner selection rule (mini tests in `m/`, `mc.py`)

- **Size (PROVEN, t5/t6/t7).**
  - A callee is inlined when its tree node count is <= N.
  - Operator nodes count; leaves and folded constant addresses do not. For example, `a[3] = c;` is about 1 node,
    and a 12-operator statement is about 13.
  - A function's base cost is about 3.
  - The limit applies per callee and is not cumulative. There is no per-caller budget: t9 inlined 40 expansions
    of about 150 nodes each.
  - Plain `-oe` means 20. A later `-oe=N` overrides the `e` in `-otexan`. `-oe=0` also means the default.
  - Large values work up to at least 2e9 and give byte-identical objects to N = 2500 for this unit.
- **Depth 1 (PROVEN, t8).**
  - An expansion is built from the callee's *original* tree, so calls inside an inlined body stay calls.
  - The callee's own standalone code still inlines those calls.
  - This is the "copies call X while the function itself inlines X" pattern seen everywhere in GAME.
- **Callees that own static data are never inlined (PROVEN, t6 from gamefix7, t13, t22).** This covers string
  literals, `static` locals and initialised auto aggregates. An example of the last kind is moveeffectors'
  `unsigned char statlist[3] = {1,4,5}` and `frames[12] = {...}`.
  - Because of this rule, moveeffectors is never inlined at any N. `mebisect.py` finds cases 18 and 129 are
    each sufficient.
  - Literals explain the kept calls to gameexit, show_help, checkcommandline, read/write_userdefs, loadlookup,
    read/write_demo, nonsharedkeys, displayhighscores, enterlevel and prelevel.
- **Recursive callees are not inlined (PROVEN, t13).**
- **No effect (PROVEN, t10/t11/t13/t19/t20).** These do not affect inlining: definition order (before or after
  the caller), `static` linkage, K&R definitions, 14 parameters, returns inside loops, goto/labels, jump-table
  switches, `&param`/`&local`, and calls in the arguments.
- **Call-site rule (PROVEN for the forms tested, t15-t18).** A call is not inlined when an operand with run-time
  address arithmetic is evaluated before it in the same expression.
  - Not inlined: `a[i] = f()`, `p[e] = f()`, `*(p+2) = f()`, `a[e] += f()`, `a[e] + f()`, `a[e] == f()`,
    `g(a[e], f())`.
  - Inlined: `d = f()`, `*p = f()`, `s.f = f()`, `a[3] = f()`, `d = f() + a[e]`, `g(f(), a[e])`, `if (f())`.
  - This rule explains why moveeffectors' `sprite[i].owner = findlocator(...)` stays a call (4 sites), while FTA
    in the same function is inlined.
- **Not inlined across objects.** This is trivially true. It explains, for example, `spawn -> EGS`,
  `render_* -> animatesprites`, `getscorename -> strget/next_text_shade`, `main -> getscorename/newscore/check_file_id`
  and `enterlevel -> minitext/numtext/displaystatus/...`. The existing COMPILER_STEERED aliases (`score_text_shade`,
  `enter_score_name`, `file_id_check`) and the `do{}while(0)` timerhandler blocks become unnecessary.
- **Other 9.5 toggles.** `#pragma on/off(inline)` and `#pragma on/off(reuse_duplicate_strings)` exist. The
  `reuse_duplicate_strings` toggle was tested: with it off, the CONST pool grows to 1713 B. The original did not
  use it. The help text has no inline depth or size switch other than `-oe[=num]`.

## Sweep (natural sources, `thresh.py`; full table in `tmp/thresh.txt`)

Differences are counted over (caller, callee) call counts against the oracle (`census.py`, `refs.py`).

| -oe | 10 | 20 | 40 | 60 | 100 | 120 | 180 | 250 | 300 | 400 | 500 | 1000 | 2000 | 2500 | 3000 ... 2e9 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| differences | 89 | 81 | 77 | 50 | 43 | 34 | 23 | 17 | 10 | 4 | 1 | 1 | 1 | **0** | **0** |

The table below gives the smallest N that inlines each callee (natural source; the window is (previous N, N]).

| Window | Callees inlined |
|---|---|
| <=20 | next_text_shade, uninittimer, wait_for_players, numtext, numtext_relative |
| 20-30 | inittimer |
| 30-40 | numtext_fixed, record, FTA |
| 40-50 | draw_weapon_tile, draw_monitor_overlay |
| 50-60 | minitext, displayquotes, draw_water_distortion, domovethings |
| 60-80 | gametext, spawn_random_effects |
| 80-100 | reset_player_motion |
| 100-120 | update_timer_input, playback_demo, strget |
| 120-140 | EGS, resetplayer |
| 160-180 | displayrooms |
| 200-250 | domovethings_multiplayer |
| 250-300 | network_move_loop, render_camera_tile |
| 350-400 | displaystatus |
| 400-500 | animatesprites, move_status9 |
| 2000-2500 | spawn (lower bound for N) |
| never | moveeffectors (static data); every string-literal function |

N >= 2500 has no upper bound in GAME, so the exact original value is undeterminable. The candidate uses 3000.

**A/B on the string rule.** The same natural source with the strings as extern names (`v/nat1.c`) leaves 34
differences at every N >= 500. All of them are calls to the literal-owning functions, which get inlined.
Making the strings literals (`v/nat1L.c`) brings this to 0.

The old NEGATIVE result ("gameexit/write_userdefs inlinable at 26 before FTA at 32") was an artefact of the
extern string names. "minitext/gametext/draw_weapon_tile calls kept" was an artefact of the macro-based source:
those calls sit inside inlined numtext, numtext_relative, strget and displayquotes bodies.

## Candidate (recommended form)

- **Files.**
  - `cand/game.c`: GAME object 1. It becomes src/game/game.c.
  - `cand/game2.c`: object 2, from spawn to getscorename. Its file name is a HYPOTHESIS; dataown suggested a
    PREMAP-like file.
  - Both are generated from src/game/game.c by `mknat.py`. Headers: `src/game.h` and `src/build_v5.h`, unchanged
    copies.
- **Flags.** `-4r -s -otexan -oe=3000`.
- **Source changes.** Natural calls replace every copy macro (FTA_INLINE, STATUS_BAR, SHOW_QUOTES, WEAPON_TILE,
  SMALL_TEXT, DRAW_VIEW, BIG_TEXT, NUMTEXT_FIXED, MOVE_STATUS9) and every written-out body listed above.
  - numtext and numtext_relative are `ltoa` plus a minitext/gametext call.
  - strget's display part is the minitext/gametext and draw_weapon_tile calls.
  - The GAME string names become literals, using the oracle's bytes (56 names).
  - "L?.MAP" is taken in checkcommandline's case 'e'. This gives the oracle's literal order.
  - Object 2 declares `level_warp`, `oldtimerhandler` and `bonus_marks` extern. These stay in object 1.

### DOS host result

Commands:

```
python tools/match.py --install wc95b -4r -s -otexan -oe=3000 "-i=build/workers/oeprobe/src" build/workers/oeprobe/cand/game.c
python tools/match.py --install wc95b -4r -s -otexan -oe=3000 "-i=build/workers/oeprobe/src" build/workers/oeprobe/cand/game2.c
```

| Object | STRICT | Bytes | Data segments |
|---|---|---|---|
| object 1 | 26/49 | 11,856 | CONST 1604 B VERIFIED at 2:00004; _DATA VERIFIED at 2:02304; _BSS VERIFIED at 2:0ac50 |
| object 2 | 4/12 | 1,533 | CONST 96 B VERIFIED at 2:00648 |
| **total** | **30/61** | **13,389** | |
| baseline src/game/game.c (`--profile game`) | 29/61 | 11,037 | |

Outputs are in `cand/dos_game.txt`, `cand/dos_game2.txt`, `cand/dos_game.json` and `cand/dos_game2.json`. The
baseline output is `tmp/dos_base.txt`.

- **Gains.**
  - displaystatus: NEAR to STRICT.
  - The 56 provisional GAME string externs disappear, and GAME's literal pool now verifies. This was OPEN in
    docs/OWNERSHIP.md.
  - timerhandler stays STRICT with six plain calls, without the do/while(0) steering.
  - No baseline STRICT function is lost.
- **Changed byte-diff counts among the NEAR functions (DOS, baseline -> candidate).** These are positional, so
  they are noisy.

  | Change | Functions |
  |---|---|
  | Better | checkcommandline 139->59, moveeffectors 10109->9731, dobonus 2744->2464, prelevel 7028->6881, domovethings 1198->1134, strget 762->725 |
  | Worse | main 3361->4273, playback_demo 569->1897, displayquotes 88->440, domovethings_multiplayer 19->383, nonsharedkeys 303->518 |
  | Size changes | strget +16, playback_demo -16 |

  The total ibad (unequal instructions, NT) is 6191 in the baseline and 6261 in the candidate. The worse ones are
  callers whose inline copies were hand-tuned. Now one callee source drives every copy, so the tuning moves to
  the callee bodies. A quick displayquotes `short offset` trial moved main from 4273 to 3576 diffs (NT): these are
  levers to work on.

## Other game units: same compiler, but they need their own natural sources

`ucensus.py` runs the current sources at the default N and at large N. `refs.py` lists the functions with no
callers.

- **PLAYER, SOUNDS.** No change at any N; they are consistent with a large N.
  - PLAYER's `shoot` inside processinput is naturally an inline: same object, depth 1. parse/execute and
    ACTORS keep calls because they are in other objects.
  - spawn_pistol_tracer and update_air are uncalled, which fits them being inlined everywhere.
- **MENUES.** At N >= 100, drawmenu/probe would be inlined into menus, but the oracle keeps 10 calls of each.
  draw_and_probe has no callers. menus probably calls draw_and_probe 10 times, inlined, and its body keeps the
  drawmenu/probe calls (depth 1). This needs a natural menues.c (HYPOTHESIS).
- **ACTORS.** At N >= 100, spawn_gib (5 sites) and addweapon (1 site) in moveactors would be inlined, but the
  oracle keeps them. spawn_blood has no callers. This fits an inlined helper whose body calls spawn_gib
  (HYPOTHESIS; not tested).
- **GAMEDEF.**
  - At N <= 1000 the result is the same as at the default.
  - At N >= 1500, wcc386 9.5 crashes (rc 127) on the current gamedef.c. Our parse/parsecommand are
    self-recursive (21/5 self-calls), and the oracle has none.
  - getlabel, keyword, passone and move_actor have no callers, which fits them being inlined.
  - The current recursive model is incompatible with a large N. OPEN.
- **SECTOR. Not explained.**
  - At N=40/100/300/3000 there are 3/6/9/22 differences against 2 at the default.
  - At large N, checksectors would inline operateactivators, operateforcefields, operatemasterswitches,
    checkcursectnums, getanimationgoal and operatesectors. The oracle keeps them, and none owns static data in
    the current source.
  - The current operatesectors is self-recursive (2 self-calls), and the oracle has none.
  - Either SECTOR was built with the default threshold, or its structure differs, for example through inlined
    wrappers. OPEN.

## Recommendation

- **Change GAME's model.** Use natural calls, split GAME into two objects at 1:0f160, make the string literals
  real, and compile with `-oe=N`, N >= 2500.
  - The call/inline pattern is reproduced exactly, with no COMPILER_STEERED aliases and no copy macros.
  - STRICT goes from 29 to 30 on DOS, and GAME CONST verifies.
  - The remaining NEAR residue is local tuning of callee bodies.
- **Use a new profile.** Add something like `game_inline` with `-4r -s -otexan -oe=3000` for game_game and the
  new second object only. Keep `game` for the other units until each is reworked naturally: a large N currently
  breaks SECTOR, crashes GAMEDEF, and changes MENUES and ACTORS.
- **Integrator work.**
  - Add the second unit to units.json, with its range 1:0f160-14fad and its CONST at 2:00648.
  - Object 2 should probably also own mspos (2:0ac60, now in sector.c), per OWNERSHIP.
  - Choose the file names.
  - The candidate files are CRLF. src/game/game.c is LF. The DOS-host CONST verified with CRLF; recheck with LF
    before adopting.

## Files
- `mknat.py`: generates `v/nat1.c`, `v/nat2.c`, `v/nat1L.c` and `v/nat1L2.c` (= `cand/game.c`) from
  src/game/game.c.
- `mc.py` compiles a file and lists the calls in each function. `census.py` / `thresh.py` compare the GAME call
  pattern with the oracle at one N or over a sweep. `ucensus.py` does the same for any unit. `refs.py` counts
  oracle near calls per function. `mebisect.py` finds what blocks moveeffectors. `tryv.py` is a variant tester.
  `ibad.py` reports ibad per function. `g4.py`, `ocalls.py`, `oesweep.py` and `allsz.py` come from gamefix7.
- `m/t*.c` and `m/s*.c` are the mini tests.
- `tmp/` holds the sweep outputs: `thresh.txt`, `census_*.txt`, `ucensus.txt`, `m_*.txt`, `ib_*.txt` and
  `dos_base.txt`.
- One compiler quirk: wcc386 9.5 hangs, compiling for more than 60 s with no exit, on `m/t1.c`, a simple test
  combining calls and a `while` loop. Use timeouts.
