# Game literal names

The proposal replaces 1,035 numeric operands with 385 additional definitions in `records.h`. The ledger distinguishes 399 changed kind/value records from existing symbols and table entries. Some names have roles in both the tile and weapon namespaces. All line and column numbers refer to the canonical source before replacement, at commit `feb9e31`.

## Applying the proposal

Run the script with its adjacent `names.json` from the repository root when the matching edits are ready.

```powershell
python -B build/workers/names/apply_names.py --dry-run src/game
python -B build/workers/names/apply_names.py src/game
python tools/gate.py
```

The script identifies each recorded operand using its source file, value and surrounding C tokens. It accepts moved lines, spacing changes and comments around the recorded operands. It treats quoted text as opaque and skips comments. It reports changed or ambiguous contexts and definition conflicts, returning 1 if any recorded sites could not be placed. A second application reports the already named sites and writes zero files. The adjacent `src/` is the verified example. GLOBAL is included there for the shared-header control.

`PROVEN_ORIGINAL` means the spelling or literal meaning comes from `DEFS.CON`, `GAME.CON`, the CON keyword table or the executable's VOC table. The `SND_` prefix and uppercase CON macro spellings are proposed C spellings. `STRONG` uses the same number and role in a later source or the Build v5 implementation. `NEUTRAL` records the kind and number with a one-line description of observed use. Menu strings establish page roles while their historical identifier spellings remain unknown. CON `GET_*` operands and native `inventory[]` slots use different orderings.

## Results by kind

| Kind | PROVEN_ORIGINAL | STRONG | NEUTRAL | Replacements |
|---|---:|---:|---:|---:|
| con_ifp | 15 | 0 | 0 | 15 |
| con_inventory | 5 | 0 | 0 | 5 |
| inventory | 0 | 0 | 5 | 24 |
| menu | 0 | 0 | 12 | 50 |
| se | 0 | 27 | 3 | 94 |
| se_mask | 0 | 0 | 2 | 7 |
| sector_lotag | 0 | 19 | 1 | 88 |
| sector_mask | 0 | 0 | 3 | 39 |
| sound | 27 | 0 | 0 | 58 |
| sprite_cstat | 0 | 6 | 7 | 56 |
| statnum | 0 | 8 | 0 | 33 |
| tile | 179 | 1 | 64 | 533 |
| wall_cstat | 0 | 7 | 2 | 19 |
| weapon_tile | 6 | 0 | 0 | 14 |
| **Total** | **232** | **68** | **99** | **1035** |

Existing entries are indexed in `names.json`, including all 60 CON opcodes, six player mode bits and all 39 VOC slots. Slots 4, 14 and 15 name VOCs absent from the archive. Slot 38 holds `rpg.voc` and is used by native firing. The executable's CON lookup scans slots 0 through 37. The prototype has no `definesound` command. The five original `MUSIC.CON` rows are included in the ledger observations.

## Map and art observations

- Tiles 263 and 264 use the bat frame base `BAT` (247) in `animatesprites`. Tile 263 is placed 17 times in M1 and seven times in M2. `DEFS.CON` gives no names for either 263 or 264, so both retain neutral names. See [bat contact sheet](render/overview.png) and [tile usage ledger](names.json).
- `FLOORTURRIT` and `CEILINGTURRIT` are the original spellings at tiles 930 and 931. L4 uses both, and N4 uses the floor turret. Tile 1820 retains `JELLYFISH`, the actor with brain-and-tentacle art. See [DEFS.CON](../../../external/x/lameduke/DEFS.CON) and [art](render/overview.png).
- SE 5 uses the locator and aiming logic found under the boss case in Duke 1.5. SE 27 tracks the player during demo playback in both versions. SE 23 in L6 and N4 changes water tiles and linked geometry. SE 129 has no shipped map placements and gets created by a plexiglass hit. SE 23 and 129 have neutral names. See [later ACTORS.C](../../../external/duke3d-1.3d/dukenukem3d/source/ACTORS.C) and [level census](../assets/LEVELS.md).
- L3 places three SE 1016 markers without a corresponding dispatch case. The `SONICGUNSPRITE` is placed in O1 through O4 and WAR1 through WAR2 and has no actor or native handler in the census. The red-tinted tile 1510 has no established runtime role and appears only in the observations. See [catalogue](names.json) and [art](render/overview.png).
- Tile 572 is a tiny transparent floor marker for the reflection effect. The same number and effect appear in `extras/WATER.C`. `WATERTILE` is the prototype's original tile 49 and is used by a separate animation. See [later WATER.C](../../../external/duke3d-1.3d/dukenukem3d/extras/WATER.C) and [DEFS.CON](../../../external/x/lameduke/DEFS.CON).

## DOS byte neutrality

The baseline and proposal were independently rebuilt in the default DOSBox-X host with the hash-pinned Watcom 9.5 compiler. `verify.py` runs the repository's `tools/match.py` entry point with the `config/units.json` profiles and stages inside this worker directory. Each complete `.OBJ` file was compared directly against its fresh baseline, including the _TEXT payload and tails, fixups, data, and CONST padding. The strict verifier output, including pre-existing mismatch details and all VERIFIED segment lines, also agreed.

| Unit | Profile | STRICT | STRICT bytes | VERIFIED segments, bytes | OMF file |
|---|---|---:|---:|---|---|
| game | game_oe | 35/49 | 22,092 | CONST 1,604, _DATA 96, _BSS 7 | Identical |
| premap | game_oe | 6/12 | 2,485 | CONST 96, _BSS 4 | Identical |
| sector | game_oe | 12/18 | 3,954 | No reported data segments | Identical |
| player | game_oe | 5/8 | 3,724 | _DATA 120 | Identical |
| actors | game_oe | 13/18 | 6,040 | No reported data segments | Identical |
| sounds | game_oe | 12/13 | 1,467 | CONST 688, _DATA 228, _BSS 524 | Identical |
| menues | game_oe | 9/11 | 3,418 | CONST 492, _DATA 500, _BSS 50,188 | Identical |
| gamedef | game_oe | 11/14 | 7,964 | CONST 1,598, _DATA 240, _BSS 15 | Identical |
| global | game | 0/0 | 0 | _DATA 1,116, _BSS 633,683 | Identical |

```powershell
python -B build/workers/names/verify.py baseline
python -B build/workers/names/verify.py candidate
```

The fresh DOS run recorded 103 STRICT functions and 51,144 STRICT bytes in these units. The reported VERIFIED data, CONST and BSS contributions were unchanged. Every proposed replacement passed the CONST check. Baseline and candidate logs are under [verify/](verify/). The smoke tests covered displaced sites, multiline operands, trivia preservation, idempotence, changed argument context and conflicting definitions. See [smoke log](verify/smoke.log).

## Proposed literal replacements

Each row summarizes the first evidence item. The complete evidence, tile render paths, map counts and each source token location are in [names.json](names.json). Later-source path prefixes in this table refer to the local `external/duke3d-1.3d/dukenukem3d/` checkout.

