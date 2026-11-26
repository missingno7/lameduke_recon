# LameDuke’s first Watcom 9.5 link experiment

**Partial.** All 34 registered units built fresh, including A.ASM with MASM 5.10. Watcom 9.5 GA rejected the ordinary link with 152 undefined symbols. Its `OPTION UNDEFSOK` produced an unmodified 637,549-byte DOS/4GW LE image with those unresolved references for static comparison. The oracle is 685,121 bytes. The diagnostic image is unsuitable for execution or matching claims.

## Reproduce

Run the following from the repository root.

```powershell
python -B build/workers/link/tools_proposed/link.py
```

The script exits 0 for a resolved link, 2 when it obtains a diagnostic image, and 1 when neither link works. `--link-only` reuses the previous objects. [game.lnk](tools_proposed/game.lnk) contains the exact ordinary directives. [diag.lnk](tools_proposed/diag.lnk) includes them and adds `OPTION UNDEFSOK`.

The tables below use the stable fresh build at `26c102032c83f6076d9164661a2d2ad50249f19e`. A second link of the same objects reproduced the candidate SHA-256 `92f3f0855d809a635121b2f7009f25342ace930e48d619f314cf2d6e5ae8249e`. [Ordinary log](baseline_26c102/strict.log), [diagnostic log](baseline_26c102/diagnostic.log), [map](baseline_26c102/diagnostic.map), [comparison JSON](baseline_26c102/comparison.json), [input hashes](baseline_26c102/inputs.json).

A second fresh build picked up the concurrent ENGINE revision at `baceb84`. It reported the same 152 undefined symbols and file size. At fixed offsets, three decimal places of obj1 agreement moved from 4.769% to 4.766%. [Latest comparison JSON](run/comparison.json), [latest input hashes](run/inputs.json).

The 9.5 GA C units used the canonical DOS compiler through `tools/wcc.py`. The Apogee C units used their registered Watcom 10.0 GA profiles through the same wrapper. The guest ran `C:\BIN\WLINK.EXE @D:\game.lnk` under pinned DOSBox-X. The 9.5 GA install was mounted read-only at `C:`, with `WATCOM=C:` as a drive designator and `C:\BIN;C:\BINB` on the guest PATH. `SYSTEM DOS4G` used the installed `BINB/WLSYSTEM.LNK` and `WSTUB.EXE`.

The directive enables `OPTION DOSSEG` and `NODEFAULTLIBS` and lists the source objects and each stock member explicitly. The script copies 49 Creative/Gravis modules and 141 Watcom modules verbatim from the pinned archives, including the data-only members, from THEADR through MODEND. The runtime order uses the 125 code contributions from the census and places the data-only inputs by their observed DGROUP order and required publics. `heapmod`, `___argc`, `minreal` and `dosseg` complete stock runtime references. Their exact historical placement remains open. The directive resolves the two names for the ripple renderer to one function and uses the stock `atoi` body for both equivalent `atoi` and `atol` entries at oracle `1:634da`. The oracle MPU-401 call at `1:5b4bd` also targets that entry.

## Container and image

| Measurement | Oracle | Diagnostic |
|---|---:|---:|
| File length | 685,121 | 637,549 |
| LE header | `0x28b8` | `0x28b8` |
| Entry | `1:62e00` | `1:62470` |
| Initial ESP | `2:1c99d0` | `2:132500` |
| Obj1 base / flags | `0x10000 / 0x2005` | `0x10000 / 0x2005` |
| Obj2 base / flags | `0x80000 / 0x2003` | `0x80000 / 0x2003` |
| Obj1 virtual size | `0x6c445` | `0x6bab5` |
| Obj2 virtual size | `0x1c99d0` | `0x132500` |
| Obj1 page range | 1–109 | 1–108 |
| Obj2 page range | 110–120 | 109–119 |
| Last physical page bytes | `0xc41` | `0x06d` |
| Fixup-section size field | `0x2cb7d` | `0x22dfa` |
| File offset of image pages | `0x2f800` | `0x25a00` |

The entire `0x28b8`-byte prefix agrees, including the 96-byte MZ header, stub body, six relocation entries and six bytes of padding. The first file difference is the LE page count at file offset `0x28cc`. Resident name `game`, module flags `0x200`, 386 CPU, OS field, `0x1000` page size and automatic data object 2 agree. `LOADONCALL` gives the exact object flags. A control with default segment flags gave `0x2045/0x2043`, and `NONSHARED` left that pair unchanged. The old standalone `DOSSEG` spelling gave Error 3033 in this build. `OPTION DOSSEG` was accepted.

