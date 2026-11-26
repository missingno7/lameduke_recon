# Game data ownership (dataown worker, 2026-11-26)

Scope: the 149 game names that every game unit only declared `extern` in the first natural link. The table also
lists `printext16_`, `_ipxcallback` and `_rmbuffer`, which other workers own. Toolchain: wc95b `-4r -s -otexan`
(profile game). Final runs used the DOS host. The link used WLINK 9.5 under DOSBox-X. Results are in
`results/*.txt`; link output is in `link/run*/`.

## Results

| Unit | Canonical STRICT | dataown STRICT | Data segments (dataown, unmodified match.py + overlay.json) |
|---|---:|---:|---|
| GAME | 27 | 28 (+enterlevel) | _DATA 96 B VERIFIED 2:02304, _BSS 7 B VERIFIED 2:0ac50, CONST 96 B VERIFIED 2:00648 |
| SECTOR | 9 | 9 | _BSS 4 B VERIFIED 2:0ac60 (mspos host) |
| PLAYER | 1 | 2 (+update_air) | _DATA 120 B VERIFIED 2:02364 |
| ACTORS | 8 | 8 | no data contribution |
| GLOBAL (new, no code) | - | - | _DATA 1116 B VERIFIED 2:023dc; _BSS 633,683 B VERIFIED 2:0ac70 under `match_pruned.py` |
| SOUNDS | 12 | 12 | CONST/_DATA/_BSS VERIFIED (unchanged) |
| MENUES | 7 | 7 | CONST VERIFIED, _DATA 500 B VERIFIED 2:0291c, _BSS 50,188 B VERIFIED 2:a59e0 (was MISMATCH) |
| GAMEDEF | 9 | 9 | CONST/_DATA VERIFIED, _BSS 15 B VERIFIED 2:b1df0 (new) |

No unit loses a STRICT function. The plain matcher reports GLOBAL's _BSS as MISMATCH for one reason:
config/symbols.json binds second spellings of the same storage inside its span (see "Registry change").

Link (`link_dataown.py`, with GLOBAL listed after ACTORS):

- With `src/game/game.c`, **58 symbols are undefined**: the 56 GAME literal names, `SoundToggle` and `camerasprite`.
- With `src/game_lit/game.c`, **2 are undefined**: `SoundToggle` and `camerasprite`.
- The baseline had 152.

In the diagnostic map, every bound data symbol keeps one constant delta within its object. The delta changes only at
object boundaries (see the para-alignment finding below). ENGINE's shift fell from -0x974c4 to -88 bytes.

## Evidence

**GLOBAL object (STRONG for one object, HYPOTHESIS for the file name).** The range `2:0ac70-a57c3` forms one monotone
Watcom 9.5b size-bucket sequence:

- 8-byte multiples at `0ac70-a5230` (music_files through animwall);
- 4-byte multiples at `a5230-a5308`;
- the 130-byte env_music;
- size 2 at `a538a-a53aa`;
- odd sizes at `a53aa-a57bd`: six 17-byte input arrays, 15-byte menu descriptors, the 741-byte voice table and the
  21-byte settings block;
- size 1 at `a57bd-a57c3`.

SECTOR's storage interleaves with storage from other units in every bucket. SECTOR has animatevel at 0eec8,
animatesect at a1430, animatecnt at a5250 and numanimwalls at a53a8. GAMEDEF's actorscrptr, script and textptr sit
among them, as does storage used by GAME, ACTORS, SOUNDS, MENUES and PLAYER. A code unit could own the span only by
defining all of it.

Names that look original fall in hash order inside each run:

- textptr/insptr 71, labelcnt 112, label 126, labelcode 179;
- ps 8 < hittype 101;
- warning 71 < error 81 < killit_flag 209.

GLOBAL's initialised part runs from 2:023dc to 02838: score names, high scores, FTA quotes and two scan maps. It lies
after PLAYER's _DATA and before SOUNDS'. GAME's _DATA must come before PLAYER's (bonus_marks at 2:02304). Therefore
`quote_text` and `scan_ascii*` cannot be GAME's. Duke Nukem 3D 1.5 keeps the same kind of storage in a GLOBAL.C with
no code. PLAYER or ACTORS could host these definitions with the same layout. Nothing in the image distinguishes those
choices.

