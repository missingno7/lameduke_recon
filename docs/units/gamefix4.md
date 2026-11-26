# gamefix4: GAME unit (game_game) function sizes

Toolchain: profile `game` (Watcom 9.5 GA `-4r -s -otexan`), stock `tools/match.py`. Exploration used
LAMEDUKE_HOST=nt. The final numbers come from the canonical DOS host (`results.json`, `results.txt`).

```
python tools/match.py --profile game "-i=build/workers/gamefix4/src" build/workers/gamefix4/src/game.c --json build/workers/gamefix4/results.json
```

## Result (DOS host)

- STRICT **27/61, 7,437 B**, unchanged. No STRICT function was lost.
- CONST (2:00648), _DATA and _BSS VERIFIED.
- Total differing bytes: 49,505 -> 48,236.
- No overlay is needed.
- Files that differ from src/game:
  - `game.c`;
  - `game.h`: `spawn` returns void;
  - `build_v5.h`: `sectortype.wallptr/wallnum` are signed `short`.
- All three files keep CRLF.

### Sizes (candidate minus oracle, 16-aligned extents)

"ibad" is the number of unequal instructions after alignment (`allsz.py`). It is a better guide to structure
than positional byte diffs while frames and slots still differ. "DOS bytes" is the match.py byte_diffs
count.

| function | oracle | size before | size after | ibad before -> after | DOS bytes before -> after |
|---|---:|---:|---:|---:|---:|
| moveeffectors | 14128 | -480 | **0** | 3255 -> 1749 | 9955 -> 10109 (frame-slot shifts) |
| spawn | 8736 | +176 | **0** | 3363 -> 1643 | 6666 -> 6412 |
| move_status9 | 1680 | +48 | **0** | 152 -> 42 | 913 -> **24** |
| domovethings | 1968 | -48 | **0** | 218 -> 79 | 1301 -> 1198 |
| nonsharedkeys | 2080 | +96 | **0** | 396 -> 106 | 1117 -> 1029 |
| playback_demo | 3184 | +96 | +32 | 185 -> 79 | 2228 -> 2188 |
| main | 7472 | +240 | +112 | 1279 -> 2721 (block order) | 4978 -> 4651 |
| dobonus | 3536 | -16 | -16 | 830 -> 519 | 2702 -> 2792 |
| prelevel | 9872 | -96 | -96 | 2979 -> 2306 | 6886 -> 7074 |
| displayrooms | 3200 | +48 | +48 | 316 | 2216 |
| timerhandler | 2496 | +16 | +16 | 18 | (volatile/RMW, see gamefix1) |
| wait_for_players, displayhighscores, render_camera_tile, render_sprite_camera | | -16 each | unchanged | | |
| render_mirror | 976 | -48 | -48 | | |

### Cumulative start offset (candidate minus oracle, after `sizes.py`)

The 16-byte cstart prefix is normalised out.

| range | offset |
|---|---:|
| up to timerhandler | 0 |
| displayrooms | +16 |
| nonsharedkeys .. wait_for_players | +64 |
| main | +48 |
| displaystatus, playback_demo | +160 |
| record .. dobonus | +192 |
| draw_monitor_overlay .. displayhighscores | +176 |
| spawn_random_effects .. spawn | +160 |
| render_camera_tile | +160 |
| render_sprite_camera | +144 |
| render_mirror | +128 |
| resetplayer .. prelevel | +80 |
| newgame .. getscorename | -16 |

- moveeffectors and spawn no longer add offset. Before this round they added -480 and +176.
- The +160 in front of them comes from earlier functions: main +112, playback_demo +32, displayrooms +48,
  timerhandler +16, and four -16s.
- Calls are verified by binding. The offset matters only for the jump tables, which are spawn's table in
  moveeffectors' tail and prelevel's table in reset_player_motion's tail. It also matters for intra-unit
  local targets.

## What changed, and why

