# LameDuke reconstruction

LameDuke is the 30 December 1994 prototype of Duke Nukem 3D. This project rebuilds its source code so that the
original 1994 compiler, assembler and linker produce the same bytes as the shipped `D3D.EXE`.

The goal is readable C, plus assembly only where the original was written in assembly. The original toolchain
must compile and link it back into the exact executable: the same code, data, relocations and, finally, the
whole file. This is a matching reconstruction, not a remake or port. Nothing is modernised until the original
program has been recovered.

## How

No source code for LameDuke itself survives. We recover it by combining every clue that does exist:

- **The executable.** The original `D3D.EXE` decides every question. A strict verifier
  (`tools/match.py`) accepts a function only when every byte, every relocation site and every referenced
  address equals the original. Positive and negative control tests check the verifier itself.
- **The original toolchain.** The binary's own evidence identifies the tools:
  - Watcom C 9.5 (unpatched), run as the original DOS compiler under DOSBox-X;
  - MASM 5.10 for Ken Silverman's `A.ASM`;
  - Watcom 10.0 for the Apogee Sound System.
- **Ancestry and related projects.** These give names, local variables, comments, data layouts and whole
  functions that can be adapted until they match. Most of the code was reconstructed from them:
  - Ken Silverman's Build engine snapshots and his Build demo game (`GAME.C`), including the December 1994
    build;
  - the released Duke Nukem 3D 1.3D/1.5 source;
  - the Apogee Sound System source;
  - Ken's `MULTI.C`;
  - raynorpat's behavioural LameDuke rebuild;
  - the game's own CON scripts.

  `docs/ANCESTRY.md`, `docs/DONORS.md` and `docs/PRIOR_ART.md` record what came from where.
- **Compiler behaviour.** Measured rules of the Watcom 9.5 code generator turn "almost" into "exact". They cover
  inlining, register allocation ties, `_BSS` ordering and constant-pool padding (`docs/TOOLCHAIN.md`,
  `docs/FINDINGS.md`).

## Progress

Only byte-identical (STRICT) functions count toward these bars. Each bar is measured against the original code
size, so it can only grow by proving more of the original.

<!-- progress:begin -->

| Part | Byte-identical (STRICT) | Functions | Bytes |
|---|---|---:|---:|
| Game code (GAME, PREMAP, SECTOR, PLAYER, ACTORS, SOUNDS, MENUES, GAMEDEF) | `██████░░░░░░░░░░░░░░░░░░░░░░░░`  18.5% | 98 / 143 | 39,408 / 212,556 |
| Build engine (ENGINE.C + A.ASM) | `█████████████████░░░░░░░░░░░░░`  57.6% | 105 / 123 | 48,703 / 84,515 |
| Multiplayer (MULTI.C) | `███████████████░░░░░░░░░░░░░░░`  50.1% | 20 / 25 | 4,328 / 8,645 |
| Apogee Sound System | `██████████████████████████░░░░`  85.0% | 351 / 375 | 45,800 / 53,853 |
| **Whole program (all non-library code)** | `████████████░░░░░░░░░░░░░░░░░░`  38.4% | 574 / 666 | 138,239 / 359,569 |

- Initialised data proven at its original address (CONST/_DATA): `███████░░░░░░░░░░░░░░░░░░░░░░░`  23.6% (10,630 of 45,056 file-backed bytes).
- Uninitialised data (_BSS) whose layout is proven: 714,205 bytes.
- Functions whose code bytes match but a referenced target is not yet independently proven: 15.
- Identified library code, not counted as reconstruction: Watcom 9.5 C runtime (CLIB3R/MATH387R/EMU387) (35,080 bytes), Creative AWE32 + Gravis GF1 vendor objects (48,812 bytes).
- Every compiled object of the program already has candidate source; the code object is 443,461 bytes in total.

Code that does not match yet (92 functions, 220,667 bytes), by how much of each function's basic blocks already have the original's content (`tools/blockdiff.py`; a diagnostic, never counted as matched):

- statements missing or wrong: 133,653 bytes (60.6%)
- nearly written: small block edits: 60,970 bytes (27.6%)
- code written: order/registers left: 21,076 bytes (9.6%)
- code written: register/stack homes differ: 4,968 bytes (2.3%)
- byte-weighted share of blocks already right: 68.5% exact, 91.8% counting near-identical blocks

<!-- progress:end -->

`docs/STATUS.md` has the per-unit numbers and `docs/FINDINGS.md` has the evidence, marked as PROVEN, STRONG,
HYPOTHESIS or NEGATIVE.

## Using it

You need your own copy of LameDuke's `D3D.EXE` in `assets/`. It is not distributed here. You also need the
historical tools listed in `config/toolchain.json` under `C:\tools`. Then run:

```powershell
python tools/gate.py
```

```powershell
python tools/match.py --profile game_oe -i=src/game src/game/sector.c
```

`tools/gate.py` runs the control tests and rebuilds and verifies every unit from scratch. `tools/match.py` checks
one source file against the original. `AGENTS.md` lists the working rules.

## Layout

| Path | Contents |
|---|---|
| `src/game/` | game code (GAME, PREMAP, SECTOR, PLAYER, ACTORS, SOUNDS, MENUES, GAMEDEF) and the GLOBAL data object |
| `src/engine/` | Build engine (`ENGINE.C`, `A.ASM`) |
| `src/multi/` | Ken's multiplayer code |
| `src/audiolib/` | Apogee Sound System |
| `config/` | toolchain pins, symbol bindings, unit definitions and status |
| `tools/` | LE reader, OMF reader, pinned compiler wrapper, strict verifier, link experiment, analysis tools |
| `docs/` | status, findings, toolchain, ancestry and per-unit notes |

The original game files and third-party reference archives are never committed.
