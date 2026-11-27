# Watcom 9.5 code generation in LameDuke

This catalogue uses the DOS-hosted Watcom 9.5 GA `engine`, `game_oe`, and `multi` profiles. Apogee examples use the DOS-hosted Watcom 10.0 GA `audiolib` profile. The data-only `game` profile has no function examples.

**PROVEN** denotes a scoped observation with STRICT examples, with DOS A/B or compiler trace evidence where cited. **STRONG** denotes a supported inference or a controlled observation with uncertain generality. **HYPOTHESIS** marks a possible explanation that still needs a matching control.

Addresses are `obj:offset`. A source line is a compiler debug association. Scheduled and inlined sequences can include work from other statements. See [IDIOMS.md](IDIOMS.md) for instructions and validation.

| Profile | Install | Default profile flags |
|---|---|---|
| `engine` | 9.5 GA | `-4r -s -or` |
| `game_oe` | 9.5 GA | `-4r -s -otexan -oe=3000` |
| `multi` | 9.5 GA | `-5r -s` |
| `audiolib` | 10.0 GA | `-zq -w4 -d1 -omaxnet -zp4 -5r -DLIBVER_ASSREV=19950518` |

GUS, GUSMIDI, and MULTIVOC have registered flag overrides. See [neutrality.json](../neutrality.json) for each unit’s effective flags.

## Switch dense

**PROVEN** · engine, game_oe, multi, audiolib

Dense dispatch can use a bounds check and a CS-relative jump table.

- **Conditions** — Several distinct targets in a compact value range.
- **Source form** — switch (hittype[i][0]) with four distinct arms.
- **Emitted code** — CMP selector,3; JA default; JMP CS:[selector*4+table]. A nonzero first case adds an index bias.
- **Counterexample or limit** — The DOS fixture with three contiguous cases used comparisons in all four profiles. Four contiguous cases used tables in all four.
- **STRICT examples** — `movefx` `1:2aa58` (`src/game/actors.c:1425`), `parsecommand` `1:2edab` (`src/game/gamedef.c:222`)
- **Evidence** — probes.json / sw_3_1 and sw_4_1

## Switch cost

**STRONG** · engine, game_oe, multi, audiolib

Case density and the optimization profile influence dispatch selection.

- **Conditions** — Distinct cases with identical arm structure, all calling separate external functions, and the same external default.
- **Source form** — switch (x) with cases 100 + i*step for i=0 through n-1.
- **Emitted code** — For stride 2, only n=4 used a table in engine/multi. game_oe/audiolib used tables for n=4,5,6,8,10,16. For stride 3, only game_oe/audiolib n=4,5 used tables. All stride 4,8,16 fixtures used comparisons.
- **Counterexample or limit** — At stride 2, n=12 uses a comparison tree in all profiles. There is no monotone case-count threshold in this fixture.
- **STRICT examples** — `addweapon` `1:25187` (`src/game/actors.c:55`), `movefx` `1:2aa58` (`src/game/actors.c:1425`)
- **Evidence** — probes.json / case_matrix

## Switch comparisons

**PROVEN** · game_oe

Compare trees retain the selector width and can encode adjacent labels as a range.

- **Conditions** — Case selector can remain in a narrow live value, and contiguous case labels share a body.
- **Source form** — switch (tempbuf[0]); switch (wall[i].picnum).
- **Emitted code** — CMP AL in getpackets. CMP AX plus JB/JBE around a contiguous case range in animatewalls.
- **Counterexample or limit** — parsecommand uses a word bounds test followed by a long index into a dispatch table.
- **STRICT examples** — `getpackets` `1:00d35` (`src/game/game.c:430`), `animatewalls` `1:155b3` (`src/game/sector.c:176`), `parsecommand` `1:2edab` (`src/game/gamedef.c:222`)
- **Evidence** — docs/FINDINGS.md / Game code-shape rules and SECTOR round 3

## Switch empty labels

**STRONG** · game_oe

Keep labels with empty bodies when reconstructing a compare tree.

- **Conditions** — A label falls through into a shared body, or shares the default destination.
- **Source form** — case 1: case 7: ... in moveeffectors. Group actual empty and contiguous labels.
- **Emitted code** — Changing the case-value set changes the split keys and block order even when the high-level effect is unchanged for tested inputs.
- **Counterexample or limit** — The specific 1/7 split at 15 is local evidence from the still-NEAR moveeffectors.
- **STRICT examples** — `animatewalls` `1:155b3` (`src/game/sector.c:176`)
- **Evidence** — docs/FINDINGS.md / GAME inlining structure

## Table before entry

**PROVEN** · engine, game_oe, multi, audiolib

Switch tables can precede the entry label in CODE.

- **Conditions** — An indexed indirect jump has a relocated intra-segment table base.
- **Source form** — A dense switch with distinct case bodies.
- **Emitted code** — Relocated dwords, then entry alignment padding, then the dispatcher public. Some Watcom 10.0 slots relocate to external tail callees.
- **Counterexample or limit** — Comparison trees have no such prefix. A table with external targets can remain inside the preceding public’s verified extent under the current verifier attribution rule.
- **STRICT examples** — `movefx` `1:2aa58` (`src/game/actors.c:1425`), `parsecommand` `1:2edab` (`src/game/gamedef.c:222`), `FX_SetVolume` `1:5e33b` (`src/audiolib/fx_man.c:552`)
- **Evidence** — docs/FINDINGS.md / Jump-table attribution in the verifier

## Small signed loads

**PROVEN** · engine, game_oe

A signed byte or word can extend directly into a long.

- **Conditions** — An 8- or 16-bit signed memory value is needed as a 32-bit operand under -4r.
- **Source form** — (long)sprite[i].sectnum; klabs(ps[snum].look_ang).
- **Emitted code** — MOVSX r32,m16, or MOV AX,m16 followed by CWDE in the EAX-specific pragma path. The signed-byte fixture used MOVSX r32,m8.
- **Counterexample or limit** — The signed global -5r fixtures may read a dword at a negative addend and shift instead.
- **STRICT examples** — `moveplayers` `1:2a66d` (`src/game/actors.c:1364`), `animateknuckles` `1:1fd6d` (`src/game/player.c:220`)
- **Evidence** — docs/FINDINGS.md / Allocator lever and MULTI; probes.json / widenschar and widensshort

## Pentium wide load

**PROVEN** · multi, audiolib

Some -5r signed extensions use a wider, negatively displaced memory read.

- **Conditions** — Promoting a signed narrow global. Watch for a relocation to an adjacent symbol or symbol plus a negative addend.
- **Source form** — i < numplayers - 1 with a signed-short numplayers. Fixture returns a signed char or short as long.
- **Emitted code** — MOV r32,[short-2]; SAR r32,16. The signed-byte fixture uses [char-3] and SAR r32,24.
- **Counterexample or limit** — A volatile short in the same -5r fixture used MOVSX directly. Several ordinary loads in the same modules also use MOVSX.
- **STRICT examples** — `netinitconnection` `1:48a98` (`src/multi/multi.c:502`)
- **Evidence** — docs/FINDINGS.md / MULTI is compiled with -5r; probes.json / widenvolatile

## Small unsigned loads

**PROVEN** · engine, game_oe, multi, audiolib

Zeroing a register followed by a low-byte or low-word load is a promotion idiom.

- **Conditions** — An unsigned narrow load must supply a zero-extended long.
- **Source form** — Unsigned char synctics or key byte used in a wide call/expression.
- **Emitted code** — XOR EAX,EAX; MOV AL,[synctics]. A word uses AX in the fixture. Other allocations can use MOVZX.
- **Counterexample or limit** — Multiple uses, preload timing, and inlining can favor AND r32,255 or a MOVZX instead.
- **STRICT examples** — `ssp` `1:25605` (`src/game/actors.c:138`), `timerhandler` `1:002c7` (`src/game/game.c:391`)
- **Evidence** — probes.json / widenuchar, widenushort; corpus timerhandler inlined calls

## Compare width

**PROVEN** · game_oe

An int-promoted expression can still compare at byte or word width.

- **Conditions** — The optimizer can establish that a narrow operand and bound fit the required comparison.
- **Source form** — sprite[i].picnum == MASTERSWITCH; sprite[i].yvel == 0; totalclock & 16.
- **Emitted code** — CMP m16,imm or CMP r16,m16. A low-bit mask on a long can become TEST m8,imm8.
- **Counterexample or limit** — The same record’s long dimensionmode compares with CMP m32,imm. A cast or declaration affects the usable width.
- **STRICT examples** — `operatemasterswitches` `1:19550` (`src/game/sector.c:623`), `cameratext` `1:0a655` (`src/game/game.c:1993`), `displaystatus` `1:0789b` (`src/game/game.c:1729`)
- **Evidence** — docs/FINDINGS.md / Game code-shape rules

## Enum width

**STRONG** · game_oe

Check replaced enums during a narrow comparison mismatch.

