# Watcom 9.5 GA natural-link layout

The fresh 35-object build used the registered profiles and pinned compiler wrappers. GAME and the other 9.5 GA C units used the DOS host. The input manifest records a stable source snapshot. The best diagnostic link uses the existing `FILE` order with GLOBAL after ACTORS, explicit `OPTION STACK=4K`, and the data-ownership worker's literal spellings with `#pragma off(reuse_duplicate_strings)` immediately before `prelevel`. The control inserts file-scope definitions and a pragma in a worker-local copy. The original function bodies are byte-for-byte unchanged in the source.

## 1. Contribution alignment

**PROVEN.** In Watcom 9.5 GA, the ordinary `_BSS` uses OMF alignment code 5 (four bytes) for a character array. Under `-4r -s -otexan`, adding an uninitialized `double` or six-byte far function pointer changes `_BSS` to OMF code 3 (16 bytes). Adding `-os` returns the same ordinary `_BSS` to code 5. An initialized `double` changes `_DATA` to code 3 while `_BSS` remains code 5. Merely declaring `extern double` leaves both at 5. `CONST` remains code 5 across these controls. WLINK follows the alignment on each contribution. The `para` control puts a paragraph-aligned `_BSS` contribution at `2:70` after a code-5 piece ending at `2:67`, then puts the next code-5 piece at `2:80` after the paragraph-aligned piece ends at `2:7f`. [Compiler probes](probes/types.txt) · [WLINK probes](probes/links.txt)

| Contribution | OMF alignment | Size | Oracle base | Scoped link base | Delta |
|---|---:|---:|---:|---:|---:|
| GAME | 16 | `7` | `0ac50` | `0ac50` | `0` |
| Second GAME-range unit (`mspos`, currently in SECTOR) | 4 | `4` | `0ac60` | `0ac58` | `-8` |
| GLOBAL | 4 | `9ab53` | `0ac70` | `0ac5c` | `-0x14` |
| SOUNDS | 4 | `20c` | `a57d0` | `a57b0` | `-0x20` |
| MENUES | 4 | `c40c` | `a59e0` | `a59bc` | `-0x24` |
| GAMEDEF | 4 | `f` | `b1df0` | `b1dc8` | `-0x28` |
| ENGINE | 4 | `10c0d1` | `b1e00` | `b1dd8` | `-0x28` |

Each base is derived from the map public minus its OMF offset. The independent oracle bindings give one common base within each of these contributions, including 87 GLOBAL and 195 ENGINE publics. The alignment result applies to the six listed post-GAME contributions through ENGINE. Later oracle contributions include dword-aligned starts, for example the stock vendor `globals` BSS at `2:1c13e4` and CRT errno slots at `2:1c8928`.

**OPEN.** The real GAME already has paragraph `_BSS`, consistent with its far timer pointer. GLOBAL, SOUNDS and ENGINE stayed dword-aligned with `-os`, `-mm` and `-zm`; `-zp16` was rejected. The GAME `-mm` and `-zm` controls retained its paragraph alignment while changing its code model or packaging. Nearby historical ENGINE/MULTI objects, including BUIL0422 ENGINE, use dword `_BSS`. The same small type control yielded paragraph `_BSS` on the patched 9.5 NT compilers and 10.0 GA as well, and 10.0 GA WLINK gave the same contribution addresses when linking the scoped objects. The alignment demand in the six historical contributions therefore remains open. [Real-unit probes](probes/real.txt) · [Small-object probes](probes/compile.txt) · [10.0 map](wc100/run/diagnostic.map)

