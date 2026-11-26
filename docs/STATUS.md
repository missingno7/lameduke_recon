# Status

Baseline 2026-11-26. Denominator: obj1 = 443,461 bytes (code object incl. runtime), 1,284 candidate function starts
(tools/funcmap.py, preliminary). Data: obj2 initialised 45,056 bytes + BSS.

| Metric | Functions | Bytes |
|---|---|---|
| STRICT reconstructed C | 0 | 0 |
| STRICT original assembly | 0 | 0 |
| Identified library code (Watcom 9.5 runtime, exact+sites) | 104 members | 20,790 |
| Identified Apogee Sound System functions (ASS 1.04 exact+sites, per function, not deduplicated) | 343 | 48,100 |
| Historical-object exact functions (engine, BUIL0801) | 9 | 790 |

Plan: (1) Dec-94 engine reconstruction from 1995-03-28 ENGINE.C + Oct-94 BUILD.H + BUILD.TXT changelog;
(2) audio library version/source recovery; (3) runtime + A.ASM completion; (4) game-code TU map and compiler
calibration with Duke3D 1.5 source as semantic reference; (5) symbol propagation and data ownership.
