# LameDuke game code vs Ken Silverman's Build demo GAME.C (kengame worker, 2026-11-26)

Ken file abbreviations. Paths are under `external/x/kenbuild-20241213/kenbuild-related-archives-20241213/`. Dates are from FILESLST.TXT.

| Tag | Path | Date |
|---|---|---|
| K94a | `BUIL0801/GAME.C` | 1994-08-01 |
| K94s | `buildold_12_30_94/GAME.C` | 1994-09-01. Only SETUP.DAT is from 12/30. |
| K95 | `buil1995/GAME.C` | 1995-06-12. Also `buil1995/LAVA.C`, the standalone lava demo. |
| E94 | `BUIL0422/ENGINE.C` | Apr-94. The only 1994 engine source. In 1994, setanimation, getanimationgoal, doanimations and the timer lived in the engine. |
| TXT | `buil1995/BUILD.TXT` | Ken's dated change log |

No Ken GAME.C exists between Sep-94 and Jun-95. LameDuke (Dec-94) falls inside that gap.

Our files: `src/game/<unit>.c`. Ken's names come from his source text.

**Confidence levels:**
- **PROVEN** means byte-identical to Ken's text compiled with the LameDuke profile.
- **HIGH** means the same algorithm and statement order.
- **MEDIUM** means recognisably derived code.
- **LOW** means the same role only.

## 1. Method

**Compiles.** All three versions were compiled with Watcom 9.5 GA in the LameDuke game profile:
- flags `-4r -s -otexan`, and also with `-oe=3000`
- host: `LAMEDUKE_HOST=nt python tools/wcc.py --install wc95b`

**Headers for K94s.** K94s needs period headers. `build/workers/kengame/k94s/inc/BUILD.H` is BUIL0801's BUILD.H plus a short labelled block:
- `numplayers`, `myconnectnum`, `connectindex[MAXCONNECTIONS]`, `connecthead`, `connectnum[]`, `connectpoint2[]`, `masterconnectnum`, `myconnectindex`
- `volatile long *lastpacket2clock`
- `spritepal[MAXSPRITES]`
- `NO` (74, from K95 NAMES.H)
- `NUMKEYS` (19) and `keys[NUMKEYS]`

The block was reconstructed from K94a's externs, K95's BUILD.H and src/multi/multi.h. K94a needs only BUIL0801 BUILD.H and NAMES.H plus BUIL0624 SPW_INT.H. K94s then compiles with 0 errors (35,263 bytes of code).

**Scans.** `python tools/libscan.py --split --min 12 OBJ` against obj1.

| Ken version | profile | hits |
|---|---|---|
| K95 | `-oe=3000` | exact: inittimer (1:00020), uninittimer (1:00080), initlava (1:2e120). The remaining hits are anonymous 0x40-byte masked windows. |
| K94a | `-otexan` and `-oe=3000` | 0 hits |
| K94s | `-otexan` and `-oe=3000` | 0 hits |

The 1994 GAME.C has no timer and no lava, and its other routines were rewritten by 3D Realms before Dec-94.

**Semantic comparison.** Each function was read against the three versions. The game, sector and player rows were cross-checked in the code. Two of them were confirmed directly:
- the svel/vel swap: `player.c:753-760` vs K94s:2229-2239
- the SLIME/`waloff != -1` block (section 4)

## 2. Proven by bytes (STRICT with Ken's text)

| LameDuke addr | current name | Ken name (version) | evidence |
|---|---|---|---|
| 1:00020 | inittimer | inittimer (K95:4070) | libscan exact+sites. Already STRICT. |
| 1:00080 | uninittimer | uninittimer (K95:4077) | libscan exact+sites. Already STRICT. |
| 1:2e120 | init_water_ripples | initlava (K95:3846) | libscan exact+sites. Ken's text verbatim (x before y) is STRICT in the candidate. |
| 1:2e200 | animate_water_ripples (also bound as update_ripple_tile) | movelava(char *dapic) (K95:3869) | **New STRICT, 1,248 B**: Ken's text verbatim with LAVAMAXDROPS 16 and floor/base 64 (Ken: 32 and 192), plus Ken's 1994 copybuf pragma (see section 6). TXT 9/29/94: "Added special bitmapped effect which I either call lava or boiling slime. See my initlava and movelava functions inside GAME.C." |

