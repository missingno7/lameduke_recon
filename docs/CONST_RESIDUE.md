# Watcom 9.5b CONST record residue

## Result

The target's five nonempty CONST contributions from currently integrated 9.5b units match with a zero seed in each CONST packet buffer. The bytes retained from that seed are all zero in the target. Three different compile environments produced all five target pools, with different THEADR names, dependency paths, and timestamps. The EXE therefore cannot identify the historical environment from these pools.

The measured replacement stages under `D:\Prog\lameduke_recon\build\workers\sounds\stage` with absolute source arguments, `s\out.obj`, temporary directory `t`, `TZ=UTC0`, and staged file mtime 788227200. It retains the repository's flags and the pinned Watcom header installation. The directory is a verified reproducing choice for this checkout. A different absolute checkout location needs verification.

## Record buffers and the byte model

**PROVEN** The 9.5b writer at PE RVA `6bcda` receives the record type in AL, the payload address in EDX, and the payload length in EBX. It constructs the three-byte header and checksum separately. The CONST buffer has a 256-byte allocation observed after the data array initializer at `5b192`. Its allocation does not clear it. Normal CONST LEDATA payloads reserve five bytes for the segment index and 32-bit offset. A literal overwrites its own byte span, including explicit and implicit terminating NULs. Alignment within a packet advances the cursor and retains existing bytes. Flushing sets the used count to zero and preserves the payload storage. A skipped alignment region between packets has no payload bytes. WLINK fills those holes with zero as established by the earlier natural-link test.

