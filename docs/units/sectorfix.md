# sectorfix — SECTOR NEAR functions toward STRICT

Toolchain: wc95b `-4r -s -otexan -oe=3000` (profile `game_oe`), stock `tools/match.py`, no overlay. Exploration on
LAMEDUKE_HOST=nt; `results.json` / `results.txt` are from the canonical DOS host. Candidate: `src/sector.c` (LF line
endings like `src/game/sector.c`); the headers in `src/` are unchanged copies of `src/game/*.h`.

Verify (repo root, DOS host):
```
python tools/match.py --profile game_oe "-i=build/workers/sectorfix/src" build/workers/sectorfix/src/sector.c --json build/workers/sectorfix/results.json
```

## Result (DOS host)

STRICT 9/18, 2,462 B before and after (same set; no STRICT lost; SECTOR has no CONST/_DATA, nothing else checked).
NEAR byte diffs 25,532 -> 25,052.

| function | before (byte diffs / size) | after | note |
|---|---|---|---|
| activatewarpelevators | 31 / 464 | **0** / 464 | all code bytes and code fixups equal; only the operatesectors jump table in its extent (1:1598c-159e0) differs |
| operateforcefields | 59 / 400 | **0** / 400 | code exact; only playerkeys' table (1:19690-19720) differs |
| checkhitceiling | 288 / 1136 | 16 / 1120 (size exact) | one zero-constant register (ah vs oracle bl); plus checksectors' table in its extent |
| checkhitwall | 528 / 1648 | 87 / 1648 | 2 residual allocation ties (below) |
| checkhitswitch | 1026 / 1648 | 1059 / 1616 | instruction-aligned diff 83 -> 10 instrs, but size 1616 vs 1632 (positional count worse) |
| operatesectors | 7449 / 10352 | 7514 / 10400 | inlined warp-elevator copies changed; instruction-aligned ibad 2014 -> 1834 |
| playerkeys | 5231 / 7296 | 5252 / 7360 | size moves toward oracle 7568 |
| checksectors | 7526 / 10528 | 7730 / 10752 | follows its inlined checkhitswitch copy |
| operateactivators | 3394 | 3394 | |

activatewarpelevators, operateforcefields and checkhitceiling cannot become STRICT until the following function's
case labels (operatesectors, playerkeys, checksectors) land at the oracle offsets.

## Source changes (all in src/sector.c)

- **checkhitwall** (NATURAL unless noted): Ken-style `startwall`/`endwall` copies and `short shade` (oracle
  compares 16-bit and keeps shade in CX); `sprite[i].hitag == wall[w].lotag` and `sector[i].hitag == wall[w].lotag`
  (oracle load order); wall loop written `wall[k].shade = shade; if (wall[k].nextwall >= 0)
  wall[wall[k].nextwall].shade = shade;` (gives CX to shade, see fact 1). COMPILER_STEERED: declaration order
  `short sn, i, j, k, startwall, endwall, nextj, shade; signed char ceiling_shade, floor_shade;` and `k` as the
  wall-loop index (both pick equal-savings sort ties: params w/x/y = ESI/EDI/EBP, stack slots of the two shades).
- **checkhitceiling**: the work moved inside the `switch` with `return 1` / final `return 0` (the oracle's two
  epilogues); same wall-loop spelling as checkhitwall; the four palette/shade stores adjacent
  (floorshade, floorpal, ceilingshade, ceilingpal) -> one CSE'd zero and exact size. COMPILER_STEERED: declaration
  order `short best, ceiling, shade, i, j, startwall, endwall;` (sprite-loop EBX/ECX tie).
- **activatewarpelevators**: `hittype[j][0] = direction; hittype[j][1] = direction;` instead of the chained form.
  COMPILER_STEERED (changes savings order; the oracle's inlined copies in operatesectors use one register for both
  stores, which the chained form also gives, so the original spelling stays OPEN).
- **operateforcefields**: per-branch `sector[wall[w].nextsector].floorpal = X; ...ceilingpal = X;` (no `sn`/`pal`
  locals). NATURAL; standalone code now exact (oracle merges the two branch tails after `sn*9`, stores through
  `[edx+eax*4+disp]`).
- **playerkeys**: the hand-written forcefield loop is `operateforcefields(-1);` (NATURAL, PROVEN by the oracle: the
  inlined copy at 1:1b34b tests `lotag == wall.lotag || lotag == -1` with `mov esi,-1` set before the loop).
- **operateactivators**: `operatesectors(sprite[i].sectnum, i);` (NATURAL, evidence: every inlined copy in
  checkhitswitch passes `movsx eax, word [sprite.sectnum]` directly; the sn/source copies gave `mov ax; cwde`).
- **checkhitswitch**: `walltype *wal = &wall[animwall[i].wallnum];` for the switch-toggle loop. HYPOTHESIS /
  approximation only: it reproduces the oracle's shape (address kept across the jump table, switch value in DX)
  but emits `add eax, offset wall` + short displacements where the oracle folds `wall` into each displacement.

## Residuals

