# NEAR function census

The unit registry contributed **125 NEAR_MATCH functions across 16 units**. The census uses the pinned DOS-hosted Watcom 9.5b compiler for game and engine code, and the registered Watcom 10.0 profile for the integrated audio-library units. Each translation unit is compiled once, with its registered flags and includes. The JSON retains the full comparison expressions, verifier status, oracle and candidate coordinates, and CFG diagnostics.

## Running the tool

From the repository root, run the following commands.

~~~powershell
$env:LAMEDUKE_HOST = 'dos'
$env:PYTHONHASHSEED = '0'
python -B build/workers/semir/tools_proposed/semir.py engine wallfront
python -B build/workers/semir/tools_proposed/semir.py engine wallfront --src path/to/ENGINE.C
python -B build/workers/semir/tools_proposed/semir.py path/to/game.c next_text_shade --profile game -i src/game
python -B build/workers/semir/tools_proposed/semir.py --census --json build/workers/semir/near_census.json
python -B build/workers/semir/tools_proposed/semir.py --strict-controls --json build/workers/semir/controls/strict.json
python -B build/workers/semir/validate.py
python -B build/workers/semir/tools_proposed/cgtrace_poc.py
~~~

A standalone candidate defaults to the game profile. Use `--unit` to inherit a unit's exact flags and headers, and `--symbols` to supply a `match.py` overlay where a function or reference needs a binding. JSON output and compiler scratch stay within the worker directory.

## Reading the result

- **EXACT** comes from a fresh `match.py` STRICT verdict for the full function extent.
- **LAYOUT** identifies a changed cross-function relative displacement to the same bound destination or to the same decoded shared epilogue. The check accounts for the entire function extent, OMF/LE relocation sites and targets, and every other byte. The reported site is the original branch instruction.
- **ALLOCATION-ONLY** means the aligned effects and control flow agree in the IR. The count includes register and stack homes, scheduling, and equivalent instruction selections. Start with live ranges around the verifier's first differing byte.
- **SOURCE** identifies the first differing modeled effect, predicate or return expression in aligned CFG traversal order. Changes in instruction selection or CFG shape can also produce a clue. The instruction location identifies where the difference becomes observable, which can be later than the defining operation.
- **UNKNOWN** flags an unresolved comparison, such as an indirect call, an unsupported instruction, an ambiguous join, an unbound data reference, or an unmatched CFG. It preserves the first blocker so that a later difference cannot be mistaken for the earliest one.

`semir.py` ports the Rayman lifter and width-aware expression comparison from commit `d94c233`. The local adapter uses `image.Image` and `match.Bindings` for hash-checked LE object coordinates and symbol resolution. Oracle extents use the next bound code address or a candidate function start from call, entry, or data-reference evidence. Candidate extents come from OMF publics. CFG traversal follows reachable code, handles common jump tables, and recognizes shared stack-unwind-and-return sequences on each side. The port adds x86 double shifts and counter branches seen in the 9.5b output.

The port replaces Rayman's fixed literal address range, content-only literal keys, default game profile, cached-object driver, and `census.Oracle` ABI glue. CONST and BSS references now use independently verified segment locations or unanimous public anchors. Callee demand and preservation remain inferred from the oracle, with Watcom register ABI fallbacks. That inference, caller-clean stack arguments, indirect calls, escaped locals, x87 instructions, CFG alignment, and string instructions' assumed clear direction flag limit coverage. Use SOURCE as a place to inspect the reconstruction.

## Controls

| Control | Observed result |
| --- | --- |
| 468 registered STRICT functions | 468 EXACT, rebuilt through the registered compiler or assembler wrapper. |
| `next_text_shade` return constant 16 → 17 | SOURCE at the returned value at `1:023fa`. |
| `inittimer` first two `outp` statements swapped | SOURCE at the first call at `1:0002d`. The modeled register arguments change from `(67, 54)` to `(64, -42)`. |
| `next_text_shade` local `phase` narrowed from `short` to `signed char` | SOURCE at the store to `hudpulse` at `1:023ef`. The expression changes from a 16-bit load and addition to sign extension from 8 bits. |
| `wallfront` | ALLOCATION-ONLY with a fresh NEAR_MATCH. |
| `initspritelists` | LAYOUT at `1:40b3b`, with both jumps executing the same shared epilogue. |

A negative layout probe inserted an unrelated function before `calcksqrt` and rebuilt the full ENGINE translation unit with its registry profile. The `loadtables` caller stayed STRICT because the call is emitted as a symbolic OMF relocation. `initspritelists` exercises an inline displacement that changes. Results are in `controls/`. The `validate.py` script generates the three source mutations as temporary full-TU copies.

## Watcom 9.5b allocator tracing

**Proven.** The pinned 9.5b `BINNT/WCC386.EXE` is an i386 PE with a direct native code section. Rayman's 10.6 observer first waits for its NT wrapper to load a CF payload and uses payload-relative RVAs. The Windows debug event transport and WOW64 context APIs transfer to 9.5b. The loader rendezvous and compiler data layouts require 9.5b-specific evidence.

The proof of concept uses one hardware execution breakpoint at PE RVA `0x4c22d` and reads the linked-list heads, link offsets, comparator and return address through the debugger. The sort helper was located from the counting loop shared with the 10.6 hook at CF RVA `0x703ae`. On the 9.5b probe it recorded 15 calls from four call sites. Observed vs ordinary NT builds produced identical whole OMF bytes. The canonical DOS build agreed on CODE bytes, coverage, publics, and fixups. See the [captured calls and hashes](controls/allocator/result.json).

**Strong.** The 9.5b comparator at `0x53951` and the 10.6 conflict comparator at `0x61a08` both compare the unsigned field at `+0x14` and feed the sorter through the same short caller pattern. Other hooks need calibration. For example, a comparator observed at `0x4d5bc` reads flag byte `+0x2d` and compares a field at `+0x08`. The 10.6 name-order comparator reads `+0x24`, while a short 9.5b comparator with the same instruction pattern reads `+0x20`.