- **Conditions** — A narrow game expression is compared against a named enum constant or macro literal.
- **Source form** — Use the byte/short comparison from the recorded PLAYER/GAMEDEF controls to choose between declarations.
- **Emitted code** — The recorded controls changed 32-bit comparisons back to 16-bit comparisons or byte tests with literal macros.
- **Counterexample or limit** — A global signed-short return expression ss == 5 and ss == enum_five generate the same CMP word and even alias to one entry in all four fixtures.
- **STRICT examples** — `operatemasterswitches` `1:19550` (`src/game/sector.c:623`), `move_actor` `1:304be` (`src/game/gamedef.c:688`)
- **Evidence** — docs/FINDINGS.md / Game code-shape rules; probes.json / cmpliteral versus cmpenum

## Narrow arithmetic

**PROVEN** · game_oe

A narrow result can pull the arithmetic itself down to word width.

- **Conditions** — Inputs are long, but only the low word survives the explicit conversion.
- **Source form** — i = (short)(totalclock - lockclock);
- **Emitted code** — MOV AX,word [totalclock]; SUB AX,word [lockclock].
- **Counterexample or limit** — A full-width relational use reads and compares dwords, as in playback’s clock wait.
- **STRICT examples** — `getinput` `1:20e6e` (`src/game/player.c:454`), `playback` `1:08351` (`src/game/game.c:1827`)
- **Evidence** — STRICT source and operand widths in index

## Signed power of two divide

**PROVEN** · engine, game_oe, multi, audiolib

Signed division by a power of two includes a correction toward zero.

- **Conditions** — A signed 32-bit dividend can be negative, divisor is a positive power of two.
- **Source form** — ((long)menu_y - m.y) / 16 + 1.
- **Emitted code** — Copy dividend to r, SAR r,31; SHL r,4; SBB dividend,r; SAR dividend,4. The +1 remains an INC.
- **Counterexample or limit** — Unsigned /16 used one SHR r,4 in all four fixtures. Signed division by 9 used IDIV.
- **STRICT examples** — `probe` `1:2c77e` (`src/game/menues.c:240`), `animateknuckles` `1:1fd6d` (`src/game/player.c:220`)
- **Evidence** — probes.json / signed16 and unsigned16

## Nonpower divide

**PROVEN** · engine, game_oe, multi, audiolib

Small non-power-of-two constants can retain hardware division.

- **Conditions** — A genuinely variable signed long dividend divided by 5, 9 or 20 in the indexed contexts.
- **Source form** — ni = n / 5; klabs(ps[snum].look_ang) / 9.
- **Emitted code** — Prepare EDX with SAR 31, load the divisor in a register, then IDIV. The unsigned fixture clears EDX and uses DIV.
- **Counterexample or limit** — (4 * 9 + 2) / 2 folds to MOV EAX,19. A /16 uses a shift sequence.
- **STRICT examples** — `scratchmarks` `1:03986` (`src/game/game.c:1157`), `animateknuckles` `1:1fd6d` (`src/game/player.c:220`), `kickview` `1:1f2e9` (`src/game/player.c:29`)
- **Evidence** — probes.json / signed9, unsigned9 and folded

## Mul near powers

**PROVEN** · engine, game_oe, multi, audiolib

Some constant products become shift plus add or subtract.

- **Conditions** — A 32-bit product by a constant one away from a power of two.
- **Source form** — zvel = (...) * 31; offs = (x + 1) * (LAVASIZ + 2) + 1;
- **Emitted code** — Copy the input, shift left 5 then subtract the input for 31. In movelava, shift left 6 and add the input to get 65*x, then double for the product by 130. The final doubling appears under the next source line anchor.
- **Counterexample or limit** — The -4r fixture multiplies by 38 with IMUL. Under both -5r profiles the scalar *38 fixture uses shifts, adds and subtraction.
- **STRICT examples** — `shoot` `1:1f873` (`src/game/player.c:141`), `movelava` `1:2e610` (`src/game/menues.c:592`)
- **Evidence** — probes.json / times31, times65 and times38

## Packed index shapes

**PROVEN** · game_oe

Packed records of different sizes produce distinct index arithmetic.

- **Conditions** — The map v5 layout uses sprite/wall/sector strides of 38/35/37. The game player record uses stride 240.
- **Source form** — sprite[i]; wall[i]; sector[sprite[i].sectnum]; ps[p].
- **Emitted code** — Sprite stride 38 often IMUL imm 0x26. Wall stride 35 is 9*i then 4*(9*i)-i. Sector stride 37 is 9*i then 4*(9*i)+i. Player stride 240 is 15*i shifted by 4.
- **Counterexample or limit** — A saved pointer, induction variable, or direct SIB scale (e.g. sintable[index*2]) changes what remains at each access.
- **STRICT examples** — `operatemasterswitches` `1:19550` (`src/game/sector.c:623`), `animatewalls` `1:155b3` (`src/game/sector.c:176`), `moveplayers` `1:2a66d` (`src/game/actors.c:1364`), `displaystatus` `1:0789b` (`src/game/game.c:1729`)
- **Evidence** — STRICT normalized index and src/game/records.h

## Constant folding

**PROVEN** · engine, game_oe

A constant subexpression can disappear into an earlier immediate load.

- **Conditions** — All operands of a parenthesized subexpression are compile-time constants.
- **Source form** — hplc2 = (65536*2048)/yb2[z]; 160L << 16.
- **Emitted code** — MOV EAX,0x08000000 before the variable division. MOV EAX,0x00a00000 for the draw argument.
- **Counterexample or limit** — The variable division still emits IDIV. The calcwalls immediate is moved across the previous statement’s debug anchor.
- **STRICT examples** — `calcwalls` `1:349f4` (`src/engine/ENGINE.C:595`), `calcwalls` `1:34a01` (`src/engine/ENGINE.C:596`), `draw_pause_sign` `1:2e09b` (`src/game/menues.c:469`)
- **Evidence** — STRICT index; probes.json / folded

## Short circuit

**PROVEN** · game_oe, audiolib

Short-circuit operands commonly occupy separate conditional branches in source order.

- **Conditions** — The next operand is evaluated only when the previous operand’s condition permits it.
- **Source form** — sprite[i].picnum == MASTERSWITCH && sprite[i].lotag == low && sprite[i].yvel == 0; flags joined by ||.
- **Emitted code** — Three CMP/JNE pairs for the && guard. A nonzero first operand jumps over the second load for || in BLASTER_BeginBufferedPlayback.
- **Counterexample or limit** — Pure operands or simple Boolean results can compile differently. Branch inversion in blockdiff hides polarity, so check symbolic assembly.
- **STRICT examples** — `operatemasterswitches` `1:19550` (`src/game/sector.c:623`), `BLASTER_BeginBufferedPlayback` `1:5fa49` (`src/audiolib/blastold.c:1119`)
- **Evidence** — STRICT index; probes.json / andalso and orelse

## Boolean materialization

**PROVEN** · engine, game_oe

SETcc materializes a Boolean in a low byte, with widening only where required.

- **Conditions** — A comparison is used as a value by the caller or enclosing arithmetic.
- **Source form** — return strget(...) != 0; ((xvect < 0) << 10).
- **Emitted code** — SETNE AL for the narrow result. SETL AL, AND EAX,255, SHL EAX,10 for an arithmetic use.
- **Counterexample or limit** — A branch consumes the flags without SETcc. The ternary fixture plain ? 1 : 0 uses SETNE, whereas plain ? 11 : 22 uses separate branches and immediate loads.
- **STRICT examples** — `getscorename` `1:14f8b` (`src/game/premap.c:1128`), `getangle` `1:3baad` (`src/engine/ENGINE.C:3195`)
- **Evidence** — STRICT index; probes.json / ternfold and ternconst

## Top tested loop

**PROVEN** · game_oe, multi

Top-tested source can preserve a loop’s early exit and bottom recheck.

- **Conditions** — A list index is initialized before a while loop and tested before dereference.
- **Source form** — while (k >= 0) or j = 1; while (j < length).
- **Emitted code** — A zero-iteration edge before the first traversal and a conditional recheck leading toward the body.
- **Counterexample or limit** — Standalone checkhitswitch variants were identical while the inline layout in checksectors changed. A while can also have a bottom physical test after a jump from the entry.
- **STRICT examples** — `movetransports` `1:0afba` (`src/game/game.c:2088`), `processreservedmessage` `1:49afc` (`src/multi/multi.c:1012`)
- **Evidence** — docs/FINDINGS.md / SECTOR round 3, Other game units round 4 and MULTI is compiled with -5r

## Tail merging

**PROVEN** · engine, game_oe

Identical return paths can share an epilogue across functions.

- **Conditions** — A compatible save set, frame, and value state permit reuse of a neighbouring tail.
- **Source form** — Distinct `return` statements or simple functions with an identical terminal sequence.
- **Emitted code** — The later routine can JMP into the earlier return or restore sequence. Entire identical fixture bodies can have two publics at one offset.
- **Counterexample or limit** — A differing stack frame, live register or branch position can prevent sharing. A neighbour that changes size can break a STRICT function’s shared jump displacement.
- **STRICT examples** — `setredbluepalette` `1:479aa`, `setverts` `1:47699`, `checkcursectnums` `1:14ff5` (`src/game/sector.c:36`)
- **Evidence** — docs/FINDINGS.md / Engine round 4; probes.json / cmpliteral and cmpenum

## Identical fallthrough

**STRONG** · game_oe

