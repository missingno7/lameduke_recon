# Apogee Sound System worker handoff

The worker copies pass at **330/375 STRICT**, up from 327, for **41,759 bytes**, up from 41,093. All 327 prior STRICT extents and all 28 prior verified data segments were preserved. The verifier confirms 36 of the 52 nonempty data contributions, including all 18 BSS layouts. The per-module table lists the first remaining difference for each code and data extent. [Results and data audit](RESULTS.md)

## Proven changes

- `AL_NoteOn` requires Watcom's framed entry. An ordinary `#pragma aux AL_NoteOn frame` produces the prologue at `1:617ec` and verifies 208 bytes under the registered AL_MIDI profile.
- In `MV_PlayVOC`, local copies of the volume and right channel and an explicit right-channel clamp recover the 523-byte extent. This is compiler steering. The recovered placement restores the branch displacements in `MV_CalcVolume` and `MV_CalcPanTable`. They verify 269 and 189 bytes. Playback itself still differs at `+0x7`.
- In `TS_SetTimerToMaxTaskRate`, an equivalent null-ended `while` loop recovers the target's saved registers and loop instructions. It leaves seven differing bytes and a displaced data reference around the initial rate and task-list loads.
- The 10.0 GA BSS calibration matched the order of 163/163 emitted symbols with 25-slot pages, later pages first, the folded identifier hash modulo 241, and the established size ranks. The 9.5 rule placed 38/163 in the same ordinal positions. The declaration moves are reproducing choices based on the measured rule and preexisting independent bindings. The original order of declarations remains open. PAS16.C now allocates the first symbol slot for `PAS_DMAChannel` at its definition. The worker header continues to declare it for callers.
- The independently bound BSS layouts newly verified in AL_MIDI, BLASTOLD, IRQ, MIDI, MULTIVOC, MUSIC, PAS16 and PCFX total 12,316 bytes.

## Negative and open results

The GPL controls were `origin/al_104` (`5f2d287`), `origin/1_12`, the reconstruction git history, and `external/rott/audiolib/SOURCE` including `MV1.C`. The later circular task list and WAV-era parser did not supply the remaining early code shapes. Under each unit's registered `wc100` flags, the tested PAS card-info argument spellings did not reproduce the extra EAX-to-EDX transfer in FX_SetupCard. GUSWAVE parser variants moved the stack allocation and switch destinations without recovering the table. Playback variants left the EDI and EBP allocation difference. AL_MIDI port argument, timbre lookup, and BLASTER-type selector variants did not produce further matches. The TASK_MAN startup and insertion trials left the allocator differences in place.

All 32 CODE_BYTES_MATCH rows have their named targets bound. The final `bindprop.py` pass produced 148 function and 229 data proposals meeting its vote threshold, all already covered by independently established registry entries, with zero conflicts. Four terminal extents also agree with whole historical 1.04 OMF member matches. These are DMA, DPMI, LL_MAN and MPU401. All 18 terminal candidates are followed by 1–15 zero bytes and no fixups before the next mapped member. The native verifier requires a function-start boundary, so the terminal extent evidence is recorded at its current proof level. [Vote receipts](out/final-votes.json) · [Terminal extent evidence](out/terminal-extents.json)

The other 14 CODE_BYTES_MATCH rows refer to anonymous CONST segments. All ten mismatched CONST segments differ in inter-literal alignment bytes, and all six mismatched initialized-data segments differ in alignment bytes between fields. Eight short worker staging paths varied some emitted padding and left all 16 segment comparisons below VERIFIED. Original host and source context are open. [Staging control](out/padding-probe.json)

## Reproduction

Run from the repository root in PowerShell. The source CLI path uses `match.py --install wc100` with each unit's exact flags and the worker include directory. It sends the matcher's temporary objects to the worker and uses a copy of the existing runtime binding registry. The compile timeout is 300 seconds, and the source-CLI child has a 315-second timeout.

```powershell
$w = 'build/workers/audfix'
python -B "$w/run.py" after
python -B "$w/run.py" direct
python -B "$w/terminal_audit.py"
python -B "$w/summarize.py"
python -B "$w/calibrate_bss.py"
```

The fresh per-unit logs are in `out/direct-ass_*.log`. Source candidates and headers are in `src/`. All work stayed under the worker directory.
