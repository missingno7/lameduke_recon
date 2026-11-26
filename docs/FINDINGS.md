# Findings (evidence log)

Levels: PROVEN (independently reproducible check), STRONG, HYPOTHESIS, NEGATIVE (tested and rejected), OPEN.
Each entry names the command or artefact that supports it.

## Oracle and container (2026-11-26)
- PROVEN `assets/D3D.EXE` 685121 bytes, sha256 698979d9…0462701. MZ stub 0x0-0x28b2 (10418-byte image), LE header at
  0x28b8 (dword at MZ+0x3C). `python tools/le.py` accounts for every file byte: header+loader 0x28b8-0x2b95,
  fixup section to 0x2f711, 239 zero bytes, data pages 0x2f800-EOF. No overlay, no debug info, no non-resident names.
- PROVEN two objects: obj1 code base 0x10000 vsize 0x6c445 (109 pages); obj2 data base 0x80000 vsize 0x1c99d0
  (11 pages, 0xb000 file-backed = initialised data; rest BSS; ESP = obj2:0x1c99d0). EIP = obj1:0x62e00 (cstart).
- PROVEN 21046 internal fixups, all source type 0x07 (32-bit offset); none are selector/import fixups.
- PROVEN provenance: archive.org item `lameduke` (lameduke.zip, md5 81a0a391cd064b4279b6f995fada7eda) contains a
  D3D.EXE identical to the oracle, dated 1994-12-30 18:38; SETUP.EXE 1994-12-29 15:01; DOS4GW.EXE 1994-05-31;
  GAME.CON 1994-12-30 19:47 (after the EXE); TILES001.ART 1995-01-01; demos to 1995-01-02.
- PROVEN resident module name `game` (linked output was GAME.EXE).
- PROVEN distributed DOS4GW.EXE = DOS/4GW 1.95 (Nov 23 1993), byte-identical to `C:/tools/watcom-10.0/BIN/dos4gw.exe`.
- OPEN the 10418-byte MZ stub matches no installed WSTUB.EXE (9.5b 10386, 10.0 10610 bytes).
- STRONG Watcom string-literal padding contains stale source-buffer bytes (e.g. "elp" before "Demorec mode on"), as
  documented for Watcom 10.0 `-ot` in other projects: data matching will depend on exact source text.

## Runtime library (2026-11-26)
- PROVEN runtime = Watcom 9.5-family: banner "WATCOM C/C++32 Run-Time system ... 1988-1993" occurs only in 9.5b CLIB
  (10.0/10.0a say 1988-1994, 10.5+ 1988-1995). `libscan.py --compare` 9.5b vs 10.0 CLIB3R: 9.5b has 86 exact+sites
  whole-member hits, 10.0 only trivially small ones. With MATH387R/MATH3R: 104 exact+sites members, 20,790 bytes
  (`build/scan/clib95b.json`). Runtime occupies roughly obj1:0x62d8d-0x6b685, cstart at 0x62e00.
- PROVEN runtime patch level is 9.5b, not 9.5c: the 9.5c (1994-06-30) CLIB3R replacement library (c32_c.zip
  3CRDL.C, an OMF library) has changed versions of scnf, sbrk386, nmalloc, grownear, fputs; none of them occurs in the
  target while their 9.5b versions match exact+sites. (close and fclose are unchanged and match in both.)
- OPEN 3 masked (sites-differ) members: sscanf, initrtns, chk8087; unexplained runtime gaps in 0x62d8d-0x6b685.

## Compiler calibration (2026-11-26)
- PROVEN Watcom 9.5 (`watcom-9.5b` BINNT/WCC386.EXE, banner "Version 9.5") with `-4r` compiles
  `kenbuild-20241213/BUIL0422/ENGINE.C` (+ its BUILD.H, NAMES.H) into an object identical to Ken's original
  `BUIL0422/ENGINE.OBJ`: all segments, 2950/2950 fixups, 88/88 functions (`tools/objcmp.py … --funcs`).
  Ken's makefiles say `wcc386 engine /4r` and `masm a.asm;`.
