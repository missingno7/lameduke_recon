# gamefix1: GAME unit (game_game) NEAR -> STRICT

Toolchain: wc95b `-4r -s -otexan` (profile `game`), stock `tools/match.py`. Exploration on LAMEDUKE_HOST=nt.
Final numbers come from the canonical DOS host (`results.json`, `results.txt`). The baseline is the same
command on the unmodified integrated source (`base_dos.json`).

```
python tools/match.py --profile game "-i=build/workers/gamefix1/src" build/workers/gamefix1/src/game.c --json build/workers/gamefix1/results.json
```

## Result (DOS host)

| | STRICT | bytes | total differing bytes (all 61) |
|---|---:|---:|---:|
| baseline (src/game) | 25/61 | 6,749 | 50,629 |
| gamefix1 | **27/61** | **7,437** | 49,536 |

- New STRICT: `update_timer_input` (448 B), `numtext` (240 B).
- No STRICT function was lost. CONST (2:00648), _DATA and _BSS still verify.
- Only `game.c` changed. `game.h` and `build_v5.h` are byte-identical copies of src/game, so no other unit is
  affected.
- No overlay is needed. All targets bind through config/symbols.json.

## Per function (DOS differing bytes, before -> after)

### update_timer_input: 284 -> STRICT (SOURCE)
- The oracle body is the timerhandler block, word for word. It works on the three velocity globals, which the
  compiler caches in EBX/ECX/EDX and stores at the end.
- `held()` uses `> 0` (jle), not `!= 0`.
- The decay code is two *sequential* ifs: `if (v < 0) {v += n; if (v > 0) v = 0;} if (v > 0) {v -= n; if (v < 0) v = 0;}`.
  The `else if` form makes the compiler emit `jle END; jmp ZERO`. The oracle has `jg ZERO` falling into
  `jle END`, which saves 2 bytes per block.
- The integrated file had a separate local-variable version of the function with its own `!= 0` held macro.
  One `held` macro now serves both routines.

### timerhandler: 1788 -> 1713, size 2544 -> 2512 (oracle 2496). Still NEAR.
- The same sequential-if decay fix applies to all 18 decay blocks, for -32 bytes.
- The remaining +16 bytes come from `++totalclock` / `totalclock += 5`. The oracle emits `inc dword [mem]` and
  `add dword [mem],5`. The non-volatile candidate emits load/inc/store, interleaved.
- **NEGATIVE.** `extern volatile long totalclock` gives the RMW forms and 699 differing bytes, but size 2480
  (-16). One mov/and versus xor/mov byte-load form differs, and the cached-register colouring changes. Volatile
  also costs elsewhere:
  - draw_monitor_overlay goes from 5 to 501 bytes (`mov edi,[tc]; test di,16` against the oracle's
    `test byte [tc],16`).
  - enterlevel grows by 16 bytes (re-read after `lockclock = totalclock = 0`).
  - moveloop and dobonus get worse.
- Redeclaring totalclock non-volatile after the ISR is rejected (E1129).
- Taking `&totalclock` anywhere has no effect.
- **OPEN.** Volatility is still inconsistent within GAME. write_demo shows the same phenomenon on the
  non-volatile `reccnt`: the oracle has `sub [reccnt],edx` where the candidate splits it into load/sub/store.
  So the cause may be a scheduling or source-shape factor, not volatile.
- **NEGATIVE.** Flag trials (NT host): `-5r`, `-3r` and `-4s` break the STRICT controls. Dropping `-oa`
  (`-otexn`, `-otex`, `-oxt`) loses the global caching that update_timer_input needs. `-otexanr`, `-fp3`,
  `-oi` and dropping `-s` change nothing.

### numtext: 24 -> STRICT (SOURCE)
- The body is the SMALL_TEXT macro shape, with local copies of x, y, text and shade. Those copies come after
  `ltoa`, so `shade` is kept in CL across the call and homed afterwards, as in the oracle.
- The written-out text is equal to `ltoa(value, tempbuf, 10); SMALL_TEXT(sx, sy, tempbuf, shade);`. The macro
  itself is defined later in the file. I did not move it, because moving source text can change the
  DOS-host CONST padding residue.

### numtext_relative: 261 -> 24, size 304 -> 272 (correct). Still NEAR.
- The body is BIG_TEXT-shaped with copies x = sx, y = sy - 1, text and shade_. `left = x` comes *before* the
  display-mode test, there is no `tile = -1` initialisation and no `tile >= 0` test, and `continue` follows
  `text++`.
- With the correct frame, Watcom 9.5b cross-jumps this function's early exit into gametext's identical epilogue
  (`je 1:02844`), as the oracle does. This is **PROVEN** from the oracle's jump target. The 32-byte saving comes
  from this.
- The remaining difference is allocation-only. The screenpeek index temp takes ECX (score 4 in cgtrace95) where
  the oracle uses EAX, and the shade home store is scheduled one instruction earlier.
- **NEGATIVE.** A sweep of 144 variants found nothing better. It crossed:
  - x copy;
  - y as copy, `y--` after `ltoa`, or `y--` before;
  - shade copy;
  - left as long, long with `(short)` cast, or short;
  - left before or after the test;
  - text initialised before or after the test.
  All variants scored at or below the kept form (insn ratio 0.909).

### gametext: 243 -> 13, size 272 -> 256 (correct). Still NEAR.
- The loop is now the minitext shape (`continue` after `text++`, no `tile = -1` or `tile >= 0`).
- `left` is a short, assigned before the display-mode test.
- Only `x` is a local copy of the parameter (`long gametext(long sx, ...) { long x = sx; ...`). Without the copy,
  the return temp lands in EDX and the screenpeek temp in ECX (cgtrace95: ECX score 4).
