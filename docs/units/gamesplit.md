# gamesplit worker notes (2026-11-26)

Scope: test whether obj1 `1:00010-14fb0` (unit game_game, 61 functions, compiled from one src/game/game.c) holds
two objects, and if so write it as two C files. Toolchain: wc95b `-4r -s -otexan` (profile game). Final numbers
come from the DOS host. Only files under build/workers/gamesplit/ were written.

## Result

| Build (DOS host) | STRICT | STRICT bytes | CONST | _DATA | _BSS |
|---|---:|---:|---|---|---|
| integrated src/game/game.c | 29/61 | 11,037 | 96 B at 2:00648 VERIFIED (2:00004-00648 are 56 `extern` names) | 96 B VERIFIED 2:02304 | 7 B VERIFIED 2:0ac50 |
| split game.c (src/) | 25/49 | 9,504 | **1604 B VERIFIED 2:00004** (56 real literals) | 96 B VERIFIED 2:02304 | 7 B VERIFIED 2:0ac50 |
| split premap.c (src/) | 4/12 | 1,533 | 96 B VERIFIED 2:00648 | none | **4 B VERIFIED 2:0ac60** (mspos) |
| split total | **29/61** | **11,037** | both pools VERIFIED | VERIFIED | VERIFIED |

- Every function keeps its status. The four STRICT functions in premap.c are enterlevel, check_file_id,
  newscore and getscorename. NEAR byte diffs are the same except two. checkcommandline falls from 139 to 59 because
  of the `"L?.MAP"` placement (see below). moveeffectors falls from 10109 to 10105, and alt11380's spawn from 6402
  to 6397. Both falls are extent effects: the function is now its object's last, so its extent ends at the
  segment end.
- On the NT host the split gives the same 29 STRICT and both CONST pools VERIFIED. The integrated build gives only
  27 STRICT on NT, because its CONST at 2:00648 fails there.
- The pool can be VERIFIED with plain literals and default pooling. Nothing needs
  `#pragma off(reuse_duplicate_strings)`. The duplicates (`Episode`, `Level`, `Board not found.`) are natural
  because they lie in different objects. The pool still holds the identical pairs exit/main banner and
  unknown_parameter ×2, and our compiler keeps them separate too (it merges only some duplicates within one unit;
  that rule is OPEN).