**Userdefs fields (STRONG).** The 21-byte block starts at 2:a57a8. These names sit at the following offsets in it:

| Offset | Names |
|---:|---|
| +0 | `user_visibility` |
| +3 | `SoundToggle` |
| +5 | `disable_armor` |
| +7 | `sound_volume` |
| +9 | `music_volume` |
| +0xb | `gamepaused` / `transition_zoom` |
| +0xd | `camerasprite` / `facing_sprite` |
| +0xf | `mouse_sensitivity` |

Separate shorts would sort into the size-2 bucket. These lie in the odd bucket after the menu descriptors, so they are
fields of one 21-byte object. `MusicToggle` (2:1bde42) is ENGINE's `option[2]`: `option` is at 2:1bde40 and holds the
SETUP.DAT card bytes.

**GAME (STRONG).** GAME's _BSS is 7 bytes: oldtimerhandler (6 bytes) and `level_warp` (byte stores at 1:0558b and
1:0571f). Its _DATA is the 96-byte `bonus_marks`; dobonus reads offsets +0 and +2. The 56 GAME pool names are
literals of GAME's CONST, in function order.

**Second object inside 1:00010-14fb0 (STRONG, new).**

- The CONST bytes at 2:00648-006a8 serve prelevel, enterlevel and check_file_id. They repeat `Episode`, `Level` and
  `Board not found.`, which already appear in the 0004-0648 pool.
- Our wc95b coalesces such duplicates within one compilation (src/game_lit gives a 1664-byte pool). The image keeps
  both copies, so these functions were a separate object with its own pool.
