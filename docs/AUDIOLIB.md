# Apogee Sound System in the December 1994 LE image

The audio contribution occupies `1:49c84–1:62d8d` (exclusive end). Creative's
`PAWE32` comes first, then Gravis's `GF1_OSF`, then Apogee C. The first Creative
member, `NRPN`, starts 0x4220 bytes before the previous `1:4dea4` estimate.
`1:62b9c` is the start of `ADLIBFX_SetCallBack`. The next two AdLib FX functions
run through `1:62d8d`, where the checked runtime starts.

`modules.json` records the ordered code members and the data-only evidence.
`functions.json` records each 1.04 OMF code symbol and each additional source
symbol, with its locations and comparisons where available. The
`other_funcmap_entries` fields preserve candidates from the LE function map,
which can include internal labels and data. `results.json` aggregates the
unmodified verifier outputs in `results/*.results.json`; the companion logs
include relocation diagnostics and data segment checks.

The final build matches whole code and sites for 13 Apogee source members
(13,108 bytes). The independent verifier reports the following function
totals across the source candidates. Byte counts are candidate extents.
The table excludes CODE data publics and zero-size aliases. Identified
vendor objects are outside these totals.

| Function status | Functions | Bytes |
|---|---:|---:|
| STRICT | 184 | 16,068 |
| CODE_BYTES_MATCH | 59 | 9,857 |
| NEAR_MATCH | 52 | 15,031 |
| UNPLACED | 91 | 13,747 |

The pinned oracle SHA-256 is
`698979d97f0f2a16ffdea20ff0059fe49ef9720a7c8933a61601356ca0462701`.
The 1.04 library SHA-256 is
`553baedbf2c59dc99070e732ffbf2707cfe449670336d3435f4cd42b67470898`.

## Code members in link order

The ends are exclusive. `Next` gives the next member's start as an upper
bound where the code end and its alignment padding remain to be separated.
Whole code+sites equality permits linked operands to differ from the OMF
fields and checks intra-segment targets where resolvable. Function-level
`STRICT` additionally requires confirmed extents and verified fixup targets.

