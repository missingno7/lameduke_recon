# Status

Updated 2026-11-26 (generated from build/verify.json by the integrator). Denominator fixed: obj1 = 443,461 bytes.
Run `python tools/gate.py` then `python tools/status.py` for live numbers.

| Unit | Functions | STRICT | STRICT bytes |
|---|---:|---:|---:|
| a_asm | 20 | 20 | 3,520 |
| engine | 102 | 65 | 31,720 |
| ass_adlibfx | 13 | 12 | 1,473 |
| ass_al_midi | 26 | 22 | 2,948 |
| ass_awe32 | 13 | 12 | 2,032 |
| ass_blastold | 51 | 48 | 6,200 |
| ass_dma | 7 | 5 | 752 |
| ass_dpmi | 7 | 6 | 324 |
| ass_fx_man | 15 | 12 | 2,184 |
| ass_gmtimbre | 0 | 0 | 0 |
| ass_gus | 4 | 3 | 406 |
| ass_gusmidi | 14 | 11 | 598 |
| ass_guswave | 18 | 13 | 1,240 |
| ass_irq | 5 | 4 | 756 |
| ass_ll_man | 5 | 4 | 156 |
| ass_midi | 26 | 25 | 4,132 |
| ass_mpu401 | 15 | 14 | 492 |
| ass_multivoc | 36 | 31 | 6,115 |
| ass_music | 22 | 21 | 1,649 |
| ass_pas16 | 42 | 40 | 5,548 |
| ass_pcfx | 14 | 12 | 1,040 |
| ass_pitch | 5 | 4 | 348 |
| ass_sndsrc | 17 | 13 | 1,032 |
| ass_task_man | 18 | 14 | 1,576 |
| ass_user | 2 | 1 | 92 |
| game_sounds | 13 | 12 | 1,467 |
| game_gamedef | 14 | 9 | 1,712 |
| multi | 25 | 18 | 3,164 |
| multi_data | 0 | 0 | 0 |
| game_menues | 11 | 6 | 1,146 |
| game_sector | 18 | 9 | 2,462 |
| game_player | 8 | 1 | 304 |
| game_game | 61 | 25 | 6,749 |
| game_actors | 18 | 7 | 1,552 |

Totals: 499 STRICT / 665 functions, 94,889 bytes; LameDuke 172 STRICT (53,796 bytes); Apogee Sound System 327 (41,093 bytes).

Identified, not reconstructed: Watcom 9.5b runtime (docs/RUNTIME_CENSUS.md), Creative AWE32 and Gravis GF1 vendor objects (docs/AUDIOLIB.md).

## Code map (obj1)
| Range | Contents | Source |
|---|---|---|
| 00000-00010 | cstart BEGTEXT | library |
| 00010-0f160 | GAME (profile game_oe) | src/game/game.c |
| 0f160-14fad | PREMAP (name hypothesis; profile game_oe) | src/game/premap.c |
| 14fb0-1f280 | SECTOR | src/game/sector.c |
| 1f280-25160 | PLAYER | src/game/player.c |
| 25160-2ba30 | ACTORS | src/game/actors.c |
| 2ba30-2c1c0 | SOUNDS | src/game/sounds.c |
| 2c1c0-2e700 | MENUES | src/game/menues.c |
| 2e700-33e5c | GAMEDEF | src/game/gamedef.c |
| 33e5c-47abf | ENGINE (Build) | src/engine/ENGINE.C |
| 47abf-49c84 | MULTI (Ken) | src/multi/multi.c (+ realmode.asm, provisional) |
| 49c84-55b30 | Creative AWE32 + Gravis GF1 vendor objects | identity only |
| 55b30-62d8d | Apogee Sound System (22 C modules) | src/audiolib/ |
| 62d8d-6b685 | Watcom 9.5b runtime | library |
| 6b685-6c445 | A.ASM | src/engine/A.ASM |

Every compiled C/asm object of the program now has candidate source; remaining work is NEAR->STRICT, data layout and the natural link.
