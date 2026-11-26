# Prior art: what transfers from duke3d-1.3d and rayman_recon

Survey 2026-11-26 (duke3d-1.3d main @69037cb + archive/recovery-workspace-2026-11-17 @e8b2c67; rayman_recon @d94c233).
Everything below about compiler behaviour is a fact about Watcom 10.x and is only a HYPOTHESIS for our 9.5 target.

## duke3d-1.3d (Watcom 10.0/10.0a/10.0b/10.5, 1996)
- Byte-identical, but only ~70 of 121 owned objects are direct Watcom compiles; the rest (incl. the whole GAME
  object) are symbolic GNU-as/TASM/MASM/NASM "backends" converted to OMF. We do NOT adopt assembly backends for C
  modules; C counterparts there are reading aids only. Accounting there gives zero credit to stock/vendor bytes.
- Flags were per object (candidates/complete_source.json): game `/4r /s /ort`, engine (10.5) `/4r /s /or`,
  audiolib `/omaxnet /zp4 /5r` (some `/zu`, GUS `/d2`), RAND `/3r /s /os`. The release makefile's `/otexan` was wrong.
  `/4r` vs `/5r` differ in function alignment/padding.
- String-literal padding contains compiler buffer residue; they reproduce it with named message arrays plus explicit
  padding fields under `#pragma data_seg`; some literals must stay literals because of inline MOVSD copies.
- BSS order depends on names (size bucket, page of 25 symbols, PJW hash), not declaration order; the first
  declaration (even `extern`) matters.
- WLINK (10.0 GA) with `system dos4g`, explicit FILE list of ~300 objects (stock library members extracted and
  interleaved), `option map/statics/stack=50k/nodefaultlibs`. DOSSEG puts BEGDATA/_NULL first. STACK enlarged by
  the linker. Object order recovered by causal FILE-order trials.
- LE fixup records: per page in LEDATA record order, descending offset within a record; Watcom C LEDATA packets
  ~172-176 bytes, assembler packets up to 1022 -> source/assembler choice affects fixup order.
- Their DUKE3D.EXE is DOS4GW-bound (vendor prefix); ours has a plain 10,418-byte stub. No LE writer exists there.
- Reusable references: profiles/watcom/tools/omf32.py, le.py, runner.py (DOSBox-X wcc386 runner), tools/
  probe_bss_order.py, inventory_watcom_runtime.py, actors_omf_link.py (OMF writer), setup_watcom.py (media URLs
  and hashes for 10.0 GA/a/b, 10.5; 10.0b = 10.0a + PTCH23.B).
- Rejected there: declaration-order swaps for allocation, grouping state into structs, leaving string padding
  to the compiler, `/5r` for game code.

## rayman_recon (Watcom 10.6, PMW1)
- Tools adopted here: omf.py (unchanged), libscan.py, match.py, wcc.py, common.py (adapted to LE).
- Status vocabulary and acceptance policy (any valid C, quality tags NATURAL/COMPILER_STEERED/SYNTHETIC) adopted.
- Still to evaluate: tumap.py, seed.py, permute.py, semir.py, explain.py/cgtrace.py (10.6 allocator tracing;
  needs re-validation on 9.5), difftest (Unicorn differential execution).
