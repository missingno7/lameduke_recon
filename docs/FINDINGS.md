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
- ADDED (2026-11-26) module-last extent rule: the last public of a segment may end before a zero-padding gap when
  every gap byte is zero without fixups, the next known boundary is exactly the end rounded up to 2/4/8/16
  (minimal padding) and the candidate's last byte is non-zero (it cannot absorb padding). Controls:
  tests/test_match.py::ObjectEndControls (positive DPMI_UnlockMemory; truncated segment and absorbed-padding
  negatives). Turns the audiolib "terminal extent" CODE_BYTES_MATCH cases STRICT; no verifier check was removed.

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

## GAME inlining structure (gamefix4, 2026-11-26)
- STRONG the original GAME was built with user-function inlining whose results do not inline again: inittimer,
  uninittimer and FTA are never called in the oracle; record and move_status9 are expanded only in domovethings,
  displayrooms only where drawn from a loop. NEGATIVE: no single global -oe=N reproduces the selection with our
  source (gameexit/write_userdefs become inlinable at 26 before FTA at 32), so written-out copies remain the model.
- PROVEN moveeffectors' compare tree needs the empty labels `case 1: case 7:` (split at 15, oracle body order);
  spawn returns void; sector wallptr/wallnum are signed short in GAME's view.

## First natural link (link worker 2026-11-26, docs/LINK_REPORT.md)
- PROVEN Watcom 9.5 GA WLINK `system dos4g` + OPTION DOSSEG, NODEFAULTLIBS, explicit FILE order and LOADONCALL
  reproduces the full 0x28b8-byte MZ prefix (stub, 96-byte header, relocations), the LE object flags and A.ASM's CODE
  segment landing last in obj1. 152 symbols remain undefined (149 game globals only declared extern, MULTI's two
  real-mode template names, ENGINE printext16_). With UNDEFSOK the image is 637,549 bytes (oracle 685,121); fixed-offset
  agreement obj1 4.77%, obj2 file-backed 18.4%; at function starts STRICT code agrees 85%.
- Ordering facts: GAME starts at 1:00010 as in the oracle; SECTOR is the first shifted unit (net code deficits
  SECTOR 736, GAMEDEF 733, ENGINE 713 bytes). CONST order differs from the first byte (GAME's pool must come first).
  The oracle's stack is larger than the stock 4 KiB (ESP differs by 0x974d0 -> option stack / BSS ownership).

## Data ownership (dataown worker 2026-11-26, docs/OWNERSHIP.md)
- STRONG a code-less object (src/global/global.c; file name HYPOTHESIS, cf. Duke 1.5 GLOBAL.C) defines the shared game
  storage: _DATA 2:023dc-02838 (1,116 B) and _BSS 2:0ac70-a57c3 (633,683 B), one Watcom size-bucket sequence that
  interleaves storage used by every game unit. Both segments VERIFIED. Linked after ACTORS.
- Ownership of all 149 previously undefined game globals assigned with evidence; link undefined symbols 152 -> 58
  (56 GAME string names still provisional externs + 2); 2 if GAME's strings are literals, but then the compiler merges
  duplicate strings the original keeps (CONST fails): OPEN.
- STRONG GAME's code range holds a second object, probably starting at 1:0f160 (own string pool 2:00648-006a8, _BSS
  mspos at 0ac60). OPEN: every _BSS contribution after GAME's starts paragraph-aligned in the oracle (ours: 4).
- Registry: 13 alias bindings dropped (one address = one symbol); SoundToggle/camerasprite kept as field aliases
  inside GLOBAL objects. match.py's BSS layout check now accepts names strictly inside one of the segment's own
  objects (field aliases); names at object starts or outside objects still fail. verify.py now records VERIFIED data
  segments per unit and fails on their regression.

## Inliner facts (gamefix7, 2026-11-26)
- PROVEN timerhandler (2,496 B STRICT) is the compiler's inline expansion of update_timer_input() six times
  (update_timer_input at 1:000d0 is never referenced): with calls and -oe>=150 the code is identical; the inline
  starts a new basic block, which keeps `inc/add [totalclock]` as memory operands (no volatile needed).
  Reproduced in the canonical profile with do{}while(0) blocks (COMPILER_STEERED). OPEN: whether the original GAME
  profile used a large -oe threshold (would make several written-out copies natural).
- STRONG Watcom 9.5 never inlines a function containing a string literal (explains the kept calls to gameexit,
  show_help, read/write_userdefs, loadlookup); functions defined after their caller can still be inlined.
- copybuf pragma is Ken's 1994 form without a modify clause (BUIL0422).

## Link layout rules (link2 worker 2026-11-26, docs/LINK2_REPORT.md)
- PROVEN Watcom 9.5 GA emits a paragraph-aligned _BSS under -ot when the unit defines a double or a far function
  pointer; WLINK honours each contribution's OMF alignment. Which early units had such a definition: OPEN (40 bytes).
- PROVEN FILE order controls same-named segment order (CONST starts with GAME's pool once its literals exist); DOSSEG
  is redundant here; the stack is the stock 0x1000 after paragraph-rounded _end (2:1c89c4 -> ESP 0x1c99d0 exact).
- PROVEN string merging: Watcom 9.5 merges duplicate literals within a unit (reuse_duplicate_strings). With real
  literals and `#pragma off(reuse_duplicate_strings)` scoped at prelevel, undefined symbols 58 -> 2 and file-backed obj2
  agreement 15% -> 99.4%, but GAME loses 10 STRICT (pool differs by 110 bytes). Not adopted. HYPOTHESIS (preferred):
  the duplicates are natural because GAME's range holds two objects (second from ~1:0f160 with its own pool
  2:00648-006a8); literals are merged only within one object.

## GAME is two objects (gamesplit worker 2026-11-26)
- STRONG the GAME range holds two objects: GAME 1:00010-11380 and a second object 1:11380-14fad (xyzmirror ...
  prelevel, enterlevel, check_file_id; Duke 1.5 PREMAP.C order; src/game/premap.c, name HYPOTHESIS). Evidence: disjoint
  CONST pool users (2:00004-00648 vs 2:00648-006a8), mspos _BSS at 2:0ac60, paragraph-aligned object starts. Boundary
  0f160 is equally consistent with the verifier; 11380 chosen (Duke order, jump-table alignment odds).
- PROVEN with the split and plain string literals (no pragma), both CONST pools VERIFIED (GAME's 1,604-byte pool newly
  verified); the natural link has 2 undefined symbols (SoundToggle, camerasprite) and file-backed obj2 agreement
  99.67%, 2:00000-01490 byte-identical. OPEN: why 9.5 keeps two identical banner/unknown_parameter literals apart in one
  unit but merges bonus_percent/bonus_points.

## GAME's inlining is real compiler inlining (oeprobe worker 2026-11-26) — supersedes the macro model
- PROVEN GAME written with ordinary function calls and compiled with Watcom 9.5 GA `-4r -s -otexan -oe=N`, N >= 2500,
  reproduces the oracle's complete intra-object call/inline pattern (0 differences over all caller/callee pairs),
  given (a) the two-object split at 1:0f160 (nothing is inlined across objects; prelevel inlines spawn, so spawn is in
  object 2 — this decides the boundary over 11380) and (b) real string literals. New profile game_oe (-oe=3000) for
  GAME and PREMAP; 30 STRICT (was 29), timerhandler STRICT as six plain calls, displaystatus newly STRICT.
