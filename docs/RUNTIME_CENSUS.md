# Runtime, Creative AWE32, Build rasterizers, and DOS stub

Ranges are half-open. Addresses are LE object offsets unless marked as file offsets. Library matches are identity evidence. `STRICT` below is reserved for the assembled `A.ASM` and the repository verifier.

## A. Runtime census

**PROVEN.** `1:62b9c-6b685` contains 35,561 bytes. The first 497 bytes belong to the Apogee AdLib FX module. The Watcom runtime starts at `1:62d8d` and contains 125 code contributions covering 35,001 bytes plus 63 zero alignment bytes. The runtime has 702 absolute LE fixup sites. Every byte in the requested interval has a disposition below. `DOS/CLIB3R.LIB:cstart` also supplies the 16-byte `BEGTEXT` contribution at `1:00000-00010`.

The comparison used the pinned `assets/D3D.EXE`, `build/scan/clib95b.json` and the libraries under `C:/tools/watcom-9.5b/LIB386`. For additional placements, compare covered bytes outside OMF fixup fields, require equality of the LE absolute site sets, and resolve known targets from linked rel32 operands and LE records. The last byte test also checked that all nonmember ranges in the Watcom interval are zero and have no LE sites. Library relocations into game code or BSS were useful placement evidence. These checks are library identity evidence with named-target cross-checks where available.

### Three apparent mismatches

| Member | Finding |
|---|---|
| `sscanf` at `1:64680` | OMF target method 4 omits a record displacement. At `+0x37` the field itself is `0x1d`. The LE target is `1:6469d`. `libscan.classify` ignored the in-field addend for method 4. The two private absolute targets and the two rel32 calls to `__scnf_` and `vsscanf_` agree. |
| `initrtns` at `1:64788` | Its `80aa:80386` Easy OMF contribution is USE32. LOC=5 fields at `+5`, `+0xa`, `+0x48` and `+0x4d` occupy the dword operands of four `mov reg,imm32` instructions. The LE has off32 records at precisely those sites, targeting `2:ac0c`, `2:ac36`, `2:ac36` and `2:ac42`. These are the XIB, XIE, YIB and YIE boundaries. |
| `chk8087` at `1:65c49` | The method-4 field at `+0x22` contains addend `5` and the LE target is `1:65c4e`. The register callbacks and `__init_80x87` agree with their 9.5b placements. |

The Easy OMF width also explains missed `cstart`, `stk`, `int386xa` and other assembly hits. `cstart` starts at the LE entry point `1:62e00`. Reading the LOC=5 fields as dwords reduces its apparent code differences to zero and aligns the absolute sites. Its `BEGTEXT` reference selects `1:3` and its three rel32 calls go to `__InitRtns`, `__CMain` and `__FiniRtns`. This width is scoped to the inspected Easy OMF/USE32 input. The assembler calibration below uses MASM's LOC=9 encoding. Repository tools were unchanged.

**NEGATIVE.** Installed DOS `CLIB3R.LIB` comparisons from 10.0, 10.0a, 10.5, 10.5-pck, 10.6 and 11.0 give `sscanf` at 117 bytes versus 102, and `chk8087` at 132–142 bytes versus 123. The later archives tested either lack the member name `initrtns` or have the 169-byte 11.0 version. The installed 9.5b library accounts for all three original diagnostics. The local `Patch32_95b.zip` has A/B deltas and no `CLIB3R.LIB` replacement under that name. The census does not distinguish an unavailable earlier 9.5 build with identical members.

**NEGATIVE.** Whole-segment `libscan` screened all 37 local 9.5b `LIB386` LIB/OBJ inputs. Alternate CLIB, MATH and platform libraries contributed overlapping shared routines and no separate extent. `NOEMU387.LIB:386inite` has two return-only entries. The target registers the 10-byte `DOS/EMU387.LIB:386inite` instead, and its rel32 jumps select the matching `__sys_init_387_emulator` and `__sys_fini_387_emulator`. The large `emu387` contribution is bounded by three zero bytes before it and `save8087` immediately after it. The DOS alternate startup objects produced no needed extent. Stock OMF parsing failed on 11 archive inputs. Walking their archive page and MODEND boundaries recovered 60 of 62 modules in each of the eight PLIB/PLBX variants with zero runtime hits, 254 of 255 in DOS `GRAPH.LIB` with one shared `fdfs` hit, and 425 of 426 in each Windows CLIB with shared runtime hits. The 19 unsupported members remain untested by whole-member comparison.

**Aliases.** The `fprintf` hit at `1:646c4` is a varargs wrapper shape collision. Its rel32 call selects `vsscanf_`, making this the tail of `sscanf`. `itoa` and `ltoa` each have two shape hits, resolved by named incoming calls. `atoi` versus `atol` at `1:634da` and the identical MATH387R/MATH3R `i64tos` copies at `1:6b468` are indistinguishable by their contribution bytes and fixups. `overlay.json` omits the ambiguous `atoi`/`atol` names. The remaining library name bindings describe member publics, with aliases where the historical object provides them.

### Obj1 address census

