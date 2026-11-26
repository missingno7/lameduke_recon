# gamefix3 — large NEAR functions in PLAYER / ACTORS / SECTOR / GAMEDEF / MENUES

Toolchain: wc95b `-4r -s -otexan` (profile `game`), stock `tools/match.py`. Exploration on LAMEDUKE_HOST=nt; final
`*.results.json` on the canonical DOS host (identical numbers to NT). Changed files only:
`src/player/player.c`, `src/actors/actors.c`, `src/sector/sector.c`, `src/sector/operate_helpers.h`,
`src/gamedef/gamedef.c` (headers copied unchanged; line endings preserved). MENUES: no change kept
(`menues.results.json` = integrated baseline; the tested candidate is `tmp/menues.uchar.c`).

Verify (DOS host, repo root), same for actors/sector/gamedef:
```
python tools/match.py --profile game "-i=build/workers/gamefix3/src/player" build/workers/gamefix3/src/player/player.c --json build/workers/gamefix3/player.results.json
```

## Metrics
`byte_diffs` (match.py) is positional and saturates in 10 KB functions (any drift randomises the rest), so it barely
moves. Progress is also measured with `aligned.py UNIT F1,F2 [SRC]`: oracle bytes not covered by
SequenceMatcher-aligned identical instructions (fixup fields masked) = "unmatched". Sizes from `sizes.py`
(oracle extent = bound address to next bound address, includes switch tables).

| Unit | STRICT before -> after | Function | size cand/oracle before -> after | unmatched before -> after | byte_diffs before -> after |
|---|---|---|---|---|---|
| player | 1/304 -> 1/304 | processinput | 15694 -> 15762 / 15692 | 7520 -> 5813 | 11793 -> 11762 |
| | | shoot | 1712 -> **1728** / 1728 | 900 -> 444 | 1134 -> 1136 |
| | | spawn_pistol_tracer, displayraise, displayweapon, getinput, update_air | unchanged | | |
| actors | 8/1936 -> 8/1936 | moveactors | 12320 = 12320 (kept) | 9491 -> 9370 | 8700 -> 8698 |
| gamedef | 9/1712 -> 9/1712 | parse | 6112 -> **6144** (= verifier oracle 6144) | 3722 -> 3115 | 4310 -> 4285 |
| | | execute | 6796 -> 6951 (verifier oracle 6824) | 3974 -> 3737 | 4817 -> 4922 |
| | | move_actor | 1024 -> 992 / 1040 | 360 -> 561 | 525 -> 495 |
| sector | 9/2462 -> 9/2462 | operatesectors | 10000 -> **10288** / 10288 | 6686 -> 7393 | 7242 -> 7453 |
| | | operateactivators | 4624 -> 4800 / 4912 | 2780 -> 2898 | 3329 -> 3374 |
| menues | 7/1546 -> 7/1546 | (no change kept) | | | |

No STRICT function lost (getinput/update_air stay at 4 differing bytes, transnum at 0).

## New toolchain facts (STRONG unless noted)
1. **enum constants vs #define (PROVEN by A/B):** Watcom 9.5b compares a `short` against an *enum* constant in
   32 bits (`movsx eax,..; cmp eax,imm32`) but against a `#define` int literal in 16 bits (`cmp ax,imm16`) and
   tests flag bits as bytes (`test ch,2`). The oracle uses the 16-bit/byte forms. PLAYER's tile enum -> #defines
   fixed shoot's size (1712 -> 1728) and its picnum/tile compares; GAMEDEF `enum move_flag` -> #defines gives the
   oracle's byte `test`s in both movement copies and brings parse to its exact size. The con_opcode enum (used only
   as case labels / 32-bit compares) is neutral and was kept. Check any other enum used in short compares.
2. **Inline copies keep parameter temporaries, and their constants are not folded.** The oracle's in-function
   copies behave like compiler inlining: arguments are copied into fresh temps (processinput: shoot copy does
   `mov eax,0xbf; mov [esp+0x100],eax` = tile, `xor dl,dl; je` = move_to_hit test kept; execute: `[esp+0x14]=i,
   [esp+0x1c]=p` copies before the dispatch loop; operatesectors: `mov [esp+0x1bc],esi` = per-instance copy of sn
   in every setanimation copy). Watcom 9.5b only folds a local whose single definition is a constant, so the
   natural reproduction is: assign the macro inputs to **function-level** variables (processinput: `i = ps[snum].i;
   tile = SHOTSPARK1; move_to_hit = 0;` before each SHOOT_BODY) or add explicit copies (execute: `short i2 = i,
   p2 = p; long x2 = x;` around the dispatch, `PARSE_ARGS` macro; SET_ANIMATION_INLINE: `short sector_ = (sn_)`).
   The shoot copies then contain the dead move_to_hit hitscan path exactly like the oracle (checkhitceiling call).