**PROVEN** In the traced runs, THEADR and the dependency COMENT records use a front-end metadata array with a different live address than the CONST array. The metadata array is released after the segment definitions at `5aa3c`. The CONST SEGDEF itself uses the start of the CONST array and writes eight payload bytes. It cannot account for the observed padding lanes, the earliest of which are farther into the array. Trace comparison uses the emitted payload, before the writer later patches the on-disk SEGDEF length. The linked target LE does not retain these compiler dependency records. See also the separate payload and record header writes in the later [Open Watcom x86 object writer](https://raw.githubusercontent.com/open-watcom/open-watcom-v2/master/bld/cg/intel/c/x86obj.c).

The replayer records the seed at allocation in trace mode and tracks only writes at overlapping addresses. In offline mode it tests a zero seed and replays the ordinary literal writes. Its input includes packet boundaries and literal spans, with the OMF fixups providing the materialized literal starts. It does not read padding back into the predicted buffer, change any object, or alter the strict verifier. The reports label each covered pad by the earlier CONST LEDATA write or by the seed slot. The seed is host allocator state. Paths, names and mtimes alone do not predict it in every configuration.

| Unit | CONST bytes | Covered padding | From earlier CONST LEDATA | From initial seed | Linked holes |
|---|---:|---:|---:|---:|---:|
| SOUNDS | 688 | 75 | 44 | 31 | 0 |
| GAMEDEF | 1,598 | 123 | 73 | 50 | 11 |
| ENGINE | 75 | 0 | 0 | 0 | 0 |
| MULTI | 118 | 0 | 0 | 0 | 0 |
| MENUES | 492 | 28 | 26 | 2 | 0 |

The original places 83 zero observations on 64 distinct per-unit seed slots. The other 143 observations agree with previous CONST literal writes. For example, SOUNDS retains `hoo` and `unn` from the earlier packet in the gaps after `bodybloa.voc` and `bodyblor.voc`. ENGINE and MULTI pack their literals and provide no padding observations. SECTOR and MULTI_DATA have empty CONST contributions.

### Positive allocator control

At the worker's alternate `cc\s` stage, GAMEDEF emits `0x38` at CONST `+0xb`. Late debugger attachment preserves that ordinary compile's output. A hardware watchpoint showed the byte already present before the first THEADR write and untouched until the first CONST LEDATA. It was inside a 2,240-byte free block, in adjacent duplicate pointer words whose low 16 bits were `0x0038`. This fits the allocator's in-band links at RVA `6b9ae–6b9ce`. No OMF record was emitted in time to write this byte. The evidence does not identify the exact pre-THEADR instruction that originally wrote the stale pointer. Starting the compiler under a debugger instead can change the seed, including reproducing Windows debug-heap fill.

The original canonical SOUNDS object has `0x04` at `+0xa7`, which is payload byte `0xac` of its first CONST packet. This position has no earlier CONST write. It is beyond the length of every preceding serialized payload. The zero-seed replay identifies this as the sole failing seed observation. The specific retired allocation that supplied `0x04` in that canonical run was not captured.

## Environment inference and negative controls

- The pinned native writer's `80 e9` COMENT payload contains a little-endian 32-bit epoch value, a one-byte name length, and the path. The two-byte payload `80 e9` terminates the list. This is neither a packed DOS date/time word pair nor a local-time-neutral serialization in the native runtime's default timezone. A source mtime of 788227200 emitted 788245200 in the default runtime timezone and 788227200 under `TZ=UTC0`. The installed May 1993 `stdio.h` emitted 736624200 and 736609800 respectively.
- **PROVEN, historical reference objects only** Ken's BUIL0422 and BUIL0801 ENGINE.OBJs both have THEADR `engine.c`, a source dependency named `D:\engine.c`, and system dependencies under `c:\watcom\h`. Their `stdio.h` epoch is 736609800, corresponding to 1993-05-05 13:50:00 UTC. The source epochs are 1994-04-20 16:42:28 UTC and 1994-08-01 15:13:12 UTC, respectively. This supports a historical hypothesis for Ken's setup. It does not establish the December target's working directory, basenames, include path, compiler timezone, or source mtimes.
- Holding the proposed stage and header paths fixed while changing the staged source/header mtime from 788227200 to 788736000 changes the dependency records and leaves all five target pools exact. A second environment uses longer `staging_long\s` paths, copied byte-identical compiler headers under `alt-headers` with mtime 700000000, and different source basenames throughout. Every pool still matches. These controls make the environment ambiguity constructive.
- Twelve native SOUNDS compiles with the same short relative source argument, staged files, flags, timestamps and include paths had identical serialized OMF prefixes through the first CONST record boundary. They yielded twelve distinct padding variants in one wave and ten in a repeat, due to the byte at `+0x19`. This demonstrates dependence on process allocation state after fixing all named environment parameters.
- `-zld` removes dependency entries but keeps the terminator. It happens to clear the SOUNDS differences in the tested worker stage. GAMEDEF then emits `0x8c` at `+0xb`. Disabling dependency emission is insufficient as a project-wide fix.
- Removing the six explicit NUL extensions in SOUNDS under the proposed stage yields 687 CONST bytes and differences at `+27f`, `+29b`, `+2aa` and `+2ab`. The compiler environment change does not eliminate the need for the current source padding in that candidate.
- An unbounded path sweep is unsafe for this compiler. The trial stopped at a 69-character stage component after the compiler hung. The proposed stage stays within the measured short range.

## Verification and reproduction

The gate ran its ten tests and fresh registered rebuild with the worker copy of `wcc.py`. The baseline column uses the canonical gate report and canonical compiled objects. The replacement column uses a fresh compile and the unmodified verifier. The registered units at capture contained 574 functions.

| Unit | Canonical CONST | Proposed CONST |
|---|---|---|
| SOUNDS | MISMATCH, one byte at +a7 | VERIFIED, 688 B |
| GAMEDEF | VERIFIED | VERIFIED, 1,598 B |
| ENGINE | VERIFIED | VERIFIED, 75 B |
| MULTI | VERIFIED | VERIFIED, 118 B |
| MENUES | VERIFIED | VERIFIED, 492 B |

The only function status changes were `bootleg_cache_alloc`, `USRHOOKS_GetMem` and `playmusic`, each promoted from CODE_BYTES_MATCH to STRICT. Gate totals increased from 436 to 439 STRICT functions, and from 73,226 to 73,498 STRICT bytes. SOUNDS has 12 STRICT functions and its existing 464-byte `spritesound` NEAR_MATCH. Results are in `gate_residue/comparison.json`.

```powershell
python -B build/workers/sounds/gate_residue.py
python -B build/workers/sounds/replay_const.py --report canonical
python -B build/workers/sounds/replay_const.py --local --report proposal
python -B build/workers/sounds/replay_const.py --trace --report traced
python -B build/workers/sounds/observe_heap.py
python -B build/workers/sounds/audit_environment.py
```

The scripts write per-position provenance and environment controls under `records`. The debugger is diagnostic only. The gate uses ordinary compiler launches, fixed staging, no forced initial memory contents, and no object postprocessing.
