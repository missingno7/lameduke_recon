# Status

Updated 2026-11-26 (generated from build/verify.json by the integrator). Denominator fixed: obj1 = 443,461 bytes.
Run `python tools/gate.py` then `python tools/status.py` for live numbers.

| Unit | Functions | STRICT | STRICT bytes |
|---|---:|---:|---:|
| a_asm | 20 | 20 | 3,520 |
| engine | 103 | 85 | 45,183 |
| ass_adlibfx | 13 | 12 | 1,473 |
| ass_al_midi | 26 | 23 | 3,156 |
| ass_awe32 | 13 | 13 | 2,263 |
| ass_blastold | 51 | 49 | 6,441 |
| ass_dma | 7 | 6 | 859 |
| ass_dpmi | 7 | 7 | 410 |
| ass_fx_man | 15 | 13 | 2,275 |
| ass_gus | 4 | 4 | 509 |
| ass_gusmidi | 14 | 11 | 598 |
| ass_guswave | 18 | 14 | 1,366 |
| ass_irq | 5 | 5 | 956 |
| ass_ll_man | 5 | 5 | 185 |
| ass_midi | 26 | 26 | 4,392 |
| ass_mpu401 | 15 | 15 | 605 |
| ass_multivoc | 36 | 34 | 7,122 |
| ass_music | 22 | 21 | 1,649 |
| ass_pas16 | 42 | 41 | 6,074 |
| ass_pcfx | 14 | 13 | 1,311 |
| ass_pitch | 5 | 5 | 461 |
| ass_sndsrc | 17 | 14 | 1,354 |
| ass_task_man | 18 | 15 | 1,785 |
| ass_user | 2 | 2 | 188 |
| game_sounds | 13 | 12 | 1,467 |
| game_gamedef | 14 | 9 | 1,712 |
| multi | 25 | 18 | 3,164 |
| game_menues | 11 | 9 | 3,418 |
| game_sector | 18 | 9 | 2,462 |
| game_player | 8 | 4 | 2,048 |
| game_game | 49 | 30 | 16,704 |
| game_premap | 12 | 4 | 1,533 |
| game_actors | 18 | 10 | 3,760 |

Totals: 558 STRICT / 666 functions, 130,403 bytes; LameDuke 210 STRICT (84,971 bytes); Apogee Sound System 348 (45,432 bytes); CODE_BYTES_MATCH 15; verified data 10,530 bytes, verified _BSS layout 714,205 bytes.

Identified, not reconstructed: Watcom 9.5 runtime (docs/RUNTIME_CENSUS.md), Creative AWE32 and Gravis GF1 vendor objects (docs/AUDIOLIB.md).

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
| 62d8d-6b685 | Watcom 9.5 runtime | library |
| 6b685-6c445 | A.ASM | src/engine/A.ASM |

Every compiled C/asm object of the program now has candidate source; remaining work is NEAR->STRICT, data layout and the natural link.