- The remaining 13 bytes are one scheduling difference: the copy `mov edi,eax` is emitted after the parameter
  homes, where the oracle emits it first.
- A 96-variant sweep covered copies of x/y/text/shade, left type and left position. Those copies are COMPILER_STEERED.

### displayquotes: 277 -> 88 (ALLOCATION-ONLY at baseline)
- Both quote blocks are written as SMALL_TEXT bodies with local copies. The first block reads the message
  pointer through a separate `char *m = msgtext`, which reproduces the oracle's `mov eax,[msgtext]; mov esi,eax`.
- What remains: the `p` parameter home against the `left` home (+0x10/+0xc swapped), and the second block's
  `quote_time` index temps (EDX/ESI roles swapped).
- **NEGATIVE.** These made it worse: a second `m` copy for `quote_text`, `ofs = -23`, and `--x` forms.

### enterlevel: 68 -> 14 (DOS)
- The player/pending zeroing chains run in the oracle's order: `player_svel[i] = pending_svel[i] = 0`, and so on.
  The earlier order targeted the wrong arrays (8 relocation problems).
- The music pointer is `ps[myconnectindex].music = music_files[ps[myconnectindex].volume][0] + 13 * ps[myconnectindex].level; playmusic(ps[myconnectindex].music);`,
  which gives the oracle's expression order.
- The remaining 14 bytes are an ESI/EDI/ECX colouring swap (map_name EDI, loop sign-extension temps).
- Forms with a `music` local get the colouring right but the expression order wrong (base folded into EAX).
- **NEGATIVE.** About 30 variants:
  - declaration order;
  - `long`/`short` local copies of myconnectindex;
  - `p` reuse;
  - `music_files` declared `[4][130]` or `[][10][13]`;
  - `(char *)` arithmetic;
  - chained assignments.
- enterlevel stays layout-blocked in any case, because it sits after several mis-sized functions (cumulative
  offset is not zero).

## Leads tested without a gain (source unchanged)

- **draw_monitor_overlay** (5 B). The inlined `next_text_shade` stores hudpulse before `mov eax,edx` in the
  oracle. I tried 6 written-out forms and 6 `next_text_shade` spellings; next_text_shade itself must stay STRICT.
  None reproduced the order.
- **newgame** (49 B). The oracle loads `disable_armor` (= userdefs.debug, 2:a57ad) late, while the candidate
  hoists it above the byte stores. I tried 12 store permutations, a `ps[0]` pointer, a constant index,
  `userdefs.debug`, and volatile.
- **write_demo** (125 B). The oracle has `if (level == 0 && volume == 0) demo_filename[7] = '0'; else filename[7] = '1';`
  (absolute store in one arm, `[ebx+7]` in the other). That if/else form fixes the arm but grows the function to
  288 bytes, because `reccnt--` splits into load/sub/store (see the RMW note under timerhandler). Kept the original.
- **domovethings_multiplayer** (52 B).
  - The oracle's 4th bit test computes the shift before the byte load.
  - Swapping test 3/4 forms and 5 spellings of the 32-bit `player_bits` assembly did not help (best is the current
    source).
- **strget** (762 B). The oracle text drawing is two expanded bodies, gametext-like and minitext-like:
  - each returns its x (`xor eax,eax` / `mov eax,edi`);
  - the result is cast to short for the cursor tile (1023 small, 1022 large);
  - the two cursor calls are cross-jumped;
  - the parameters look like short x,y, unsigned char shade, unsigned char limit.

  The first loop uses BL=0 as a loop-invariant zero (`cmp bl,[ks]`). The two `keystatus[scan] = 0` stores are
  merged, and backspace is `if (len > 0) len--; else len = 0;`. A first rewrite along these lines
  (`v_sg2.txt`) is 1088 bytes against 1024 and was not adopted. It is a starting point.
- **cgtrace95/explain95** (`build/cg95/gamefix1_*.json`). Run on numtext_relative and gametext: the first
  attributable decision is a colour score (ECX=4 against the oracle's register 0) for the screenpeek index temp,
  and a stack-home order for y/left. The home sort input order of equal-priority temps decides the y/left slot
  swap. No source lever was found beyond the parameter-copy shapes above.

## Layout notes

- Cumulative size offset is now 0 again from displayquotes through animatesprites:
  - timerhandler +16;
  - moveloop +16;
  - network_move_loop -32;
  - gametext and numtext_relative fixed.
- So gametext, numtext_relative and displayquotes are pure code problems now. The functions after displayrooms
  (+48) are layout-blocked, even when their code is close (enterlevel, newgame, write_demo).
- Fixing timerhandler's size alone would move the zero-offset window. timerhandler, moveloop and
  network_move_loop have to be fixed together.

## Files

- `src/game.c`: candidate. `src/game.h` and `src/build_v5.h` are unchanged copies.
- `results.json` / `results.txt`: canonical DOS run of the candidate. `base_dos.json` is the baseline.
- Helpers (worker-local):
  - `cmp.py`: compile on NT and diff instruction text against the oracle;
  - `variant.py` / `mkvar.py` / `filevar.py` / `setfn.py`: variant testing;
  - `chk.py`: match.py plus a baseline delta;
  - `shape.py`, `dasm.py`, `seqif.py`;
  - `gen_gt.py` / `gen_ntr.py`: the variant sweeps.