3. **Sprite-list loops in the big SECTOR functions are top-tested** in the oracle (`test bx,bx; jl exit ... jmp
   back-to-test`), the `while (i >= 0) { ...; i = next[i]; }` form; Watcom compiles `for` to `jmp test` (bottom
   test) and small functions to a guarded inverted loop. Counted `for (w = first; w <= last; w++)` loops stay
   rotated in the oracle. Applied to operateactivators only (`for2while.py`); blanket conversion of operatesectors/
   playerkeys/checksectors moved sizes away from the oracle (-96/-272/-512), see negatives.
4. CSE'd constant compare: oracle `cmp ax,ax` (processinput fire switch) / `cmp ax,bx` (moveactors master switch)
   = a compare against a constant the register is known to hold on that switch edge (`if (curr_weapon == CLUB)`
   inside a CLUB/HEAVYHBOMB/HANDREMOTE case group; `switch (lotag)` inside `if (statnum == 3)`). Not reproduced yet.

## PLAYER
processinput (all NATURAL unless noted; each verified against the oracle block order):
- newowner: `i = ps[snum].newowner; if (i >= 0) { ps.posx = sprite[i].x ... }` (oracle keeps it in DI, no pointer).
- `xvect = yvect = 0;` moved to just before `if (player_vel | player_svel)` (oracle zeroes there; the state82 goto
  skips it).
- if/else polarity to the oracle block order: poszv clamps (`> -2048` / `< 2048` first), `fz - (eye<<8) > posz`
  (airborne branch first), jumping_counter `< 1024` first, footstep `== 3` first with `switch(lotag){case 1; case 2;
  default}`, damage `disable_armor == 0 && armor > 0` first, weapon_pos `(kick&0x7f) && != 6` then `kick & 0x80`
  inc first; `posz += 3072/2048` as if/else; `tics2 = tics + tics` (`lea ecx,[ebp+ebp]`).
- `long kick` (oracle `xor eax; mov al,[weapon_pos]` dword at [esp+0x8c], 32-bit `cmp eax,2`): 7145 -> 6402 alone.
- Function-level `short i, tile; unsigned char move_to_hit;` for the 5 SHOOT_BODY copies (fact 2).
- SHOOT_BODY locals one per line in the order zvel, y, sect, z, x, distance, p, angle, hitsprite, j, hitwall
  (COMPILER_STEERED: hill climb `climb.py` over declaration order, objective shoot+processinput unmatched).
- shoot main switch `switch ((long)(unsigned short)tile)` (COMPILER_STEERED; oracle `movsx; and eax,0xffff;
  cmp eax,imm32`); `tile == RPG` and the inner switch are plain (16-bit after fact 1).
Remaining: processinput frame 0x100 vs 0x110 and +70 bytes; shoot frame 0x28 vs 0x2c (oracle has an
unreferenced dword at [esp+0x18]; unused locals are eliminated by 9.5b, so not reproducible that way);
spawn_pistol_tracer: oracle computes the hitscan z (`posz - (wp<<10) + 5376`, wp zero-extended unsigned char)
before ang, keeps ang as a dword sharing its slot with `size`. `unsigned char weapon_pos` gives 404 -> 332
unmatched but 576 bytes (oracle 592) -> not kept; `long ang` 560 bytes; z-variable/shared ang-size variants
560-592 bytes, >= 338 unmatched (v/spt1.py, v/spt2.py).
Negatives: tile/switch compare variants (v/sh1, sh3-sh5), `long p` / `long j` / p-free APLAYER branch
(v/sh6, v/sh7: worse), the CLUB case-group with `if (curr_weapon == CLUB)` (v/pi4, v/pi5: 32-bit compare, not `cmp
ax,ax`), unused extra locals (v/sh8: no effect), 5 kick types (only long/int help).

## GAMEDEF
- `enum move_flag` -> #defines (fact 1): parse exact size, move_actor gets the oracle byte tests but loses 32 bytes
  (oracle spills flags to [esp+4]; candidate keeps it in a register).