| Range | Bytes | Contribution | Evidence |
|---|---:|---|---|
| `1:62b9c-62d8d` | 497 | Apogee `adlibfx.c` tail | Callback setter, timer setup and shutdown |
| `1:62d8d-62d9b` | 14 | `DOS/CLIB3R.LIB:outp` | Baseline `libscan` |
| `1:62d9b-62dd2` | 55 | `DOS/CLIB3R.LIB:d_getvec` | Baseline `libscan` |
| `1:62dd2-62dff` | 45 | `DOS/CLIB3R.LIB:d_setvec` | Baseline `libscan` |
| `1:62dff-62e00` | 1 | Alignment | Zero fill, no fixups |
| `1:62e00-63068` | 616 | `DOS/CLIB3R.LIB:cstart` | LE entry point, absolute sites and three rel32 calls |
| `1:63068-630cf` | 103 | `DOS/CLIB3R.LIB:puts` | Baseline `libscan` |
| `1:630cf-630f5` | 38 | `DOS/CLIB3R.LIB:exit` | Targeted 9.5b OMF comparison |
| `1:630f5-63184` | 143 | `DOS/CLIB3R.LIB:ltoa` | `ltoa_` call with operand at `1:66d8a` |
| `1:63184-633b3` | 559 | `DOS/CLIB3R.LIB:open` | Baseline `libscan` |
| `1:633b3-63483` | 208 | `DOS/CLIB3R.LIB:read` | Baseline `libscan` |
| `1:63483-634b6` | 51 | `DOS/CLIB3R.LIB:close` | Baseline `libscan` |
| `1:634b6-634da` | 36 | `DOS/CLIB3R.LIB:printf` | Baseline `libscan` |
| `1:634da-6352b` | 81 | `DOS/CLIB3R.LIB:atol / atoi` | `atoi` and `atol` have identical code and relocations |
| `1:6352b-63548` | 29 | `DOS/CLIB3R.LIB:unlink` | Baseline `libscan` |
| `1:63548-6371b` | 467 | `DOS/CLIB3R.LIB:write` | Baseline `libscan` |
| `1:6371b-63755` | 58 | `DOS/CLIB3R.LIB:rand` | Baseline `libscan` |
| `1:63755-63792` | 61 | `DOS/CLIB3R.LIB:filelen` | Baseline `libscan` |
| `1:63792-639ba` | 552 | `DOS/CLIB3R.LIB:fopen` | Baseline `libscan` |
| `1:639ba-63b3b` | 385 | `DOS/CLIB3R.LIB:fseek` | Baseline `libscan` |
| `1:63b3b-63b67` | 44 | `DOS/CLIB3R.LIB:ftell` | Baseline `libscan` |
| `1:63b67-63cc3` | 348 | `DOS/CLIB3R.LIB:fread` | Baseline `libscan` |
| `1:63cc3-63e03` | 320 | `DOS/CLIB3R.LIB:fclose` | Baseline `libscan` |
| `1:63e03-63e10` | 13 | Alignment | Zero fill, no fixups |
| `1:63e10-63eb1` | 161 | `DOS/CLIB3R.LIB:_strcmp` | Baseline `libscan` |
| `1:63eb1-63ec0` | 15 | Alignment | Zero fill, no fixups |
| `1:63ec0-63f63` | 163 | `DOS/CLIB3R.LIB:__stos` | Baseline `libscan` |
| `1:63f63-63f7e` | 27 | `DOS/CLIB3R.LIB:getch` | Baseline `libscan` |
| `1:63f7e-63fe9` | 107 | `DOS/CLIB3R.LIB:nmalloc` | Baseline `libscan` |
| `1:63fe9-63feb` | 2 | `DOS/CLIB3R.LIB:disable` | Targeted 9.5b OMF comparison |
| `1:63feb-63fed` | 2 | `DOS/CLIB3R.LIB:enable` | Targeted 9.5b OMF comparison |
| `1:63fed-64011` | 36 | `DOS/CLIB3R.LIB:nfree` | Baseline `libscan` |
| `1:64011-64043` | 50 | `DOS/CLIB3R.LIB:lseek` | Baseline `libscan` |
| `1:64043-64064` | 33 | `DOS/CLIB3R.LIB:strcpy` | Baseline `libscan` |
| `1:64064-6407d` | 25 | `DOS/CLIB3R.LIB:strlen` | Baseline `libscan` |
| `1:6407d-64080` | 3 | Alignment | Zero fill, no fixups |
| `1:64080-64098` | 24 | `DOS/CLIB3R.LIB:memset` | rel32 selects `__STOSB` at `1:63ec0` |
| `1:64098-640b5` | 29 | `DOS/CLIB3R.LIB:int386x` | Baseline `libscan` |
| `1:640b5-640bd` | 8 | `DOS/CLIB3R.LIB:inp` | Baseline `libscan` |
| `1:640bd-640e2` | 37 | `DOS/CLIB3R.LIB:memcpy` | Baseline `libscan` |
| `1:640e2-64100` | 30 | `DOS/CLIB3R.LIB:int386` | Baseline `libscan` |
| `1:64100-64131` | 49 | `DOS/CLIB3R.LIB:dosio` | Baseline `libscan` |
| `1:64131-64134` | 3 | Alignment | Zero fill, no fixups |
| `1:64134-64152` | 30 | `DOS/CLIB3R.LIB:dosfile` | Baseline `libscan` |
| `1:64152-6415b` | 9 | `DOS/CLIB3R.LIB:inpw` | Baseline `libscan` |
| `1:6415b-64166` | 11 | `DOS/CLIB3R.LIB:outpw` | Baseline `libscan` |
| `1:64166-6418b` | 37 | `DOS/CLIB3R.LIB:strncmp` | Baseline `libscan` |
| `1:6418b-641dc` | 81 | `DOS/CLIB3R.LIB:getenv` | Baseline `libscan` |
| `1:641dc-6436c` | 400 | `DOS/CLIB3R.LIB:strtol` | Baseline `libscan` |
| `1:6436c-643a3` | 55 | `DOS/CLIB3R.LIB:stricmp` | Baseline `libscan` |
| `1:643a3-643a4` | 1 | Alignment | Zero fill, no fixups |
| `1:643a4-643bd` | 25 | `DOS/CLIB3R.LIB:chainint` | Baseline `libscan` |
| `1:643bd-6441b` | 94 | `DOS/CLIB3R.LIB:atexit` | Baseline `libscan` |
| `1:6441b-6441c` | 1 | Alignment | Zero fill, no fixups |
| `1:6441c-64465` | 73 | `DOS/CLIB3R.LIB:dosstime` | Baseline `libscan` |
| `1:64465-64468` | 3 | Alignment | Zero fill, no fixups |
| `1:64468-645c0` | 344 | `MATH387R.LIB:pow387` | Baseline `libscan` |
| `1:645c0-645df` | 31 | `MATH387R.LIB:fchop` | Baseline `libscan` |
| `1:645df-645e0` | 1 | Alignment | Zero fill, no fixups |
| `1:645e0-645ea` | 10 | `DOS/EMU387.LIB:386inite` | XI/YI entries and jumps to DOS emulator hooks |
| `1:645ea-64601` | 23 | `DOS/CLIB3R.LIB:strerror` | Baseline `libscan` |
| `1:64601-64680` | 127 | `DOS/CLIB3R.LIB:fgets` | Baseline `libscan` |
| `1:64680-646e6` | 102 | `DOS/CLIB3R.LIB:sscanf` | `sscanf` field addend +0x1d at +0x37 |
| `1:646e6-646e8` | 2 | Alignment | Zero fill, no fixups |
| `1:646e8-6472e` | 70 | `DOS/CLIB3R.LIB:stk` | Targeted 9.5b OMF comparison |
| `1:6472e-6473c` | 14 | `DOS/CLIB3R.LIB:toupper` | Baseline `libscan` |
| `1:6473c-6473d` | 1 | `DOS/CLIB3R.LIB:crwdata` | `__FPE_handler` at `2:a804` points to this `ret` |
| `1:6473d-64787` | 74 | `DOS/CLIB3R.LIB:cmain386` | Baseline `libscan` |
| `1:64787-64788` | 1 | Alignment | Zero fill, no fixups |
| `1:64788-6480e` | 134 | `DOS/CLIB3R.LIB:initrtns` | Four LOC=5 off32 sites target XIB, XIE, YIB and YIE |
| `1:6480e-64871` | 99 | `DOS/CLIB3R.LIB:fputs` | Baseline `libscan` |
| `1:64871-6490a` | 153 | `DOS/CLIB3R.LIB:fputc` | Baseline `libscan` |
| `1:6490a-64927` | 29 | `DOS/CLIB3R.LIB:isatty` | Baseline `libscan` |
| `1:64927-64933` | 12 | `DOS/CLIB3R.LIB:errno` | Pointer getters target two consecutive errno slots |
| `1:64933-649bb` | 136 | `DOS/CLIB3R.LIB:dosret` | Baseline `libscan` |
| `1:649bb-64a17` | 92 | `DOS/CLIB3R.LIB:iomode` | Baseline `libscan` |
| `1:64a17-64a9b` | 132 | `DOS/CLIB3R.LIB:fprtf` | Baseline `libscan` |
| `1:64a9b-64aa4` | 9 | `DOS/CLIB3R.LIB:stack386` | Targeted 9.5b OMF comparison |
| `1:64aa4-64c24` | 384 | `DOS/CLIB3R.LIB:initargv` | Baseline `libscan` |
| `1:64c24-64c32` | 14 | `DOS/CLIB3R.LIB:tolower` | Baseline `libscan` |
| `1:64c32-64d12` | 224 | `DOS/CLIB3R.LIB:allocfp` | Baseline `libscan` |
| `1:64d12-64d37` | 37 | `DOS/CLIB3R.LIB:chktty` | Baseline `libscan` |
| `1:64d37-64dd1` | 154 | `DOS/CLIB3R.LIB:flush` | Baseline `libscan` |
| `1:64dd1-64e3c` | 107 | `DOS/CLIB3R.LIB:ioalloc` | Baseline `libscan` |
| `1:64e3c-64f6b` | 303 | `DOS/CLIB3R.LIB:fgetc` | Baseline `libscan` |
| `1:64f6b-64fad` | 66 | `DOS/CLIB3R.LIB:tell` | Baseline `libscan` |
| `1:64fad-64fcb` | 30 | `DOS/CLIB3R.LIB:qread` | Baseline `libscan` |
| `1:64fcb-64fd1` | 6 | `DOS/CLIB3R.LIB:getpid` | Operand selects `__psp`. `execaddr` has the same shape |
| `1:64fd1-64fd6` | 5 | `DOS/CLIB3R.LIB:remove` | Tail jump selects `unlink_` at `1:6352b` |
| `1:64fd6-64fd8` | 2 | Alignment | Zero fill, no fixups |
| `1:64fd8-65193` | 443 | `DOS/CLIB3R.LIB:memalloc` | Baseline `libscan` |
| `1:65193-65444` | 689 | `DOS/CLIB3R.LIB:grownear` | Baseline `libscan` |
| `1:65444-65447` | 3 | `DOS/CLIB3R.LIB:nmemneed` | Called from `nmalloc` at `1:63fc5` |
| `1:65447-65448` | 1 | Alignment | Zero fill, no fixups |
| `1:65448-65820` | 984 | `DOS/CLIB3R.LIB:int386xa` | Targeted 9.5b OMF comparison |
| `1:65820-65848` | 40 | `DOS/CLIB3R.LIB:segread` | Baseline `libscan` |
| `1:65848-6585b` | 19 | `DOS/CLIB3R.LIB:doserror` | DOS I/O calls it. The rel32 selects `_set_errno_` |
| `1:6585b-6589d` | 66 | `DOS/CLIB3R.LIB:strnicmp` | Baseline `libscan` |
| `1:6589d-658a0` | 3 | Alignment | Zero fill, no fixups |
| `1:658a0-65968` | 200 | `MATH387R.LIB:exp387` | Baseline `libscan` |
| `1:65968-659df` | 119 | `MATH387R.LIB:pow87e` | Baseline `libscan` |
| `1:659df-659f4` | 21 | `DOS/CLIB3R.LIB:setefg` | Targeted 9.5b OMF comparison |
| `1:659f4-65c49` | 597 | `MATH387R.LIB:dosinite` | Baseline `libscan` |
| `1:65c49-65cc4` | 123 | `DOS/CLIB3R.LIB:chk8087` | `chk8087` field addend +5 at +0x22 |
| `1:65cc4-6672c` | 2664 | `DOS/CLIB3R.LIB:scnf` | Baseline `libscan` |
| `1:6672c-66737` | 11 | `DOS/CLIB3R.LIB:cinit` | Targeted 9.5b OMF comparison |
| `1:66737-66fcc` | 2197 | `DOS/CLIB3R.LIB:prtf` | Baseline `libscan` |
| `1:66fcc-67013` | 71 | `DOS/CLIB3R.LIB:initfile` | Baseline `libscan` |
| `1:67013-6706b` | 88 | `DOS/CLIB3R.LIB:ioexit` | Baseline `libscan` |
| `1:6706b-670e8` | 125 | `DOS/CLIB3R.LIB:qwrite` | Baseline `libscan` |
| `1:670e8-6711c` | 52 | `DOS/CLIB3R.LIB:flushall` | Baseline `libscan` |
| `1:6711c-67137` | 27 | `DOS/CLIB3R.LIB:getche` | Baseline `libscan` |
| `1:67137-67278` | 321 | `DOS/CLIB3R.LIB:sbrk386` | Baseline `libscan` |
| `1:67278-6734c` | 212 | `DOS/CLIB3R.LIB:setenvp` | Baseline `libscan` |
| `1:6734c-673c4` | 120 | `MATH387R.LIB:math87e` | Baseline `libscan` |
| `1:673c4-674a4` | 224 | `MATH387R.LIB:math2err` | Baseline `libscan` |
| `1:674a4-674b4` | 16 | `DOS/CLIB3R.LIB:noefgfmt` | String target `2:209c` and both startup handler pointers |
| `1:674b4-675cf` | 283 | `DOS/CLIB3R.LIB:efgfmt` | Baseline `libscan` |
| `1:675cf-675dd` | 14 | `MATH387R.LIB:cnvs2d` | Targeted 9.5b OMF comparison |
| `1:675dd-67705` | 296 | `DOS/CLIB3R.LIB:hook387` | Baseline `libscan` |
| `1:67705-67708` | 3 | Alignment | Zero fill, no fixups |
| `1:67708-6a8da` | 12754 | `DOS/EMU387.LIB:emu387` | `emu387` body and fixup sites |
| `1:6a8da-6a8db` | 1 | `DOS/CLIB3R.LIB:save8087` | `___Save8087` and `___Rest8087` at `2:abcc` and `2:abd0` |
| `1:6a8db-6a8dc` | 1 | Alignment | Zero fill, no fixups |
| `1:6a8dc-6a909` | 45 | `DOS/CLIB3R.LIB:init8087` | Baseline `libscan` |
| `1:6a909-6a90c` | 3 | Alignment | Zero fill, no fixups |
| `1:6a90c-6a964` | 88 | `DOS/CLIB3R.LIB:fdfs` | Baseline `libscan` |
| `1:6a964-6a9f3` | 143 | `DOS/CLIB3R.LIB:itoa` | `itoa_` call with operand at `1:66db4` |
| `1:6a9f3-6aa5c` | 105 | `MATH387R.LIB:_matherr` | Targeted 9.5b OMF comparison |
| `1:6aa5c-6adc0` | 868 | `MATH387R.LIB:ftos` | Baseline `libscan` |
| `1:6adc0-6af6a` | 426 | `MATH387R.LIB:strtod` | Baseline `libscan` |
| `1:6af6a-6af6c` | 2 | Alignment | Zero fill, no fixups |
| `1:6af6c-6af71` | 5 | `MATH387R.LIB:__mtherr` | rel32 selects `matherr_`, which also exists in MATH3R |
| `1:6af71-6af90` | 31 | `DOS/CLIB3R.LIB:seterrno` | Baseline `libscan` |
| `1:6af90-6b002` | 114 | `MATH387R.LIB:nan_inf` | Baseline `libscan` |
| `1:6b002-6b004` | 2 | Alignment | Zero fill, no fixups |
| `1:6b004-6b014` | 16 | `MATH387R.LIB:fabs387` | Baseline `libscan` |
| `1:6b014-6b083` | 111 | `MATH387R.LIB:log387` | Baseline `libscan` |
| `1:6b083-6b193` | 272 | `MATH387R.LIB:scale` | Baseline `libscan` |
| `1:6b193-6b2ff` | 364 | `MATH387R.LIB:cvt` | Baseline `libscan` |
| `1:6b2ff-6b300` | 1 | Alignment | Zero fill, no fixups |
| `1:6b300-6b42e` | 302 | `DOS/CLIB3R.LIB:amodf` | Baseline `libscan` |
| `1:6b42e-6b431` | 3 | `MATH387R.LIB:matherr` | Called through `__matherr`. MATH3R copy identical |
| `1:6b431-6b467` | 54 | `MATH387R.LIB:log87e` | Baseline `libscan` |
| `1:6b467-6b468` | 1 | Alignment | Zero fill, no fixups |
| `1:6b468-6b636` | 462 | `MATH387R.LIB:i64tos / MATH3R.LIB:i64tos` | MATH387R and MATH3R member bytes are identical |
| `1:6b636-6b685` | 79 | `MATH387R.LIB:frexp` | Baseline `libscan` |