## 3. Semantic map: GAME (`game.c`) and PREMAP (`premap.c`)

| LameDuke addr | current name | Ken counterpart | versions (closest) | conf | ours→Ken params/locals | Ken constants | ours→Ken globals | Ken comments (paraphrase) | evidence |
|---|---|---|---|---|---|---|---|---|---|
| 1:000d0 | update_timer_input | keytimerstuff | K95 only | HIGH | – | – | avel→angvel; vel→svel; svel→vel; keys[0..3,5] = fwd/back/left/right/strafe | keys[5] held = strafe instead of turn | K95:4090-4123; ours:349-415. Differences: arrow keys OR'd in, angvel decays by 10 (Ken 12), no tilt or player-2 block |
| 1:002a0 | timerhandler | timerhandler | K95 | HIGH (STRICT already) | – | – | totalclock | 120 Hz (9942) | K95:4083; ours:426 (keytimerstuff ×6 for option[2]) |
| 1:00c60 | gameexit | inline shutdown in main / prepareboard failure | K94s/K95 | MEDIUM | message ≈ "Board not found" printf | – | option[1] sound, option[2] music, option[4] multiplayer mode | "Set back to text mode" | K94s:327-337; K95:861-871 |
| 1:00d10 | domovethings_multiplayer | getpackets | K94s (types 2/3/4/255); case 5 from K95 | HIGH | other→otherconnectnum; packbufleng→tempbufleng; i,j,k same | MAXPLAYERS | player_svel/vel/avel/bits→syncvel/syncsvel/syncangvel/syncbits; script_tics→synctics; input_text(_length)→getmessage(leng); msgtime→getmessagetimeoff | [2] receive master sync buffer, [3] receive slave sync buffer, [255] logout | K94s:3168-3237; ours:460-530. Bits widened to 4 bytes; 255 sets gm=0x80 where Ken sets keystatus[1] |
| 1:01120 | moveloop | waitforeverybody | K95 only | MEDIUM | clock→oldtotalclock | – | playerreadyflag | – | K95:5044-5086 (Ken uses 2 passes, timeout 8; ours 1 pass, 120) |
| 1:018c0 | network_move_loop | sync | K94s (`synctics > (option[5]<3)`) + K95 faketimerhandler additions | HIGH | i,j,k same; getinput(16)→getinput(MAXPLAYERS) | MAXPLAYERS | pending_*→osync*; player_*→sync* | "I am the MASTER!", "I am a SLAVE" | K94s:3062-3166; ours:568-658 |
| 1:02400 | displayquotes | getmessage display/timeout in drawscreen | K94s | LOW | – | – | msgtext/msgtime→getmessage/getmessagetimeoff | – | K94s:2698-2723 |
| 1:02a40 | strget | typemode branch of getinput | K94a/K94s (`==60`, `+= synctics`) | HIGH (key loop) | scan→i | – | scan_ascii(_shift)→scantoasc(withshift); input_text(_length)→typemessage(leng); script_tics→synctics | "Here's a trick of making key repeat after a 1/2 second" | K94s:2985-3025; ours:790-846. **Ken's `> 0` tests improve bytes (section 5)** |
| 1:02e90 | EGS | spawnsprite macro (K95) / inline insertsprite (94) | K95 | LOW | – | – | – | "parameters are in exact order of sprite structure" | K95:369-386 |
| 1:02fb0 | cacheit | precache | K94s (quotes the engine precache) | LOW | – | MAXSPRITES 4096, MAXSTATUS 1024 | – | "This is my exact precache code in ENGINE.C" | K94s:3478-3501 |
| 1:03760 | loadlookup | makepalookup setup in main | K95 | LOW | – | – | – | – | K95:476-484 |
| 1:03a20 | animatesprites | analyzesprites(short daang) | K94a/K94s | MEDIUM (STRICT already) | ang→daang; ours j/i→Ken i/j | 3200 = 3072+128 | thesprite/spritepicnum/spritepal/spritesortcnt same | "only 5 pictures for 8 angles (3 are x-flipped)" | K94s:1212-1255; ours:1168-1290 |
| 1:03f50 | draw_water_distortion | floor-mirror pass (pink 255 marker) | K95 | LOW | – | – | – | – | K95:2923-2959 |
| 1:04050 | displayrooms | drawscreen | K94a/K94s | MEDIUM | p→snum | – | ps.display_mode (2/3)→dimensionmode[]; ps.mapzoom→zoom[] | draw2dscreen "0 is grid type" | K94s:2646-2674 |
| 1:04cd0 | nonsharedkeys | private-keys part of getinput (V, F12, screenpeek cycling) | K94s; K95 brightness | MEDIUM | – | – | gm bit 16→typemode; screenpeek | – | K94s:2814-2823, 2958-2977 |
| 1:057c0 | wait_for_players | waitplayers block in main | K94s exact formula `(option[4]>=1)&&(option[4]<=4) ? 2 : option[4]-3` | HIGH | numwanted→waitplayers | – | option[4] | – | K94s:206-298 |
| 1:05b60 | main | main | init order K95, loop K94s | MEDIUM | – | – | init_water_ripples→initlava; network_move_loop→sync; domovethings→movethings | – | K95:423-436; K94s:306-322 |
| 1:07890 | displaystatus | setup3dscreen status bar | K94s | LOW | snum | – | screensize/health/score | "Draw status bar only once" | K94s:3384-3396 |
| 1:081c0 | playback_demo | playback | K94a/K94s | HIGH (inner loops) | i | 16384 | demo_*→recsync*; demomode 2 ≈ recstat 0 | – | K94s:3279-3359. Ours lacks `&& i<reccnt` |
| 1:08e30 | record | record block in movethings | K94a/K94s | HIGH (STRICT already) | player→i | 16384/16383 | demo_svel→recsyncvel, demo_vel→recsyncsvel, demo_avel→recsyncangvel, demo_bits→recsyncbits, demo_tics→recsynctics; demomode 1 ≈ recstat 1 | "Can only record 1-player game" | K94s:89-94, 2766-2781 |
| 1:08ea0 | domovethings | movethings (94) / domovethings (95) | K94s (`doanimations(synctics)`, `synctics=0`) | MEDIUM | p→i | – | lockclock; script_tics→synctics | "Move player", "Door code, moving sector code", "Monster / bullet code" | K94s:2740-2798 |
| 1:0ae90 | move_status9 | warp-sector block of tagcode / warp() | K94s/K95 | LOW | – | – | – | "Make sure you don't warp more than once" | K94s:1342-1391 |
| 1:0b630, 1:0b700 | saveplayer, loadplayer | savegame, loadgame | K95 | LOW | – | – | – | – | K95:4179, 4373 |
| 1:0ba30 | moveeffectors | tagcode sector effects (swing/revolve clipinsidebox undo, drag + floor panning) | K94s/K95 | LOW | – | – | – | – | K95:1582-1709 |
| 1:0f160 | spawn (premap) | sprite-scan switch in prepareboard | all | LOW | – | – | – | status 2 = monster active only when it sees you | K94s:931-953 |
| 1:11a10 | render_mirror (premap) | mirror part of drawscreen | K95 | LOW | – | – | – | – | K95:2971-3041 |
| 1:11de0 | resetplayer (premap) | per-player init in prepareboard | K94s/K95 | LOW | p→i | – | horiz 100, zoom 768, health 100 | – | K94s:658-680 |
| 1:11f30 | reset_player_motion (premap) | head of prepareboard | K95 | MEDIUM | p→i | – | randomseed=17, animatecnt=0, display_mode→dimensionmode | "Clear (do)animation's list" | K95:853-900 |
| 1:12210 | prelevel (premap) | tag scan of prepareboard | K94a/K94s | MEDIUM | i,k | MAXSPRITES, MAXSTATUS, MAXWALLS>>3 | numanimwalls/animwall ≈ ypanningwallcnt/ypanningwalllist | "Scan sector tags", "Scan sprite picnum's", "Map starts out completed" | K94s:692-959 |
| 1:149b0 | enterlevel (premap) | prepareboard tail + main sequence (initplayersprite, waitforeverybody, resettiming) | K94s/K95 | MEDIUM | – | MAXPLAYERS(+1) | ps[i]=ps[0]→posx[i]=posx[0]…; moveloop→waitforeverybody | "Board not found" | K94s:651-690; K95:524-528 |