Identical blocks can be merged into a sibling that falls through to the common join.

- **Conditions** — Two interchangeable blocks join the same successor and at least one falls through.
- **Source form** — Repeated update_menu_cursor(614); break; tails in switch arms.
- **Emitted code** — A merge leaves one block. With two explicit outgoing jumps both identical blocks survived the recorded mini test.
- **Counterexample or limit** — The suspected shared paths in processinput existed in both builds, so this rule did not explain its 184-byte size deficit.
- **STRICT examples** — `checkcursectnums` `1:14ff5` (`src/game/sector.c:36`), `setverts` `1:47699`
- **Evidence** — docs/FINDINGS.md / Other game units round 4 and PLAYER and SECTOR round 5

## Distinct store order

**PROVEN** · game_oe

Separate stores can fix emitted store order and remain distinct if their values coincide.

- **Conditions** — Multiple stores have equal values or are adjacent in a branch.
- **Source form** — syncvel[snum] = 0; syncsvel[snum] = 0; syncangvel[snum] = 0;
- **Emitted code** — Three stores in the order in getinput. The same value can be shared in a register.
- **Counterexample or limit** — The recorded chained-assignment rewrite changes bytes. resetplayer needed a separate order for equal-valued stores.
- **STRICT examples** — `getinput` `1:20ec2` (`src/game/player.c:462`), `resetplayer` `1:11de0`, `saveplayer` `1:0b630`, `loadplayer` `1:0b700`
- **Evidence** — docs/FINDINGS.md / GAME findings, PLAYER/ACTORS/MENUES round 2 and PREMAP/GAME round 3

## Block boundary schedule

**PROVEN** · game_oe

A basic-block boundary can retain a memory read-modify-write.

- **Conditions** — A memory update is followed by a branch or an inlined body that can affect scheduling.
- **Source form** — reccnt--; do { if (...) ... else ... } while (0); or totalclock++ before keytimerstuff().
- **Emitted code** — SUB [reccnt],EDX or INC [totalclock] survives. Inline expansion starts a new basic block in the recorded timer probe.
- **Counterexample or limit** — Removing the do/while in closedemowrite splits the update and adds 16 bytes. reccnt-- in playback has a register load at its debug anchor.
- **STRICT examples** — `closedemowrite` `1:0b937` (`src/game/game.c:2294`), `timerhandler` `1:002c1` (`src/game/game.c:390`), `playback` `1:081f8` (`src/game/game.c:1813`)
- **Evidence** — docs/FINDINGS.md / Inliner facts and GAME NEAR round 2

## Inline threshold

**PROVEN** · game_oe

-oe=N applies a per-callee expression-tree budget.

- **Conditions** — Same translation unit, callee eligible for user-function inlining. A forward definition is eligible.
- **Source form** — Natural calls under -4r -s -otexan -oe=3000.
- **Emitted code** — Plain -oe used N=20 in the recorded controls. N >=2500 fits the GAME call/inline pattern. Unit floors from oeunits were MENUES >=90, PLAYER >=485, SECTOR roughly >=1520, GAMEDEF roughly >=1330.
- **Counterexample or limit** — ENGINE has no -oe and all 181 intra-engine calls match. One ACTORS addweapon site needs the integrated comma-expression spelling at N=3000.
- **STRICT examples** — `timerhandler` `1:002c7` (`src/game/game.c:391`), `wait_for_players` `1:0580a` (`src/game/game.c:1563`), `drawrooms` `1:33f18`
- **Evidence** — docs/FINDINGS.md / GAME’s inlining is real compiler inlining, All game units and Engine compiled without inlining

## Inline depth

**PROVEN** · game_oe

Inline expansion is one level deep in the recorded cases, including recursive callees.

- **Conditions** — An otherwise eligible same-unit call under -oe=N.
- **Source form** — An eligible caller contains a call to getpackets or a recursive parser path.
- **Emitted code** — The outer callee expands and the copy retains inner calls. parsecommand retains five recursive calls in the oracle.
- **Counterexample or limit** — Genuinely separate units never inline across the object boundary. A recursive call is eligible for one expansion.
- **STRICT examples** — `wait_for_players` `1:0580a` (`src/game/game.c:1563`), `parsecommand` `1:2ed60`, `enterlevel` `1:149b0`
- **Evidence** — docs/FINDINGS.md / oeprobe and oeunits (supersedes the early macro model)

## Inline veto

**STRONG** · game_oe

Owned static data and prior runtime address computation can leave calls intact.

- **Conditions** — Callee owns a literal, static local or initialized local array, or caller evaluates a dynamic address before the call within one expression.
- **Source form** — A GAME callee with a literal or sprite[i].owner = findlocator(...).
- **Emitted code** — Retains an ordinary CALL in the documented controls, even above the size threshold.
- **Counterexample or limit** — Parameter count, static linkage and forward definition alone do not veto inlining. Splitting an expression can change the result.
- **STRICT examples** — `gameexit` `1:00c60`, `bootleg_cache_alloc` `1:2ba3f` (`src/game/sounds.c:51`)
- **Evidence** — docs/FINDINGS.md / Inliner facts and oeprobe

## Inline context

**PROVEN** · game_oe

Two inline copies of the same source need not use identical instructions.

- **Conditions** — Caller liveness and uses extend across several calls to an eligible same-unit function.
- **Source form** — Six keytimerstuff() calls in timerhandler.
- **Emitted code** — At the first copy the byte-load prefix uses XOR EAX/MOV AL then XOR EDX/MOV DL. The fifth inline site uses EDX for consecutive loads then AND EDX,255.
- **Counterexample or limit** — The standalone body has its own prologue and allocation. On the getpackets example, changing one mask operand order altered an inline copy while the standalone match survived.
- **STRICT examples** — `timerhandler` `1:002c7` (`src/game/game.c:391`), `timerhandler` `1:00463` (`src/game/game.c:394`), `timerhandler` `1:005eb` (`src/game/game.c:395`), `timerhandler` `1:00777` (`src/game/game.c:396`), `timerhandler` `1:008ff` (`src/game/game.c:397`), `timerhandler` `1:00a90` (`src/game/game.c:398`), `keytimerstuff` `1:000d0`, `wait_for_players` `1:0580a` (`src/game/game.c:1563`)
- **Evidence** — STRICT code at timerhandler 1:002c7 and 1:008ff; docs/FINDINGS.md / GAME NEAR round 2

## Register call args

**PROVEN** · engine, game_oe, multi, audiolib

The tested Watcom register call puts four integer arguments in EAX, EDX, EBX, ECX.

- **Conditions** — An ordinary __watcall with six integer arguments and no custom pragma.
- **Source form** — sink(side(1), side(2), side(3), side(4), side(5), side(6)).
- **Emitted code** — Calls side(6), side(5), then side(4) to side(1). Pushes the first two results, moves the other results into ECX, EBX, EDX, and leaves the final result in EAX.
- **Counterexample or limit** — Unrelated computations can move between pushes. Variadic printf/open and #pragma aux functions use other arrangements.
- **STRICT examples** — `spawn_blood` `1:256e6` (`src/game/actors.c:151`), `check_file_id` `1:14e03` (`src/game/premap.c:1083`)
- **Evidence** — probes.json / args; STRICT EGS call in spawn_blood

## Stack args and cleanup

**PROVEN** · game_oe

Variadic calls can use stack arguments with caller cleanup.

- **Conditions** — The library call is variadic, even when the format or actual argument count is constant.
- **Source form** — open("FILE_ID.DIZ", O_RDWR | O_BINARY, S_IREAD) and printf(help_usage, argv[0]).
- **Emitted code** — Rightmost argument pushed first, pointer pushed last, CALL, ADD ESP,12 or ADD ESP,8.
- **Counterexample or limit** — Ordinary game calls store four integer arguments in registers and pass excess arguments on the stack.
- **STRICT examples** — `check_file_id` `1:14e03` (`src/game/premap.c:1083`), `comlinehelp` `1:054fe` (`src/game/game.c:1457`)
- **Evidence** — STRICT index

## Old style declaration

**PROVEN** · game_oe

A missing prototype keeps default promotions at the call site.

- **Conditions** — Caller sees an old-style function declaration or implicit int; callee can be in another TU.
- **Source form** — extern int EGS(); as seen by PLAYER; extern int strget(); in PREMAP.
- **Emitted code** — Arguments are built/passed without the prototype’s narrowing conversions, and getscorename tests EAX for the declared int return. Its own narrow Boolean result is SETNE AL.
- **Counterexample or limit** — A prototype with short parameter emitted CWDE in the long-to-short fixture. Actual declarations vary by caller; wholesale old-style conversion lost matches.
- **STRICT examples** — `shoot` `1:1fa60` (`src/game/player.c:177`), `getscorename` `1:14f8b` (`src/game/premap.c:1128`), `movefta` `1:258c0`
- **Evidence** — docs/FINDINGS.md / PLAYER follow-up, PREMAP/GAME round 3 and Unit-wide declaration experiment; probes.json / oldarg and narrowarg

## Allocator savings

**PROVEN** · engine, game_oe

Use counts can move one value ahead of others in allocation.

