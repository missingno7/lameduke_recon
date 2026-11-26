# unify worker: one record definition, comments, safe renames (2026-11-26)

Deliverables: `build/workers/unify/src/game/*` and `build/workers/unify/src/global/global.c` are complete
replacement files for `src/game/` and `src/global/`; `src/game/records.h` is new. Nothing under `src/` or
`config/` was edited. `orig/` is the integrated tree copied before any edit (baseline).

## Result

Every unit compiles to the same object as the integrated tree. Canonical DOS host, unit profiles from
config/units.json (game_oe for the eight game units, game for GLOBAL):

| unit | segments + fixups + publics (cmp.py) | objcmp.py --funcs | match.py output vs baseline |
|---|---|---|---|
| sounds | identical | 13/13 functions, 1931/1931 bytes | identical: STRICT 12/13, CONST/_DATA/_BSS VERIFIED |
| gamedef | identical | 14/14, 20905/20905 | identical: STRICT 9/14, CONST/_DATA/_BSS VERIFIED |
| menues | identical | 11/11, 9434/9434 | identical: STRICT 8/11, CONST/_DATA/_BSS VERIFIED |
| sector | identical | 18/18, 41150/41150 | identical: STRICT 9/18 |
| player | identical | 8/8, 24106/24106 | identical: STRICT 2/8, _DATA VERIFIED |
| game | identical | 49/49, 61779/61779 | identical: STRICT 27/49, CONST/_DATA/_BSS VERIFIED |
| premap | identical | 12/12, 24109/24109 | identical: STRICT 4/12, CONST/_BSS VERIFIED |
| actors | identical | 18/18, 26980/26980 | identical: STRICT 10/18 |
| global | identical | (no code) | identical: _DATA 1116 B, _BSS 633683 B VERIFIED |

All segments (_TEXT, CONST, CONST2, _DATA, _BSS) have equal bytes and equal normalized fixups, and every public
(name, segment, offset, so every _BSS offset) is unchanged; the extern sets are unchanged. Raw object files
differ only in the dependency COMENT list (one more entry, `records.h`) and in the zero-filled class-0 COMENT
filler records the writer emits after it (their lengths shift by the 23 bytes of the new entry; in GAME the
filler shift moves later record boundaries, LEDATA contents unchanged). `new/rawcmp.txt` has the record counts.

Reproduce (from the repo root):

    python build/workers/unify/cmp.py --base          # baseline objects from orig/ (once)
    python build/workers/unify/cmp.py                 # compile src/ and compare every unit (DOS host)
    python tools/match.py --profile game_oe "-i=build/workers/unify/src/game" build/workers/unify/src/game/sector.c
    python tools/match.py --profile game "-i=build/workers/unify/src/global" "-i=build/workers/unify/src/game" build/workers/unify/src/global/global.c
    python tools/objcmp.py build/workers/unify/obj/dos/orig/game.obj build/workers/unify/obj/dos/src/game.obj --funcs

Saved outputs: `base/match_*.txt` (integrated tree), `new/match_*.txt`, `new/objcmp_*.txt`, `new/rawcmp.txt`.
tools/verify.py was not run because it writes build/verify.json and build/verify/ outside the worker directory;
per-unit match.py on the integrated tree (`base/`) is the same check.

## 1. One record definition: src/game/records.h

records.h holds only struct tags and macros, which take no Watcom 9.5b symbol slot (docs/BSS_LAYOUT.md rule 1),
so it can be included anywhere. All slot-taking declarations (typedefs, externs, prototypes) stay where they
were, with the same count and order in every unit. Contents:

- `struct sectortype`, `struct walltype`, `struct spritetype`: map v5, field names/types after Ken's Oct-94
  src/engine/BUILD.H (`unsigned short wallptr, wallnum`; `nextsector1/nextwall1`).
- `struct player_struct` (176 bytes, field names from docs/RENAMES.md Priority 3 incl. 0.2/0.4, each field
  commented with its offset and meaning from config/semantics.json).
- `struct user_defs` (ud, 21 bytes; fields visibility, quotes, brightness, SoundToggle, MusicToggle, debug,
  sound_volume, music_volume, gamepaused, camerasprite, mouse_sensitivity, reserved[4], per semantics.json).
