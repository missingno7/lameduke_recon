# Early Apogee audiolib follow-up

The worker build covers the 22 integrated Apogee code units and the GMTIMBRE data-only unit. The existing module inventory tracks the Creative and Gravis vendor objects separately. All candidate C and headers are under `src2/`. The original GPL notices remain in the copied sources.

## Reproduction

Run from the repository root in PowerShell.

```powershell
$worker = 'build/workers/audio'
python -B "$worker/followup.py" before
python -B "$worker/followup_bind.py"
python -B "$worker/followup.py" after --overlay "$worker/overlay.json"
python -B "$worker/followup_summary.py" --cli
python -B "$worker/followup_data.py"
$objects = (Get-ChildItem "$worker/followup/after/*.obj").FullName
python -B tools/libscan.py --json "$worker/followup/whole.json" $objects
```

`followup.py` invokes the pinned `tools/wcc.py` compiler wrapper sequentially and saves each unit's exact flags and the compiler output. It keeps temporary files within the worker directory. The after build removes `-zu` from the integrated MULTIVOC, AL_MIDI and MIDI profiles. MULTIVOC keeps `-omaxne`. AL_MIDI and MIDI use `-omaxnet`. The other units use the integration flags. The earlier `early_mv.py` script records how the MULTIVOC candidate was seeded from the released MV1 source. The final candidate includes subsequent edits.

[Per-module before and after counts and segment reports](followup/SUMMARY.md). Raw results with data segments are in `followup/before/` and `followup/after/`. The direct `match.py --json` outputs and diagnostic logs are in `followup/cli/`. The user-supplied gate baseline was 189 STRICT publics. The table rebuilds both source sets using the current verifier and excludes the six CODE data publics and zero-byte aliases. The final baseline is 185 STRICT functions.

The final scan found 15 whole code-and-site matches totaling 21,235 bytes. PAS16 and MUSIC are the newly matched whole code segments. The verifier still requires separate data and extent proofs for their remaining functions. The data map records individual public bindings, exact candidate mismatch offsets, and target BSS spans supported by the data references.

[Whole code matches](followup/whole.json) and [data layout evidence](followup/data-map.json).

## PROVEN

- MULTIVOC uses 144-byte linked voice nodes with a 16-entry per-page activity array. The four mode-specific mixers are C functions at `1:593ae`, `1:59479`, `1:59597` and `1:5967e`. All four verify STRICT for 1,032 bytes under Watcom 10.0. The 16-bit mixers load signed words from their volume tables and write unsigned output with saturation. The service routine at `1:5983d` directly calls all four entries. The mixers lie within the MULTIVOC code lock range, and MV_Init references the same state. This establishes the placement and C source of the early mixer.
- MULTIVOC's 242-byte CONST segment at `2:017c0` and 92-byte initialized data at `2:094b4` verify. Its two volume-table pointers relocate to `2:1c2048`. The recorded voice allocation size includes 2,048 bytes for the 8-bit clip table, following the array of voice records. The target's BSS order differs from the emitted candidate layout.
- TASK_MAN has a nullable head and tail, a 1,024-byte private IRQ stack, a separate task dispatch loop and one-shot atexit registration. Shutdown restores the interrupt vector and reads the CMOS clock. The 26-byte BSS verifies at `2:1c1420`. Its candidate initialized data differs in one byte between the stack selector and the clock rate.
- PAS16 has the same 1,024-byte IRQ stack size. Its code checks an unsigned IRQ and writes the earlier sample-size and filter bits. All 6,182 code bytes and relocation sites match as a single segment at `1:57350-1:58b76`. The native verifier leaves the error-string function and the final lock function below STRICT.
- BLASTER's error table has 17 slots for status -1 through 15. The older DSP 4 playback drives unsigned mono and stereo modes and uses a separate PCM halt opcode per sample size. Its card settings setter writes seven dwords. WaveBlaster setup checks mixer presence and programs register 0x83, with no port argument or return value.
- MIDI allocates a single DPMI-locked block and inserts 28-byte track records in a linked list. The older reset and all-notes-off paths have fewer operations, and backend setup returns a status. The 92 initialized bytes verify at `2:09518`.
- AL_MIDI calculates its fine-tune tables at runtime with x87, using 2 and 1/384. It has a single stereo flag and the older 35-read OPL data delay. Its 21-byte CONST segment verifies at `2:01e40`.
- MUSIC checks the result of MIDI backend installation. WaveBlaster setup preserves the caller's MIDI port. The AWE32, FM, GUS, MIDI-port and volume-fade routines now individually verify STRICT. The whole 1,945-byte code segment matches at `1:58b80-1:59319`. The error-string function still needs a CONST byte match.
- MPU401 initializes its delay value to 500 and GUSMIDI uses `ULTRAMID.INI`. Both initialized segments now verify. The latter corrects the earlier notes about the filename.
- Allocator and deallocator bindings at `1:2bb60` and `1:2bb90` reuse the independent STRICT evidence in `docs/GAME_MAP.md`.

## NEGATIVE and OPEN

- The MULTIVOC 1.04 TASM mixer control and the later C source did not reproduce the early mix-loop entries. With the early C mix loops, `-zu` or `-omaxnet` reduced matches. With the backported AL_MIDI and MIDI, the previous `-zu` profile also produced fewer matches. An AL_MIDI `-fpi87` probe did not independently improve the result.
- Several terminal routines have equal code and relocation sites through their final RET, followed by the next member's alignment gap. The current verifier still reports an unconfirmed extent for them. The worker overlay has no invented function-start symbols for those boundaries.
- Target-linked BSS and initialized-data layouts still need reconciliation where the native report shows a mismatch. Watcom's emitted BSS order can change with declarations or local names. This was observed directly with the task stack variables and DPMI register work areas.
- The remaining MULTIVOC differences include the playback and VOC parser paths, several shared epilogues, and BSS member order. TASK_MAN still differs in timer-rate selection, startup and task insertion. BLASTER Init, AL_MIDI timbre and voice programming, MIDI event dispatch and PlaySong, and the GUSWAVE parser remain the largest source differences.
- Moving the MULTIVOC and MIDI data declarations did not reproduce their BSS order. The GUSWAVE alternate `-zu`, `-omaxne` and debug builds reduced code matches. Reordering its parser locals and the FX setup argument expressions left their relocation and entry-size discrepancies unresolved.
- Several initialized-segment differences sit between named objects or after the NUL in a string. Examples are the AL_MIDI operator-offset tail at +0x5a, the PAS stack-selector tail at +0x32 and the first PAS error string's tail at +0x16. The data audit retains each byte difference even where a source-level field or literal agrees.
- One rebuild used the compiler wrapper's newly introduced shared staging path at `build/cc`. The worker driver now sets its staging location to `build/workers/audio/cc`. All final artifacts were regenerated with that local setting.
- The older research report `NOTES.md` and the integrated source tree are reference points for the first pass. This report and the new verifier artifacts supersede first-pass findings for the changed candidates.