The AdLib FX tail begins with a callback-pointer setter at `1:62b9c`. The init and shutdown routines at `1:62ba4` and `1:62cb8` act on the same flags and use the lock interval `1:627cc-62b98`. `1:627cc` matches `ADLIBFX_SendOutput_` and the timer callback at `1:6288c` matches `ADLIBFX_Service_` in ASS 1.04. The scan's `1:62b9c` audio endpoint was a public-match cutoff inside the module.

### Obj2 initialized runtime data

**PROVEN.** 46 Watcom contributions occupy 2,296 initialized bytes. Code fixups anchor data segments, then the complete segment bytes and LE site sets are compared. The initializer tables were checked entry by entry against their priority bytes and linked code targets. The `_NULL` bytes identify `cstart` at the start of DGROUP. The `noefgfmt` string and both handler pointers distinguish its 16-byte code from `nomem`, which shares the instruction shape.

| Range | Bytes | Contribution | Segment or initializer target |
|---|---:|---|---|
| `2:00000-00004` | 4 | `DOS/CLIB3R.LIB:cstart` | `_NULL` |
| `2:01ef0-0202d` | 317 | `DOS/CLIB3R.LIB:strerror` | `CONST` |
| `2:02030-0209c` | 108 | `MATH387R.LIB:math2err` | `CONST` |
| `2:0209c-020c0` | 36 | `DOS/CLIB3R.LIB:noefgfmt` | `CONST` |
| `2:020c0-020d0` | 16 | `DOS/CLIB3R.LIB:hook387` | `CONST` |
| `2:020d0-0215a` | 138 | `MATH387R.LIB:_matherr` | `CONST` |
| `2:0215c-02164` | 8 | `MATH387R.LIB:nan_inf` | `CONST` |
| `2:02164-02265` | 257 | `DOS/CLIB3R.LIB:istable` | `CONST2` |
| `2:02268-02270` | 8 | `DOS/CLIB3R.LIB:scnf` | `CONST2` |
| `2:02270-02278` | 8 | `DOS/CLIB3R.LIB:hugeval` | `CONST2` |
| `2:02278-02294` | 28 | `MATH387R.LIB:_matherr` | `CONST2` |
| `2:02294-022b4` | 32 | `MATH387R.LIB:scale` | `CONST2` |
| `2:022b4-02304` | 80 | `MATH387R.LIB:fpow10` | `CONST2` |
| `2:0a71c-0a724` | 8 | `DOS/CLIB3R.LIB:cstart` | `_DATA` |
| `2:0a724-0a730` | 12 | `DOS/CLIB3R.LIB:exit` | `_DATA` |
| `2:0a730-0a738` | 8 | `DOS/CLIB3R.LIB:argcargv` | `_DATA` |
| `2:0a738-0a73c` | 4 | `DOS/CLIB3R.LIB:rand` | `_DATA` |
| `2:0a73c-0a76c` | 48 | `DOS/CLIB3R.LIB:nmalloc` | `_DATA` |
| `2:0a76c-0a76e` | 2 | `DOS/CLIB3R.LIB:_8087` | `_DATA` |
| `2:0a770-0a7b4` | 68 | `DOS/CLIB3R.LIB:strerror` | `_DATA` |
| `2:0a7b4-0a7c8` | 20 | `DOS/CLIB3R.LIB:stk` | `_DATA` |
| `2:0a7c8-0a808` | 64 | `DOS/CLIB3R.LIB:crwdata` | `_DATA` |
| `2:0a808-0aa19` | 529 | `DOS/CLIB3R.LIB:iob` | `_DATA` |
| `2:0aa1c-0aa30` | 20 | `DOS/CLIB3R.LIB:dosret` | `_DATA` |
| `2:0aa30-0aa34` | 4 | `DOS/CLIB3R.LIB:umaskval` | `_DATA` |
| `2:0aa34-0aa8c` | 88 | `DOS/CLIB3R.LIB:iomode` | `_DATA` |
| `2:0aa8c-0aa94` | 8 | `DOS/CLIB3R.LIB:environ` | `_DATA` |
| `2:0aa94-0aadd` | 73 | `MATH387R.LIB:dosinite` | `_DATA` |
| `2:0aae0-0aae4` | 4 | `DOS/CLIB3R.LIB:amblksiz` | `_DATA` |
| `2:0aae4-0ab40` | 92 | `MATH387R.LIB:math2err` | `_DATA` |
| `2:0ab40-0ab48` | 8 | `DOS/CLIB3R.LIB:noefgfmt` | `_DATA` |
| `2:0ab48-0ab4a` | 2 | `DOS/CLIB3R.LIB:8087cw` | `_DATA` |
| `2:0ab4c-0ab4e` | 2 | `DOS/CLIB3R.LIB:hook387` | `_DATA` |
| `2:0ab50-0abca` | 122 | `DOS/EMU387.LIB:emu387` | `_DATA` |
| `2:0abcc-0abd4` | 8 | `DOS/CLIB3R.LIB:save8087` | `_DATA` |
| `2:0abd4-0abd8` | 4 | `MATH387R.LIB:_matherr` | `_DATA` |
| `2:0abd8-0abdc` | 4 | `MATH387R.LIB:ftos` | `_DATA` |
| `2:0ac0c-0ac12` | 6 | `DOS/CLIB3R.LIB:argcargv` | `XI` to `1:64aa4` |
| `2:0ac12-0ac18` | 6 | `DOS/CLIB3R.LIB:fltused` | `XI` to `1:659df` |
| `2:0ac18-0ac1e` | 6 | `DOS/EMU387.LIB:386inite` | `XI` to `1:645e0` |
| `2:0ac1e-0ac24` | 6 | `DOS/CLIB3R.LIB:_8087` | `XI` to `1:65c85` |
| `2:0ac24-0ac2a` | 6 | `DOS/CLIB3R.LIB:stk` | `XI` to `1:646e8` |
| `2:0ac2a-0ac30` | 6 | `DOS/CLIB3R.LIB:stinit` | `XI` to `1:66fcc` |
| `2:0ac30-0ac36` | 6 | `DOS/CLIB3R.LIB:environ` | `XI` to `1:67278` |
| `2:0ac36-0ac3c` | 6 | `DOS/EMU387.LIB:386inite` | `YI` to `1:645e5` |
| `2:0ac3c-0ac42` | 6 | `DOS/CLIB3R.LIB:stinit` | `YI` to `1:67013` |

