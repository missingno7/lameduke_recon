# gameB2: GAME functions in obj1 04cd0-0ba30

All 15 functions are now in `src/game.c`, in address order between `draw_water_distortion` and `read_demo`. The 6 routines that were already in the window are kept. Results come from the canonical DOS host:

```
python tools/match.py --profile game --symbols build/workers/gameB2/overlay.json "-i=build/workers/gameB2/src" build/workers/gameB2/src/game.c --json build/workers/gameB2/results.json
```

The run reports STRICT 19/40 functions and 2,669 bytes; the baseline was 16 functions and 2,061 bytes. The new STRICT functions are show_help, saveplayer and loadplayer. All 16 previous STRICT functions remain STRICT. The CONST (2:0069c), _DATA and _BSS segments still verify. The build is warning-free.

| Function | Oracle | Cand | Status | Byte diffs | First diff | Class |
|---|---:|---:|---|---:|---|---|
| nonsharedkeys | 2080 | 2176 | NEAR | 1091 | +1e6 | The warp loop's inline view, text and register allocation diverge. Size is +96 |
| show_help | 128 | 128 | **STRICT** | 0 | - | |
| checkcommandline | 592 | 592 | NEAR | 139 | +9 | REGALLOC: argc/argv/mapname go to ebp/edi/ebx in the candidate, edi/esi/ebp in the oracle. The structure and layout of all cases match |
| wait_for_players | 928 | 912 | NEAR | 480 | +39 | Scheduling of the bit-mask tests in the packet-2 loop. Size is -16 |
| main | 7472 | 7728 | NEAR | 4939 | +48 | All sections are present in order: init, inline inittimer, attract demo loop, game-over/score entry, typing, HUD, single/master/slave input and menus. Size is +256 |
| displaystatus | 2352 | 2352 | NEAR | 1072 | +2c | STACKSLOT: the instruction stream is about 0.89 equal and the numtext_fixed temporaries use different stack offsets |
| playback_demo | 3184 | 3280 | NEAR | 2279 | +10 | Byte-order scheduling of `which`. Size is +96 |
| domovethings | 1968 | 1920 | NEAR | 1301 | +3c | record() is inlined by the compiler; the status-9 macro has register and stack differences |
| dobonus | 3536 | 3520 | NEAR | 2702 | +8 | Loop/stack layout. Size is -16 |
| draw_monitor_overlay | 720 | 720 | NEAR | 5 | +13 | SCHED: the hudpulse store is placed one instruction later than in the oracle |
| displayhighscores | 1024 | 1008 | NEAR | 585 | +37 | STACKSLOT in the macro locals. Size is -16 |
| spawn_random_effects | 480 | 480 | NEAR | 231 | +7 | STACKSLOT/order of the written-out EGS body |
| move_status9 | 1680 | 1728 | NEAR | 913 | +8 | Register types of the two partner loops. The second loop is still off. Size is +48 |
| saveplayer | 208 | 208 | **STRICT** | 0 | - | |
| loadplayer | 272 | 272 | **STRICT** | 0 | - | |

The range totals 26,624 oracle bytes against 27,056 candidate bytes. The verifier still reports the _TEXT layout CONFLICT, because gameA's and gameC's functions are absent. Calls into 1:00d10, 1:02400, 1:03a20, 1:04050 and 1:0ba30 have unbound or LAYOUT displacements.

## Findings

- **PROVEN.** saveplayer and loadplayer use local pointers to the two save-name buffers. The slot is a `char` updated with `slot += '1'`. The order is boardname[5], [6], [7], then gamename[7]. Folding the address into the stores breaks loadplayer's ESI-held board name, and every other store order leaves at least two off32 targets swapped. 12 store orders were checked with the verifier.
- **PROVEN.** checkcommandline uses Duke's `i = 1; if (argc > 1) while (i < argc)` shape. The default case precedes `case '?'` and falls into it. The /e case reuses `i` as its inner counter, which leaves i at 10. Both "Unknown command line paramater" literals exist separately (2:294 and 2:304).
- **PROVEN.** In wait_for_players the packet loop is nested: `while (numplayers < wanted && !keystatus[1]) while ((len = getpacket(...)) > 0) switch ...`.
- **STRONG.** Several bodies are written out textually in the original, not inlined by the compiler. These are draw_weapon_tile, minitext, gametext, numtext_fixed, the displayrooms view, displaystatus/displayquotes through helper calls, and the status-9 walk. The same callees are called normally in other functions. move_status9 is defined after domovethings, yet its body appears inside domovethings. playback_demo expands displaystatus with *calls* to numtext_fixed, while displaystatus expands numtext_fixed itself.
- **NEGATIVE** (Watcom 9.5b, game profile plus `-oe=N`, NT host). `-oe=40` inlines show_help into checkcommandline (816 bytes against the oracle's 592 with a call). `-oe>=50` inlines draw_weapon_tile into draw_monitor_overlay (5 differences) and also every minitext call in displayhighscores (1376 bytes; the oracle keeps the two tempbuf calls). EGS is never inlined into spawn_random_effects, even at `-oe=80`. A larger global inline threshold therefore does not explain the expansions; source-level duplication does.
- **NEGATIVE.** Making next_text_shade return `unsigned char` changes its own code (`add al` instead of `add eax`). Callers instead store the shade as a short, using `(unsigned char)next_text_shade()`. The variant `hudpulse += 128; return ...` keeps next_text_shade STRICT and improves displayhighscores (445 differences instead of 585), but worsens draw_monitor_overlay (122 instead of 5).
- **NOTE.** keystatus must be `volatile`: the stores to key bytes happen before the dependent loads. All existing STRICT results are unchanged.
- **NOTE.** wait_for_players, playback_demo and spawn_random_effects have no inbound reference anywhere in obj1, so they are dead code in this build. Their bindings rest on the start-boundary evidence in docs/units/game.md.

## Overlay

`overlay.json` holds 12 function and 59 data bindings. Each one cites call sites or relocation sites. The CONST literal bindings are provisional: the literal text at each address plus the referencing site. The `msgtime`, `msgtext`, `playerreadyflag` and `fta_quotes` bindings carry the same names and addresses as the gameA2 overlay.

## Tools

`dasm.py` produces annotated disassembly. `cmp.py` compiles the source and diffs instructions with relocated fields masked. `variant.py FUNC FILE [--match]` and `permute.py` test source variants, and `setfn.py`, `ins.py` and `mkmacro.py` edit the source. `extents.json` is only the extent table used by cmp.py; it is not a binding file.

## Suggested next steps

1. draw_monitor_overlay: work on the store scheduling of the inline shade.
2. checkcommandline: register-allocation trials.
3. displaystatus: the stack-slot order of the numtext_fixed temporaries. The same fix likely applies to the macro users.
4. The size gaps in main (+256), nonsharedkeys (+96), playback_demo (+96) and move_status9 (+48).
