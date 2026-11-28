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