- **Conditions** — A long-lived local or compiler temporary competes with other values for overlapping register homes.
- **Source form** — playmusic(ps[screenpeek].music) instead of a song local; stage a common tile sum in cacheit.
- **Emitted code** — Different conflict savings/order can change the selected colour and later spills. One known control stages a named long tile and gains a 1,968-byte STRICT function.
- **Counterexample or limit** — A move of source text that preserves backend conflict order can give byte-identical output across many trials.
- **STRICT examples** — `cacheit` `1:02fb0`, `moveplayers` `1:2a470`
- **Evidence** — docs/CG95.md / Validation and engine results; docs/FINDINGS.md / Allocator lever

## Allocator ties

**PROVEN** · engine, game_oe

Register preference on a savings tie depends on traversal, overlap and prior-use state.

- **Conditions** — The allocator sorts conflicts by descending savings and visits eligible register masks in order.
- **Source form** — Check same-priority spills and local scope/declaration order. In calcwalls and hline, homes change with declaration order.
- **Emitted code** — The first equal-score eligible mask stays incumbent unless a challenger was already used and the incumbent was not. The gap sorter can reorder equal-priority records as other records cross them.
- **Counterexample or limit** — A fixed list such as “always EAX then EDX” cannot predict the winner. checkhitswitch is a two-byte near control at equal savings 564; changing visit order also changed its neighbour.
- **STRICT examples** — `calcwalls` `1:3493d`, `hline` `1:39a7f`, `movetransports` `1:0ae90`
- **Evidence** — docs/CG95.md / Breakpoints and recovered decisions; docs/FINDINGS.md / Allocation ties and checkhitswitch follow-up

## Stack homes

**PROVEN** · engine, game_oe

Home order depends on priority and address-taking; declaration order can break ties.

- **Conditions** — Frame slots survive optimization. Trace home priority uses a flag and size; aliases of the same temp have an offset order.
- **Source form** — Address-take s in spawn or reorder equal-priority locals as in hline.
- **Emitted code** — Trace placed spawn’s address-taken s at its last local home ([esp+0x14]) across declarations tested. hline changes the relative homes of r and xr.
- **Counterexample or limit** — A variable may remain in registers or share a home, and different flags or lifetimes can override any declaration-order effect.
- **STRICT examples** — `hline` `1:39a7f`, `calcwalls` `1:3493d`, `shoot` `1:1f660`
- **Evidence** — docs/CG95.md / stack home comparator; docs/FINDINGS.md / PREMAP/GAME round 3 and Engine round 4

## Short address home

**PROVEN** · game_oe

One address-taken short receives a four-byte local slot in the matched frame.

- **Conditions** — Watcom 9.5 GA game_oe and the exact moveweapons local layout.
- **Source form** — short distance; findplayer(i, (long *)&distance);
- **Emitted code** — Slot at ESP+0xc is written as a long while projectile branches compare its signed low word.
- **Counterexample or limit** — The recorded 4-byte array and union variants leave 17 differences. Writing a long through a pointer cast to a short object has undefined behavior in portable C.
- **STRICT examples** — `moveweapons` `1:28f5f` (`src/game/actors.c:1012`), `moveweapons` `1:28fba` (`src/game/actors.c:1021`)
- **Evidence** — docs/FINDINGS.md / ACTORS/PREMAP on gpt-6.2-astra

## Copybuf pragma

**PROVEN** · game_oe

The shape of a pragma aux declaration affects scheduling and register allocation.

- **Conditions** — An inline pragma supplies the string move and register parameter mapping.
- **Source form** — Ken’s copybuf form with parm [esi][edi][ecx], without a modify clause.
- **Emitted code** — REP MOVSD in movelava and compiler unrolling around it, reproducing the 1,248-byte function.
- **Counterexample or limit** — The manual unroll and later modify clause were different source models. klabs without a C prototype also gives a distinctive MOV AX/CWDE path.
- **STRICT examples** — `movelava` `1:2e3d6` (`src/game/menues.c:565`), `animateknuckles` `1:1fd6d` (`src/game/player.c:220`)
- **Evidence** — docs/FINDINGS.md / Ken-Build ancestry and Allocator lever

## Literal pooling

**PROVEN** · engine, game_oe, multi, audiolib

An ordinary literal is addressed by relocation into the unit’s CONST pool and can be reused.

- **Conditions** — Same literal bytes, same unit, ordinary duplicate-string setting.
- **Source form** — string_sink("the same text"); in two procedures.
- **Emitted code** — The fixture emits the same CONST relocation and folds the identical procedures to the same entry. Game open/printf sites push the literal address.
- **Counterexample or limit** — An explicit reuse_duplicate_strings pragma or moving a call across a TU boundary changes pooling. Trailing embedded NUL bytes can affect the pool.
- **STRICT examples** — `check_file_id` `1:14e03` (`src/game/premap.c:1083`), `USRHOOKS_GetMem` `1:2bb71` (`src/game/sounds.c:96`), `loadefs` `1:301e4` (`src/game/gamedef.c:603`)
- **Evidence** — docs/FINDINGS.md / Link layout rules, GAME is two objects and CONST residue; probes.json / string_a–c

## Literal residue

**PROVEN** · engine, game_oe

String alignment padding can inherit compiler record-buffer contents.

- **Conditions** — Alignment gaps in reused covered LEDATA records, especially in literal-heavy GAMEDEF and SOUNDS.
- **Source form** — Use the actual source literal and headers to reproduce the full CONST segment.
- **Emitted code** — Prior LEDATA record contents can survive in covered padding. A record-transition hole is zero-filled by the linker. DOS 9.5 gives zero in the seed bytes observed in the oracle.
- **Counterexample or limit** — NT can seed the buffer from heap residue. Splitting GAMEDEF into .inc files changed about 50 bytes under DOS. Apogee 10.0 uses different residue in some pools.
- **STRICT examples** — `loadefs` `1:301e4` (`src/game/gamedef.c:603`), `USRHOOKS_GetMem` `1:2bb71` (`src/game/sounds.c:96`)
- **Evidence** — docs/CONST_RESIDUE.md and docs/FINDINGS.md / Canonical DOS host

## Ass residue

**STRONG** · audiolib

Watcom 10.0 CONST residue can come from the reused source-read buffer.

- **Conditions** — A covered alignment gap in an Apogee unit after an include read or different predecessor unit.
- **Source form** — Unchanged source with the recorded guest include and source paths.
- **Emitted code** — Source offset model 4096*chunk+2153-round_up_4(DOS path length) predicts seven held-out offsets. Predecessor BLASTOLD reproduces some residue in MUSIC and FX_MAN.
- **Counterexample or limit** — No tested build reproduced all nine remaining pools. Exact 10.0 header versions and make order remain open.
- **STRICT examples** — `DMA_ErrorString` `1:5ec90`, `BLASTER_GetCurrentPos` `1:5f6d0`
- **Evidence** — docs/FINDINGS.md / Watcom 10.0 on the DOS host and Watcom 10.0 CONST residue solver

## Bss order

**PROVEN** · engine, game_oe, multi, audiolib

Watcom orders BSS by size buckets and declaration-slot and identifier-hash rules.

- **Conditions** — File-scope uninitialized definitions; emitted allocation is sensitive to a rename or a new declaration.
- **Source form** — Same variables with reordered declarations; a far pointer or double under -ot can alter segment alignment.
- **Emitted code** — 9.5 rule uses newest 22-slot page first and ascending 12-bit identifier hash modulo 211, with type alignment under -ot and witnessed paragraph alignment for double/far function pointer. 10.0 uses its separately calibrated 25-slot/%241 model.
- **Counterexample or limit** — Using the 9.5 rule for the 10.0 calibration predicted 38/163; the calibrated 10.0 model predicted 163/163.
- **STRICT examples** — `keyhandler` `1:33e5c`, `BLASTER_Init` `1:60770`
- **Evidence** — docs/BSS_LAYOUT.md; docs/FINDINGS.md / Compile environment, Link layout rules and Watcom 10.0 GA _BSS rule

## Volatile access count

**PROVEN** · engine, game_oe, multi, audiolib

Volatile keeps independent accesses and can affect operand selection.

- **Conditions** — A repeated read or write to the same visible volatile object in one expression or block.
- **Source form** — return vol + vol; vol = 1; vol = 2; if (BLASTER_SoundPlaying || BLASTER_SoundRecording).
- **Emitted code** — The first fixture uses MOV and ADD from memory (two reads), while the nonvolatile version loads once and doubles the register. Both volatile stores survive. Both modes select a genuine word access for volatile short promotion.
- **Counterexample or limit** — Engine/multi also keep both nonvolatile stores in this fixture, whereas game_oe/audiolib keep only the second. A memory operand alone cannot identify volatile.
- **STRICT examples** — `BLASTER_BeginBufferedPlayback` `1:5fa49` (`src/audiolib/blastold.c:1119`), `BLASTER_DSP2xx_BeginPlayback` `1:5f7f4` (`src/audiolib/blastold.c:1033`), `timerhandler` `1:002c1` (`src/game/game.c:390`)
- **Evidence** — probes.json / readvolatile, storevolatile and widenvolatile; STRICT volatile uses

## Global register order

**HYPOTHESIS** · engine, game_oe

