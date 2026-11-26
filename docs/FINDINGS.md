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
- SUPERSEDED (2026-11-26, see 'Patch level' below: the runtime is 9.5 GA, the install named watcom-9.5b is unpatched) runtime patch level is 9.5b, not 9.5c: the 9.5c (1994-06-30) CLIB3R replacement library (c32_c.zip
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

## Compile environment and BSS layout (2026-11-26)
- PROVEN Watcom 9.5b CONST padding residue depends on the compile path: the same GAMEDEF source compiled from a
  22-character-longer directory changes 4 covered padding bytes; from a short directory it is deterministic and
  oracle-identical. tools/wcc.py therefore stages every C compile into one fixed directory (build/cc/s, fixed TMP
  build/cc/t, fixed object name, lock file); override with LAMEDUKE_STAGE. Results are reproducible only for the
  same stage path length; the original path is unknown (OPEN; the residue may constrain it).
- PROVEN (sounds worker, docs/BSS_LAYOUT.md, tools/bssrule.py) Watcom 9.5b orders _BSS by size bucket, newest
  22-slot declaration page first, then ascending 12-bit identifier hash modulo 211; -ot adds type-based alignment
  (6 bytes for far pointers). Predicts all 293 _BSS and 12 _DATA publics of three BUIL0422 objects. Differs from
  the 10.x rule (25-slot pages, % 241).

## CONST residue model (sounds worker 2026-11-26, docs/CONST_RESIDUE.md)
- PROVEN all 226 covered CONST padding bytes of the five integrated 9.5b units are explained: 143 by earlier LEDATA
  writes in the same reused packet buffer, 83 by the initial contents ("seed") of freshly allocated compiler memory.
  The target has 0 in every seed position. The NT-hosted 9.5b compiler leaves heap garbage there, so its output
  depends on the environment (path lengths etc.). Only a path-tuned NT stage reproduces all five pools (not adopted:
  fragile). HYPOTHESIS: the original ran the DOS-hosted compiler (zero fresh memory) -> worker doshost tests BINB
  under DOSBox-X.
- PROVEN residue cannot uniquely identify the December environment (three different environments reproduce every
  pool). Ken's Apr/Aug-94 ENGINE.OBJs record THEADR engine.c, D:\engine.c and c:\watcom\h.
- PROVEN SOUNDS' explicit NUL extensions are still required under every tested environment (removing them changes
  CONST at +27f.. under the reproducing stage): they model real source content of unknown spelling (OPEN).
- Watcom 9.5b BINNT hangs on stage path components of ~69 characters (keep stage paths short).

## DOS-hosted 9.5b compiler under MS-DOS Player (2026-11-26) — NEGATIVE
- Watcom 9.5b's DOS-hosted compiler is C:/tools/watcom-9.5b/BIN/WCC386.EXE, bound to the Phar Lap 386|DOS-Extender
  (strings "Phar Lap Software 1986-92"; BIN/WCC386P.EXE is only a launcher for wcc386.exe; 9.5b has no BINB wcc386).
- Under C:/tools/nmlgcdos/msdos.exe it exits silently with status 1 and no output for every tried option set:
  default, -x (EMS/VCPI/XMS), -d, -x -d, -x -v6.22 (BUIL0422 ENGINE.C, -4r). HX HDPMI32 as a DPMI host under
  MS-DOS Player fails with "HDPMI32: insufficient memory" (-x). Prior evaluation (C:/tools/dos-backends/REPORT.md)
  also found MS-DOS Player has no DPMI host. MS-DOS Player stays the backend for 16-bit real-mode tools (MASM 5.10);
  the extended compiler needs DOSBox-X (worker doshost).

## Canonical DOS host (doshost worker 2026-11-26, adopted)
- PROVEN Watcom 9.5b's DOS compiler (BIN/WCC386.EXE, bound Phar Lap extender; DOS/4GW rejects it) under DOSBox-X
  reproduces Ken's BUIL0422 ENGINE.OBJ completely: code, data, all 2,950 fixups, THEADR engine.c and all 16 dependency
  paths (D:\engine.c, c:\watcom\h...); only the dependency timestamps differ. Its CONST residue is stable under path
  and mtime changes (50 builds). It is the canonical wc95b host (tools/doshost.py via tools/wcc.py; median 1.7 s per
  compile; LAMEDUKE_HOST=nt selects the fast NT compiler for exploration, which gives identical code).
- PROVEN under the DOS host, SOUNDS CONST verifies (12 STRICT) and GAMEDEF's CONST picked up 50 bytes of the
  worker's con_dispatch.inc: inlining both .inc files into gamedef.c restores the verified CONST (9 STRICT). The
  record buffer residue therefore constrains source-file structure: the original GAMEDEF was a single file
  (no such includes). Avoid private .inc splits in reconstructed sources.