No Ken counterpart (GAME/PREMAP):
- next_text_shade, FTA, draw_weapon_tile (LOW: overwritesprite with the sector shade), gametext, minitext
- numtext_relative, numtext, numtext_fixed, draw_meter, findlocator, show_help, checkcommandline (Ken only copies argv[1])
- dobonus, draw_monitor_overlay, update_menu_cursor, displayhighscores, spawn_random_effects
- read/write_userdefs, read/write_demo, level_load_hook
- render_camera_tile, render_sprite_camera, newgame, check_file_id, newscore, getscorename

## 4. Semantic map: SECTOR, PLAYER, ACTORS, SOUNDS, MENUES

| LameDuke addr | current name | Ken counterpart | versions (closest) | conf | ours→Ken params/locals | ours→Ken globals | Ken comments (paraphrase) | evidence |
|---|---|---|---|---|---|---|---|---|
| 1:150a0 | doanimations | doanimations(long numtics) (engine loop) | E94 closest; K95:3970 adds animateacc/TICSPERFRAME | HIGH (STRICT) | tics→numtics; a→animval | animateptr/animategoal/animatevel/animatecnt same; animatesect LameDuke-only | call every frame with the tics | E94:738-761; sector.c:60-103 |
| 1:15340 | getanimationgoal | getanimationgoal(long animptr) | E94 (forward loop) | HIGH (STRICT) | position→animptr; index→j | same | returns the index if the variable is animating, else -1 | E94:4176-4188 |
| 1:15380 | setanimation | setanimation(long *animptr, long thegoal, long thevel) | E94 (3 args, `return(animatecnt-1)` quirk) | HIGH (STRICT) | position→animptr; goal→thegoal; velocity→thevel; sn added | same | velocity 128 = normal door | E94:4190-4211 |
| 1:15000 | findplayer | inline "Choose a target player" in statuslistcode | K95:1841 | HIGH | sn→i; i→p; closest→mindist; closest_player→target; distance→dist | ps[].posx/posy→posx[]/posy[] | "Choose a target player" | K95:1841-1846; sector.c:42-58 |
| 1:159e0 | operatesectors | operatesector(short dasector) | K94a/K94s (3-arg setanimation) | case 9 HIGH; 21/22 HIGH-MEDIUM; 20 MEDIUM | sn→dasector; cx,cy→centx,centy; first,last→startwall,endwall; door[2]→wallfind[2]; dx,dy→dax2,day2; goal→daz | – | tag 9 "Smooshy-wall sideways double-door"; 6 up, 7 down, 8 split door; "If door already moving, reverse its direction" | case 9 (sector.c:292-360) = K94s:429-490 statement for statement; case 21 = Ken tag 7, case 22 = tag 8, case 20 = tag 6 |
| 1:18210, 1:19540 | operateactivators, operatemasterswitches | "operate all sectors/sprites with matching hitag" loops | K94s:1289-1303, 2498-2511 | LOW | – | – | – | – |
| 1:19720 | playerkeys | zoom / screen size / 2D-3D toggle fragments of processinput | K94s | MEDIUM | p→snum | ps.mapzoom→zoom[]; ps.viewmode→dimensionmode[]; ps.screen_size→screensize[] | Keypad Enter toggles dimensionmode 3↔2 | K94s:2364-2427, 2589-2609 |
| 1:1c5e0 | checksectors | space-bar (neartag) block of processinput | K94s (10-arg neartag) | MEDIUM | p→snum | neartag* same names; ps.use_pressed→spaceflag[] | "1024 is range (1 largest grid unit)"; sector tag 65535 ends the game (ours: lotag -1 ends the level) | K94s:2456-2551 |
| 1:1b4b0, 1:1bb10 | checkhitswitch, checkhitwall | switch picnum toggle; shot-wall picnum swap | K94s | LOW | – | – | SWITCH1ON = one-time switch | – |
| 1:15550 | animatewalls | gotpic-gated MIRROR and SLIME blocks of drawscreen | TXT 9/29/94 example; K95:3082-3089 | MEDIUM | – | `gotpic[49>>3]&2` = `gotpic[SLIME>>3]&(1<<(SLIME&7))`; tile 49 = SLIME | "Only animate lava if its picnum is on screen"; LameDuke's `waloff[49] != -1` is exactly TXT's 9/29/94 example (K95 tests `!= 0`) | sector.c:166-169 |
| 1:20e60 | getinput | getinput(short snum) | K94s closest (K95 has no parameter) | MEDIUM | p→snum; elapsed→i | (see section 7) | "Make sure synctics as char won't go over 255" | K94s:2800-2932; player.c:415-476 |
| 1:21410 | processinput | processinput(short snum) | K94s closest | MEDIUM (movement/turn/crush) | tics→synctics; tics2→doubvel; cz/fz→hiz/loz (K94s), globhiz/globloz (K95) | ps.*→posx[]/…/hvel[]/playersprite[] | "Lt. shift makes turn velocity 50% faster"; "Ceiling and floor are smooshing you!" | same sintable offsets K94s:2229-2239 vs player.c:753-760 |
| 1:1f660 | shoot | shootgun(snum,x,y,z,daang,dahoriz,dasectnum,guntype) | K94s:1030 | LOW | tile→guntype; angle→daang; sect→dasectnum | – | – | – |
| 1:1fe40 | displayweapon | gun overwritesprite in drawscreen (floorshade) | K94s:2665 | LOW | – | – | – | – |
| 1:25750 | movesector | revolving-sector rotatepoint/dragpoint loop | K94s:1502-1512 | MEDIUM | – | msx/msy→revolvex/revolvey; pivot→revolvepivotx/y | – | actors.c:149-167 |
| 1:258c0 | wakeactors | status-2 "monster waiting for you" list | K94s:2122 | MEDIUM | – | – | status 2 = becomes active when it sees you | – |
| 1:25480, 1:25690 | hitradius, nuke | bomb radius damage / bombexplode | K94s:1910-1966; K95:2314 | LOW | – | – | "Check for hurting monsters" | – |
| 1:25600 | ssp | none. But its movesprite call uses exactly K94s's 9-argument signature (walldist 128, ceil/flor 4<<8) | K94s:3451 | (dating evidence) | – | – | – | actors.c:126-129 |
| 1:25c30, 1:28f30, 1:2a470, 1:25f10, 1:294e0 | ifhitbyweapon, moveweapons, moveplayers, moveactors, moveeffects | checktouchsprite proximity, status-6 bullet, "Update sprite representation of player", status-1 monster loop, status 3/4/5/7 lifetimes | K94s | LOW | – | – | – | – |
| 1:2bcb0, 1:2be70, 1:2c090 | xyzsound, spritesound, pan3dsound | sndvol(xplc,yplc)+wsay; wsayfollow | K94s:3239; K95 | LOW | – | – | – | – |
| 1:2e0d0, 1:2e6e0 | vscrn, resetparallax | screensize→setview; `pskyoff[0]=0; pskybits=…` in main | K94s; K95:426 | LOW | – | – | – | – |
| 1:2e120 | init_water_ripples | initlava | K95 | PROVEN | radius→r; i→z | ring_x/ring_y/ring_count→lavaradx/lavarady/lavaradcnt; ripple_strength→lavadropsizlookup; ripple_palette→lavainc; ripple_count→lavanumdrops; ripple_phase→lavanumframes | – | section 2 |
| 1:2e200 | animate_water_ripples / update_ripple_tile | movelava | K95 | PROVEN | pixels→dapic; plus Ken's locals x,y,z,zx,dalavadropsiz,dadropsizlookup,offs,offs2,dalavax,dalavay,dat,ptr | ripple_x/ripple_y/ripple_age→lavadropx/lavadropy/lavadropsiz; ripple_border→lavabakpic | "Back up dapic with 1 pixel extra on each boundary (to prevent anding for wrap-around)" | section 2 |