- 9.5 inliner rules (mini tests / A/B): a callee is inlined if its expression-tree node count <= N (plain -oe = 20;
  -oe=N overrides the e in -otexan; no per-caller budget); depth 1 (copies keep inner calls); never inlined: functions
  owning static data (string literals, static locals, initialised local arrays); CORRECTED (oeunits): recursive functions ARE
  inlined one level deep, the copy keeping its self-calls as calls; definition
  order, static, parameter count do not matter; a call stays a call when run-time address arithmetic precedes it in the
  same expression (sprite[i].owner = findlocator(...)).
- Functions with no callers (wait_for_players, playback_demo, strget, network_move_loop, numtext copies,
  spawn_random_effects, reset_player_motion, render_camera_tile copies) are inline sources, not dead code.
- Other units: PLAYER/SOUNDS unaffected by N; MENUES/ACTORS change at N>=100 (untested); GAMEDEF crashes wcc386 at
  N>=1500 (recursive parse); SECTOR not explained by large N (open). wcc386 9.5 can hang: always use timeouts.

## Ken-Build ancestry (kengame worker 2026-11-26, docs/ANCESTRY.md)
- PROVEN Ken's Build demo GAME.C shares code with LameDuke: inittimer, uninittimer, initlava (Jun-95 source, -oe=3000,
  exact) and movelava (Ken's text, 16 drops / base colour 64, STRICT 1,248 B). Original names adopted (initlava,
  movelava, lava* globals); MENUES _BSS still VERIFIED under Ken's names.
- PROVEN movelava matches only with Ken's 1994 copybuf pragma without a modify clause (`parm [esi][edi][ecx]`); Watcom
  itself unrolls Ken's loops (hand-unrolled source was the wrong model).
- STRONG semantic correspondences: update_timer_input = Ken's keytimerstuff, domovethings_multiplayer = getpackets,
  network_move_loop = sync, record/playback/wait_for_players/findplayer/operatesectors case 9 follow Ken's code.
  Naming hazard: our svel/player_svel is Ken's vel/syncvel (forward) and our vel is Ken's svel (strafe).

## All game units are -oe=3000 natural-call sources (oeunits worker 2026-11-26)
- PROVEN MENUES (N>=90), PLAYER (>=485), SECTOR (>=~1520), GAMEDEF (>=~1330), SOUNDS (any) and ACTORS (>=101, one comma
  steer for addweapon) compile with natural calls under -oe=3000 to the same STRICT sets and verified data as their
  macro/alias versions: draw_and_probe, draw_menu_slider, shoot, update_air, spawn_pistol_tracer, operatesectors,
  checkhitswitch, getlabel, keyword, parse/move_actor, ifhitbyweapon, spawn_blood, guts are inline sources. One N fits
  the whole game; operate_helpers.h and the #pragma aux aliases are gone.
- PROVEN recursive functions are inlined one level deep and the copy keeps its self-calls (oracle self-calls:
  operatesectors 2, parse 40, parsecommand 5) — the earlier "recursive callees are not inlined" was wrong.

## Engine compiled without inlining (engfix3 2026-11-26)
- PROVEN the engine profile (-4r -s -or, no -oe) reproduces all 181 intra-engine calls; every tested -oe threshold loses
  STRICT matches (consistent with Ken's makefiles). Engine plateau at 81/103: remaining NEAR are allocation ties
  (scansector 7 B, hline 8 B confirmed by cg95), rotatesprite 35 bytes long (three shared returns), and the VESA buffer
  binding (naming it moves it in BSS). 8 dated snapshots and 30 historical objects compared; no closer December form.

## Allocator lever: use counts (plactfix 2026-11-26)
- PROVEN reducing the number of uses of a variable lowers its allocator "savings", changing the register assignment order
  and flipping ties elsewhere in the function (movemusic STRICT via playmusic(ps[screenpeek].music) instead of a song
  local; moveplayers followed). Prefer use-count changes over expression respellings for allocation ties.
- klabs is declared only by #pragma aux (no prototype), as in Ken's PRAGMAS.H -> oracle's `mov ax; cwde`.

## One set of records (unify worker 2026-11-26)
- The player record, map v5 types and ud/menu/sound records are defined once in src/game/records.h (struct tags and
  macros only, no symbol slots); all game units and GLOBAL include it; objects identical to before (segment bytes,
  fixups, publics, externs). Field renames applied by byte offset (fixes swapped scuba_on/jetpack_on, fta/ftq, opos*).
- OPEN type conflict: sector wallptr/wallnum are unsigned in Ken's BUILD.H and for SECTOR, but GAME's moveeffectors
  needs signed loads (oracle mov+cwde); records.h keeps unsigned and moveeffectors has (short) casts (COMPILER_STEERED).
- OPEN prototypes still differ between units (spritesound, getincangle, getangle, spawn, myos, EGS ...): changing them
  changes code; the original may have had per-unit declarations or a shared header with different types.

## Watcom 10.0 GA _BSS rule differs from 9.5 (audfix worker 2026-11-26)
- PROVEN a calibrated 10.0 GA BSS order rule predicts 163/163 fixture symbols vs 38/163 for the 9.5 rule; with
  declaration reordering all 18 Apogee _BSS layouts verify (build/workers/audfix/calibrate_bss.py; docs/units/audfix.md).
- Audio library 330/375 STRICT; 32 CODE_BYTES_MATCH held by 18 terminal extents (module-last functions whose end is not
  a confirmed boundary) and 14 CONST references; remaining data mismatches are compiler padding.

## SECTOR/GAMEDEF NEAR round 2 (r2secgd, 2026-11-26; worker files build/workers/r2secgd/, integrated)
- STRONG GAMEDEF's `CON_NEXT()` is `break` with `return 0;` after the switch (Duke 1.5 parse style): reproduces the
  oracle's 40 separate parse self-calls in parse and execute (was 21) and the killit prologue; unaligned
  instructions parse 1759 -> 744, execute 1231 -> 788. Positional byte-diff counts rise because later bytes shift.
- HYPOTHESIS (local evidence PROVEN) GAMEDEF saw the 18 game functions it calls without prototypes (old-style
  `extern short EGS();`): the oracle computes every EGS argument in 32 bits without narrowing and loads `*insptr`
  as a long for shoot/spawn/addweapon. Declared only in gamedef.h; other units keep their prototypes (OPEN: what
  the other units saw).
- PROVEN blockers: activatewarpelevators, operateforcefields (exact code), checkhitceiling, transnum and move_actor
  are held only by a neighbour's jump table falling inside their extent (operatesectors, cheatkeys, checksectors,
  parsecommand, parse).
- NEGATIVE ~40 checkhitswitch toggle-loop forms: none folds the `wall` base into each access across the jump table
  (size now exact 1632, 10 instructions differ). parse allocation needs j's savings above x's (x 708, i 512, j 408);
  extra j assignments incl. the Duke IFP `j = 1` chain leave j at 408. checkhitceiling (AH vs BL zero register) and
  checkhitwall (savings tie t83/t249) retraced without a natural fix.

