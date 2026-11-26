# Toolchain

| Component | Evidence | Status |
|---|---|---|
| Runtime library | Watcom 9.5 GA (unpatched) CLIB3R/MATH387R/MATH3R/EMU387: 125 contributions; pre-patch-a members; banner 1988-1993 | PROVEN 9.5 GA |
| C compiler | Watcom 9.5 GA (unpatched; install dir watcom-9.5b, BPATCH -q 'not patched'); DOS-hosted BIN/WCC386.EXE (Phar Lap) is canonical. Reproduces BUIL0422 ENGINE.OBJ fully; patch levels a/b/c do not (36-38/88) | PROVEN |
| Flags | engine `-4r -s -or`, multi `-4r -s`, game `-4r -s -otexan`; audiolib Watcom 10.0 GA by its author | PROVEN by STRICT matches |
| Assembler (A.ASM) | Ken's makefile `masm a.asm;`; build-reconstructions: MASM 5.10 for A.OBJ <= 1996-05 | STRONG (MASM 5.10) |
| Audio library | ASS ~1.04-era LIB, compiled by its author (1.04 originals were Watcom 10.0b + TASM 3.1) | OPEN exact version |
| Linker | WLINK (version OPEN), DOS/4GW 1.95 stub/extender (DOS4GW.EXE == Watcom 10.0's) | OPEN |

Profiles live in `config/toolchain.json`. A profile that reproduces bytes is a reproducing configuration; the table
above says what the evidence supports about the historical toolchain.