A simple global register-order rule is unlikely to explain the remaining ties.

- **Conditions** — A function has a late alternate assignment with equal apparent costs in the disassembly.
- **Source form** — Compare neighbouring named and anonymous live ranges before editing the first differing operand.
- **Emitted code** — A local lifetime change may reproduce a tie by reordering the global conflict list. A controlled winning edit still needs a new strict verification.
- **Counterexample or limit** — The 150-trial permutation sweeps did not resolve the closest remaining ties; the trace establishes conditional preferences instead of a universal ranking.
- **STRICT examples** — `movetransports` `1:0ae90`, `calcwalls` `1:3493d`
- **Evidence** — docs/CG95.md; docs/FINDINGS.md / First tools/permute sweep

<!-- cross-version audit -->
## Rules imported from Watcom 10.x

The audit catalogs 575 source rows across eight documents. 125 have a scoped confirmation, 36 have a counterexample, and 414 remain open as transfers to the matched corpus.

Each row in [quirks.json](quirks.json) retains its source range, full imported text, 9.5 scope, and the matching examples or reason it remains open. Confirmations apply only to the claim shown below. A matching C analogue cannot establish a C++ compiler rule, exact allocator internals, or an A/B difference. The 9.5 examples exclude Apogee’s Watcom 10.0 units.

### Confirmed transfers

#### Register

The ordinary integer register ABI uses EAX, EDX, EBX, ECX and spills overflow arguments. Callee saves and custom pragma conventions remain context-dependent.

9.5 STRICT examples include `spawn_blood` `1:256e6` (game_oe), `getangle` `1:3ba86` (engine).
DOS controls include `args` under `engine`.
Imported rows include `ray/COMPILER.md:46`, `ray/COMPILER.md:121`, `skill/watcom.md:43`, `skill/watcom.md:54`.

#### Proto

The caller’s visible type controls narrowing and return tests. PREMAP declares strget as int and tests EAX. The caller-specific declaration rule transfers.

9.5 STRICT examples include `getscorename` `1:14f8b` (game_oe), `shoot` `1:1fa60` (game_oe).
DOS controls include `oldarg` under `game_oe`, `narrowarg` under `game_oe`.
Imported rows include `ray/COMPILER.md:15`, `ray/COMPILER.md:31`, `ray/COMPILER.md:238`, `ray/COMPILER.md:240`, `levers/levers.md:25`, `lanes/codegen_rules_lanes.md:137`, `kegg/compiler-notes.md:140`, `skill/watcom.md:70`.

#### Inlining

With -oe, ordinary eligible same-TU calls expand while standalone publics can remain. This game uses a measured threshold of at least 2500 across GAME, and the 9.5 default is 20 expression-tree nodes.

9.5 STRICT examples include `timerhandler` `1:002c7` (game_oe), `wait_for_players` `1:0580a` (game_oe).
DOS controls include `thin1` under `game_oe`, `thin3` under `game_oe`.
Imported rows include `ray/COMPILER.md:157`, `cyber/codegen_rules.md:26`, `levers/levers.md:18`, `levers/levers.md:19`, `lanes/codegen_rules_lanes.md:13`, `skill/watcom.md:63`.

#### Inlinecopy

The same source can emit different instructions standalone and at different inlined sites. The fixed 10.6 register or stack recipe is a separate hypothesis.

9.5 STRICT examples include `timerhandler` `1:002c7` (game_oe), `keytimerstuff` `1:0011e` (game_oe).
Imported rows include `cyber/codegen_rules.md:76`, `lanes/codegen_rules_lanes.md:27`, `lanes/codegen_rules_lanes.md:61`, `lanes/codegen_rules_lanes.md:78`, `lanes/codegen_rules_lanes.md:87`, `lanes/codegen_rules_lanes.md:101`, `lanes/codegen_rules_lanes.md:150`.

#### Wide16

9.5 -5r promotes a nonvolatile signed short with a dword read at -2 followed by SAR 16. Under -4r, the small control emits MOVSX. The formal OMF and LE target must still resolve to the correct base and addend.

9.5 STRICT examples include `netinitconnection` `1:48a98` (multi).
DOS controls include `widensshort` under `engine`, `widensshort` under `multi`, `widenvolatile` under `multi`.
Imported rows include `ray/COMPILER.md:129`, `ray/COMPILER.md:143`, `skill/watcom.md:59`.

#### Bss general

Declaration context and identifier hash can change 9.5 BSS layout. The 9.5 size/page/hash model is independently calibrated in docs/BSS_LAYOUT.md. The exact 10.x constants differ.

9.5 STRICT examples include `keyhandler` `1:33e5c` (engine).
Imported rows include `kegg/compiler-notes.md:202`.

#### Pool

Ordinary C string literals relocate into CONST and can be shared. The 9.5 verified pools in GAME and GAMEDEF and two equal synthetic literal publics establish the scoped rule. An extern char-array format string is a separate case.

9.5 STRICT examples include `check_file_id` `1:14e03` (game_oe), `loadefs` `1:301e4` (game_oe).
DOS controls include `string_a` under `game_oe`, `string_b` under `game_oe`.
Imported rows include `ray/COMPILER.md:48`, `ray/COMPILER.md:260`, `skill/watcom.md:73`.

#### Internal initializer

The 9.5 compiler can place an initialized local aggregate in CODE ahead of the first public. animateknuckles takes the address of its knuckle_frames image at the PLAYER CODE prefix 1:1f280.

9.5 STRICT examples include `animateknuckles` `1:1fd2b` (game_oe).
Imported rows include `ray/COMPILER.md:154`.

#### Packing

Default one-byte member packing holds in 9.5. The 9.5 fixture returns sizeof=7 for char/int/short with dword at +1. STRICT map accesses independently give stride 38 for sprite and 37 for sector.

9.5 STRICT examples include `operatemasterswitches` `1:19550` (game_oe), `moveplayers` `1:2a66d` (game_oe).
DOS controls include `packsize` under `game_oe`, `packoffset` under `game_oe`.
Imported rows include `ray/COMPILER.md:136`, `kegg/compiler-notes.md:99`.

#### Mask

Word and dword storage can be tested at byte width when the mask fits. The disassembly alone cannot distinguish a plain mask, a cast, or a packed bitfield.

9.5 STRICT examples include `cameratext` `1:0a655` (game_oe), `move_actor` `1:30555` (game_oe).
DOS controls include `bitlong` under `stackd2`, `bitarg` under `stackd2`, `bitarg` under `game_oe`.
Imported rows include `ray/COMPILER.md:32`, `kegg/compiler-notes.md:90`.

#### Compare

Equivalent operand order can change the first load and invert the signed comparison direction. The probe ga<gb versus gb>ga distinguishes them. No left-first rule applies to all binary expressions.

9.5 STRICT examples include `moveplayers` `1:2a66d` (game_oe), `getangle` `1:3bac1` (engine).
DOS controls include `cmpab` under `game_oe`, `cmpba` under `game_oe`, `sumdeeper` under `game_oe`.
Imported rows include `ray/COMPILER.md:34`, `ray/COMPILER.md:249`, `levers/levers.md:39`, `levers/levers.md:44`, `lanes/codegen_rules_lanes.md:25`, `skill/watcom.md:71`.

#### Testzero

The ordinary scalar zero-comparison form selects TEST on the live register in the surveyed 9.5 C sites. The four CMP AL,0 observations are inline strcpy loops in parsecommand, the same kind of exception stated by Rayman.

9.5 STRICT examples include `getangle` `1:3ba94` (engine), `parsecommand` `1:2f728` (game_oe).
DOS controls include `eqbyte` under `game_oe`, `truthbyte` under `game_oe`.
Imported rows include `ray/COMPILER.md:41`, `ray/COMPILER.md:431`, `ray/COMPILER.md:435`, `cyber/codegen_rules.md:22`.

#### D1

Fresh DOS object pairs establish byte, coverage and decoded fixup neutrality for the registered 9.5 C profiles. This covers full segments and ordinary publics. Other debug modes were tested separately.

9.5 STRICT examples include `keyhandler` `1:33e5c` (engine), `netinitconnection` `1:488f5` (multi), `gametext` `1:02750` (game_oe).
Imported rows include `cyber/codegen_rules.md:15`, `skill/watcom.md:74`.

#### Top loop

A source loop that may run zero times has a guard before the first traversal. 9.5 may duplicate the test at the back edge, so the exact borrowed jump skeleton still depends on flags.

9.5 STRICT examples include `processreservedmessage` `1:49afc` (multi), `movetransports` `1:0afba` (game_oe).
DOS controls include `loopfor` under `game_oe`, `loopwhile` under `game_oe`, `loopdo` under `game_oe`.
Imported rows include `ray/COMPILER.md:42`, `cyber/codegen_rules.md:18`, `lanes/codegen_rules_lanes.md:89`, `lanes/codegen_rules_lanes.md:108`.

#### Logical

The 9.5 optimized profiles preserve source short-circuit effects while freely inverting or threading jumps. Three field tests in operatemasterswitches use direct branches with distinct loads.