Byte agreement uses the larger span as denominator, with missing bytes counted as differences. All bytes are counted, including linked operands. Fixed offsets compare the same object offsets or file offsets. Region-start comparisons independently anchor each pair at its own start.

| Span | Equal bytes / denominator | Agreement |
|---|---:|---:|
| Entire EXE at fixed file offsets | 39,616 / 685,121 | 5.78% |
| Obj1 virtual code at fixed offsets | 21,148 / 443,461 | 4.77% |
| Obj2 virtual at fixed offsets | 1,220,189 / 1,874,384 | 65.10% |
| Obj2 actually file-backed at fixed offsets | 8,121 / 44,097 | 18.42% |
| MZ prefix | 10,424 / 10,424 | 100% |
| LE header and loader region | 691 / 733 | 94.27% |
| Fixup section at its start | 7,591 / 183,164 | 4.14% |
| Gap after fixup records at its start | 118 / 239 | 49.37% |
| Physical pages at their start | 25,856 / 490,561 | 5.27% |

Obj2's virtual percentage is dominated by zero-fill. Its eleven logical pages cover up to `0xb000`, but their physical bytes end at `2:ac41` in the oracle and `2:a06d` in the diagnostic. Obj1 contains 3,003 oracle padding bytes past virtual size in its last page, versus 1,355 candidate bytes.

## Code placement and byte attribution

The map places `BEGTEXT`, then `_TEXT`, then A.ASM's separate `CODE` segment in class `CODE`, all in obj1. GAME starts at the correct address. Its first linked difference is at `1:0002e`, in the rel32 operand of the call at `1:0002d` from `inittimer` to `outp` (`1:62d8d` versus `1:623fd`). SECTOR is the first shifted unit. The candidate code object is shorter by `0x990`, which is also the offset delta of the runtime and A.ASM.

| Contribution | Oracle start | Diagnostic start | Fixed | At region starts |
|---|---:|---:|---:|---:|
| BEGTEXT | `0` | `0` | 100% | 100% |
| GAME | `10` | `10` | 6.67% | 6.67% |
| SECTOR | `14fb0` | `14fa0` | 6.21% | 11.04% |
| PLAYER | `1f280` | `1ef90` | 6.51% | 10.33% |
| ACTORS | `25160` | `24ea0` | 4.78% | 20.63% |
| SOUNDS | `2ba30` | `2b770` | 6.10% | 73.40% |
| MENUES | `2c1c0` | `2bf00` | 5.36% | 25.57% |
| GAMEDEF | `2e700` | `2e400` | 4.29% | 13.08% |
| ENGINE | `33e5c` | `3387f` | 3.17% | 50.32% |
| MULTI | `47abf` | `47219` | 2.45% | 8.00% |
| Creative/Gravis | `49c84` | `4930c` | 3.41% | 36.92% |
| Apogee C | `55b30` | `551b0` | 5.63% | 48.96% |
| Watcom runtime | `62d8d` | `623fd` | 2.38% | 95.88% |
| A.ASM | `6b685` | `6acf5` | 1.45% | 84.86% |

At each individual member start, the 34 vendor code contributions have 44,694 / 48,582 equal bytes (92.00%). The vendor block's lower aggregate score includes the eight-byte change in Creative-to-Gravis alignment. The 122 Watcom members with a unique code public in the map have 33,539 / 34,983 equal bytes (95.87%). The three remaining code contributions are `crwdata`, `noefgfmt` and `save8087`, which lack a map code public. All 533 A.ASM byte differences at its linked start fall within OMF fixup fields. Its 247 absolute LE sites agree at the same relative offsets.

| Oracle obj1 byte class | Equal at fixed offsets | Agreement | At function starts |
|---|---:|---:|---:|
| `STRICT` | 4,974 / 101,703 | 4.89% | 86,497 / 101,703 (85.05%) |
| `CODE_BYTES_MATCH` | 373 / 6,501 | 5.74% | 5,399 / 6,501 (83.05%) |
| `NEAR_MATCH` | 13,113 / 249,881 | 5.25% | 45,907 / 250,400 (18.33%) |
| Independently identified code padding | 25 / 102 | 24.51% | |
| Independently identified embedded data | 27 / 147 | 18.37% | |
| Other code or embedded data | 2,636 / 85,127 | 3.10% | |

