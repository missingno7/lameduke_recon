# GAMEDEF follow-up

## Verification

The integrated `game_gamedef` source was copied to `src2/` and built fresh with Watcom 9.5b `-4r -s -otexan`. Both the integrated gate and worker build report **7 STRICT / 800 bytes, 2 CODE_BYTES_MATCH / 912 bytes, 5 NEAR_MATCH / 20,652 oracle bytes**. The source probes below produced no promotion under the standard verifier.

**PROVEN** The data-only natural link verifies the complete **1,600-byte** CONST contribution including alignment, all 60 keyword-table target addresses, and the 10 CONST references in `transword`, `transnum` and `loadefs`. The new BSS binding resolves the loader's file-end pointer. Between the standard verifier and the linked-data check, all 34 `transword` references and all 58 `loadefs` references have corroboration. The standard verifier still requires complete emitted OMF coverage of referred-to data segments, so its two function statuses remain CODE_BYTES_MATCH.

## How the padding is made

The game-profile object has 1,598 CONST bytes, of which 1,587 are explicitly emitted. The baseline produced zero disagreements among emitted bytes, including at `+1b`. The eleven uncovered offsets are `+aa,+ab,+15d,+15e,+15f,+20d,+20e,+20f,+579,+57a,+57b`.

**PROVEN** When Watcom 9.5b emits a gap within a CONST LEDATA record, that gap retains bytes from the same positions in the reused record buffer. The first record contains `actor` at offset `+4`. The next record starts at `+ac` with `ends`, and its offsets `+5..+7` contain `cto` from `actor`. The scratch control `actor` → `ACTOR` changes those bytes to `CTO`. Likewise, `addscore` → `ADDSCORE` changes `ore` after `ifaction` at `+139..+13b`. Replaying the record buffer explains **all 123 emitted padding bytes**, including 66 nonzero bytes, across the pool of 84 literals.

**PROVEN** Alignment at a record transition can leave a hole with no LEDATA. A hash-checked Watcom 9.5 WLINK `SYSTEM DOS4G` link writes zeros into all eleven holes and the two bytes before `_DATA`. The WLINK map locates CONST and `_DATA` independently of the CON code. All 60 ordered keyword table fixups confirm the relative literal placements, and the linked bytes equal `2:00b44–01184` in D3D.EXE. The proof uses the ordinary source literals. The link allows unresolved game calls and is used only to inspect locally resolved data and fixups.

**NEGATIVE** LF/CRLF, moving literals onto separate lines, adjacent literal concatenation, `#line`, `-d1`, `-d2`, `-ot`, `-or` and `-otexa` left the same eleven holes and the same covered bytes. `-os` removed the holes by eliminating the speed profile's four-byte literal alignment and produced a different pool. The short-literal control also ruled out reuse of just the immediately preceding literal's scanner contents.

## Independent loader binding

**STRONG** Proposal `last_used_text = 2:b1df0` in `overlay2.json`. The private word and byte state occupies `2:b1df4–b1dff`, with its word slots independently anchored by lexer and interpreter references. The next engine allocation starts at `2:b1e00`. The expected group has one pointer, four shorts and three chars, or 15 bytes rounded to 16 at the following alignment. The later GAMEDEF source declares the file-buffer pointer as private state. Eight Watcom 9.5b controls place the sole pointer at offset zero in a 15-byte group regardless of public/static linkage, forward/reverse declaration order and a name prefix. This predicts `2:b1df0` from the group boundary and other fields, independently of the single store at `1:30283`. The standard verifier resolves this store with the overlay. The individual linkage and order of the remaining game BSS contributions are still open.

## Remaining code

| Function | First difference | Classification | Findings and negative controls |
| --- | --- | --- | --- |
| `transnum` | `+18c` / `1:2ec6c` | Layout | Candidate bytes and fixup sites agree. This first mismatched target is already in the 61-entry `parsecommand` table. All 396 bytes of the numeric lexer precede it. Its diagnostic relocation is confirmed by the linked-data check. |
| `parsecommand` | `+a` / `1:2ed6a` | Register allocation at entry; source shape downstream | EAX/EDX allocation in the error/warning sum differs. Swapping operand order or using long promotions leaves it unchanged. Under `-oe=100` the entry reaches `+22`, where a forward-branch displacement differs. That probe emits 5,296 bytes and leaves six out-of-line `keyword` calls. Explicitly expanding the label and lookahead loops emits 5,424 bytes and changes the first difference to `+8`. The 5,232-byte oracle also requires the original case lowering and branch placements. |
| `move_actor` | `+6` / `1:304a6` | Register allocation and frame spill; layout later | Candidate stack reservation is 4 bytes versus 8. The oracle spills the 16-bit movement flags and holds the sprite index in ECX. Signed/unsigned and volatile flags, equivalent early checks, joining the owner tests, and swapping the count update with the movement-pointer read do not reproduce the prologue. An unsigned-flags, pointer-first probe saves 48 candidate bytes and keeps the same first mismatch. Its table-bearing extent is farther from the oracle. |
| `parse` | `+8` / `1:308b8` | Register allocation; source shape and layout later | The candidate holds the sprite index in EDI. The oracle spills both short arguments and holds the distance argument in EDI. Volatile short parameters perturb later cases, and moving the terminating cases earlier has no effect. An explicit shared stop label reduces the extent by 16 bytes and leaves the first mismatch at `+8`. |
| `execute` | `+6` / `1:32266` | Register allocation and frame spill; source shape later | The 24-byte candidate frame differs from the 44-byte oracle frame before interpretation. The emitted dispatch and movement code have the expected roles. Volatile argument controls enlarge the frame and add loads throughout the in-function dispatch. The closest such extent still differs at `+6`. |

The code experiments used the pinned 9.5b compiler and the game profile unless an additional `-oe=100` is specified. The highest-leverage remaining code issue is the compiler dispatcher's shape, because its case layout also governs `transnum`'s reported trailing-table fixups. A gate path that independently verifies file-backed linked data and checks every target would allow the two completed code extents to use the natural CONST proof.

## Reproduce

```powershell
$w = 'build/workers/gamedef'
New-Item -ItemType Directory -Force "$w/scratch" | Out-Null
python -B "$w/run.py" wcc --profile game "$w/src2/gamedef.c" "-i=$w/src2" "-fo=$w/scratch/con.obj"
python -B "$w/run.py" match --symbols "$w/overlay2.json" --obj "$w/scratch/con.obj" --json "$w/results2.json"
python -B "$w/prove_const.py" "$w/scratch/con.obj" "$w/results2.json"
python -B "$w/prove_state_layout.py"
python -B "$w/gate_local.py"
```

The matcher returns 1 for the reported partial statuses. The worker-local gate runs the repository's unittest suite and unmodified `verify.py` with scratch redirected under the worker. It completed with nine tests passing and no integrated regressions.