## NEAR census (semir worker 2026-11-26)
- tools/semir.py (ported from rayman_recon) classifies all 125 registered NEAR functions: 62 SOURCE, 21
  ALLOCATION-ONLY, 18 LAYOUT, 24 UNKNOWN (docs/NEAR_CENSUS.md). Controls: all 468 STRICT functions report EXACT;
  constant, statement-order and local-width mutations report SOURCE at the right effect.
- A 9.5b allocator tracing probe works on the NT compiler (identical code to DOS); full cgtrace/explain port estimated
  1-2 weeks; targeted hooks 2-4 days (start with wallfront).

## GAME findings (range workers A/B + merge, 2026-11-26)
- STRONG the original GAME source repeats several bodies written out in full rather than compiler-inlined:
  update_timer_input (6 copies in timerhandler), minitext/gametext/numtext/draw_weapon_tile, the view drawing in
  displayrooms, the status-9 walk inside domovethings, the network packet decoder (5 copies). Watcom 9.5b -oe=N does
  not reproduce these (it inlines different callees at every threshold). Modelled as macros (likely the original
  form, cf. Duke 1.5's macro-heavy style).
- PROVEN saveplayer/loadplayer store order (board[5],[6],[7] then game[7]) is the only one of 12 orders that matches.
- Dead code: wait_for_players, playback_demo and spawn_random_effects have no inbound reference in obj1.
- Verifier fix: own data-segment placement could be locked by a reference from a still-NEAR function; match.py now
  presets the unique content-verified base when references imply conflicting bases (all references still checked).

## Allocation ties (gamefix2 round 2, 2026-11-26)
- PROVEN (cg95 traces) the last few bytes of movemusic, getinput, update_air, wakeactors, hitradius are register
  ties among equal-priority temporaries (~45-60 per function) broken by the 9.5b sorter's order over the whole
  function. Local spelling searches (270 and 360 combinations) and all 720 declaration orders (movesector) do not
  move them. Lever to try: replay the sorter (cg95 replay reproduces it) to find how many/which extra equal-priority
  values flip a tie, then look for natural source elsewhere in the function that creates them (the original may
  differ upstream). Large SOURCE-class functions are the better investment meanwhile.

## Patch level: everything is unpatched Watcom 9.5 GA (2026-11-26, integrator) — CORRECTS earlier "9.5b" claims
- PROVEN BPATCH -q (C:/tools/watcom-9.5b/BINB/BPATCH.EXE under MS-DOS Player) reports both C:/tools/watcom-9.5b
  BINNT/WCC386.EXE and BIN/WCC386.EXE (the canonical DOS host) as "has not been patched": the install is 9.5 GA
  despite its directory name. Its CLIB3R lacks the liballoc/heapen modules added by patch a and has the pre-patch
  versions of all 23 members patch a replaced; the target contains those pre-patch versions and none of the a or c
  replacements. So the target's runtime library is 9.5 GA (unpatched), not 9.5b.
- PROVEN the official patch kits (archive.org Watcom_C_9.5 c32_a/b/c; WCC386NT.A/.B/.C applied in order with BPATCH)
  give compilers 9.5a/9.5b/9.5c (derived installs C:/tools/watcom-9.5a-nt, watcom-9.5b-patched-nt, watcom-9.5c-nt;
  toolchain installs wc95a, wc95b_p, wc95c). On Ken's untouched BUIL0422 ENGINE.C -4r they reproduce only 38/38/36 of
  88 functions versus 88/88 for GA: Ken compiled with the unpatched GA compiler in April 1994. On all integrated game
  units the patched compilers also give fewer STRICT functions than GA (e.g. GAME 19 vs 26). NEGATIVE: the patch level
  does not explain the remaining read-modify-write / allocation residue in GAME.
- Consequence: the profile name wc95b means "the 9.5 GA install in C:/tools/watcom-9.5b" (kept for hash pinning).

## Game code-shape rules (gamefix3, 2026-11-26)
- PROVEN (A/B) Watcom 9.5 compares a `short` against an enum constant in 32 bits but against a #define literal in
  16 bits (and byte bit tests); the original uses the 16-bit/byte forms: tile and flag constants were #defines
  (PLAYER, GAMEDEF size fixes). Check every enum compared against short/char in all units.
- STRONG the original's in-function copies (shoot inside processinput, parse inside execute, setanimation inside
  operatesectors) keep their parameters in fresh temporaries (no constant folding): written-out macro/copy bodies
  with argument variables, consistent with the GAME findings.
- STRONG SECTOR's sprite-list loops are top-tested while loops.
