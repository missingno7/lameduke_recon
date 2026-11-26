# gametune: callee-side tuning of GAME (game_game) and PREMAP (game_premap) under game_oe

Toolchain: wc95b profile `game_oe` (`-4r -s -otexan -oe=3000`), stock `tools/match.py`, no overlay.
Exploration on LAMEDUKE_HOST=nt; final numbers on the default DOS host (`res_dos_game.*`, `res_dos_premap.*`).
Base: copies of src/game/{game.c,premap.c,game.h,build_v5.h} at task start (`base_dos_*.txt/json`).

```
python tools/match.py --profile game_oe "-i=build/workers/gametune/src" build/workers/gametune/src/game.c
python tools/match.py --profile game_oe "-i=build/workers/gametune/src" build/workers/gametune/src/premap.c
```

## Result (DOS host)
| object | STRICT before -> after | NEAR byte diffs before -> after | data |
|---|---|---|---|
| game.c | 26/49 -> **27/49** (11,856 -> 13,824 B) | 28,311 -> 25,056 | CONST 1604 @2:00004, _DATA @2:02304, _BSS @2:0ac50 VERIFIED |
| premap.c | 4/12 -> 4/12 (1,533 B) | 14,599 -> 14,580 | CONST 96 @2:00648, _BSS @2:0ac60 VERIFIED |
| total | 30 -> **31** | 42,910 -> 39,636 | |

No STRICT function lost. All function sizes unchanged versus the base (strget +16, playback_demo -16,
displayrooms +48, moveeffectors and prelevel are pre-existing size errors).

Integrator note (2026-11-26) applied: `init_water_ripples` -> `initlava`; strget body taken from
build/workers/kengame (Ken's getinput typing loop).

## Per function (DOS byte diffs; ibad = unequal instructions, NT, `gt.py tab`)
| function | bytes before -> after | ibad before -> after | change |
|---|---|---|---|
| domovethings | 1134 -> **STRICT** | 56 -> 0 | `short p` loop index (oracle `mov bx,[connecthead]`), and move_status9's `sprite[i].z == sector[sect].floorz` operand order |
| playback_demo | 1897 -> 1208 | 67 -> 10 | via domovethings inline (`short p`). Left: one displaystatus-copy loop (below) and the -16 size it causes |
| domovethings_multiplayer | 383 -> 19 | 16 -> 7 | via domovethings inline |
| main | 4273 -> 3353 | 225 -> 188 | via inlines; `check_file_id` now called without a prototype (implicit int: oracle `test eax,eax`) |
| strget | 725 -> 629 | 225 -> 151 | kengame's Ken-form loop |
| network_move_loop | 1609 -> 1604 | 97 -> 72 | via domovethings inline |
| checkcommandline | 59 -> 14 | 14 -> 8 | `i++` moved to the end of the `if (*c == '/')` branch (block order: oracle puts `inc edx; jmp` straight after the switch and the else/exit block after it) |
| move_status9 | 24 -> 22 | 23 -> 21 | operand order `sprite[i].z == sector[sect].floorz` |
| nonsharedkeys | 518 -> 518 | 37 -> 37 | **SOURCE fix**: the display-mode toggle calls `qsetmode640350` (oracle 1:43f42, bound name), not qsetmode640480; removes a target problem |
| resetplayer (premap) | 93 -> 133 | 38 -> 15 | Ken-style separate stores `jumping_counter = 0; poszv = 0;` and oracle store order state80, flash_sprite, newowner, hard_landing |
| reset_player_motion (premap) | 255 -> 196 | 157 -> 174 | via resetplayer inline |

## Levers found (natural C, game_oe)
- **Loop index width drives inline copies**: `short p` in domovethings fixed the standalone and every copy
  (playback_demo, network_move_loop, domovethings_multiplayer/wait_for_players paths).
- **Comparison operand order** selects which operand is loaded (move_status9 / domovethings copy).
- **Statement placement controls block layout**: `i++` inside the then-branch of checkcommandline.
- **Wrong callee**: nonsharedkeys qsetmode640350 (semir SOURCE class found it).
- **Implicit declaration** reproduces int-returning call sites of a char-returning function in another object
  (check_file_id in main).
- semir classification of the rest (`semir_game.txt`): mostly ALLOCATION-ONLY; SOURCE leads left:
  spawn_random_effects (EGS copy store/arg order), displayhighscores (numtext copy: oracle materialises shade
  before ltoa, y after), wait_for_players/moveloop/network_move_loop (domovethings_multiplayer copy bit tests).

## Negative / not adopted (NT host unless noted)
- **next_text_shade `hudpulse += 128; return (sintable[hudpulse & 2047] >> 10) + 16;`** keeps the
  standalone STRICT and gives the oracle's early hudpulse store in the inline copies (draw_monitor_overlay,
  strget-in-main, dobonus), ibad -121 overall; but dobonus and main become -16 bytes and draw_monitor_overlay's
  3rd draw_weapon_tile copy changes registers (5 -> 293 bytes). Reverted to keep sizes exact; it is the right
  semantic shape, the remaining blocker is allocation in the copies.
