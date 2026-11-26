# libfix worker results

Fresh verification promoted 15 functions and 5,405 bytes to STRICT. Every one of the 142 baseline STRICT functions still verifies. Counts below include the full native verifier report for each unit. NEAR bytes are candidate extents.

| Unit | STRICT before to after | STRICT bytes before to after | NEAR before to after | NEAR bytes before to after | CODE_BYTES_MATCH |
|---|---:|---:|---:|---:|---:|
| MULTI | 18 to 18 | 3,164 to 3,164 | 7 to 7 | 5,247 to 5,245 | 0 |
| AL_MIDI | 21 to 22 | 2,488 to 2,948 | 5 to 4 | 2,912 to 2,112 | 0 |
| MIDI | 22 to 25 | 2,468 to 4,132 | 3 to 0 | 1,648 to 0 | 1 |
| BLASTOLD | 47 to 48 | 5,620 to 6,200 | 1 to 0 | 632 to 0 | 3 |
| GUSWAVE | 13 to 13 | 1,240 to 1,240 | 3 to 3 | 1,024 to 1,024 | 2 |
| MULTIVOC | 21 to 31 | 3,414 to 6,115 | 14 to 4 | 4,213 to 1,493 | 1 |
| Total | 142 to 157 | 18,394 to 23,799 | 33 to 18 | 15,676 to 9,874 | 7 |

## PROVEN

- **MIDI.** The LE fixup at `1:5bfdc` gives the callback that `MIDI_PlaySong` passes in EAX to `TS_ScheduleTask` at `1:5bfe5`. Its value is `1:5b6a0`, where `push ebp; mov ebp,esp` follows the seven jump-table entries at `1:5b684–5b69f`. The registered `_MIDI_ServiceRoutine_` binding at `1:5b6a3` begins after that prologue. The GPL source restores the original frame when its commented `#pragma aux ... frame` is enabled. The worker spells the function `MIDI_ServiceRoutine` in C so the additive overlay can bind the real entry. Integration can restore the GPL spelling and correct the canonical address to `1:5b6a0`, or register the worker name. The corrected boundary also verifies `_MIDI_ResetTracks_` (108 bytes), including its embedded switch table for the following routine. At `1:5be23–5be36`, advance the file pointer before clearing the two track-list fields in C. This aligns the allocator call and verifies all 712 bytes of `MIDI_PlaySong`.
- **BLASTOLD.** The three mixer reads use direct calls to `1:5faf4` from `1:607d6`, `1:607e7` and `1:607f6`. The explicit function-pointer casts in `BLASTER_Init` suppress expansion in Watcom 10.0 while preserving those direct calls. Clearing the callback by assignment and preserving the saved status on the lock-error path give 580 STRICT bytes. A function pointer stored in a local and the tested inline-depth and optimize pragmas did not reproduce the three calls.
- **AL_MIDI.** The pitch calculation uses `PercussionTable[key].Key` at `1:61126–61140` and the voice key itself for melodic channels. The earlier 11-byte timbre records and two-byte drum-map entries come from `GMTMBOLD.C` and independently mapped data at `2:09e80` and `2:09f80`. The pitch function verifies for 460 bytes. The timbre candidate now uses the map's Timbre byte, compares it in each arm and uses the stereo flag. The volume candidate restores the velocity floor observed at `1:60fae–60fb8` and the carrier-only output path. The latter compiles to the original 280-byte extent with 30 differing instruction bytes starting at `+0x7b`. The fresh semantic diagnosis classifies the remaining difference as ALLOCATION-ONLY.
- **MULTIVOC.** The demand-feed entry reads rate, volume, left and right from the four argument registers and the callback from the stack, then returns with `ret 4` at `1:5a434`. The five arguments are also present in the older GPL `MV1.C` stub. The target allocates with priority `0x07ffffff` and stores `0x7fffffff` in the voice. It uses zero pitch offset and initializes the 16 activity dwords to 1. The loop lets Watcom emit the linked Watcom runtime `__STOSD` call at `1:63ef7`. In the parser's callback arm, placing the Playing assignment after subtraction removes a three-byte reload and restores all switch cases. The table in the demand-feed extent then also verifies, yielding 561 and 516 STRICT bytes respectively.
- **MULTIVOC shared returns and cleanup.** Reordering the demand-feed pointer clear and the error assignment in `MV_Kill` restores its 188-byte size and the jump from `MV_SetPitch` to the earlier mixer epilogue. The parser correction restores the original displacements to the shared epilogues reached by `MV_DeleteDeadVoices`, `MV_ServiceVoc`, `MV_Kill`, `MV_AllocVoice` and `MV_StopPlayback`. The recording routine writes PlayPage before MixPage. In shutdown the typed loop clears 16 mix-buffer pointers through `__STOSD`, which restores the saved ECX and shared local return. The promotions and exact byte totals are in the verifier JSON.

