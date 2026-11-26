# gamefix7: GAME unit (game_game): timerhandler, wait_for_players, render_* sizes

Toolchain: profile `game` (wc95b `-4r -s -otexan`), stock `tools/match.py`. Exploration on LAMEDUKE_HOST=nt;
final numbers from the default DOS host.

```
python tools/match.py --profile game "-i=build/workers/gamefix7/src" build/workers/gamefix7/src/game.c --json build/workers/gamefix7/results.json
```

**Base.** The integrator replaced `src/game/game.c` during this run (19:37: data ownership, `avel/vel/svel`
defines, LF line endings). This worker is rebased on that file. `base_dos.json` is the same command on the
integrated `src/game`.

## Result (DOS host)
- STRICT 28 -> **29/61** (8,541 -> 11,037 B). New STRICT: `timerhandler` (2,496 B). No STRICT function was lost.
- Differing bytes 44,160 -> 41,518. CONST, _DATA and _BSS VERIFIED. No overlay is needed.
- `game.h` and `build_v5.h` are identical to `src/game`.
- Size errors: only `displayrooms` +48 is left. Fixed this round:
  - `timerhandler` +16;
  - `wait_for_players` -16;
  - `render_camera_tile` -16;
  - `render_sprite_camera` -16;
  - `render_mirror` -48.
- Everything after displayrooms is still +48 positional, so wait_for_players and the render_* functions keep
  positional byte diffs.

## The timerhandler/totalclock puzzle: solved, and not volatile
- **PROVEN. The oracle timerhandler is the compiler's inline expansion of `update_timer_input()`.**
  - The test source is `totalclock++; update_timer_input(); if (option[2]) { totalclock += 5;` followed by
    `update_timer_input()` five times `} outp(0x20,0x20);`.
  - Compiled with `-oe>=150` (the inline threshold for this callee is between 100 and 150), it gives
    timerhandler with 0 unequal instructions at the oracle size (`tmp/inl.c`).
  - `update_timer_input` (1:000d0) has no call or fixup reference anywhere in obj1. It is emitted only because it
    is extern, which is consistent with every call having been inlined.
- **Mechanism (PROVEN with cgtrace95 + variants).**
  - totalclock is not register-cached in either build: the conflict savings are 0 in both traces.
  - The written-out copy puts `totalclock++` in the same basic block as the following `keystatus[keys[5]]`
    test. The 9.5b scheduler then splits `inc [mem]` / `add [mem],5` into load/op/store and interleaves it.
  - The inline expansion starts a new block, so the RMW stays in memory. An unreferenced label after each
    increment, or `do { } while (0)` around each copy, gives the identical result.
- **Adopted.** Each of the six copies is wrapped in `do { ... } while (0);`. This is COMPILER_STEERED: it models
  the inline boundary, and there is a comment in the source.
  - totalclock stays non-volatile, which is consistent with `test byte [tc],16`, the word load in dobonus and
    the enterlevel store order.
  - Ken's handlers (BUI1101B and others) are plain `totalclock++` with `volatile` in BUILD.H. GAME's ISR is
    different, so volatile is not needed to explain it.
- **NEGATIVE (NT host).**
  - A block-scope `extern volatile long totalclock` gives E1129.
  - `volatile long *clk = &totalclock` keeps the pointer in a register (+90 ibad).
  - `++*(volatile long *)&totalclock` is folded away and has no effect.
- **Inliner facts (STRONG), for the integrator's inlining model.**
  - 9.5b never inlines a function that contains a string literal (mini test `tmp/m/t6.c`). This explains why
    calls to gameexit, show_help, read_userdefs, write_userdefs and loadlookup are kept: their literals live in
    the GAME CONST pool.
  - A function defined after its caller can still be inlined, because codegen is deferred.
  - The inline threshold is per callee node count and is not cumulative.
  - A single global `-oe` still cannot explain why minitext, gametext and draw_weapon_tile calls are kept
    (OPEN).
  - render_mirror and render_sprite_camera look like inlines of `render_camera_tile(...)`, as in raynorpat's
    `rendertile` (view.c:124).

## wait_for_players (-16 -> 0, ibad 22 -> 4)
- **Change.** `case 4:` now starts with `input_text_length = 0;`. This is natural: the message length is reset
  before the new text is copied.
- **Evidence (STRONG).** 9.5b emits only `xor cl,cl` for this store and drops the store itself. The oracle has
  exactly this "dead" `xor cl,cl` at the head of case 4. The code is byte-identical through the rest of the case.