| Member | obj1 range | Comparison with 1.04 | Compiler evidence |
|---|---|---|---|
| NRPN | 49c84–4a77c | Whole code+sites | MetaWare High C, same member |
| SBKLOAD | 4a77c–4ba90 | Whole code+sites | MetaWare High C, same member |
| HARDWARE | 4ba90–4c4f4 | Whole code+sites | MetaWare High C, same member |
| MIDIENG | 4c4f4–4e168 | Whole code+sites | MetaWare High C, same member |
| OS | 4e170–4e5a3 | Whole code+sites | GF1_OSF reference |
| GF1SOUND | 4e5b0–4ec37 | Whole code+sites | GF1_OSF reference |
| GF1MIXER | 4ec40–4ed2e | Whole code+sites | GF1_OSF reference |
| GF1REC | 4ed30–4ed84 | Whole code+sites | GF1_OSF reference |
| GF1UART | 4ed90–4edd7 | Whole code+sites | GF1_OSF reference |
| GF1MIDI | 4ede0–4f14d | Whole code+sites | GF1_OSF reference |
| GF1ERROR | 4f150–4f265 | Whole code+sites | GF1_OSF reference |
| TIMER | 4f270–4f4f2 | Whole code+sites | GF1_OSF reference |
| GF1NOTE1 | 4f500–4f657 | Whole code+sites | GF1_OSF reference |
| GF1NOTE | 4f660–51a94 | Whole code+sites | GF1_OSF reference |
| DIGPOS | 51aa0–51beb | Whole code+sites | GF1_OSF reference |
| DIGPAUSE | 51bf0–51c56 | Whole code+sites | GF1_OSF reference |
| DIGITAL | 51c60–52fe5 | Whole code+sites | GF1_OSF reference |
| GF1WAVE | 52ff0–5313d | Whole code+sites | GF1_OSF reference |
| GF1PAT1 | 53140–5326f | Whole code+sites | GF1_OSF reference |
| GF1PATCH | 53270–53883 | Whole code+sites | GF1_OSF reference |
| GF1ADDR | 53890–53991 | Whole code+sites | GF1_OSF reference |
| GF1DMA1 | 539a0–539dd | Whole code+sites | GF1_OSF reference |
| GF1DMA | 539e0–53ea9 | Whole code+sites | GF1_OSF reference |
| GF1VOICE | 53eb0–540bd | Whole code+sites | GF1_OSF reference |
| GF1UTILS | 540c0–541c0 | Whole code+sites | GF1_OSF reference |
| GF1DRAM | 541c0–54553 | Whole code+sites | GF1_OSF reference |
| GF1MEM | 54560–54996 | Whole code+sites | GF1_OSF reference |
| GF1DETCT | 549a0–549ff | Whole code+sites | GF1_OSF reference |
| GF1ISR | 54a00–54f45 | Whole code+sites | GF1_OSF reference |
| GF1INIT | 54f50–558e2 | Whole code+sites | GF1_OSF reference |
| GF1WISR | 558f0–559ea | Whole code+sites | GF1_OSF reference |
| GF1SEM | 559ec–55a5c | Whole code+sites | GF1_OSF assembly reference |
| GF1INIT2 | 55a5c–55abf | Whole code+sites | GF1_OSF assembly reference |
| GF1AINIT | 55ac0–55b2b | Whole code+sites | GF1_OSF assembly reference |
| USER | 55b30–55bec | Changed | 10.0 reproduces whole code, 10.0a produces 1.04 code |
| TASK_MAN | 55bf0–56490 (next) | Changed | 10.0 shares timer routines, task list differs |
| SNDSRC | 56490–56bee | Changed | 10.0 plus callback and rate backport reproduces whole code |
| PITCH | 56bf0–56dbd | Changed | 10.0 plus restored x87 initializer reproduces whole code |
| PCFX | 56dc0–5734b | Changed | 10.0 with one later public removed reproduces whole code |
| PAS16 | 57350–58b80 (next) | Changed | 10.0 has more exact public extents than 10.0a |
| MUSIC | 58b80–59319 (next) | Changed | 10.0 backport reproduces portions of the old FX card dispatch and MIDI API |
| MULTIVOC | 59319–5b330 (next) | Changed | 10.0 reproduces a few small functions, voice structures differ |
| MPU401 | 5b330–5b58d | Whole code+sites | Both 10.0 and 10.0a reproduce the whole code |
| MIDI | 5b590–5c6c0 (next) | Changed | 10.0 with `-zu` and per-channel user volume removed reproduces more functions |
| LL_MAN | 5c6c0–5c779 | Whole code+sites | Both 10.0 and 10.0a reproduce the whole code |
| IRQ | 5c780–5cb3c | Changed | Both 10.0 and 10.0a reproduce the target code |
| GUSWAVE | 5cb40–5d4f0 (next) | Changed | 10.0 reproduces most public extents using the early 84-byte voice layout |
| GUSMIDI | 5d4f0–5dba2 | Changed | 10.0 optimized with `-zp1` and early patch config source reproduces whole code |
| GUS | 5dba2–5dd9f | Changed | 10.0 `-d2` reproduces whole code with the earlier voice layout |
| FX_MAN | 5dda0–5e9e0 (next) | Changed | 10.0 backport places 15 entries, with two NEAR_MATCH extents |
| DPMI | 5e9e0–5eb7a | Whole code+sites | Both 10.0 and 10.0a reproduce the whole code |
| DMA | 5eb80–5ef23 | Whole code+sites | Both 10.0 and 10.0a reproduce the whole code |
| BLASTER | 5ef30–60ab0 (next) | Changed | `BLASTOLD.C` is the useful source lineage, 10.0 reproduces a subset |
| AL_MIDI | 60ab0–61e80 (next) | Changed | Several blocks reproduced with 10.0 and `-zu` |
| AWE32 | 61e80–62757 | Changed | 10.0 plus early initialization reproduces whole code |
| ADLIBFX | 62760–62d8d | Changed | 10.0 plus early voice setup reproduces whole code |

