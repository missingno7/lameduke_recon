# Status

Updated 2026-11-26. Denominator fixed: obj1 = 443,461 bytes, 1,284 candidate function starts (tools/funcmap.py).
Run `python tools/status.py` and `python tools/gate.py` for current numbers.

| Metric | Functions | Bytes |
|---|---|---|
| STRICT reconstructed C (engine) | 39 | 9,184 |
| STRICT original assembly (A.ASM) | 20 | 3,520 |
| NEAR reconstructed C (engine) | 59 | ~70,000 |
| Identified library code: Watcom 9.5b runtime (exact+sites) | 107 members | 32,906 |
| Identified library code: Apogee Sound System (ASS 1.04 functions, exact+sites) | ~345 | 46,324 |
| Game probes matched in worker scratch (not integrated) | 9 | 555 |

## Code map (obj1)
| Range | Contents | State |
|---|---|---|
| 00000-00010 | cstart BEGTEXT | library |
| 00010-33e5c | game: GAME 00010, SECTOR 14fb0, PLAYER 1f280, ACTORS 25160, SOUNDS 2ba30, MENUES 2c1c0, GAMEDEF 2e700 (STRONG) | workers sounds, gamedef |
| 33e5c-47abf | Build ENGINE.OBJ (98 functions) | unit engine |
| 47abf-49c84 | Ken's MULTI.OBJ (networking, moved out of the engine 9/15/94) | not started |
| 49c84-4e168 | Creative AWE32 modules (MetaWare High C, inside AUDIO_WF.LIB) | library |
| 4e168-62d8d | Apogee Sound System | worker audio |
| 62d8d-6b685 | Watcom 9.5b runtime | library (census complete) |
| 6b685-6c445 | Ken's A.OBJ | unit a_asm, STRICT |