Zero alignment inside the runtime ranges occupies `2:202d-2030`, `2:215a-215c`, `2:2265-2268`, `2:a76e-a770`, `2:aa19-aa1c`, `2:aadd-aae0`, `2:ab4a-ab4c`, `2:ab4e-ab50` and `2:abca-abcc`. The `2:abdc-ac0c` initializer records target Apogee audio code. Zero fill follows Watcom's last YI entry at `2:ac42`. `cstart` names `2:ac50` as `_edata`, with file-backed zero fill through `2:b000`. The errno slots at `2:1c8928-1c8930` are BSS. The `bits` member has the same eight constant bytes as `scnf` at `2:2268`. The linked `__scnf_` fixup points into that segment directly.

## B. Engine-to-audio boundary

**PROVEN.** The observed engine tail precedes `1:49c84`. The next 17,636 bytes are four consecutive Creative AWE32 modules in `origlibs/104/AUDIO_WF.LIB`. Their library comments identify MetaWare High C `dosomf v2.05b(4pcs)` and `hc2.5k -O6 -386 -X7`. With the Easy OMF LOC=5 off32 width established from the instructions and table dwords, all four full code contributions have equal covered bytes and absolute sites. The default `libscan --split` finds many shaped hits and calls them masked because it counts zero absolute OMF sites for these members.

