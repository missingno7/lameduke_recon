# Toolchain

| Component | Evidence | Status |
|---|---|---|
| Runtime library | Watcom 9.5b CLIB3R/MATH387R/MATH3R: 104 exact+sites members; banner 1988-1993 unique to 9.5 | PROVEN 9.5 family |
| Engine compiler | `wc95b -4r` reproduces BUIL0422 ENGINE.OBJ byte-for-byte; Ken's makefiles `wcc386 engine /4r` | PROVEN for Apr-94 engine; HYPOTHESIS for Dec-94 |
| Game compiler | not yet calibrated | OPEN |
| Assembler (A.ASM) | Ken's makefile `masm a.asm;`; build-reconstructions: MASM 5.10 for A.OBJ <= 1996-05 | STRONG (MASM 5.10) |
| Audio library | ASS ~1.04-era LIB, compiled by its author (1.04 originals were Watcom 10.0b + TASM 3.1) | OPEN exact version |
| Linker | WLINK (version OPEN), DOS/4GW 1.95 stub/extender (DOS4GW.EXE == Watcom 10.0's) | OPEN |

Profiles live in `config/toolchain.json`. A profile that reproduces bytes is a reproducing configuration; the table
above says what the evidence supports about the historical toolchain.