9.5 STRICT examples include `operatemasterswitches` `1:19550` (game_oe).
DOS controls include `conjunction` under `game_oe`, `disjunction` under `game_oe`.
Imported rows include `levers/levers.md:21`, `lanes/codegen_rules_lanes.md:69`.

#### Callee first

When all arguments call an effectful helper, the tested 9.5 register convention evaluates from the right and emits stack overflow arguments first. Staging the argument as a separate statement can move the call before these pushes.

9.5 STRICT examples include `spawn_blood` `1:256e6` (game_oe).
DOS controls include `args` under `game_oe`, `sideeffect` under `game_oe`.
Imported rows include `lanes/codegen_rules_lanes.md:116`, `kegg/compiler-notes.md:97`.

#### Tail

A later function can enter a shared earlier epilogue. Adjust the owner and its relative placement together. This applies independently of the exact 10.6 size threshold or destructor-specific pattern.

9.5 STRICT examples include `setverts` `1:478bb` (engine), `setredbluepalette` `1:47a55` (engine).
Imported rows include `cyber/codegen_rules.md:30`, `cyber/codegen_rules.md:55`, `cyber/codegen_rules.md:79`, `levers/levers.md:22`, `levers/levers.md:32`, `levers/levers.md:33`, `lanes/codegen_rules_lanes.md:17`.

#### Identical

9.5 can fold complete identical procedures at one OMF offset. The 9.5 short comparison and prefix/postfix controls demonstrate it, while STRICT ENGINE witnesses cross-public tail sharing. This does not claim the exact empty-function placement rule for every TU.

9.5 STRICT examples include `setverts` `1:47699` (engine), `level_load_hook` `1:00290` (game_oe).
DOS controls include `eqbyte` under `game_oe`, `truthbyte` under `game_oe`, `postinc` under `game_oe`, `preinc` under `game_oe`.
Imported rows include `ray/COMPILER.md:264`, `ray/COMPILER.md:495`, `skill/watcom.md:72`.

#### Tree

Compare trees retain case ranges when contiguous labels share a target. A narrow selector can stay in AX or AL. The source labels and signedness control more than the count alone.

9.5 STRICT examples include `animatewalls` `1:155b3` (game_oe), `getpackets` `1:00d35` (game_oe).
Imported rows include `ray/COMPILER.md:43`, `ray/COMPILER.md:176`, `ray/COMPILER.md:308`, `lanes/codegen_rules_lanes.md:19`, `lanes/codegen_rules_lanes.md:29`, `lanes/codegen_rules_lanes.md:128`, `lanes/codegen_rules_lanes.md:138`, `kegg/compiler-notes.md:310`, `kegg/compiler-notes.md:318`.

#### Dense4

For the controlled shape with distinct arms, four contiguous cases produce a table and three produce comparisons. The borrowed universal density cutoff fails in other 9.5 fixtures, including twelve stride-two cases.

9.5 STRICT examples include `movefx` `1:2aa58` (game_oe), `addweapon` `1:25187` (game_oe).
Imported rows include `kegg/compiler-notes.md:315`.

#### Scan switch

The scan-table mechanism transfers to controlled 9.5 C. The engine and multi fixtures use REPNE SCASW followed by a CS-indexed jump for 22 or 24 short cases separated by 27. The same inputs under game_oe choose compare trees. No SCASW dispatcher occurs in the 210 STRICT 9.5 procedures, and case count alone cannot predict the form.

DOS controls include `sw_22_27` under `engine`, `sw_24_27` under `multi`, `sw_22_27` under `game_oe`, `sw_24_3` under `game_oe`.
Imported rows include `cyber/codegen_rules.md:27`.

#### Align4

The dword-aligned entry pattern transfers to the 9.5 -3s -ot -od fixture. The 9.5 engine has byte-aligned publics and game_oe supplies a different alignment, so this remains a flag-specific relationship.

9.5 STRICT examples include `keyhandler` `1:33e5c` (engine), `gametext` `1:02750` (game_oe).
DOS controls include `truthglobal` under `stackotod`.
Imported rows include `kegg/compiler-notes.md:36`.

#### Ledatasize

The 172-byte minimum chunk mechanism occurs in 9.5. A 9.5 CODE census gives both 172-byte and larger near-threshold chunks; short chunks also exist. This confirms the ordinary size boundary without asserting the exact 192-byte import accounting.

9.5 STRICT examples include `keyhandler` `1:33e5c` (engine), `parsecommand` `1:2ed60` (game_oe).
DOS controls include `loopfor` under `stackd2`.
Imported rows include `kegg/compiler-notes.md:153`, `kegg/compiler-notes.md:244`.

#### Intrinsic

Using the Watcom headers under game_oe emits the string-copy template and retains memset as a call in the test. STRICT newscore uses the byte-tail copy. The engine/MULTI convention is different, so an intrinsic prediction needs the actual profile.

9.5 STRICT examples include `newscore` `1:14ed2` (game_oe), `simulateint` `1:48b97` (multi).
DOS controls include `stringcopy` under `game_oe`, `stringcopy4` under `game_oe`, `stringmemset` under `game_oe`, `stringcopy` under `engine`.
Imported rows include `ray/COMPILER.md:45`, `ray/COMPILER.md:149`, `ray/COMPILER.md:325`, `lanes/codegen_rules_lanes.md:135`, `skill/watcom.md:64`, `skill/watcom.md:68`.

#### Pragmas

The general pragma ABI technique transfers where documented original helpers supply the register or string-instruction form. The specific 10.6 Rayman helpers do not establish shared lineage.

9.5 STRICT examples include `keyhandler` `1:33e75` (engine), `movelava` `1:2e3d6` (game_oe).
Imported rows include `ray/COMPILER.md:47`, `ray/COMPILER.md:171`, `skill/watcom.md:75`.

#### Savings

The 9.5 tracer and STRICT cacheit control establish savings-sensitive placement. The 10.6 numeric loop weighting and exact register lists remain separate, unmeasured transfers.

9.5 STRICT examples include `cacheit` `1:02fb0` (game_oe), `movetransports` `1:0ae90` (game_oe).
Imported rows include `ray/COMPILER.md:207`, `lanes/codegen_rules_lanes.md:28`.

#### Allocator

9.5 also uses an unstable gap sorter for descending conflict savings, established by 47 replayed orders in docs/CG95.md. The exact 10.6 prior-GivenRegisters policy is not inferred from machine code.

9.5 STRICT examples include `calcwalls` `1:3493d` (engine), `hline` `1:39a7f` (engine), `cacheit` `1:02fb0` (game_oe).
Imported rows include `ray/COMPILER.md:363`, `ray/COMPILER.md:364`, `causes/CAUSES.md:41`.

#### Homes

9.5 home priority depends on flag, size and the sort input, with alias offsets for the same temp. Matched calcwalls and hline changed homes with declaration order. No fixed declaration-only or ascending-size-only layout follows.

9.5 STRICT examples include `hline` `1:39a7f` (engine), `calcwalls` `1:3493d` (engine), `moveweapons` `1:28f5f` (game_oe).
Imported rows include `ray/COMPILER.md:39`, `ray/COMPILER.md:213`, `ray/COMPILER.md:295`, `ray/COMPILER.md:359`, `cyber/codegen_rules.md:31`, `cyber/codegen_rules.md:68`, `levers/levers.md:56`, `lanes/codegen_rules_lanes.md:77`, `lanes/codegen_rules_lanes.md:117` and 2 additional rows.

#### Path

Keep historical compile staging short. On this repository’s 9.5 NT build the compiler can hang on long stage components, and CONST residue depends on the host. Canonical DOSBox-X removes the cited path sensitivity for the checked 9.5 pools.

9.5 STRICT examples include `loadefs` `1:301e0` (game_oe), `timerhandler` `1:002a0` (game_oe).
Imported rows include `kegg/compiler-notes.md:236`, `skill/watcom.md:102`.

#### Dostools

Pinned MS-DOS Player fails silently for the 9.5 extended C compiler on the recorded controls. The DOSBox-X compile succeeds while MASM continues to use MS-DOS Player. This is a host compatibility result.

9.5 STRICT examples include `keyhandler` `1:33e5c` (engine), `gametext` `1:02750` (game_oe).
Imported rows include `skill/watcom.md:109`.

#### Runtime

Release-unique runtime members discriminate independently of game object code. LameDuke’s 125 CLIB/MATH contributions and pre-patch members identify Watcom 9.5 GA. Apogee’s C profile is kept separate.

9.5 STRICT examples include `getangle` `1:3ba86` (engine), `keyhandler` `1:33e5c` (engine).
Imported rows include `ray/COMPILER.md:123`, `skill/watcom.md:36`.

#### Layout

Leading tables can explain a predecessor extent that looks wrong. 9.5 confirms the real owner by its fixup targets and bound public, rather than accepting a normalized match as proof.

9.5 STRICT examples include `draw_water_distortion` `1:03f50` (game_oe), `operateforcefields` `1:19590` (game_oe), `parsecommand` `1:2ed60` (game_oe).
Imported rows include `ray/COMPILER.md:11`, `ray/COMPILER.md:301`, `causes/CAUSES.md:152`, `levers/levers.md:23`, `levers/levers.md:34`, `levers/levers.md:64`.

#### Stores