Estimate **2–4 engineering days** for targeted home and conflict tracing, including mapping phase boundaries and recovering record fields against a small set of local-width and live-range probes. Estimate **1–2 weeks** for a validated `cgtrace` and `explain` port with phase snapshots, instruction association, rover and Score hooks, and scheduler coverage. The estimates assume a viable analogue for each 10.6 pass. Use the census first for SOURCE and LAYOUT cases, and start targeted allocator tracing with `wallfront` or another ALLOCATION-ONLY function with a compact body.

## Results

Registry SHA-256 `c9601991129b91ae4b619b2646ba1a993969868bf0b2e2ba85f1690960bec1e4`. Candidate size is the OMF public extent. The STRICT control total is 81,602 verified bytes.

The final census includes the working-tree ACTORS revision compiled during the run. All seven registered ACTORS STRICT controls were also rerun on that revision.

| Unit | Functions | SOURCE | ALLOCATION-ONLY | LAYOUT | EXACT | UNKNOWN | Candidate bytes |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `engine` | 40 | 22 | 3 | 13 | 0 | 2 | 51,134 |
| `ass_al_midi` | 5 | 3 | 1 | 0 | 0 | 1 | 2,912 |
| `ass_blastold` | 1 | 1 | 0 | 0 | 0 | 0 | 632 |
| `ass_fx_man` | 2 | 0 | 0 | 0 | 0 | 2 | 764 |
| `ass_guswave` | 3 | 1 | 0 | 0 | 0 | 2 | 1,024 |
| `ass_midi` | 3 | 2 | 1 | 0 | 0 | 0 | 1,648 |
| `ass_multivoc` | 14 | 1 | 2 | 5 | 0 | 6 | 4,213 |
| `ass_task_man` | 3 | 0 | 2 | 0 | 0 | 1 | 416 |
| `game_sounds` | 1 | 0 | 1 | 0 | 0 | 0 | 464 |
| `game_gamedef` | 5 | 2 | 1 | 0 | 0 | 2 | 18,108 |
| `multi` | 7 | 4 | 2 | 0 | 0 | 1 | 5,247 |
| `game_menues` | 5 | 2 | 1 | 0 | 0 | 2 | 8,320 |
| `game_sector` | 9 | 5 | 2 | 0 | 0 | 2 | 38,032 |
| `game_player` | 7 | 5 | 1 | 0 | 0 | 1 | 23,838 |
| `game_game` | 9 | 7 | 2 | 0 | 0 | 0 | 3,344 |
| `game_actors` | 11 | 7 | 2 | 0 | 0 | 2 | 25,241 |
| **Total** | 125 | 62 | 21 | 18 | 0 | 24 | 185,337 |

The table shows the first differing effect or comparison blocker. The JSON contains the full expressions.

