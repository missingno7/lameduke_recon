# A.ASM restoration

The worker copy combines the December 1994 reconstruction in `src/engine/A.ASM` with names and comments from Ken Silverman's April 1994 `BUIL0422/A.ASM` and May 1995 `buil1995/A.ASM`. Both donor files are under `external/x/kenbuild-20241213/kenbuild-related-archives-20241213/`. The names are supported by corresponding donor code. Their December spellings remain open.

## Source correspondence

- **April.** Restored the `hxsiz*` and `hysiz*` numbering by pixel position within the one-, two- and four-pixel groups. The chained version retains April's numbering even though December draws the partial groups before the four-pixel loop. Restored the `mach1*` and `mach2*` coordinate increment labels, `skipthe*`, the VGA plane loop labels, and the order of `PUBLIC hchainasm64_` followed by `startx`, `starty`, `startp`, `xc4`, `yc4` and `plane`.
- **May.** Restored `hoffs*` and `pal*` for the opaque texture and palette loads. The vertical routines use Ken's `machv*`, `machmv*` and `fixchain*` patch labels. The four-column masked dispatch has all sixteen `mvcase*` labels, with the December NOP runs and jump widths. The masked and translucent spans use Ken's branch and operand labels, register notes and inline comments. Where a branch lands on a patchable instruction, the code uses the historical pair of branch and patch labels at the same address, for example `mbeghline` and `mmach3d`.
- **December-specific code.** The chained texture and palette operands, the chained loop's extra patched increments and two partial-group targets, the stride-update exit, and the sprite loop entry and carry mask keep their generated names where the donors give no corresponding label. The starting plane lives in the immediate at `hchainskip5+1`. April has a separate `startplane` byte. The `fixchain1a` negative stride lives in December's store displacement. May patches an initial subtraction. December's sprite `smach2` adds the signed integer part of the X increment and the carry. The base Y step is in the later LEA, so the May comment was shortened to `;xincshr16`.

## Verification

Ran the pinned MASM 5.10 under MS-DOS Player via `tools/masm.py` for each input in sequence. A temporary Python startup hook redirected the wrapper's scratch directory to `build/workers/asmhuman/tmp` and Python bytecode writes were disabled to keep the run inside the worker directory.

```powershell
python tools/masm.py src/engine/A.ASM build/workers/asmhuman/BASE.OBJ
python tools/masm.py build/workers/asmhuman/A.ASM build/workers/asmhuman/A.OBJ
python tools/match.py --obj build/workers/asmhuman/A.OBJ
python tools/objcmp.py build/workers/asmhuman/BASE.OBJ build/workers/asmhuman/A.OBJ --funcs
```

- **PROVEN.** MASM reported zero warnings and zero severe errors for both objects. The executable verifier reported `STRICT 20/20 functions, 3520 bytes` using the existing bindings.
- **PROVEN.** `objcmp.py` reported equal CODE bytes and all 288 fixups, with `first-byte-diff None`. A direct byte comparison also found all 6,439 bytes of the two OMF files identical. Their SHA-256 is `362da7e4faadfb6f4e920e06dbdd24dadc65cacf23a4690e148079fa3061be7f`.