| Range | AWE32 member | Evidence |
|---|---|---|
| `1:49c84-4a77c` | `nrpn.c` | `AWE32INITNRPN` at `1:49c8c`, called at `1:62650`. It installs controller handlers and initializes channel parameters. |
| `1:4a77c-4ba90` | `sbkload.c` | `AWE32TOTALPATCHRAM` at `1:4a784`. `AWE32SFONTLOADREQUEST` at `1:4b480` tests `RIFF`, `SFBK`/`sfbk`, `INFO` and `ifil`. |
| `1:4ba90-4c4f4` | `hardware.c` | `AWE32DETECT` at `1:4ba98`, hardware init at `1:4c338`, termination at `1:4c480`. |
| `1:4c4f4-4e168` | `midieng.c` | MIDI register access, note handling and `AWE32PROGRAMCHANGE` at `1:4dcec`. `1:4dea4` is `AWE32POLYKEYPRESSURE` inside this module. |

`build/map/funcmap.json` has call starts at the corresponding public entry points and shows the switch-table cross-references inside the longer members. Engine-side calls at `1:49797`, `1:49817` and `1:498c5` still participate in the DOS interrupt, conventional-memory and networking neighborhood before the Creative start marker. `1:4e168-4e170` is zero fill after the MIDI member. The four-module evidence accounts for the entire formerly unidentified `1:4a000-4dea4` span as Apogee AWE32 support compiled from Creative code. `libscan.py --split` over all 339 historical `external/x` OBJs returned 68 hits overall and zero in `1:49c84-4dea4`. This includes the Ken engine, cache, multiplayer and KDM object candidates. The 1.09, 1.1, 1.12 and ROTT beta/demo ASS libraries also show the shared Creative member signatures, so this block alone does not determine the exact Apogee release.