PROVEN. The 38 whole 1.04 placements account for 50,713 code bytes. The stock
`tools/libscan.py` reports 34 whole exact+sites hits (33,077 bytes) and 345
split hits (48,484 bytes). The split total includes duplicate hits. The four
extra whole placements above are Creative High C Easy OMF members. The
stock parser treats their LOC=5 fields as two bytes. The diagnostic census
uses four-byte fields only for High C Easy OMF and checks agreement with
LE fixup sites and intra-segment destinations. In particular `AWE32_Init`
independently calls the NRPN start/end thunks at `1:49c84` and `1:4a774`,
and its initializer at `1:49c8c`. The strict verifier and OMF reader were
used without changes. Creative and Gravis vendor matches are library identification.
The same stock split scan gives 329 exact+sites hits for 1.09, 299 for 1.1,
298 for 1.12, 297 for 2002 production and 216 for 2002 debug.

## Data-only members and source differences

- PROVEN. Eight Gravis initializer records occupy `2:abdc–2:ac0c`, in the
  order WDIGITAL, WTIMER, GF1WSND, GF1WUART, GF1WNOTE, GF1WDMA, GF1WVOIC,
  GF1WMEM. GF1AINIT loads both bounds. Their flag words and absolute
  fixup targets identify the eight data-only members. VOLTAB, GF1SCALE,
  DIVISOR, GLOBALS, NRPNVAR, MIDIVAR and EMBED have bound initialized publics.
- PROVEN. The target's 2,170-byte general MIDI data member is reproduced by
  `GMTMBOLD.C` and `OLDTIMBR.C`. Its `_PercussionTable` starts at `2:9e80`
  and `_ADLIB_TimbreBank` at `2:9f80`. The source build selects GMTMBOLD.
  The 1.04 GMTIMBRE data, GMTEMP, LEEOLD and LEETIMBR probes did not match.
- PROVEN. `PITCH_Init` computes a 12×25 table with x87 `pow` and the 16.16
  conversion. The released source retains the initializer in a comment.
  Sound Source uses a 500 Hz task and defers the callback until the FIFO
  loop exits. Its `No sound playing.` string starts at `2:14b4`. The 1.04
  string mentions SndSrc and shifts subsequent CONST offsets by eight bytes.
- PROVEN. The target has neither 1.04 `PCFX_VoiceAvailable` nor
  `ADLIBFX_VoiceAvailable` between the adjacent playback entries.
  ADLIBFX reserves OPL voice zero during initialization and releases it at
  shutdown. The later source retained both calls in comments.
- PROVEN. The early GUS MIDI error routine belongs to GUSMIDI, while
  GUSWAVE_Shutdown belongs to the debug-built GUS member. The early loader
  reads `ULTRASND.INI` under the instrument directory. The
  released `GUSMIDI2.C` retains that path-building code. The target's
  `Can't find ULTRASND.INI file.` is at `2:1a34`. 1.04 tries
  `GUSMIDI.INI`; 1.09 and later select `ULTRAMID.INI`.
- PROVEN. GUSWAVE's early 84-byte voice records contain a direct VOC
  continuation path, with no separate format dispatcher. The 1.04
  candidate has 101-byte records and adds the WAV and demand-feed paths.
  The early initializer allocates 896 bytes per voice and GUS shutdown
  frees all eight buffers.
- PROVEN. FX has 15 located entries, with a 12-card enum that omits
  SoundScape. It uses the four-argument MV_Init mix-mode interface and
  reports `Invalid VOC file.` at `2:1ad4`. The 1.04 ASS version error and
  expanded WAV/raw/looping/recording entry points are absent from this member.
  MUSIC uses an `AWE32 error.` literal at `2:171c`. AWE32's earlier driver
  calls BLASTER_GetEnv directly, uses the address as the EMU fallback, and
  tries InitMPU once.
