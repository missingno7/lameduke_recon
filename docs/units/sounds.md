# SOUNDS reconstruction

## Boundaries and build

**PROVEN** The previous routine returns at `1:2ba26`. Nine zero bytes without LE fixups precede the allocator entry at `1:2ba30`. The last routine returns at `1:2c1ba`. Five zeros without LE fixups precede the menu initializer at `1:2c1c0`. `sounds_text_end` records the independently observed end boundary `1:2c1bb` for the verifier. The linked placement interval includes those final five zeros.

The thirteen entries below occupy all 1,931 bytes in `1:2ba30–2c1bb`. The funcmap reaches twelve entries. The padding after the pop routine ends at `1:2bae0`, where a complete, unreferenced function walks the allocation stack and returns immediately before the initializer's alignment. The full C build produces both the pop and the reset at those independent starts. Internal padding follows Watcom's 16-byte instruction fill. The candidate's sole `_TEXT` base is `1:2ba30` and every public lands on its oracle start.

The build uses the `game` profile, Watcom 9.5b `-4r -s -otexan`. Run from the repository root. The worker-local entry point calls the unmodified verifier and pinned compiler wrapper, and directs all build writes to this worker. The verifier parses the source path before the `-i=` argument.

```powershell
python -B build/workers/sounds/match_local.py --profile game --symbols build/workers/sounds/overlay.json build/workers/sounds/src/sounds.c "-i=build/workers/sounds/src" --json build/workers/sounds/results.json
python -B tools/bindprop.py build/workers/sounds/results.json
```

## Functions

| Function | Oracle extent | Bytes | Status |
|---|---|---:|---|
| `bootleg_cache_alloc` | `1:2ba30–2baa0` | 112 | STRICT |
| `bootleg_cache_pop` | `1:2baa0–2bae0` | 64 | STRICT |
| `bootleg_cache_reset` | `1:2bae0–2bb30` | 80 | STRICT |
| `init_bootleg_cache` | `1:2bb30–2bb60` | 48 | STRICT |
| `USRHOOKS_GetMem` | `1:2bb60–2bb90` | 48 | STRICT |
| `USRHOOKS_FreeMem` | `1:2bb90–2bbd0` | 64 | STRICT |
| `loadsounds` | `1:2bbd0–2bc40` | 112 | STRICT |
| `playmusic` | `1:2bc40–2bcb0` | 112 | STRICT |
| `xyzsound` | `1:2bcb0–2be70` | 448 | STRICT |
| `spritesound` | `1:2be70–2c040` | 464 | NEAR_MATCH |
| `stopsound` | `1:2c040–2c090` | 80 | STRICT |
| `pan3dsound` | `1:2c090–2c190` | 256 | STRICT |
| `TestCallBack` | `1:2c190–2c1bb` | 43 | STRICT |

The voice fields at +0, +4, +12, +14 and +18 are confirmed by the oracle. Both play routines store Y and then Z at +4. The reconstruction preserves the two writes. The voice field at +8 and the file-table dword at +4 have no confirmed use in this TU. Source names and declaration order remain provisional where no historical source survives.

## Data

| Contribution | Oracle addresses | Size | Verification |
|---|---|---:|---|
| CONST | `2:006a8–00958` | 688 | VERIFIED, full byte coverage |
| `_DATA` | `2:02838–0291c` | 228 | VERIFIED, all 48 pointer fixups resolve into CONST |
| `_BSS` | Several ranges | 341,577 | MISMATCH for single-base layout |

The 39 VOC pointers, nine device-name pointers, and nine device IDs reproduce the complete initialized table. The CON parser independently indexes the VOC table at `1:2f0fd`. Startup indexes the device map. The device-name table tracks nine consecutive relocations between those publics.