- **Left.** The 4<< bit test, oracle `mov ecx,ebp; and ecx,esi` against our `and esi,ebp`. Operand-order
  spellings change nothing, and cgtrace finds no anchor.

## render_camera_tile / render_sprite_camera / render_mirror (sizes now exact)
- **copybuf pragma.** It is now `#pragma aux copybuf = "rep movsd" parm [esi] [edi] [ecx];` with no modify
  clause.
  - This is Ken's 1994 form (BUIL0422 ENGINE.C/BUILD.C, BUIL1020 PRAGMAS.H). `modify exact [ecx esi edi]` only
    appears in later PRAGMAS.H.
  - Without `exact`, the loops reload `tile` every iteration, as the oracle does. Sizes become exact for
    camera_tile and sprite_camera, and render_mirror goes -48 -> -16.
  - copybuf is used only in these three functions in GAME. menues.h keeps its own pragma.
- **render_mirror.**
  - The wall endpoints are loaded into locals `x1, y1, x2, y2` before the averages. This matches the oracle's
    load order: wall[w].x, wall[w].y, point2, then p2.x/y, with y1 spilled. Size is now 0, ibad 121 -> 90.
  - The first copy loop uses `tile` instead of the literal 570. The oracle has `push 0x23a` for rotatesprite,
    which is constant propagation from `tile = 570`. ibad 90 -> 82.
- **Left (allocation only).**
  - Parameter and loop-variable colours: the oracle has x->EBP, y->[esp], z->EDI, dest->EBP and row->EDX.
  - The oracle spills the `(long)tile` temp to [esp+4]. Frames are 0x18/0x1c/0x24 against ours 0x14/0x20/0x28.
  - **NEGATIVE.** These change nothing or make it worse:
    - declaration order of row/dest and x/y/z;
    - K&R parameter lists;
    - the order of the sprite-field assignments (7 permutations);
    - compiler inlining of render_camera_tile at -oe=400 (render_mirror 960/114, sprite_camera 864/199).

## displayrooms (+48, OPEN): root cause found, no fix
- **Root cause (PROVEN with cgtrace95, `tmp/dr.json`).**
  - The water copy's `reflection` is used before it is set (the `depth == 0` def is conditional). Its conflict
    therefore spans the whole function, from [0,0,0] to [154,2,0], and it is coloured early (savings 11280).
  - It is live across every drawrooms and permanentwritesprite call, where EBX/ECX/EDX are argument
    registers, so it takes ESI. Then `p` gets EDI and `j` gets no register (savings 5322), which costs +48.
- **The oracle is different.** It keeps reflection in EBX inside the water loop, as standalone
  draw_water_distortion does. It also uses EBX as scratch and for `l` throughout the animatesprites copies, and
  loads EBX as the drawrooms argument. So in the original, reflection is not live before the water code.
- **NEGATIVE.** Liveness stays whole-function with all of these:
  - compiler inlining of draw_water_distortion (`-oe=60`), defined before or after the caller;
  - compiler inlining of animatesprites;
  - `do {} while (0)` blocks;
  - `register`, `static` or `long` reflection;
  - declaring it in the column loop or the if block;
  - declaration orders.

  Mini reproduction: `tmp/m/w2.c`. Any explicit initialiser (`= 0`, `= pixel`) does fix the colouring, but it
  adds an instruction the oracle lacks.
- Water declarations at displayrooms function scope ("top") give +32 and ibad 281. Not adopted.
- **Next.** Find a codeless definition of reflection on all paths, or another source shape the 9.5b liveness
  treats differently.

## Files
- `src/game.c`, plus `src/game.h` and `src/build_v5.h` (copies of src/game).
- `results.json` / `results.txt`: the DOS-host run. `base_dos.json`: the integrated baseline.
- Helpers:
  - `g4.py`, `allsz.py`, `trym.py`, `fdd.py`, `cgt.py`, `summ.py`: from gamefix6;
  - `dowhile.py`: applies the timerhandler wrap;
  - `oesweep.py`: inlining per `-oe` threshold;
  - `ocalls.py`: intra-unit call census, oracle against candidate;
  - `tcuse.py`: per-function uses of a data symbol;
  - `cnames.py`: named conflicts and colours from a trace;
  - `wv.py`: trace variants of mini sources;
  - `mini.py`: compile and disassemble a mini file;
  - `strs.py`.
- `v/`: variant files and checkpoints (`ckpt0.c` is the old base; `ckpt_int2.c` is the integrated base used
  here). `tmp/`: scratch, traces and mini tests.