The Creative data matches adjacent `_DATA` contributions at `2:2e7c-3484` (`nrpnvar.c`), `2:3484-36c8` (`nrpn.c`), `2:36c8-38d0` (`sbkload.c`), `2:38d0-3ad8` (`hardware.c`), `2:3ad8-4434` (`midivar.c`), and `2:4434-49c4` (`midieng.c`).

## C. Ken's A.ASM

**PROVEN.** MASM 5.10 reproduces the complete 2,742-byte `BUIL0422/A.OBJ` given the lower-case `a.asm;` command. SHA-256 `b0b10c83900d5bb49ebcc67c7f1ee2b7b7a118a7bb440b2b722661ec44f8bd23`. `tools/objcmp.py --funcs` reports 7/7 functions, 1,441/1,441 code bytes and 134/134 normalized fixups. The upper-case input spelling changes the THEADR case and record checksum while preserving the code and fixups.

| Historical comparison | Evidence |
|---|---|
| April calibration versus `BUIL0801/A.OBJ` | August grows the code from 1,441 to 2,712 bytes and the fixup count from 134 to 220. First raw code difference at `+0x27`. All seven common public extents changed. August adds chain setup, translucent vertical drawing and the four-column masked loop. |
| June/July/August versus D3D | `BUIL0624` and `BUIL0719` each have 2,683 code bytes and 218 fixups. All three versions locate the 503-byte `hchainasm64` at `1:6b931`. Three of the ten August fields absent from the LE site set are resolved same-segment rel32 branches. The remaining seven palette references changed from static `_PALOOKUP` relocations to `88888888h` operands filled by `chainsetpalookupaddress`. |
| September/October/November/December objects | `BUIL0906`, `BUIL1020`, `BUIL1109` and `BUIL1211` supply variant rasterizers. No whole code segment matches. `BUIL1020` gives exact+sites anchors for both horizontal alternate entries. |
| May 1995 source assembled with MASM 5.10 | Six split exact+sites anchors: `mvlineasm1`, `tvlineasm1`, `vlineasm4`, `mvlineasm4`, `mhlineskipmodify` and `thlineskipmodify`. The 3,361-byte May object has a different whole-unit layout. |