- No literals occur from write_demo (1:0b920) to prelevel (1:12210). Boundaries without zero padding exist at
  1:0f160 (after moveeffectors' jump table), 1:11380, 1:149b0 and 1:14e00.
- `mspos` at 2:0ac60 is that object's _BSS. Its users are spawn (0f160), resetplayer (11de0) and prelevel (12210).
  GAME's _BSS ends at 0ac57 and GLOBAL's starts at 0ac70.
- The likely boundary is 1:0f160, which suggests a later PREMAP-like file.

The split moves function bodies, so it is left to the integrator. Until then, sector.c hosts mspos, which gives the
same link position.

**Para-aligned _BSS contributions (STRONG, cause open).** Every _BSS contribution after GAME's starts on a 16-byte
boundary:

| Start | Previous contribution ends |
|---|---|
| 0ac60 | 0ac57 |
| 0ac70 | 0ac64 |
| a57d0 | a57c3 |
| a59e0 | a59dc |
| b1df0 | b1dec |
| b1e00 | b1dff |

Our objects declare DWORD _BSS (OMF align 5), and WLINK packs them to 4 bytes. The dataown link therefore loses
8 + 12 + 12 + 4 + 4 bytes. Under dword alignment, the chance of this pattern is about (1/4)^6. The original objects or
link must have para-aligned _BSS. The _DATA and CONST contributions are not para-aligned (02364, 023dc). Because the
cause is open, no layout claim here depends on it.

**MENUES file_id_record (HYPOTHESIS).** The 500-byte initialised record at 2:0291c-02b10 lies after SOUNDS' _DATA and
before GAMEDEF's keyword table. By link order, it is either MENUES' _DATA or a prefix of GAMEDEF's. Only GAME's
check_file_id reads it.

**GAMEDEF (STRONG).** GAMEDEF's _BSS runs from b1df0 to b1dff, after MENUES' ripple state (which ends at b1dec,
followed by the para gap). The oracle order needs hit_sprite on the same symbol page as line_number and total_lines,
and checking_ifelse on cached_cansee's page. gamedef.h now declares them that way.

## Unresolved and negative results

- **`SoundToggle` and `camerasprite` in SOUNDS stay unresolved.** Writing them as `userdefs.sound` and
  `userdefs.camera` gives the same addresses and widths but changes register allocation:
  - xyzsound keeps the sound number in ESI instead of spilling it;
  - pan3dsound swaps EDI and EBP.

  Both functions lose STRICT. Moving the `userdefs` extern into the slot of `camerasprite` in sounds.h did not help.
  `alias_test.py` shows that every other field or alias macro (SECTOR, PLAYER, ACTORS, MENUES, GAMEDEF, GAME) is
  neutral. OPEN: how the original SOUNDS reached these two fields.
- **GAME CONST as literals (`src/game_lit`) does not verify.** CONST is 1664 bytes, MISMATCH at +290:
  - our build coalesces the duplicate "Unknown command line paramater" message in checkcommandline;
  - our build encounters `L?.MAP` before the unknown-parameter message, while the image has it after
    `I/O path rerouted`;
  - the second object's duplicates are coalesced too.

  Code bytes stay equal, but CONST is unverified. Nine functions drop from STRICT to CODE_BYTES_MATCH or NEAR:
  gameexit, loadlookup, loadplayer, read_demo, read_userdefs, saveplayer, show_help, write_userdefs and
  check_file_id. The delivered `src/game/game.c` therefore keeps those 56 names extern.
- **GLOBAL declaration order is COMPILER_STEERED.** It uses nine symbol pages, each holding one hash-ascending run,
  and 115 real game prototypes as zero-size fillers (`gen_global.py`). Three placeholders have no reference:
  `unref_9d430` (16384 bytes), `unref_a539e` (2 bytes) and `unref_a5778` (a 15-byte descriptor). `input_text` is
  modelled as 80 bytes.
- **The registry conflicts on the velocity names.** game2 binds strafe at a52fc and forward at a5300. player binds
  vel and svel to the same addresses with the opposite meanings. GLOBAL names them avel/vel/svel by address only.

## Registry change proposed (integrator)

1. Remove these second-spelling bindings from config/symbols.json; they name fields or duplicates: turn_velocity,
   strafe_velocity, forward_velocity, menupage, user_visibility, SoundToggle, disable_armor, sound_volume,
   music_volume, gamepaused, transition_zoom, camerasprite, facing_sprite, mouse_sensitivity, MusicToggle.
   `match_pruned.py` runs the unmodified matcher with exactly this change.
2. Add the four bindings from `overlay.json`:
   - `level_tick_state` at 2:a57c0 (enterlevel's byte store);
   - three layout-only placeholders.
3. Register GLOBAL as a unit with no functions (profile game), linked after ACTORS or after PLAYER.

## Files

- **Sources:** `src/<unit>/<unit>.c` plus headers for game, sector, player, actors, sounds, menues, gamedef and
  global; also `src/game_lit/game.c`.
- **Generators** (re-run them against the current src/game):
  - `xform_game.py [--literals]`
  - `xform_units.py`
  - `gen_menues.py`
  - `gen_global.py`, with its inputs `global_items.py`, `global_head.c` and `global_tail.c`
- **Tools:**
  - `refs.py`: oracle references into an obj2 range
  - `objpub.py`
  - `alias_test.py`
  - `match_pruned.py`
  - `link_dataown.py`
- **Verifier outputs:** `results/<unit>.{canonical,match,pruned}.txt` and `results/game_lit.match.txt`.
- **Links:** `link/run` (58 undefined) and `link/run_literals` (2 undefined).

Commands, run from the repository root:

```
python -B tools/match.py --profile game --symbols build/workers/dataown/overlay.json "-i=build/workers/dataown/src/<unit>" build/workers/dataown/src/<unit>/<unit>.c
python -B build/workers/dataown/match_pruned.py <same arguments>    # for GLOBAL
python -B build/workers/dataown/link_dataown.py [--literals]
```

## Full table (152 names of the baseline link)

| Symbol | Bound | Referenced by | Owner (segment +offset) | Confidence | Definition / note |
|---|---|---|---|---|---|
| `exit_banner` | 2:00004 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `lookup_filename` | 2:000f8 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `lookup_missing` | 2:00104 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `warp_title` | 2:00120 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `warp_episode` | 2:0012c | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `warp_level` | 2:00134 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `help_banner` | 2:0013c | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `help_usage` | 2:00168 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `help_help` | 2:0017c | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `help_level` | 2:0019c | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `help_volume` | 2:001b8 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `help_skill` | 2:001d4 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `help_quotes` | 2:001f0 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `help_debug` | 2:00210 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `help_record` | 2:00234 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `help_output` | 2:00250 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `help_encode` | 2:00278 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `unknown_parameter` | 2:00294 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `demorec_on` | 2:002bc | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `output_rerouted` | 2:002d0 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `encode_mapname` | 2:002ec | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `encode_message` | 2:002f4 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `unknown_parameter2` | 2:00304 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `main_banner` | 2:0032c | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `main_loading_stats` | 2:00420 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `main_init_cache` | 2:00438 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `main_checking` | 2:00454 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `main_no_good` | 2:00474 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `main_init_engine` | 2:00480 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `main_multiplayer` | 2:00490 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `main_loading_art` | 2:004ac | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `main_tiles_file` | 2:004bc | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `main_init_remaps` | 2:004cc | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `main_init_fx` | 2:004e4 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `main_reading_sounds` | 2:004f8 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `main_init_music` | 2:0050c | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `main_locating` | 2:0051c | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `main_master` | 2:00538 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `main_slave` | 2:00540 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `main_game_over` | 2:00548 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `bonus_press_space` | 2:00560 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `bonus_title` | 2:00578 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `bonus_secrets` | 2:00580 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `bonus_percent` | 2:00594 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `bonus_points` | 2:0059c | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `bonus_squashed` | 2:005a4 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `bonus_killed` | 2:005b8 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `score_rank_title` | 2:005c8 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `score_quote_title` | 2:005cc | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `score_score_title` | 2:005dc | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `userdefs_filename` | 2:005e4 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `savegame_filename` | 2:005f4 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `saveboard_filename` | 2:00604 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `saveboard_missing` | 2:00614 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `demo_filename` | 2:00628 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `demo_missing` | 2:00638 | game | GAME CONST | STRONG | string literal in the GAME pool 2:00004-00648 (macro spelling in src/game_lit only) |
| `bonus_marks` | 2:02304 | game | GAME _DATA +0 | STRONG | `short bonus_marks[3][16]` initialised; 96-byte _DATA VERIFIED at 2:02304 |
| `score_names` | 2:023dc | game | GLOBAL _DATA +0 | STRONG | char score_names[12][33] |
| `high_scores` | 2:02568 | game | GLOBAL _DATA +18c | STRONG | long high_scores[12] |
| `file_id_record` | 2:0291c | game | MENUES _DATA +0 | HYPOTHESIS | `char file_id_record[500]` initialised; 500-byte _DATA VERIFIED at 2:0291c |
| `rmbuffer` | 2:02cfa | multi | (other workers: ENGINE / MULTI) | - | out of scope; already resolved in the latest canonical sources |
| `level_warp` | 2:0ac56 | game | GAME _BSS +6 | STRONG | `char level_warp;` after oldtimerhandler; 7-byte _BSS VERIFIED at 2:0ac50 |
| `mspos` | 2:0ac60 | game | second GAME-range object (_BSS 2:0ac60); hosted in SECTOR | HYPOTHESIS | `long mspos;` in sector.c; 4-byte _BSS VERIFIED (layout) at 2:0ac60 |
| `music_files` | 2:0ac70 | game,actors,gamedef | GLOBAL _BSS +0 | STRONG | char music_files[4][10][13] |
| `actorscrptr` | 2:0ae78 | game,player,gamedef | GLOBAL _BSS +208 | STRONG | long *actorscrptr[4096] |
| `output_path` | 2:0ee78 | game | GLOBAL _BSS +4208 | STRONG | char output_path[80] |
| `demo_avel` | 2:0f6c8 | game | GLOBAL _BSS +4a58 | STRONG | signed char demo_avel[16384] |
| `demo_vel` | 2:136c8 | game | GLOBAL _BSS +8a58 | STRONG | signed char demo_vel[16384] |
| `script` | 2:176c8 | gamedef | GLOBAL _BSS +ca58 | STRONG | long script[6144] |
| `playerreadyflag` | 2:1d6c8 | game | GLOBAL _BSS +12a58 | STRONG | char playerreadyflag[16] |
| `demo_bits` | 2:1d6d8 | game | GLOBAL _BSS +12a68 | STRONG | long demo_bits[16384] |
| `demo_tics` | 2:2d6d8 | game | GLOBAL _BSS +22a68 | STRONG | unsigned char demo_tics[16384] |
| `demo_svel` | 2:316d8 | game | GLOBAL _BSS +26a68 | STRONG | signed char demo_svel[16384] |
| `sound_file_table` | 2:356d8 | sounds | GLOBAL _BSS +2aa68 | STRONG | struct sound_file sound_file_table[39] |
| `music_buffer` | 2:35810 | sounds | GLOBAL _BSS +2aba0 | STRONG | unsigned char music_buffer[40000] |
| `bootleg_cache` | 2:3f450 | sounds | GLOBAL _BSS +347e0 | STRONG | char bootleg_cache[300000] |
| `ps` | 2:88830 | game,sector,player,actors,sounds,menues,gamedef | GLOBAL _BSS +7dbc0 | STRONG | struct player_view ps[17] |
| `hittype` | 2:893e0 | game,sector,actors,gamedef | GLOBAL _BSS +7e770 | STRONG | long hittype[4096][5] |
| `input_text` | 2:9d3e0 | game | GLOBAL _BSS +92770 | STRONG | unsigned char input_text[80] |
| `tempbuf` | 2:a4830 | game,gamedef | GLOBAL _BSS +99bc0 | STRONG | char tempbuf[1024] |
| `textptr` | 2:a5230 | gamedef | GLOBAL _BSS +9a5c0 | STRONG | char *textptr |
| `insptr` | 2:a5234 | gamedef | GLOBAL _BSS +9a5c4 | STRONG | long *insptr |
| `labelcnt` | 2:a5238 | gamedef | GLOBAL _BSS +9a5c8 | STRONG | long labelcnt |
| `label` | 2:a523c | gamedef | GLOBAL _BSS +9a5cc | STRONG | char *label |
| `actor_header` | 2:a5240 | gamedef | GLOBAL _BSS +9a5d0 | STRONG | long *actor_header |
| `labelcode` | 2:a5244 | gamedef | GLOBAL _BSS +9a5d4 | STRONG | long *labelcode |
| `msgtext` | 2:a5248 | game | GLOBAL _BSS +9a5d8 | STRONG | unsigned char *msgtext |
| `scriptptr` | 2:a524c | gamedef | GLOBAL _BSS +9a5dc | STRONG | long *scriptptr |
| `reccnt` | 2:a5254 | game | GLOBAL _BSS +9a5e4 | STRONG | long reccnt |
| `demomode` | 2:a5258 | game | GLOBAL _BSS +9a5e8 | STRONG | long demomode |
| `pending_bits` | 2:a525c | game | GLOBAL _BSS +9a5ec | STRONG | long pending_bits[17] |
| `player_bits` | 2:a52a0 | game,sector,player,actors,menues,gamedef | GLOBAL _BSS +9a630 | STRONG | long player_bits[17] |
| `fx_device` | 2:a52e4 | game | GLOBAL _BSS +9a674 | STRONG | struct fx_device fx_device |
| `current_menu` | 2:a52f0 | game,menues | GLOBAL _BSS +9a680 | STRONG | long current_menu |
| `menupage` | 2:a52f0 | sector | GLOBAL current_menu | STRONG | second spelling; macro after the old extern |
| `neartaghitdist` | 2:a52f4 | sector | GLOBAL _BSS +9a684 | STRONG | long neartaghitdist |
| `avel` | 2:a52f8 | player | GLOBAL _BSS +9a688 | STRONG | long avel |
| `turn_velocity` | 2:a52f8 | game | GLOBAL avel | STRONG | second spelling; macro after the old extern |
| `strafe_velocity` | 2:a52fc | game | GLOBAL vel | STRONG | second spelling; macro after the old extern |
| `vel` | 2:a52fc | player | GLOBAL _BSS +9a68c | STRONG | long vel |
| `forward_velocity` | 2:a5300 | game | GLOBAL svel | STRONG | second spelling; macro after the old extern |
| `svel` | 2:a5300 | player | GLOBAL _BSS +9a690 | STRONG | long svel |
| `lockclock` | 2:a5304 | game,player | GLOBAL _BSS +9a694 | STRONG | long lockclock |
| `env_music` | 2:a5308 | actors,gamedef | GLOBAL _BSS +9a698 | STRONG | char env_music[10][13] |
| `viewscreen` | 2:a538a | game,sector,actors | GLOBAL _BSS +9a71a | STRONG | short viewscreen |
| `screenpeek` | 2:a538c | game,sector,player,actors,sounds,menues | GLOBAL _BSS +9a71c | STRONG | short screenpeek |
| `msgtime` | 2:a538e | game | GLOBAL _BSS +9a71e | STRONG | short msgtime |
| `hudpulse` | 2:a5390 | game | GLOBAL _BSS +9a720 | STRONG | short hudpulse |
| `menu_mouse` | 2:a5394 | game,menues | GLOBAL _BSS +9a724 | STRONG | short menu_mouse |
| `menu_previous_buttons` | 2:a5396 | game | GLOBAL _BSS +9a726 | STRONG | short menu_previous_buttons |
| `menu_drag` | 2:a5398 | game,player,menues | GLOBAL _BSS +9a728 | STRONG | short menu_drag |
| `menu_x` | 2:a539a | game,player,menues | GLOBAL _BSS +9a72a | STRONG | short menu_x |
| `menu_y` | 2:a539c | game,player,menues | GLOBAL _BSS +9a72c | STRONG | short menu_y |
| `randomstash` | 2:a53a0 | game,actors | GLOBAL _BSS +9a730 | STRONG | short randomstash |
| `neartagsprite` | 2:a53a2 | sector | GLOBAL _BSS +9a732 | STRONG | short neartagsprite |
| `neartagsector` | 2:a53a4 | sector | GLOBAL _BSS +9a734 | STRONG | short neartagsector |
| `neartagwall` | 2:a53a6 | sector | GLOBAL _BSS +9a736 | STRONG | short neartagwall |
| `pending_avel` | 2:a53aa | game | GLOBAL _BSS +9a73a | STRONG | signed char pending_avel[17] |
| `player_avel` | 2:a53bb | game,player | GLOBAL _BSS +9a74b | STRONG | signed char player_avel[17] |
| `pending_vel` | 2:a53cc | game | GLOBAL _BSS +9a75c | STRONG | signed char pending_vel[17] |
| `player_vel` | 2:a53dd | game,player,gamedef | GLOBAL _BSS +9a76d | STRONG | signed char player_vel[17] |
| `pending_svel` | 2:a53ee | game | GLOBAL _BSS +9a77e | STRONG | signed char pending_svel[17] |
| `player_svel` | 2:a53ff | game,player,gamedef | GLOBAL _BSS +9a78f | STRONG | signed char player_svel[17] |
| `sound_voices` | 2:a5420 | game,sounds | GLOBAL _BSS +9a7b0 | STRONG | struct sound_voice sound_voices[39] |
| `user_visibility` | 2:a57a8 | player | GLOBAL userdefs.visibility (+0) | STRONG | second spelling; macro after the old extern |
| `userdefs` | 2:a57a8 | game | GLOBAL _BSS +9ab38 | STRONG | struct user_settings userdefs |
| `SoundToggle` | 2:a57ab | sounds | GLOBAL userdefs.sound (+3) | STRONG | second spelling; macro after the old extern; LEFT UNRESOLVED in SOUNDS (field spelling costs xyzsound/pan3dsound STRICT) |
| `disable_armor` | 2:a57ad | game,sector,player,actors,gamedef | GLOBAL userdefs.debug (+5) | STRONG | second spelling; macro after the old extern |
| `sound_volume` | 2:a57af | player,menues | GLOBAL userdefs.sound_volume (+7) | STRONG | second spelling; macro after the old extern |
| `music_volume` | 2:a57b1 | actors,menues | GLOBAL userdefs.music_volume (+9) | STRONG | second spelling; macro after the old extern |
| `gamepaused` | 2:a57b3 | game,sector | GLOBAL userdefs.paused (+0xb) | STRONG | second spelling; macro after the old extern |
| `transition_zoom` | 2:a57b3 | menues | GLOBAL userdefs.paused (+0xb) | STRONG | second spelling; macro after the old extern |
| `camerasprite` | 2:a57b5 | game,sounds | GLOBAL userdefs.camera (+0xd) | STRONG | second spelling; macro after the old extern; LEFT UNRESOLVED in SOUNDS (field spelling costs xyzsound/pan3dsound STRICT) |
| `facing_sprite` | 2:a57b5 | game,gamedef | GLOBAL userdefs.camera (+0xd) | STRONG | second spelling; macro after the old extern |
| `mouse_sensitivity` | 2:a57b7 | player,menues | GLOBAL userdefs.mouse_sensitivity (+0xf) | STRONG | second spelling; macro after the old extern |
| `warning` | 2:a57bd | gamedef | GLOBAL _BSS +9ab4d | STRONG | unsigned char warning |
| `error` | 2:a57be | gamedef | GLOBAL _BSS +9ab4e | STRONG | unsigned char error |
| `killit_flag` | 2:a57bf | gamedef | GLOBAL _BSS +9ab4f | STRONG | char killit_flag |
| `level_tick_state` | 2:a57c0 | game | GLOBAL _BSS +9ab50 | STRONG | unsigned char level_tick_state |
| `script_tics` | 2:a57c1 | game,player,actors,gamedef | GLOBAL _BSS +9ab51 | STRONG | unsigned char script_tics |
| `input_text_length` | 2:a57c2 | game | GLOBAL _BSS +9ab52 | STRONG | unsigned char input_text_length |
| `last_used_text` | 2:b1df0 | gamedef | GAMEDEF _BSS | STRONG | defined in gamedef.c, header order adjusted; 15-byte _BSS VERIFIED at 2:b1df0 |
| `cached_cansee` | 2:b1df4 | gamedef | GAMEDEF _BSS | STRONG | defined in gamedef.c, header order adjusted; 15-byte _BSS VERIFIED at 2:b1df0 |
| `line_number` | 2:b1df6 | gamedef | GAMEDEF _BSS | STRONG | defined in gamedef.c, header order adjusted; 15-byte _BSS VERIFIED at 2:b1df0 |
| `total_lines` | 2:b1df8 | gamedef | GAMEDEF _BSS | STRONG | defined in gamedef.c, header order adjusted; 15-byte _BSS VERIFIED at 2:b1df0 |
| `hit_sprite` | 2:b1dfa | gamedef | GAMEDEF _BSS | STRONG | defined in gamedef.c, header order adjusted; 15-byte _BSS VERIFIED at 2:b1df0 |
| `checking_ifelse` | 2:b1dfc | gamedef | GAMEDEF _BSS | STRONG | defined in gamedef.c, header order adjusted; 15-byte _BSS VERIFIED at 2:b1df0 |
| `brace_depth` | 2:b1dfd | gamedef | GAMEDEF _BSS | STRONG | defined in gamedef.c, header order adjusted; 15-byte _BSS VERIFIED at 2:b1df0 |
| `parsing_state` | 2:b1dfe | gamedef | GAMEDEF _BSS | STRONG | defined in gamedef.c, header order adjusted; 15-byte _BSS VERIFIED at 2:b1df0 |
| `MusicToggle` | 2:1bde42 | sounds | ENGINE option[2] (2:1bde40+2) | STRONG | second spelling; macro after the old extern |
| `ipxcallback` | unbound | multi | (other workers: ENGINE / MULTI) | - | out of scope; already resolved in the latest canonical sources |
| `printext16_` | unbound | engine | (other workers: ENGINE / MULTI) | - | out of scope; already resolved in the latest canonical sources |