- execute: dispatch copy wrapped in `{ short i2 = i, p2 = p; long x2 = x; ... }` with `PARSE_ARGS` (fact 2):
  3974 -> 3737, the oracle's pre-loop copies now appear. Copying i/p for the movement block too (oracle:
  `mov esi,[esp+0x24]; mov [esp+0x20],p`) matched that block locally but 3737 -> 3981 overall: not kept
  (tmp/gamedef.v2.c is the kept version).
Remaining structural item: the oracle has 40 separate `call parse` sites in both parse and execute; the candidate
cross-jumps them to 21 / 3 (identical arg setup -> tail merged). In the oracle the two PARSE_IF arms use
different registers (i spilled to [esp+0xc], x in EDI) so they are not merged; this is the main size/shape gap of
execute. Oracle execute calls getincangle_shift twice (not inlined), candidate inlines the IFCANSEE one.
Negatives: PARSE_IF without do/while or as plain block (v/gd3: worse), parse local declaration order (720 perms via
climb, size-preserving: no gain), con_opcode enum -> defines (neutral), killit if/else form (no change).

## SECTOR
- operate_helpers.h `SET_ANIMATION_INLINE`: per-copy `short sector_ = (sn_)` (fact 2): operatesectors 10000 ->
  10288 = oracle extent; frame 0x17c -> 0x1d0 (oracle 0x1c4). Aligned unmatched 6686 -> 7393 (allocation/slot
  order shuffles), kept for size.
- operateactivators: outer loop `if (sprite[i].lotag == lotag) { if (picnum == 23) sector[sprite[i].sectnum]...
  else { sn = ...; source = i; ... } }` (oracle reads sectnum separately in each branch) and sprite-list loops in
  the function as while (fact 3): 4624 -> 4800 (oracle 4912), top-tested loops as in the oracle.
Negatives: while-conversion of all list loops in operatesectors/playerkeys/checksectors (tmp/sec_all.c: sizes
-96/-272/-512, unmatched +1000 total); for-loop of operateactivators without the if-restructure (worse).

## ACTORS
- moveactors: `t = hittype[i];` after `sect = sprite[i].sectnum` (oracle order nexti, findplayer, sect, t):
  9491 -> 9370, size kept 12320.
- Found but not kept: MASTERSWITCH loop is `if (statnum == 3) switch (lotag) { case 2: case 21: t[0]=1; case 3:
  t[4]=1; case 23: shade=1 } else if (statnum == 6 && picnum == SEENINE) ...` in the oracle (constants 1 hoisted
  into EDI/ESI/CL, `cmp ax,bx` with bx==3). Reproduces that block but moveactors grows to 12336 (+16):
  tmp/actors.switch.c. SHARK block: oracle uses `ps[p]` (no `pl` pointer) and `if (turn < 0) t[0]=4; else
  t[0]=2;` statements instead of the ternary (not attempted further).

## MENUES (nothing kept)
- menus MENU_ACTION copies: oracle stores the color argument as a byte (`mov al,4; mov [esp+0xf4],al`) and pushes
  `movsx eax,[menu_y]` directly. `unsigned char color = 4;` + no `cursor` temp: unmatched 2137 -> 1626, but size
  4912 -> 4864 (oracle 4976) and byte_diffs 2989 -> 3409, so not kept (tmp/menues.uchar.c). The oracle passes the
  color zero-extended (`xor ecx,ecx; mov cl,..`) to drawmenu's `short` parameter; making drawmenu's parameter
  unsigned char/long loses drawmenu STRICT (26 B: two local slots swap; all 24 local declaration orders tested), a
  K&R definition changes nothing.

## Tools here
`dd.py UNIT FUNC [SRC] [--all]` aligned listing; `odis.py START END [BASE]` oracle listing; `orng.py`; `hunks.py
DD [min]` hunks with instruction-count mismatch; `drift.py`; `sizes.py UNIT [SRC]`; `aligned.py UNIT F1,F2 [SRC]`;
`alib.py`; `tune2.py UNIT FUNCS VARIANTS.py` (SAVE=name writes the variant into src/); `climb.py UNIT FUNCS SPEC.py
[iters] [seed]` permutation hill climb (optional TARGET size penalty); `pos.py UNIT SRC...` positional totals;
`for2while.py FILE START END` (list loops only); `m.sh UNIT`. Variant files in `v/`.
