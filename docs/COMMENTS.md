# COMMENTS: proposed header comments (semantics worker, 2026-11-26)

For the integrator to paste into src/. Comments never change code bytes, but CONST padding can contain residue of the original file text (docs/CONST_RESIDUE.md): keep comments out of string literals and re-run the gate after editing. Attributions name the source the wording or fact comes from; unattributed lines are this project's own binary-derived description. Function names below are the proposed ones (current name in parentheses).

## src/game/game.c (game_game)

```c
/* GAME: LameDuke's main module (obj1 1:00010-0f160): timer, keyboard ramps, master/slave packet sync and demo recording,
   descended from Ken Silverman's Build demo GAME.C (K94s Sep-94 getpackets/sync/playback; K95 Jun-95 timers, keytimerstuff, waitforeverybody),
   plus 3D Realms' HUD text, status bar, command line, main loop, bonus screen, saves and sector effectors,
   whose later forms are Duke Nukem 3D 1.5 GAME.C / PREMAP.C / ACTORS.C / MENUES.C (1996). */
```

### 1:00020 inittimer
`/* Set the PIT to 120 Hz and hook IRQ0 (INT 8) with timerhandler, saving the old vector. Ken Silverman, Build GAME.C (1995). */`
- 120 times/sec  (Ken, GAME.C K95:4072 (Jun-95))

### 1:00080 uninittimer
`/* Restore the BIOS 18.2 Hz timer and the original IRQ0 handler. Ken Silverman, Build GAME.C (1995). */`
- 18.2 times/sec  (Ken, GAME.C K95:4079 (Jun-95))

### 1:000d0 keytimerstuff (update_timer_input)
`/* Per-tick keyboard ramps for angvel, svel (strafe) and vel (forward). After Ken Silverman's keytimerstuff, Build GAME.C (1995). */`
- keys[5] (strafe) held turns the left/right keys into sidestep (svel) instead of turning (angvel).  (paraphrase of Ken, GAME.C K95:4090-4110 (Jun-95))
- LameDuke also ORs in keypad 4/6/8/2 (and their 0xE0 extended codes +128) and decays turning by 10 (Ken: 12); no tilt mode or second-keyboard player.  (comparison with Ken K95 keytimerstuff)

### 1:00290 level_load_hook
`/* Empty hook called around loadboard (the caller passes the map name in EAX). */`
- Stub: called with the map name before and after loadboard; possibly the map encode/decode hook whose /e switch only prints names in this build.  (binary-derived; /e behaviour per raynorpat source/premap.c (0x15748))

### 1:002a0 timerhandler
`/* 120 Hz timer interrupt. After Ken Silverman's timerhandler, Build GAME.C (1995). */`
- With music on, the sound library's TaskMan owns the PIT and chains this handler at about 18.2 Hz, so each call counts 6 ticks of the 120 Hz clock.  (paraphrase of raynorpat source/game.c clock_tics comment)
- Acknowledge the interrupt at the PIC (port 0x20).  (binary-derived)

### 1:00c60 gameexit
`/* Shut everything down and exit to DOS with a message. Duke Nukem 3D 1.5 keeps this name. */`
- Set back to text mode  (Ken, GAME.C K94s:336 (Sep-94))
- option[2] = music card, option[1] = sound card, option[4] = multiplayer mode.  (docs/ANCESTRY.md section 7)

### 1:00d10 getpackets (domovethings_multiplayer)
`/* Read and apply all pending network packets. After Ken Silverman's getpackets, Build GAME.C (1994). */`
- [2] (receive master sync buffer)  (Ken, GAME.C K94s:3181 (Sep-94))
- [3] (receive slave sync buffer)  (Ken, GAME.C K94s:3205 (Sep-94))
- [4] chat text: copied into typemessage and shown for leng<<4 frames.  (binary-derived; Ken K94s:3227 used getmessage/getmessagetimeoff)
- [5] ready flag for waitforeverybody.  (binary-derived; Ken K95 waitforeverybody (Jun-95))
- [255] (logout)  (Ken, GAME.C K94s:3232 (Sep-94))

### 1:01120 waitforeverybody (moveloop)
`/* Wait until every player is ready. After Ken Silverman's waitforeverybody, Build GAME.C (1995); Duke 1.5 PREMAP.C keeps the name. */`
- Master: re-send the ready packet every second until every player has answered. Slave: send ready to the master until the master's flag arrives.  (paraphrase of Ken, GAME.C K95:5044-5086 (Jun-95), one pass and 120-tick perio...)

### 1:018c0 sync (network_move_loop)
`/* Master/slave input synchronisation for one frame. After Ken Silverman's sync, Build GAME.C (1994). */`
- I am the MASTER!  (Ken, GAME.C K94s:3066 (Sep-94))
- Move EVERYTHING (you too!)  (Ken, GAME.C K94s:3123 (Sep-94))
- I am a SLAVE  (Ken, GAME.C K94s:3126 (Sep-94))
- Slave input goes in slot 16 (MAXPLAYERS), as in Ken's getinput(MAXPLAYERS).  (Ken, GAME.C K94s sync (Sep-94))

### 1:023d0 next_text_shade
`/* Pulsing text shade for menus and HUD captions; each call advances the pulse. */`
- Every call steps the pulse; main, dobonus and the score screens inline copies.  (raynorpat source/view.h (0x123D0), paraphrase)

### 1:02400 operatefta (displayquotes)
`/* Draw the chat message and the active help quote, decrementing their timers. Later Duke: operatefta(). */`
- Chat line at (8,162); help quote at y 189, or 166 with the status bar.  (binary-derived; raynorpat source/view.c displayquotes)

### 1:02630 FTA
`/* Show help quote q ('first time active' message). Duke Nukem 3D 1.5 keeps the name. */`
- Only when the quote is not already running; the duration scales with the text length.  (binary-derived)

### 1:026c0 myos (draw_weapon_tile)
`/* Draw an overlay tile in the viewed player's sector palette. Later Duke: myos()/myospal(). */`
- Nothing is drawn in the 2D map (dimensionmode 2).  (binary-derived)

### 1:02750 gametext
`/* Big-font text; returns the next x. Duke Nukem 3D 1.5 keeps the name. */`
- Returns 0 without drawing in the 2D map; a space advances 5 px.  (binary-derived)

### 1:02850 minitext
`/* Small-font text; returns the next x. Duke Nukem 3D 1.5 keeps the name. */`
- Glyph tile = STARTALPHANUM + (ch - '!'); a space advances 2 px.  (binary-derived; assets/DEFS.CON STARTALPHANUM 1024)

### 1:02930 gamenumber (numtext_relative)
`/* Big-font number. Later Duke: gamenumber(). */`

### 1:02a40 strget
`/* Line editor for chat and high-score names; returns nonzero on Enter. Typing loop after Ken Silverman's getinput typemode, Build GAME.C (1994). */`
- Here's a trick of making key repeat after a 1/2 second  (Ken, GAME.C K94s:3012 (Sep-94))
- Backspace's keystatus byte counts held tics (by synctics) up to 60.  (paraphrase of Ken, GAME.C K94s:3010-3020 (Sep-94))

### 1:02e40 LocateTheLocator (findlocator)
`/* Find locator n (status 7) in sector sn (-1 = anywhere). Later Duke: LocateTheLocator(). */`

### 1:02e90 EGS
`/* Spawn a sprite with explicit fields (Duke's 'EGS'). Duke Nukem 3D 1.5 keeps the name. */`
- hittype[i][4] (T5) = actor action pointer, hittype[i][1] (T2) = move pointer, sprite.extra = strength, from actorscrptr[picnum].  (binary-derived; same layout as Duke3D 1.5 GAME.C EGS (1996))
- SEENINE (57) keeps its own hitag.  (binary-derived; assets/DEFS.CON SEENINE 57)

