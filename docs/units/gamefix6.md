# gamefix6: GAME unit (game_game) sizes and block order

Toolchain: profile `game` (wc95b `-4r -s -otexan`), stock `tools/match.py`. Exploration used LAMEDUKE_HOST=nt.
The final numbers come from the default DOS host (`results.json`, `results.txt`).

```
python tools/match.py --profile game "-i=build/workers/gamefix6/src" build/workers/gamefix6/src/game.c --json build/workers/gamefix6/results.json
```

## Result (DOS host)
- STRICT 27/61, 7,437 B. This is unchanged, and no STRICT function was lost.
- CONST, _DATA and _BSS are VERIFIED.
- Total differing bytes went from 48,236 to 44,280.
- No overlay is needed.
- Only `game.c` changed. `game.h` and `build_v5.h` are identical copies of `src/game`.
- Seven size errors are fixed:
  - main +112;
  - playback_demo +32;
  - prelevel -96;
  - dobonus -16;
  - displayhighscores -16;
  - nonsharedkeys, which stays exact while its DRAW_VIEW copies grow;
  - the main/playback_demo STATUS_BAR copies.
- Sizes still wrong (bytes):

| function | size error |
|---|---:|
| timerhandler | +16 |
| displayrooms | +48 |
| wait_for_players | -16 |
| render_camera_tile | -16 |
| render_sprite_camera | -16 |
| render_mirror | -48 |

## Per function (size cand-oracle, ibad = unequal instructions from `allsz.py`, DOS byte diffs)

The ibad metric now masks fixup displacements properly, so its baseline is 11,350 rather than gamefix4's numbers.

| function | size before -> after | ibad before -> after | DOS bytes before -> after |
|---|---|---|---|
| main | +112 -> **0** | 2663 -> 140 | 4651 -> 3361 |
| playback_demo | +32 -> **0** | 36 -> 28 | 2188 -> 569 |
| prelevel | -96 -> **0** | 2334 -> 1162 | 7074 -> 7047 (frame 0x30 vs 0x1c) |
| spawn | 0 | 1643 -> 1002 | 6412 -> 6402 |
| dobonus | -16 -> **0** | 499 -> 270 | 2792 -> 2744 |
| displayhighscores | -16 -> **0** | 60 -> 35 | 585 -> 433 |
| nonsharedkeys | 0 | 56 -> 18 | 1029 -> 303 |
| wait_for_players | -16 | 77 -> 22 | 480 -> 403 |
| displayrooms | +48 | 788 -> 285 | 2216 -> 2209 |
| total ibad | | 11350 -> 6320 | |

## What changed, and why

1. **main: the game-over block follows the frame loop (PROVEN by the oracle).**
   - In the oracle, `play:` tests `gm & 0x80` at +4d6 and jumps forward to +749e. The newscore, highscore and
     write_demo block is placed at the end of main.
   - The source is now `while ((gm & 128) == 0) { frame } <game-over block>` instead of
     `for (;;) { if (gm & 128) {...; return;} ... }`.
   - Effect: ibad 2663 -> 473.
2. **Calls the original kept as calls.**
   - The oracle calls getscorename in main, and calls next_text_shade at main+c01 and playback_demo+36e. Those two
     are the monitor-overlay copies.
   - Our `-oe` inlines all of them. They now go through `#pragma aux` aliases: `score_text_shade` (existing; its
     declaration moved up to sit after next_text_shade) and the new `enter_score_name` -> `getscorename_`. This is
     COMPILER_STEERED, the same technique as newscore's.
   - **Also COMPILER_STEERED:** `file_id_check` -> `check_file_id_` returning `long`. The oracle tests the result
     with `test eax,eax`, but check_file_id returns with `mov al,1`.
     - The original apparently called it with no prototype in scope (implicit int).
     - wc95b rejects that with E1062.
3. **Copies keep their own parameter temporaries (Rule 2).** The oracle pattern is
   `mov ax,[screenpeek]; movsx ebx,ax; mov esi,eax` at every DRAW_VIEW and STATUS_BAR site.
   - DRAW_VIEW is now `short p_ = (P); short p = p_;`.
   - STATUS_BAR has its own `short snum = (P); short j;`. The oracle loop counter is a short (`cmp di,0x19`), and
     there is no strength reduction.
   - SHOW_QUOTES uses `offset -= 23`, as displayquotes does (oracle `sub esi,0x17`).
   - The chat send block in main has its own `short k` (oracle `mov cx,[connecthead]`).
4. **nonsharedkeys**
   - The warp keys are two separate stores, `keystatus[0x48] = 0; keystatus[0xc8] = 0;`, where there was one chained
     store. The oracle stores the zero byte twice without a reload.
   - `ps[0].level &= 15;` now comes before `ps[0].volume &= 3;`. This gives the oracle's RMW on volume.
5. **prelevel and spawn**
   - The spawn copy in prelevel now has per-case `changespritestat(i, N)` tails. The `stat` variable and the final
     `if (stat != -1)` are removed.
   - **SE (lotag) jump-table body order (PROVEN from the oracle table at 1:f0f0):**
     27, 12, 13, 17, 24, 23, 20, 3, 4, 9(->8), 11(->0 group). Both spawn and the prelevel copy are reordered
     (`reorder_se.py`). Watcom emits jump-table bodies in source order.
   - **prelevel wall switch.** The body order is now 819(->570), 761(->764), 406, then 395, as the oracle places
     them. The 406 case is a complete body and no longer falls through: the oracle duplicates the tag/wallnum code
     there with different registers.
   - In the 395 and 761 bodies, the increment is a separate `numanimwalls++;`. The oracle computes the index once.