## NEGATIVE and OPEN

- **MULTI.** BUIL0422 `ENGINE.C` establishes the older serial-ring lineage. BUIL0801 `ENGINE.OBJ` has the December `comsend` shape, while the June 1995 `2DRW0628/MULTI.OBJ` uses different escape bytes and a larger ring. Ordinary indexing and local-pointer forms for the second escape store made the match worse. The preserved candidate has five byte differences and a moved relocation at the second escape store, first difference `+0x5d`. At `1:48da1–48daa`, the timeout decrements and tests before repeating the ECB read. The new loop reflects that branch order. The first difference in `sendpacket` remains the displacement at `+0x13`. In `netinitconnection` a common result/exit reduces the differing bytes from its early-return shape, though its result register and the username row indexing still differ (first difference `+0x15`). Flat username indexing and a volatile swap temporary did not solve it. For `getpacket`, splitting or narrowing the masked ring-index expression did not generate the original DX/DH sequence at `1:4913c–4914d`. The first verifier difference remains `+0x16`, a branch displacement.
- **GUSWAVE.** The 372-byte `GUSWAVE_PlayVOC` candidate has matching fixup sites and verified targets. The five instruction bytes starting at `+0x07` swap EDI and EBP for sample and pitch offset. The SOURCE classification starts from that register difference. Register qualifiers, parameter aliases and equivalent header-offset expressions did not recover the allocation.
- **AL_MIDI.** `AL_SetVoiceTimbre` still differs from `+0x0c` and needs the original allocator and register scheduling through its port-write blocks. `AL_SetVoiceVolume` matches its left stereo arm and still differs in mono and right stereo register evaluation. The alternative port locals and operand forms did not close these cases.
- **MULTIVOC.** `MV_PlayVOC` now clears the recycled voice's DemandFeed field, reads the header offset first and uses the evidenced frame. The 526-byte candidate differs at `+0x07` in stack reservation and argument homes. Merging final clamps and qualifying the arguments did not recover the target's 16-byte frame or panning shape. `MV_CalcVolume` and `MV_CalcPanTable` each differ in one byte in the shared-return displacement (`+0x109` and `+0xb9`). `MV_SetMixMode` has two allocation bytes starting at `+0x11`. Resolve playback's frame and panning shape to continue the shared-return work. Data-segment and terminal-extent debt is visible in the per-unit logs.

## Reproduce

Use PowerShell from the repository root. The driver keeps compiler staging and output in this worker, uses each unit's registered flags, and calls the native matcher with `--obj` and the additive `--symbols` overlay. The final run used DOS-hosted Watcom 9.5b (`-4r -s`) for MULTI and Watcom 10.0 GA for audio (the `audiolib` profile, with `-omaxne` in place of `-omaxnet` for MULTIVOC). MULTI iteration used `LAMEDUKE_HOST=nt`.

```powershell
$w = 'build/workers/libfix'
$env:LAMEDUKE_HOST = 'dos'
foreach ($unit in 'multi','ass_al_midi','ass_midi','ass_blastold','ass_guswave','ass_multivoc') {
    python -B "$w/probe.py" m $unit after > "$w/out/after-$unit.log"
}
python -B "$w/scores.py"
python -B "$w/probe.py" s ass_midi MIDI_ServiceRoutine MIDI_PlaySong
python -B "$w/probe.py" s ass_multivoc MV_StartDemandFeedPlayback MV_GetNextVOCBlock MV_PlayVOC
```

Source candidates and copied headers are under `src/`. `overlay.json` contains the evidence and binding for the MIDI callback alias. `out/before-*.json` and `out/after-*.json` are native matcher results. The GPL lineage examined was `external/duke3d-1.3d/reference/audiolib-reconstructions`, including the `5f2d287` ASS 1.04 restoration and the GPL `MV1.C` retained in `7abecdd`. All verification here was static.

