# plactfix — NEAR -> STRICT in PLAYER and ACTORS (profile game_oe)

Toolchain: Watcom 9.5 GA (`wc95b`), profile `game_oe` (`-4r -s -otexan -oe=3000`), stock `tools/match.py`.
Exploration on LAMEDUKE_HOST=nt (one unit compile + verify = ~0.15 s), final numbers on the DOS host.
Sources: `src/player/` and `src/actors/` (copies of `src/game/*.h` plus the unit .c; only the files listed under
"Changes" differ from `src/game`).

## Result (canonical DOS host, `*.results.json` / `*.results.txt` here; baseline `*.base.json`, NT)

| unit | STRICT before -> after | NEAR byte_diffs total | data |
|---|---|---|---|
| ACTORS | 8/18, 1,936 B -> **10/18, 3,760 B** (movemusic 816 + moveplayers 1,008) | 15,602 -> 15,393 | none (as before) |
| PLAYER | 2/8, 704 B -> 2/8, 704 B | 16,622 -> 15,947 | _DATA 120 B VERIFIED |

No STRICT function lost. Per function (byte_diffs before -> after, u = aligned-unmatched oracle bytes):
displayraise 108 -> **0 code bytes** (still NEAR only because its extent holds displayweapon's knee jump table,
whose targets need displayweapon's first 0x256 bytes exact); shoot 1136 -> 797 (u 444 -> 391);
displayweapon 3368 -> 3171 (frame now 0x60 = oracle); processinput 11633 -> 11602; moveeffects 2846 -> 2758
(u 2847 -> 1418); movestandables u 1583 -> 1436; moveweapons u 379 -> 130 (frame/slots only left).
Unchanged: getinput 12, hitradius 2, wakeactors 17, movesector 106, spawn_pistol_tracer 365, guts 0 (moveactors'
table), moveactors.

Verify (repo root, DOS host):
```
python tools/match.py --profile game_oe "-i=build/workers/plactfix/src/actors" build/workers/plactfix/src/actors/actors.c --json build/workers/plactfix/actors.results.json
python tools/match.py --profile game_oe "-i=build/workers/plactfix/src/player" build/workers/plactfix/src/player/player.c --json build/workers/plactfix/player.results.json
```

## Changes (only these files differ from src/game; line endings preserved: .c LF, player.h CRLF)
- `src/actors/actors.c`
  - movemusic: `playmusic(ps[screenpeek].music);` (was `playmusic(song)`) -> movemusic + moveplayers STRICT. NATURAL.
  - moveweapons: no `distance` copy; `short p, i, dist, nexti;` and `findplayer(i, (long *)&dist)` (uncast `&dist`
    gives identical code with W113). STRONG from the oracle's word compares.
  - moveeffects: `ssp(i, 0)` / `ssp(i, 1)` instead of the MOVE_ALONG macro at the SHELL site, the `ssp(i, 0);`
    statement before the klabs floor test, and the `ssp(i, 1) == 0 && ...` site; movestandables: one `ssp(i, 0);`
    statement. ssp is the TU's own helper, inlined at -oe=3000 (NATURAL). Other MOVE_ALONG sites tested one by one:
    identical code or worse (see v/ssp_slots.py); the three gib-piece sites are a non-monotone u lead (2 of 3 as ssp
    -> moveeffects u 1360), not adopted.
- `src/player/player.h`: klabs declared by `#pragma aux` only (no prototype), as in Ken's PRAGMAS.H.
- `src/player/player.c`
  - displayraise: no `tile` local, `frames[ps[p].raise_frame - 1] + 681` passed directly (code bytes exact). NATURAL.
  - displayweapon: `xvel = 0; if (player_bits[p] & 32) xvel = player_avel[p] >> 3;` and separate `case 5:` /
    `case 6:` each `x -= 40`. NATURAL.
  - shoot: `long p` + declaration order (COMPILER_STEERED, marked in source).

## Leads not adopted
- spawn_pistol_tracer: one `long ang` reused for the laser size (the oracle shares ang/size in one dword slot), a
  named `z` computed first, `(ps[p].weapon_pos & 127)` inline: u 404 -> 306 but its inlined copy pushes
  processinput u 5785 -> 6005 (`alt/spt_lead.c.txt`).
- displayweapon prefix (for displayraise): remaining are stack-home order (bob/p, shade, x, y/weapon_pos slots) and
  the knee frame byte in DH; declaration-order climbs move only a few bytes.

## Learned semantics / toolchain facts

- **Savings lever (PROVEN, movemusic):** reducing the number of uses of a register variable lowers its conflict
  "savings" and moves it later in the 9.5 colouring order, which flips equal-score register ties elsewhere.
  `playmusic(ps[screenpeek].music)` instead of `playmusic(song)` (one use fewer of `song`) gives the oracle's
  EBX song pointer, the cross-jumped `add ebx,eax` tail, the EBP distance sum, and makes movemusic STRICT; its
  jump table sits in moveplayers' extent, so moveplayers becomes STRICT too.
- **klabs has no prototype (PROVEN, displayraise/displayweapon):** Ken's June 1994 PRAGMAS.H declares `klabs` only
  by `#pragma aux` (no `long klabs(long)` prototype). Without a prototype a `short` argument is promoted by the
  old-style default promotion, which Watcom 9.5 emits as `mov ax,mem ... cwde` (the oracle's form) instead of
  `movsx eax,mem`. ACTORS (all klabs arguments long) is unaffected.
- **Watcom hoists an expression common to two switch successors into the dispatch block (PROVEN):** writing
  `case 5: x -= 40; break; case 6: x -= 40; break;` (instead of `case 5: case 6:`) reproduces the oracle's
  `mov eax,x; sub eax,40` before the knee-frame jump table (displayweapon).
- **moveweapons' distance is a short (STRONG):** the oracle compares `word ptr [dist]` (`cmp word [esp+0xc],0x200`)
  and passes `movsx` of that word to execute; there is no separate `distance` copy. `short dist` with
  `findplayer(i, (long *)&dist)` reproduces it (the uncast `&dist` compiles with W113 and the same code).
- **Commutation, if/else polarity, compound-assignment spelling and loop form are canonicalised (NEGATIVE):** a
  400-step random walk over these rewrites (perm.py) never changed hitradius' 2-byte tie (87% of mutants compile
  to identical code), and 600 steps on moveweapons only shuffled stack slots; for/while/direct-index
  variants of hitradius compile to identical code.
- **hitradius tie explained (STRONG, trace):** the latch conversion t69 (`movsx ?,di`) and the index shift t70
  have equal savings 13044; t69 is coloured first, all scores 0, keeps the incumbent EAX. If t70 were coloured
  first it would score EDI 2 (the load's result j is DI) and pull t69 to EDI (the oracle's `movsx edi,di;
  mov di,[edi*2+...]`). Only the gap-sort order of equal-savings records decides it.
- **getinput tie (STRONG, trace + sort replay):** the three `(a|b)<<n` lines that differ are equal-savings (8)
  conversion pairs whose gap-sort order decides which operand gets EDX. `sortsim.py` shows no single record
  deletion or savings change fixes all three while keeping the matching pairs; one extra savings>=10 record at
  list position 70 would. No source spelling found (30-slot random search of cast/shift spellings: no change).
- wakeactors: the natural `sprite[i].z - (sprite[i].yrepeat << 7)` always coalesces the sub result into EBX
  (call-arg score 4); 18 spellings tested, none reproduces the oracle's `sub eax,ebx; mov ebx,eax`.
- movesector: loop forms (j++ placement, for/while) give u 131 -> 115 but more positional bytes; not adopted.

## Tools (this directory)
`tune.py UNIT FUNCS|ALL VARIANTS.py` (fast NT compile+verify, d/u/size per function; SAVE=name), `combo.py` (random
slot search; metrics d, u, pN prefix), `climb.py` (declaration-order climb; METRIC=u|pN), `perm.py` (pycparser random
walk: commute/polarity/compound/decl swap), `sortsim.py TRACE 'a<b,..' [keep]` (replays the 9.5 conflict gap sort and
searches insert/delete/savings perturbations), `cgt.py`/`cgx.py` (cg95 tracer/explainer wrappers writing to `cg/`),
`dd.py`, `hunks.py`, `tunelib.py`. Variant files in `v/`, traces in `cg/`.
