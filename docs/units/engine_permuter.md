# Serial Watcom 9.5b source permutation pilot

This copy of the Rayman permuter uses the LameDuke engine profile and oracle.
It calls the stock staged compiler and the unmodified matcher. The launcher
accepts one compile at a time and limits output to the engine worker directory.
It keeps the original full translation unit and emits CP437 source with CRLF.

From the LameDuke repository root, use an unused output directory.

```powershell
$p = 'build/workers/engine'
python -B "$p/permuter/permute.py" "$p/src5/ENGINE.C" calcwalls --out "$p/work5/next-calcwalls" --conservative --mechanism home --seed 942 --max-compiles 20 --seconds 32
```

The run charges the pinned preprocessor, baseline, candidate compiles, and any
fresh acceptance check against the same budget. An admitted compiler call may
finish after the time limit. It writes a manifest, the candidate log, and a
summary. A strict result requires the ordinary full-extent verifier, preservation
of baseline matches, and identical non-code shape. Scores are diagnostic.

The parser handles Watcom 9.5 preprocessor line splices, implicit-int definitions
and the SDK's segment, interrupt, far-function-pointer, and based-pointer
declarations as parser context. It retains the original declarations in emitted
source. The inherited residual-macro guard can refuse otherwise viable functions
when a dormant SDK definition shares an identifier with the function.

Source provenance and the upstream mutation-pass license are in
`permute.LICENSE`. The fifth-pass engine report records results by function.
