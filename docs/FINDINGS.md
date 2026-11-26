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
- OPEN 3 masked (sites-differ) members: sscanf, initrtns, chk8087; unexplained runtime gaps in 0x62d8d-0x6b685.

## Compiler calibration (2026-11-26)
- PROVEN Watcom 9.5 (`watcom-9.5b` BINNT/WCC386.EXE, banner "Version 9.5") with `-4r` compiles
  `kenbuild-20241213/BUIL0422/ENGINE.C` (+ its BUILD.H, NAMES.H) into an object identical to Ken's original
  `BUIL0422/ENGINE.OBJ`: all segments, 2950/2950 fixups, 88/88 functions (`tools/objcmp.py … --funcs`).
  Ken's makefiles say `wcc386 engine /4r` and `masm a.asm;`.
- OPEN whether Watcom 10.0 -4r also reproduces BUIL0422 (discriminating probe pending) and which flags the Duke
  game code used.

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