## GAME NEAR round 2 (r2game, 2026-11-26; worker files build/workers/r2game/, integrated)
- PROVEN movetransports STRICT by declaration order alone (`long p, k; short i, j, m, sect; long otherlotag,
  sectlotag;`); only 6 of 72 orders also keep its inlined copy in domovethings. Declaration order flips register
  ties that statement/operand respellings cannot reach (64 operand-order combinations compiled identically).
- PROVEN wait_for_players STRICT via operand order of getpackets' bit tests (tests 1, 2, 4 Ken's
  `(tempbuf[k>>3] & (N<<(k&7))) > 0`, test 3 mask-first). A callee's operand order can change its inline copies
  while its own code stays identical. Cost: waitforeverybody now +16 bytes (diffs 1057 -> 1158), sync 1604 -> 1685.
- closedemowrite STRICT is COMPILER_STEERED (marked in source): the level/volume if/else sits in `do{}while(0)`
  after `reccnt--`; without that block boundary the 9.5 scheduler splits `sub [reccnt],edx` (+16 bytes).
- STRONG playback's remaining difference is a register tie in the battery loop of the inlined displaystatus copy;
  levers found (domovethings `(long)i` temp savings, one conflict record in displaystatus) all live in shared
  callees that are STRICT on their own. gamenumber jumps into gametext's shared return path (STRICT only together).
- NEGATIVE 20 meter-loop spellings, cameratext shade types, next_text_shade forms, 54 gametext variants, EGS
  statement orders/parameter types, newgame, checkcommandline, lotsofglass, declaration-order searches over 13 more
  functions.

## PLAYER/ACTORS/MENUES NEAR round 2 (r2pam, 2026-11-26; worker files build/workers/r2pam/, integrated)
- PROVEN getinput STRICT: menu-path zeroing as three statements `syncvel[snum] = 0; syncsvel[snum] = 0;
  syncangvel[snum] = 0;` (not a chained assignment). Natural source.
- PROVEN initmenus STRICT: only the field order inside each menu block changed (blocks contiguous, same order,
  each starts with x, y); order found by hill-climbing against the oracle's store sequence.
- PROVEN animateknuckles STRICT: displayweapon spelled with `short frame`, `weapon_xoffset = xvel +
  ((6 - weapon_pos) << 4) + 12` and if/else for `cw` puts the knee jump-table case labels at the oracle offsets
  (table relocation targets verified). displayweapon itself stays NEAR (frame in DH, stack homes).
