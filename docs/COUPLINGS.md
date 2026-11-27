# Couplings by oracle extent

This table covers every row of build/blockdiff.json frozen with the 3fae5a8 sources. The extent comes from the oracle side of blockdiff. For some unfinished functions the candidate ends elsewhere. “Reverse” reports STRICT helper anchors from the historical blocker census. “Possible” means that the source contains a same-unit call and the oracle has no direct call to the helper. Optimizer removal and nested expansion can also give this result. “Possible STRICT caller” tests the other direction. A direct external call has its own activation and uses independently bound entry addresses.

A carrier physically holds the following public’s jump table. Branch endpoints come from decoded direct oracle branches into another public, in either direction. The final column names STRICT functions with relative branches crossing this entry. Resizing an intervening body can change those displacements. A branch or table is a placement constraint. It does not establish incorrect source. A dash means no witness from these tests. [Oracle branch sites and targets](edges.json) · [Earlier first-blocker analysis](../../../docs/BLOCKER_CENSUS.md).

The starting census had one more NEAR public, screencapture (1,511 B, stack-slot displacement). The refreshed source is STRICT. The ENGINE and ACTORS integrations preserve the oracle branch targets.

## NEAR_MATCH

| Unit / public | Oracle B | Earlier first blocker | Reverse STRICT helper | Possible STRICT helper | Possible STRICT caller | Table boundary | Branch endpoint | STRICT branch across entry |
|---|---|---|---|---|---|---|---|---|
| game_player / processinput | 15692 | Stack-slot displacement | shoot | update_air | — | — | — | — |
| game_game / moveeffectors | 14016 | Frame reservation | FTA | — | — | — | — | — |
| game_actors / moveactors | 12360 | Switch-table target | ifhitbyweapon | guts, ifsquished | — | guts carries 40 B | — | — |
| game_sector / checksectors | 11448 | Switch-table target | — | — | — | checkhitceiling carries 264 B | — | — |
| game_sector / operatesectors | 10288 | Switch-table target | activatewarpelevators, checkcursectnums, setanimation | getanimationgoal | — | activatewarpelevators carries 84 B | — | — |
| game_premap / prelevel | 9992 | Switch-table target | resetprestat | — | — | resetprestat carries 120 B | — | — |
| engine / drawsprite | 9875 | Stack-slot displacement | — | — | — | — | printext | cansee, ceilspritescan, deletespritesect, deletespritestat, draw2dgrid, initspritelists, insertspritesect, insertspritestat, keepaway, makeradarang, permanentwritesprite, printext16, printext256, rotatepoint, setredbluepalette, setverts |
| game_premap / spawn | 8736 | Stack-slot displacement | — | — | — | — | — | — |
| game_sector / cheatkeys | 7604 | Switch-table target | operateforcefields | — | — | operateforcefields carries 144 B | — | — |
| game_game / main | 7472 | Instruction or access order | displaystatus, gametext, inittimer, playback | wait_for_players | — | — | — | — |
| game_gamedef / execute | 7424 | Switch-table target | move_actor | — | — | — | — | — |
| game_gamedef / parse | 6580 | Instruction or access order | — | getincangle | — | move_actor carries 264 B | — | — |
| game_sector / operateactivators | 4996 | Switch-table target | — | — | — | — | — | — |
| game_player / displayweapon | 4180 | Stack-slot displacement | — | — | — | animateknuckles carries 52 B | — | — |
| game_actors / moveexplosions | 3984 | Branch displacement | ssp | spawn_blood | — | — | — | — |
| game_actors / movestandables | 3735 | Frame reservation | ifhitbyweapon, ssp | — | — | — | — | — |
| engine / getcliplines | 3589 | Frame reservation | — | — | — | — | — | draw2dgrid, keepaway, makeradarang, printext16, printext256, rotatepoint, setredbluepalette, setverts |
| game_game / dobonus | 3536 | Stack-slot displacement | gametext, minitext | next_text_shade, numtext | — | — | — | — |
| game_game / displayrooms | 3268 | Switch-table target | animatesprites, draw_water_distortion | — | playback | draw_water_distortion carries 68 B | — | — |
| engine / draw2dscreen | 3202 | Frame reservation | — | — | — | — | printext | makeradarang, printext16, printext256, setredbluepalette, setverts |
| engine / getzrange | 2938 | Frame reservation | — | — | — | — | — | makeradarang, setredbluepalette, setverts |
| game_game / sync | 2832 | Branch displacement | getpackets | — | — | — | — | — |
| engine / grouvline | 2169 | Frame reservation | — | — | — | — | — | cansee, deletespritesect, deletespritestat, draw2dgrid, initspritelists, insertspritesect, insertspritestat, keepaway, makeradarang, printext16, printext256, rotatepoint, setredbluepalette, setverts |
| game_game / nonsharedkeys | 2080 | Stack-slot displacement | gametext | gamenumber | — | — | — | — |
| game_game / waitforeverybody | 1952 | Branch displacement | getpackets | — | — | — | — | — |
| engine / drawmaskwall | 1890 | Instruction or access order | — | — | — | — | drawmasks | cansee, ceilspritescan, deletespritesect, deletespritestat, draw2dgrid, initspritelists, insertspritesect, insertspritestat, keepaway, makeradarang, permanentwritesprite, printext16, printext256, rotatepoint, setredbluepalette, setverts |
| engine / hitscan | 1745 | Stack-slot displacement | — | — | — | — | — | draw2dgrid, keepaway, makeradarang, printext16, printext256, rotatepoint, setredbluepalette, setverts |
| game_sector / checkhitswitch | 1740 | Equal-savings conflict | — | operateforcefields, operatemasterswitches | — | — | — | — |
| engine / neartag | 1644 | Stack-slot displacement | — | — | — | — | clipmove | draw2dgrid, keepaway, makeradarang, printext16, printext256, rotatepoint, setredbluepalette, setverts |
| engine / drawline16 | 1541 | Register choice | — | — | — | — | printext | draw2dgrid, makeradarang, printext16, printext256, setredbluepalette, setverts |
| engine / rotatesprite | 1497 | Frame reservation | — | — | — | — | printext | setredbluepalette, setverts |
| multi / getpacket | 1424 | Branch displacement | — | — | — | — | netinitconnection | processreservedmessage |
| ass_al_midi / AL_SetVoiceTimbre | 1112 | Register choice | — | AL_SendOutput | — | — | — | — |
| multi / neton | 1079 | Instruction or access order | — | — | — | — | — | — |
| engine / overwritesprite | 1063 | Instruction or access order | — | — | — | — | wallscan | cansee, deletespritesect, deletespritestat, draw2dgrid, initspritelists, insertspritesect, insertspritestat, keepaway, makeradarang, permanentwritesprite, printext16, printext256, rotatepoint, setredbluepalette, setverts |
| multi / sendpacket | 1054 | Branch displacement | — | — | sendlogoff | — | — | processreservedmessage |
| game_game / strget | 1024 | Stack-slot displacement | gametext, minitext, myos | — | — | — | — | — |
| game_game / displayhighscores | 1024 | Stack-slot displacement | minitext, next_text_shade | numtext | — | — | — | — |
| game_menues / draw_and_probe | 896 | Instruction or access order | menutext, probe | — | menus | carries menus table (240 B) | — | — |
| engine / scansector | 756 | Register choice | — | — | — | — | drawmasks | drawrooms |
| engine / raytrace | 613 | Instruction or access order | — | — | — | — | wallscan | draw2dgrid, makeradarang, printext16, printext256, rotatepoint, setredbluepalette, setverts |
| engine / makepalookup | 599 | Stack-slot displacement | — | — | — | — | printext | setredbluepalette, setverts |
| multi / netsend | 593 | Saved-register set | — | — | — | — | — | — |
| game_player / spawn_pistol_tracer | 592 | Register choice | — | — | — | — | — | — |
| game_game / operatefta | 560 | Stack-slot displacement | minitext | — | playback | — | — | — |
| engine / wallfront | 554 | Register choice | — | — | — | — | addboxlinum, ceilspritescan | drawrooms |
| ass_multivoc / MV_SetMixMode | 525 | Register choice | — | — | — | MV_Pan3D carries 16 B | MV_GetNextVOCBlock, MV_StartPlayback | MV_AllocVoice, MV_DeleteDeadVoices, MV_Kill, MV_ServiceVoc |
| ass_multivoc / MV_PlayVOC | 523 | Frame reservation | — | — | — | — | — | MV_CalcPanTable, MV_CalcVolume |
| ass_al_midi / AL_Init | 518 | Saved-register set | — | — | — | — | — | — |
| ass_fx_man / FX_SetupCard | 488 | Switch-table target | — | — | — | carries FX_Init table (48 B) | — | — |
| game_game / lotsofglass | 480 | Stack-slot displacement | EGS | — | — | — | — | — |
| game_sounds / spritesound | 464 | Instruction or access order | — | xyzsound | — | — | — | — |
| game_game / update_menu_cursor | 448 | Branch displacement | myos | next_text_shade | — | — | — | — |
| ass_guswave / GUSWAVE_GetNextVOCBlock | 436 | Switch-table target | — | — | — | GUSWAVE_AllocVoice carries 40 B | — | — |
| ass_guswave / GUSWAVE_PlayVOC | 372 | Register choice | — | — | — | — | — | — |
| game_actors / ms | 368 | Stack-slot displacement | — | — | — | — | — | — |
| ass_al_midi / AL_SetVoiceVolume | 280 | Instruction or access order | — | AL_SendOutput | — | — | — | — |
| ass_task_man / TS_Startup | 192 | Saved-register set | — | — | — | — | — | — |
| multi / comsend | 141 | Register choice | — | — | — | — | — | — |
| ass_task_man / TS_SetTimerToMaxTaskRate | 60 | Instruction or access order | — | — | — | — | — | — |