### 1:02fb0 cacheit
`/* Precache the tiles a level needs. Later Duke split it into cachespritenum() and cachegoodsprites(). */`
- This is my exact precache code in ENGINE.C (the sprite loop skips statnum >= MAXSTATUS)  (Ken, GAME.C K94s:3476 (Sep-94), quoted for the loop structure)
- The frame loop loads i + j (sprite index), not picnum + j; kept as in the binary.  (binary-derived (STRICT); Duke 1.5 cachespritenum uses PN)

### 1:03760 genspriteremaps (loadlookup)
`/* Load sprite colour remaps from LOOKUP.DAT. Later Duke: genspriteremaps(). */`
- Missing file is fatal: 'Palette remaps not found.'  (binary string)

### 1:03800 numtext
`/* Small-font number (minitext counterpart of gamenumber). */`

### 1:038f0 digitalnumber (numtext_fixed)
`/* Status-bar digits, right-aligned. Role of Duke 1.5's digitalnumber(). */`
- Tile = ch + 0x536 = SCORENUM + (ch - '0').  (binary-derived; assets/DEFS.CON SCORENUM 1382)

### 1:03970 scratchmarks (draw_meter)
`/* Tally marks in groups of five. Survives commented out as scratchmarks() in Duke 1.5 GAME.C. */`
- Groups of five scratch marks; FIRST_METER_TILE 0x570 = SCRATCH - 1.  (binary-derived; Duke3D 1.5 GAME.C scratchmarks (1996, commented out))

### 1:03a20 animatesprites
`/* Choose rotation/animation frames for visible sprites. Duke Nukem 3D 1.5 keeps the name (Ken's analyzesprites). */`
- Between drawrooms() and drawmasks() is the perfect time to animate sprites  (Ken, GAME.C K94s:1217-1218 (Sep-94))
- This guy has only 5 pictures for 8 angles (3 are x-flipped)  (Ken, GAME.C K94s:1229 (Sep-94))
- 3200 = 3072 + 128: half a view step so each picture centres on its angle.  (binary-derived; Ken K94s:1228 writes 3072+128)

### 1:03f50 reflectfloor (draw_water_distortion)
`/* Reflective floor effect for tile 572: mirror the view into the floor's marker pixels. */`
- Only runs when the reflective floor tile (572) was drawn this frame.  (binary-derived; raynorpat source/view.c reflectfloor)

### 1:04050 displayrooms
`/* Draw the 2D map or the 3D view (player or camera). Duke Nukem 3D 1.5 keeps the name (Ken's drawscreen). */`
- 0 is grid type, range:0-5  (Ken, GAME.C K94s:2673 (Sep-94), on the draw2dscreen call)
- Sync bits 19, 20 and 22 (screen-size and map keys) repaint the border with BIGHOLE (1170).  (paraphrase of raynorpat source/view.c (0x160BD))

### 1:04cd0 nonsharedkeys
`/* Local (non-synchronised) function keys and the debug warp screen. Duke Nukem 3D 1.5 keeps the name. */`
- F12  (Ken, GAME.C K94s:2958 (Sep-94), on screencapture)
- V cycles visibility down and wraps (Ken: 7 -> 15; here below 7 -> 16).  (paraphrase of Ken, GAME.C K94s:2971-2976 (Sep-94))
- K: view the next player in the connection ring, then reset the video mode for that player's dimensionmode.  (paraphrase of Ken, GAME.C K94s:2814-2822 (Sep-94))
- Warp mode (debug, W): Page Up/Down pick the episode, arrows the level; any exit key but Esc enters the level.  (paraphrase of raynorpat source/game.c warp (0x14CD0))

### 1:054f0 comlinehelp (show_help)
`/* Command-line help. Later Duke: comlinehelp(). */`

### 1:05570 checkcommandline
`/* Parse the command line. Duke Nukem 3D 1.5 keeps the name. */`
- /e only prints the map names; no map is written.  (raynorpat source/premap.c (0x15748), paraphrase)
- An unknown switch falls through into the /? help case.  (binary-derived)

### 1:057c0 wait_for_players
`/* Wait for the other players to log on. From the waitplayers loop of Ken Silverman's main, Build GAME.C (1994). */`
- option[4]: 1-4 = COM port game (2 players), >= 5 = network with option[4]-3 players.  (docs/ANCESTRY.md section 7, from Ken K94s:207-210)

### 1:05b60 main
`/* Program entry: initialise, run demos and the game loop, save scores and exit. */`
- A failed integrity check deletes the executable (unlink(argv[0])).  (binary-derived)
- attract: demos play until a game starts; MODE_DEMO (gm bit 2) from the menu returns here.  (binary-derived; MODE_* names from Duke3D 1.5 DUKE3D.H:135-142 (1996))
- MODE_EOL: next level; MODE_TYPE: chat line sent as packet type 4; MODE_END ends the loop.  (binary-derived; Duke3D 1.5 DUKE3D.H MODE_* (1996))

### 1:07890 coolgaugetext (displaystatus)
`/* Status bar and inventory panel for player snum (3D view only). */`
- TOPGUAGE 1380 / BOTTOMGUAGE 1381; ammo icons BATTERYICON..RPGICON 1400-1403; inventory panel INVENTORYBAR 1398 with INVENTORYNOTCH 1399 marks.  (assets/DEFS.CON tile names)
- Weapons by curr_weapon tile: BULLET/CLUB -> batteryammo, SHOTSPARK1 -> ammo, HEAVYHBOMB/HANDREMOTE -> remote_ammo, RPG -> rocket_ammo.  (binary-derived; tile names from assets/DEFS.CON)

### 1:081c0 playback (playback_demo)
`/* Demo playback / attract loop. After Ken Silverman's playback, Build GAME.C (1994); Duke 1.5 keeps the name. */`
- Recorded screen-size bits (0x180000) are stripped from the replayed input.  (binary-derived; raynorpat source/demo.c applies the same mask)
- gm == MODE_END quits; MODE_GAME starts play, unless MODE_DEMO is also set (next demo).  (binary-derived; MODE_* from Duke3D 1.5 DUKE3D.H (1996))

### 1:08e30 record
`/* Record one tick of input into the demo buffers. Ken Silverman's recording code, Build GAME.C (1994). */`
- Can only record 1-player game  (Ken, GAME.C K94s:89 (Sep-94))

### 1:08ea0 domovethings
`/* Advance the whole game by one tick. Duke Nukem 3D 1.5 keeps the name (Ken's movethings). */`
- Door code, moving sector code, other stuff  (Ken, GAME.C K94s:2794 (Sep-94), on tagcode (here moveeffectors))
- Monster / bullet code / explosions  (Ken, GAME.C K94s:2795 (Sep-94), on statuslistcode (here moveactors/moveweapons))
- randomstash = krand() once per tick (Duke 1.5: global_random = TRAND).  (binary-derived; Duke3D 1.5 GAME.C:8597 (1996))

### 1:09650 dobonus
`/* Bonus/episode-map screen between levels. Duke Nukem 3D 1.5 keeps the name. */`
- The tally points are added to the score on every frame they are shown.  (raynorpat source/game.c levelscreen comment, paraphrase)
- BONUSSCREEN1/2 (1132/1135) backgrounds, PLAYERWASHERE/PLAYERISHERE (1133/1134) level markers from bonus_marks.  (assets/DEFS.CON tile names)

### 1:0a420 cameratext (draw_monitor_overlay)
`/* Camera-view frame overlay. Later Duke: cameratext(). */`
- The light blinks with totalclock bit 4.  (binary-derived; same test in Duke3D 1.5 cameratext (1996))