The reconstructed source uses mnemonics and symbolic operands. Its only data declarations are the initialized zero scratch slots for the planar rasterizer. Near branches forced by MASM use `near ptr`, and the masked column dispatch uses `OFFSET` for its relocated immediate. The public names come from the historical objects, the April and May sources, and March 1995 `ENGINE.C` declarations and call signatures. Local labels are reconstructed. A direct engine caller is recorded for each public in the overlay. In particular, engine branches at `1:37344` versus `1:37370` distinguish the chain and linear setup, and `1:39dd8` versus `1:39e38` select their draw routines. `1:471fb` selects the sprite-column loop and its preceding stores bind all four scratch globals. The March horizontal span caller independently corroborates the `asm3` count at `2:1bc3b8`. Historical `BUIL0719/ENGINE.OBJ` and `BUIL1211/ENGINE.OBJ` give the four-slot order, and the historical A objects identify the consumers by name.

The object has 3,520 code bytes, 247 absolute fixups and 41 resolved rel32 fixups. Thirty-nine absolute references target the four engine scratch globals. The final object SHA-256 is `362da7e4faadfb6f4e920e06dbdd24dadc65cacf23a4690e148079fa3061be7f`.

### Fresh assemble and verify

Run from the repository root in PowerShell. The source basename and output remain short inside the worker scratch directory.