## CODE_BYTES_MATCH

| Unit / public | Oracle B | Earlier first blocker | Reverse STRICT helper | Possible STRICT helper | Possible STRICT caller | Table boundary | Branch endpoint | STRICT branch across entry |
|---|---|---|---|---|---|---|---|---|
| ass_gusmidi / GUS_GetPatchMap | 508 | CONST contents | — | — | — | — | — | — |
| ass_gusmidi / GUSMIDI_LoadPatch | 464 | CONST contents | — | — | — | — | — | — |
| ass_blastold / BLASTER_GetEnv | 444 | CONST contents | — | — | — | — | — | — |
| engine / setvesamode | 292 | BSS target verification | — | — | — | — | drawmasks | setredbluepalette, setverts |
| ass_fx_man / FX_ErrorString | 228 | CONST contents | — | — | — | — | — | — |
| ass_sndsrc / SS_Init | 220 | CONST contents | — | SS_Shutdown | — | — | — | — |
| ass_blastold / BLASTER_ErrorString | 212 | CONST contents | — | — | — | BLASTER_CardConfig carries 68 B | — | — |
| ass_sndsrc / SS_DetectSoundSource | 212 | CONST contents | — | — | — | — | — | — |
| ass_music / MUSIC_ErrorString | 176 | CONST contents | — | — | — | carries MUSIC_Init table (48 B) | — | — |
| ass_pas16 / PAS_ErrorString | 156 | CONST contents | — | — | — | PAS_Interrupts carries 48 B | — | — |
| ass_guswave / GUSWAVE_ErrorString | 116 | CONST contents | — | — | — | GUSWAVE_PanTable carries 32 B | — | — |
| ass_gusmidi / GUS_ErrorString | 100 | CONST contents | — | — | — | — | — | — |
| ass_adlibfx / ADLIBFX_ErrorString | 84 | CONST contents | — | — | — | — | — | — |
| ass_pcfx / PCFX_ErrorString | 84 | CONST contents | — | — | — | — | — | — |
| ass_sndsrc / SS_ErrorString | 80 | CONST contents | — | — | — | — | — | — |