### 1:0a6f0 update_menu_cursor
`/* Update and draw the menu mouse pointer. */`
- A click registers on release: menu_mouse takes the previous buttons when they go up.  (binary-derived)

### 1:0a8b0 displayhighscores
`/* High-score table. */`

### 1:0acb0 lotsofglass (spawn_random_effects)
`/* Throw glass shards from sprite i. Later Duke: lotsofglass() (which adds a wall parameter). */`

### 1:0ae90 movetransports (move_status9)
`/* Transporters and water-level crossings (status 9). Later Duke: movetransports(). */`
- Sector lotag 1 = water surface, 2 = underwater; hittype[i][0] is the warp cooldown.  (binary-derived; same conventions in Duke3D 1.5 ACTORS.C movetransports (1996))
- The partner search keeps the last skipped candidate's sector lotag as the one tested next.  (raynorpat source/actors.c moveteleporters comment, paraphrase)

### 1:0b520 read_userdefs
`/* Read USERDEFS.TMP (settings + high scores). */`
- Defaults: visibility 13, volumes 255, brightness 1, sound and music on, mouse sensitivity 128.  (binary-derived; raynorpat source/config.c)

### 1:0b5d0 write_userdefs
`/* Write USERDEFS.TMP (settings + high scores). */`
- The open flags include both O_TEXT and O_BINARY, as in the original call.  (binary-derived (existing source comment))

### 1:0b630 saveplayer
`/* Save a game to slot spot. Duke Nukem 3D 1.5 keeps the name. */`

### 1:0b700 loadplayer
`/* Load a game from slot spot. Duke Nukem 3D 1.5 keeps the name. */`

### 1:0b810 opendemoread (read_demo)
`/* Open and load demo which_demo. Later Duke: opendemoread(). */`
- File: 3 header bytes (volume, level, skill), long reccnt, then forward, strafe, turn, tics bytes and 4-byte bits per record.  (binary-derived; raynorpat source/demo.c)

### 1:0b920 closedemowrite (write_demo)
`/* Write the recorded demo at exit. Later Duke split this into opendemowrite()/closedemowrite(). */`
- A recording drops its last sample before writing.  (raynorpat source/demo.c (0x1B920), paraphrase)

### 1:0ba30 moveeffectors
`/* Sector effector logic (status 3). Duke Nukem 3D 1.5 keeps the name. */`
- Nothing for now used as the pivot  (Duke3D 1.5 ACTORS.C moveeffectors, case 1 (1996))
- SE6 is the subway engine and SE14 its cars, following locators (status 7) in order.  (binary-derived; same lotags in Duke3D 1.5 ACTORS.C (1996))
- SE27: during demo playback (recstat 2) take over the view as ud.camerasprite when the player is near and visible.  (binary-derived; Duke3D 1.5 ACTORS.C case 27 (1996))
- SE to control glass breakage  (Duke3D 1.5 ACTORS.C moveeffectors, case 128 (1996))

## src/game/premap.c (game_premap)

```c
/* PREMAP: level setup (spawn, prelevel, newgame, enterlevel), player and level-state resets, the 96x96 camera/mirror tile renderer
   and the high-score entry helpers. Names follow Duke3D 1.5 PREMAP.C/GAME.C (1996) where the role matches; Ken's Sep-94 GAME.C prepareboard
   (K94s:640-959) is the ancestor of the reset/prelevel/enterlevel sequence. LameDuke's ps.opos* is the respawn point, not the previous-frame
   position (that is ps.previous_*). */
```

### 1:0f160 spawn
`/* Set up map sprite pn (j < 0) or spawn tile pn at sprite j; per-picnum defaults and sector-effector setup. */`
- Actor header: strength -> extra, action -> T5, move -> T2, flags -> hitag (not for SEENINE).  (Duke3D 1.5 GAME.C spawn (1996); LameDuke code)
- CAMERA1..CAMERA1+4 (265-269) share CAMERA1's actor script.  (LameDuke code)
- Enemies count toward max_squashed and max_actors_killed; turrets and BOUNCEMINE only the latter.  (LameDuke code (spawn cases 408..1820, 930, 931, 940))
- SE: yvel = sector extra (speed); statnum 9 (transport) for SE 7, else 3.  (Duke3D 1.5 GAME.C spawn SECTOREFFECTOR (1996))
- SE 8/9: the exe overwrites the computed shade step with 8.  (raynorpat actors.c; LameDuke code)
- SE 5/6/14: the exe seeds updatesector with the wall loop's end index.  (raynorpat actors.c; LameDuke code)
- ACTIVATORLOCKED (23) starts its sector locked (lotag ^= 0x4000).  (LameDuke code; Duke3D 1.5 lock bit 16384)

### 1:11380 render_camera_tile
`/* Render a 96x96 view from (x,y,z,ang,sect) into tile tilenume (viewscreens, mirror). */`
- Copy, draw the tile back with rotatesprite, copy again: leaves the view in Build's column order.  (LameDuke code; Duke3D 1.5 later uses squarerotatetile)
- vscrn(screenpeek) restores the player's view window afterwards.  (LameDuke code)

### 1:116b0 render_sprite_camera
`/* Render sprite i's viewpoint into its own tile. */`

### 1:11a10 render_mirror
`/* Draw the fake mirror: the view from wall w's midpoint toward the viewer, into MIRROR (570). */`
- Called once per frame at most, from animatewalls when MIRROR was drawn.  (LameDuke code (sector.c animatewalls))

### 1:11de0 resetplayer
`/* Respawn player snum at its start point with fresh view, motion and weapon-raise state (CON 'resetplayer'). */`
- knuckle_incs = 1 starts the knuckle-crack animation, as in Duke3D 1.5.  (Duke3D 1.5 PREMAP.C resetplayerstats (1996))
- Health is APLAYER's script strength (*actorscrptr[283]), else 100.  (LameDuke code)

