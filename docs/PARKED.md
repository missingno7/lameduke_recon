# Parked functions (do not re-attack without new evidence)

A function is parked when several independent methods have failed on the same residual and further attempts repeat
the same experiment. Workers must not spend time on a parked function unless their brief names it or they bring
NEW evidence (a new donor body, a new compiler fact, a source change made for another reason that moves it).
Keep each parked function at its current status or better. The integrator un-parks a function when evidence changes.

| Function | Residual | Class | Methods already tried (see FINDINGS) | Parked |
|---|---|---|---|---|
| scansector | 7 B | allocation-only | declaration order/use counts, permute v3 (32-variant), synth, vibesynd, engine rounds 3-5 | 2026-11-27 |
| checkhitswitch | 2 B at +0x374 | allocation tie (case 405/406 conflict priority) | owcg (equal savings 160+404), synth (second ECX route worsens caller), inverse (case-406 priority 568->488 breaks switch table), vibesynd | 2026-11-27 |
| drawmaskwall | 7 B | allocation (xb1[z] EBP vs ESI) | owcg trace, inverse (stock build reaches ESI but extent 4 B short, costs 18 STRICT), engine rounds | 2026-11-27 |
| neton | 10 B | allocation-only (replay exact) | r13-multi rounds, flip/climb, inverse | 2026-11-27 |
| sendpacket | 10 B | allocation-only (replay exact) | r13-multi-2, flip replay | 2026-11-27 |
| setvesamode | 0 code bytes (CODE_BYTES_MATCH) | evidence gap: eight never-referenced engine BSS publics block the _BSS layout proof | integrator check 2026-11-27 | 2026-11-27 |
| 14 Sound System CODE_BYTES_MATCH functions | CONST padding residue | data, not code (Watcom 10.0 source-buffer residue) | residue solver (70 -> 45 B); constres 2026-11-27: PCFX, SNDSRC, ADLIBFX CONST blocks (incl. all 26 padding bytes) are identical in Wacky Wheels 1.1 (Oct 1994) and SW Jan 1995, so the residue was fixed when the library members were compiled; SW's debug trailer confirms C:\PRG\AUDIO\source; reproducing it needs the lost 1994 DPMI.H text | 2026-11-27 |
| operatefta, displayhighscores, draw_and_probe, nonsharedkeys | homes only (all blocks + fixups match) | inlined font-helper stack-home ties | smallgame (flip95 exact), permute v3, handcopy (helpers are real functions; hand copies fix homes but not the body) | 2026-11-27 |
| sync, main (decoder group) | 607/623 blocks | allocation tie in the inlined packet decoder (AND result vs packet byte, ECX 2-0) | u-game rounds 2-3 (84 trials), staged index fixes sync but costs main | 2026-11-28 |
| displayrooms | 120/154 blocks, 3,300 B (+32) | allocation scores: inlined sprite cursors 5,322 vs address temporaries 5,360; every raising form costs playback or animatesprites STRICT (shared inlined code) | u-game rounds 4-7 | 2026-11-28 |
| rotatesprite | 17 B (5 stack slots), 75/75 blocks | sine home ESP+20h unreachable with the current temporary set (declaration-order replay caps it at +14h); Feb-95 corroborates +20h | r12-engine rounds 8-10, u-engine2 (462 probes) | 2026-11-28 |
| wallfront, grouvline | 67 B (entry block) / 448 B | allocation and statement-form residue; no gain over 3 rounds | r12-engine 9-10, u-engine2 | 2026-11-28 |
| moveactors (frame) | 497/521 blocks; 4 entry homes | no declaration order/scope recovers all entry homes and keeps 497 (best 7/10 slots at 496); slot table build/workers/u-actors2/r4/FRAME.md | u-actors, u-actors2 rounds 1-4 | 2026-11-28 |
| raytrace | 27 B, 23/24 blocks | retry block: Watcom coalesces the leading product with the y delta in EAX; Jul/Dec multiply into EBX; 101 EBX probes never place it at +0x198 | u-engine2 passes 2-4 (860 probes) | 2026-11-28 |
| processinput (frame) | 392/422 blocks | home-sort tie at ESP+0x10c: Watcom gives it to shot 2's hitwall, the oracle to shot 3's sect (address-taken shorts and ordinary short spills tie); every source form that flips it moves the findplayer spills and loses blocks | u-player rounds 4-6, u-player2 rounds 1-4 | 2026-11-28 |
| movestandables | 150/155 blocks at exact extent, first diff +0x6f | 543 trials; prefix controls reach +0x268 but pair fewer blocks; no form keeps both | u-actors rounds 1-8, u-actors2 rounds 1-5 | 2026-11-28 |
| AL_SetVoiceTimbre | 14 B at +0x70 | Watcom 10 load order of the slot lookup vs bank address; 92 authentic forms with the attested slotVoice[][2] array, none better; the only exact form needs an invented volatile struct (rejected) | misc rounds 3-5 | 2026-11-28 |
| strget | 56/65 blocks | 33 caller-local variants, no gain | u-game rounds 4-10 | 2026-11-28 |
| waitforeverybody | 93/94 (94/94 + 2 B as a trade) | shared decoder with wait_for_players: the final slave sum's 2:2 ECX/ESI tie cannot break toward ESI without costing wait_for_players STRICT (302 trials, no shared form) | u-game rounds 9-11 | 2026-11-28 |
| parse, execute | 198/202, 223/236 | pieces solved separately but no combination beats canonical: exact 482 B IFDEAD/FALL/NUKE (build/workers/u-gamedef2/followup/spill/closed), dispatch spill (saved-script), execute exact extent (extent-spill), movement blocks (movement-restored) - each costs blocks elsewhere via the parser inlined into execute | u-gamedef rounds 1-4, u-gamedef2 rounds 1-6 | 2026-11-28 |
| spawn, prelevel | 545/596 combined | pieces solved separately (endwall latches - canonical; all 13 table targets at 532 - r2/table-recovery; spawn.i home + prelevel +0x2b9 at 540 - r2/entry-recovery); shift accumulates in inlined spawn (tile-1404 divisor hoist) | u-premap, u-premap2 rounds 1-2 | 2026-11-28 |