No counterpart:
- **SECTOR:** checkcursectnums, animatecamsprite, activatewarpelevators (Ken's tag-10 warp sectors differ), operateforcefields, checkhitceiling, allignwarpelevators.
- **PLAYER:** kickview, spawn_pistol_tracer, displayraise, update_air.
- **ACTORS:** addweapon, spawn_gib, spawn_blood, ifsquished, effectinsect, guts, movemusic, movestandables.
- **SOUNDS:** bootleg_cache_* , USRHOOKS_*, loadsounds, playmusic, stopsound, TestCallBack.
- **MENUES:** initmenus, draw_menu_slider, drawmenu, probe, draw_and_probe, menus, draw_fade_transition.
- **GAMEDEF:** all 14 functions (the CON compiler/interpreter is 3D Realms' own).

## 5. Byte results (DOS host, canonical)

**MENUES.** Candidate `build/workers/kengame/src/menues/menues.c` + `menues.h`.

```
python tools/match.py --profile game "-i=build/workers/kengame/src/menues" build/workers/kengame/src/menues/menues.c
```

Result: STRICT 8/11 functions, 2,794 bytes. Integrated baseline: 7/11, 1,546 bytes.
- New STRICT: `animate_water_ripples` 1,248 B. Before, it was NEAR with 625 byte diffs.
- `init_water_ripples` is still STRICT with Ken's initlava text.
- No regression. CONST, _DATA and _BSS are VERIFIED.
- The `_TEXT layout CONFLICT` line is unchanged from baseline (initmenus is NEAR).

**GAME.** Candidate `build/workers/kengame/src/game/game.c`; only `strget` changed.

```
python tools/match.py --profile game_oe "-i=build/workers/kengame/src/game" build/workers/kengame/src/game/game.c
```

- STRICT count is unchanged: 26/49, 11,856 bytes.
- `strget`: NEAR 725 → 629 byte diffs.
- `main`: NEAR 4,273 → 3,935 byte diffs (strget is expanded inline there).
- The character loop now matches the oracle instruction for instruction (Ken's `keystatus[i] > 0`, `if (scantoasc[i] != 0)` loads). The remaining diffs are the stack homes of x/y and the inlined minitext/gametext tail.
- NEGATIVE: declaring x,y `short` (the oracle reads them with `movsx word`) made the loop worse on its own.

**GAME, getpackets text (NEGATIVE).** Ken's K94s getpackets form (`i=connecthead; while(i!=-1)…`, Ken's operand order) raised domovethings_multiplayer diffs 383 → 581. A `short p` loop variable (the oracle keeps the loop index in `bx`) gave 383 → 603. Both were reverted.

## 6. Toolchain fact (new, PROVEN by A/B)

MENUES needs Ken's 1994 copybuf pragma with **no modify clause**:

```
#pragma aux copybuf = "rep movsd" parm [esi][edi][ecx];
```

This is the form in BUIL0422 and buil1995 PRAGMAS.H. The integrated menues.h has `modify exact [ecx esi edi]`; with it, movelava keeps its loop counter in a register and the source pointer in a different register, and stays NEAR. The no-modify form makes the compiler spill `x` to `[esp+0xc]` and keep offs/offs2 in ebx/edx, exactly as in the oracle. Watcom also unrolls Ken's `for(x…) copybuf(…)` loop ×2 and his border loop ×4 by itself, so our hand-unrolled loops were the wrong model. This is the same finding as GAME/PREMAP (gamefix7: "copybuf pragma is Ken's 1994 form without a modify clause"). Recommendation: one shared copybuf form in every game header.

## 7. Global and constant correspondences (ours → Ken)

- **velocities:**
  - svel (forward_velocity) → vel
  - vel (strafe_velocity) → svel
  - avel (turn_velocity) → angvel
  - **Our svel/vel names are swapped relative to Ken's.** Confirmed: `player_svel` is multiplied by sintable[ang+2560] (cos, forward), as Ken's `syncvel` is (K94s:2229-2239 vs player.c:753-760).
- **sync arrays:**
  - player_svel/player_vel/player_avel/player_bits[17] → syncvel/syncsvel/syncangvel/syncbits[MAXPLAYERS+1]. Ken's bits are short; ours are long.
  - pending_* → osync*
  - script_tics → synctics
- **demo arrays:**
  - demo_svel/vel/avel/bits/tics → recsyncvel/recsyncsvel/recsyncangvel/recsyncbits/recsynctics
  - reccnt = reccnt
  - demomode ≈ recstat
- **text:**
  - input_text/input_text_length → typemessage/typemessageleng (also getmessage/getmessageleng)
  - msgtext/msgtime → getmessage/getmessagetimeoff
  - scan_ascii/scan_ascii_shift → scantoasc/scantoascwithshift
- **player state:**
  - ps[].display_mode → dimensionmode[]
  - mapzoom → zoom[]
  - screen_size → screensize[]
  - use_pressed → spaceflag[]
  - i → playersprite[]
  - poszv → hvel[]
  - Caution: LameDuke's ps[].opos* is the respawn point, while Ken's opos* is the previous-frame position used for smoothing.
- **lava:** see section 4. Ken's constants: LAVASIZ 128, LAVALOGSIZ 7, LAVAMAXDROPS (LameDuke 16; K95 32).
- **Same names in both:** animateptr/animategoal/animatevel/animatecnt (MAXANIMATES 512), neartag*, lockclock, totalclock, screenpeek, connecthead/connectpoint2, moustat, oldtimerhandler, randomseed, playerreadyflag, tempbuf.
- **keys[] layout** (NUMKEYS 17 in LameDuke's ENGINE.C, the K94s layout):

  | index | action | bit |
  |---|---|---|
  | 0 | fwd | – |
  | 1 | back | – |
  | 2 | left | – |
  | 3 | right | – |
  | 4 | run | 8 |
  | 5 | strafe | – |
  | 6 | fire | 11 |
  | 7 | space/open | 10 |
  | 8 | stand high | 0 |
  | 9 | stand low | 1 |
  | 10 | look up | 2 |
  | 11 | look down | 3 |
  | 12 | 2D/3D | 12 |
  | 13 | screenpeek | – |
  | 14 | – | 4 |
  | 15 | – | 5 |
  | 16 | typing mode | – |

- **option[]:**

  | index | meaning |
  |---|---|
  | 0 | video |
  | 1 | sound card |
  | 2 | music |
  | 3 | mouse |
  | 4 | multiplayer mode: 0 single, 1-4 COM, ≥5 network with option[4]-3 players |
  | 5 | COM speed |

## 8. Proposed renames (integrator; config/ not edited)

Byte-proven:

| current | proposed | basis |
|---|---|---|
| init_water_ripples (1:2e120) | initlava | PROVEN |
| animate_water_ripples (1:2e200) | movelava | PROVEN. Also drop the duplicate binding `update_ripple_tile`, which has the same address. |
| ripple_count, ripple_phase | lavanumdrops, lavanumframes | PROVEN through initlava/movelava |
| ripple_x, ripple_y, ripple_age | lavadropx, lavadropy, lavadropsiz | PROVEN |
| ripple_strength, ripple_palette | lavadropsizlookup, lavainc | PROVEN |
| ring_count, ring_x, ring_y | lavaradcnt, lavaradx, lavarady | PROVEN |
| ripple_border | lavabakpic | PROVEN |

HIGH (same algorithm, already STRICT or close):

| current | proposed |
|---|---|
| update_timer_input | keytimerstuff |
| domovethings_multiplayer | getpackets |
| network_move_loop | sync |
| moveloop | waitforeverybody (MEDIUM) |
| avel / vel / svel | angvel / svel / vel (fixes the swap) |
| player_svel / player_vel / player_avel / player_bits | syncvel / syncsvel / syncangvel / syncbits |
| pending_* | osyncvel / osyncsvel / osyncangvel / osyncbits |
| script_tics | synctics |
| demo_svel / demo_vel / demo_avel / demo_bits / demo_tics | recsyncvel / recsyncsvel / recsyncangvel / recsyncbits / recsynctics |
| scan_ascii / scan_ascii_shift | scantoasc / scantoascwithshift |
| input_text / input_text_length | typemessage / typemessageleng |

These were not renamed:
- **Ken's name, but Duke 1.5 keeps it:** getinput, processinput, doanimations, setanimation, getanimationgoal, animatesprites, displayrooms, operatesectors, record. These keep their current names (Duke 1.5 kept them too).
- **Inline code in Ken, no function name to borrow:** playback_demo (Ken: playback), strget, wait_for_players, findplayer.

## 9. Open / next steps

- Apply Ken's statement forms to other HIGH rows that are still NEAR:
  - operatesectors case 9 (K94s:429-490)
  - playback_demo (K94s playback)
  - wait_for_players
  - getinput (K94s getinput)
- In strget, the x/y stack homes are still open.
- No Ken GAME.C from Oct-94 to May-95 exists in the archive. Every 1994 non-timer/non-lava routine was rewritten, so no further byte-exact Ken matches are expected.