### 1:11f30 resetprestat (reset_player_motion)
`/* Reset level tallies and global level state before a level starts (Duke3D 1.5 resetprestat), then resetplayer. */`
- Clear (do)animation's list  (Ken K95 GAME.C prepareboard (Jun-95))
- Leaves 2D map mode (setgamemode) and copies the loaded start into opos* (respawn point).  (LameDuke code)
- Zeroes the keyboard ramps svel/vel/avel (= Ken's vel/svel/angvel; our svel/vel names are swapped).  (docs/ANCESTRY.md section 7)

### 1:12210 prelevel
`/* Set up a freshly loaded board: reset state, scan sector tags, spawn all map sprites, collect animated walls. */`
- Map starts out completed  (Ken K94s GAME.C:955 (Sep-94))
- Sprites appear on the 2D map only in debug mode (/d).  (LameDuke code (show2dsprite = 255 * userdefs.debug))
- Found a secret room  (Duke3D 1.5 PREMAP.C prelevel (1996))
- GPSPEED (310) sets its sector's extra (speed), then is deleted.  (Duke3D 1.5 PREMAP.C prelevel (1996); LameDuke code)
- That is, if sprite exists  (Ken K94s GAME.C:934 (Sep-94), statnum < MAXSTATUS)
- W_FORCEFIELD: shade < 32 starts solid; its lotag is copied to the other side.  (LameDuke code; Duke3D 1.5 prelevel same rule)
- LIGHTSWITCH+1 (406): the exe passes the wall's lotag, not its index, to checkhitswitch.  (raynorpat premap.c; LameDuke code)
- waloff[] == -1 marks an uncached tile here (Duke3D 1.5 tests 0).  (LameDuke code)

### 1:148a0 newgame
`/* Start a new game: episode vn, level ln, skill sk; fresh inventory, pistol and ammo for player 0. */`
- curr_weapon 191 (SHOTSPARK1) is the pistol; gotweapon[1] = 1.  (LameDuke code; assets/DEFS.CON)
- Debug mode (/d) starts with ammo 144, battery 120, remote 696, rockets 99.  (LameDuke code)

### 1:149b0 enterlevel
`/* Load and start the current level; g becomes the game mode (MODE_GAME / MODE_DEMO). */`
- Bonus screen when entering in MODE_GAME, unless debug mode.  (LameDuke code)
- Board not found  (Ken K94s GAME.C:655 (Sep-94))
- moveloop is Ken's waitforeverybody.  (docs/ANCESTRY.md section 3 (K95:5044-5086))
- Multiplayer: each player starts (and respawns) at its own APLAYER sprite.  (LameDuke code; Duke3D 1.5 resetpspritevars)

### 1:14e00 check_file_id
`/* Check FILE_ID.DIZ against the embedded notice; nonzero = missing or modified. */`
- '*' in the record skips two file bytes (wildcard).  (LameDuke code)
- On failure main prints a warning and unlink(argv[0]) deletes the EXE.  (LameDuke code (game.c main))

### 1:14e80 newscore
`/* Place the current score in the high-score table; returns its row or -1. */`
- The shift starts at row 10 and stops at i+1: old row i is overwritten, not moved.  (raynorpat menus.c; LameDuke code)

### 1:14f80 getscorename
`/* Edit the high-score name on row; nonzero when entry is finished. */`

### 1:14fad game_text_end
`/* Linker/layout marker or private table, not a function (game_text_end). */`

## src/game/sector.c (game_sector)

```c
/* SECTOR: the animation list (Ken's 1994 engine doanimations/getanimationgoal/setanimation, moved into the game), doors and lifts by
   sector lotag, activators, master switches, force fields, the per-player interface keys, switches, shot walls/ceilings and the use key.
   Function order and names match Duke3D 1.5 SECTOR.C (1996) almost one for one; the sliding star door (lotag 9) is Ken's Sep-94
   operatesector code. Sector lotag bit 0x8000 = open/moving state, 0x4000 = locked. */
```

### 1:14fb0 checkcursectnums
`/* First player standing in sector sect, or -1. */`
- Tests ps[].cursectnum; Duke3D 1.5 tests the player sprite's sectnum.  (LameDuke code vs Duke3D 1.5 SECTOR.C)

### 1:15000 findplayer
`/* Nearest player to sprite i (Manhattan x+y); distance in *d. */`
- Choose a target player  (Ken K95 GAME.C:1841 (Jun-95))
- x+y only and uninitialised closest_player; Duke3D 1.5 adds z/16 and starts at 0.  (LameDuke code vs Duke3D 1.5)

### 1:150a0 doanimations
`/* Advance the animation list by numtics (Ken's 1994 engine routine plus riders and lift sounds). */`
- Players on the ground in the moving sector ride along (posz += vel * tics).  (LameDuke code)
- On arrival: stop sound 36 (ELEV2.VOC), play 35 (ELEV3.VOC) at the sector's first wall.  (LameDuke code; sound table in sounds.c)
- A finished entry is replaced by the last one.  (Ken E94 ENGINE.C doanimations (Apr-94))

### 1:15340 getanimationgoal
`/* Slot index animating animptr, or -1 if it is not moving. */`

### 1:15380 setanimation
`/* Start (or retarget) moving *animptr toward thegoal at thevel per tic for sector animsect. */`
- Returns animatecnt-1, not the slot used (Ken's 1994 quirk; Duke returns j).  (Ken E94 ENGINE.C setanimation (Apr-94))
- Full at MAXANIMATES-1 (511) entries.  (Ken E94 / LameDuke code)

### 1:15410 animatecamsprite
`/* Refresh the viewscreen tile (CAM1) from its camera every 12 frames when the player is near. */`
- A player looking through a camera (newowner) makes it the viewscreen's owner.  (Duke3D 1.5 SECTOR.C animatecamsprite (1996))

### 1:15550 animatewalls
`/* Animate the walls collected by prelevel and the gotpic-driven lava and mirror tiles. */`
- Only animate lava if its picnum is on screen  (Ken K95 GAME.C:3082 (Jun-95))
- W_TECHWALL3 tests 859, so it never wraps and runs on into W_TECHWALL4's frames.  (LameDuke code (exe behaviour))
- W_L* screens: animwall.tag remembers the picture while W_SCREENSTATIC shows.  (LameDuke code; Duke3D 1.5 same scheme)
- gotpic[570>>3] &= 0xeb also clears tile 572's bit.  (LameDuke code)

### 1:15810 activatewarpelevators
`/* Set every warp elevator (SE 17) of s's group moving in direction d, or stop them. */`
- A lift can move if away from its stop (T3) or the target floor has sector hitag current-d.  (LameDuke code; Duke3D 1.5 same test)
- Sound 35 (ELEV3) on stop, 0 (ELEV1) on start: Duke's ELEVATOR_OFF/ON.  (LameDuke code; Duke3D 1.5 SECTOR.C)

### 1:1598c operatesectors_table
`/* Linker/layout marker or private table, not a function (operatesectors_table). */`

### 1:159e0 operatesectors
`/* Operate sector sn according to its lotag (doors, lifts, elevators); ii is the initiator. */`
- Smooshy-wall sideways double-door  (Ken K94s GAME.C:429 (Sep-94), lotag 9)
- Find the opening direction by averaging the two neighbours of wallfind[j].  (Ken K94s GAME.C:447 (paraphrase))
- The split doors: run lotag 22 then 9, then restore 26.  (Duke3D 1.5 SECTOR.C operatesectors (1996))
- If door already moving, reverse its direction  (Ken K94s GAME.C:368 (Sep-94), lotags 21/22)
- 21/22: the exe compares animategoal[sn] (sector number) instead of slot j.  (raynorpat sector.c; LameDuke code)
- No neighbour that way: flip lotag bit 0 (16<->17, 18<->19) and try the other way.  (LameDuke code)
- Warping elevators: in the sector, jump (bit 0) goes up and crouch (bit 1) down.  (Duke3D 1.5 comment; LameDuke code)
- 15, 25 and 28 use sprite[-1] when no matching effector exists (unchecked).  (raynorpat sector.c; LameDuke code)

### 1:18210 operateactivators
`/* Fire every activator on channel low: unlock/lock or operate its sector. */`

### 1:19540 operatemasterswitches
`/* Arm the master switches on channel low. */`

### 1:19590 operateforcefields
`/* Switch the force fields on channel low (or all) on/off. */`
- cstat 85 = blocking + bottom-aligned + masked + hitscan-blocking.  (raynorpat docs/map-format.md wall cstat bits)

### 1:19720 cheatkeys (playerkeys)
`/* Handle player snum's interface and inventory keys (Duke3D 1.5 cheatkeys), including the /d door cheat. */`
- One-shot keys wait for interface_toggle_flag; it clears when no bit above 7 is held.  (LameDuke code; Duke3D 1.5 cheatkeys)
- C (bit 16) starts the knuckle crack; I (bit 17) toggles the inventory bar.  (LameDuke code (getinput scancodes))
- Without /d, F1/F2/F3 also open menu pages 20/50/10.  (LameDuke code)
- /d + U: unlock and operate every lotag 15-29 sector, toggle all force fields.  (LameDuke code; raynorpat player.c)

### 1:1b4b0 checkhitswitch
`/* Use or shoot wall switch w: flip all switches on its channel and fire the channel. */`
- The second switch reads w's picnum after the flip: 405 = now on, 406 = now off.  (LameDuke code)
- SLOTDOOR: quote 4 and sound 25 (SWITCH.VOC); SE 12 of hitag lotag get T1 = 2 on / 1 off.  (LameDuke code)

### 1:1bb10 checkhitwall
`/* Effects of a shot on wall dawallnum: glass, tech walls and breakable lights. */`
- GLASS/GLASS3/GLASS2: becomes GLASS2, spawns SE 128 (T2 = 5 frames, T3 = wall), sound 5.  (LameDuke code)
- W_TECHWALL frames become W_HITTECHWALL tiles with sound 23 (LITEFLCK.VOC).  (LameDuke code)
- WALLLIGHT with lotag: sectors with hitag == lotag go dark; their SE 8/9 are deleted.  (LameDuke code)
- updatesector starts from an unset sn (Duke 1.5 initialises sn = -1).  (LameDuke code vs Duke3D 1.5)

### 1:1c180 checkhitceiling
`/* Shot ceiling light in sector sn: bust it and black out its hitag group; 1 if handled. */`
- best is left unset when no linked ceiling is darker (exe).  (LameDuke code)
- On a miss the exe clears only AL; callers test the result as == 0.  (raynorpat sector.c)

### 1:1c5e0 checksectors
`/* Sector specials and the use key for player snum. */`
- 1024 is range (1024 = 1 largest grid unit)  (Ken K94s GAME.C:2458 (Sep-94))
- Space or Esc triggers use, latched by toggle_key_flag.  (LameDuke code)
- At a VIEWSCREEN (180): Space takes the next free CAMERA1 (yvel = in use); Esc returns to the body.  (LameDuke code; Duke3D 1.5 newowner scheme)

### 1:1f190 allignwarpelevators
`/* Align all warp elevators of a group to the master (shade > 16) at level start. */`

### 1:1f27e sector_text_end
`/* Linker/layout marker or private table, not a function (sector_text_end). */`

## src/game/player.c (game_player)

```c
/* PLAYER: per-tick input sampling (getinput), player physics and weapon state machine (processinput), first-person weapon drawing (displayweapon, animateknuckles) and weapon firing (shoot).
   Descends from Ken Silverman's Sep-94 Build demo GAME.C getinput/processinput (sync arrays; NOTE our svel/vel names are swapped vs Ken's vel/svel) and is the direct ancestor of Duke Nukem 3D 1.5 PLAYER.C (1996):
   same sync-bit layout (jump 0, crouch 1, fire 2, run 5, look 6/7, weapons 8+, look up/down 13/14, center 18, pause 21, Esc 31) and same player_struct field order (fta/ftq ... somethingonplayer, on_crane, one_parallax_sectnum; toggle_key_flag, knuckle_incs ... hard_landing, max_secret_rooms).
   BINARY_DERIVED; semantics from DEFS.CON/GAME.CON, raynorpat rebuild, Ken K94s GAME.C and Duke 1.5 PLAYER.C. */
```

### 1:1f2e0 kickview
`/* Kick player snum's view (horiz +-4, look_ang = +-2*damage) away from the side sprite i hit from. */`
- Only applies while sprite[ps[snum].i].extra/20 >= ps[snum].health (low health relative to the player sprite's extra); kept as in the binary.  (binary-derived (STRICT source))

### 1:1f410 spawn_lasersite (spawn_pistol_tracer)
`/* Pistol laser sight: hitscan along the gun's aim and place a one-frame LASERSITE dot at the hit point. */`
- LASERSITE dot: GAME.CON kills it the next frame, so this redraws the sight every tick.  (assets/GAME.CON + DEFS.CON (1994-12))

### 1:1f660 shoot
`/* Fire weapon tile atwith from sprite i (player or actor); with impact != 0, move projectile i onto what it hits. */`
- Third argument (impact) has no Duke 1.5 equivalent; CON 'shoot' passes only two, leaving it from the caller's register.  (binary-derived; src/game/gamedef.c CON_SHOOT)
- A player shot sets ps.shooting (CON 'ifp pshooting') and flashes the holoduke sprite (shade 0).  (binary-derived; GAME.CON pshooting 4096)

### 1:1fd20 animateknuckles (displayraise)
`/* Draw the knuckle-crack animation (CRACKKNUCKLES) for player snum; return 1 if it was drawn. */`
- Duke 1.5 animateknuckles keeps this table and layout; it indexes by knuckle_incs>>1.  (Duke3D 1.5 PLAYER.C (1996))

### 1:1fe40 displayweapon
`/* Draw player snum's first-person weapon (after the knuckle-crack animation). */`
- Tiles: CLUB 1230 (+CLUBSPARK 1239..1242), RPGGUN 1250-1252, GUN2A 882-886, FIRSTGUN 1150, FIRSTGUNRELOAD 1160, FALLINGCLIP 1162, CLIPINHAND 1163, HANDTHROW 1143, HANDREMOTE 1140.  (assets/DEFS.CON (1994-12))
- Pistol reload: CLIPOUT sound at kickback 10, CLIPIN at 13.  (binary-derived; sounds.c sound_names)

### 1:20e60 getinput
`/* Sample local input into the sync slot snum (Ken getinput; bit layout survives into Duke 1.5). */`
- Make sure synctics as char won't go over 255  (Ken Silverman, K94s GAME.C:2928 (Sep-94))
- Mouse y feeds svel, which is the FORWARD accumulator (Ken's vel); names are swapped vs Ken.  (ANCESTRY.md section 7; player.c processinput sintable offsets)

### 1:21280 update_air
`/* Air and scuba handling for player snum in underwater (lotag 2) sectors. */`
- With scuba_on, airleft is the tank charge: at <1 scuba switches off and air resets to 1728.  (binary-derived (STRICT source))

### 1:21410 processinput
`/* Run one tick of player snum from syncvel/syncsvel/syncangvel/syncbits: movement, physics, view and weapon. */`
- if(p->on_crane >= 0) goto HORIZONLY;  (Duke3D 1.5 PLAYER.C processinput (1996): same jump as 'state82 >= 0 goto domove')
- Ceiling and floor are smooshing you!  (Ken Silverman, K94s GAME.C:2440 (crush test klabs(cz-fz) < 7168))
- fta[ftq] counts down to 1, never 0, so each quote shows once per level (FTA tests fta[q]==0).  (binary-derived; game.c FTA)
- weapon_clock holds only the low byte of lockclock; the 'lockclock - weapon_clock' tests compare it with the full clock.  (binary-derived)

### 1:2515c player_text_end
`/* Linker/layout marker or private table, not a function (player_text_end). */`

## src/game/actors.c (game_actors)

```c
/* ACTORS: per-tick sprite updates by status list (1 actors, 2 sleeping actors, 4 projectiles, 5 effects, 6 standables, 10 players, 11 music/sound fx) plus small spawn helpers (guts, blood, gibs, nuke, hitradius).
   Status 1/2 keep Ken Silverman's Sep-94 GAME.C meanings ('active monster' / 'monster waiting for you'); the rest is 3D Realms code and the direct ancestor of Duke Nukem 3D 1.5 ACTORS.C (1996):
   ms, movefta, ifsquished, ifhitsectors, ifhitbyweapon, ssp, guts, hitradius, moveweapons, moveexplosions (statnum 5), movestandables, moveplayers, movefx keep their roles there.
   Hard-coded tiles are DEFS.CON names; everything else is handed to the CON interpreter execute(). BINARY_DERIVED. */
```

### 1:25180 addweapon
`/* Give player p the weapon tile and switch to it if new or force is set (Duke 1.5 addweapon role). */`
- gotweapon[]: 0 CLUB, 1 SHOTSPARK1, 2 BULLET, 3 HEAVYHBOMB, 4 RPG; weapon_pos 5 starts the raise.  (binary-derived (STRICT source))

### 1:25480 hitradius
`/* Damage everything in status 1/6/10 within r of sprite i by spawning RADIUSEXPLOSION damage sprites. */`
- Victims with extra < 0 (already dead) are skipped; damage is delivered later by moveweapons/ifhitbyweapon.  (binary-derived)

### 1:255b0 spawn_gib
`/* Spawn one moving gib (status 5) of tile gtype from sprite i. */`

### 1:25600 ssp
`/* Move sprite i by its velocity; 1 if the move was not blocked. */`
- The set sprite function  (Duke3D 1.5 ACTORS.C:694 (1996))

### 1:25690 nuke
`/* CON 'nuke': FLAIR flash plus EXPLOSION at (x,y,z) from sprite i. */`

### 1:256e0 spawn_blood
`/* Spawn one BLOOD drop from sprite i. */`

### 1:25750 ms (movesector)
`/* Move sector effector i and drag its walls (msx/msy from T2, angle T3). */`
- T1,T2 and T3 are used for all the sector moving stuff!!!  (Duke3D 1.5 ACTORS.C ms (1996))

### 1:258c0 movefta (wakeactors)
`/* Wake status-2 actors that can see a player (Duke 1.5 movefta; check_fta_sounds inline). */`
- Go through monster waiting for you list  (Ken Silverman, K94s GAME.C:2122 (Sep-94))
- FEMANOID quote 0 (FTA_HF), MANDROID/CAPTAIN 0 (FTA_HM), DRONE1/TROOP1 3 (FTA_R), GUN2ASPRITE/FIRSTGUNSPRITE 1, REACTOR 2, COKE 6.  (DEFS.CON + global.c quote_text)

### 1:25a60 ifsquished
`/* Crush test for sprite i in a closing sector; returns i if squished, else -1. */`
- Squashable tiles: FEMANOID 408, MANDROID 490, DRUNKGUY1 631, TROOP1 1536, CAPTAIN 1680, JELLYFISH 1820, DRONE1 579.  (DEFS.CON (1994-12))

### 1:25be0 ifhitsectors (effectinsect)
`/* First status-5 effect sprite in sprite i's sector, or -1 (Duke 1.5 ifhitsectors). */`
- LameDuke accepts any status-5 effect except LASERSITE; Duke 1.5 tests EXPLOSION2 only.  (binary-derived vs Duke3D 1.5 ACTORS.C)

### 1:25c30 ifhitbyweapon
`/* Projectile (status 4) touching sprite i: apply its damage and return it, else -1. */`
- Actors and players hit by BULLET spawn STICKSPARK.  (binary-derived)

### 1:25d50 guts
`/* CON 'guts': throw arm, leg and head pieces from sprite i. */`

### 1:25f10 moveactors
`/* Update active actors (status 1): hard-coded tiles here, everything else through CON execute(). */`
- Go through active monster sprites  (Ken Silverman, K94s GAME.C:1708 (Sep-94))
- CAM1 within 2048 of a player becomes the monitor camera (cf. Duke 1.5 camsprite).  (binary-derived; Duke3D 1.5 ACTORS.C:2114)
- HEAVYHBOMB inlines ifsquished and explodes when the owner's hbomb_on is cleared by the detonator.  (binary-derived; Duke3D 1.5 ACTORS.C:4078 same hbomb_on test)

### 1:28f30 moveweapons
`/* Update projectiles (status 4): flight, player hits and impacts, then CON. */`
- Sparks and RADIUSEXPLOSION act on their second tick: the first sets shade -15 as a marker.  (binary-derived)
- Player hits: within 768 (512 for explosions), not own shot, /dz/ <= 48<<8: kickview + shield/health damage.  (binary-derived)

### 1:294e0 moveexplosions (moveeffects)
`/* Update effect sprites (status 5): debris, gibs, casings and splashes; other effects run CON. */`
- // STATNUM 5  (Duke3D 1.5 ACTORS.C:4377 moveexplosions (1996))
- Gibs: t[0] frame counter, t[1] bounces, t[2] landed flag, t[4] rest timer (deleted after 256 ticks when far).  (binary-derived)

### 1:2a470 moveplayers
`/* Update player sprites (status 10) and holoduke decoys. */`
- Update sprite representation of player  (Ken Silverman, K94s GAME.C:2429 (Sep-94))
- Looking thru the camera  (Duke3D 1.5 ACTORS.C moveplayers (1996), for the newowner >= 0 branch)

### 1:2a860 movefx (movemusic)
`/* Range-triggered sound effects and level/environment music switching (status 11). */`
- Music state t[0]: 0 start level song, 1 wait to enter range, 2 start env song, 3 wait to leave; t[1] fades volume by 12 per tick.  (binary-derived)

### 1:2ab90 movestandables
`/* Update standables (status 6): force posts, cranes, cans; script standables run CON. */`
- Crane: t[0] state 0..9, t[1] target sector, t[2] wait counter, t[4] base index into msx/msy (pickup point +1, pole sprite in msy[+2]).  (binary-derived; premap spawn CRANE)

### 1:2ba27 actors_text_end
`/* Linker/layout marker or private table, not a function (actors_text_end). */`

## src/game/sounds.c (game_sounds)

```c
/* SOUNDS: the 'BootLeg CacheMaster' LIFO arena (also the Apogee Sound System USRHOOKS memory hooks), VOC loading, MIDI playback
   and positional effects through Jim Dose's ASS (FX_PlayVOC3D/FX_Pan3D). Fixed 39-name VOC table, at most 3 copies of a sound.
   Duke3D 1.5 SOUNDS.C (1996) keeps xyzsound/spritesound/stopsound/pan3dsound/TestCallBack/playmusic/USRHOOKS_*; cache names are invented. */
```

### 1:2ba30 bootleg_cache_alloc
`/* BootLeg CacheMaster: allocate size bytes from the LIFO arena (exits on overflow). */`
- LIFO only: blocks are freed by popping in reverse order (bootleg_cache_pop).  (binary-derived)

### 1:2baa0 bootleg_cache_pop
`/* Free the newest BootLeg cache block. */`

### 1:2bae0 bootleg_cache_reset
`/* Free all BootLeg cache blocks (unused). */`
- Unreferenced in the binary.  (docs/units/sounds.md)

### 1:2bb30 init_bootleg_cache
`/* Initialise the BootLeg CacheMaster arena (300000 bytes). */`

### 1:2bb60 USRHOOKS_GetMem
`/* Apogee Sound System hook: allocate size bytes into *ptr from the BootLeg cache. */`

### 1:2bb90 USRHOOKS_FreeMem
`/* Apogee Sound System hook: free (pops the newest BootLeg cache block). */`
- Assumes the library frees in reverse allocation order.  (binary-derived)

### 1:2bbd0 loadsounds
`/* Load every VOC of sound_names into the BootLeg cache (startup, after FX_SetupCard). */`
- A missing sound keeps a null pointer and is still passed to FX_PlayVOC3D.  (raynorpat docs/audio.md)

### 1:2bc40 playmusic
`/* Load and loop a MIDI song through MUSIC_PlaySong. */`
- No open-failure or size check: music_buffer holds 40000 bytes (Duke 1.5 checks l >= 72000).  (binary-derived; Duke3D 1.5 SOUNDS.C)

### 1:2bcb0 xyzsound
`/* Play sound num at map position x,y (owner sprite i, -1 = player 0). */`
- Skipped when sound is off or 3 copies already play.  (binary-derived)
- The prototype also stores Z in the Y field.  (sounds worker (existing source comment))

### 1:2be70 spritesound
`/* Play sound num at sprite i (inline copy of xyzsound). */`

### 1:2c040 stopsound
`/* Stop sound num (its last voice). */`

### 1:2c090 pan3dsound
`/* Re-pan all playing sounds to their owners (call once per frame). */`

### 1:2c190 TestCallBack
`/* FX callback: a voice of sound num finished. */`

### 1:2c1bb sounds_text_end
`/* Linker/layout marker or private table, not a function (sounds_text_end). */`

## src/game/menues.c (game_menues)

```c
/* MENUES: the prototype's menu pages (descriptor setup, big-font text, slider, key/mouse probe, page dispatcher), the PAUSE sign, the view window,
   and Ken Silverman's lava/boiling-slime effect (initlava/movelava, Ken-Build GAME.C; BUILD.TXT 9/29/94) placed in this object.
   Menu code is BINARY_DERIVED; Duke3D 1.5 MENUES.C (1996) is its descendant (probe, bar, menutext, menus). */
```

### 1:2c1c0 initmenus
`/* Set up the static menu descriptors (title tile, text lines, position, input flags). */`
- Title tiles are DEFS.CON names: HELPMENU 1129, EPISODE 616, SKILL 617, RESTOREGAME 613, SAVEGAME 1137, INSTRUCTIONS 615, OPTIONS 1128, ORDERINGINFO 1127, HIGHSCORES 1138.  (assets/DEFS.CON)

### 1:2c430 bar (draw_menu_slider)
`/* Draw and edit one options slider (32 notches, value 0..255) at x,y. */`
- Value = pointer x - 32, clamped to 0..255; Left/Right move the pointer 8 pixels.  (binary-derived; raynorpat docs/menus.md)

### 1:2c590 menutext (drawmenu)
`/* Draw menu item text (big font) with the pointer line pulsing; returns the x after the last glyph. */`
- Any other character reuses the previous glyph tile.  (raynorpat docs/menus.md)
- Draws nothing in the 2D map view (display_mode 2).  (binary-derived)

### 1:2c6f0 probe
`/* Read menu keys/mouse for page m: -1 quit (Q), -2 back (Esc), n>0 line picked, 0 nothing. */`
- The flags are tested with / (always true), so every page accepts all three input groups.  (binary-derived (oracle has no flag tests))

### 1:2c880 draw_and_probe
`/* Draw a menu page (spinning title, items) and return its probe result. */`
- Title zoom: rotatesprite zoom (menu_zoom<<3)-4096, angle menu_zoom, so it spins while growing.  (binary-derived; raynorpat titlespin)

### 1:2ccf0 menus
`/* Run the current menu page for one frame (pages 0,1,2,10,20,30,40,50,60,100). */`
- Bit 7 of current_menu marks an opened page; opening resets the pointer to 160,100 and the title zoom.  (binary-derived)
- Page 0 opened in a level (gm bit 4) becomes the in-game menu, page 100.  (binary-derived; raynorpat docs/menus.md)

### 1:2e060 draw_pause_sign (draw_fade_transition)
`/* Draw the spinning PAUSE sign (tile 1139) while the game is paused. */`
- userdefs.paused is both the pause flag (set to 1) and the sign's zoom (grows to 8192).  (binary-derived)

### 1:2e0d0 vscrn
`/* Apply player snum's screen size to the view window (setview with a screen_size margin). */`

### 1:2e120 initlava
`/* Ken's lava/slime effect: precompute drop rings and the per-row wave ramp (K95 GAME.C initlava). */`
- Added special bitmapped effect which I either call lava or boiling slime.  (Ken Silverman, BUILD.TXT 9/29/94)

### 1:2e200 movelava
`/* Ken's lava/slime effect: one animation step of the 128x128 tile dapic (K95 GAME.C movelava). */`
- Back up dapic with 1 pixel extra on each boundary (to prevent anding for wrap-around)  (Ken Silverman, buil1995/GAME.C movelava)
- Only animate lava if its picnum is on screen  (Ken Silverman, buil1995/GAME.C drawscreen (caller; LameDuke tests waloff[49] ...)

### 1:2e6e0 resetparallax
`/* Reset the parallaxing sky (pskyoff[0]=0, pskybits=3). */`

### 1:2e6fa menues_text_end
`/* Linker/layout marker or private table, not a function (menues_text_end). */`

## src/game/gamedef.c (game_gamedef)

```c
/* GAMEDEF: the CON script compiler (loadefs/parsecommand -> script[] of 60-keyword bytecode) and interpreter (execute/parse/move) for actors.
   Earliest known form of 3D Realms' CON language (Todd Replogle; Duke Nukem 3D 1.5 GAMEDEF.C, 1996, is its descendant: 49 of LameDuke's 60 keyword slots keep number and role there, 45 with the same keyword).
   Lexer helpers ispecial/isaltok/getlabel/keyword are byte-identical to the 1996 text; the rest is BINARY_DERIVED with Duke 1.5 semantics. */
```

### 1:2e700 getincangle (getincangle_shift)
`/* Angle step from a toward na (Build 2048-unit circle), shifted right by shift. Later Duke: getincangle(a,na). */`
- LameDuke folds the shift into the routine; Duke 1.5 getincangle has no shift and callers write getincangle(a,na)>>n.  (comparison with Duke3D 1.5 GAMEDEF.C (1996))
- The wrap branch returns ((a+1024)&2047)-na, not Duke 1.5's re-centred difference; kept as in the binary.  (binary-derived (STRICT source))

### 1:2e740 ispecial
`/* True if c ends a label (LF, space, CR); LF advances line_number. */`

### 1:2e770 isaltok
`/* True if c may appear in a CON token (isalnum or { } / * - _ .). */`

### 1:2e7b0 getlabel
`/* Read the next label into label+(labelcnt<<6); the slot becomes permanent only if the caller increments labelcnt. */`

### 1:2e850 keyword
`/* Peek the next token and return its keyword number (-1 = not a keyword, 0 = end of text). */`
- Returns 0 (the 'end' opcode) at end of text, so operand lists also stop there.  (binary-derived)

### 1:2e920 transword
`/* Translate the next word: emit keyword number (or label value) at scriptptr and return it; -1 on EOF or undefined symbol. */`
- Returns its code #  (Duke3D 1.5 GAMEDEF.C transword (1996))
- LameDuke falls back to the label table; Duke 1.5 instead reports 'Expecting key word'.  (comparison with Duke3D 1.5 GAMEDEF.C)

### 1:2eae0 transnum
`/* Translate a numeric operand (label value or decimal literal) into script[]. */`
- The diagnostic keeps the original spelling 'Paramater' (Duke 1.5: 'Parameter').  (D3D.EXE CONST pool)

### 1:2ed60 parsecommand
`/* Compile one CON command into script[]. Returns 1 to end the enclosing loop (end, '}', EOF or too many errors). */`
- Leave a spot for the fail location  (Duke3D 1.5 GAMEDEF.C parsecommand (1996), on the if/else branch slot)
- The sound lookup scans 38 names: rpg.voc (39th) is played only from code.  (D3D.EXE 1:2f12e cmp ebx,0x98 / 1:2f136 cmp ecx,0x26)
- '}' returns 1 with no error even outside a block; 'enda' messages say 'aend'.  (binary-derived / D3D.EXE CONST pool)

### 1:301d0 passone
`/* Single compile pass over the loaded text. */`
- Tokenize  (Duke3D 1.5 GAMEDEF.C loadefs, comment on its passone() call (1996))

### 1:301e0 loadefs
`/* Load and compile GAME.CON (and its includes) into script[]; script[0] receives the end pointer. */`
- if (fd) is true for a failed open (-1): a missing game.con is not detected here.  (binary-derived)
- Three bootleg_cache_pop calls free text, label and labelcode after compiling.  (binary-derived)

### 1:303b0 furthestangle
`/* Pick the most open of angs evenly spaced headings around sprite i (longest hitscan). */`
- hitsect is passed for both the sector and the sprite output, as in the binary.  (binary-derived (oracle uses one stack slot))

### 1:304a0 move (move_actor)
`/* Apply sprite i's CON move and hitag move flags (Duke 1.5 move()). */`
- Flag order: on_owner, slow_to_stop, face_player, geth, getv, houver, stick_to_ceiling, stick_to_floor, face_to_player, rhr.  (binary-derived; names from assets/GAME.CON)
- execute also contains an inlined copy of this body.  (docs/units/gamedef.md)

### 1:308b0 parse
`/* Run one CON instruction for actor i (player p at distance x); nonzero ends the current block. */`
- An if operand is followed by the address of its false branch (Duke 1.5 parseifelse).  (binary-derived; Duke3D 1.5 GAMEDEF.C parseifelse)
- An opcode with no case (e.g. end's 0x80000000) exits the game with the actor's tile number.  (binary-derived)
- ifspawnedby compares the owner's sprite index with the operand (a tile number in GAME.CON).  (raynorpat docs/con-script.md)

### 1:32260 execute
`/* Execute sprite i's actor script (from actorscrptr[picnum]+4) for one game tick. */`
- Action timer: lotag += synctics; past action[4] (delay) actioncount++ and actionframe += action[3] (inc); wraps at frames*inc.  (binary-derived; Duke 1.5 execute same with TICSPERFRAME)

## src/engine/ENGINE.C, src/multi/multi.c

Engine and MULTI names, locals and structure are already Ken's (backported from his 1994/1995 sources). Suggested one-line headers from Ken's own documentation (buil1995/BUILD.TXT, Jun-95, first sentence, attributed):

- 1:33f18 `drawrooms`: /* This function draws the 3D screen to the current drawing page, which is not yet shown. (Ken, BUILD.TXT) */
- 1:39127 `loadboard`: /* Loads the given board file into memory for the BUILD engine. (Ken, BUILD.TXT) */
- 1:39337 `saveboard`: /* Saves the given board from memory inro the specified filename. (Ken, BUILD.TXT) */
- 1:39845 `setgamemode`: /* This function sets the video mode to 320*200*256color graphics. (Ken, BUILD.TXT) */
- 1:3a0c0 `initengine`: /* Sets up interrupt vectors for keyboard, and initializes many variables for the BUILD engine. (Ken, BUILD.TXT) */
- 1:3a459 `uninitengine`: /* Restores interrupt vectors for keyboard and timer, and frees buffers. (Ken, BUILD.TXT) */
- 1:3a534 `nextpage`: /* After a screen is prepared, use this function to view the screen. (Ken, BUILD.TXT) */
- 1:3aa03 `allocatepermanenttile`: /* This function allocates a place on the cache as permanent. (Ken, BUILD.TXT) */
- 1:3aac9 `loadtile`: /* This function will load the tile, tilenum, into the artwork cache. (Ken, BUILD.TXT) */
- 1:3abcb `loadpics`: /* Loads the given artwork file into memory for the BUILD engine. (Ken, BUILD.TXT) */
- 1:3aed0 `showengineinfo`: /* Use this function after setting to text mode to view some statics about the engine, such as frame ... (Ken, BUILD.TXT) */
- 1:3af4a `resettiming`: /* Resets timing, such as setting totalclock = 0. (Ken, BUILD.TXT) */
- 1:3b390 `screencapture`: /* Capture the screen and save it as a .BMP file. (Ken, BUILD.TXT) */
- 1:3b977 `inside`: /* Tests to see whether the overhead point (x, y) is inside sector (sectnum) Returns either 0 or 1, ... (Ken, BUILD.TXT) */
- 1:3ba86 `getangle`: /* Use this function call to determine the angle between two points. (Ken, BUILD.TXT) */
- 1:3bb6c `ksqrt`: /* A square root function optimized for integers. (Ken, BUILD.TXT) */
- 1:3bbe6 `copytilepiece`: /* Copies section of tile 1 (walnume1) with top-left corner (x1,y1) and rectangular size (xsiz, ysiz) to top-left corner ... (Ken, BUILD.TXT) */
- 1:3bd2b `printext`: /* Use this function to print text anywhere on the screen from a font that you can create in ... (Ken, BUILD.TXT) */
- 1:3bfdb `drawmasks`: /* This function draws all the sprites and masked walls to the current drawing page which is not yet ... (Ken, BUILD.TXT) */
- 1:3faa0 `permanentwritesprite`: /* - Added permanentwritesprite function for status bars or other sections of the screen that will not be overwritten ... (Ken, BUILD.TXT) */
- 1:40867 `setsprite`: /* This function simply sets the sprite's position to a specified coordinate (newx, newy, newz) without any checking to ... (Ken, BUILD.TXT) */
- 1:408c9 `movesprite`: /* This function moves the sprite given by spritenum by the 3 increments, xchange, ychange, and zchange. (Ken, BUILD.TXT) */
- 1:40b40 `insertsprite`: /* Whenever you insert a sprite, you must pass it the sector number, and a status number (statnum). (Ken, BUILD.TXT) */
- 1:40c8f `deletesprite`: /* Deletes the sprite. (Ken, BUILD.TXT) */
- 1:40e57 `changespritesect`: /* Changes the sector of sprite (spritenum) to the newsector (newsectnum). (Ken, BUILD.TXT) */
- 1:40ea0 `changespritestat`: /* Changes the status of sprite (spritenum) to status (newstatus). (Ken, BUILD.TXT) */
- 1:40ee9 `nextsectorneighborz`: /* This function searches z-coordinates of neighboring sectors to find the closest (next) ceiling starting at the given z-coordinate ... (Ken, BUILD.TXT) */
- 1:41016 `cansee`: /* This function determines whether or not two 3D points can "see" each other or not. (Ken, BUILD.TXT) */
- 1:41895 `neartag`: /* long neartagrange, //Choose maximum distance to scan (scale: 1024=largest grid size) char tagsearch) //1-lotag only, 2-hitag only, 3-lotag&hitag ... (Ken, BUILD.TXT) */
- 1:42122 `dragpoint`: /* This function will drag a point in the exact same way a point is dragged in 2D EDIT ... (Ken, BUILD.TXT) */
- 1:4221e `lastwall`: /* Use this function as a reverse function of wall[].point2. (Ken, BUILD.TXT) */
- 1:4225c `clipmove`: /* Moves any object (x, y, z) in any direction at any velocity and will make sure the object ... (Ken, BUILD.TXT) */
- 1:43797 `updatesector`: /* This function updates the sector number according to the x and y values passed to it. (Ken, BUILD.TXT) */
- 1:43861 `rotatepoint`: /* This function is a very convenient and fast math helper function. (Ken, BUILD.TXT) */
- 1:43f42 `qsetmode640350`: /* Set to the 2D map mode #1 (640*350*16) (Ken, BUILD.TXT) */
- 1:43ff8 `qsetmode640480`: /* Set to the 2D map mode #2 (640*480*16) (Ken, BUILD.TXT) */
- 1:4445e `draw2dscreen`: /* Draws the 2d screen - this function is a direct replacement for the drawrooms() and drawmasks() functions. (Ken, BUILD.TXT) */
- 1:45423 `getspritescreencoord`: /* This function returns the actual screen coordinates of a sprite. (Ken, BUILD.TXT) */
- 1:45647 `krand`: /* This simply returns a random number. (Ken, BUILD.TXT) */
- 1:46414 `precache`: /* This function will go through the tilenums of all sectors, walls, and sprites and call loadtile() on them. (Ken, BUILD.TXT) */
- 1:46c60 `rotatesprite`: /* (sx, sy) is the center of the sprite to draw defined as screen coordinates shifted up by 16. (Ken, BUILD.TXT) */
- 1:47239 `makepalookup`: /* This function allows different shirt colors for sprites. (Ken, BUILD.TXT) */

## src/audiolib/*

The Apogee Sound System sources already carry their original "Function:" header blocks (335 functions matched); no new comments proposed.