### moveeffectors: rewritten in Duke 1.3d style (-480 -> 0)
- **Switch tree (PROVEN by the oracle's compare tree).** The oracle splits the tree at 15 and the candidate
  split at 16. The oracle has two more labels, `case 1` and `case 7`, which go straight to the loop
  continuation. With 30 labels the median is 15. Adding `case 1: case 7: break;` gives the oracle tree. The
  body of case 17 is then placed right after the dispatch, as in the oracle.
- **Variables.** The function-level `long q, l, m, x, st, j, d, *t; short i, k, nexti, p, sect;` follow Duke
  1.3d (`long q,l,m,x,st,j,*t`). The oracle slots show the sharing:
  - x (0x0c) is the findplayer distance, the loop counters and the angles;
  - m (0x10) is used in cases 8 and 18;
  - l (0x14) is the minimum distance, the sign, the pointer in case 21, and the stale sector in case 23;
  - q (0x18) is the speed or angle delta.

  `j` is `long` everywhere, because the oracle tests `test reg,reg` on a movsx'd value.
- **ksgn pragma (PROVEN).** The oracle's ksgn is `add ebx,0x80000000 / sbb eax,eax / add ebx,0x7fffffff /
  adc eax,0`. It occurs 10 times in obj1, 7 of them in GAME. The 1995 `add ebx,ebx; sbb; cmp; adc` form
  never occurs in the oracle. Only moveeffectors uses ksgn.
- **Case semantics taken from the oracle.** Raynorpat/candidate forms differed in these cases:
  - **128:** the `wall[t[2]].cstat | 32` test; t[2] is reloaded on every use; 15 EGS shards with arguments
    evaluated inline; `if (t[0] < t[1]) t[0]++; else {...kill}`.
  - **5:** the locator search recomputes the distance into `l` after `q = owner`. `rnd(32)` is
    `(krand()>>8) >= 223`. The FTA(7) expansion follows.
  - **6:** `if (t[4] > 0) {...} else xvel = 256`, and x counts the followers.
  - **14:** the sprite loop also tests `sector[sect].hitag != 65535`.
  - **21:** `l = (long)&ceilingz/&floorz` and `xvel = ksgn(...) * (yvel << 4)`, as in Duke SE21.
  - **13:** `if (t[2] == 0) {search} else {...}`. This places the blocks as the oracle does: search, then the
    t2==4 part, then the else part. The search's second walk is nested inside the hit test.
  - **23:** the first wall walk uses the stale `l`, and `floorpicnum != 49` guards only its own store.
  - **27:** `x < (unsigned)hitag`, an unsigned compare.
- **FTA.** In the oracle FTA was inlined by the compiler (see Toolchain facts). Case 5 keeps
  `FTA_INLINE(7)`, which is the constant-folded expansion. Case 19 uses a `{ short q = 8; ... }` block. The
  oracle's case 19 keeps q in BX, `mov ebx,8; movsx eax,bx`.
- **Wall-loop end variables.** The oracle computes `wallptr + wallnum` with a 16-bit add and keeps it in a short
  (spilled, or in BX). Cases 3, 4, 8, 12 (first loop), 13 and 23 use `k = wallptr + wallnum; for (x = wallptr;
  x < k; x++)`. In case 8 the end variable is `p`.
- **Header.** `sectortype.wallptr/wallnum` are now signed `short`, as in Build. The oracle uses movsx. This
  changes no other GAME function size. reset_player_motion moves by 6 ibad.

### spawn (+176 -> 0)
- The oracle never loads `i` into EAX before `ret`. Every case ends `call changespritestat; ret`, and default
  returns directly. The function has no `return i`. It is now `void spawn(long, short)` (game.h too). The GAME
  callers ignore the result.
- **Case-body order (PROVEN from the oracle body addresses).** The order is:
  1408, 1165, 1406, 5, 1404, 1359, 686/1370, 283, 215, 1280, 1175, 1190, 960, 962, 961, 305, 32, 408, 631,
  34, 23, 30, 1122, 38, 931, 930, 940, 323, 265, 327, 175, 57, 606.
  - Fallthrough pairs stay adjacent.
  - ibad went from 3312 to 1678.
- `case 1360:` has its own head, `if (j >= 0 && sprite[j].statnum == 10) z += 4096;`, and falls into
  686/1370. The repeat is chosen by an if/else, not a ternary.

### prelevel (size still -96)
- The written-out spawn copy got the same order and the same 1360 split (ibad 2979 -> 2306).
- The oracle's copy is a compiler inline of spawn. Only 3 changespritestat calls survive, because the case
  tails are cross-jumped. The candidate's copy still uses `stat`.

### move_status9 and domovethings
- `p` is `long`. The oracle has `movsx edi,[connecthead]; test edi,edi` and index arithmetic on EDI.
- The `hittype[i][0]` decrement is the else branch of `if (hittype[i][0] == 0) {...}`. The oracle places it
  out of line at +647.
- The other two changes are `i != k` and `ps[p].cursectnum == sect`.
- move_status9 is down to 24 DOS bytes, all register-colour ties (EBX/EDX).
- domovethings has the record(0) body written in place, which is the compiler inline in the oracle.

### DRAW_VIEW and BIG_TEXT macros (nonsharedkeys, playback_demo, main, dobonus)
- DRAW_VIEW is the inlined displayrooms. It now copies its argument (`short p = (P);`, the inline parameter)
  and uses `short s = userdefs.camera` for the camera-sprite clamp.
- BIG_TEXT assigns `left = x` before the display-mode test, as gametext does.
- main: inittimer's body is written in place. The oracle inlines it, and inittimer has no callers in obj1.

## Toolchain facts (new)
- **STRONG. The original was compiled with user-function inlining, and inlined bodies do not inline again.**
  The oracle has no calls to inittimer, uninittimer or FTA. Other observations:
  - domovethings contains record(0) and move_status9 inline.
  - main, playback_demo, network_move_loop and domovethings_multiplayer contain domovethings copies whose
    record and move_status9 stay calls.
  - The displayrooms copies call animatesprites, while displayrooms itself contains animatesprites inline.
  - With our compiler, `-oe=40` inlines FTA, and Watcom's inline of FTA(7)/FTA(8) reproduces case 5
    exactly (1119 B) and the FTA part of case 19 byte for byte.
- **NEGATIVE. A global `-oe=N` cannot reproduce the unit.** The thresholds in our source are:
  - inittimer 23;
  - gameexit and write_userdefs 26;
  - FTA and record 32;
  - numtext_fixed 33;
  - show_help, read_userdefs and newgame <= 40.

  The oracle inlines FTA, record and inittimer but not gameexit or write_userdefs. The original callees'
  node counts must differ, or the inliner has a per-call-site rule. Written-out macro copies stay the model.
- **NEGATIVE.** A `long zvel` EGS parameter would explain the unnarrowed zvel arguments, `and eax,0x7ff;
  push ebx` in case 128 and `-(krand()&511)-512` in the case-13 inline. But it breaks EGS (STRICT -> 49
  ibad, frame `sub esp,4`). It stays short. The cost is about 1 byte per call.
- Watcom switch trees split at the median label. Every label counts, including empty ones that only `break`.
- Watcom moves the tree's fall-through case body right after the dispatch. Other bodies keep source order.
- Small edits move the global register allocation of moveeffectors a lot, often 50 to 100 bytes per case.
  Sizes were balanced with the wall-loop end variable choices. The per-case deltas that remain are allocation
  (constant-in-register 0x26, st kept in a register, slot order).

## Negative / not adopted
- `case 13` as `if (t[2]) {A} else {search}` gives the wrong block order (A first).
- The moveeffectors loop-end variable in case 11, the case-12 second loop and case 26: each gives the wrong
  size or a worse ibad (`v/tloops.py`).
- displayrooms with function-level animatesprites variables: `top1`/`top2` are +64 with ibad 788. `top3` is
  ibad 274 at +48 and was not adopted.
- Reordering in the domovethings record body (`++reccnt`, `reccnt++`, `reccnt = reccnt + 1`, `+= 1`, and the
  `short player = 0` temp) is all equal. The oracle's `add eax,ebx` reuse of the demomode==1 register is not
  reproduced.
- `BIG_TEXT` with `x += ...; text++` order: no effect.

## Open / next steps
- **main (+112).** The oracle places the DRAW_VIEW block at +5a6, inside the frame loop; the candidate
  places it elsewhere. Compare the loop/if nesting of main's frame loop with playback_demo, which is now +32.
- **prelevel (-96).** Its spawn copy should follow the inlined spawn exactly: per-case
  `changespritestat(i, N)` tails, not `stat`. It also has an extra sector loop near +191 (`edi = 0x100`).
- **displayrooms (+48).** Allocation only. Its draw_water_distortion tail tables need displayrooms code exact
  up to +77d.
- **moveeffectors** is size-exact. The remaining work is register allocation:
  - `st` should be in EDI and `i*38`/`i` should be kept in ESI/ECX across the dispatch;
  - frame 0x60 against 0x48;
  - case 4 needs the floorpal tail merge.
- **spawn** case 175 (+19..37) and 1175 (-24) are allocation or inner-switch issues.

## Files
- `src/game.c`, `src/game.h`, `src/build_v5.h`: the candidate.
- `results.json` and `results.txt`: canonical DOS run.
- `base_nt.json`: baseline on the NT host.
- Helpers:
  - `g4.py`: compile, diff, switch-tree case table;
  - `cases.py` (`TREE_AT=o,c` hex skip for later trees), `cdd.py` and `fdd.py`: per-case and per-function
    side by side;
  - `try.py` and `apply.py`: variant sweeps;
  - `allsz.py`: size and ibad table;
  - `calls.py`: inline census under XFLAGS;
  - `slots.py`: [esp+N] per case;
  - `sporder.py`;
  - `macroedit.py`;
  - `v/*.py`: the variant files;
  - `v/game_ckpt*.c`: checkpoints.