- PROVEN discriminating probe: the same BUIL0422 source with `-4r` under wc100 / wc100a / wc105 / wc106 reproduces only
  13 / 11 / 13 / 13 of 88 functions. Watcom 9.5 code generation is distinguishable from 10.x, so any byte-exact
  match of 1994 engine code identifies the 9.5 code generator.
- OPEN which flags the Duke game code used.

## Engine (2026-11-26)
- PROVEN target map version 5 (all assets/*.MAP start with 05 00 00 00). Per buil1995/BUILD.TXT: map v5 introduced
  9/16/94 (CONVMAP5), v6 on 1/1/95. So the target's engine is between the Oct-94 BUILD.H
  (build_1994_10_17_partial, map v5) and the 1995-03-28 ENGINE.C (map v6). BUILD.TXT dates every engine change
  between them (gotsector 11/9/94, parallax tiling 11/23/94, floor sprites 11/29/94, MAXWALLS 8192 12/23/94,
  startumost/dmost to short 1/3/95, …).
- NEGATIVE `buildold_12_30_94` is not a Dec-94 engine snapshot: its GAME.C is dated 1994-09-01, BUILD.EXE 1994-08-28,
  only SETUP.DAT is from 12/30/94; no ENGINE.C/OBJ.
- NEGATIVE `SRC_9403/ENGINE.OBJ` (FILESLST date 1995-03-21) is a March-1994 build (44 KB code, includes NAMES.H like
  BUIL0408/0422); 0 function hits.
- PROVEN historical ENGINE.OBJ files give only small exact hits: BUIL0801 9 functions/790 bytes (ksqrt, krand,
  getmousevalues, netoff, comsend, convalloc32, simulateint, getcrc, uninstallbicomhandlers). Engine code lies at
  about obj1:0x34000-0x4a000.
- OPEN BUIL0801/A.OBJ matches at obj1:0x6b931 only masked (sites differ): A.OBJ is placed after the runtime and
  differs from the Aug-94 version.

## Audio library (2026-11-26)
- PROVEN the target links the Apogee Sound System (multivoc, FX_MAN, BLASTER, GUS/GUSWAVE/GUSMIDI, PAS16, AdLib,
  PC speaker, Sound Source, AWE32, MPU-401, task manager, DMA, DPMI, ll_man). ASS 1.04 AUDIO_WF.LIB
  (gamesrc-ver-recreation origlibs) gives 31 whole-module exact+sites hits (22.6 KB) and 343 per-function
  exact+sites hits (48.1 KB); later versions give fewer. Region obj1:0x4dea4-0x62b9c.

## Layout (STRONG, from scans)
- obj1 0x0000-0x0010 cstart BEGTEXT; ~0x10-~0x34000 game code (Duke-specific, no historical source);
  ~0x34000-~0x4a000 Build engine (ENGINE.OBJ); ~0x4a000-0x4dea4 unidentified; 0x4dea4-0x62b9c Apogee Sound System;
  0x62d8d-0x6b685 Watcom 9.5 runtime; ~0x6b685-0x6c445 Ken's A.OBJ (assembly).
- `tools/funcmap.py`: 1284 candidate function starts, 404 KB reached by recursive descent, 1326 jump-table entries,
  39 KB unreached (dead functions or data in code).

## Worker results 2026-11-26 (verified by the integrator)
- PROVEN runtime census (docs/RUNTIME_CENSUS.md): obj1:62d8d-6b685 = 125 Watcom 9.5b CLIB3R/MATH387R/EMU387/MATH3R
  contributions (35,001 bytes) + zero alignment; 46 runtime data contributions in obj2 (2,296 bytes). The three
  "masked" libscan members were a scanner limitation (OMF target method 4 keeps the addend in the field; Easy OMF
  LOC=5 fields are dwords), not version differences. libscan.py classify() still has this limitation (OPEN fix).
- PROVEN obj1:49c84-4e168 = four Creative AWE32 modules (nrpn, sbkload, hardware, midieng) shipped inside ASS 1.04's
  AUDIO_WF.LIB, compiled by MetaWare High C (dosomf v2.05b, hc2.5k -O6 -386). So the engine ends before 1:49c84.
- PROVEN MZ stub = Watcom 9.5b BINB/WSTUB.EXE load image + relocations (D3D[0x40:0x28b2] == WSTUB[0x20:]) with the MZ
  header grown from 64 to 96 bytes (as WLINK writes it). Linker build OPEN.
- PROVEN MASM 5.10 (`masm a.asm;`, under MS-DOS Player) reproduces BUIL0422/A.OBJ byte-for-byte; reconstructed
  Dec-94 A.ASM = 20/20 STRICT, 3,520 bytes (unit a_asm). The target's A.OBJ differs from every surviving A.OBJ
  (Apr/Jun/Jul/Aug-94, Sep-95..Dec-95) as a whole unit.
- STRONG game layout (docs/GAME_MAP.md): game 1:00010-33e5c in seven TUs ~ GAME(+PREMAP) 00010-14fb0, SECTOR
  14fb0-1f280, PLAYER 1f280-25160, ACTORS 25160-2ba30, SOUNDS 2ba30-2c1c0, MENUES 2c1c0-2e700, GAMEDEF 2e700-33e5c;
  engine starts at 1:33e5c (keyhandler IRQ). Map structs (v5): sector 37, wall 35, sprite 38, extra 10 bytes.
- PROVEN game compiler profile reproducing 9 probe functions: wc95b `-4r -s -otexan` (also -otexa / without -s);
  -4r alone, -ot, -ox, -3r/-5r fail; wc100/100a/105/106 fail USRHOOKS_GetMem at +1. Packing 1 byte (-zp1 default).
- FIXED verifier gap found by the game worker: a candidate shorter than the original function could pass as STRICT.
  match.py now requires the extent end to be a confirmed boundary (bound next public, bound symbol, funcmap start
  or object end); negative control tests/test_match.py::test_truncated_candidate.

## Audio library (audio worker 2026-11-26, integrated; details docs/AUDIOLIB.md)
- PROVEN audio region 1:49c84-62d8d holds 56 code members in link order: Creative PAWE32 (MetaWare High C, 4),
  Gravis GF1_OSF SDK (30, incl. 3 assembly), then Apogee C modules USER, TASK_MAN, SNDSRC, PITCH, PCFX, PAS16, MUSIC,
  MULTIVOC, MPU401, MIDI, LL_MAN, IRQ, GUSWAVE, GUSMIDI, GUS, FX_MAN, DPMI, DMA, BLASTER, AL_MIDI, AWE32, ADLIBFX.
  38 members are whole-code+sites identical to ASS 1.04's members.
- STRONG the Apogee modules were compiled by their author with Watcom 10.0 GA (13 complete target code segments
  reproduce with wc100; wc95b reproduces 0 whole 1.04 segments, wc100a reproduces 1.04 but not the target's USER).
  So library and game used different compilers: the game/runtime are Watcom 9.5b.
- PROVEN the target library predates 1.04: earlier MULTIVOC voice layout, GUSWAVE 84-byte voices (VOC only),
  GUSMIDI reads ULTRASND.INI, 12-card FX enum without SoundScape, no *_VoiceAvailable, PITCH table computed with pow.
- Creative and Gravis members are binary vendor libraries: identity only (config/library_symbols.json).

## Data padding and holes (gamedef worker 2026-11-26, PROVEN)
- Watcom 9.5b CONST padding: within an LEDATA record, alignment gaps keep the bytes at the same positions of the
  reused record buffer (e.g. "cto" from "actor"). Replaying the buffer explains all 123 padding bytes of GAMEDEF's
  CONST pool (66 nonzero) from the ordinary source literals: natural literals reproduce it; no steering needed.
- Alignment at a record transition leaves an OMF hole (no LEDATA); Watcom 9.5 WLINK `system dos4g` writes zeros
  there (natural link check). tools/match.py now models data-segment holes as linker zero fill (DATA_HOLES_ARE_ZERO;
  CODE holes still rejected; control tests/test_match.py::test_data_hole_model). SOUNDS' explicit trailing NULs are
  probably unnecessary under this model (to re-check).
- GAMEDEF private state: last_used_text = 2:b1df0 (STRONG, BSS group-layout controls).