- **playback_demo**: only diff is the batteryammo meter loop of the displaystatus copy (oracle `mov edx,eax;
  shl edx,4` / i in eax). A switch temp (`short i, w; w = ps[snum].curr_weapon; switch (w)`) makes
  playback_demo ibad 0 and size exact, but moves displaystatus's numtext_fixed stack homes (+9, loses STRICT).
  32 comparison-direction combos, casts on the switch, block-scoped/`register`/other-typed `w`, w/numtext_fixed
  declaration-order sweeps (16) and caller-side playback_demo spellings all fail to keep both.
- **wait_for_players**: reversing all four bit-test operand orders in domovethings_multiplayer case 2
  (`tempbuf[k>>3] & (N << (k&7))`) gives wait_for_players ibad 0 (standalone unchanged), but moveloop +16 bytes
  (breaks draw_meter's animatesprites jump table) and network_move_loop ibad 72 -> 990. Best partial: tests
  2-4 reversed, wait_for_players 32 -> 7, network_move_loop +16 ibad. Not adopted.
- **gametext**: oracle prologue = `x` is the parameter itself (`mov edi,eax` first). With `long gametext(long x,
  ...)` numtext_relative becomes ibad 0, but gametext's body/return path changes (return via edx) and strget
  grows +16 (draw_meter loses STRICT). Return/param type variants (int/short) no better.
- **strget**: `short x, short y` (oracle reads both as words) with an `int strget();` declaration in game.h
  keeps getscorename STRICT but turns the cached-zero `bl` into a cached `0x3c` and costs +32..+48 bytes;
  `ch` temp, `> 0` shift test, casts `(short)x` in the minitext call: all worse than kengame's form.
- **write_demo**: `if (level == 0 && volume == 0) filename[7]='0'; else '1';` reproduces the oracle branch
  shape (ibad 12 -> 7) but is +16 bytes; the remaining `sub [reccnt],edx` RMW split by the scheduler did not
  respond to `--reccnt`, `-= 1`, if-block form. Reverted.
- **checkcommandline**: remaining 'o' char (oracle cl, ours bh) and 'l' constant 1 (ch vs cl): loop
  spellings, declaration orders and removing `mapname` do not move them; cg95 explain: no unique anchor.
- **newgame**: constant 1 for state9c[1] in cl (ours) vs bl (oracle); not resolved.
- **resetplayer**: 14 chained-assignment combos tested; none below ibad 15.
- **draw_monitor_overlay**, **displayquotes**, **move_status9**: ALLOCATION-ONLY; declaration/type/operand
  variants listed in v/ did not help.
- `keystatus` non-volatile: worse (timerhandler/update_timer_input lose).

## Tools (this directory)
- `gt.py tab|diff|try`: NT per-function size/ibad, instruction diff, and multi-file variant runner
  (variant files in `v/`; subs may target game.h/premap.c).
- `apply.py VARFILE NAME`, `sbs.py SRC FUNC START END` (side-by-side disassembly), `chk.py [dos] [json]`
  (both objects, STRICT lost/new vs base), `g4.py` (from oeprobe, profile game_oe).
- `cgt.py SRC FUNC TAG`: runs tools/cg95 tracer + explainer with WORKER redirected into `cg/` (writing to
  build/cg95 was refused by the permission system).
- `v/game_ckpt*.c`: checkpoints.

## Suggested next steps
- displaystatus: find a spelling that keeps standalone stack homes while shifting the playback copy
  (playback_demo would be STRICT, 3184 B).
- domovethings_multiplayer copies: understand why test-1 operand order changes moveloop size.
- spawn_random_effects / displayhighscores: SOURCE leads from semir (argument evaluation order of inlined
  EGS / numtext).