**NEGATIVE.** `-zp1` and `-zp8` leave the ordinary segment alignment unchanged in the speed-optimized probe. `#pragma align 16` leaves the char-only contribution dword-aligned, `__declspec(align(16))` fails to parse, and zero-length `double` arrays and empty structs are rejected. Defining a `double` in an explicitly named `#pragma data_seg("_BSS", "BSS")` emitted a second code-5 `_BSS` piece. WLINK 9.5 rejects `SEGMENT _BSS ALIGN=16`, `OPTION ALIGNMENT=16` and `OPTION PACKDATA=16` for this DOS/4GW link. The documented newer `ORDER` directive is also rejected by 9.5. [Additional compiler probes](probes/followup.txt) · [WLINK probes](probes/links.txt) · [Watcom linker manual](https://downloads.openwatcom.org/ftp/archive/11.0c/docs/linkeruserguide.pdf)

## 2. CONST and initialized data order

**PROVEN.** Reversing `FILE` order reverses contributions to each of `CONST`, `_DATA` and `_BSS`. The two char-only modules use four-byte boundaries for both initialized segments, including the padding between different unaligned lengths. Changing explicit `OPTION DOSSEG` left the same-name order unchanged in the small no-CRT control. In the full scoped link the CRT inputs made the output byte-identical with and without explicit `DOSSEG`. Keep `FILE obj\game.obj` before the other game objects. The older baseline's `Maxed out on anim walls` is the compiled remnant of the second GAME-range pool, at oracle `2:00648`. In the fresh baseline GAME supplies that text at `2:00004` and SECTOR has a zero-length CONST. After restoring the missing initial GAME strings, `2:00004` begins with the oracle banner and the remaining initialized regions shift to their expected addresses. Both literal builds put the banner first. [Small maps](probes/links.txt) · [Baseline map](baseline/run/diagnostic.map) · [Scoped map](scoped/run/diagnostic.map)

The scoped link places the `CONST`, `CONST2`, `_DATA`, `_GF1INIT`, `XI`, `YI` and `_BSS` starts at `4`, `2164`, `2304`, `abdc`, `ac0c`, `ac36` and `ac50`, respectively. `_DATA` starts for GAME, GLOBAL, SOUNDS, MENUES, GAMEDEF and ENGINE also agree individually at `2304`, `23dc`, `2838`, `291c`, `2b10` and `2c00`. All offsets are in obj2.

## 3. Stack arithmetic

**PROVEN.** In the stock `cstart` `_TEXT` contribution, relocation sites `+172`, `+1ef` and `+20b` name `_end`; `+1f4` names `_edata`. The corresponding oracle LE sites target `2:1c89c4` and `2:0ac50`. With the measured `STACK` paragraph alignment, `align16(0x1c89c4) = 0x1c89d0` and `ESP - 0x1c89d0 = 0x1000`. Obj2 vsize equals ESP at `0x1c99d0`. The tool's stock `cstart` contributes a `0x1000`-byte `STACK` segment. A real relink with explicit `OPTION STACK=4K` is byte-identical to the baseline with the default. A small `OPTION STACK=8K` control moves ESP by precisely `0x1000` while leaving the bottom of `STACK` fixed. The historically used directive spelling is open. [WLINK probes](probes/links.txt) · [Explicit-stack comparison](stack/run/comparison.json)

The GLOBAL-inclusive baseline gap is `0x680`, of which `0x650` is before `_BSS`. The scoped map restores `_edata` and leaves `_end=2:1c8994`, `STACK=2:1c89a0` and ESP/vsize `0x1c99a0`, each `0x30` below the corresponding oracle boundary. The six early contribution starts account for 40 bytes through ENGINE. The remaining layout debt includes MULTI's split BSS, audio variable placement, and stock CRT tail ordering. The observed ESP difference originates before the stack.

## 4. Duplicate strings

**PROVEN.** On the DOS-hosted 9.5 GA compiler, `#pragma off(reuse_duplicate_strings)` changes three identical `"same"` references from a shared address to three separate addresses. Within a speed-optimized probe the offsets change from `0,0,0` to `0,0x14,0x1c`; the pool grows from `0x23` to `0x33`. This works with `-os` as well, where literals are packed. `-od`, `-os`, `-zc` and `-zm` alone retain pooling in the control. `-zc` moves the pool to `_TEXT`, and `-zm` separates functions without giving each its own CONST pool. An explicit `\0` also makes a distinct literal, but the oracle's four repeated banner/error-message copies have nonzero bytes immediately after their terminator. Independently named character arrays in `#pragma data_seg("CONST", "DATA")` retain separate addresses too, and this emits a second CONST contribution after that object's literal pool. [Object probes](probes/compile.txt) · [Pragma A/B](probes/followup.txt) · [Watcom C user guide](https://openwatcom.org/ftp/archive/11.0c/docs/cuserguide.pdf)

In the real-unit control, default pooling drops 36 bytes from the expected GAME-range pool length. Whole-GAME `off` adds 112 excess bytes. Scoping `off` at `prelevel` restores the extra `Episode`, `Level` and `Board not found.` copies and the full `0x6a4`-byte pool length. The full GAME pool still differs by 110 bytes, with first difference at `2:00294`. `encode_mapname` is encountered too early in the current source and displaces four surrounding literals. The tail pool at `2:00648` has six differences in padding. The duplicate-preserving pragma reproduces the observed separate references in the current combined TU. A historical split and a historical pragma remain distinct possibilities.

## Full link controls

| Run | Undefined | GAME CONST | Obj2 ESP / vsize | Obj2 file-backed agreement | Obj1 agreement | Whole EXE agreement |
|---|---:|---:|---:|---:|---:|---:|
| Fresh baseline / explicit stack only | 58 | `60` | `1c9350` | 15.03% | 6.55% | 9.58% |
| Literals, default pooling | 2 | `680` | `1c9970` | 37.22% | 6.63% | 6.20% |
| Literals, whole-GAME pooling off | 2 | `714` | `1c9a10` | 27.68% | 6.74% | 6.14% |
| Literals, pooling off from `prelevel` | 2 | `6a4` | `1c99a0` | **99.42%** | 6.68% | 6.24% |

The scoped candidate agrees in 43,842 of 44,097 file-backed obj2 bytes at fixed offsets, including linked operands. Of the 255 differences, 184 fall in CONST and 71 in `_DATA` or the initializer tables. The remaining undefined symbols are `_SoundToggle` and `_camerasprite`. The unmodified object verifier drops GAME from 29 STRICT functions (11,037 bytes) to 19 (8,317 bytes). Ten functions (2,720 bytes), including `enterlevel` and `check_file_id`, fall to `CODE_BYTES_MATCH` because the combined CONST fails its byte check. The 32 NEAR_MATCH functions have the same total candidate extent of 74,912 bytes. `_DATA` and `_BSS` remain verified. This is a diagnostic link, and the source variant would regress current matching evidence if integrated as-is. [Scoped comparison](scoped/run/comparison.json) · [Verifier](scoped/game-match.txt)

## Reproduce

From the repository root, run each command to completion before starting the next. The normal diagnostic link exits 2. Probe links without all CRT dependencies are for static inspection only.

```powershell
$w = 'build/workers/link2'
python -B "$w/proposed/link.py"
python -B "$w/prepare_game.py"
python -B "$w/proposed/link.py" --label scoped --reuse "$w/baseline/run" --game-source "$w/controls/scoped/game.c" --directives "$w/proposed/candidate.lnk"
python -B tools/match.py --obj "$w/scoped/run/d/obj/game.obj"
python -B "$w/probe.py"
python -B "$w/probe.py" --real
python -B "$w/probe.py" --followup
python -B "$w/probe.py" --types
python -B "$w/link_probes.py"
```
