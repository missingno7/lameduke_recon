# gamefix2 — NEAR -> STRICT in SECTOR / PLAYER / ACTORS / GAMEDEF / MENUES

Toolchain: wc95b `-4r -s -otexan` (profile `game`), stock `tools/match.py`. Exploration on LAMEDUKE_HOST=nt,
final numbers on the canonical DOS host (`*.results.json` in this directory). Oracle D3D.EXE sha256 698979d9...

## Results (canonical DOS host)

| Unit | Before STRICT | After STRICT | Changed functions |
|---|---:|---:|---|
| game_actors | 7 / 1,552 B | **8 / 1,936 B** | ifsquished STRICT (384 B); moveplayers 36 -> 0 differing bytes |
| game_menues | 6 / 1,146 B | **7 / 1,546 B** | probe STRICT (400 B) |
| game_player | 1 / 304 B | 1 / 304 B | getinput 566 -> 4 differing bytes |
| game_sector | 9 / 2,462 B | 9 / 2,462 B | playerkeys 5,234 -> 5,215 differing bytes |
| game_gamedef | 9 / 1,712 B | 9 / 1,712 B | unchanged (source not copied; results.json is the integrated baseline) |

No STRICT function lost in any unit. Changed files (only these differ from src/game):
`src/actors/actors.c`, `src/menues/menues.h`, `src/player/player.c`, `src/sector/sector.h`.
menues.h and sector.h are included only by their own units. Original line endings preserved
(player.c LF, menues.h CRLF, actors.c CRLF, sector.h LF).

Verify (DOS host, repo root):
```
python tools/match.py --profile game "-i=build/workers/gamefix2/src/actors" build/workers/gamefix2/src/actors/actors.c --json build/workers/gamefix2/actors.results.json
```
(same for player, menues, sector; gamedef from src/game).

## Per function

