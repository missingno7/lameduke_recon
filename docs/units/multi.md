# December 1994 MULTI.OBJ

**Partial.** The candidate builds all 25 C routines in address order. Watcom
9.5b with `-4r -s` gives 18 STRICT functions and 3,164 verified code bytes.
Seven functions are NEAR_MATCH, covering 5,247 candidate bytes and 5,455 oracle
bytes. The candidate has multiple implied `_TEXT` bases. Whole-object
matching remains open. Verification was static.

## Extent and provenance

- PROVEN. The preceding ENGINE `setbrightness` ends at `1:47abf`. The
  24-byte prefix at `1:47abf–47ad7` has six LE fixups to the baud-rate cases
  inside `initmultiplayers`. The compiler produces the same prefix and all
  six targets in the STRICT result for `initmultiplayers`.
- PROVEN. The terminal `ret` of `getoutputcirclesize` is at `1:49c81`.
  Two unreferenced zero bytes at `1:49c82–49c84` align the independently
  identified Creative NRPN contribution. Its ASS 1.04 `AUDIO_WF.LIB`
  `nrpn.c` object has `_TEXT` SEGDEF alignment 5 (dword). The overlay's
  `multi_text_end` labels the independently established extent boundary at
  `1:49c82`. See `docs/AUDIOLIB.md` and `audit.py map`.
- STRONG. Ken's `buil1995/BUILD.TXT` entry for 9/15/94 names the split
  into MULTI.OBJ. The 10/16/94 entry documents the terminal API function.
  `BUIL0801/ENGINE.OBJ` provides exact+sites anchors for `netoff`,
  `comsend`, `convalloc32`, `simulateint`, `getcrc`, and
  `uninstallbicomhandlers`. The checked-out canonical registry did not
  contain these bindings, so the worker overlay supplies them.
- Source evidence includes K1's `BUIL0422/ENGINE.C`, `BUIL0801/ENGINE.OBJ`,
  `buil1995/BICOMC.C`, and `2DRW0628/MULTI.OBJ`. The later Ken Silverman
  `MULTI.C` is in `external/duke3d-1.3d/dukenukem3d/SRC/MULTI.C` and
  `reference/build-reconstructions/MULTI.C` in that same checkout. CRC,
  COM setup and shutdown, DOS interrupt setup, and player-list logic use the
  surviving C as a baseline. The early packet formats and protected-mode
  IPX handler were reconstructed from the December executable. Respect the
  later Build sources' `BUILDLIC.TXT` provenance when integrating.
- NEGATIVE. K1's June and later MULTI objects scanned with
  `tools/libscan.py --split --min 5` yielded no further exact+sites hits in
  this interval. The later objects add `startcom`, `setsocket`, 16 KiB
  rings and a different IPX receive routine. K1 BUIL0906 and BUIL1109
  objects yielded zero anchors in this interval.

## C functions

All extents and differences use the original verifier. First differences
are offsets within the candidate, excluding relocation fields. Many branch
displacements change as downstream instructions move. An early displacement
difference can therefore precede the source expression that explains it.

| Function | Oracle start | Oracle bytes | Candidate bytes | Status | First difference |
|---|---|---:|---:|---|---|
| initmultiplayers | `1:47ad7` | 334 | 334 | STRICT | |
| uninitmultiplayers | `1:47c25` | 40 | 40 | STRICT | |
| netoff | `1:47c4d` | 56 | 56 | STRICT | |
| neton | `1:47c85` | 1,079 | 949 | NEAR_MATCH | `+03` EDI save versus EBP |
| comon | `1:480bc` | 523 | 523 | STRICT | |
| comoff | `1:482c7` | 250 | 250 | STRICT | |
| netsend | `1:483c1` | 593 | 594 | NEAR_MATCH | `+03` additional EDI save |
| comsend | `1:48612` | 141 | 141 | NEAR_MATCH | `+5d` second escape store uses ADD and a different fixup site |
| nethandler | `1:4869f` | 228 | 228 | STRICT | |
| comhandler | `1:48783` | 370 | 370 | STRICT | |
| netinitconnection | `1:488f5` | 515 | 484 | NEAR_MATCH | `+15` return register on duplicate ID |
| netuninitconnection | `1:48af8` | 82 | 82 | STRICT | |
| convalloc32 | `1:48b4a` | 64 | 64 | STRICT | |
| simulateint | `1:48b8a` | 203 | 203 | STRICT | |
| sendpacket | `1:48c55` | 1,054 | 1,026 | NEAR_MATCH | `+13` displacement to common exit |
| getpacket | `1:49073` | 1,424 | 1,409 | NEAR_MATCH | `+16` displacement to zero return |
| initcrc | `1:49603` | 88 | 88 | STRICT | |
| getcrc | `1:4965b` | 68 | 68 | STRICT | |
| installbicomhandlers | `1:4969f` | 248 | 248 | STRICT | |
| uninstallbicomhandlers | `1:49797` | 128 | 128 | STRICT | |
| allocbicom | `1:49817` | 174 | 174 | STRICT | |
| processreservedmessage | `1:498c5` | 649 | 644 | NEAR_MATCH | `+14` displacement to the type 254 arm |
| sendlogon | `1:49b4e` | 185 | 185 | STRICT | |
| sendlogoff | `1:49c07` | 71 | 71 | STRICT | |
| getoutputcirclesize | `1:49c4e` | 52 | 52 | STRICT | |