- STRONG. DEBUGIO, SNDSCAPE and the 1.04 MV_MIX member have no distinct
  linked extent here. Tiny SNDSCAPE hits resolve into other drivers.
  The C mixer and VOC parser are inside MULTIVOC. An additional assembly
  source for the early mixer remains unestablished.

## Compiler and assembler probes

The calibration used the `origin/al_104` commit `5f2d287`, revision macro
`LIBVER_ASSREV=19950518`, and the per-file production options from that
commit's AUDIO.MAK. The macro selects the ancestry baseline in the GPL
sources. It does not date the target. The release source and the
reconstruction branch's alternate files supplied additional cross-checks.

| Watcom install | 1.04 identical public extents (bytes and normalized fixups) | 1.04 identical nonempty C code segments | Target discriminators |
|---|---:|---:|---|
| `wc95b` | 221 / 468 | 0 | USER and IRQ whole code fail, DMA has no identical 1.04 public |
| `wc100` | 429 / 468 | 10 | Whole USER code and the better PAS16 control |
| `wc100a` | 466 / 468 | 22 | Reproduces the 1.04 USER member, no whole target USER hit |

STRONG. Watcom 10.0 GA reproduces the target code for 13 complete source
segments with the recovered options. In contrast, the linked runtime is
Watcom 9.5. The evidence supports separate compiler builds for the library
and the runtime. 10.0a is close enough to 1.04 to reproduce 22 nonempty
code segments, but IRQ and TASK_MAN prevent a complete 1.04 result.
10.0b is unavailable in the pinned installs. The full comparison excludes
debug metadata, filenames and timestamps. Raw OMF objects were never used
as reconstruction inputs.

NEGATIVE. With the 1.04 control sources, adding `-3r`, `-4r`, `-s`,
`-omaxne`, `-od`, `-zu` or `-zp1` as individual probes did not explain
the changed members. GUSMIDI specifically needs a fresh optimized
invocation. Adding `-d1` after the original `-d2` left frame setup in
function bodies. Replacing the debug profile with the production options
and `-zp1` was decisive. Building `MV1.C` against the release headers
fails due to obsolete voice layouts and signatures. It remains useful
source evidence for the earlier mixer family.

PROVEN. TASM 3.1 identifies itself in the 1.04 mixer OMF. It is unavailable
locally. TASM 4.0 under MS-DOS Player reproduced 1.04's 1,210-byte TEXT
segment and all 57 normalized fixups from MV_MIX5 after replacing the
six IDEAL-order PROC declarations with MASM-order PROC declarations.
The instructions were unchanged. NEGATIVE. None of the compiled mixer
variants MV_MIX, MV_MIX1 through MV_MIX6, or MVREVERB found a whole or
per-public match across obj1. GF1 assembly results remain historical
library identifications.

## Reproduce

See [BUILD.md](BUILD.md) for commands. `overlay.json` includes evidence
for every exported binding. Duplicate local names such as D32DosMemAlloc
and the PAS/BLASTER stack helpers are omitted from the shared binding
namespace. Whole code+sites results are recorded even when the function
verifier requires additional data evidence or an independent boundary
between a TU end and linker padding.

## Open work

- Complete the earlier MULTIVOC voice layout and playback/allocator
  paths. GUSWAVE's remaining differences concentrate in register
  allocation and scheduling in the VOC parser and playback entry.
- Recover the early task-list representation and the remaining PAS,
  BLASTER, AL_MIDI, MIDI and MUSIC functions. The TS_FreeTaskList caller
  passes an eight-byte aggregate and the routine traverses a null-ended
  chain. The 1.04 source uses a circular sentinel list.
- The FX_SetupCard candidate uses different registers for PAS_GetCardInfo's
  two field addresses. Its instruction sequence is two bytes shorter, which displaces
  entries in the switch table included in FX_ErrorString's candidate
  extent. FX_Shutdown uses a compiler-generated frame via `#pragma aux`.
- Check CONST padding, private data symbols and exact extents, especially
  where a TU ends just before inter-module alignment bytes. Finish
  function- and data-level verification before attempting a whole link.
