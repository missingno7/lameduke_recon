# Historical donor research (2026-11-27)

A second pass over historical sources and binaries that could help the matching decompilation. It distinguishes
material that was only **identified** from material that was **acquired and inspected**, and from material that
produced **measured matching gains**. Evidence classes as in `docs/FINDINGS.md`. Raw records:
`build/workers/acquire/inventory.json` (acquisition), `build/workers/extras/` (Duke3D extras),
`build/workers/assrott/` (ROTT / Apogee Sound System / SETUP.EXE), `build/binscan/` (binary donor scan).
Reference inventory: `config/references.json` (34 archives, leads).

## Infrastructure added

| Tool | Purpose |
|---|---|
| `tools/binscan/` | Binary donor corpus and scanner. Inventories every code artefact under `external/` (MZ, LE/LX, PE, NE, OMF objects and libraries, ZIP members), dedupes by sha256, censuses functions (OMF publics; recursive descent for linked executables), ranks donors for every LameDuke binding by mnemonic/operand n-grams, block content (tools/blockdiff), CFG shape, calls, constants and data, and records dated byte-identical copies (lineage). `python -B tools/binscan/binscan.py [--add-dir DIR]`, then `python -B tools/binscan/report.py` → `build/binscan/NOTES.md`, `build/binscan/out/{inventory,corpus,lineage,top_donors}.json`. New directories under `external/x` are picked up automatically. |
| `tools/blockdiff.py` | Block-level diagnostic of non-STRICT functions (content / loose / fuzzy / skeleton / layout / regs, `--worklist`, `--census`); used by binscan for block signals. |
| `tools/omfdeps.py` | Decodes Watcom OMF dependency comments (COMENT 0xE9): 32-bit time_t under Watcom 9.5, DOS date/time under 10.0/10.0a, which dates which compiler built an object. |
| `tools/xscan/` | Existing source-donor scan (compiles historical C with our compiler). |

Calibration of binscan: STRICT `getmousevalues` and `ksqrt` rank first at 1.000 against Ken's BUIL0801
ENGINE.OBJ (1994-08-01; ksqrt only relocation-aware equal); unrelated Watcom `_nmalloc_` scores 0.023 against
`getinput` and 0.0008 against `processinput`.

## Acquired and inspected

27 public artefacts (89.6 MB downloaded, 343 MB extracted) in new directories `external/dl/<id>/`,
`external/x/<id>/`; nothing downloaded was executed. Plus the Duke3D `extras/` directory (already on disk, never
scanned before) and ROTT 1.0 shareware (downloaded earlier, never unpacked).

| Artefact | Date | What it is | Value for LameDuke |
|---|---|---|---|
| jonof-l7pdemo (75M.EXE, 75T.EXE, 75MAIN.EXE) | 1994-09-22 | Legend of the Seven Paladins demo, Build map v4, Watcom 1988-1993 runtime banner as D3D.EXE | **HIGH**: closest-dated engine binaries; ~6% of D3D.EXE code windows verbatim; best donor for draw2dscreen (0.60), drawmaskwall (0.59), grouvline (0.55), rotatesprite |
| lesbird-whaven (Witchaven source) | 1995-03..07 | Ken's ENGINE.OBJ 1995-07-16, BUILD.OBJ, A.OBJ/CACHE1D.OBJ 1995-06-15 with symbols; BUILD.H and BUILD.TXT to 1995-07-20; Capstone GAME.OBJ (GAME.C 1995-03-20) | MEDIUM-HIGH: new dated Ken objects; compiler dating (Watcom 10.0a from July 1995) |
| ROTT 1.0 shareware (ROTT.EXE) | 1994-12-21 | Apogee Sound System 1.01, Watcom 10.0a-family runtime | **gain**: TS_AddTask STRICT; 6 of 27 non-STRICT ASS bodies equal |
| Duke3D extras/ (110 files) | 1993-1997 | 89 duplicates of Duke 1.5/1.3D; GAMERBG.H (1995), RGAME.H; CAVE.C/GRUNT.C (pre-LameDuke, other engine); 1996 GAME/PLAYER snapshots | names (GAMERBG.H) applied; no code donors closer than Duke 1.5 |
| idtech-3dduke99 | 1996-01-05 | Duke3D 0.99 beta (from the 3D Realms FTP mirror; originally leaked) | best (weak, 0.2-0.48) binary donor for most large game functions; early CON |
| idtech-3dduke-share | 1996-01 | Duke3D 1.0 shareware EXE/CON (GRP truncated by the split installer) | ASS 1.0x; game lineage |
| idtech-1rott13 | 1995-08 | ROTT 1.3 | ASS between 1.01 and Duke 1.0 |
| ny-sw-src-addendum-20190129 | 1994-1997 | SW source + Build addendum (NY00123) | AUDIO_WF.LIB 1995-11-13, USRHOOKS.C 1994-09-04; engine: 1996 BUILD.TXT, 1997 ENGINE.C only |
| advsys-convmaps | 1995 | Ken's map converters with source | exact v4->v5->v6 layouts (LameDuke maps are v5) |
| advsys-build-apr1994 | 1994-04 | Ken's Apr-1994 Build | = kenbuild BUIL0408 except the original unheadered BUILD.H, BUILD.TXT, GAME.C |
| advsys-buildc | 1996 | readable C of every A.ASM routine | semantic only |
| wayback-fandom-prototype-texts | 1995-09 | September-1995 GAME.CON transcription (TerminX, 2017) | semantic: CON between LameDuke and 0.99 |
| advsys build1/builgrid/buildsec/groudraw/cave/polytex/kenbuild2000/2draw/cubes5/chess, jonof whaven/wtchfix2/tekwar demos, lesbird tekwar/c8gwsrc, idtech swsource2005/misc-lameduke | 1993-2005 | mostly duplicates or later | low; recorded for dedupe and lineage |

