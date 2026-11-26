# Watcom 9.5b data layout and SOUNDS follow-up

## _BSS rule

**PROVEN** for the pinned 32-bit Watcom 9.5b compiler and ordinary flat near objects. Layout is computed inside each OMF contribution.

1. Allocate a symbol slot on the first declaration of each ordinary identifier. A later definition reuses that slot. An unused extern, function declaration, or typedef still consumes a slot. Struct tags, member names, enum constants and preprocessor macros did not consume slots in the probes. Defined functions, formals, block declarations and compiler-generated symbols can consume slots too. A function with one formal and one automatic variable consumed three slots. An initialized pointer table containing two distinct strings consumed three slots, including the table itself.
2. A bare translation unit begins at effective slot 18. A symbol's page is its slot divided by 22, rounded down. Header declarations can move the first user page boundary. The starting phase modulo 22 is sufficient for a slice that includes all allocations for the segment.
3. Emit objects by total byte size in this bucket order. Multiples of 8 come first, followed by other multiples of 4, even sizes above 2, size 2, odd sizes above 1, and size 1. Within a bucket, visit later pages first. Within a page, visit lower hash buckets first. For a collision, emit the later slot first.
4. Hash the original case-sensitive identifier, excluding the OMF underscore. Start at zero and compute `h = ((h << 4) + byte) & 0xfff` for each ASCII byte. The bucket is `h % 211`. At PE RVA `2dc47`, the compiler rotates the masked high nibble in a 32-bit register before applying the 12-bit mask. This discards the high nibble. The caller at `2dbf3` takes the remainder modulo 211. The later Open Watcom hash explicitly folds high bits into the low bits. [Open Watcom cscan.c](https://raw.githubusercontent.com/open-watcom/open-watcom-v2/master/bld/cc/c/cscan.c)

The same size and symbol ordering applies with `-4r`, `-os` and `-ot`. `-4r` and `-os` pack the measured contributions. With `-ot`, round each start upward to the largest leaf scalar size in its type. This includes a six-byte alignment for a far pointer and an eight-byte alignment for a double. Struct packing controls member offsets and total size. The compiler still aligns a packed five-byte struct containing a long to four bytes. The predictor takes emitted alignment as input. WLINK placement, custom segments, far storage and common allocation remain outside the rule.

### Evidence and controls

- A fixture with 42 sizes from 1 to 131,072 bytes matched every offset. Arrays with equal total size and different element types shared a bucket. The experiment around 65,536 bytes rejected a separate large-array threshold. The 1,800-symbol fixtures matched every offset in both declaration orders, including across page 32.
- Case variants and collisions distinguish the hash and reverse insertion order. `abc` and `Abc` both hash to 24 and appear in reverse declaration order on the same page. The 10.x 25-slot pages, 241-bucket modulus and folded hash fail here.
- Adding an unused extern or typedef shifts the page cut. Repeating an extern leaves it in its original slot. Moving the first declaration of `v07` ahead of the synthetic eight-object group moves it across a page and changes emitted offsets. Explicit `= 0` allocates a named initializer in `_DATA`.
- Historical validation uses Watcom's `-p` output from the original C and headers. Prefix compiles with 22 marker symbols measure header phase and count compiler-added slots. A separate compile queries `sizeof` for every declared object. The predictor receives no public offsets from the historical OMF reader.

| BUIL0422 historical object | _BSS publics | _BSS extent | _DATA publics |
|---|---:|---:|---:|
| ENGINE.OBJ | 207/207 | 902,225 B | 9/9 |
| EDITART.OBJ | 64/64 | 1,034,409 B | 1/1 |
| WSAY.OBJ | 22/22 | 42,978 B | 2/2 |

## Initialized contributions

**PROVEN** for the measured ordinary segments and flags.

- `_DATA` puts named initialized objects in definition order. An earlier extern affects the BSS symbol stream but does not move the initializer. The same type-alignment rules apply. Character arrays retain byte alignment even when their total size is a multiple of four. File-scope named const initializers used the same ordering and speed alignment in the `_TEXT` prefix.
- The ordinary `CONST` string pool uses first encounter order across function bodies and file-scope initializers. It coalesces identical decoded byte strings including their terminating NUL. Equal prefixes, suffixes, and strings with different lengths after an embedded NUL remain distinct in the probes. `-ot` aligns each start to four bytes. The segment ends after the final literal's bytes. `-4r` and `-os` pack consecutive literals. `-zc` moves the pool into code and emits longer literals first, with reverse first encounter order among equal lengths. `CONST2` was empty in the fixtures.
- The offset predictor does not derive bytes in alignment gaps. `-ot` can emit nonzero padding in both initialized segments. For example, after `char a=1` before `int b=2`, a probe emitted `03 01 00` in the `_DATA` alignment gap. The SOUNDS candidate retains the explicit trailing NULs in six literals from the first pass.

## SOUNDS placement

The predictor reproduces all eight former candidate BSS offsets with the measured starting phase of 13. The 741-byte voice array is at `2:a5420`, before the 512-byte cache stack at `2:a57d0`. This contradicts the bucket order of a single ordinary contribution for any spellings, declaration order, or header phase. The positions also span unrelated menu and settings storage.

**STRONG** The cache bookkeeping is the SOUNDS contribution. Its four objects are adjacent and directly used by the allocator, its initializer, and hooks. It ends at `2:a59dc`, four bytes ahead of the next mapped region. The file table, MIDI buffer, arena, and voice table retain their independent LE bindings as shared external data in the candidate. Their exact declaring units or historical segment declarations remain open. The candidate relies on those definitions in a future link.

| Owned name | Size | Hash | Predicted offset | Oracle |
|---|---:|---:|---:|---|
| `cache_sizes` | 512 | 195 | 0 | `2:a57d0` |
| `cache_depth` | 4 | 61 | 512 | `2:a59d0` |
| `cache_remaining` | 4 | 71 | 516 | `2:a59d4` |
| `cache_cursor` | 4 | 126 | 520 | `2:a59d8` |

The predictor agrees with the three initialized table starts and all 52 literal starts. The unmodified matcher reports 524-byte `_BSS` layout VERIFIED, 688-byte CONST VERIFIED, and 228-byte `_DATA` VERIFIED with 48 relocations. All 12 previously STRICT functions remain STRICT for 1,467 bytes. No additional bindings are needed.

## spritesound

`spritesound` remains NEAR_MATCH. It spans 464 bytes at `1:2be70` with 149 differences and the first at `+0xa`. The best candidate still spills the sound number from EAX before fetching source coordinates. The oracle spills it from EBX after the fetch. The later distance accumulator and emitter store also differ.

- 710 local and parameter renaming variants and all 22 preceding symbol phases left the normalized instruction comparison score unchanged under `-4r -s -otexan`.
- 601 record update variants and 649 formal-width, index-lifetime, distance and voice-guard variants did not improve on the existing source. Pointer and byte-offset aliases, changing the emitter selection expression, and splitting the handle and count index all failed to remove allocation differences.
- A plain wrapper calling `xyzsound` compiles to 48 bytes. Raising the expansion threshold with `-oe=1000` expands it to a 448-byte candidate that differs at `+0x7`. `#pragma inline_depth(5)` and `#pragma inline(xyzsound)` left the plain wrapper unchanged in the probes.
- Qualifying the active count or sound toggle as volatile worsened the candidate. `-otexa` and `-otxan` retain the same sprite difference. Removing `a` via `-otexn` worsens it.

## Reproduction

```powershell
python -B build/workers/sounds/bssvalidate.py
python -B build/workers/sounds/match_local.py --profile game build/workers/sounds/src2/sounds.c "-i=build/workers/sounds/src2" --json build/workers/sounds/results2.json
```

Provide an ordered JSON array of `[name, size, alignment]` triples to `bssrule.py`, or call `predict` from Python. For BSS, supply alignment 1 for packed emission or the measured scalar alignment for `-ot`. Use size zero for a symbol slot without BSS storage. Include file-wide first declarations and unnamed slots in the stream, or supply an independently measured phase with `--first-slot` for a flattened slice. `--segment data --speed` expects named initialized definitions for one segment and enables `-ot` alignment.

The integration gate's nine verifier tests and full registered rebuild passed with outputs redirected to the worker directory. The worker's `src2` source was verified separately against the pinned LE. The canonical source and configuration were read only.