- menutext uses `long s` and a new local order (COMPILER_STEERED, marked in source); menus diffs 3442 -> 2566.
- NEGATIVE draw_and_probe (`unsigned char s` gives exact code, but menus' tables sit in its extent and menutext
  then swaps a stack slot; 1,296 variants), hitradius (>1,000 spellings; tie on the i*38 multiply's savings),
  movefta (~120 combinations), ms (best loop form 131 -> 105 unmatched, not adopted). guts holds moveactors' table
  (targets 15 KB into moveactors): STRICT only together with moveactors.

## Engine round 4 (engfix4, Codex gpt-6.2-sol, 2026-11-26; build/workers/engfix4/NOTES.md, integrated)
- PROVEN calcwalls and hline STRICT by local declaration order alone (stack homes; confirmed with cg95 traces).
- PROVEN rotatesprite's December extent is 1,497 bytes: saving the column origins (startu/startv) and a separate
  short tile height restore it, which makes setverts and setredbluepalette STRICT (their shared-epilogue
  displacements depended on it). rotatesprite itself stays NEAR (1,225 bytes differ); setvesamode is
  CODE_BYTES_MATCH (eight unverified _BSS references). All three edits are marked COMPILER_STEERED in ENGINE.C.
- NEGATIVE scansector: declaration-order and use-count trials leave the 7 register bytes.

## Block-level diagnostic (tools/blockdiff.py, 2026-11-26; ported from jng_recon blockdiff + blupi_recon residues)
- Diagnostic only. Self-test: all 558 STRICT functions score 1.0 on every measure (tests/test_blockdiff.py).
  Porting needed three LameDuke rules: per-function code base for references outside the extent (shared tails,
  a neighbour's jump table), MASM relocated intra-segment jumps, and Watcom filler instructions ending in zero
  bytes (only whole zero instructions before an unconfirmed boundary are linker padding).
- First census (108 non-STRICT functions, 228,420 B): 7.1% code written with only order/registers left, 2.4% only
  register/stack homes, 14.0% nearly written (fuzzy >= 0.95), 76.3% statements missing or wrong. Byte-weighted
  block content 0.53, fuzzy 0.83. Skeletons (calls/globals/constants) are 0.92-1.00 even for the large functions,
  so their statements are mostly present but differ in shape: e.g. checksectors' frame is 0xa8 in the oracle vs
  0x78 ours (missing locals) and it uses sector+0xc/+0x1f where ours uses +0x20/+0x23.
- The extra "loose" measure (register and stack-slot operands unified) moves only 5 KB out of "statements
  missing": Watcom spill choices are not the main residue.

## PLAYER round 3 (r3player, 2026-11-26; build/workers/r3player/NOTES.md, integrated)
- STRONG `EGS` was called without a prototype in the original game units: GAMEDEF (round 2) and PLAYER both need
  `extern short EGS();` (32-bit argument building without narrowing); in PLAYER alone it cut processinput's
  structural residue 5892 -> 2077 B. Declared old-style in gamedef.h and player.h only (OPEN: the other units).
- PROVEN by compare tree and case-body order: processinput's fire switch (`case RPG: kp=1; break; case CLUB: ...
  case HEAVYHBOMB: case HANDREMOTE: kp=1; break;`) and the knee dispatch `switch(frame){case 255: case 3: case 4:}`.
- shoot: `switch(atwith & 0xffff)`, zvel from `sprite[ps[p].i]`, APLAYER branch via `sprite[i].yvel`, SHOTSPARK1
  `sprite[EGS(...)].yvel = hitwall`, address-taken `long j` shared with CLUB; declaration order COMPILER_STEERED
  (hill climb) restores size 1728 and keeps animateknuckles STRICT.
- Progress (tools/blockdiff.py): processinput content 0.395 -> 0.586, fuzzy 0.722 -> 0.894, skeleton 0.997;
  shoot content 0.522 -> 0.846. No STRICT change (4/8). Remaining processinput residue is allocation (frame 0x110
  vs 0x100, tics2 spilled).
- NEGATIVE jetpack `++` forms and ksqrt operand order compile identically; nothing reproduces the oracle's sz store
  hoisted before the CLUB call (8 B, OPEN); spawn_pistol_tracer `long sa` + unsigned char weapon_pos is closer but
  32 B short and costs animateknuckles; displayweapon with `myos()` unprototyped reaches exact size but
  `unsigned char frame` loses animateknuckles' case offsets.

## SECTOR round 3 (r3sector, 2026-11-26; build/workers/r3sector/NOTES.md, integrated)
- STRONG SECTOR saw `nextsectorneighborz` returning long/int (ENGINE.C defines it with no return type): the oracle
  indexes sector[] with the raw return value without sign extension. sector.h now declares it long (OPEN: game.c,
  premap.c, global.c still declare short, untested).
- PROVEN-by-layout source forms in operatesectors: top-tested `while` sprite-list loops (also in the inlined
  activatewarpelevators copies; the standalone function stays STRICT), symmetric lift/elevator labels with case
  16/18 first, case 15 `if (p >= 0)`, Ken's direct `sector[nextsectorneighborz(...)].floorz` in cases 20-22/29,
  case 23 `j = -1 ... break`; operatesectors size now exactly 10288 and its case order matches (cases 17, 19, 23,
  25 byte-exact). Local declaration order `short a; long j, middle, daz; short p, i;` COMPILER_STEERED (marked).
- cheatkeys steroids/shield/holoduke/jetpack/holster as if/else or flag-reuse forms; checksectors use-key as two
  byte tests and the Duke 1.5 CLEARCAMERAS shape (`long i, w`): its 833-byte head has the oracle's size.
- Progress (tools/blockdiff.py content/layout): operatesectors .516 -> .646 / .670 -> .849, operateactivators
  .265 -> .424 / .667 -> .911, cheatkeys .324 -> .668 / .818 -> .970, checksectors .245 -> .305 / .663 -> .952.
  No STRICT change (9/18). Remaining residue is allocation (frames 0x1bc vs 0x1c4, 0x88 vs 0xa8; more spills).
- NEGATIVE `gm |= MODE_MENU` (oracle load/or/store only with `gm = gm | MODE_MENU`, OPEN), removing the extra
  block braces, case 21 animategoal reload forms, case 9 loop-variable splits and 150 local orders, case 15
  `p != -1`/`p < 0`, a `sectortype *sptr` pointer in case 29.

## PREMAP/GAME round 3 (r3premap, 2026-11-27; build/workers/r3premap/NOTES.md, integrated)
- PROVEN resetplayer STRICT (336 B) by swapping the targets of equal-valued stores (driven by match.py off32
  problem lines; instruction-shape metrics hide such target differences).
- STRONG spawn follows the Duke 1.5 text: Duke local names (one `s` for picnum, loop counters and the `&s`
  updatesector hint), parameter `j` reused as a loop variable, Duke-order chained xrepeat/yrepeat, Duke 1.5
  sector-effector code for lotags 13/20/3/4/8/9/11 (lotag 9 `labs`). Frame now 0x18 as in the oracle; byte diffs
  6402 -> 4834; size 8752 vs 8736 (allocation side effects). Declaration order from an exhaustive 720-order search
  (COMPILER_STEERED order, Duke names and types).
- prelevel: wall-loop `switch (wall[i].overpicnum)` (cases 1184, 1185/1186), `case 406:` falling into `case 405:`;
  inlined resetprestat in Duke 1.5 statement order (standalone resetprestat has 0 byte diffs but stays NEAR:
  prelevel's jump table lies in its extent).
- STRONG PREMAP and GAME call `strget` without a prototype (`extern int strget();` in premap.c; the prototype was
  dropped from game.h): getscorename passes plain int arguments. strget is `unsigned char strget(short x, short y,
  unsigned char shade, unsigned char dalen)` without local copies; main's frame now 0x28 as in the oracle.
- PROVEN (cg95 trace) Watcom 9.5 moves an address-taken local to the end of the stack-slot assignment list, so the
  address-taken variable decides the frame layout (spawn: `s` stays at [esp+0x14] in every declaration order).
- NEGATIVE dropping -oa gives spawn the exact size but breaks enterlevel (STRICT): -oa is right for PREMAP;
  strget return respellings; newgame orders; main's early constant load; moveeffectors frame 0x58 vs 0x60 (two
  stack slots missing, cause not found).

## ROTT 1.0 shareware and the Apogee Sound System lineage (assrott, Codex, 2026-11-27; build/workers/assrott/NOTES.md)
- PROVEN ROTT 1.0 shareware (1994-12-21; ROTT1BBS._1-._3 = one PKSFX 2.04g stream, 17 ZIP entries) ROTT.EXE is
  DOS/4GW Professional + LE (module `rott`, 4 objects, entry 1:5bf58); its banner names **Apogee Sound System
  1.01**; runtime STRONG Watcom 10.0a family (10.0a-only members cstrt386, grownear, scnf, timeutil present).
- PROVEN LameDuke and ROTT share the early ASS lineage but not the same build: D3D has no version banner, a
  different PAS message ("No sound playing."), ULTRASND.INI vs GUSMIDI.INI, a different OPL port-delay loop and
  timbre data; ROTT has stack-checked GUSMIDI. Of LameDuke's 27 non-STRICT ASS functions, 6 are byte+target equal
  in ROTT (ADLIBFX/DMA/PCFX/SS_ErrorString, TS_SetTimerToMaxTaskRate, TS_AddTask), 3 equal bodies with different
  referenced content, 18 differ. Equal December bodies prove the remaining differences of those 6 are ours.
- PROVEN TS_AddTask STRICT (156 B): read `ptr->prev` directly (no local copy), as both December binaries do; the
  later 1.04 source uses a circular list. ASS now 349/375.
- PROVEN LameDuke SETUP.EXE shares D3D's DOS/4GW stub and Watcom 9.5 GA runtime fingerprint; SETUP.DAT holds six
  option bytes and 17 scancodes; no ASS code in SETUP.
- NEGATIVE 50 TS_SetTimerToMaxTaskRate variants (best 7 bytes), 33 AL_SetVoiceTimbre forms, released and al_104
  donor rebuilds: no further STRICT gains; ErrorString bodies stay held by whole-CONST verification.

## Duke3D extras/ as a donor corpus (extras worker, 2026-11-27; build/workers/extras/NOTES.md)
- PROVEN 89 of the 110 extras files duplicate Duke 1.5 / the 1.3D tree once line endings and the 3DR banner are
  ignored; none duplicates Ken's archives. GAME/MENUES/SOUNDS/PREMAP/GAMEDEF/GLOBAL/DUKE3D.H are 1996 Duke 1.5
  snapshots (some later than the release: VERSION "1.5", BYTEVERSION 28/117, UK and TEN builds). CAVE.C and GRUNT.C
  predate LameDuke but target a different engine; GAMEN.C/PLAYERN.C are Ken's 1996 patch notes; BB.C, FRAGMENT.C,
  SE40.C, CD.C, TESTDRV.C are 1996-97. Only GAMERBG.H (1995: MAXANIMATES 512, recsync[16384], hittype[1536]) and the
  later RGAME.H sit between LameDuke and 1.3D.
- NEGATIVE no extras implementation is closer than our source or Duke 1.5 for any of the 56 non-STRICT game
  functions (best extras donor 0.517; same-name donors within +-0.008 of Duke 1.5). WATER.C DoWater scores 0.312
  vs draw_water_distortion, whose code is already identical: its last 0x44 bytes are displayrooms' jump tables, so
  it becomes STRICT only together with displayrooms (NEAR_CENSUS "ALLOCATION-ONLY" for it is stale).
- STRONG names from GAMERBG.H (same offsets/types/shapes; applied, byte-neutral): player fields current_weapon,
  ammo_amount, ammo_bat_amount, ammo_h_amount, ammo_r_amount, fire_flag, max_squashable, squashable; the 19-byte
  SOUNDOWNER voice record (i, voice, num) and SAMPLE ptr; data clubframes[6][20], bonus_xy[3][16], diz[500],
  user_quote, user_quote_time (formerly knee_frames, bonus_marks, file_id_record, msgtext, msgtime).
  HYPOTHESIS: temp_data as the pre-struct name of hittype[][5]; MusicPtr, typebuf/typebuflen; player_orig fields.
- PROVEN (asset worker, 2026-11-27) the CON compiler's `sound` lookup scans 38 names and then tests `i == 38`
  (1:2f12e cmp ebx,0x98 / 1:2f136 cmp ecx,0x26): the 39th table entry (rpg.voc) can never be named from CON and
  the "Sound not found" error is reachable. src/game/gamedef.c tested `i == 39` (dead error path); fixed.

## Donor acquisition pass (acquire worker, 2026-11-27; build/workers/acquire/NOTES.md, config/references.json)
- 27 public artefacts acquired (89.6 MB; external/dl/<id>/, external/x/<id>/): 13 of Ken's advsys.net archives
  (BUILD1, builgrid, buildsec, Build Apr-1994, CONVMAPS, GROUDRAW, CAVE, POLYTEX, BUILDC, kenbuild2000, 2DRAW,
  CUBES5, CHESS), jonof.id.au L7P/Witchaven/TekWar demos, Les Bird's Witchaven/TekWar/C8GW sources, NY00123's SW
  source + Build addendum, 3D Realms FTP mirror items (Duke3D 0.99 beta, 1.0 shareware, ROTT 1.3, SW 2005 source,
  misc/lameduke), Wayback copies of the prototype-CON pages.
- STRONG jonof-l7pdemo 75M.EXE / 75T.EXE / 75MAIN.EXE are Build (map v4) binaries dated 1994-09-22 with the same
  Watcom 1988-1993 runtime banner as D3D.EXE; ~6% of D3D.EXE code windows occur verbatim in 75M.EXE (engine and
  runtime regions) vs 1.2% for Ken's Apr-1994 GAME.EXE: the closest-dated engine binary available.
- STRONG Watcom OMF dependency comments (COMENT class 0xE9) store a 32-bit time_t under Watcom 9.5 (checked on our
  own output) but DOS date/time words under 10.0/10.0a (with C:\WATCOM\H headers dated 1994-05-31 / 1994-09-01).
  This dates Ken's switch from 9.5 to 10.0a to 1995-07-10..16 (Witchaven ENGINE.OBJ 1995-07-16), consistent with a
  9.5 engine in LameDuke; Capstone's own objects are 10.0a. Decoder: tools/omfdeps.py.
- Duplicates (by sha256): Build Apr-1994 = kenbuild BUIL0408 (26/31; originals lack the 4-line licence header),
  kenbuild2000 = dukenukem3d SRC/UTIL (41/45), SW addendum = buildsrc-20190520 (108 files) + audiolib originals (7);
  new there: AUDIO_WF.LIB 1995-11-13, USRHOOKS.C 1994-09-04, 1996 BUILD.TXT, 1997 ENGINE.C.
- Leads only (not acquired): SW 1995-01-15/02-10 prototypes (free Steam app 238070, needs a Steam client), L7P full
  builds 1994-09-26/11-01 (not public), decrypted SW betas (circumvention), unreleased Duke builds (Apr/May 1994,
  "LamerDuke", 1995-05-09 beta), Hendricks266's 2017 source finds (topic 7266 #610; unreleased).

## Engine round 5 with the September-1994 binary donors (engfix5, Codex, 2026-11-27; build/workers/engfix5/NOTES.md)
- No STRICT change (85/103 engine). L7P 75M.EXE (1994-09-22, same Watcom 9.5) clarified setview's span and stride
  calculation: 123 -> 9 differing bytes, block content 0.62 -> 0.94 (rest: register choice, load/store order).
  July-1994 BUIL0719 evidence for hitscan's sprite deltas and height: content 0.727 -> 0.765. overwritesprite and
  raytrace fewer differing bytes. Four edits marked COMPILER_STEERED in ENGINE.C.
- NEGATIVE draw2dscreen and rotatesprite donor-guided trials changed extents and regressed STRICT neighbours.
- NEGATIVE setview allocator pass (engfix5 follow-up): cg95 scores the stride temporary 6 for EAX vs 4 for EDX
  (oracle uses EDX); reusing `j` fixes the six stride bytes but adds nine in the span; restoring the viewoffset
  store before the xdimen load adds other encoding differences. 454 full-unit builds over declaration order, use
  counts, statement order and address-taking: no net gain (still 9 bytes). build/workers/engfix5/sv/NOTES.md.

## Jump-table attribution in the verifier (2026-11-27)
- PROVEN Watcom 9.5 (and 10.0 in the ASS units) emits a function's switch tables, followed by alignment padding,
  immediately BEFORE the function's entry label, i.e. inside the previous public's span. 36 such tables exist
  in the candidate objects (`jump_table_tail` census), 9 of them at the end of non-STRICT functions whose own code
  was already exact (draw_water_distortion, operateforcefields, activatewarpelevators, checkhitceiling, transnum,
  draw_and_probe, FX_ErrorString/FX_SetupCard, GUSWAVE_AllocVoice, MUSIC_ErrorString).
- tools/match.py now verifies such a table as part of the function that owns it: a run of >= 2 absolute 4-byte
  fixups into the same segment whose targets all lie in the next public's span, followed only by < 16 bytes
  without fixups. The shortened extent's end is accepted ONLY when the oracle confirms the table (next public
  bound exactly after it; every entry an oracle fixup into the next function's span); the generic boundary
  sources do not apply there, because a funcmap "start" can be the table itself. The table bytes are compared as
  the owner's leading bytes. Controls: tests/test_match.py::JumpTableControls (positive draw_water_distortion;
  negatives: table relocations removed, owner bound elsewhere). No check was removed; extents only move bytes
  to the function the relocations prove they belong to. tools/blockdiff.py follows the same extents.
- Result: +7 STRICT whose code was already exact (draw_water_distortion, operateforcefields, activatewarpelevators,
  transnum, resetprestat, guts, FX_ErrorString); tables of NEAR owners now count against their owners.

## Watcom 10.0 on the DOS host (assdos, Codex, 2026-11-27; build/workers/assdos/NOTES.md)
- PROVEN Watcom 10.0 GA's DOS compiler is the BINB/wcc386.exe LX image run through BIN/w32run.exe (DOS/4GW
  rejects it: DOSCALLS.234); it runs under the pinned DOSBox-X. tools/doshost.py now hosts wc100 too and
  tools/wcc.py uses the DOS host for both wc95b and wc100 by default (LAMEDUKE_HOST=nt for fast exploration).
- PROVEN with the guest paths recorded in Apogee's 1.04 objects (C:\PRG\AUDIO\source, C:\LN\WATCOM), DMA's
  100-byte CONST verifies and DMA_ErrorString becomes STRICT (+88 B; ASS 351/375); no status regressed.
- STRONG the remaining CONST padding in nine ASS pools is earlier include text in Watcom 10.0's reused source-read
  buffer (MUSIC = ll_man.h at offset 2141, ADLIBFX = adlibfx.h, PCFX/SNDSRC = dpmi.h, BLASTER = _blaster.h,
  PAS16 = _pas16.h), plus guest-memory reuse across compiler runs in one session (compile order changes GUSMIDI,
  FX_MAN, GUSWAVE). OPEN: the 1994 header revisions, paths and make order that produced those bytes.
- NEGATIVE memory size, TZ/TMP, header relocation, basenames, -zld, LF line endings, make order and reverse order:
  no further CONST pool verified.

## MULTI is compiled with -5r (r4multi, Codex, 2026-11-27; build/workers/r4multi/NOTES.md)
- PROVEN Ken's MULTI object was built with `-5r -s` (Pentium register calling), not `-4r -s`: the unchanged source
  at -5r reproduces netinitconnection (515 B) with the oracle's signed-short promotion (dword load two bytes
  early + `sar reg,16`, relocation to `_numplayers-2`), which also appears in getpacket, sendpacket and
  processreservedmessage; every -4r STRICT function still passes. -3r/-ot/-os/-or/-oa/-ol/-ox/-od/-oi and packing
  do not produce it; -5r on ENGINE drops to 27/103 (engine stays -4r). Profile `multi` changed to -5r -s.
- processreservedmessage STRICT (649 B) with Ken's later `j = 1; while (j < length)` loop once sendpacket and
  getpacket recover their exact extents (1,054 / 1,424 B). MULTI 18 -> 20/25. sendpacket content .354 -> .943.
  Two steered forms marked (sequence step `+ 254` on a byte; ring mask on the short index).
- Engine: neartag 222 -> 202 and makepalookup 306 -> 194 differing bytes (no STRICT change).

## GAME round 4 (r4game, 2026-11-27; build/workers/r4game/NOTES.md, integrated)
- No STRICT change (31/49); main 3477 -> 1891 and sync 1685 -> 1319 differing bytes (sizes exact); NEAR total
  24,685 -> 22,733. Ordinary C read off the oracle's dataflow (no steering):
  syncbits/osyncbits `unsigned long` in game.c (oracle shifts with shr; the other game units already declare them
  unsigned); sync master sends the four syncbits bytes as four `tempbuf[j++] = (... & 255)` statements (Ken's 1994
  form extended); no `synctics = 0;` after domovethings() (domovethings clears it); main `short i, j, k;`;
  `attract: if (warp_on == 0) playback(); else warp_on = 0;`.
- STRONG GAME calls getscorename without a prototype (`extern int getscorename();`; prototype removed from game.h):
  main tests EAX after the call while getscorename itself returns only `setne al` (same pattern as strget).
- Levers not adopted: gamenumber's swapped left/ac stack slots need one anonymous temp fewer (or one late);
  2 of 16 getpackets bit-test orders make waitforeverybody exact size but break wait_for_players (STRICT);
  `hudpulse += 128` helps update_menu_cursor/dobonus but costs cameratext/main.
- NEGATIVE ~870 gametext variants (local names have no effect), update_menu_cursor shade forms, lotsofglass/EGS
  store orders, displayrooms ang/look_ang operand swap (identical code), main declaration-order search (timed out).

## Other game units round 4 (r4units, 2026-11-27; build/workers/r4units/NOTES.md, integrated)
- PROVEN (mini test) Watcom 9.5 merges an identical block into one that falls through to the same join point, but
  does not merge two identical blocks when neither falls through. So the oracle's separate copies of identical
  bodies mean each ends in its own jump: menus pages each end with their own `update_menu_cursor(614); break;`
  (content .945 -> .962, layout 1.000; size 5232 vs oracle 5216).
- moveexplosions: prologue order (nexti, sect, `t = hittype[i]`, findplayer), final `else if (++t[0] == 2)
  deletesprite(i); else ...` restoring the oracle's 10 separate deletesprite calls, `if (xvel > 0)` first in
  1201-1205, split WATERSPLASH deletes, `yrepeat = xrepeat = v`, if/else for ternaries, else-if debris chain,
  SHELL `hittype[i][1]`, gib `t[2]`/`t[4]`: content .475 -> .677, fuzzy .945, skeleton .998.
- movestandables: loop-level `t` with `t[n]` and `ps[p].field` (no pointer): content .579 -> .729, skeleton 1.000.
- checkhitswitch case 405/406 sprite loops top-tested `while` (standalone identical; the inlined copy in
  checksectors gains the oracle's shape: checksectors layout .952 -> .976).
- Not adopted (loses a STRICT function): `unsigned char s` in menutext + new menus makes draw_and_probe STRICT
  (+896 B) but menutext NEAR (-352 B; start_x/ac stack-home swap unresolved after 300+ variants).
- NEGATIVE spritesound, movefta, newgame, checkhitceiling store orders, checkhitwall, 26 checkhitswitch wall-toggle
  forms, old-style prototypes in sector.h (no code change), moveweapons/render_mirror forms, for->while over all
  loops; moveactors `t`/`ps[p]` raises content to .58 but shrinks it so that guts' table target moves (reverted).
- Next lead: apply the identical-block rule to processinput (-184 B), checksectors (-224 B), prelevel (-32 B).

## ACTORS/PREMAP round 5 (r5cx-actors, Codex, 2026-11-27; build/workers/r5cx-actors/NOTES.md, integrated)
- No STRICT change (ACTORS 11/18, PREMAP 6/12; guts stays STRICT). prelevel now has the oracle's exact size (9,872)
  after the tag-store postfix increment restores tile 570's separate tail (identical-block rule).
- moveactors: separate OOZ deletes, slime and flicker loop exits and the bomb crush path restored (content .401 ->
  .409, fuzzy .626 -> .648; size 12,496 vs oracle 12,320). An exploratory loop-level `t[n]`/`ps[p]` candidate reaches
  .751/.929 with guts STRICT, but its last case target is still 32 B early (not adopted yet).
- moveweapons: full-width distance output and explicit short copy: byte diffs 1,036 -> 815 (size 1,440 vs 1,456).
- NEGATIVE movefta (cansee register conflict traced; baseline best), spawn exact-size pointer variant (worse).

## PLAYER and SECTOR round 5 (r5cx-player, r5cx-sector, Codex, 2026-11-27; integrated)
- No STRICT change (PLAYER 4/8 with animateknuckles; SECTOR 11/18). The identical-block hypothesis does not explain
  processinput's 184 missing bytes (the suspected blocks exist in both builds).
- processinput: earlier `tics2` computation and declaration order recover one spill slot (frame 0x100 -> 0x104 vs
  oracle 0x110); size 15,508 -> 15,581 of 15,692; content .586 -> .603, fuzzy .894 -> .906.
- STRONG PLAYER calls myos without a prototype (`extern void myos();` in player.h; promoted arguments), as with
  EGS/strget/getscorename: displayweapon reaches the oracle's exact size (4,180) and keeps animateknuckles STRICT;
  content .222 -> .244, fuzzy .657 -> .713. spawn_pistol_tracer fuzzy .580 -> .789.
- PROVEN cheatkeys calls qsetmode640350 (1:199eb -> 1:43f42): the call was missing from our source. cheatkeys and
  checksectors also regain the lift-success tail; checksectors size 10,976 -> 11,072 of 11,184, cheatkeys 7,584 vs
  7,568 (frame 0x64 vs 0x60). checkhitceiling 16 bytes from STRICT (boundary depends on checksectors' table).
- SECTOR follow-up (r5cx-sector-2): checkhitceiling STRICT (+856 B) after reordering its palette writes and
  wall-bound loads; its table-confirmed boundary now holds against checksectors' new extent. checksectors,
  operateactivators and cheatkeys reach the oracle's exact sizes (11,184 / 4,912 / 7,568); checksectors content .294
  -> .425 (frame 0xb4 vs oracle 0xa8); operatesectors reaches the oracle frame 0x1c4. Door layout recovered with a
  dedicated case 9 loop index, the case 21 ceiling-height operand and case 22 midpoint initialisation. SECTOR 12/18.

## Engine round 6 (r6cx-engine, Codex, 2026-11-27; build/workers/r6cx-engine/NOTES.md, integrated)
- No STRICT change (85/103). getcliplines recovers indexed sprite loads (content .253 -> .276); getzrange recovers
  the shared face- and wall-sprite height checks (.170 -> .257); grouvline projection and declaration order (fuzzy
  .884 -> .899); hitscan local order (-3 bytes). drawsprite/draw2dscreen trials that scored higher changed extents
  and regressed STRICT neighbours (not adopted).
- ACTORS/PREMAP follow-up (r5cx-actors-2): moveactors rewritten with loop-level `t[n]`/`ps[p].field`, the historical
  inline `ksgn` and restored late cases reaches the oracle's exact size (12,320) with content .791 / fuzzy .941 (was
  .41/.65); its last case target lands exactly at +12,127 so guts stays STRICT; seven other targets within 3 B; frame
  0x4c vs 0x48. STRONG ACTORS saw `getangle` returning int (ENGINE.C defines it without a return type) and
  getincangle without a prototype (actors.h). spawn reaches the oracle's size (8,736) via the lotag-23 index and
  repeat-store order (sprite-index stack slot restored).

## GAMEDEF round 6 (r6cx-gamedef, Codex, 2026-11-27; build/workers/r6cx-gamedef/NOTES.md, integrated)
- No STRICT change (10/14; CONST/_DATA/_BSS verified). With jump tables assigned to their dispatchers: parse content
  .322 -> .670 (fuzzy .962; 6,608 vs 6,580 B), execute .346 -> .798 (7,376 vs 7,424 B), parsecommand .719 -> .830,
  move_actor 1.000/1.000 (all 31 block shapes; player index and movement flags in swapped stack slots, 768 vs 776 B).
  Restored hit-result branches and the shoot/sound arguments in execute; reusing `j` for visibility restores parse's
  entry allocation.
- NEGATIVE explicit inventory tails and movement declaration variants.

## PLAYER follow-up (r5cx-player-2, Codex, 2026-11-27; build/workers/r5cx-player/followup/NOTES.md, integrated)
- PROVEN shoot STRICT (+1,728 B) after saving the pre-decrement z and ordering the scoped locals; player.h declares
  `EGS` returning int (implicit-int call of an unprototyped function; GAME's definition returns short). PLAYER 5/8.
- processinput regains the bullet-frame reload and separate air-reset stores: 15,649 of 15,692 B, content .603 ->
  .661, fuzzy .906 -> .945 (frame 0x104 vs 0x110). spawn_pistol_tracer content .035 -> .138, fuzzy .789 -> .910.
- PLAYER follow-up 2 (r5cx-player-3): processinput jump-counter load moved (one oracle reference recovered; 15,656 of
  15,692 B, content .665); spawn_pistol_tracer angle bias applied last restores its frame 0x20 (fuzzy .934); two
  missing findplayer homes located in processinput (OPEN). NEGATIVE short player index (breaks shoot), direct pistol
  field accesses, RPG/HANDREMOTE branch rewrites in displayweapon.
- SECTOR follow-up 2 (r5cx-sector-3): checkhitswitch 1,070 -> 6 differing bytes (content/fuzzy 1.000, all fixups
  verified; keeping the wall displacement live across the jump table leaves three register choices); checkhitwall
  87 -> 78 bytes (declaration order restores the sign extension after the second EGS call); cheatkeys frame 0x64 vs
  0x60, content .624; checksectors frame 0xb0 vs 0xa8 (content .411; the new inlining merges one lift tail).

## GAMEDEF follow-up (r6cx-gamedef-2, Codex, 2026-11-27; build/workers/r7cx-gamedef/NOTES.md, integrated)
- PROVEN move_actor STRICT (776 B): two byte tests and a function-scope hover temporary resolve the swapped movement
  slots (6 B), the hover ADD (1 B) and alignment (1 B). GAMEDEF 11/14.
- parse reaches the oracle size 6,580 (differing bytes 4,492 -> 1,459; content .699, fuzzy .971); execute 7,422 of
  7,424 (content .863, fuzzy .971); parsecommand 5,508 vs 5,476 (content .842).
- NEGATIVE volatile flags and address aliases for move_actor; shared-return variants for getincangle break its
  STRICT match.

## ACTORS/PREMAP follow-up 3 (r5cx-actors-3, Codex, 2026-11-27; build/workers/r5cx-actors/NOTES.md, integrated)
- PROVEN movefta STRICT (416 B) through direct height subtraction and a 32-bit `findplayer` declaration in actors.h
  (SECTOR defines it returning short: another implicit-int-style call; it lengthens NEAR moveexplosions by 32 B).
  ACTORS 12/18, guts STRICT.
- moveweapons reaches the oracle size (1,456) with all 66 blocks and fixups equal (content/fuzzy 1/1); 17 bytes of
  stack-slot displacements left. moveactors frame 0x4c -> 0x48 (oracle), four case targets exact (deltas +1,+1,-3,-3),
  byte differences 7,949 -> 6,141. spawn: repeat-store and declaration order, content .705 -> .789 (252/276 blocks).
- NEGATIVE the slime cstat temporary reproduces the byte load/OR/store in isolation but shifts case targets.
- SECTOR follow-up 3 (r5cx-sector-4): checkhitswitch 6 -> 2 differing bytes (content/fuzzy 1.000): a signed byte
  offset raises the priority of both inlined master-switch conversions and selects the oracle's EAX; case 405 still
  selects EAX where the oracle uses ECX. checkhitwall unchanged at 78 bytes (two glass arms match in isolation;
  combined they shift neighbouring code).
- GAMEDEF follow-up 2 (r6cx-gamedef-3): execute reaches the oracle size 7,424 (widened saved player index; byte diffs
  5,067 -> 4,560; content .863 -> .815); parsecommand tests the final music index before incrementing (3,063 -> 3,049
  bytes); parse unchanged (first register difference in the inlined ifrnd at 1:30900; cg95 cannot attribute it across
  the indirect dispatch). NEGATIVE animation temporaries, parser pointer locals, fill loops.
- ACTORS follow-up 4 (r5cx-actors-4): moveexplosions reaches the oracle size 3,984 (ssp call; distance slot reused
  for the FLAIR counter; content .663 -> .681, fuzzy .929 -> .961; frame 0x20 vs 0x1c); moveactors five exact case
  targets (deltas +1, +1, -2; frame 0x48 as oracle); spawn non-player casing offset first: 5,313 -> 4,646 differing
  bytes. moveweapons 17 bytes unresolved: cg95 shows the 32-bit distance sorting late and its 16-bit alias early.
- PROVEN (cg95, r5cx-sector-5) checkhitswitch's last 2 bytes: in case 405 the conversion t116 and the scale t117
  both have savings 564; the conversion is visited first and takes EAX on the tie, so the scale keeps EAX where the
  oracle uses ECX. Moving case 406's next-sprite load earlier reverses the visit order and fixes the 2 bytes but
  changes case 406 and loses checkhitceiling's table boundary. NEGATIVE signed offset, local scope, loop order.

## PLAYER on gpt-6.2-astra (r7astra-player, 2026-11-27; build/workers/r7astra-player/NOTES.md, integrated)
- No STRICT change (5/8; shoot, animateknuckles STRICT). processinput reaches the oracle frame 0x110 (was 0x104;
  15,664 of 15,692 B; content .665 -> .682) by staging the tracer aim adjustment; displayweapon reaches the oracle
  frame 0x60 (case-local pistol x, regrouped drawing expressions; content .244 -> .283). The oracle's raw y spill
  is unexplained.
- PROVEN (witness build) a function-scoped `short` in shoot gives the first and fifth inlined findplayer results
  distinct stack slots and reproduces both oracle offsets (0x94, 0x108) with the signed low-word reloads; that
  variant is 73 B short overall (not adopted; build/workers/r7astra-player/address_pi/).

## ACTORS/PREMAP on gpt-6.2-astra (r8astra-actors, 2026-11-27; build/workers/r8astra-actors/NOTES.md, integrated)
- PROVEN moveweapons STRICT (1,456 B; all 83 references verified) with a `short distance` local whose address is
  passed to findplayer's `long *` output: Watcom 9.5 gives the short a 4-byte slot at ESP+0xc, findplayer writes the
  whole slot and the projectile tests use the signed low word. Undefined in portable C (a 4-byte write into a 2-byte
  object), so it is documented in the source; 4-byte array/union variants keep all 17 differences. ACTORS 13/18.
- moveexplosions frame restored (FLAIR fallthrough, gib ssp calls): 2,590 -> 2,019 differing bytes, content .681 ->
  .777, fuzzy .984. prelevel recovers its 0x1c frame and 13/28 exact jump-table entries (was 3). spawn 4,646 -> 4,592.
  moveactors keeps five exact targets incl. +12,127; slime flag update still open.

## SECTOR on gpt-6.2-astra (r8astra-sector, 2026-11-27; build/workers/r8astra-sector/NOTES.md, integrated)
- No STRICT change (12/18). checksectors restores both missing lift calls with direct player-field arguments and a
  wide initiator (8,004 -> 7,542 differing bytes; fuzzy .825 -> .869; frame 0xb8 vs 0xa8). cheatkeys reuses the decoy
  index and reads the menu flag earlier: 5,230 -> 3,605 differing bytes at the exact extent. checkhitswitch still 2
  bytes (case 405 allocation tie), checkhitwall 78.

## GAMEDEF on gpt-6.2-astra (r8astra-gamedef, 2026-11-27; build/workers/r8astra-gamedef/NOTES.md, integrated)
- PROVEN parsecommand STRICT (5,476 B; all 495 references verified): direct lexer calls, separate read and fill
  indices, actor-header writes, and case-specific `break` / `return` exits. GAMEDEF 12/14.
- execute passes the widened actor index to deletesprite and move_actor (4,560 -> 4,519 differing bytes; content
  .815 -> .842); parse unchanged (first register divergence in the inlined IFRND at 1:30900).
- PLAYER astra follow-up (r7astra-player-2): processinput reaches the oracle's exact 15,692-byte extent with the 0x110
  frame, the raw x/y spills at 0x24/0x20 and both findplayer result slots at 0x94/0x108 (signed reloads); expanding
  the tracer angle at each use recovers the raw y spill. Content .682 -> .702; first difference +0x5b -> +0x1e9.
  displayweapon reproduces all 11 CLUB jump-table targets and all 23 myos sites (3,431 -> 2,994 differing bytes).
- ACTORS astra follow-up (r8astra-actors-2): moveactors uses the oracle's SHARK comparison order and score-addition
  spelling (6,066 -> 5,904 differing bytes; five exact case targets incl. +12,127). moveexplosions gains one exact
  arm-piece block (160/167; content .806): Watcom reproduces the oracle's `shl` by 0x40 from a shift whose count
  is outside the C-defined range (undefined in portable C; reproduced under the pinned compiler, noted in source).
  spawn local reorder helps both PREMAP functions. findplayer audit: of 15 emitted calls one reads the distance as
  a short, four use the full width, ten ignore it.
- GAMEDEF astra follow-up (r8astra-gamedef-2): parse 1,459 -> 495 differing bytes: a local-scope change gives all 40
  instructions of the inlined IFRND at 1:30900 the oracle's registers and order; two exits still branch one byte
  early because IFSTRENGTH is one byte short. execute 4,519 -> 2,948 differing bytes at the exact size: the low-byte
  GET_H test gives the inlined HOVER the oracle's five-byte `add eax, 0x2000`; its frame is still 4 bytes too large.

## GAME on gpt-6.2-astra (r9astra-game, 2026-11-27; build/workers/r9astra-game/NOTES.md, integrated)
- PROVEN five new STRICT (game_game 31 -> 36/49, +5,792 B): playback (screenpeek captured in a short before
  displayrooms restores the 16 missing bytes), getpackets (staged shift count of the fourth flag), cameratext (updated
  HUD pulse kept in a short), checkcommandline (`i` initialised before clearing the global flags), gametext (incoming
  x saved before narrowing the line origin).
- main 1,891 -> 449 differing bytes at the exact size; strget reaches 1,024 B; gamenumber regresses 6 -> 219 bytes
  in its inlined gametext copy (NEAR either way). moveeffectors recovers the 0x60 frame (content .649 -> .664; 80 B
  over its proven 14,016-byte body; the next 112 bytes are spawn's jump table).
- SECTOR astra follow-up (r8astra-sector-2): scoping the sector index and staging the key latch recover checksectors'
  exact frame 0xa8 with both lift paths (content .419 -> .603, fuzzy .912); storing the animation pointer before its
  goal recovers 20 operatesectors blocks (content .672 -> .786). checkhitswitch (2 B tie) and checkhitwall unchanged.
- PLAYER astra follow-up (r7astra-player-3): only the displayweapon change is adopted (separate HANDREMOTE reads feed
  one shared draw; 2,994 -> 2,949 differing bytes; all 23 draw calls and 11 CLUB targets verify). Alternatives kept in
  build/workers/r7astra-player/followup2/: a processinput candidate matching 388/422 blocks (.753/.960) that moves the
  fifth findplayer home, and a tracer prefix variant that moves both inlined findplayer homes.