- `struct menu`, `struct sound_file`, `struct sound_voice`, `struct animwalltype`, `struct fx_device`.
- Macros: MODE_* (ps.gm bits), STAT_* (status lists), SB_* (sync bits, semantics.json `sync_bits`).

Views removed (all now `struct player_struct` / the records.h tags):

| file | old view(s) |
|---|---|
| sector.h | typedef'd anonymous spritetype/sectortype/walltype/playertype/animwalltype -> `typedef struct X X;` (5 typedefs kept, same order) |
| actors.h | anonymous spritetype/sectortype/playertype -> typedefs of the tags (3 kept) |
| player.h | struct playerstate, spritetype, sectortype |
| gamedef.h | struct spritetype, sectortype, playerstate (prefix[10]/motion[32]/view[10]/... padding view) |
| menues.h | struct menu, menu_player (prefix[2]/movement[62]/controls[62]...), menu_sector (prefix[29]/floorpal/tags[7]) |
| sounds.h | struct sound_file, sound_voice, sound_sprite, sound_player (fields_before_pos[8]...) |
| build_v5.h | struct spritetype/sectortype/walltype (typedef size checks and externs kept) |
| game.c, premap.c | struct user_settings, player_view, game_sound_voice, animwall_entry, anonymous fx_device |
| global.c | struct player_view, user_settings, sound_file, sound_voice, menu, animwall_entry, fx_device |
| sector/player/actors/gamedef/menues/sounds .c | struct user_settings (6 copies) |

Access sites were renamed by byte offset (`rewrite.py`: every `ps[...]` / `pl->` access mapped from the unit's
old view to the unified field at the same offset; constant indices recomputed). Notable mappings:
sector.c jetpack_on@0x93 -> scuba_on, scuba_on@0x94 -> jetpack_on, jetpack@0x97 -> inventory[0], scuba@0x98 ->
inventory[1], steroids/shield/holoduke_charge -> inventory[2..4], nightvision_charge -> shield_amount,
gotweapon[0..3]@0x9d -> gotweapon[1..4], weapon_select -> weapon_pos, use_pressed -> toggle_key_flag,
jump_pressed -> knuckle_incs, keys_pressed -> interface_toggle_flag, camera -> newowner, weapon_locked ->
somethingonplayer; gamedef.c horiz@0x48 -> look_ang, poszv@0x5a -> jumping_counter, rammo -> rocket_ammo;
player.c weapon_delay[]/weapon_slot -> fta[]/ftq; previous_x/y/z/ang -> oposx/y/z/oang and opos*/oang/ocursectnum
-> startpos*/startang/startsectnum everywhere (RENAMES 0.2); actors.c kills -> actors_squashed, turret_kills ->
actors_killed, crane -> on_crane, face_parasite -> somethingonplayer; menues.c episode -> volume, game_mode -> gm.
Full list: `rewrite_log.txt`. Wall fields: `.nextsector/.nextwall` -> `.nextsector1/.nextwall1` (game, premap, sector).