## Definition-removal tests by NEAR public

Numbers give direct call or jump sites in the candidate before and after replacing the named callee body by its declaration. Both objects were compiled on the canonical DOS host. An increase supports a same-unit dependency and can include nested expansion. An unchanged count can still hide a change to an argument, a local allocation, or an inlining decision elsewhere. See [all sites and reduced extents](REDUCTIONS.md).

| Unit / NEAR | STRICT callee removed | STRICT caller when this NEAR definition is removed |
|---|---|---|
| game_player / processinput | shoot 0→5, update_air 0→1 | — |
| game_game / moveeffectors | FTA 0→2, LocateTheLocator 4→4 | domovethings 1→1 |
| game_actors / moveactors | guts 0→1, ifhitbyweapon 0→5, ifsquished 0→1, addweapon 1→1, hitradius 1→1 | — |
| game_sector / checksectors | — | — |
| game_sector / operatesectors | activatewarpelevators 0→4, checkcursectnums 0→1, getanimationgoal 0→4, setanimation 0→20 | — |
| game_premap / prelevel | resetprestat 0→1 | enterlevel 1→1 |
| engine / drawsprite | animateoffs 1→1, maskwallscan 1→1, transmaskwallscan 1→1, wallmost 2→2, ceilspritescan 1→1, loadtile 2→2, spritewallfront 1→1 | drawmasks 3→3 |
| game_premap / spawn | — | — |
| game_sector / cheatkeys | operateforcefields 0→1 | — |
| game_game / main | domovethings 2→3, gametext 1→2, inittimer 0→1, playback 0→1, wait_for_players 0→1, cameratext 1→2, checkcommandline 1→1, closedemowrite 1→1, displaystatus 1→2, gameexit 4→4, genspriteremaps 1→1, next_text_shade 1→2, read_userdefs 1→1, write_userdefs 1→1 | — |
| game_gamedef / execute | move_actor 0→1 | — |
| game_gamedef / parse | getincangle 0→1, furthestangle 1→1 | — |
| game_sector / operateactivators | — | — |
| game_player / displayweapon | animateknuckles 1→1 | — |
| game_actors / moveexplosions | spawn_blood 0→3, ssp 0→6 | — |
| game_actors / movestandables | ifhitbyweapon 0→3, ssp 0→1 | — |
| engine / getcliplines | addclipline 16→16, clipinsideboxline 2→2 | clipmove 1→1 |
| game_game / dobonus | minitext 6→15, gametext 0→2, next_text_shade 0→1, numtext 0→6 | — |
| game_game / displayrooms | animatesprites 0→1, draw_water_distortion 0→1 | playback 0→1 |
| engine / draw2dscreen | clear2dscreen 1→1, draw2dgrid 1→1 | — |
| engine / getzrange | clipinsideboxline 5→5 | — |
| game_game / sync | getpackets 0→2, domovethings 2→3 | — |
| engine / grouvline | loadtile 2→2 | calcwalls 1→1 |
| game_game / nonsharedkeys | gametext 2→5, gamenumber 0→2 | playback 1→1 |
| game_game / waitforeverybody | getpackets 0→2 | — |
| engine / drawmaskwall | animateoffs 1→1, maskwallscan 1→1, transmaskwallscan 1→1, wallmost 2→2 | drawmasks 2→2 |
| engine / hitscan | lintersect 1→1 | — |
| game_sector / checkhitswitch | operateforcefields 0→2, operatemasterswitches 0→2 | — |
| engine / neartag | lintersect 1→1 | — |
| engine / drawline16 | — | draw2dgrid 1→1 |
| engine / rotatesprite | animateoffs 1→1, loadtile 1→1 | — |
| multi / getpacket | getcrc 1→1, processreservedmessage 2→2 | — |
| ass_al_midi / AL_SetVoiceTimbre | AL_SendOutput 0→14, AL_SendOutputToPort 30→30 | AL_NoteOn 1→1 |
| multi / neton | allocbicom 1→1, convalloc32 1→1, netinitconnection 1→1, simulateint 2→2 | initmultiplayers 1→1 |
| engine / overwritesprite | loadtile 1→1 | — |
| multi / sendpacket | getcrc 1→1 | processreservedmessage 3→3, sendlogon 1→1, sendlogoff 0→0 |
| game_game / strget | minitext 0→1, gametext 0→1, myos 0→1 | — |
| game_game / displayhighscores | minitext 2→6, next_text_shade 0→1, numtext 0→2 | — |
| game_menues / draw_and_probe | menutext 0→1, probe 0→1 | menus 0→10 |
| engine / scansector | addboxlinum 1→1 | drawrooms 1→1, drawalls 1→1 |
| engine / raytrace | — | clipmove 1→1 |
| engine / makepalookup | initcache 1→1 | — |
| multi / netsend | simulateint 1→1 | — |
| game_player / spawn_pistol_tracer | — | — |
| game_game / operatefta | minitext 0→2 | playback 0→1 |
| engine / wallfront | — | bunchfront 2→2 |
| ass_multivoc / MV_SetMixMode | — | MV_Init 1→1 |
| ass_multivoc / MV_PlayVOC | MV_AllocVoice 1→1, MV_GetNextVOCBlock 1→1 | MV_PlayVOC3D 1→1 |
| ass_al_midi / AL_Init | AL_CalcPitchInfo 1→1, AL_Reset 1→1, AL_ResetVoices 1→1 | — |
| ass_fx_man / FX_SetupCard | — | — |
| game_game / lotsofglass | EGS 0→1 | — |
| game_sounds / spritesound | xyzsound 0→1 | — |
| game_game / update_menu_cursor | myos 0→1, next_text_shade 0→1 | — |
| ass_guswave / GUSWAVE_GetNextVOCBlock | — | GUSWAVE_CallBack 1→1 |
| ass_guswave / GUSWAVE_PlayVOC | GUSWAVE_AllocVoice 1→1 | — |
| game_actors / ms | — | — |
| ass_al_midi / AL_SetVoiceVolume | AL_SendOutput 0→0, AL_SendOutputToPort 2→2 | AL_SetChannelVolume 1→1, AL_NoteOn 1→1 |
| ass_task_man / TS_Startup | TS_AllocateTimerStack 1→1, TS_LockMemory 1→1, TS_UnlockMemory 2→2 | TS_ScheduleTask 1→1 |
| multi / comsend | — | — |
| ass_task_man / TS_SetTimerToMaxTaskRate | TS_SetClockSpeed 1→1 | TS_Terminate 1→1, TS_SetTaskRate 1→1 |