| Unit | Function | Candidate bytes | Class | First semantic difference or blocker | Hint |
| --- | --- | ---: | --- | --- | --- |
| `engine` | `drawrooms` | 928 | SOURCE | `1:341ed` branch predicate. DOS `lt_s32(phi@341e4:ebx, sx32(ld16[numbunches+0]))`, C `lt_s16(tr16(phi@388:ebx), ld16[numbunches+0])` | Compare the predicate width, operands and test order at the listed branch. |
| `engine` | `scansector` | 748 | SOURCE | `1:3c321` return value (eax). DOS `phi@3447a:eax`, C `phi@6ff:eax` | Trace the return value back to its defining load or arithmetic. |
| `engine` | `wallfront` | 562 | ALLOCATION-ONLY | Modeled effects agree. | Inspect live ranges and stack homes at the verifier's first byte difference. |
| `engine` | `spritewallfront` | 114 | ALLOCATION-ONLY | Modeled effects agree. | Inspect live ranges and stack homes at the verifier's first byte difference. |
| `engine` | `calcwalls` | 5,068 | UNKNOWN | `1:34abe` comparison depended on unsupported values. | Check unresolved references and the expression or join named in the diagnostic. |
| `engine` | `hline` | 965 | SOURCE | `1:39e38` effect #1 differs. DOS `call hlineasm64(edx=(shl32(phi@39d0b:esi, (31 and zx32(ld8[globalyshift+0]))) + ld32[globalypanning+0]), ebx=0, ec?`, C `call hlineasm64(edx=(shl32(phi@5eaa:esi, (31 and zx32(ld8[globalyshift+0]))) + ld32[globalypanning+0]), ebx=0, ecx?` | Check the callee, evaluation order and argument values at the listed call. |
| `engine` | `screencapture` | 1,511 | SOURCE | `1:3b604` branch predicate. DOS `lt_s32(phi@3b602:ecx, phi@3b602:edi)`, C `lt_s32(phi@77a6:esi, phi@77a6:edi)` | Compare the predicate width, operands and test order at the listed branch. |
| `engine` | `drawmaskwall` | 1,887 | ALLOCATION-ONLY | Modeled effects agree. | Inspect live ranges and stack homes at the verifier's first byte difference. |
| `engine` | `drawsprite` | 10,422 | SOURCE | `1:3d4fb` branch predicate. DOS `lt_s32(mul32(sar32((mul32(ld32[cosglobalang+0], (-shrd32(..., ..., ...) + ld32[sprite+4 + (38*sx32(tr16(in_ecx)))]?`, C `lt_s32(shrd32((mul32(ld32[cosglobalang+0], (-ld32[globalposx+0] + ld32[sprite+0 + (38*sx32(tr16(in_ecx)))] + -shrd?` | Compare the predicate width, operands and test order at the listed branch. |
| `engine` | `ceilspritescan` | 402 | LAYOUT | `1:3f2a4` both jumps execute the same verified instruction sequence of a shared epilogue. | Compare the linked displacement after restoring the neighboring function placement. |
| `engine` | `ceilspritehline` | 1,044 | SOURCE | `1:3f3a3` branch predicate. DOS `eq8(ld8[option+0], 0)`, C `lt_s32(shrd32(mul32(ld32[globalzd+0], ld32[horizlookup+800 + (4*in_ebx + -4*ld32[globalhoriz+0])]), mulhi_s32(ld32?` | Compare the predicate width, operands and test order at the listed branch. |
| `engine` | `overwritesprite` | 1,323 | SOURCE | `1:3f73c` branch predicate. DOS `eq8((1 and arg[4]8), 0)`, C `eq32(ld32[stereofps+0], 0)` | Compare the predicate width, operands and test order at the listed branch. |
| `engine` | `permanentwritesprite` | 1,358 | SOURCE | `1:3fb0c` branch predicate. DOS `eq32(ld32[waloff+0 + (4*sx32(tr16(in_ebx)))], -1)`, C `eq32(ld32[waloff+0 + (4*sx32(tr16(in_ebx)))], -1)` The compared memory histories diverge. | Compare the predicate width, operands and test order at the listed branch. |
| `engine` | `grouvline` | 2,337 | SOURCE | `1:40023` branch predicate. DOS `lt_s32(ld32[umost+0 + (4*in_eax)], ld32[dmost+0 + (4*in_eax)])`, C `lt_s32(ld32[umost+0 + (4*in_eax)], ld32[dmost+0 + (4*in_eax)])` The compared memory histories diverge. | Compare the predicate width, operands and test order at the listed branch. |
| `engine` | `initspritelists` | 206 | LAYOUT | `1:40b3b` both jumps execute the same verified instruction sequence of a shared epilogue. | Compare the linked displacement after restoring the neighboring function placement. |
| `engine` | `insertspritesect` | 158 | LAYOUT | `1:40b73` both jumps execute the same verified instruction sequence of a shared epilogue. | Compare the linked displacement after restoring the neighboring function placement. |
| `engine` | `insertspritestat` | 156 | LAYOUT | `1:40c11` both jumps execute the same verified instruction sequence of a shared epilogue. | Compare the linked displacement after restoring the neighboring function placement. |
| `engine` | `deletespritesect` | 219 | LAYOUT | `1:40cc3` both jumps execute the same verified instruction sequence of a shared epilogue. | Compare the linked displacement after restoring the neighboring function placement. |
| `engine` | `deletespritestat` | 217 | LAYOUT | `1:40d9e` both jumps execute the same verified instruction sequence of a shared epilogue. | Compare the linked displacement after restoring the neighboring function placement. |
| `engine` | `cansee` | 388 | SOURCE | `1:41056` branch predicate. DOS `lt_s16(tr16(phi@4104f:sp-24), tr16(phi@4104f:edi))`, C `lt_s32(phi@d736:sp-28, phi@d736:sp-20)` | Compare the predicate width, operands and test order at the listed branch. |
| `engine` | `hitscan` | 1,659 | SOURCE | `1:4123e` branch predicate. DOS `lt_s16(tr16(phi@41235:sp-32), tr16(phi@41235:sp-16))`, C `lt_s16(tr16(phi@d7a9:sp-24), tr16(phi@d7a9:sp-32))` | Compare the predicate width, operands and test order at the listed branch. |
| `engine` | `neartag` | 1,583 | SOURCE | `1:41981` branch predicate. DOS `lt_s16(tr16(phi@41978:sp-28), tr16(phi@41978:sp-16))`, C `lt_s16(tr16(phi@dea3:sp-28), tr16(phi@dea3:sp-36))` | Compare the predicate width, operands and test order at the listed branch. |
| `engine` | `lintersect` | 548 | SOURCE | C has an extra effect. DOS `<none>`, C `call name:labs(eax=(mul32((arg[24]32 + -arg[16]32), (in_ecx + -arg[12]32)) + -mul32((arg[20]32 + -arg[12]32), (arg?` | Check the width, destination, and value of the listed store or effect. |
| `engine` | `clipmove` | 807 | SOURCE | `1:422d8` effect #0 differs. DOS `call getcliplines(eax=ld32[in_eax], edx=ld32[in_edx], ebx=ld32[in_ebx], ecx=(ld32[in_eax] + sar32(arg[4]32, 14)); ?`, C `call getcliplines(eax=ld32[in_eax], edx=ld32[in_edx], ebx=ld32[in_ebx], ecx=(ld32[in_eax] + sar32(arg[4]32, 14)); ?` The compared memory histories diverge. | Check the ordering of loads and stores that may alias. |
| `engine` | `getcliplines` | 3,585 | SOURCE | `1:425e2` branch predicate. DOS `eq32((mul32((in_ecx + -in_eax), (in_ecx + -in_eax)) + mul32((-in_edx + arg[4]32), (-in_edx + arg[4]32))), 0)`, C `lt_s16(tr16(arg[8]32), 0)` | Compare the predicate width, operands and test order at the listed branch. |
| `engine` | `keepaway` | 355 | LAYOUT | `1:434d5` both jumps execute the same verified instruction sequence of a shared epilogue. | Compare the linked displacement after restoring the neighboring function placement. |
| `engine` | `raytrace` | 524 | SOURCE | `1:43556` DOS effect #0 has no partner. DOS `st32 [arg[4]32] = (in_eax + sar32(in_ebx, 14))`, C `<none>` | Check the width, destination, and value of the listed store or effect. |
| `engine` | `rotatepoint` | 179 | LAYOUT | `1:4390f` both jumps execute the same verified instruction sequence of a shared epilogue. | Compare the linked displacement after restoring the neighboring function placement. |
| `engine` | `drawline16` | 1,171 | SOURCE | `1:43949` branch predicate. DOS `lt_s32(in_eax, 0)`, C `lt_s32((in_ebx + -in_eax), 0)` | Compare the predicate width, operands and test order at the listed branch. |
| `engine` | `qsetmode640480` | 272 | SOURCE | `1:43f50` branch predicate. DOS `eq32(ld32[qsetmode+0], 350)`, C `eq32(ld32[qsetmode+0], 480)` | Compare the predicate width, operands and test order at the listed branch. |
| `engine` | `draw2dgrid` | 735 | LAYOUT | `1:44459` both jumps execute the same verified instruction sequence of a shared epilogue. | Compare the linked displacement after restoring the neighboring function placement. |
| `engine` | `draw2dscreen` | 3,169 | SOURCE | `1:44b37` branch predicate. DOS `lt_s32(phi@44b2a:sp-28, sx32(ld16[numwalls+0]))`, C `lt_s32(phi@10cd6:sp-56, 0)` | Compare the predicate width, operands and test order at the listed branch. |
| `engine` | `getzrange` | 2,819 | SOURCE | `1:456be` DOS effect #0 has no partner. DOS `st32 [arg[4]32] = ld32[sector+c + (37*sx32(tr16(in_ecx)))]`, C `<none>` | Check the width, destination, and value of the listed store or effect. |
| `engine` | `makeradarang` | 229 | LAYOUT | `1:462c1` both jumps execute the same verified instruction sequence of a shared epilogue. | Compare the linked displacement after restoring the neighboring function placement. |
| `engine` | `setview` | 336 | SOURCE | `1:463ee` DOS effect #0 has no partner. DOS `st32 [ytop+0] = sar32((-ld32[ydimen+0] + 200), 1)`, C `<none>` | Check the width, destination, and value of the listed store or effect. |
| `engine` | `rotatesprite` | 1,538 | UNKNOWN | `1:46cf0` comparison depended on unsupported values. | Check unresolved references and the expression or join named in the diagnostic. |
| `engine` | `makepalookup` | 593 | SOURCE | `1:3bfd8` return value (eax). DOS `phi@47463:eax`, C `phi@130d0:eax` | Trace the return value back to its defining load or arithmetic. |
| `engine` | `setvesamode` | 292 | LAYOUT | `1:475af` both jumps execute the same verified instruction sequence of a shared epilogue. | Compare the linked displacement after restoring the neighboring function placement. |
| `engine` | `setverts` | 551 | LAYOUT | `1:478bb` both jumps execute the same verified instruction sequence of a shared epilogue. | Compare the linked displacement after restoring the neighboring function placement. |
| `engine` | `setredbluepalette` | 176 | LAYOUT | `1:47a55` both jumps execute the same verified instruction sequence of a shared epilogue. | Compare the linked displacement after restoring the neighboring function placement. |
| `ass_al_midi` | `AL_SetVoiceTimbre` | 1,108 | SOURCE | `1:60b5f` terminator. DOS `br`, C `jmp` | Inspect the first differing control-flow exit or join and its incoming values. |
| `ass_al_midi` | `AL_SetVoiceVolume` | 576 | SOURCE | `1:60fb1` branch predicate. DOS `lt_s32((ld32[Voice+10 + (40*in_eax)] + ld32[Channel+2c + (48*ld32[Voice+14 + (40*in_eax)])]), 128)`, C `lt_s32((ld32[Voice+10 + (40*in_eax)] + ld32[Channel+2c + (48*ld32[Voice+14 + (40*in_eax)])]), 127)` | Compare the predicate width, operands and test order at the listed branch. |
| `ass_al_midi` | `AL_SetVoicePitch` | 512 | SOURCE | `1:61161` branch predicate. DOS `lt_s32((phi@61141:ecx + ld32[Channel+28 + (48*ld32[Voice+14 + (40*in_eax)])] + ld32[Channel+10 + (48*ld32[Voice+14?`, C `lt_s32((phi@7e7:ecx + ld32[Channel+28 + (48*ld32[Voice+14 + (40*in_eax)])] + ld32[Channel+10 + (48*ld32[Voice+14 +?` | Compare the predicate width, operands and test order at the listed branch. |
| `ass_al_midi` | `AL_NoteOn` | 196 | ALLOCATION-ONLY | Modeled effects agree. | Inspect live ranges and stack homes at the verifier's first byte difference. |
| `ass_al_midi` | `AL_Init` | 520 | UNKNOWN | stack addresses escape: escaped locals modelled as memory, invalidated by every call and pointer store; frame offsets renamed bijectively | Check unresolved references and the expression or join named in the diagnostic. |
| `ass_blastold` | `BLASTER_Init` | 632 | SOURCE | `1:607e7` effect #0 differs; different effect kinds. DOS `call BLASTER_ReadMixer(eax=50)`, C `out8((ld16[BLASTER_MixerAddress+0] + 4), 50, IOentry())` | Check the callee, evaluation order and argument values at the listed call. |
| `ass_fx_man` | `FX_ErrorString` | 276 | UNKNOWN | `1:5dec7` return value. DOS `(obj2 + 0x1ae8)`, C `UNK(unbound reference ('CONST', 56))` | Check unresolved references and the expression or join named in the diagnostic. |
| `ass_fx_man` | `FX_SetupCard` | 488 | UNKNOWN | `1:5df54` unresolved or unsupported value in effect. DOS `call BLASTER_SetCardSettings(; stack: UNK(missing stack arg), UNK(missing stack arg), UNK(missing stack arg), UNK(?`, C `call BLASTER_SetCardSettings(; stack: UNK(missing stack arg), UNK(missing stack arg), UNK(missing stack arg), UNK(?` | Check unresolved references and the expression or join named in the diagnostic. |
| `ass_guswave` | `GUSWAVE_AllocVoice` | 248 | UNKNOWN | `1:5d086` unresolved stack delta at return. DOS `None`, C `None` | Inspect unsupported instructions before interpreting subsequent differences. |
| `ass_guswave` | `GUSWAVE_GetNextVOCBlock` | 404 | UNKNOWN | `1:5d11b` unsupported terminator. DOS `indirect jump dword ptr cs:[ebp + 0x5d088]`, C `indirect jump dword ptr cs:[ebp + 0x548]` | Check unresolved references and the expression or join named in the diagnostic. |
| `ass_guswave` | `GUSWAVE_PlayVOC` | 372 | SOURCE | `1:5d287` effect #1 differs. DOS `st32 [(ret(1:5cfb8) + 24)] = (zx32(ld16[(clob_ebp(call GUSWAVE_AllocVoice(ecx=in_ecx)) + 20)]) + clob_ebp(call GUS?`, C `st32 [(ret(1:5cfb8) + 24)] = (clob_edi(call GUSWAVE_AllocVoice(ecx=in_ecx)) + zx32(ld16[(clob_edi(call GUSWAVE_All?` | Check the width, destination, and value of the listed store or effect. |
| `ass_midi` | `_MIDI_ResetTracks_` | 108 | ALLOCATION-ONLY | Modeled effects agree. | Inspect live ranges and stack homes at the verifier's first byte difference. |
| `ass_midi` | `_MIDI_ServiceRoutine_` | 832 | SOURCE | `1:5b6d7` branch predicate. DOS `eq32(ld32[(phi@5b6d3:esi + 12)], 0)`, C `eq32(ld32[(phi@141:esi + 12)], 0)` | Compare the predicate width, operands and test order at the listed branch. |
| `ass_midi` | `MIDI_PlaySong` | 708 | SOURCE | `1:5be60` effect #1 differs. DOS `call USRHOOKS_GetMem(eax=(obj2 + 0x9558), edx=(28*phi@5bddb:eax), ebx=phi@5bdf9:ebx, edi=phi@5bdf9:edi)`, C `call USRHOOKS_GetMem(eax=(obj2 + 0x9558), edx=(28*phi@83f:eax), ebx=phi@800:eax, edi=phi@85d:edi)` | Check the callee, evaluation order and argument values at the listed call. |
| `ass_multivoc` | `MV_DeleteDeadVoices` | 135 | UNKNOWN | `1:5981b` unsupported instruction with observable effects. DOS `opaque0()`, C `opaque0()` | Inspect unsupported instructions before interpreting subsequent differences. |
| `ass_multivoc` | `MV_ServiceVoc` | 332 | ALLOCATION-ONLY | Modeled effects agree. | Inspect live ranges and stack homes at the verifier's first byte difference. |
| `ass_multivoc` | `MV_Kill` | 184 | UNKNOWN | `1:59a95` unresolved or unsupported value in effect. DOS `st32 [(UNK(pushfd pushfd) + 4)] = ret(1:599ce)`, C `st32 [(UNK(pushfd pushfd) + 4)] = ret(1:599ce)` | Inspect unsupported instructions before interpreting subsequent differences. |
| `ass_multivoc` | `MV_AllocVoice` | 201 | LAYOUT | `1:59ba1` both jumps execute the same verified instruction sequence of a shared epilogue. | Compare the linked displacement after restoring the neighboring function placement. |
| `ass_multivoc` | `MV_SetPitch` | 112 | LAYOUT | `1:59c3e` both jumps execute the same verified instruction sequence of a shared epilogue. | Compare the linked displacement after restoring the neighboring function placement. |
| `ass_multivoc` | `MV_SetMixMode` | 509 | ALLOCATION-ONLY | Modeled effects agree. | Inspect live ranges and stack homes at the verifier's first byte difference. |
| `ass_multivoc` | `MV_StopPlayback` | 172 | LAYOUT | `1:5a0fd` both jumps execute the same verified instruction sequence of a shared epilogue. | Compare the linked displacement after restoring the neighboring function placement. |
| `ass_multivoc` | `MV_StartRecording` | 213 | UNKNOWN | `1:5a1d6` unsupported terminator. DOS `falls off decoded range`, C `falls off decoded range` | Check unresolved references and the expression or join named in the diagnostic. |
| `ass_multivoc` | `MV_StartDemandFeedPlayback` | 589 | SOURCE | `1:5a434` stack cleanup. DOS `ret 4`, C `ret 16` | Trace the return value back to its defining load or arithmetic. |
| `ass_multivoc` | `MV_GetNextVOCBlock` | 519 | UNKNOWN | `1:5a4bd` unsupported instruction with observable effects. DOS `opaque0()`, C `opaque0()` | Inspect unsupported instructions before interpreting subsequent differences. |
| `ass_multivoc` | `MV_PlayVOC` | 520 | UNKNOWN | `1:5a86d` unresolved stack delta at return. DOS `None`, C `16` | Check the reported control-flow edges and function bounds. |
| `ass_multivoc` | `MV_CalcVolume` | 269 | LAYOUT | `1:5aa0e` both jumps execute the same verified instruction sequence of a shared epilogue. | Compare the linked displacement after restoring the neighboring function placement. |
| `ass_multivoc` | `MV_CalcPanTable` | 189 | LAYOUT | `1:5aacb` both jumps execute the same verified instruction sequence of a shared epilogue. | Compare the linked displacement after restoring the neighboring function placement. |
| `ass_multivoc` | `MV_Shutdown` | 269 | UNKNOWN | `1:5af1a` unsupported instruction with observable effects. DOS `opaque0()`, C `opaque0()` | Inspect unsupported instructions before interpreting subsequent differences. |
| `ass_task_man` | `TS_SetTimerToMaxTaskRate` | 60 | UNKNOWN | `1:55ccb` unresolved stack delta at return. DOS `None`, C `None` | Inspect unsupported instructions before interpreting subsequent differences. |
| `ass_task_man` | `TS_Startup` | 200 | ALLOCATION-ONLY | Modeled effects agree. | Inspect live ranges and stack homes at the verifier's first byte difference. |
| `ass_task_man` | `TS_AddTask` | 156 | ALLOCATION-ONLY | Modeled effects agree. | Inspect live ranges and stack homes at the verifier's first byte difference. |
| `game_sounds` | `spritesound` | 464 | ALLOCATION-ONLY | Modeled effects agree. | Inspect live ranges and stack homes at the verifier's first byte difference. |
| `game_gamedef` | `transnum` | 640 | ALLOCATION-ONLY | Modeled effects agree. | Inspect live ranges and stack homes at the verifier's first byte difference. |
| `game_gamedef` | `parsecommand` | 3,536 | UNKNOWN | `1:2edc1` unsupported terminator. DOS `('sx', 32, ('sum', 16, 1, ((1, ('tr', 16, ('callret', 32, ('call', 32, '1:2e920', (), (), (('Mentry',), ()))))),)))`, C `indirect jump dword ptr cs:[edx + 0x56c]` | Check the reported control-flow edges and function bounds. |
| `game_gamedef` | `move_actor` | 1,024 | SOURCE | `1:304ff` branch predicate. DOS `eq8((tr8(shr16(ld16[sprite+22 + (38*sx32(tr16(in_eax)))], 8)) and 2), 0)`, C `eq8((2 and tr8(shr32(sx32(ld16[sprite+22 + (38*sx32(tr16(in_eax)))]), 8))), 0)` | Compare the predicate width, operands and test order at the listed branch. |
| `game_gamedef` | `parse` | 6,112 | UNKNOWN | `1:308f9` unsupported terminator. DOS `indirect jump dword ptr cs:[ebx + 0x307a8]`, C `indirect jump dword ptr cs:[ebx + 0x19fc]` | Check the reported control-flow edges and function bounds. |
| `game_gamedef` | `execute` | 6,796 | SOURCE | `1:32312` effect #0 differs. DOS `st32 [hittype+8 + (20*sx32(tr16(in_eax)))] = (ld32[hittype+8 + (20*sx32(tr16(in_eax)))] + 1)`, C `st16 [sprite+20 + (38*sx32(tr16(in_eax)))] = 0` The compared memory histories diverge. | Check the ordering of loads and stores that may alias. |
| `multi` | `neton` | 949 | UNKNOWN | `1:47c94` unsupported instruction with observable effects. DOS `opaque0()`, C `opaque0()` | Inspect unsupported instructions before interpreting subsequent differences. |
| `multi` | `netsend` | 594 | ALLOCATION-ONLY | Modeled effects agree. | Inspect live ranges and stack homes at the verifier's first byte difference. |
| `multi` | `comsend` | 141 | SOURCE | `1:48697` effect #0 differs. DOS `st16 [ld32[outbufend+0]] = ins16((ld16[ld32[outbufend+0]] + 1), (tr8(shr32((ins32(..., ...) + 1), 8)) and 15))`, C `st16 [ld32[outbufend+0]] = ins16((ld16[ld32[outbufend+0]] + 1), (tr8(shr32((ins32(..., ...) + 1), 8)) and 15))` DOS (tr8(shr32((ins32(phi@4868b:edx, ld16[ld32[outbufend+0]]) + 1), 8)) and 15) \| C (tr8(shr32((ins32(phi@b4b:edx, ld16[ld32[outbufend+0]]) + 1), 8)) and 15) [join value edx?. | Check the width, destination, and value of the listed store or effect. |
| `multi` | `netinitconnection` | 484 | SOURCE | `1:48ab5` branch predicate. DOS `lt_s32(phi@48aa8:edi, (sx32(ld16[numplayers+0]) + -1))`, C `lt_s32(phi@f54:edi, (sx32(ld16[numplayers+0]) + -1))` The compared memory histories diverge. | Compare the predicate width, operands and test order at the listed branch. |
| `multi` | `sendpacket` | 1,026 | SOURCE | `1:48da4` branch predicate. DOS `lt_s32((phi@48da6:ebx + -1), 1)`, C `lt_s32(phi@1240:ebx, 1)` | Compare the predicate width, operands and test order at the listed branch. |
| `multi` | `getpacket` | 1,409 | SOURCE | `1:49156` effect #0 differs. DOS `st8 [(sx32(ins16((ld16[outbufindex+0 + (4*sx32(ins16(ins16(ld16[inbuf+0], ld8[(ld32[inbuf+0] + sx32(ld16[ld32[inbu?`, C `st8 [(sx32((4095 and (ld16[outbufindex+0 + (4*sx32(ins16(ins16(ld16[inbuf+0], ld8[(ld32[inbuf+0] + sx32(ld16[ld32[?` | Check the width, destination, and value of the listed store or effect. |
| `multi` | `processreservedmessage` | 644 | ALLOCATION-ONLY | Modeled effects agree. | Inspect live ranges and stack homes at the verifier's first byte difference. |
| `game_menues` | `initmenus` | 624 | SOURCE | `1:2c352` effect #38 differs. DOS `st16 [menu_ordering+8] = 1127`, C `st16 [menu_instructions+4] = 1` | Check the width, destination, and value of the listed store or effect. |
| `game_menues` | `probe` | 400 | ALLOCATION-ONLY | Modeled effects agree. | Inspect live ranges and stack homes at the verifier's first byte difference. |
| `game_menues` | `draw_and_probe` | 1,136 | UNKNOWN | `1:2ca3f` comparison depended on unsupported values. | Check unresolved references and the expression or join named in the diagnostic. |
| `game_menues` | `menus` | 4,912 | UNKNOWN | `1:2df8b` unresolved or unsupported value in effect. DOS `call probe(; stack: ld32[menu_pause+0], ld32[menu_pause+4], ld32[menu_pause+8], cat32(ld16[menu_pause+c], cat16(ld?`, C `call probe(; stack: ld32[menu_pause+0], ld32[menu_pause+4], ld32[menu_pause+8], cat32(ld16[menu_pause+c], cat16(ld?` | Check the reported control-flow edges and function bounds. |
| `game_menues` | `animate_water_ripples` | 1,248 | SOURCE | `1:2e35c` effect #0 differs. DOS `st32 [ripple_count+0] = (ld32[ripple_count+0] + -1)`, C `st32 [ripple_x+0 + phi@20a1:sp-36] = ld32[ring_count+7c + phi@20a1:sp-40]` The compared memory histories diverge. | Check the ordering of loads and stores that may alias. |
| `game_sector` | `activatewarpelevators` | 464 | ALLOCATION-ONLY | Modeled effects agree. | Inspect live ranges and stack homes at the verifier's first byte difference. |
| `game_sector` | `operatesectors` | 10,000 | SOURCE | `1:15a1b` branch predicate. DOS `lt_u16((ins16(ld16[sector+1f + (37*sx32(tr16(in_eax)))], (127 and tr8(shr32(ins32(..., ...), 8)))) + -9), 21)`, C `lt_u16((ins16(ld16[sector+1f + (37*sx32(tr16(in_eax)))], (tr8(shr32(ins32(..., ...), 8)) and 127)) + -9), 21)` | Compare the predicate width, operands and test order at the listed branch. |
| `game_sector` | `operateactivators` | 4,624 | UNKNOWN | `1:182db` unsupported terminator. DOS `indirect jump dword ptr cs:[ebp + 0x181bc]`, C `indirect jump dword ptr cs:[ebp + 0x30e0]` | Check unresolved references and the expression or join named in the diagnostic. |
| `game_sector` | `operateforcefields` | 400 | ALLOCATION-ONLY | Modeled effects agree. | Inspect live ranges and stack homes at the verifier's first byte difference. |
| `game_sector` | `playerkeys` | 7,344 | SOURCE | `1:19738` branch predicate. DOS `eq8((ld8[player_bits+2 + (4*sx32(tr16(in_eax)))] and 1), 0)`, C `eq8((ld8[player_bits+2 + (4*sx32(tr16(in_eax)))] and 1), 0)` The compared memory histories diverge. | Compare the predicate width, operands and test order at the listed branch. |
| `game_sector` | `checkhitswitch` | 1,728 | SOURCE | `1:1b9d4` branch predicate. DOS `eq16((tr16(phi@1b9c7:ebx) + -ld16[sprite+20 + (38*sx32(tr16(phi@1b9c7:ecx)))]), 0)`, C `eq16((tr16(phi@672a:esi) + -ld16[sprite+20 + (38*sx32(tr16(phi@672a:ecx)))]), 0)` | Compare the predicate width, operands and test order at the listed branch. |
| `game_sector` | `checkhitwall` | 1,648 | SOURCE | `1:1bf91` branch predicate. DOS `lt_s16(tr16(phi@1bf71:ecx), sx16(ld8[wall+e + (35*sx32(tr16(phi@1bf71:ebx)))]))`, C `lt_s32(phi@6cf8:ebx, sx32(ld8[wall+e + (35*sx32(tr16(phi@6cf8:ecx)))]))` | Compare the predicate width, operands and test order at the listed branch. |
| `game_sector` | `checkhitceiling` | 1,088 | UNKNOWN | `1:1c3a8` comparison depended on unsupported values. | Check the reported control-flow edges and function bounds. |
| `game_sector` | `checksectors` | 10,736 | SOURCE | `1:1c695` branch predicate. DOS `eq8((ld8[player_bits+0 + (4*sx32(tr16(in_eax)))] and 16), 0)`, C `eq32((0x80000010 and ld32[player_bits+0 + (4*sx32(tr16(in_eax)))]), 0)` | Compare the predicate width, operands and test order at the listed branch. |
| `game_player` | `spawn_pistol_tracer` | 592 | SOURCE | `1:1f50f` effect #0 differs. DOS `call hitscan(eax=(ld32[ps+a + (176*sx32(tr16(in_eax)))] + sar32(sx32(ld16[sintable+0 + (2*(2047 and (sx32(ld16[ps+?`, C `call hitscan(eax=(sar32(sx32(ld16[sintable+0 + (2*((sx32((-16*zx16((... and ...)) + ld16[ps+3e + (176*sx32(tr16(in?` | Check the callee, evaluation order and argument values at the listed call. |
| `game_player` | `shoot` | 1,712 | SOURCE | `1:1f6e6` effect #0 differs. DOS `call hitscan(eax=ld32[sprite+0 + (38*sx32(tr16(in_eax)))], edx=ld32[sprite+4 + (38*sx32(tr16(in_eax)))], ebx=ld32[?`, C `call hitscan(eax=ld32[sprite+0 + (38*sx32(tr16(in_eax)))], edx=ld32[sprite+4 + (38*sx32(tr16(in_eax)))], ebx=ld32[?` DOS <sx32(ld16[sintable+0 + (2*(2047 and (sx32(ld16[sprite+12 + (38*sx32(tr16(in_eax)))]) + 512)))]), sx32(ld16[sintable+0 + (2*sx32(ins16(ld16[sprite+12 + (38*sx32(tr16(in_e?. | Check the callee, evaluation order and argument values at the listed call. |
| `game_player` | `displayraise` | 288 | UNKNOWN | `1:1fd3a` unresolved or unsupported value in effect. DOS `rep0(local32(), (obj1 + 0x1f280), 5, <Mentry(), <st32 [local32()] = in_eax>>)`, C `rep0(local32(), UNK(unbound reference (0,)), 5, <Mentry(), <st32 [local32()] = in_eax>>)` | Check unresolved references and the expression or join named in the diagnostic. |
| `game_player` | `displayweapon` | 4,096 | SOURCE | `1:1feed` terminator. DOS `br`, C `jmp` | Inspect the first differing control-flow exit or join and its incoming values. |
| `game_player` | `getinput` | 1,056 | SOURCE | `1:20eb9` effect #1 differs. DOS `st32 [player_bits+0 + (4*sx32(ld16[myconnectindex+0]))] = 0`, C `st32 [strafe_velocity+0] = 0` The compared memory histories diverge. | Check the ordering of loads and stores that may alias. |
| `game_player` | `update_air` | 400 | ALLOCATION-ONLY | Modeled effects agree. | Inspect live ranges and stack homes at the verifier's first byte difference. |
| `game_player` | `processinput` | 15,694 | SOURCE | `1:21503` effect #2 differs. DOS `call setsprite(eax=ld16[ps+0 + clob_edi(call clipmove(eax=(176*sx32(tr16(in_eax)) + obj2 + 0x8883a), edx=(176*sx32?`, C `call setsprite(eax=ld16[ps+0 + clob_esi(call clipmove(eax=(176*sx32(tr16(in_eax)) + obj2 + 0x8883a), edx=(176*sx32?` | Check the callee, evaluation order and argument values at the listed call. |
| `game_game` | `update_timer_input` | 448 | SOURCE | `1:0011c` branch predicate. DOS `lt_s32((zx32(ld8[keystatus+0 + zx32(ld8[keys+2])]) or zx32(ld8[keystatus+cb]) or zx32(ld8[keystatus+4b])), 1)`, C `eq32((zx32(ld8[keystatus+0 + zx32(ld8[keys+2])]) or zx32(ld8[keystatus+cb]) or zx32(ld8[keystatus+4b])), 0)` | Compare the predicate width, operands and test order at the listed branch. |
| `game_game` | `gametext` | 272 | SOURCE | `1:0284a` terminator. DOS `ret`, C `jmp` | Inspect the first differing control-flow exit or join and its incoming values. |
| `game_game` | `numtext_relative` | 304 | SOURCE | `1:0298b` branch predicate. DOS `eq8(ld8[phi@2987:esi], 0)`, C `eq8(ld8[tempbuf+0], 0)` | Compare the predicate width, operands and test order at the listed branch. |
| `game_game` | `strget` | 1,024 | SOURCE | `1:02aca` effect #0 differs. DOS `st8 [input_text_length+0] = (ld8[input_text_length+0] + 1)`, C `st8 [input_text+0 + zx32(ld8[input_text_length+0])] = ld8[scan_ascii+0 + sx32(tr16(phi@680:eax))]` The compared memory histories diverge. | Check the ordering of loads and stores that may alias. |
| `game_game` | `numtext` | 240 | SOURCE | `1:038c7` effect #0 differs. DOS `call overwritesprite(eax=phi@3864:edi, edx=(phi@3864:sp-24 + sx32(tr16(phi@38ad:edx))), ebx=(zx16(ld8[phi@3864:esi?`, C `call overwritesprite(eax=phi@bb4:edi, edx=(phi@bb4:sp-24 + sx32(tr16(phi@bfd:edx))), ebx=(zx16(ld8[phi@bb4:esi]) +?` | Check the callee, evaluation order and argument values at the listed call. |
| `game_game` | `draw_meter` | 144 | ALLOCATION-ONLY | Modeled effects agree. | Inspect live ranges and stack homes at the verifier's first byte difference. |
| `game_game` | `draw_water_distortion` | 192 | ALLOCATION-ONLY | Modeled effects agree. | Inspect live ranges and stack homes at the verifier's first byte difference. |
| `game_game` | `update_menu_cursor` | 448 | SOURCE | `1:0a73d` effect #2 differs. DOS `call getmousevalues(eax=local32(), edx=local32(), ebx=(obj2 + 0xa5398), ecx=zx16((tr8(sar32(sx32(ld16[sintable+0 +?`, C `call getmousevalues(eax=local32(), edx=local32(), ebx=(obj2 + 0xa5398), ecx=ins16(tr16(in_ecx), (tr8(sar32(sx32(ld?` | Check the callee, evaluation order and argument values at the listed call. |
| `game_game` | `write_demo` | 272 | SOURCE | `1:0b94f` DOS effect #0 has no partner. DOS `st8 [demo_filename+7] = 48`, C `<none>` | Check the width, destination, and value of the listed store or effect. |
| `game_actors` | `hitradius` | 304 | UNKNOWN | stack addresses escape: escaped locals modelled as memory, invalidated by every call and pointer store; frame offsets renamed bijectively | Check unresolved references and the expression or join named in the diagnostic. |
| `game_actors` | `movesector` | 368 | SOURCE | `1:2588f` branch predicate. DOS `lt_s16((ld16[local32()] + 1), ld16[local32()])`, C `lt_s16((ld16[local32()] + 1), ld16[local32()])` The compared memory histories diverge. | Compare the predicate width, operands and test order at the listed branch. |
| `game_actors` | `wakeactors` | 416 | UNKNOWN | stack addresses escape: escaped locals modelled as memory, invalidated by every call and pointer store; frame offsets renamed bijectively | Check unresolved references and the expression or join named in the diagnostic. |
| `game_actors` | `ifsquished` | 384 | SOURCE | `1:25b83` effect #2 differs. DOS `st8 [ps+ad + (176*sx32(tr16(in_edx)))] = (ld8[ps+ad + (176*sx32(tr16(in_edx)))] + 1)`, C `st8 [ps+ad + (176*sx32(tr16(in_edx)))] = (ld8[ps+ad + (176*sx32(tr16(in_edx)))] + 1)` The compared memory histories diverge. | Check the ordering of loads and stores that may alias. |
| `game_actors` | `guts` | 448 | ALLOCATION-ONLY | Modeled effects agree. | Inspect live ranges and stack homes at the verifier's first byte difference. |
| `game_actors` | `moveactors` | 12,320 | SOURCE | `1:25f46` effect #0 differs. DOS `call findplayer(eax=tr16(phi@25f2c:sp-52), edx=local32(), esi=(38*sx32(tr16(phi@25f2c:sp-52))))`, C `call findplayer(eax=tr16(phi@dcc:sp-32), edx=local32(), esi=phi@dcc:esi)` | Check the callee, evaluation order and argument values at the listed call. |
| `game_actors` | `moveweapons` | 1,456 | SOURCE | `1:28f61` effect #0 differs. DOS `call findplayer(eax=tr16(phi@28f49:edi), edx=local32(), esi=(38*sx32(tr16(phi@28f49:edi))))`, C `call findplayer(eax=tr16(phi@3de9:edi), edx=local32(), esi=sx32(tr16(phi@3de9:edi)))` | Check the callee, evaluation order and argument values at the listed call. |
| `game_actors` | `moveeffects` | 3,984 | SOURCE | `1:29531` effect #0 differs. DOS `call findplayer(eax=tr16(phi@294f9:edi), edx=local32(), esi=(20*sx32(tr16(phi@294f9:edi)) + obj2 + 0x893e0))`, C `call findplayer(eax=tr16(phi@4399:edi), edx=local32(), esi=(20*sx32(tr16(phi@4399:edi)) + obj2 + 0x893e0))` | Check the callee, evaluation order and argument values at the listed call. |
| `game_actors` | `moveplayers` | 1,008 | ALLOCATION-ONLY | Modeled effects agree. | Inspect live ranges and stack homes at the verifier's first byte difference. |
| `game_actors` | `movemusic` | 816 | SOURCE | `1:2a898` branch predicate. DOS `lt_s16(ld16[sprite+20 + (38*sx32(tr16(phi@2a876:esi)))], 1000)`, C `lt_s16(ld16[sprite+20 + (38*sx32(tr16(phi@5716:esi)))], 1000)` The compared memory histories diverge. | Compare the predicate width, operands and test order at the listed branch. |
| `game_actors` | `movestandables` | 3,737 | SOURCE | `1:2abed` branch predicate. DOS `eq16(ld16[sprite+10 + (38*sx32(tr16(phi@2abac:sp-28)))], 57)`, C `eq16(ld16[sprite+10 + (38*sx32(tr16(phi@5a49:esi)))], 57)` | Compare the predicate width, operands and test order at the listed branch. |