ud aliases retired (RENAMES 0.4): the macros disable_armor, gamepaused, camerasprite, facing_sprite,
transition_zoom, user_visibility, sound_volume, music_volume, mouse_sensitivity, menupage and
turn_velocity/strafe_velocity/forward_velocity are gone; code uses `ud.debug`, `ud.gamepaused`,
`ud.camerasprite`, ..., `current_menu`, `angvel/svel/vel`. The now-unused extern declarations of those names
stay in place because they hold symbol slots (each unit's dataown comment says so). sounds.c keeps its bound
externs `camerasprite`/`SoundToggle` (changing them to `ud.x` would change the fixup target names); its
`MusicToggle` macro (really SETUP.DAT option[2], the music card) is renamed `MusicDevice` (Duke 1.5 name).

### Type conflicts found

- **score** (0x36): `unsigned long` in game/premap/global, `long` in player.h/actors.h/gamedef.h. The other units
  only add to it; `unsigned long` compiles identically everywhere (kept, semantics.json: unsigned).
- **music** (0x86): `char *` vs player.h `long music_offset` (unused in player.c): `char *` everywhere.
- **sector wallptr/wallnum: REAL CONFLICT.** SECTOR needs `unsigned short` (signed changes doanimations at
  +0x18 and shifts every later function); GAME's moveeffectors (NEAR_MATCH, 14099 bytes) needs
  signed (unsigned changes 9 wall-loop starts, +32 bytes). Everything else (premap, player, actors, gamedef) is
  indifferent. The oracle cannot settle the field type at those sites: the original moveeffectors loads the
  word with `mov ax,[..]` + `cwde`, i.e. through a short temporary, which also fits an unsigned field.
  Resolution: records.h follows Ken's BUILD.H (`unsigned short`), and moveeffectors' nine wall-loop starts read
  `j = (short)sector[s].wallptr` (commented COMPILER_STEERED in game.c). Better fix for the moveeffectors owner:
  short loop variables like the original, which also targets its remaining NEAR differences.
- No other field needs a different type in any unit (cstat `char` vs `unsigned char` is the same type under
  Watcom's default unsigned char).

### Not unified

- **hittype**: GAMEDEF views a row as `struct actorstate {movecount, move, actioncount, actionframe, action}`
  (kept, RENAMES Priority 4); all other units index `long hittype[4096][5]`. Unifying means rewriting every
  `hittype[i][k]` and `t = &hittype[i][0]` site with casts; left as is (struct commented in gamedef.h).
- **Function prototypes still disagree between units** (outside the task, but they change code, so they cannot be
  unified byte-neutrally without per-call checks): spritesound (short|long|int sound), stopsound (short|long),
  getincangle (4 signatures), getangle (int|long|short), shoot (2 or 3 params), spawn (void vs short),
  myos (5 type mixes), saveplayer (char vs long), checkhitswitch (short vs long), checkhitceiling (char vs long),
  EGS (short vs long ang/xvel/zvel, short vs signed char stat).

## 2. Comments (docs/COMMENTS.md)

`comments.py` inserted the eight unit header blocks and 150 function header lines (game 50, premap 13, sector 19,
player 9, actors 19, sounds 14, menues 12, gamedef 15); an existing note directly above a function stays under
the new line, dataown blocks stay separate. Skipped: the `*_text_end` / `operatesectors_table` pseudo-entries
(no function). The premap header was updated for the startpos*/opos* rename. Statement-level bullets were not
inserted (they need hand placement). The DOS-host CONST pools are unchanged after the comments.

## 3. Priority 5/6 renames

- Priority 6 (`locals.py`): 185 parameter/local renames from semantics.json `functions[*].params/locals`, done
  per function body, never on member names or literals. Skipped automatically (collision with a name already in
  the function, or a macro body that names the caller's local): gametext sx->x, findplayer sn->i/distance->x,
  checkhitswitch j->x, shoot distance->x, hitradius k->x/hit->k, movefta/moveactors/movefx/movestandables
  dist|d->x, spritesound number->num/source->i, keyword p->temptextptr (PARSE_ARGS); EGS's 14 parameter renames
  were skipped on purpose (TEXT_ROW names `y`, a partial rename would mix styles). Log: `locals_log.txt`.
  Comments inside function bodies still use some old local names.
- Priority 5: MODE_* on every `ps[].gm` test/assignment and `enterlevel(MODE_GAME|MODE_DEMO)` (44 sites),
  STAT_* on `headspritestat[n]`, `.statnum ==/!= n` and `changespritestat(x, n)` (91 sites), SB_* on single-bit
  `syncbits[x] & bit` tests (63 sites). All #defines (no enums). Tile literals (DEFS.CON names) were not
  replaced; units' existing tile macros are unchanged.

## Notes for the integrator

- Copy `src/game/*` (incl. new records.h) and `src/global/global.c` together; no config/symbols.json change is
  needed (no bound symbol was renamed; only struct members, macros, locals).
- Line endings follow each original file (game.c and several headers CRLF, the rest LF).
- Stale orphan comments left by the GAME/PREMAP split ("The status bar drawing, written out through the helper
  calls." above inittimer/spawn, etc.) were left in place.