- checkhitwall +0x197: case 790 conversion of the EGS result. Trace: t83 = (long)i and its copy t249 (for the
  `*5` hittype index) both have savings 1776; t249 is coloured first and gives t83 an EDX bonus (score 4), oracle
  needs t83 first (EAX, `cwde`). `sortsim3.py` shows that deleting any single conflict in blocks 7-34 (790 case ..
  179 case) would order t83 first and keep the other fixed ties; no natural source change found (~400 variants:
  declaration orders, loop forms, variable choice per loop/case, break/return, nested default switch).
- checkhitwall picnum stores in cases 819/823/860/864: oracle uses `mov reg,imm; mov [m],reg16` in cases
  1,3,4 (ECX, EDX, EDI) and an immediate in case 2; candidate: immediate in case 1, EDX/EDI/ESI in 2-4. These
  constant registers never appear as allocator colour events (decided after register allocation).
- checkhitceiling +0x2b3: the hoisted byte zero (t2, savings 967, range blocks 6-45) takes AH (score 1); oracle BL.
- checkhitswitch: see HYPOTHESIS above. With the `short j` spelling the address is recomputed in every case and
  the function is 1680 bytes.

## New toolchain facts (Watcom 9.5b, NT compiler traced; same code as DOS)

1. **Colour eligibility code 1** (leaf colouring, compiler RVA 0x53049 result from 0x53630): the allocator walks
   every instruction of the conflict range in block-list order; for a single-operand instruction whose operand
   or result is the temp (e.g. CONVERT short->char for `wall[].shade = shade`) it calls 0x53838 with the mask of
   registers already given at that instruction. If all byte registers are taken there, the register is refused.
   Consequence: `next = wall[k].nextwall; wall[k].shade = shade;` (nextwall live across the convert) costs shade
   its CX; the store-first spelling does not. PROVEN by trace (d12/d14/d16 variants).
2. Block-list order is not source order: in checkhitwall the 179/331/573 case body is block 34, between the
   light-case head (33) and its loop exit (36), independent of case order in the source. Linear [first,last]
   ranges therefore make z (ECX) overlap the second switch.
3. Watcom 9.5b never keeps `&wall[j]` (j a register variable) across `if`/`switch` branches: each branch
   recomputes `movsx; lea; add; lea; sub` from j, with or without a jump table, `continue`, `register`, `static j`,
   long/unsigned j, expression index, inlined helper with index param. A pointer variable (or inlined pointer
   param) is kept but with the base added (`add eax, offset wall`). Micro tests m/m1-m6.c. NEGATIVE for the
   oracle's checkhitswitch shape; open.
4. Equal-savings ties decide most residuals here; `sortsim*.py` replays the gap sort (cgtrace95.replay_sort) to
   test single insertions/deletions of conflicts.
5. A chained `a = b = 0` / two `= 0` statements: adjacent stores share one zero temp; a store separated by other
   statements gets its own zero (two registers, e.g. BL/BH in the oracle's inlined forcefield copies).

## Semantics recorded

- checkhitwall(w, x, y, z, weapon): overpicnum 179/331/573 -> set overpicnum 573, spawn SE (lotag 128,
  hittype[1]=5, [2]=w) + sound 5; overpicnum 790 -> if weapon 563 set both sides' overpicnum 573, else spawn SE
  lotag 129 with hittype[2]=w. picnum 819-822/823-826/860-863/864-867 (animated screens, cf. animatewalls) ->
  broken tiles 1183/1182/1180/1181 + sound 23 at the next sector's floor. picnum 35/36/26/212 (wall lights) with
  lotag -> 664/665/670/671, darkest wall shade of the next sector copied to every sector whose hitag equals the
  wall lotag (SE lotag 8/9 deleted there, pals cleared), SE lotag 3 sprites get hittype[3]=shade, [4]=1.
- checkhitceiling(sn): same light logic for ceilings (35/36/26/212 -> 664/665/670/671), returns 1 on a hit.
- checkhitswitch(w): wall switches 395/396 (master switch on/off) and 405/406 (forcefield switch); toggles every
  animated wall with the same lotag, FTA(4), sound 25, operateactivators(lotag), then operatemasterswitches or
  operateforcefields (405/406 also set hittype[0]=2/1 of SE lotag 12 sprites with that hitag).
- operateforcefields(lotag): lotag -1 means all forcefields (playerkeys' debug key uses it).

## Files

Tools: `fd.py`/`mc.py`/`g4.py` (from oeunits; instruction diff), `t.sh`, `vary.py` (substitution trials),
`combo.py` (exhaustive slot search), `cgt.py`/`cgx.py` (cgtrace95/explain95 with output redirected to this
directory, tools unchanged), `cgs.py`, `tr.sh` (trace summary), `sortsim*.py` (gap-sort replay), `wdis.py`
(read-only disassembly of the pinned compiler). Variants in `v/`, slot files in `p/`, micro tests in `m/`,
traces in `cg/`. r1-r3 = NT intermediate runs, base.json = NT baseline.