The unmodified object verifier classified 516 `STRICT`, 32 `CODE_BYTES_MATCH` and 117 `NEAR_MATCH` functions in the fresh registered objects. The `NEAR_MATCH` oracle intervals use the next supported start from the function census and symbol bindings, so their ends are estimates. At function starts, 63 of the 516 verified `STRICT` functions have every linked byte equal. The object-level verifier status and the unadjusted linked byte agreement measure different things.

## DGROUP, stack and fixup order

`_NULL` at `2:00000` agrees. The first obj2 difference is `2:00004`, the start of CONST. The oracle starts with byte `de` in the GAME constant pool, while the diagnostic starts with `4d` from a SECTOR message. The primary code order can therefore agree even as the first DGROUP data contribution changes.

| Data region | Oracle start | Diagnostic start | At region starts |
|---|---:|---:|---:|
| `_NULL` | `00000` | `00000` | 100% |
| `CONST` | `00004` | `00004` | 3.27% |
| `CONST2` | `02164` | `01b20` | 97.12% |
| `_DATA` | `02304` | `01cc0` | 18.53% |
| `_GF1INIT` | `0abdc` | `0a008` | 66.67% |
| `XI` | `0ac0c` | `0a038` | 66.67% |
| `YI` | `0ac36` | `0a062` | 66.67% |
| Padding before `_BSS` | `0ac42` | `0a06e` | 14.29% |
| `_BSS` | `0ac50` | `0a070` | 66.23% |
| Stack start, assuming 4 KiB in the oracle | `1c89d0` | `131500` | 100% |

The candidate uses the stock 4 KiB stack from `cstart` and no stack-size override. Its ESP is `0x974d0` lower than the oracle. The 195 bound ENGINE BSS publics all shift by `-0x974c4`, including `_wall` from `2:176310` to `2:dee4c`. That uniform shift locates most missing storage ahead of the ENGINE contribution. MULTI's split BSS shows several different deltas. `_crctable` moves by `-0x97bc4` and `_connectpoint2` by `-0x96904`. Its initialized `_myconnectindex` lands at `2:22a8` instead of `2:2e7a`.

The oracle has 21,046 absolute records and the diagnostic has 16,615, including duplicate records for page crossings. Obj1 has 20,857 versus 16,426. Obj2 has 189 in both, at entirely displaced source sites. All 108 overlapping obj1 page-map entries agree, while obj2 starts one physical page earlier. No common obj1 page has an identical raw fixup list. The first record on page 1 starts at `1:00e04` in the oracle and `1:00a47` in the candidate. In the candidate, `1:00a47` and `1:00a3e` are descending sites in one GAME LEDATA packet. The next packet starts its run at `1:00b8f`. All 2,094 adjacent same-page, same-packet GAME pairs descend. That packet pattern makes differences in code length, absolute sites and packet boundaries relevant before testing historical fixup ordering. The candidate's GF1 and CRT initializer lists preserve all 17 six-byte entry sites in table order after accounting for their start addresses. Their target values move with code.

## Priorities

1. **Supply the missing definitions from reconstructed source.** The ordinary diagnostics list 149 game data names, two MULTI template names and one missing ENGINE routine, `printext16_` at oracle `1:450e0`. Restore the GAME/GAMEDEF globals and initializers in their evidence-backed owners. Among the blockers are `ps`, `hittype`, the demo streams, the sound cache and actor scripts. These also address the large BSS shortfall and missing relocation sites. The ordinary linker log records each reference by input object.
2. **Recover MULTI's data packaging.** The separate `realmode.asm` assembles and resolves `_rmbuffer` and `_ipxcallback` in a control, but introduces a third LE object of `0x191` bytes and moves DGROUP to obj3 at base `0x90000`. Use the documented `_rmbuffer` placement inside MULTI initialized data and the original inline IPX callback construction to guide this work. The baseline includes the registered `multi_data.c` and omits the unregistered standalone assembly template.
3. **Close the code contribution lengths in address order.** The first shift is SECTOR at `1:14fa0` versus `1:14fb0`. SECTOR, GAMEDEF and ENGINE have the largest net region deficits, at 736, 733 and 713 bytes respectively. Restoring lengths and internal function placement reduces displacement changes across the entire vendor and CRT tail.
4. **Recheck initialized ownership, BSS order and the fixup stream once the link resolves.** The `CONST` deficit moves `CONST2` and `_DATA` down by `0x644`. The remaining initialized-data deficits move the GF1 and CRT lists down by `0xbd4`. Check data-only runtime placement, the stack boundary, and the exact page and LEDATA grouping after these shifts and unresolved sites are accounted for. The observed MZ prefix and object flags already reproduce under the 9.5 DOS linker.