Store and scheduling order can affect exact bytes after values and addresses are established. Separating the three getinput stores or reordering equal stores in resetplayer is verified. Exact order for the imported unrelated structures remains untested.

9.5 STRICT examples include `getinput` `1:20ec2` (game_oe), `resetplayer` `1:11de0` (game_oe).
Imported rows include `ray/COMPILER.md:37`, `cyber/codegen_rules.md:10`, `levers/levers.md:54`.

#### Scope

Temporary or local home changes can shift other allocation choices. STRICT cacheit, hline and movetransports establish the scoped family. The imported specific spelling and result register still need a local A/B pair.

9.5 STRICT examples include `cacheit` `1:02fb0` (game_oe), `hline` `1:39a7f` (engine), `movetransports` `1:0ae90` (game_oe).
Imported rows include `ray/COMPILER.md:38`, `ray/COMPILER.md:248`, `levers/levers.md:50`, `levers/levers.md:51`, `lanes/codegen_rules_lanes.md:11`.

### Refuted transfers

These rows have a contrary 9.5 example or a controlled 9.5 counterexample. Their original 10.x scope remains attached in the JSON.

#### Depth

The recorded 10.6 three-level expansion does not transfer. 9.5 builds the tested copies only one level deep, including recursive callers. In the probe thin3 retains a call to thin1 after expanding thin2.

9.5 STRICT examples include `wait_for_players` `1:0580a` (game_oe), `parsecommand` `1:2fa3f` (game_oe).
DOS controls include `thin1` under `game_oe`, `thin2` under `game_oe`, `thin3` under `game_oe`.
Imported rows include `cyber/codegen_rules.md:77`, `lanes/codegen_rules_lanes.md:30`, `lanes/codegen_rules_lanes.md:130`.

#### Threshold

The numeric -oe default and thresholds are version and source dependent. 9.5 A/B controls give a plain -oe threshold of 20 expression-tree nodes, unlike the imported 10.6 default of 100.

9.5 STRICT examples include `timerhandler` `1:002c7` (game_oe).
Imported rows include `cyber/codegen_rules.md:52`, `lanes/codegen_rules_lanes.md:131`.

#### Bss

The transfer of the exact 25-slot, modulo-241 formula fails on 9.5. Its calibrated rule uses 22 slots and modulo 211. Size buckets and name sensitivity survive as a family resemblance.

9.5 STRICT examples include `keyhandler` `1:33e5c` (engine), `calcwalls` `1:3493d` (engine).
Imported rows include `ray/COMPILER.md:146`, `kegg/compiler-notes.md:185`, `kegg/compiler-notes.md:192`, `skill/watcom.md:61`.

#### Pool limit

The 10.x 28-byte pooling limit does not transfer to unpatched 9.5 GA. Under both -or and -ot, 9.5 shares the controlled 29- and 30-byte duplicates and splits 31- and 32-byte copies. Watcom 10.0 splits 29 onward. The controls use distinct external callees to prevent whole-function folding.

9.5 STRICT examples include `check_file_id` `1:14e03` (game_oe).
Imported rows include `ray/COMPILER.md:354`.

#### Residue

The 10.0 main-file 4-KiB buffer formula does not transfer to the 9.5 DOS profiles. The 9.5 verified GAMEDEF pool follows reused OMF record positions plus initial seed bytes. The 10.0 Apogee profile retains its own source-read residue evidence.

9.5 STRICT examples include `loadefs` `1:301e4` (game_oe).
Imported rows include `kegg/compiler-notes.md:218`, `kegg/compiler-notes.md:226`, `kegg/compiler-notes.md:232`, `kegg/compiler-notes.md:341`, `kegg/compiler-notes.md:345`, `kegg/compiler-notes.md:346`, `kegg/compiler-notes.md:348`, `skill/watcom.md:62`.

#### Data alignment

Natural file-scope alignment needs the optimization context. Under 9.5 engine and multi the 9.5 fixture places a short at offset 5, an int at 7 and a double at 11. Under game_oe the same definitions start at 6, 8 and 16.

9.5 STRICT examples include `keyhandler` `1:33e5c` (engine).
DOS controls include `packsize` under `engine`, `packsize` under `game_oe`.
Imported rows include `ray/COMPILER.md:138`, `skill/watcom.md:67`.

#### Compound canonical

9.5 does not always canonicalize commutative square sums. In the register-profile controls x*x+y*y and y*y+x*x retain opposite IMUL order.

9.5 STRICT examples include `shoot` `1:1f6ef` (game_oe).
DOS controls include `sumxy` under `game_oe`, `sumyx` under `game_oe`.
Imported rows include `cyber/codegen_rules.md:48`.

#### Postfix opt

Unused postfix and prefix are not a universal source discriminator under optimized 9.5. The game_oe probe with the same parameter update aliases both spellings to one entry. The -3s -d2 probe does distinguish them.

9.5 STRICT examples include `closedemowrite` `1:0b937` (game_oe).
DOS controls include `postinc` under `game_oe`, `preinc` under `game_oe`, `postinc` under `stackd2`.
Imported rows include `ray/COMPILER.md:197`.

#### Const args

The imported address-MOV-then-PUSH sequence is not a general 9.5 rule. STRICT variadic open pushes a relocated string address directly, and game register calls pass addresses in a parameter register.

9.5 STRICT examples include `check_file_id` `1:14e03` (game_oe), `USRHOOKS_GetMem` `1:2bb71` (game_oe).
DOS controls include `stringsvoid` under `stackd2`.
Imported rows include `kegg/compiler-notes.md:101`.

#### Tableafter

The borrowed -ot after-function placement prediction fails for 9.5 game_oe. STRICT movefx and parsecommand carry relocated leading tables with -ot enabled through -otexan.

9.5 STRICT examples include `movefx` `1:2aa58` (game_oe), `parsecommand` `1:2edab` (game_oe).
Imported rows include `cyber/codegen_rules.md:41`.

#### Alignment

The 10.6 aligned-entry claim does not transfer as stated. 9.5 -ot -od with -3s emits dword-aligned entries; game_oe pads to paragraphs, but a 9.5 -5r -otexan control uses dword-aligned entries while keeping paragraph-aligned _TEXT metadata.

9.5 STRICT examples include `gametext` `1:02750` (game_oe), `nethandler` `1:4869f` (multi).
DOS controls include `eqglobal` under `game_oe`, `eqglobal` under `g5`, `truthglobal` under `stackotod`.
Imported rows include `ray/COMPILER.md:134`, `cyber/codegen_rules.md:7`, `cyber/codegen_rules.md:8`, `cyber/codegen_rules.md:72`, `grinding/grinding.md:21`.

#### Ports

The specific through-DX claim for all immediate I/O does not transfer. STRICT keyhandler uses IN AL,0x60 and OUT 0x61,AL through the known Ken pragma. 9.5 has both immediate and DX forms, depending on the actual C/pragma.

9.5 STRICT examples include `keyhandler` `1:33e75` (engine).
Imported rows include `ray/COMPILER.md:318`.

#### Fpround

The exact 10.x 16-significant-digit grid fails for unpatched 9.5 GA. The same DOS source gives 0.10526315789473684 in 9.5 and 0.1052631578947368 in 10.0 under both -or and -ot. 9.5 is not always correctly rounded either. The corpus has no matching long-decimal literal; this is a controlled compiler comparison.

DOS controls include `some_fp` under `game_oe`.
Imported rows include `ray/COMPILER.md:442`, `ray/COMPILER.md:448`, `ray/COMPILER.md:454`.

#### Release

A generally interchangeable 10.x code generator or binary recipe does not transfer to this target. The accepted 9.5 unpatched engine control gave 88/88 historical object functions; comparative 10.x installs gave 11–13. This does not dispute equality within the other project’s selected TUs.

9.5 STRICT examples include `keyhandler` `1:33e5c` (engine), `drawrooms` `1:33f18` (engine).
Imported rows include `ray/COMPILER.md:159`, `cyber/codegen_rules.md:51`, `skill/watcom.md:40`.

### Untested transfers with targeted evidence

The rows below were checked with an explicit control or corpus selector. Remaining rows have an open result and search context in the JSON. The standalone compiler controls carry no STRICT status.

#### Wide8

The exact memory-to-signed-long byte promotion was not located in STRICT MULTI. The 9.5 fixture confirms the -5r [char-3] plus SAR 24 idiom. Other SAR 24 instructions in MULTI extract the high byte from a true long.

DOS controls include `widenschar` under `engine`, `widenschar` under `multi`.
Imported rows include `skill/watcom.md:66`.

#### Bitfields

The 9.5 STRICT source views packed map flags through masks, with no qualifying equivalent bitfield pair. The byte-container fixture confirms LSB-first layout. Its high-field truth test and an explicit mask both lower to TEST byte,0xfc, so the test shape cannot identify a bitfield alone.

9.5 STRICT examples include `cameratext` `1:0a655` (game_oe).
DOS controls include `flagsread` under `game_oe`, `flagstest` under `game_oe`, `bytemask` under `game_oe`, `flagsstore` under `game_oe`.
Imported rows include `ray/COMPILER.md:40`, `ray/COMPILER.md:141`, `ray/COMPILER.md:186`, `ray/COMPILER.md:245`, `lanes/codegen_rules_lanes.md:71`, `lanes/codegen_rules_lanes.md:72`, `lanes/codegen_rules_lanes.md:126`, `kegg/compiler-notes.md:92`, `kegg/compiler-notes.md:144`.