| Kind | Current | Proposed | Confidence | Sites | Evidence |
|---|---:|---|---|---:|---|
| con_ifp | 1 | `PSTANDING` | PROVEN_ORIGINAL | 1 | GAME.CON:26 (pstanding = 1) |
| con_ifp | 2 | `PWALKING` | PROVEN_ORIGINAL | 1 | GAME.CON:27 (pwalking = 2) |
| con_ifp | 4 | `PRUNNING` | PROVEN_ORIGINAL | 1 | GAME.CON:28 (prunning = 4) |
| con_ifp | 8 | `PDUCKING` | PROVEN_ORIGINAL | 1 | GAME.CON:29 (pducking = 8) |
| con_ifp | 16 | `PFALLING` | PROVEN_ORIGINAL | 1 | GAME.CON:30 (pfalling = 16) |
| con_ifp | 32 | `PJUMPING` | PROVEN_ORIGINAL | 1 | GAME.CON:31 (pjumping = 32) |
| con_ifp | 64 | `PHIGHER` | PROVEN_ORIGINAL | 1 | GAME.CON:32 (phigher = 64) |
| con_ifp | 128 | `PWALKINGBACK` | PROVEN_ORIGINAL | 1 | GAME.CON:33 (pwalkingback = 128) |
| con_ifp | 256 | `PRUNNINGBACK` | PROVEN_ORIGINAL | 1 | GAME.CON:34 (prunningback = 256) |
| con_ifp | 512 | `PLOOKINGLEFT` | PROVEN_ORIGINAL | 1 | GAME.CON:35 (plookingleft = 512) |
| con_ifp | 1024 | `PLOOKINGRIGHT` | PROVEN_ORIGINAL | 1 | GAME.CON:36 (plookingright = 1024) |
| con_ifp | 2048 | `PFACINGFRAMES` | PROVEN_ORIGINAL | 1 | GAME.CON:37 (pfacingframes = 2048) |
| con_ifp | 4096 | `PSHOOTING` | PROVEN_ORIGINAL | 1 | GAME.CON:38 (pshooting = 4096) |
| con_ifp | 8192 | `PONGROUND` | PROVEN_ORIGINAL | 1 | GAME.CON:39 (ponground = 8192) |
| con_ifp | 16384 | `PJETPACK` | PROVEN_ORIGINAL | 1 | GAME.CON:40 (pjetpack = 16384) |
| con_inventory | 0 | `GET_STEROIDS` | PROVEN_ORIGINAL | 1 | GAME.CON:1423 (GET_STEROIDS = 0) |
| con_inventory | 1 | `GET_SHIELD` | PROVEN_ORIGINAL | 1 | GAME.CON:1424 (GET_SHIELD = 1) |
| con_inventory | 2 | `GET_SCUBA` | PROVEN_ORIGINAL | 1 | GAME.CON:1425 (GET_SCUBA = 2) |
| con_inventory | 3 | `GET_HOLODUKE` | PROVEN_ORIGINAL | 1 | GAME.CON:1426 (GET_HOLODUKE = 3) |
| con_inventory | 4 | `GET_JETPACK` | PROVEN_ORIGINAL | 1 | GAME.CON:1427 (GET_JETPACK = 4) |
| inventory | 0 | `INVENTORY_0` | NEUTRAL | 5 | GAME.CON:1457. Native slot for Scuba (CON dispatch selects this array index). |
| inventory | 1 | `INVENTORY_1` | NEUTRAL | 5 | GAME.CON:1475. Native slot for Jetpack (CON dispatch selects this array index). |
| inventory | 2 | `INVENTORY_2` | NEUTRAL | 5 | GAME.CON:1439. Native slot for Steroids (CON dispatch selects this array index). |
| inventory | 3 | `INVENTORY_3` | NEUTRAL | 5 | GAME.CON:1448. Native slot for Shield (CON dispatch selects this array index). |
| inventory | 4 | `INVENTORY_4` | NEUTRAL | 4 | GAME.CON:1466. Native slot for Holoduke (CON dispatch selects this array index). |
| menu | 0 | `MENU_0` | NEUTRAL | 19 | src/game/menues.c:299. Main menu. |
| menu | 1 | `MENU_1` | NEUTRAL | 3 | src/game/menues.c:317. Episode selection. |
| menu | 2 | `MENU_2` | NEUTRAL | 5 | src/game/menues.c:330. Skill selection. |
| menu | 10 | `MENU_10` | NEUTRAL | 4 | src/game/menues.c:359. Restore a game. |
| menu | 20 | `MENU_20` | NEUTRAL | 4 | src/game/menues.c:376. Instructions and input display. |
| menu | 30 | `MENU_30` | NEUTRAL | 3 | src/game/menues.c:389. Game options. |
| menu | 40 | `MENU_40` | NEUTRAL | 2 | src/game/menues.c:403. Ordering info. |
| menu | 50 | `MENU_50` | NEUTRAL | 3 | src/game/menues.c:412. Save a game. |
| menu | 60 | `MENU_60` | NEUTRAL | 2 | src/game/menues.c:429. High scores. |
| menu | 100 | `MENU_100` | NEUTRAL | 2 | src/game/menues.c:437. Pause menu. |
| menu | 127 | `MENU_127` | NEUTRAL | 1 | src/game/menues.c:297. Mask that removes the menu initialization bit before dispatch. |
| menu | 128 | `MENU_128` | NEUTRAL | 2 | src/game/menues.c:287. Bit set when cursor and zoom state have been initialized. |
| se | 0 | `SE_ROTATING_SECTOR` | STRONG | 2 | Duke 1.5 source/ACTORS.C:4934 Recorded in 6 maps. |
| se | 1 | `SE_ROTATION_PIVOT` | STRONG | 2 | Duke 1.5 source/ACTORS.C:5090 Recorded in 6 maps. |
| se | 2 | `SE_EARTHQUAKE` | STRONG | 3 | Duke 1.5 source/ACTORS.C:5507 Recorded in 2 maps. |
| se | 3 | `SE_RANDOM_LIGHTS_AFTER_SHOT` | STRONG | 6 | Duke 1.5 source/ACTORS.C:5574 Recorded in 8 maps. |
| se | 4 | `SE_RANDOM_LIGHTS` | STRONG | 2 | Duke 1.5 source/ACTORS.C:5615 Recorded in 14 maps. |
| se | 5 | `SE_BOSS_MOVER` | STRONG | 4 | Duke 1.5 source/ACTORS.C:5672 Recorded in 2 maps. |
| se | 6 | `SE_SUBWAY` | STRONG | 4 | Duke 1.5 source/ACTORS.C:5108 Recorded in 3 maps. |
| se | 7 | `SE_TRANSPORT` | STRONG | 2 | Duke 1.5 source/ACTORS.C:2778-2784 (transporter status list) Recorded in 16 maps. |
| se | 8 | `SE_DOOR_LIGHT_UP` | STRONG | 3 | Duke 1.5 source/ACTORS.C:5752 Recorded in 9 maps. |
| se | 9 | `SE_DOOR_LIGHT_DOWN` | STRONG | 5 | Duke 1.5 source/ACTORS.C:5753 Recorded in 4 maps. |
| se | 10 | `SE_DOOR_AUTO_CLOSE` | STRONG | 1 | Duke 1.5 source/ACTORS.C:5823 Recorded in 22 maps. |
| se | 11 | `SE_SWINGING_DOOR` | STRONG | 4 | Duke 1.5 source/ACTORS.C:5856 Recorded in 4 maps. |
| se | 12 | `SE_LIGHT_SWITCH` | STRONG | 4 | Duke 1.5 source/ACTORS.C:5914 Recorded in 4 maps. |
| se | 13 | `SE_EXPLOSIVE_SECTOR` | STRONG | 3 | Duke 1.5 source/ACTORS.C:5982 Recorded in 12 maps. |
| se | 14 | `SE_SUBWAY_CAR` | STRONG | 4 | Duke 1.5 source/ACTORS.C:5150 Recorded in 2 maps. |
| se | 15 | `SE_SLIDING_DOOR` | STRONG | 4 | Duke 1.5 source/ACTORS.C:6047 Recorded in 2 maps. |
| se | 16 | `SE_TYPE_16` | NEUTRAL | 3 | Steps the ceiling and can delete its effector when its sector is empty. Recorded in 1 maps. |
| se | 17 | `SE_WARP_ELEVATOR` | STRONG | 8 | Duke 1.5 source/ACTORS.C:6111 Recorded in 8 maps. |
| se | 18 | `SE_INCREMENTAL_PLANE_MOTION` | STRONG | 2 | Duke 1.5 source/ACTORS.C:6227 Recorded in 2 maps. |
| se | 19 | `SE_EXPLOSION_LOWER_CEILING` | STRONG | 2 | Duke 1.5 source/ACTORS.C:6308 Recorded in 1 maps. |
| se | 20 | `SE_EXTENDING_BRIDGE` | STRONG | 3 | Duke 1.5 source/ACTORS.C:6400 Recorded in 1 maps. |
| se | 21 | `SE_CASCADING_PLANE` | STRONG | 4 | Duke 1.5 source/ACTORS.C:6466 Recorded in 3 maps. |
| se | 22 | `SE_TEETH_DOOR_FOLLOWER` | STRONG | 2 | Duke 1.5 source/ACTORS.C:6494 Recorded in 1 maps. |
| se | 23 | `SE_TYPE_23` | NEUTRAL | 3 | On damage, applies water tiles and changes heights across matching sector hitags. Recorded in 2 maps. |
| se | 24 | `SE_CONVEYOR` | STRONG | 2 | Duke 1.5 source/ACTORS.C:6504 Recorded in 2 maps. |
| se | 25 | `SE_PISTON` | STRONG | 3 | Duke 1.5 source/ACTORS.C:6626 Recorded in 3 maps. |
| se | 26 | `SE_MOVING_PLATFORM` | STRONG | 3 | Duke 1.5 source/ACTORS.C:6650 Recorded in 1 maps. |
| se | 27 | `SE_DEMO_CAMERA` | STRONG | 2 | Duke 1.5 source/ACTORS.C:6702 Recorded in 9 maps. |
| se | 128 | `SE_GLASS_BREAKAGE` | STRONG | 2 | Duke 1.5 source/ACTORS.C:7058 |
| se | 129 | `SE_TYPE_129` | NEUTRAL | 2 | No placements in the shipped v5 map census.. Temporary animator cycles the two plexiglass overlays after a hit. |
| se_mask | 255 | `SE_TAG_MASK_255` | NEUTRAL | 1 | Low byte checked when an extended bridge is operated. |
| se_mask | 32767 | `SE_TAG_MASK_32767` | NEUTRAL | 6 | Low fifteen bits checked for selected sector effectors. |
| sector_lotag | -1 | `LOTAG_END_LEVEL` | STRONG | 1 | Duke 1.5 source/SECTOR.C:2899 Recorded in 15 maps. |
| sector_lotag | 0 | `LOTAG_0` | NEUTRAL | 5 | Untagged sector in native transport and secret reset logic. Recorded in 29 maps. |
| sector_lotag | 1 | `LOTAG_ABOVE_WATER` | STRONG | 11 | Duke 1.5 source/ACTORS.C:2886 (movetransports) Recorded in 11 maps. |
| sector_lotag | 2 | `LOTAG_UNDERWATER` | STRONG | 12 | Duke 1.5 source/ACTORS.C:2905 (movetransports) Recorded in 9 maps. |
| sector_lotag | 9 | `LOTAG_SLIDING_STAR_DOOR` | STRONG | 3 | Duke 1.5 source/SECTOR.C:594 Recorded in 1 maps. |
| sector_lotag | 15 | `LOTAG_WARP_ELEVATOR` | STRONG | 5 | Duke 1.5 source/SECTOR.C:679 Recorded in 8 maps. |
| sector_lotag | 16 | `LOTAG_PLATFORM_DOWN` | STRONG | 4 | Duke 1.5 source/SECTOR.C:709 Recorded in 12 maps. |
| sector_lotag | 17 | `LOTAG_PLATFORM_UP` | STRONG | 4 | Duke 1.5 source/SECTOR.C:710 Recorded in 18 maps. |
| sector_lotag | 18 | `LOTAG_ELEVATOR_DOWN` | STRONG | 4 | Duke 1.5 source/SECTOR.C:734 Recorded in 5 maps. |
| sector_lotag | 19 | `LOTAG_ELEVATOR_UP` | STRONG | 4 | Duke 1.5 source/SECTOR.C:735 Recorded in 2 maps. |
| sector_lotag | 20 | `LOTAG_CEILING_DOOR` | STRONG | 3 | Duke 1.5 source/SECTOR.C:782 Recorded in 20 maps. |
| sector_lotag | 21 | `LOTAG_FLOOR_DOOR` | STRONG | 3 | Duke 1.5 source/SECTOR.C:820 Recorded in 3 maps. |
| sector_lotag | 22 | `LOTAG_SPLIT_DOOR` | STRONG | 4 | Duke 1.5 source/SECTOR.C:842 Recorded in 12 maps. |
| sector_lotag | 23 | `LOTAG_SWING_DOOR` | STRONG | 5 | Duke 1.5 source/SECTOR.C:866 Recorded in 4 maps. |
| sector_lotag | 25 | `LOTAG_SUBWAY_SLIDING_DOOR` | STRONG | 3 | Duke 1.5 source/SECTOR.C:906 Recorded in 2 maps. |
| sector_lotag | 26 | `LOTAG_SPLIT_STAR_DOOR` | STRONG | 5 | Duke 1.5 source/SECTOR.C:578 Recorded in 2 maps. |
| sector_lotag | 27 | `LOTAG_EXTENDING_BRIDGE` | STRONG | 4 | Duke 1.5 source/SECTOR.C:938 Recorded in 1 maps. |
| sector_lotag | 28 | `LOTAG_DROP_FLOOR` | STRONG | 3 | Duke 1.5 source/SECTOR.C:958 Recorded in 3 maps. |
| sector_lotag | 29 | `LOTAG_TEETH_DOOR` | STRONG | 3 | Duke 1.5 source/SECTOR.C:753 Recorded in 1 maps. |
| sector_lotag | 32767 | `LOTAG_SECRET` | STRONG | 2 | Duke 1.5 source/SECTOR.C:2895 Recorded in 1 maps. |
| sector_mask | 4095 | `LOTAG_MASK_4095` | NEUTRAL | 1 | Low twelve bits retained by the developer door-opening sweep. |
| sector_mask | 32767 | `LOTAG_MASK_32767` | NEUTRAL | 21 | Low fifteen bits retained when dispatching the sector type. |
| sector_mask | 32768 | `LOTAG_MASK_32768` | NEUTRAL | 17 | High bit toggled or inspected by the sector operator. |
| sound | 1 | `SND_SHOOTING` | PROVEN_ORIGINAL | 1 | D3D.EXE 2:006a8 VOC name table entry 1 (sound_names, SOUNDS literals) |
| sound | 2 | `SND_RUNNING` | PROVEN_ORIGINAL | 1 | D3D.EXE 2:006a8 VOC name table entry 2 (sound_names, SOUNDS literals) |
| sound | 5 | `SND_GLASS` | PROVEN_ORIGINAL | 1 | D3D.EXE 2:006a8 VOC name table entry 5 (sound_names, SOUNDS literals) |
| sound | 6 | `SND_DOOR1` | PROVEN_ORIGINAL | 12 | D3D.EXE 2:006a8 VOC name table entry 6 (sound_names, SOUNDS literals) |
| sound | 7 | `SND_KNUCKLE` | PROVEN_ORIGINAL | 1 | D3D.EXE 2:006a8 VOC name table entry 7 (sound_names, SOUNDS literals) |
| sound | 8 | `SND_JUMP` | PROVEN_ORIGINAL | 1 | D3D.EXE 2:006a8 VOC name table entry 8 (sound_names, SOUNDS literals) |
| sound | 9 | `SND_LAND` | PROVEN_ORIGINAL | 1 | D3D.EXE 2:006a8 VOC name table entry 9 (sound_names, SOUNDS literals) |
| sound | 10 | `SND_SCUBA` | PROVEN_ORIGINAL | 2 | D3D.EXE 2:006a8 VOC name table entry 10 (sound_names, SOUNDS literals) |
| sound | 13 | `SND_BODYBLOP` | PROVEN_ORIGINAL | 3 | D3D.EXE 2:006a8 VOC name table entry 13 (sound_names, SOUNDS literals) |
| sound | 17 | `SND_DRIPPING` | PROVEN_ORIGINAL | 3 | D3D.EXE 2:006a8 VOC name table entry 17 (sound_names, SOUNDS literals) |
| sound | 18 | `SND_JETPAKON` | PROVEN_ORIGINAL | 1 | D3D.EXE 2:006a8 VOC name table entry 18 (sound_names, SOUNDS literals) |
| sound | 19 | `SND_JETPAKI` | PROVEN_ORIGINAL | 2 | D3D.EXE 2:006a8 VOC name table entry 19 (sound_names, SOUNDS literals) |
| sound | 20 | `SND_FTA_HM` | PROVEN_ORIGINAL | 1 | D3D.EXE 2:006a8 VOC name table entry 20 (sound_names, SOUNDS literals) |
| sound | 21 | `SND_FTA_HF` | PROVEN_ORIGINAL | 1 | D3D.EXE 2:006a8 VOC name table entry 21 (sound_names, SOUNDS literals) |
| sound | 22 | `SND_FTA_R` | PROVEN_ORIGINAL | 1 | D3D.EXE 2:006a8 VOC name table entry 22 (sound_names, SOUNDS literals) |
| sound | 23 | `SND_LITEFLCK` | PROVEN_ORIGINAL | 4 | D3D.EXE 2:006a8 VOC name table entry 23 (sound_names, SOUNDS literals) Recorded in 1 maps. |
| sound | 24 | `SND_BIGLITE` | PROVEN_ORIGINAL | 3 | D3D.EXE 2:006a8 VOC name table entry 24 (sound_names, SOUNDS literals) |
| sound | 25 | `SND_SWITCH` | PROVEN_ORIGINAL | 4 | D3D.EXE 2:006a8 VOC name table entry 25 (sound_names, SOUNDS literals) |
| sound | 26 | `SND_REACTOR` | PROVEN_ORIGINAL | 3 | D3D.EXE 2:006a8 VOC name table entry 26 (sound_names, SOUNDS literals) |
| sound | 27 | `SND_JETPAKOF` | PROVEN_ORIGINAL | 1 | D3D.EXE 2:006a8 VOC name table entry 27 (sound_names, SOUNDS literals) |
| sound | 28 | `SND_RUNNINGW` | PROVEN_ORIGINAL | 2 | D3D.EXE 2:006a8 VOC name table entry 28 (sound_names, SOUNDS literals) |
| sound | 30 | `SND_REBOUND` | PROVEN_ORIGINAL | 1 | D3D.EXE 2:006a8 VOC name table entry 30 (sound_names, SOUNDS literals) |
| sound | 31 | `SND_GUN1` | PROVEN_ORIGINAL | 1 | D3D.EXE 2:006a8 VOC name table entry 31 (sound_names, SOUNDS literals) |
| sound | 32 | `SND_SUBWAY` | PROVEN_ORIGINAL | 4 | D3D.EXE 2:006a8 VOC name table entry 32 (sound_names, SOUNDS literals) |
| sound | 33 | `SND_CLIPIN` | PROVEN_ORIGINAL | 1 | D3D.EXE 2:006a8 VOC name table entry 33 (sound_names, SOUNDS literals) |
| sound | 34 | `SND_CLIPOUT` | PROVEN_ORIGINAL | 1 | D3D.EXE 2:006a8 VOC name table entry 34 (sound_names, SOUNDS literals) |
| sound | 38 | `SND_RPG` | PROVEN_ORIGINAL | 1 | D3D.EXE 2:006a8 VOC name table entry 38 (sound_names, SOUNDS literals) |
| sprite_cstat | 1 | `CSTAT_SPRITE_BLOCKING` | STRONG | 18 | src/engine/BUILD.H:96 |
| sprite_cstat | 2 | `CSTAT_SPRITE_TRANSLUCENT` | STRONG | 4 | src/engine/BUILD.H:100 |
| sprite_cstat | 4 | `CSTAT_SPRITE_XFLIP` | STRONG | 19 | src/engine/BUILD.H:101 |
| sprite_cstat | 8 | `CSTAT_SPRITE_YFLIP` | STRONG | 4 | src/engine/BUILD.H:102 |
| sprite_cstat | 9 | `CSTAT_SPRITE_9` | NEUTRAL | 1 | spawn uses this sprite cstat operand. |
| sprite_cstat | 12 | `CSTAT_SPRITE_12` | NEUTRAL | 2 | animatesprites uses this sprite cstat operand. |
| sprite_cstat | 14 | `CSTAT_SPRITE_14` | NEUTRAL | 1 | spawn uses this sprite cstat operand. |
| sprite_cstat | 16 | `CSTAT_SPRITE_WALL` | STRONG | 2 | src/engine/BUILD.H:103 |
| sprite_cstat | 34 | `CSTAT_SPRITE_34` | NEUTRAL | 1 | spawn uses this sprite cstat operand. |
| sprite_cstat | 64 | `CSTAT_SPRITE_ONE_SIDED` | STRONG | 1 | src/engine/ENGINE.C:3792,3968 |
| sprite_cstat | 65 | `CSTAT_SPRITE_65` | NEUTRAL | 1 | shoot uses this sprite cstat operand. |
| sprite_cstat | 128 | `CSTAT_SPRITE_128` | NEUTRAL | 1 | moveexplosions uses this sprite cstat operand. |
| sprite_cstat | 253 | `CSTAT_SPRITE_253` | NEUTRAL | 1 | moveplayers uses this sprite cstat operand. |
| statnum | 0 | `STAT_DEFAULT` | STRONG | 1 | Placed or inactive sprites. |
| statnum | 1 | `STAT_ACTOR` | STRONG | 2 | Active actor list. |
| statnum | 3 | `STAT_EFFECTOR` | STRONG | 4 | Sector effectors. |
| statnum | 4 | `STAT_PROJECTILE` | STRONG | 6 | Weapon projectile list. |
| statnum | 5 | `STAT_MISC` | STRONG | 15 | Debris, transient effects and explosions. |
| statnum | 9 | `STAT_TRANSPORT` | STRONG | 1 | Transporters. |
| statnum | 10 | `STAT_PLAYER` | STRONG | 2 | Player sprites and holoduke. |
| statnum | 1024 | `MAXSTATUS` | STRONG | 2 | Build's free sprite-list sentinel. |
| tile | 5 | `MUSICANDSFX` | PROVEN_ORIGINAL | 1 | DEFS.CON:1 (MUSICANDSFX = 5) Recorded in 7 maps. |
| tile | 23 | `ACTIVATORLOCKED` | PROVEN_ORIGINAL | 3 | DEFS.CON:2 (ACTIVATORLOCKED = 23) Recorded in 13 maps. |
| tile | 26 | `WALLLIGHT3` | PROVEN_ORIGINAL | 4 | DEFS.CON:3 (WALLLIGHT3 = 26) Recorded in 28 maps. |
| tile | 30 | `WATERPIPE` | PROVEN_ORIGINAL | 1 | DEFS.CON:4 (WATERPIPE = 30) Recorded in 4 maps. |
| tile | 31 | `ACTIVATOR` | PROVEN_ORIGINAL | 1 | DEFS.CON:5 (ACTIVATOR = 31) Recorded in 19 maps. |
| tile | 32 | `TOUCHPLATE` | PROVEN_ORIGINAL | 2 | DEFS.CON:6 (TOUCHPLATE = 32) Recorded in 16 maps. |
| tile | 34 | `LOCATORS` | PROVEN_ORIGINAL | 2 | DEFS.CON:7 (LOCATORS = 34) Recorded in 6 maps. |
| tile | 35 | `WALLLIGHT1` | PROVEN_ORIGINAL | 4 | DEFS.CON:8 (WALLLIGHT1 = 35) Recorded in 18 maps. |
| tile | 36 | `WALLLIGHT2` | PROVEN_ORIGINAL | 4 | DEFS.CON:9 (WALLLIGHT2 = 36) Recorded in 20 maps. |
| tile | 38 | `GUN2ASPRITE` | PROVEN_ORIGINAL | 4 | DEFS.CON:11 (GUN2ASPRITE = 38) Recorded in 21 maps. |
| tile | 46 | `RADIUSEXPLOSION` | PROVEN_ORIGINAL | 2 | DEFS.CON:12 (RADIUSEXPLOSION = 46) |
| tile | 49 | `WATERTILE` | PROVEN_ORIGINAL | 7 | DEFS.CON:13 (WATERTILE = 49) Recorded in 14 maps. |
| tile | 51 | `MASTERSWITCH` | PROVEN_ORIGINAL | 1 | DEFS.CON:14 (MASTERSWITCH = 51) Recorded in 3 maps. |
| tile | 57 | `SEENINE` | PROVEN_ORIGINAL | 5 | DEFS.CON:15 (SEENINE = 57) Recorded in 13 maps. |
| tile | 60 | `COKE` | PROVEN_ORIGINAL | 3 | DEFS.CON:16 (COKE = 60) Recorded in 10 maps. |
| tile | 70 | `DUDPIPE1` | PROVEN_ORIGINAL | 1 | DEFS.CON:17 (DUDPIPE1 = 70) Recorded in 2 maps. |
| tile | 80 | `DUDPIPE1HIT` | PROVEN_ORIGINAL | 1 | DEFS.CON:18 (DUDPIPE1HIT = 80) |
| tile | 97 | `TILE_97` | NEUTRAL | 2 | TILES000.ART tile 97, 12 x 15 pixels. First uppercase big-font glyph in ART; precache starts at this index. |
| tile | 159 | `SPINCOKE` | PROVEN_ORIGINAL | 2 | DEFS.CON:19 (SPINCOKE = 159) |
| tile | 174 | `FLAIR` | PROVEN_ORIGINAL | 2 | DEFS.CON:21 (FLAIR = 174) |
| tile | 175 | `SECTOREFFECTOR` | PROVEN_ORIGINAL | 3 | DEFS.CON:22 (SECTOREFFECTOR = 175) Recorded in 28 maps. |
| tile | 179 | `GLASS` | PROVEN_ORIGINAL | 1 | DEFS.CON:23 (GLASS = 179) Recorded in 4 maps. |
| tile | 180 | `VIEWSCREEN` | PROVEN_ORIGINAL | 1 | DEFS.CON:24 (VIEWSCREEN = 180) Recorded in 12 maps. |
| tile | 191 | `SHOTSPARK1` | PROVEN_ORIGINAL | 2 | DEFS.CON:26 (SHOTSPARK1 = 191) |
| tile | 212 | `WALLLIGHT4` | PROVEN_ORIGINAL | 4 | DEFS.CON:27 (WALLLIGHT4 = 212) Recorded in 14 maps. |
| tile | 214 | `LASERSITE` | PROVEN_ORIGINAL | 2 | DEFS.CON:28 (LASERSITE = 214) |
| tile | 215 | `WATERBUBBLE` | PROVEN_ORIGINAL | 3 | DEFS.CON:29 (WATERBUBBLE = 215) |
| tile | 216 | `WATERBUBBLEMAKER` | PROVEN_ORIGINAL | 1 | DEFS.CON:30 (WATERBUBBLEMAKER = 216) Recorded in 3 maps. |
| tile | 235 | `SCRAP1` | PROVEN_ORIGINAL | 7 | DEFS.CON:32 (SCRAP1 = 235) |
| tile | 239 | `SCRAP2` | PROVEN_ORIGINAL | 2 | DEFS.CON:33 (SCRAP2 = 239) |
| tile | 243 | `SCRAP3` | PROVEN_ORIGINAL | 2 | DEFS.CON:34 (SCRAP3 = 243) |
| tile | 246 | `TILE_246` | NEUTRAL | 1 | TILES000.ART tile 246, 21 x 19 pixels. Tile operand in moveexplosions (comparison or assignment). |
| tile | 247 | `BAT` | PROVEN_ORIGINAL | 3 | DEFS.CON:35 (BAT = 247) |
| tile | 263 | `TILE_263` | NEUTRAL | 3 | TILES001.ART tile 263, 18 x 27 pixels. Placed bat actor marker that draws frames from BAT; M1 has 17 and M2 has 7. Recorded in 2 maps. |
| tile | 264 | `TILE_264` | NEUTRAL | 1 | TILES001.ART tile 264, 14 x 27 pixels. Shared with 263 in animatesprites; no placed sprite in the v5 census. |
| tile | 265 | `CAMERA1` | PROVEN_ORIGINAL | 6 | DEFS.CON:36 (CAMERA1 = 265) Recorded in 9 maps. |
| tile | 266 | `TILE_266` | NEUTRAL | 1 | TILES001.ART tile 266, 21 x 26 pixels. Tile operand in spawn (switch case). Recorded in 8 maps. |
| tile | 267 | `TILE_267` | NEUTRAL | 1 | TILES001.ART tile 267, 23 x 25 pixels. Tile operand in spawn (switch case). Recorded in 6 maps. |
| tile | 268 | `TILE_268` | NEUTRAL | 1 | TILES001.ART tile 268, 20 x 24 pixels. Tile operand in spawn (switch case). Recorded in 5 maps. |
| tile | 269 | `TILE_269` | NEUTRAL | 1 | TILES001.ART tile 269, 14 x 23 pixels. Tile operand in spawn (switch case). Recorded in 1 maps. |
| tile | 273 | `DEADBAT` | PROVEN_ORIGINAL | 2 | DEFS.CON:37 (DEADBAT = 273) |
| tile | 275 | `CAMCORNER` | PROVEN_ORIGINAL | 1 | DEFS.CON:38 (CAMCORNER = 275) |
| tile | 276 | `TILE_276` | NEUTRAL | 1 | TILES001.ART tile 276, 36 x 29 pixels. Tile operand in cameratext (argument to myos). |
| tile | 277 | `TILE_277` | NEUTRAL | 1 | TILES001.ART tile 277, 36 x 29 pixels. Tile operand in cameratext (argument to myos). |
| tile | 278 | `TILE_278` | NEUTRAL | 1 | TILES001.ART tile 278, 36 x 29 pixels. Tile operand in cameratext (argument to myos). |
| tile | 279 | `TILE_279` | NEUTRAL | 1 | TILES001.ART tile 279, 7 x 7 pixels. Tile operand in cameratext (argument to myos). |
| tile | 283 | `APLAYER` | PROVEN_ORIGINAL | 11 | DEFS.CON:39 (APLAYER = 283) Recorded in 24 maps. |
| tile | 305 | `BOLT1` | PROVEN_ORIGINAL | 4 | DEFS.CON:40 (BOLT1 = 305) Recorded in 3 maps. |
| tile | 308 | `TILE_308` | NEUTRAL | 1 | TILES001.ART tile 308, 32 x 128 pixels. Tile operand in moveactors (comparison or assignment). |
| tile | 309 | `PARKINGMETERDEAD` | PROVEN_ORIGINAL | 1 | DEFS.CON:41 (PARKINGMETERDEAD = 309) |
| tile | 310 | `GPSPEED` | PROVEN_ORIGINAL | 1 | DEFS.CON:42 (GPSPEED = 310) Recorded in 6 maps. |
| tile | 311 | `EXPLODINGBARREL2` | PROVEN_ORIGINAL | 1 | DEFS.CON:43 (EXPLODINGBARREL2 = 311) |
| tile | 312 | `EXPLODINGBARREL3` | PROVEN_ORIGINAL | 1 | DEFS.CON:44 (EXPLODINGBARREL3 = 312) |
| tile | 313 | `DUDPIPE2` | PROVEN_ORIGINAL | 1 | DEFS.CON:45 (DUDPIPE2 = 313) Recorded in 3 maps. |
| tile | 314 | `DUDPIPE3` | PROVEN_ORIGINAL | 1 | DEFS.CON:46 (DUDPIPE3 = 314) Recorded in 1 maps. |
| tile | 315 | `DUDPIPE4` | PROVEN_ORIGINAL | 1 | DEFS.CON:47 (DUDPIPE4 = 315) Recorded in 1 maps. |
| tile | 316 | `DUDPIPE4HIT` | PROVEN_ORIGINAL | 1 | DEFS.CON:48 (DUDPIPE4HIT = 316) |
| tile | 323 | `STEAM` | PROVEN_ORIGINAL | 2 | DEFS.CON:49 (STEAM = 323) |
| tile | 327 | `CEILINGSTEAM` | PROVEN_ORIGINAL | 2 | DEFS.CON:50 (CEILINGSTEAM = 327) Recorded in 1 maps. |
| tile | 331 | `GLASS3` | PROVEN_ORIGINAL | 1 | DEFS.CON:51 (GLASS3 = 331) Recorded in 1 maps. |
| tile | 358 | `GLASSPIECE1` | PROVEN_ORIGINAL | 2 | DEFS.CON:52 (GLASSPIECE1 = 358) Recorded in 1 maps. |
| tile | 381 | `PARKINGMETER` | PROVEN_ORIGINAL | 1 | DEFS.CON:54 (PARKINGMETER = 381) Recorded in 1 maps. |
| tile | 389 | `BULLET` | PROVEN_ORIGINAL | 3 | DEFS.CON:55 (BULLET = 389) |
| tile | 395 | `SLOTDOOR` | PROVEN_ORIGINAL | 4 | DEFS.CON:56 (SLOTDOOR = 395) Recorded in 16 maps. |
| tile | 396 | `TILE_396` | NEUTRAL | 4 | TILES001.ART tile 396, 32 x 32 pixels. Adjacent SLOTDOOR image used by the door switch. |
| tile | 405 | `LIGHTSWITCH` | PROVEN_ORIGINAL | 5 | DEFS.CON:57 (LIGHTSWITCH = 405) Recorded in 8 maps. |
| tile | 406 | `TILE_406` | NEUTRAL | 4 | TILES001.ART tile 406, 32 x 32 pixels. Second state of the wall LIGHTSWITCH. Recorded in 1 maps. |
| tile | 408 | `FEMANOID` | PROVEN_ORIGINAL | 7 | DEFS.CON:58 (FEMANOID = 408) Recorded in 6 maps. |
| tile | 481 | `MAINMENU` | PROVEN_ORIGINAL | 1 | DEFS.CON:59 (MAINMENU = 481) |
| tile | 482 | `TITLE` | PROVEN_ORIGINAL | 1 | DEFS.CON:60 (TITLE = 482) |
| tile | 490 | `MANDROID` | PROVEN_ORIGINAL | 4 | DEFS.CON:62 (MANDROID = 490) Recorded in 4 maps. |
| tile | 563 | `RPG` | PROVEN_ORIGINAL | 1 | DEFS.CON:63 (RPG = 563) |
| tile | 570 | `MIRROR` | PROVEN_ORIGINAL | 5 | DEFS.CON:64 (MIRROR = 570) Recorded in 2 maps. |
| tile | 572 | `WATERREFLECTION` | STRONG | 3 | Duke 1.5 extras/WATER.C:49 Recorded in 3 maps. |
| tile | 573 | `GLASS2` | PROVEN_ORIGINAL | 5 | DEFS.CON:65 (GLASS2 = 573) Recorded in 13 maps. |
| tile | 579 | `DRONE1` | PROVEN_ORIGINAL | 5 | DEFS.CON:66 (DRONE1 = 579) Recorded in 15 maps. |
| tile | 606 | `TOILET` | PROVEN_ORIGINAL | 1 | DEFS.CON:67 (TOILET = 606) Recorded in 4 maps. |
| tile | 611 | `NUKEWARNINGICON` | PROVEN_ORIGINAL | 1 | DEFS.CON:69 (NUKEWARNINGICON = 611) |
| tile | 613 | `RESTOREGAME` | PROVEN_ORIGINAL | 1 | DEFS.CON:71 (RESTOREGAME = 613) |
| tile | 614 | `MOUSECURSOR` | PROVEN_ORIGINAL | 11 | DEFS.CON:72 (MOUSECURSOR = 614) |
| tile | 615 | `INSTRUCTIONS` | PROVEN_ORIGINAL | 1 | DEFS.CON:73 (INSTRUCTIONS = 615) |
| tile | 616 | `EPISODE` | PROVEN_ORIGINAL | 1 | DEFS.CON:74 (EPISODE = 616) |
| tile | 617 | `SKILL` | PROVEN_ORIGINAL | 1 | DEFS.CON:75 (SKILL = 617) |
| tile | 620 | `REACTOR` | PROVEN_ORIGINAL | 3 | DEFS.CON:76 (REACTOR = 620) Recorded in 3 maps. |
| tile | 623 | `TILE_623` | NEUTRAL | 1 | TILES002.ART tile 623, 69 x 108 pixels. Tile operand in moveactors (comparison or assignment). |
| tile | 625 | `TILE_625` | NEUTRAL | 1 | TILES002.ART tile 625, 75 x 108 pixels. Tile operand in moveactors (comparison or assignment). |
| tile | 627 | `EXPLODINGBARREL` | PROVEN_ORIGINAL | 3 | DEFS.CON:77 (EXPLODINGBARREL = 627) Recorded in 7 maps. |
| tile | 628 | `HORSELITE` | PROVEN_ORIGINAL | 1 | DEFS.CON:78 (HORSELITE = 628) Recorded in 6 maps. |
| tile | 631 | `DRUNKGUY1` | PROVEN_ORIGINAL | 2 | DEFS.CON:79 (DRUNKGUY1 = 631) Recorded in 1 maps. |
| tile | 643 | `RPGSECONDSET` | PROVEN_ORIGINAL | 1 | DEFS.CON:81 (RPGSECONDSET = 643) |
| tile | 664 | `WALLLIGHTBUST1` | PROVEN_ORIGINAL | 4 | DEFS.CON:82 (WALLLIGHTBUST1 = 664) Recorded in 1 maps. |
| tile | 665 | `WALLLIGHTBUST2` | PROVEN_ORIGINAL | 4 | DEFS.CON:83 (WALLLIGHTBUST2 = 665) Recorded in 1 maps. |
| tile | 670 | `WALLLIGHTBUST3` | PROVEN_ORIGINAL | 4 | DEFS.CON:84 (WALLLIGHTBUST3 = 670) Recorded in 3 maps. |
| tile | 671 | `WALLLIGHTBUST4` | PROVEN_ORIGINAL | 4 | DEFS.CON:85 (WALLLIGHTBUST4 = 671) Recorded in 1 maps. |
| tile | 677 | `OPERATOREYE` | PROVEN_ORIGINAL | 1 | DEFS.CON:86 (OPERATOREYE = 677) Recorded in 4 maps. |
| tile | 678 | `NOTCHON` | PROVEN_ORIGINAL | 1 | DEFS.CON:87 (NOTCHON = 678) |
| tile | 679 | `NOTCHOFF` | PROVEN_ORIGINAL | 1 | DEFS.CON:88 (NOTCHOFF = 679) |
| tile | 681 | `CRACKKNUCKLES` | PROVEN_ORIGINAL | 3 | DEFS.CON:90 (CRACKKNUCKLES = 681) |
| tile | 686 | `SMALLSMOKE` | PROVEN_ORIGINAL | 5 | DEFS.CON:91 (SMALLSMOKE = 686) |
| tile | 713 | `JETPACKICON` | PROVEN_ORIGINAL | 1 | DEFS.CON:93 (JETPACKICON = 713) |
| tile | 716 | `SCUBAGEARICON` | PROVEN_ORIGINAL | 1 | DEFS.CON:94 (SCUBAGEARICON = 716) |
| tile | 717 | `STEROIDSICON` | PROVEN_ORIGINAL | 1 | DEFS.CON:95 (STEROIDSICON = 717) |
| tile | 718 | `HOLODUKEICON` | PROVEN_ORIGINAL | 1 | DEFS.CON:96 (HOLODUKEICON = 718) |
| tile | 719 | `SHIELDICON` | PROVEN_ORIGINAL | 1 | DEFS.CON:97 (SHIELDICON = 719) |
| tile | 720 | `HOLODUKE` | PROVEN_ORIGINAL | 1 | DEFS.CON:98 (HOLODUKE = 720) Recorded in 4 maps. |
| tile | 721 | `HEAVYHBOMB` | PROVEN_ORIGINAL | 1 | DEFS.CON:99 (HEAVYHBOMB = 721) Recorded in 5 maps. |
| tile | 722 | `SIXPAK` | PROVEN_ORIGINAL | 1 | DEFS.CON:100 (SIXPAK = 722) Recorded in 6 maps. |
| tile | 723 | `AMMO` | PROVEN_ORIGINAL | 1 | DEFS.CON:101 (AMMO = 723) Recorded in 8 maps. |
| tile | 724 | `STEROIDS` | PROVEN_ORIGINAL | 1 | DEFS.CON:102 (STEROIDS = 724) Recorded in 3 maps. |
| tile | 726 | `SHIELD` | PROVEN_ORIGINAL | 1 | DEFS.CON:103 (SHIELD = 726) Recorded in 5 maps. |
| tile | 727 | `AIRTANK` | PROVEN_ORIGINAL | 1 | DEFS.CON:104 (AIRTANK = 727) |
| tile | 728 | `JETPACK` | PROVEN_ORIGINAL | 1 | DEFS.CON:105 (JETPACK = 728) Recorded in 5 maps. |
| tile | 737 | `W_LIGHT` | PROVEN_ORIGINAL | 1 | DEFS.CON:106 (W_LIGHT = 737) Recorded in 3 maps. |
| tile | 761 | `W_SCREENSTATIC` | PROVEN_ORIGINAL | 5 | DEFS.CON:107 (W_SCREENSTATIC = 761) Recorded in 12 maps. |
| tile | 762 | `TILE_762` | NEUTRAL | 1 | TILES002.ART tile 762, 64 x 64 pixels. Tile operand in animatewalls (switch case). |
| tile | 763 | `TILE_763` | NEUTRAL | 1 | TILES002.ART tile 763, 64 x 64 pixels. Tile operand in animatewalls (switch case). |
| tile | 764 | `W_L5` | PROVEN_ORIGINAL | 2 | DEFS.CON:108 (W_L5 = 764) Recorded in 1 maps. |
| tile | 765 | `W_M2` | PROVEN_ORIGINAL | 2 | DEFS.CON:109 (W_M2 = 765) Recorded in 2 maps. |
| tile | 766 | `W_L4` | PROVEN_ORIGINAL | 2 | DEFS.CON:110 (W_L4 = 766) Recorded in 1 maps. |
| tile | 767 | `W_L1` | PROVEN_ORIGINAL | 2 | DEFS.CON:111 (W_L1 = 767) Recorded in 3 maps. |
| tile | 790 | `PLEXIGLASS` | PROVEN_ORIGINAL | 3 | DEFS.CON:112 (PLEXIGLASS = 790) Recorded in 2 maps. |
| tile | 819 | `W_TECHWALL1` | PROVEN_ORIGINAL | 4 | DEFS.CON:113 (W_TECHWALL1 = 819) Recorded in 2 maps. |
| tile | 820 | `TILE_820` | NEUTRAL | 2 | TILES003.ART tile 820, 64 x 128 pixels. Tile operand in animatewalls (switch case). |
| tile | 821 | `TILE_821` | NEUTRAL | 2 | TILES003.ART tile 821, 64 x 128 pixels. Tile operand in animatewalls (switch case). |
| tile | 822 | `TILE_822` | NEUTRAL | 3 | TILES003.ART tile 822, 64 x 128 pixels. Tile operand in animatewalls (switch case). |
| tile | 823 | `W_TECHWALL2` | PROVEN_ORIGINAL | 4 | DEFS.CON:114 (W_TECHWALL2 = 823) Recorded in 4 maps. |
| tile | 824 | `TILE_824` | NEUTRAL | 2 | TILES003.ART tile 824, 64 x 128 pixels. Tile operand in animatewalls (switch case). |
| tile | 825 | `TILE_825` | NEUTRAL | 2 | TILES003.ART tile 825, 64 x 128 pixels. Tile operand in animatewalls (switch case). |
| tile | 826 | `TILE_826` | NEUTRAL | 3 | TILES003.ART tile 826, 64 x 128 pixels. Tile operand in animatewalls (switch case). |
| tile | 859 | `TILE_859` | NEUTRAL | 1 | TILES003.ART tile 859, 128 x 128 pixels. Tile operand in animatewalls (comparison or assignment). Recorded in 2 maps. |
| tile | 860 | `W_TECHWALL3` | PROVEN_ORIGINAL | 4 | DEFS.CON:115 (W_TECHWALL3 = 860) Recorded in 4 maps. |
| tile | 861 | `TILE_861` | NEUTRAL | 2 | TILES003.ART tile 861, 64 x 128 pixels. Tile operand in animatewalls (switch case). |
| tile | 862 | `TILE_862` | NEUTRAL | 2 | TILES003.ART tile 862, 64 x 128 pixels. Tile operand in animatewalls (switch case). |
| tile | 863 | `TILE_863` | NEUTRAL | 2 | TILES003.ART tile 863, 64 x 128 pixels. Tile operand in animatewalls (switch case). |
| tile | 864 | `W_TECHWALL4` | PROVEN_ORIGINAL | 4 | DEFS.CON:116 (W_TECHWALL4 = 864) Recorded in 3 maps. |
| tile | 865 | `TILE_865` | NEUTRAL | 2 | TILES003.ART tile 865, 64 x 128 pixels. Tile operand in animatewalls (switch case). |
| tile | 866 | `TILE_866` | NEUTRAL | 2 | TILES003.ART tile 866, 64 x 128 pixels. Tile operand in animatewalls (switch case). |
| tile | 867 | `TILE_867` | NEUTRAL | 3 | TILES003.ART tile 867, 64 x 128 pixels. Tile operand in animatewalls (switch case). |
| tile | 870 | `ARMPIECE` | PROVEN_ORIGINAL | 3 | DEFS.CON:117 (ARMPIECE = 870) |
| tile | 871 | `TILE_871` | NEUTRAL | 1 | TILES003.ART tile 871, 18 x 19 pixels. Tile operand in moveexplosions (comparison or assignment). |
| tile | 873 | `TILE_873` | NEUTRAL | 1 | TILES003.ART tile 873, 12 x 23 pixels. Tile operand in moveexplosions (comparison or assignment). |
| tile | 874 | `HEADPIECE` | PROVEN_ORIGINAL | 1 | DEFS.CON:118 (HEADPIECE = 874) |
| tile | 875 | `TILE_875` | NEUTRAL | 1 | TILES003.ART tile 875, 15 x 21 pixels. Tile operand in moveexplosions (comparison or assignment). |
| tile | 877 | `TILE_877` | NEUTRAL | 1 | TILES003.ART tile 877, 18 x 26 pixels. Tile operand in moveexplosions (comparison or assignment). |
| tile | 878 | `LEGPIECE` | PROVEN_ORIGINAL | 1 | DEFS.CON:119 (LEGPIECE = 878) |
| tile | 879 | `TILE_879` | NEUTRAL | 1 | TILES003.ART tile 879, 57 x 22 pixels. Tile operand in moveexplosions (comparison or assignment). |
| tile | 881 | `TILE_881` | NEUTRAL | 2 | TILES003.ART tile 881, 33 x 37 pixels. Tile operand in moveexplosions (comparison or assignment). |
| tile | 882 | `GUN2A` | PROVEN_ORIGINAL | 3 | DEFS.CON:120 (GUN2A = 882) |
| tile | 905 | `WOODENHORSE` | PROVEN_ORIGINAL | 3 | DEFS.CON:121 (WOODENHORSE = 905) Recorded in 2 maps. |
| tile | 925 | `DRONE1HIT` | PROVEN_ORIGINAL | 1 | DEFS.CON:122 (DRONE1HIT = 925) |
| tile | 930 | `FLOORTURRIT` | PROVEN_ORIGINAL | 5 | DEFS.CON:123 (FLOORTURRIT = 930) Recorded in 2 maps. |
| tile | 931 | `CEILINGTURRIT` | PROVEN_ORIGINAL | 4 | DEFS.CON:124 (CEILINGTURRIT = 931) Recorded in 1 maps. |
| tile | 940 | `BOUNCEMINE` | PROVEN_ORIGINAL | 2 | DEFS.CON:125 (BOUNCEMINE = 940) Recorded in 1 maps. |
| tile | 960 | `WATERDRIP` | PROVEN_ORIGINAL | 1 | DEFS.CON:126 (WATERDRIP = 960) Recorded in 1 maps. |
| tile | 961 | `GREENSLIME` | PROVEN_ORIGINAL | 4 | DEFS.CON:127 (GREENSLIME = 961) Recorded in 1 maps. |
| tile | 962 | `TILE_962` | NEUTRAL | 2 | TILES003.ART tile 962, 46 x 62 pixels. Tile operand in moveactors (comparison or assignment). |
| tile | 963 | `TILE_963` | NEUTRAL | 1 | TILES003.ART tile 963, 51 x 64 pixels. Tile operand in moveactors (comparison or assignment). |
| tile | 965 | `TILE_965` | NEUTRAL | 1 | TILES003.ART tile 965, 41 x 62 pixels. Tile operand in moveactors (comparison or assignment). |
| tile | 966 | `TILE_966` | NEUTRAL | 1 | TILES003.ART tile 966, 64 x 50 pixels. Tile operand in moveactors (comparison or assignment). |
| tile | 968 | `TILE_968` | NEUTRAL | 1 | TILES003.ART tile 968, 63 x 49 pixels. Upper endpoint of the native GREENSLIME tile range. |
| tile | 970 | `GLASSPIECES` | PROVEN_ORIGINAL | 2 | DEFS.CON:128 (GLASSPIECES = 970) |
| tile | 973 | `TILE_973` | NEUTRAL | 1 | TILES003.ART tile 973, 23 x 29 pixels. Tile operand in moveexplosions (comparison or assignment). |
| tile | 974 | `WOODENHORSEFALL` | PROVEN_ORIGINAL | 2 | DEFS.CON:129 (WOODENHORSEFALL = 974) Recorded in 1 maps. |
| tile | 1022 | `BIGFNTCURSOR` | PROVEN_ORIGINAL | 1 | DEFS.CON:130 (BIGFNTCURSOR = 1022) |
| tile | 1023 | `SMALLFNTCURSOR` | PROVEN_ORIGINAL | 1 | DEFS.CON:131 (SMALLFNTCURSOR = 1023) |
| tile | 1024 | `STARTALPHANUM` | PROVEN_ORIGINAL | 2 | DEFS.CON:132 (STARTALPHANUM = 1024) |
| tile | 1119 | `RPGAMMO` | PROVEN_ORIGINAL | 1 | DEFS.CON:133 (RPGAMMO = 1119) Recorded in 7 maps. |
| tile | 1120 | `BATTERYAMMO` | PROVEN_ORIGINAL | 2 | DEFS.CON:134 (BATTERYAMMO = 1120) Recorded in 13 maps. |
| tile | 1121 | `RPGSPRITE` | PROVEN_ORIGINAL | 2 | DEFS.CON:135 (RPGSPRITE = 1121) Recorded in 7 maps. |
| tile | 1122 | `OOZ` | PROVEN_ORIGINAL | 1 | DEFS.CON:136 (OOZ = 1122) |
| tile | 1123 | `DOORSHOCK` | PROVEN_ORIGINAL | 1 | DEFS.CON:137 (DOORSHOCK = 1123) Recorded in 3 maps. |
| tile | 1125 | `FIRSTGUNSPRITE` | PROVEN_ORIGINAL | 4 | DEFS.CON:139 (FIRSTGUNSPRITE = 1125) Recorded in 2 maps. |
| tile | 1127 | `ORDERINGINFO` | PROVEN_ORIGINAL | 1 | DEFS.CON:140 (ORDERINGINFO = 1127) |
| tile | 1128 | `OPTIONS` | PROVEN_ORIGINAL | 1 | DEFS.CON:141 (OPTIONS = 1128) |
| tile | 1129 | `HELPMENU` | PROVEN_ORIGINAL | 4 | DEFS.CON:142 (HELPMENU = 1129) |
| tile | 1130 | `TILE_1130` | NEUTRAL | 1 | TILES004.ART tile 1130, 41 x 24 pixels. Tile operand in cacheit (precache range start). |
| tile | 1132 | `BONUSSCREEN1` | PROVEN_ORIGINAL | 1 | DEFS.CON:143 (BONUSSCREEN1 = 1132) |
| tile | 1133 | `PLAYERWASHERE` | PROVEN_ORIGINAL | 1 | DEFS.CON:144 (PLAYERWASHERE = 1133) |
| tile | 1134 | `PLAYERISHERE` | PROVEN_ORIGINAL | 1 | DEFS.CON:145 (PLAYERISHERE = 1134) |
| tile | 1135 | `BONUSSCREEN2` | PROVEN_ORIGINAL | 1 | DEFS.CON:146 (BONUSSCREEN2 = 1135) |
| tile | 1136 | `SELECTDIR` | PROVEN_ORIGINAL | 3 | DEFS.CON:147 (SELECTDIR = 1136) |
| tile | 1137 | `SAVEGAME` | PROVEN_ORIGINAL | 1 | DEFS.CON:148 (SAVEGAME = 1137) |
| tile | 1138 | `HIGHSCORES` | PROVEN_ORIGINAL | 1 | DEFS.CON:149 (HIGHSCORES = 1138) |
| tile | 1139 | `PAUSE` | PROVEN_ORIGINAL | 4 | DEFS.CON:150 (PAUSE = 1139) |
| tile | 1143 | `HANDTHROW` | PROVEN_ORIGINAL | 2 | DEFS.CON:152 (HANDTHROW = 1143) |
| tile | 1150 | `FIRSTGUN` | PROVEN_ORIGINAL | 2 | DEFS.CON:153 (FIRSTGUN = 1150) |
| tile | 1160 | `FIRSTGUNRELOAD` | PROVEN_ORIGINAL | 3 | DEFS.CON:154 (FIRSTGUNRELOAD = 1160) |
| tile | 1161 | `TILE_1161` | NEUTRAL | 4 | TILES004.ART tile 1161, 139 x 148 pixels. Tile operand in displayweapon (argument to myos). |
| tile | 1162 | `FALLINGCLIP` | PROVEN_ORIGINAL | 1 | DEFS.CON:155 (FALLINGCLIP = 1162) |
| tile | 1163 | `CLIPINHAND` | PROVEN_ORIGINAL | 1 | DEFS.CON:156 (CLIPINHAND = 1163) |
| tile | 1164 | `TILE_1164` | NEUTRAL | 2 | TILES004.ART tile 1164, 98 x 80 pixels. Tile operand in displayweapon (argument to myos). |
| tile | 1165 | `SHELL` | PROVEN_ORIGINAL | 6 | DEFS.CON:157 (SHELL = 1165) |
| tile | 1170 | `BIGHOLE` | PROVEN_ORIGINAL | 3 | DEFS.CON:158 (BIGHOLE = 1170) |
| tile | 1174 | `CRANEPOLE` | PROVEN_ORIGINAL | 2 | DEFS.CON:159 (CRANEPOLE = 1174) Recorded in 4 maps. |
| tile | 1175 | `CRANE` | PROVEN_ORIGINAL | 3 | DEFS.CON:160 (CRANE = 1175) Recorded in 3 maps. |
| tile | 1176 | `TILE_1176` | NEUTRAL | 1 | TILES004.ART tile 1176, 44 x 50 pixels. Tile operand in movestandables (comparison or assignment). |
| tile | 1177 | `TILE_1177` | NEUTRAL | 2 | TILES004.ART tile 1177, 33 x 52 pixels. Tile operand in movestandables (comparison or assignment). |
| tile | 1178 | `TILE_1178` | NEUTRAL | 1 | TILES004.ART tile 1178, 0 x 0 pixels. Tile operand in moveeffectors (comparison or assignment). |
| tile | 1180 | `W_HITTECHWALL3` | PROVEN_ORIGINAL | 1 | DEFS.CON:161 (W_HITTECHWALL3 = 1180) |
| tile | 1181 | `W_HITTECHWALL4` | PROVEN_ORIGINAL | 1 | DEFS.CON:162 (W_HITTECHWALL4 = 1181) |
| tile | 1182 | `W_HITTECHWALL2` | PROVEN_ORIGINAL | 1 | DEFS.CON:163 (W_HITTECHWALL2 = 1182) |
| tile | 1183 | `W_HITTECHWALL1` | PROVEN_ORIGINAL | 1 | DEFS.CON:164 (W_HITTECHWALL1 = 1183) |
| tile | 1184 | `W_FORCEFIELD` | PROVEN_ORIGINAL | 3 | DEFS.CON:165 (W_FORCEFIELD = 1184) Recorded in 7 maps. |
| tile | 1185 | `TILE_1185` | NEUTRAL | 1 | TILES004.ART tile 1185, 64 x 64 pixels. Tile operand in prelevel (switch case). |
| tile | 1186 | `TILE_1186` | NEUTRAL | 1 | TILES004.ART tile 1186, 64 x 64 pixels. Tile operand in prelevel (switch case). |
| tile | 1190 | `WATERSPLASH` | PROVEN_ORIGINAL | 2 | DEFS.CON:166 (WATERSPLASH = 1190) |
| tile | 1200 | `CANWITHSOMETHING` | PROVEN_ORIGINAL | 2 | DEFS.CON:167 (CANWITHSOMETHING = 1200) Recorded in 2 maps. |
| tile | 1201 | `CANDESTROYED` | PROVEN_ORIGINAL | 2 | DEFS.CON:168 (CANDESTROYED = 1201) |
| tile | 1202 | `TILE_1202` | NEUTRAL | 2 | TILES004.ART tile 1202, 60 x 28 pixels. Tile operand in moveexplosions (comparison or assignment). |
| tile | 1203 | `TILE_1203` | NEUTRAL | 2 | TILES004.ART tile 1203, 62 x 33 pixels. Tile operand in moveexplosions (comparison or assignment). |
| tile | 1204 | `TILE_1204` | NEUTRAL | 2 | TILES004.ART tile 1204, 29 x 63 pixels. Tile operand in moveexplosions (comparison or assignment). |
| tile | 1205 | `TILE_1205` | NEUTRAL | 3 | TILES004.ART tile 1205, 24 x 63 pixels. Tile operand in moveexplosions (comparison or assignment). |
| tile | 1210 | `OTHERSHOTSPARK` | PROVEN_ORIGINAL | 2 | DEFS.CON:169 (OTHERSHOTSPARK = 1210) |
| tile | 1230 | `CLUB` | PROVEN_ORIGINAL | 2 | DEFS.CON:170 (CLUB = 1230) |
| tile | 1231 | `TILE_1231` | NEUTRAL | 1 | TILES004.ART tile 1231, 103 x 176 pixels. Tile operand in displayweapon (argument to myos). |
| tile | 1239 | `CLUBSPARK` | PROVEN_ORIGINAL | 1 | DEFS.CON:171 (CLUBSPARK = 1239) |
| tile | 1240 | `TILE_1240` | NEUTRAL | 1 | TILES004.ART tile 1240, 44 x 62 pixels. Tile operand in displayweapon (weapon display tile base). |
| tile | 1250 | `RPGGUN` | PROVEN_ORIGINAL | 1 | DEFS.CON:172 (RPGGUN = 1250) |
| tile | 1251 | `TILE_1251` | NEUTRAL | 1 | TILES004.ART tile 1251, 205 x 194 pixels. Tile operand in displayweapon (argument to myos). |
| tile | 1252 | `TILE_1252` | NEUTRAL | 2 | TILES004.ART tile 1252, 212 x 193 pixels. Tile operand in displayweapon (argument to myos). |
| tile | 1280 | `SHARK` | PROVEN_ORIGINAL | 5 | DEFS.CON:173 (SHARK = 1280) Recorded in 2 maps. |
| tile | 1320 | `W_NUMBERS` | PROVEN_ORIGINAL | 2 | DEFS.CON:174 (W_NUMBERS = 1320) Recorded in 4 maps. |
| tile | 1334 | `TILE_1334` | NEUTRAL | 1 | TILES005.ART tile 1334, 71 x 10 pixels. Tile operand in numtext_fixed (tile base argument to overwritesprite). Recorded in 1 maps. |
| tile | 1359 | `E_SPOT` | PROVEN_ORIGINAL | 2 | DEFS.CON:175 (E_SPOT = 1359) |
| tile | 1360 | `EXPLOSION2` | PROVEN_ORIGINAL | 5 | DEFS.CON:176 (EXPLOSION2 = 1360) |
| tile | 1370 | `EXPLOSION` | PROVEN_ORIGINAL | 3 | DEFS.CON:177 (EXPLOSION = 1370) |
| tile | 1380 | `TOPGUAGE` | PROVEN_ORIGINAL | 3 | DEFS.CON:178 (TOPGUAGE = 1380) |
| tile | 1381 | `BOTTOMGUAGE` | PROVEN_ORIGINAL | 1 | DEFS.CON:179 (BOTTOMGUAGE = 1381) |
| tile | 1392 | `STATUSNOTCH` | PROVEN_ORIGINAL | 5 | DEFS.CON:181 (STATUSNOTCH = 1392) |
| tile | 1398 | `INVENTORYBAR` | PROVEN_ORIGINAL | 1 | DEFS.CON:183 (INVENTORYBAR = 1398) |
| tile | 1399 | `INVENTORYNOTCH` | PROVEN_ORIGINAL | 5 | DEFS.CON:184 (INVENTORYNOTCH = 1399) |
| tile | 1400 | `BATTERYICON` | PROVEN_ORIGINAL | 1 | DEFS.CON:185 (BATTERYICON = 1400) |
| tile | 1401 | `CLIPICON` | PROVEN_ORIGINAL | 1 | DEFS.CON:186 (CLIPICON = 1401) |
| tile | 1402 | `HBOMBICON` | PROVEN_ORIGINAL | 1 | DEFS.CON:187 (HBOMBICON = 1402) |
| tile | 1403 | `RPGICON` | PROVEN_ORIGINAL | 1 | DEFS.CON:188 (RPGICON = 1403) |
| tile | 1404 | `STICKSPARK` | PROVEN_ORIGINAL | 3 | DEFS.CON:189 (STICKSPARK = 1404) |
| tile | 1405 | `TILE_1405` | NEUTRAL | 1 | TILES005.ART tile 1405, 76 x 76 pixels. Tile operand in cacheit (argument to loadtile). |
| tile | 1406 | `BLUESPARK` | PROVEN_ORIGINAL | 1 | DEFS.CON:190 (BLUESPARK = 1406) |
| tile | 1408 | `CAM1` | PROVEN_ORIGINAL | 2 | DEFS.CON:191 (CAM1 = 1408) Recorded in 1 maps. |
| tile | 1420 | `TILE_1420` | NEUTRAL | 1 | TILES005.ART tile 1420, 32 x 65 pixels. Tile operand in cacheit (precache range start). |
| tile | 1536 | `TROOP1` | PROVEN_ORIGINAL | 6 | DEFS.CON:192 (TROOP1 = 1536) Recorded in 13 maps. |
| tile | 1680 | `CAPTAIN` | PROVEN_ORIGINAL | 6 | DEFS.CON:193 (CAPTAIN = 1680) Recorded in 7 maps. |
| tile | 1780 | `TILE_1780` | NEUTRAL | 1 | TILES007.ART tile 1780, 50 x 39 pixels. Tile operand in cacheit (precache range start). |
| tile | 1820 | `JELLYFISH` | PROVEN_ORIGINAL | 5 | DEFS.CON:194 (JELLYFISH = 1820) Recorded in 3 maps. |
| wall_cstat | 1 | `CSTAT_WALL_BLOCKING` | STRONG | 1 | src/engine/BUILD.H:65 |
| wall_cstat | 2 | `CSTAT_WALL_SWAP_BOTTOM` | STRONG | 2 | src/engine/BUILD.H:66 |
| wall_cstat | 4 | `CSTAT_WALL_ALIGN_BOTTOM` | STRONG | 2 | src/engine/BUILD.H:67 |
| wall_cstat | 8 | `CSTAT_WALL_XFLIP` | STRONG | 2 | src/engine/BUILD.H:68 |
| wall_cstat | 16 | `CSTAT_WALL_MASKED` | STRONG | 2 | src/engine/BUILD.H:69 |
| wall_cstat | 32 | `CSTAT_WALL_ONE_WAY` | STRONG | 5 | src/engine/BUILD.H:70 |
| wall_cstat | 85 | `CSTAT_WALL_85` | NEUTRAL | 1 | operateforcefields uses this wall cstat operand. |
| wall_cstat | 128 | `CSTAT_WALL_TRANSLUCENT` | STRONG | 3 | src/engine/BUILD.H:72 |
| wall_cstat | 254 | `CSTAT_WALL_254` | NEUTRAL | 1 | animatewalls uses this wall cstat operand. |
| weapon_tile | 191 | `SHOTSPARK1` | PROVEN_ORIGINAL | 4 | DEFS.CON:26 (SHOTSPARK1 = 191) |
| weapon_tile | 389 | `BULLET` | PROVEN_ORIGINAL | 2 | DEFS.CON:55 (BULLET = 389) |
| weapon_tile | 563 | `RPG` | PROVEN_ORIGINAL | 3 | DEFS.CON:63 (RPG = 563) |
| weapon_tile | 721 | `HEAVYHBOMB` | PROVEN_ORIGINAL | 2 | DEFS.CON:99 (HEAVYHBOMB = 721) Recorded in 5 maps. |
| weapon_tile | 1140 | `HANDREMOTE` | PROVEN_ORIGINAL | 1 | DEFS.CON:151 (HANDREMOTE = 1140) |
| weapon_tile | 1230 | `CLUB` | PROVEN_ORIGINAL | 2 | DEFS.CON:170 (CLUB = 1230) |