`allocbicom` is a descriptive name. The June object and later C fold its job
into `installbicomhandlers`. The six-entry COM configuration switch,
1/16-byte UART FIFO behavior, 4096-byte rings, 64 serial sequence numbers,
and two IPX payloads per ordinary packet all have December code evidence.
The observed DOS memory side effects are preserved. This includes the
initial write through `incnt` before dispatch and the low-memory template
copy after the two clock stores in `allocbicom`.

## Data and real-mode routines

| Contribution | Address | Bytes | Evidence |
|---|---|---:|---|
| C `CONST` | `2:011d0–01246` | 118 | VERIFIED by `match.py`, including string boundaries |
| C `_DATA` prefix | `2:02cf0–02cfa` | 10 | VERIFIED by `match.py`. Two option bytes and two clock pointers |
| Serial template | `2:02cfa–02e7a` | 384 | MASM 5.10 source assembly compared to all 384 oracle bytes, including ten exact rel16 branch values |
| C `myconnectindex` | `2:02e7a–02e7c` | 2 | VERIFIED by `match.py --obj` on `multi_data.obj` |
| C `_BSS` | `2:1bded4–1bedf2` | 3,870 | Expected extent from array strides and register-frame offsets. Candidate segment size matches. Publics imply different bases |

The original serial template includes 21 bytes of shared state, 194 bytes
of 16-bit code, and zero fill to 384 bytes. The later MULTI.C names the
assembly origin `realcom.asm`. The worker's `src/realmode.asm` rebuilds
this template with ordinary instructions and declared data. The audit
compares each OMF rel16 field against the displacement to its independently
decoded oracle branch target. The assembled template has no LE absolute
fixup sites and is checked separately from the C function score.

December `neton` wrote a 17-byte real-mode callback into conventional
memory. The C candidate copies the assembled template and fills its two SI
immediates at runtime. The audit verifies the constant bytes and placement
of both immediates against the original stores. This accounts for missing C
instruction bytes in `neton`. The extra template segment's final placement
and a natural link of the reconstructed objects are open.

The candidate factors C storage into `multi_data.c` so the code object can
verify its named references independently of Watcom's current BSS ordering.
The original owned this storage in MULTI.OBJ. Some private names
(`prevpacket`, `prevpacketindex`, `prevpacketlength`) are descriptive,
with addresses independently derived from producers and consumers.

## Compiler findings and reproduction

- PROVEN. With the same C source, `wc95b -4r -s` gives 18 STRICT and the
  requested `engine` flags `-4r -s -or` give 2 STRICT (92 bytes).
  Scheduling changes are visible in `convalloc32` and
  `uninstallbicomhandlers` even though the C expressions correspond to
  later historical source. `-or` controls scheduling in the compiler.
  [Open Watcom user guide](https://open-watcom.github.io/open-watcom-v2-wikidocs/cguide.html)
- PROVEN. Watcom 9.5 shares epilogues across function boundaries. The
  `uninitmultiplayers` IPX arm falls through to `netoff`.
  `comhandler`, `installbicomhandlers` and `sendlogoff` reproduce
  branches into earlier epilogues or call sequences. Source order matters.
- Compiler steering is limited to C expression forms and evidence-backed
  DOS inline I/O and interrupt primitives. The leading `j = 0` and
  polynomial local in `initcrc`, the signed long ring index in
  `nethandler`, the direct versus `+0` I/O arguments in `comon`, and
  the early FIFO counter decrement in `comhandler` are byte-significant.
  The integer-address expression for the second `comsend` escape leaves
  five non-relocation byte differences and one displaced fixup.
- NEGATIVE. Watcom 9.5b `-4r` by itself emits stack checks and yields 2
  STRICT interrupt handlers. Adding `-ot`, `-os`, `-ol`, or `-oa` to
  `-4r -s` gave 3, 2, 7, and 11 STRICT respectively on the preceding
  candidate. `-d1` and `-zq` gave the same 18. `-zp4` displaced
  `simulateint`. This compiler rejected `-ob` and `-or-`.
- NEGATIVE. With `-4r -s`, moving BSS definitions before or after the
  functions left the same non-relocation function differences.
  Equivalent array indexing, local index types and pointer volatility trials
  left `comsend` mismatches. Byte casts, comma grouping, and
  implicit-int-style returns left `netsend` mismatches.

From the repository root, run the commands sequentially.

```powershell
python -B build/workers/multi/verify.py
python -B build/workers/multi/verify.py --engine
python -B build/workers/multi/verify.py --data
python -B build/workers/multi/audit.py serial-check
```

The first two use the repository's pinned `tools/wcc.py` implementation
and its unmodified `tools/match.py --obj` path. The wrapper keeps temporary
and object writes inside this worker, while the matcher's canonical and
library bindings are still read from their original paths. The control's
`results_engine.json` is the requested profile's result. `results.json`
records the better `-4r -s` reproduction. Both function runs return 1 for
remaining mismatches. `--data` prints segment checks and has no function
rows, so the BSS result must be read from the segment line.