#### D2abi

There is no 9.5 STRICT C unit built under -3s -d2 -s in this corpus. Under those flags the 9.5 control confirms the full stack prologue, [EBP+0x14] first argument, return spill and caller cleanup.

DOS controls include `oldstack` under `stackd2`, `postinc` under `stackd2`, `packoffset` under `stackd2`.
Imported rows include `kegg/compiler-notes.md:7`, `kegg/compiler-notes.md:9`, `kegg/compiler-notes.md:12`, `kegg/compiler-notes.md:14`, `kegg/compiler-notes.md:16`, `kegg/compiler-notes.md:20`, `kegg/compiler-notes.md:63`, `kegg/compiler-notes.md:85`, `kegg/compiler-notes.md:87` and 2 additional rows.

#### D2push

No matching -3s -d2 versus -od occurrence in the 9.5 STRICT corpus. The 9.5 control with identical source and flags otherwise confirms MOV/PUSH versus memory PUSH.

DOS controls include `stringcopy` under `stackd2`, `stringcopy` under `stackod`.
Imported rows include `kegg/compiler-notes.md:41`, `kegg/compiler-notes.md:43`, `kegg/compiler-notes.md:48`, `kegg/compiler-notes.md:132`, `skill/watcom.md:55`.

#### Narrowpush

The 9.5 STRICT ordinary calls use register profiles. The 9.5 -3s -d2 and -od controls both use MOV/PUSH for short -1 and PUSH -1 for int. The short is sign-extended under the tested signed short prototype.

DOS controls include `shortarg` under `stackd2`, `intarg` under `stackd2`, `shortarg` under `game_oe`, `intarg` under `game_oe`.
Imported rows include `kegg/compiler-notes.md:24`, `skill/watcom.md:56`.

#### D2postfix

The precise -3s -d2 dead-load trigger is absent from 9.5 STRICT. The 9.5 -3s -d2 pair reproduces it. Under game_oe these postfix/prefix controls coalesce to one body, so the rule cannot be applied unconditionally to this target.

DOS controls include `postinc` under `stackd2`, `preinc` under `stackd2`, `postinc` under `game_oe`, `preinc` under `game_oe`.
Imported rows include `kegg/compiler-notes.md:46`, `kegg/compiler-notes.md:53`, `kegg/compiler-notes.md:64`, `kegg/compiler-notes.md:66`, `kegg/compiler-notes.md:68`, `skill/watcom.md:57`.

#### D2loop

The exact unrotated -3s -d2 pattern has no 9.5 STRICT occurrence. The controlled -3s -d2 loop shows the same split test/step/body. In 9.5 game_oe both for and while have an early guard and the same conditional back edge.

DOS controls include `loopfor` under `stackd2`, `loopwhile` under `stackd2`, `loopfor` under `game_oe`, `loopwhile` under `game_oe`.
Imported rows include `kegg/compiler-notes.md:30`, `kegg/compiler-notes.md:294`, `kegg/compiler-notes.md:295`, `kegg/compiler-notes.md:296`, `kegg/compiler-notes.md:298`, `kegg/compiler-notes.md:303`, `kegg/compiler-notes.md:308`.

#### D2logic

There is no eligible STRICT -3s -d2 unit. The 9.5 debug control reproduces the && trampoline and the nested-if direct exits. Under game_oe it emits direct exits in both forms.

9.5 STRICT examples include `operatemasterswitches` `1:19550` (game_oe).
DOS controls include `nested` under `stackd2`, `conjunction` under `stackd2`, `nested` under `game_oe`, `conjunction` under `game_oe`.
Imported rows include `kegg/compiler-notes.md:83`, `kegg/compiler-notes.md:260`, `kegg/compiler-notes.md:269`, `kegg/compiler-notes.md:276`, `kegg/compiler-notes.md:277`, `kegg/compiler-notes.md:278`, `kegg/compiler-notes.md:279`, `kegg/compiler-notes.md:280`, `kegg/compiler-notes.md:281` and 6 additional rows.

#### D2assign

The exact dereference-assignment reload does not occur in the STRICT 9.5 register-profile corpus. The -3s -d2 fixture reloads the stored value; the game_oe version tests the existing register.

9.5 STRICT examples include `clipmove` `1:42312` (engine).
DOS controls include `assigncond` under `stackd2`, `ptrcond` under `stackd2`, `assigncond` under `game_oe`.
Imported rows include `kegg/compiler-notes.md:70`, `kegg/compiler-notes.md:131`.

#### D2flat

The exact 4-wide flat versus nested array pair is absent in 9.5 STRICT. The comparable -3s -d2 fixture confirms two SHL 2 versus one SHL 4. Both become SHL 4 under game_oe.

DOS controls include `arrflat` under `stackd2`, `arrnested` under `stackd2`, `arrflat` under `game_oe`, `arrnested` under `game_oe`.
Imported rows include `kegg/compiler-notes.md:81`.

#### Depth eval

No equivalent pair with only conversion-node depth changed was located in STRICT. The 9.5 multiply-before-add fixture shows that deeper work can lead, while simple memory comparisons keep source operand order.

DOS controls include `sumdeeper` under `game_oe`, `sumdepthrev` under `game_oe`, `cmpab` under `game_oe`, `cmpba` under `game_oe`.
Imported rows include `lanes/codegen_rules_lanes.md:129`, `kegg/compiler-notes.md:74`, `kegg/compiler-notes.md:115`, `skill/watcom.md:58`.

#### Fallthrough

9.5 public return-tail merging is confirmed separately. The imported exact trigger is natural fall-through into an immediately following empty definition. The surveyed 9.5 corpus has no verified witness with that exact source and physical fall-through.

9.5 STRICT examples include `setverts` `1:478bb` (engine).
Imported rows include `ray/COMPILER.md:167`, `ray/COMPILER.md:256`, `cyber/codegen_rules.md:45`, `lanes/codegen_rules_lanes.md:95`.

#### Ledatamath

The index contains final instruction ranges, not first-reference import buffer state for every debug flush. The live objects give a 172-byte chunk threshold witness. The precise 192-byte cut rule and per-page WLINK ordering need a dedicated OMF/link probe.

Imported rows include `kegg/compiler-notes.md:157`, `kegg/compiler-notes.md:164`, `kegg/compiler-notes.md:167`, `kegg/compiler-notes.md:172`, `kegg/compiler-notes.md:176`, `kegg/compiler-notes.md:206`, `kegg/compiler-notes.md:246`, `kegg/compiler-notes.md:248`, `skill/watcom.md:60`.

#### Fp

No floating arithmetic appeared in the STRICT 9.5 indexed units. The 9.5 -fpc control calls __FDD/__FDA while the default emits FLD/FDIV/FADD. No inference about the executable’s complete floating-point profile follows.

DOS controls include `some_fp` under `soft`, `some_fp` under `game_oe`.
Imported rows include `ray/COMPILER.md:162`, `skill/watcom.md:69`.

#### Byte rover

The corpus contains many AH/AL allocations but cannot expose LdStAlloc visit order or prove the same first eligible byte. The 9.5 tracer scopes savings ties; it does not expose this 10.6 rover sequence.

9.5 STRICT examples include `parsecommand` `1:2f66e` (game_oe).
Imported rows include `ray/COMPILER.md:334`, `ray/COMPILER.md:367`, `ray/COMPILER.md:415`, `ray/COMPILER.md:419`, `causes/CAUSES.md:81`.

#### Score ring

An emitted MOV and TEST alone cannot identify the compiler’s equivalence ring. The exact 10.6 Score pass, hard-coded ring tie rule and BP exception need instrumentation in 9.5.

Imported rows include `ray/COMPILER.md:201`, `ray/COMPILER.md:287`, `ray/COMPILER.md:405`, `ray/COMPILER.md:408`, `ray/COMPILER.md:410`.

#### Sched

STRICT source anchors show movement across independent statements, but do not give post-allocation DAG edges, stall costs or FU tables. 9.5 -5r demonstrably changes selected instructions and alignment in controls; it does not identify the borrowed scheduler algorithm.

9.5 STRICT examples include `calcwalls` `1:349f4` (engine).
DOS controls include `loopfor` under `game_oe`, `loopfor` under `g5`.
Imported rows include `ray/COMPILER.md:459`, `ray/COMPILER.md:462`, `ray/COMPILER.md:468`, `ray/COMPILER.md:472`, `causes/CAUSES.md:112`.

### Reproduction

Run `python -B build/idioms/tools/idioms/import_probes.py` and `python -B build/idioms/tools/idioms/version_controls.py` followed by `python -B build/idioms/tools/idioms/import_audit.py`. Source documents are read-only and pinned by hash so a line-specific judgment cannot silently attach to another claim.

The index contributes 210 STRICT 9.5 functions. The OMF chunk census observed 426 172-byte records across the registered 9.5 units. See the [9.5 probe output](../import_probes.json) and [9.5 versus 10.0 controls](../version_controls.json).