### ACTORS ifsquished — NEAR (20 B) -> STRICT. Class SOURCE.
Oracle keeps a copy of `i` in EDI (`mov edi, ecx` before the switch), indexes the first case's sprite through it
and returns it from that case and the default. Fix (NATURAL/COMPILER_STEERED): assign `result = i;` *before*
`sprite[i].z = sector[...].floorz;` and write the first case's stores as `sprite[result].xvel = 0;
sprite[result].cstat &= ~1;` after `ps[p].kills++`. Statement order kills/xvel/cstat matters (6 permutations x
index variable tested, only k-x-c with `sprite[result]` is STRICT). Negatives: no result variable (break/return
forms, 9 variants, >=156 B); result assigned at declaration (262 B); int result (179 B).

### ACTORS moveplayers — 36 B -> 0 code bytes, still NEAR (2 problems).
Oracle reloads `sprite[i].yvel` for `ps[sprite[i].yvel].ang` in the walking branch: an intervening store to
`sprite[i].xvel` kills the CSE. Fix: order `sprite[i].xvel = 128; sprite[i].ang = ps[sprite[i].yvel].ang;
sprite[i].yvel++;` and pass `script_tics` directly to movesprite (the `tics` local removed; gives the oracle's
`xor ebx,ebx; mov bl,[script_tics]`). All other 5 orders worse (>=183 B). Remaining problems are two switch-table
targets of the *next* function (movemusic, `+3e4/+3ec` -> 1:2ab41/2ab4e vs 2ab42/2ab4f): moveplayers becomes STRICT
as soon as movemusic's case bodies reach the oracle layout (movemusic currently 1 byte short before those cases).

### MENUES probe — 54 B -> STRICT. Class ALLOCATION-ONLY (actually scheduling).
`keystatus` declared `extern volatile unsigned char keystatus[256];` in menues.h (as player.h already does and as
Build declares it). The oracle never reorders keystatus stores across other code; the non-volatile declaration let
the scheduler move them. NATURAL. Other MENUES functions unchanged by this (draw_and_probe 636 -> 635).

### PLAYER getinput — 566 B -> 4 B. Class SOURCE.
Zeroing path order: `svel = 0; player_bits[myconnectindex] = 0; player_avel[p] = player_vel[p] = player_svel[p]
= 0; avel = vel = 0;` (the chained byte zeroing before the long pair). Remaining 4 bytes at +1ac..+1ba: in
`(keystatus[0x2a] | keystatus[0x36]) << 5` the oracle loads the first operand into BL and ORs into EDX; candidate
the reverse. This is an allocation tie: with separate statements `player_svel[p]=0; player_vel[p]=0;
player_avel[p]=0;` the +1ac line matches but the identical `(keystatus[0x4a] | keystatus[0x0c]) << 20` line at
+305 flips instead (always exactly one of the two). Negatives (no change or worse): 72 zero-block orders/chains,
casts removed/long on all 30 bit lines, `*2^n` instead of `<<n`, `x = x + ...` (much worse), operand swap
(changes fixup order), 11 spellings of the <<5 line, 6 spellings of the script_tics clamp.

### SECTOR keystatus volatile (sector.h)
`extern volatile char keystatus[];` (split from `option[]`): playerkeys 5,234 -> 5,215, extent 7,344 -> 7,328 vs
oracle 7,568 (no STRICT change). Kept because it is the Build declaration and strictly improves.

### Not fixed (with what was tried)
- ACTORS hitradius (2 B, +ff/+103, latch `movsx edi,di` vs `movsx eax,di`). Negatives: all 54 declaration
  orders; damage long/int; direct `sprite[EGS(..)].extra`; while / do-while / goto-continue / nextj loops;
  7 distance spellings (inline `(a)*(a)+(b)*(b)` gives oracle instruction order but swaps the i/damage spill
  slots, 6 B); rr precompute; 17 extra flags (-or -oi -ol -om -oc -ou -op -oe=0/100 -zp1/4 -zc -ei -j -oa...).
  cg95 trace: the latch conversion temp (t66) has EDI excluded by live-register interference with `j` (t8, DI);
  explain95 finds no unique anchor.
- PLAYER update_air (4 B at +d4): health path `movsx ebx,cx` vs eax. Negatives: 10 health/armor spellings,
  parameter renames (8), int p, copy `short p = snum` (3 forms), FX_SetVolume/krand/spritesound prototypes
  (36 combos). explain95: no unique anchor.
- ACTORS wakeactors (17 B): oracle computes the cansee z argument in EAX and moves it to EBX. explain95 (both the
  hack and the natural `sprite[i].z - (sprite[i].yrepeat << 7)` spelling): first divergence is t91 at 1:2593b,
  EAX blocked by t35. 22 z spellings x 3 declaration orders, z statement before/after findplayer: no gain.
- ACTORS movemusic (158 B): first divergence `add ebp,eax` (sum kept in the first klabs register); explain95:
  t25 wanted EAX, blocked by t175, 45 equal-savings records. 25 dist/var/type/order spellings, song selection
  ternary/inverted/++ forms: no gain. Its case-0/2 song pointer is in EBX in the oracle, which lets the two
  branches cross-jump their common `add ebx,eax` tail (candidate 1 byte longer -> moveplayers table targets).
- ACTORS guts (42 B): also blocked by moveactors' trailing switch table in its extent.
- SECTOR checkhitwall (528 B): oracle shade is `short` (16-bit compare, CX) and uses startwall/endwall-style
  copies (`mov ebx,edx`); `startwall/endwall` + long shade gives 488 B, but the w/x/y parameter registers stay
  rotated (oracle w=ESI, x=EDI, y=EBP; candidate EBP/ESI/EDI). Not kept.
- SECTOR activatewarpelevators / operateforcefields, ACTORS guts, PLAYER displayraise, GAMEDEF transnum are
  blocked by the following function's dispatch table inside their extent (operatesectors, playerkeys, moveactors,
  displayweapon, parsecommand).
- MENUES initmenus (253 B): pure store scheduling/constant-register CSE. 300-step hill climb over field order
  within each menu block (search_init.py) moved the first difference 0x6d -> 0xb4 but shrank the body to 592 B; not
  kept.
- GAMEDEF move_actor (525 B): oracle spills `flags` to [esp+4] and keeps i in ECX; not attempted further.

## cg95 tracer notes (for the integrator)
`tools/cg95/cgtrace95.py --unit game_actors|game_player ...` aborts with "traced and ordinary NT
code/publics/fixups differ" even on the integrated sources (game_sounds control works). Cause (PROVEN with
`trdiff.py`): one uninitialised padding byte differs between the observed and the ordinary NT build — ACTORS
_TEXT+7 (0x0d vs 0x00, inside the leading data/padding at 1:25160, not in any function), PLAYER _TEXT+0x1f. The
function code is identical. `trdiff.py` here re-uses the tool's own functions, records the differing offsets in
the JSON (`nt_code_diff_offsets`, `dos_vs_traced_diff_offsets`), uses the DOS object, and the unchanged
`explain95.py` then runs. Traces/explanations: build/cg95/gamefix2/{hitradius,update_air,wakeactors,
wakeactors_shl,movemusic,activatewarpelevators}.json and *-explain.json.

## Tools in this directory
- `dd.py UNIT FUNC [SRC]` — side-by-side aligned disassembly with fixup targets.
- `tune.py UNIT FUNC[,FUNC|ALL] VARIANTS.py` — apply text replacements (to .c, falling back to .h), compile
  (NT host for speed), report verdicts; `SAVE=name` keeps a variant. Variant files are in `v/`.
- `colours.py TRACE.json` — compact listing of the 9.5b colour decisions in a cg95 trace.
- `search_init.py` — hill climb for initmenus field order (negative).