```powershell
$worker = 'build/workers/runtime'
New-Item -ItemType Directory -Force "$worker/probe" | Out-Null
Copy-Item "$worker/src/A.ASM" "$worker/probe/A.ASM"
Push-Location "$worker/probe"
$scratch = (Get-Location).Path
$env:TMP = $scratch
$env:TEMP = $scratch
& C:/tools/nmlgcdos/msdos.exe C:/tools/masm-5.10/files/MASM.EXE 'a.asm;'
Pop-Location
python -B tools/match.py --obj "$worker/probe/A.OBJ" --symbols "$worker/overlay.json"
```

The assembler reported zero warnings and zero severe errors. The verifier reported the following.

```text
# object _TEXT layout: contiguous
STRICT           SETHLINESIZES                1:6b685   size  119
STRICT           SETHLINECHAINSIZES           1:6b6fc   size  119
STRICT           SETPALOOKUPADDRESS           1:6b773   size   36
STRICT           CHAINSETPALOOKUPADDRESS      1:6b797   size   36
STRICT           HLINEASM64                   1:6b7bb   size  374
STRICT           HCHAINASM64                  1:6b931   size  503
STRICT           SETVLINEBPL                  1:6bb28   size  191
STRICT           VLINEASM1                    1:6bbe7   size   61
STRICT           MVLINEASM1                   1:6bc24   size   57
STRICT           FIXTRANSLUSCENCE             1:6bc5d   size   21
STRICT           TVLINEASM1                   1:6bc72   size   67
STRICT           VLINEASM4                    1:6bcb5   size  308
STRICT           MVLINEASM4                   1:6bde9   size  839
STRICT           SPRITEVLINE                  1:6c130   size  144
STRICT           MSETHLINESHIFT               1:6c1c0   size   36
STRICT           MHLINE                       1:6c1e4   size   80
STRICT           MHLINESKIPMODIFY             1:6c234   size  214
STRICT           TSETHLINESHIFT               1:6c30a   size   36
STRICT           THLINE                       1:6c32e   size   70
STRICT           THLINESKIPMODIFY             1:6c374   size  209
# STRICT 20/20 functions, 3520 bytes
```

Tool SHA-256 pins

- `C:/tools/masm-5.10/files/MASM.EXE`: `233120302473749eafe9a54a30f8bda0127140c44346e5bcc047d113828a741e`
- `C:/tools/nmlgcdos/msdos.exe`: `f7f6cb0a3e816c5edb13112d327c1bddbf7463fe7bf9a005ca1eb5317751bd02`

To reproduce April calibration, copy `BUIL0422/A.ASM` into `build/workers/runtime/cal/A.ASM`, run the same assembler command there, then run `python -B tools/objcmp.py <K1>/BUIL0422/A.OBJ build/workers/runtime/cal/A.OBJ --funcs`.

## D. DOS MZ stub

**PROVEN.** The load image and all six 4-byte relocation entries in `D3D.EXE` match the installed Watcom 9.5b `BINB/WSTUB.EXE` byte-for-byte. This can be verified directly with `D3D[0x40:0x28b2] == WSTUB[0x20:]`. The MZ header grew from 64 to 96 bytes. Its relocation table moved from `0x20` to `0x40`, its last-page byte count grew from 146 to 178, and its page count remained 21. In the common first 32 bytes, only file offsets `0x02`, `0x08` and `0x18` differ. Added header space carries zeros and `e_lfanew=0x28b8`. Six zero bytes follow the MZ declared end at `0x28b2` before the LE signature.

The stub contains the 1988–1992 Watcom Systems Inc. notice, `DOS4GPATH` and `Stub exec failed:`. Watcom 9.5b `SRC/WSTUB.C` implements those paths and messages, and `BINB/WLSYSTEM.LNK` selects `wstub.exe` with `format os2 le` for `system dos4g`. The expanded 10,418-byte stub also matches `assets/SETUP.EXE` and multiple historical Build EXEs in full. Installed `WSTUBQ.EXE` and Watcom 10.x and 11.0 stubs have different load images. The native 9.5b `BINNT/WLINK.EXE` link probe did not return in this environment. The byte comparison identifies the stub input and the format transformation. The historical linker executable build remains open.

- Expanded MZ SHA-256 `ee3a4916bc5b76147697397739bc9e9004a0f66dde7d25803028520e3e9a4dbd`
- Installed WSTUB SHA-256 `95aec99b60d20426ad4e79d24df599fc5e40e16c40ea9d60e0566181d116adaf`
