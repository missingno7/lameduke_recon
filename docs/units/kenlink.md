# Ken link gaps

## ENGINE

Watcom 9.5 GA, the pinned DOS host, and the `engine` profile (`-4r -s -or`) verify `printext16` STRICT at `1:450e0` for 748 bytes. The function starts at ENGINE `_TEXT+11284`, matching the oracle's ENGINE-relative placement. The adjacent `printmessage16` and `getspritescreencoord` also pass. All 78 baseline STRICT functions retain their status. The final tally is 81 STRICT (38,423 bytes) and 22 NEAR_MATCH (42,607 candidate bytes). The new STRICT functions are `printext16`, `printext256`, and `makeradarang`.

The source follows the dated May and June 1995 printer implementation. December's emitted code establishes unsigned glyph indices and row bits, the `mul80` intrinsic, and existing VGA latch primitives. Accumulating the byte offset, row address, and VGA base in three statements and putting the shifted font row first in the right-byte mask expression reproduces the final register and stack choices. The two independently bound font fixups pass.

The candidate moves `raytrace`'s hit-index initialization into the loop initializer alongside its clip index. This preserves the behavior of the original candidate and gives Watcom a distinct frame from `printext16`. The oracle discards `0x34` bytes in `raytrace` and `0x30` in `printext16`, then uses the `wallscan` return tail for both. With baseline `raytrace`, the initial printer insertion caused Watcom to share its `0x30` cleanup with `raytrace` and redirected earlier functions to the selected return tail. That DOS backport was 751 bytes with 572 mismatches and lost ten STRICT functions. Using the selected `raytrace` source retained the 613-byte extent and reduced its differences from 297 to 158. The intermediate printer had 15 differences with both font fixups verified.

## MULTI

Three distinct preserved 1995 MULTI objects have a single C THEADR module with local `_rmbuffer` at `_DATA+8`, `_DATA` class DATA with USE32, and DGROUP membership. There is no second template contribution. The archives are `2DRW0628` (SHA-256 prefix `8927ba85623e9f73`), `buil0723` (`cd743139f362a0a0`), and the September object in `19951025` (`96638a2facf9428b`). The surviving Ken `SRC/MULTI.C:42` declares a static initialized `rmbuffer` and refers to `realcom.asm`. December's independently bound layout places the array ten bytes into MULTI's _DATA, before `myconnectindex`.

`multi/realcom.asm` supplies the instruction source. Pinned MASM 5.10 and WLINK 9.5 GA generate a temporary 384-byte COM image for the historical C initializer. All 384 bytes agree with the bound December data and there are zero LE fixups in the template. The December IPX callback is installed by the 17 C byte stores and two runtime SI immediates corroborated by `1:47d57–47e12`. This removes the provisional `_ipxcallback` reference. The existing split BSS definitions remain in `multi_data.c`, with `myconnectindex` moving to the same DATA contribution as the serial template.

The DOS `multi` profile (`-4r -s`) preserves 18 STRICT functions and 3,164 bytes. Seven functions remain NEAR_MATCH (5,374 candidate bytes), including `neton` at 1,078 bytes, 298 differences and first difference `+0x10b`. The stock verifier checks the full 396-byte `_DATA` contribution at oracle `2:02cf0`. The stock link order reports 149 remaining undefined symbols, down from 152. Its diagnostic image has two objects, DGROUP in obj2 at base `0x80000`, a correctly bound `allocbicom` fixup to local `_rmbuffer`, and all 396 data bytes agree at their contribution start.

The diagnostic link places `_rmbuffer` at `2:022a6` and `myconnectindex` at `2:02426`, each `0xa54` before its oracle address. Of that, `0x644` precedes `_DATA` and `0x410` comes from earlier `_DATA` contributions. The linked printer entry `1:44b03` has the same `-0x5dd` shift as the whole ENGINE contribution.

## Reproduce

Run from the repository root. The wrapper uses the stock compiler and matcher with scratch under this worker. No symbol overlay is required. The verifier exits 1 for these partially reconstructed units. The link command exits 2 for the diagnostic image and uses the checked snapshot of the other registered build objects in `build/link/run/d/obj`.

```powershell
$w = 'build/workers/kenlink'
Remove-Item Env:LAMEDUKE_HOST -ErrorAction SilentlyContinue
python -B "$w/link_probe.py"
python -B "$w/verify.py" "$w/engine/ENGINE.C" engine
python -B "$w/verify.py" "$w/multi/multi.c" multi --profile multi
python -B "$w/verify_link.py"
```