The pinned compiler reproduced historical nonzero padding in the filename and device pools from the ordinary literal sequence alone. For example, `bodybloa.voc` and `bodyblor.voc` had trailing `hoo` and `unn` after the first NUL, matching the oracle. The bytes are consistent with stale string-scanner storage, as described for later Watcom in `docs/PRIOR_ART.md`. A control build with plain literals had a 687-byte CONST contribution, four differing bytes at `2:00927,00943,00952,00953` and six uncovered zero bytes at `2:0075d–00760,008b9–008bc`. Explicit trailing NULs in six literals cover those holes, zero the four mismatches and include the final zero at `2:00957`. Every nonzero padding byte still comes from the compiler.

The candidate defines the file table, MIDI storage, cache arena, voice array and cache bookkeeping as ordinary uninitialized globals. The oracle places those at `2:356d8–35810`, `35810–3f450`, `3f450–88830`, `a5420–a5705` and `a57d0–a59dc`, respectively. The 40,000-byte MIDI capacity is inferred from the next allocation. The file-table stride and record count independently place the MIDI region's start. The arena's capacity and the independent `ps` base constrain its start. Watcom emits all candidate globals into one `_BSS` contribution, beginning with the arena. The oracle divides the ranges between other game globals. The original common-data allocation and link order remain open.

## Compiler findings and bindings

- Separating the cache stack store from the depth increment matches the allocator. The postfix subscript form changes the save set and the extent.
- `loadsounds` passes `S_IREAD` as the variadic mode with `O_RDWR | O_BINARY`. Watcom's `S_IWRITE` is 0x80 and produces two byte differences against the oracle's 0x100.
- The released Build `PRAGMAS.H` preserves Ken's original inline `klabs` intrinsic. Its branch-and-negate sequence is present twice in each of the three prototype positioning routines and in the Build angle routine. The candidate retains that existing assembly intrinsic. Watcom's ordinary `labs` generates a sign-mask sequence in the initial probes. The June 1995 `parm [eax]` variant and the later `parm nomemory [eax]` variant both preserve the recorded matches.
- `xyzsound` matches using the coordinate differences in both expressions and repeating the distance shift at comparison and call. `pan3dsound` matches when the sprite coordinates are loaded into locals and the two coordinate differences are written directly in the angle and distance expressions.
- In `stopsound`, using an unsigned-long index for the voice-count access keeps the short index live across the one-argument ASS call. The plain short index keeps the scaled index instead and emits a 64-byte function. Adding a second ASS argument also fails.
- `getangle` is independently identifiable by the axis and diagonal cases and the ratio table from the March 1995 Build engine. The sprite stride and base are independently established by the map-v5 loader. The audio toggle, camera and player bindings have separate GAME uses.
- No exact public match for the four ASS entries in the tested 1.04, ROTT beta and ROTT demo library scans. Their bindings use the oracle callees and the released ASS source. The 3D VOC dispatcher at `1:5e660` forwards the callback and priority to the known AdLib and PC speaker play backends. `1:5e788` pans a handle and applies attenuation through `ADLIBFX_SetVolume`. `1:5e898` calls identified `ADLIBFX_Stop` and `PCFX_Stop` with a handle. `1:58e94` stops the preceding MIDI sequence through the same backend used by the music stop entry, then forwards the song buffer and loop flag to the MIDI parser. Watcom I/O bindings resolve through the existing runtime census.

The final results produce 3 function and 10 data binding proposals through `tools/bindprop.py`, with zero conflicts. The overlay records those votes alongside the independent evidence.

## Open issues

- `spritesound` is the sole NEAR_MATCH. It has the correct 464-byte extent, with 149 reported byte differences and the first at `+0xa`. The first difference is an early store of the sound number from EAX, while the oracle delays the spill until after loading the sprite coordinates and uses EBX. The distance shift and the emitter write also differ in register allocation and scheduling. The candidate uses explicit signed short copies of the register arguments to reproduce their low-word uses. Plain short formals score worse in this source shape.
- The pinned compiler did not inline a plain `xyzsound` wrapper. It compiled to 48 bytes including a call to the public. Other probes varied coordinate staging, index conversions, local types, control flow and voice-record update expressions. These left instruction or fixup-site differences.
- The compiler's BSS public offsets imply multiple target bases. The source does not claim a single linked BSS match.