Commands (repository root):
```
python build/workers/gamesplit/tools/split.py 0f160      # writes src/  (game.c, premap.c, game.h, build_v5.h)
python build/workers/gamesplit/tools/split.py 11380      # writes alt11380/src/
python tools/match.py --profile game "-i=build/workers/gamesplit/src" build/workers/gamesplit/src/game.c
python tools/match.py --profile game "-i=build/workers/gamesplit/src" build/workers/gamesplit/src/premap.c
python -B build/workers/gamesplit/link/link.py --label split
```
Outputs: results/src_{game,premap}.dos.txt, results/alt11380_src_{game,premap}.dos.txt, results/base.dos.txt
(integrated control). results/*.lst holds the per-function status lists.

## Evidence for two objects (STRONG)

1. **Padding.** WLINK fills between objects with zeros; the compiler fills between its own functions with `lea`
   no-ops (`8d 80 00 00 00 00`, `8d 92 ...`, `8b c0`, `90`). wc95b emits no fill after an object's last function:
   our objects end unaligned (game.obj _TEXT is 61,707 bytes). The oracle ends at `getscorename` with
   `c3 00 00 00` at 14fad. _TEXT is paragraph-aligned (OMF align 3), so an object can start only at a 16-byte
   boundary where the previous function needs no `lea` fill. In 1:0ba30-14fb0 the only such places are 0f160
   (moveeffectors' 28-entry jump table ends exactly there), 11380 (spawn's `ret` ends exactly there), 149b0 and
   14e00. Every other function start in the range follows `lea` fill, so it is in the same object as its
   predecessor.
2. **String pools.** Functions up to write_demo reference only 2:00004-00648. prelevel, enterlevel and check_file_id
   reference only 2:00648-006a8. Nothing between them (moveeffectors, spawn, render_*, resetplayer,
   reset_player_motion) references any literal. The second pool repeats `Episode`, `Level` and
   `Board not found.` from the first, and the split compiles both pools byte-exact with plain literals.
3. **_BSS.** mspos (2:0ac60) is the paragraph-aligned contribution after GAME's 7 bytes. The only obj1 references
   are spawn (11 sites), reset_player_motion (11fc3) and prelevel (12339...141aa). The one outside
   reference, 1:2f705 in parsecommand, is `label-1` (2:0ac6f = GLOBAL's 0ac70 - 1), not mspos. So the `long mspos`
   that sector.c hosts today belongs to the second object (VERIFIED there). SECTOR keeps 9/18 STRICT without it
   (checked with link/sector.c).
4. **Call directions.** Calls go both ways: main, nonsharedkeys, playback_demo, read_demo, domovethings and
   move_status9 call into the second part; the second part calls EGS, cacheit, displaystatus, dobonus, moveloop,
   minitext, numtext, gameexit, level_load_hook, next_text_shade and strget. That is consistent with two
   separately compiled files. The integrated TU needed `#pragma aux` aliases (enter_score_name -> getscorename_,
   score_text_shade in newscore, call_level_load_hook) to stop cross-calls from being expanded in place. In the
   split, two of them become plain calls with identical code.
   - main -> getscorename is now a direct call.
   - newscore -> next_text_shade is now a direct call.
   - check_file_id keeps its alias: main tests the whole of EAX, as an undeclared int function.
   - call_level_load_hook keeps its alias: enterlevel passes an argument to level_load_hook(void).

## Where the second object starts: 0f160 or 11380 (OPEN, code-neutral)

Both splits compile to the same function statuses and the same VERIFIED segments (alt11380/ results).
149b0 and 14e00 would split the 2:00648 pool and the prelevel/enterlevel chain without any gain, so they are not
pursued. Parsimony favours one second object.

- **For 0f160 (the hypothesis I was given; delivered in src/).** Every mspos user is in the second object. The
  boundary follows moveeffectors' jump table. Coincidence it needs: spawn's code happens to end on a 16-byte
  boundary, chance about 1/16.
- **For 11380 (delivered in alt11380/src/).** Coincidence it needs: the 4-aligned jump table (starts 0f0f0,
  112 bytes) happens to end on a 16-byte boundary, chance about 1/4. This makes 11380 about 4 times more likely on
  the padding evidence alone.
- The Duke 1.5 analogue also favours 11380. Its PREMAP.C runs xyzmirror -> vscrn -> resetplayerstats -> ...
  -> prelevel -> newgame -> ... -> enterlevel, matching render_camera_tile/render_sprite_camera/render_mirror ->
  resetplayer -> reset_player_motion -> prelevel -> newgame -> enterlevel. spawn is in Duke 1.5 GAME.C.
- **Recommendation: 11380**, with spawn as GAME's last function, unless new evidence appears. spawn alone as a third
  object would explain both exact ends, but it needs a one-function file: not pursued.
  In both variants mspos is defined in premap.c.

## Source changes besides moving bodies

- The 56 provisional `extern char name[]` CONST names became string literals at their use sites. The texts come
  from build/workers/dataown/src/game_lit, and the high-ASCII banner bytes are kept via latin-1 I/O.
- checkcommandline: the oracle pool puts `"L?.MAP"` between `output_rerouted` and `encode_message`
  (2:002ec), so the literal is now written in case 'e' rather than before the loop. The compiler still loads it
  into EBP before the loop (`mov ebp, 0x2ec` in the oracle). The function's NEAR diffs fall from 139 to 59.
- read_demo: `char *filename = "mydemo__.dmo"; filename[7] = ...`, as write_demo spells it. It stays STRICT.
- Declarations superseded by dataown's macros were dropped, because the macros now come first from game.h:
  `extern long turn_velocity...`, `extern short facing_sprite`, `extern short disable_armor`.
- game.h holds the declarations both files use, in their original order. It contains the user_settings and
  player_view structs, the PRAGMAS.H inline helpers, the dataown field macros and the prototypes. Each .c file
  keeps only its own declarations and data definitions: game.c has oldtimerhandler, level_warp and bonus_marks;
  premap.c has mspos. No .inc files were used.

## Link (optional step 4)

link/link.py is link2's proposed/link.py with the paths redirected.
- GAME is built from src/, `game_premap` is added and listed as `file obj\premap.obj` after game.obj, and SECTOR
  is built from link/sector.c (mspos removed).
- It uses link2's candidate directive otherwise, with WLINK 9.5 under DOSBox-X.

Results:
- **Undefined symbols: 2** (`_SoundToggle`, `_camerasprite`, the known SOUNDS field aliases). Every GAME string name
  now resolves.
- **obj2 file-backed agreement: 99.67%** (43,953/44,097; link2's best scoped control was 99.42%).
- **2:00000-01490 is identical.** Both pools and GAME's `_DATA` lie in that range. The 144 differing bytes all lie
  at 2:01491 or later, in 2-3 byte runs: CONST 72, _DATA 38, _GF1INIT/XI/YI 34. These are other units' data, not
  this split's.
- In the map, mspos lands at 2:0ac58 instead of 0ac60. This is the known OPEN paragraph-alignment of post-GAME
  _BSS contributions; premap.obj declares dword _BSS.

## For the integrator

- Add a unit for src/game/premap.c (or game_premap): profile game, include src/game. Move the 12 functions
  (or 11 for 11380) to it in config/units.json. Remove `long mspos;` from src/game/sector.c.
- Add `file obj\premap.obj` after game.obj in the link directives.
- This removes the 56 CONST pseudo-symbols for GAME's literals from the registry. OWNERSHIP.md rows 191-245 become
  literals.
- OPEN: whether premap.c starts at 0f160 or 11380 (both delivered; 11380 recommended); the file name; and why
  wc95b keeps some identical literals separate within one unit (banners, unknown_parameter) while it merges others
  (bonus_percent/bonus_points ×3).