## Identified only (not acquired)

- Shadow Warrior 1995-01-15 / 1995-02-10 prototypes: free Steam app 238070 "Shadow Warrior Classic (1997)"
  (Devolver, 2017); needs a Steam client and account (user decision). Next-closest engine binaries after L7P.
- Duke3D 0.99 beta on hendricks266.duke4.net (described there as leaked; the 3D Realms FTP mirror copy was used),
  Shadow Warrior 0.90 beta, Blood alpha/beta, Witchaven II source, Fate preview, Ken's DOS32 utilities, kenbuild
  unmodified-data patches, build95_exe_20190520.7z, remaining 3D Realms FTP items (listing:
  `build/workers/acquire/idtech/listing.json`, 357 entries).
- Decrypted (dongle-cracked) Shadow Warrior betas: not acquired (circumvention).

## Unavailable

The October-1994 Duke3D prototype ("LamerDuke", mid-1994 builds) and the 1995-05-09 beta are not public (3D Realms
archive, seen under NDA). Hendricks266 (forums.duke4.net topic 7266 #610, 2017-05-01) found Duke3D 1.3D/1.4
source and fragments of much earlier builds; nothing released. Legend of the Seven Paladins full builds
1994-09-26 / 1994-11-01 are not public. forums.duke4.net refused direct access during the pass (Wayback copies
used); live fandom pages sit behind a Cloudflare check.

## Historical relationships established

- **Apogee Sound System:** ROTT 1.0 (1994-12-21) embeds ASS 1.01; LameDuke shares the early lineage but not the
  same build (no banner, different PAS message, ULTRASND.INI vs GUSMIDI.INI, different OPL delay loop and timbre
  data). PROVEN.
- **Watcom versions by object dating:** Ken's objects carry 9.5-style dependency timestamps until July 1995, then
  10.0a (1995-07-10..16); Capstone used 10.0a. Consistent with a 9.5 engine in LameDuke. STRONG.
- **Duke3D extras:** only GAMERBG.H (1995) and RGAME.H sit between LameDuke and 1.3D; the "alternative" game
  files are 1996 Duke 1.5 snapshots. PROVEN.
- **Lineage of STRICT code:** of 560 STRICT functions, 33 have dated literal copies and 239 dated relocation-aware
  copies in the corpus (`build/binscan/out/lineage.json`: first and last dated appearance).
- **Large game functions:** none of the 30 largest non-STRICT functions has a complete relocation-aware copy in any
  artefact; the best binary donors (0.99 beta, Witchaven, Ken's GAME.EXEs) score 0.2-0.48 — the Duke game code
  changed substantially between Dec 1994 and Jan 1996.

## Function-level donors (top binary donors, `build/binscan/NOTES.md`)

| Target | Best donor | Score |
|---|---|---:|
| hitscan | BUIL0719 ENGINE.OBJ (1994-07-19) | 0.62 |
| draw2dscreen | L7P 75M.EXE (1994-09-22) | 0.60 |
| drawmaskwall | L7P 75M.EXE (1994-09-22) | 0.59 |
| drawsprite | SRC_9505 ENGINE.OBJ (1995-05-12) | 0.58 |
| grouvline | L7P 75T.EXE (1994-09-22) | 0.55 |
| getcliplines | SRC_9505 ENGINE.OBJ | 0.49 |
| parsecommand | Duke3D 0.99 DUKE3D.EXE (1996-01-05) | 0.48 |
| getzrange | SRC_9505 ENGINE.OBJ | 0.42 |
| operateactivators | Duke3D 0.99 | 0.37 |
| moveeffectors | Duke3D 0.99 | 0.33 |

## Matching improvements attributable to this pass

| Source | Change | Result |
|---|---|---|
| ROTT 1.0 ROTT.EXE | TS_AddTask reads `ptr->prev` directly (identical December body in both binaries) | +1 STRICT, +156 B (ASS 349/375) |
| Duke3D extras GAMERBG.H | player fields, SOUNDOWNER/SAMPLE, clubframes/bonus_xy/diz/user_quote names | names only (byte-neutral) |
| Asset/CON cross-check | CON `sound` lookup tests `i == 38` as the executable does | semantic fix (no byte change) |
| L7P 75M.EXE (Sep 1994), BUIL0719 | setview span/stride form, hitscan sprite deltas, overwritesprite, raytrace | no STRICT change; setview 123 -> 9 differing bytes, content 0.62 -> 0.94; hitscan 0.73 -> 0.77 |

## Promising directions

1. Engine: setview is 9 bytes from STRICT after the L7P-guided rewrite (registers/load order); draw2dscreen and rotatesprite need extent-preserving forms.
2. Shadow Warrior January/February 1995 prototypes if the user installs the free Steam app.
3. September-1995 GAME.CON and the 0.99 CON as intermediate CON vocabulary for gamedef (parse/execute).
4. A source-level xscan run over the newly acquired C (Witchaven/TekWar GAME.C, CONVMAPS, BUILDC) for names and
   locals.
5. ASS: Nov-1995 AUDIO_WF.LIB and ROTT 1.3 as later lineage points for the 12 remaining NEAR ASS functions.