6. **dobonus**
   - The volume test is now `switch (ps[0].volume) { case 0: ...; case 1: ... }`. The oracle has
     `test al,al; jbe; cmp al,1; je` with a cross-jumped overwritesprite call.
   - `bonus_marks` is declared as `short [][16]` and indexed `[volume][i*2]` / `[i*2+1]`. The oracle addresses it
     as `[edx+eax*4+2304/2306]` with two separate loads. The declaration is extern only; the data lives elsewhere.
7. **displayhighscores**
   - The second ltoa/minitext pair copies `y` and the shade into temporaries before ltoa. The oracle computes
     `imul ecx` and `movzx esi` before the call.
   - SMALL_TEXT takes its text through an extra copy, `t_` -> `text`. This gives the oracle's eax->esi move.
8. **wait_for_players**
   - In case 3 the bit updates are now `player_bits[other] &= mask; player_bits[other] |= byte << n;`. The oracle
     emits byte stores of zero followed by an `or`.
   - Effect: ibad 77 -> 22. The size is still -16: the oracle is 3 bytes longer.
9. **displayrooms.** The animatesprites copies declare `long l;` before `short i, j, k, sect;`. This changes
   allocation only (ibad 788 -> 285); the size is still +48.
10. **playback_demo.** `read_demo(which); which++;` gives the oracle's two separate byte loads.

## Negative results (NT host unless noted)
- **timerhandler.**
  - With `extern volatile long totalclock` timerhandler becomes size-exact. Only 3 instructions are left: a dead
    `mov eax,[totalclock]` read from volatile `totalclock++`, plus padding.
  - But volatile costs elsewhere:
    - moveloop, playback_demo and enterlevel +16;
    - dobonus -16;
    - draw_monitor_overlay `test byte [tc],16` becomes a register load; `(char)` and other casts do not restore it.
  - `++totalclock` / `+= 1` under volatile change the whole cached-global colouring (+707 ibad).
  - Under non-volatile, `++`, `+= 1`, `= +1`, a pointer, block placement and a volatile cast (ignored by wc95b) all
    give the same code. Not kept.
- **next_text_shade.** The oracle's inlined copies (draw_monitor_overlay, main chat) store hudpulse before
  `mov eax,edx`, but the standalone function stores it late. Every rewrite that moves the store breaks the
  standalone STRICT. `hudpulse += 128` keeps the standalone function STRICT but ruins the inlines (dobonus +528).
- **render_camera_tile, render_sprite_camera, render_mirror.**
  - The oracle keeps the stack param `tile` in memory: it does a movsx per loop iteration, spills to `[esp+4]`,
    and reuses that temp for rotatesprite. The candidate caches `tile` in EBP from before animatesprites.
  - None of these change the size:
    - do-while, while, `long`/`short` tile copies, a `waloff` pointer;
    - declaration and initialisation orders.
  - cgtrace: "no unique traced decision anchor".
- **wait_for_players.** Operand swaps, a while loop and a separate increment for case 4 do not help. The oracle
  has an apparently dead `xor cl,cl` before case 4's loop, and a `mov ecx,ebp; and ecx,esi` form in the 4<< test.
- **main.** `short j` (at function level, or j/k together, or all three) is worse. Only the block-local `short k`
  in the chat send was kept.
- **getscorename spelled differently** (if/return, ternary, local): our `-oe` still inlines it. Compiling without
  `e` (`-otxan`) keeps it as a call but loses gameexit and draw_meter STRICT. So the original inlines selectively,
  and the alias stays.
- **displayrooms.** Other declaration orders of the copies give the same size. `long ang` gives +96.

## Remaining
- timerhandler +16 blocks the zero-offset window. draw_water_distortion has 0 byte diffs; its 13 problems are
  displayrooms' table entries, which need timerhandler and displayrooms exact.
- displayrooms: `j` gets no register (savings 5322, every register live), while the oracle keeps j in EDI and p in
  ESI.
- The render_* sizes are explained by `tile` register caching (above).
- prelevel, main and dobonus: stack-slot and frame layout dominate the positional byte diffs.

## Files
- `src/game.c` (candidate), `src/game.h`, `src/build_v5.h` (unchanged copies). `results.json` and `results.txt` are
  the DOS run.
- Helpers:
  - `g4.py` (from gamefix4, with fixup-displacement masking added), `allsz.py`, `fdd.py`;
  - `trym.py VARFILE [FUNCS]`: variant runner that reports every function that changed;
  - `calls.py FUNC [-v]`: call sequence, candidate vs oracle;
  - `summ.py`: JSON delta and lost-STRICT check;
  - `flagcmp.py`: flag experiments;
  - `reorder_se.py`: SE switch body reorder;
  - `cgt.py`: cgtrace95 wrapper with output under this worker directory.
- `v/`: variant files and checkpoints `ckpt1..4.c`. `tmp/`: scratch.
