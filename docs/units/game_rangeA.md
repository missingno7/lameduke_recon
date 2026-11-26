# gameA2: GAME obj1 0x002a0-0x04cd0

All 12 functions are written in C and inserted in address order into a rebase of the current
integrated `src/game/game.c`. The deliverable is `src/game.c`, `game.h` and `build_v5.h` in
this directory. The build is warning-free. Canonical DOS-host run (`results.json`): **5/12
STRICT, 3,904 bytes**. Full object (`results_all.json`): 21/46 STRICT, 6,013 bytes. The
integrated baseline under the same host is 2,061 bytes.

Command (DOS host, no LAMEDUKE_HOST):
`python tools/match.py --profile game --symbols build/workers/gameA2/overlay.json "-i=build/workers/gameA2/src" build/workers/gameA2/src/game.c timerhandler gameexit domovethings_multiplayer moveloop network_move_loop displayquotes FTA EGS cacheit animatesprites draw_water_distortion displayrooms --json build/workers/gameA2/results.json`

| Function | Oracle / cand. bytes | Status | First difference / class |
|---|---:|---|---|
| timerhandler | 2496 / 2544 | NEAR (1787) | +21: `totalclock++` is load/inc/store, the oracle uses `inc dword [mem]`. With a volatile `totalclock` it is 2512 bytes and the first difference moves to +5d (cached-global register order strafe/forward/turn). Volatile breaks enterlevel, see DECLS.md |
| gameexit | 176 | **STRICT** | uninittimer is inlined by the compiler; `setvmode` pragma. The banner is a provisional extern (below) |
| domovethings_multiplayer | 1040 / 1040 | NEAR (52) | +ff: instruction schedule of the 4th sync-bit test and the bits assembly. Register/schedule only |
| moveloop | 1952 / 1968 | NEAR (1172) | +11: size +16. The two packet-loop copies' register allocation and the second `other` slot order |
| network_move_loop | 2832 / 2800 | NEAR (1580) | +6: frame 8 vs 0xc. The oracle spills the `(bits>>8)&255` char conversion through `[esp]` in the master branch |
| displayquotes | 560 / 560 | NEAR (277) | +1a: the inlined minitext loads the text pointer to eax and copies it to esi. A two-variable form fixes the copy but reorders the stack slots |
| FTA | 144 | **STRICT** | |
| EGS | 288 | **STRICT** | Needs CSE'd `actorscrptr[picnum]`, `signed char` status and long-int insertsprite |
| cacheit | 1968 | **STRICT** | Needs the long temporary `tile = i + j` in the per-sprite loop and `loadtile(long)` |
| animatesprites | 1328 | **STRICT** | 1280 case is `l--` / `l++`, which Watcom value-numbers to `dec` / `add ebx,ebx` |
| draw_water_distortion | 256 / 256 | NEAR (0 code diffs) | Body is byte-identical through +ba (needs `shade = *reflection & 31; shade = shade + 96 - (depth >> 1);`). The extent's last 56 bytes are displayrooms' two jump tables. Their entries are wrong only because displayrooms is NEAR (LAYOUT-dependent) |
| displayrooms | 3200 / 3248 | NEAR (2216) | +6: frame 0x1c vs 0x14. The loop counter `j` is spilled in both inlined animatesprites copies, while the oracle keeps it in edi |

Net `_TEXT` size over the range: +80 bytes (timerhandler +48, moveloop +16, network -32,
displayrooms +48). Later intra-object targets are LAYOUT.

## Findings

- **PROVEN:** Watcom 9.5b `-oe` does not inline these large callees. timerhandler (six
  update_timer_input bodies), displayquotes (two minitext bodies) and displayrooms (two
  animatesprites bodies plus the water body) contain textual copies. A test calling
  `minitext()` or `update_timer_input()` emitted calls. The integrated file's `FTA_INLINE`
  macro and the old "shared ... rendering" blocks use the same pattern. I wrote the copies
  as commented blocks.
- **STRONG:** timerhandler caches the three velocity globals in registers for the whole ISR
  and stores them once before `outp(0x20,0x20)`. Writing the blocks on the globals
  reproduces this; locals do not. `held()` here tests `> 0` (jle), unlike `!= 0` in
  update_timer_input.
- **STRONG:** totalclock volatility is inconsistent within GAME. The ISR wants `inc [mem]`,
  which only `volatile` + `++totalclock` gives. moveloop and enterlevel want a single
  non-volatile read. **OPEN**
- **PROVEN:** the packet decoder exists in four copies: domovethings_multiplayer, moveloop
  ×2 and network_move_loop ×2. The variable-width decode has the same source in each copy,
  but the schedules differ by context. Forms A/B of `(n<<(k&7)) & tempbuf[k>>3]` were tested
  per test; the 3rd test needs the swapped form in domovethings_multiplayer.
- **NEGATIVE:** the following changed nothing:
  - variable names (stack slots and registers are name-independent);
  - extern declaration order of the velocities;
  - declaration order of `i, j, k`;
  - (long) or (int) casts on `i + j` in cacheit.

  A long temporary fixes cacheit.
- **Verifier note (needs integrator attention):** with `loadtile(long)`, prelevel (other
  worker) improves from 7123 to 6886 differing bytes. One of its NEAR relocations at +234a
  then coincides with an oracle reference to 2:1bde80. In the full-object run, match.py
  places CONST at 2:1bde80 instead of 2:00648, and check_file_id reports NEAR. Verified
  alone (`... game.c check_file_id`), check_file_id is STRICT and CONST VERIFIED at 2:00648.
  This is a CONST-placement inference artifact from a NEAR function's coincidental site.
  `loadtile(short)` restores the old placement but makes cacheit NEAR (38).
- **Provisional extern:** `exit_banner` stands for the first GAME pool literal at 2:00004:
  `\xde`, 78×`\xdf`, `\xdd\xde DUKE DUKEM 3D Beta version 1.3.95 -- DO NOT DISTRIBUTE!!! That would be bad. \xdd\xde`, 78×`\xdc`, `\xdd`
  (240 bytes, verified against the oracle). As a literal, gameexit matches code bytes but
  CONST cannot verify until the literals of main and the other intermediate functions
  exist. This follows the existing lookup_filename convention.

## Overlay (overlay.json)

Functions:
- domovethings_multiplayer 1:00d10 (calls from main)
- network_move_loop 1:018c0 (recovered entry, no inbound reference)
- displayquotes 1:02400
- domovethings 1:08ea0 (moveloop and network call sites)

Data:
- msgtime 2:a538e
- msgtext 2:a5248
- playerreadyflag 2:1d6c8
- exit_banner 2:00004

The integrator had already bound moveloop, cacheit, animatesprites, quote_text and
pending_* under the same addresses. I removed my duplicates and renamed the source to match.

## Files

- `src/`: the deliverable.
- `old/`: the pre-rebase version.
- `*_a.c` … `*_d.c`: variant bodies.
- `try.sh`, `variant.py`, `m.sh`, `md.py`, `odis.py`, `calls.py`, `cand.sh`: helpers.
- `d_*.txt`: annotated oracle disassembly. It is git-ignored build output.
