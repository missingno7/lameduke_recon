# GAME contribution survey

Oracle `assets/D3D.EXE` is checked against `config/oracles.json` by `tools/image.py`. Addresses and ranges below are hexadecimal and half-open. The build uses Watcom 9.5b with `-4r -s -otexan`.

## Translation-unit boundary

**STRONG.** One code-bearing GAME contribution spans `1:00010–14fad` (85,917 bytes), plus three linker-fill bytes through `1:14fb0`. The evidence supports one contribution across startup, rendering, simulation, saves, actors and later PREMAP functions. The original link map and source filename are open. An object that contributes only data would be invisible to this code map.

- **PROVEN.** The first two addresses in funcmap are data. LE fixup sites `1:0dafe` and `1:0f04b`, both in `moveeffectors`, address `1:00010` and `1:00013`. The prefix through `1:00020` includes both private tables and alignment. The first executable entry programs the PIT. Its return and instruction fill lead to the second timer function.
- **PROVEN.** `1:14fac` is the final `ret`. The three bytes `1:14fad–14fb0` are zero and have no relocation. The next entry walks the connection list and is SECTOR's `checkcursectnums`. Scanning every 16-byte boundary found two internal sequences resembling a return followed by zero fill. The apparent returns at `0be9e` and `0dd1d` are immediate operands in `moveeffectors`.
- **PROVEN.** The other aligned return tails inside procedures at `025df`, `0342f` and `1037f` lead to blocks entered by their own functions. The boundary at `081c0` follows a return at `081bf` with no available fill slot. `081c0` has its own prologue and references the same playback data and graphics routines as main's attract sequence. CFG and pool evidence favor the same TU there.
- **PROVEN.** `1:000d0–00282` restores its save set and reaches the sole `ret` of `level_load_hook` via instruction fill. Placing `uninittimer` immediately before the hook reproduced the compiler behavior in a control. The original compaction connects the ISR helper and board-load hook in a single code contribution.
- **PROVEN.** 68 distinct targets in `2:00004–006a8` have 93 inbound fixup sites, all from this GAME code. The run follows function order from `gameexit` through `loadlookup`, keys/command-line/main, bonus/scores, user data, save and demo I/O, `prelevel`, `enterlevel`, and the file-ID path. Shared writable filename literals bridge the read/write pairs. Four marker bytes before the first referenced literal have no inbound fixup. At `2:006a8`, the first SOUNDS filename is reached from the SOUNDS pointer table.
- **STRONG.** At the proposed later PREMAP boundary `1:12210`, the prior function returns at `12197`. The following 28 dwords belong to PREMAP's first dispatcher, with a base operand in that dispatcher at `13622`. Instruction fill ends at `12210`. The previous function and `prelevel` also both reset movement/animation state. The layout makes a standalone PREMAP object unlikely. At `11380`, which follows the large sprite initializer, the rendering block resumes within the same compiler-aligned run. The first GAME prefix's references from `0ba30` cross the putative save/actor split at `0ba30`.

### Code tables

The canonical matcher accounts for table bytes in the extent of the preceding public. These are data ranges in the contribution. Two adjacent displayrooms tables appear as a single run of fixup sites in funcmap.

| Range | Consumer | Evidence |
|---|---|---|
| `00010–00020` | `0ba30` | Two referenced private constants and alignment |
| `039fc–03a18` | `03a20` | 7 dwords and base relocation at `03ae2` |
| `0400c–04044` | `04050` | 14 dwords. The operands at `04227` and `047a0` address bases `0400c` and `04028` |
| `0f0f0–0f160` | `0f160` | 28 dwords, base operand `10439` |
| `12198–12208` | `12210` | 28 dwords, base operand `13622` |

## Function inventory

61 entries in address order. 50 are call- or pointer-seeded after excluding the seven data labels. Eleven were recovered from entry, exit and alignment evidence. A question mark denotes an unresolved part of the ABI or identity. Descriptive names introduced here include `update_timer_input`, `network_move_loop`, `numtext_relative`, `numtext_fixed`, `draw_meter`, `wait_for_players`, `playback_demo`, `draw_monitor_overlay`, `spawn_random_effects`, `move_status9`, `render_sprite_camera` and `reset_player_motion`. The ABI column is a survey, with confirmed candidate declarations in `src/game.h`. Integer widths in Ghidra drafts were checked against register consumers where possible. Byte and word uses alone leave the original formal types open.

| Obj1 extent | Name | ABI and identification |
|---|---|---|
| `00020–00080` | `inittimer` † | `void(void)`. Main inlines the PIT divisor and IRQ0 install |
| `00080–000d0` | `uninittimer` † | `void(void)`. Gameexit inlines the PIT and vector restore |
| `000d0–00290` | `update_timer_input` † | `void(void)`. Acceleration and damping. Shares the hook's return |
| `00290–002a0` | `level_load_hook` | `void(void)`. Two calls around board I/O |
| `002a0–00c60` | `timerhandler` | `void __interrupt __far(void)`. Installed vector, Watcom ISR prologue and `iretd` |
| `00c60–00d10` | `gameexit` | `void(char *message)`. I/O teardown and exit |
| `00d10–01120` | `domovethings_multiplayer` | `void(void)` proposed. Applies queued network input and calls simulation steps |
| `01120–018c0` | `moveloop` | `void(void)` proposed. Input packet/tick coordination |
| `018c0–023d0` | `network_move_loop` † | Likely `void(void)`. Host split, packet receive and tick dispatch. Return at `01ee6` with subsequent branch targets through `023c2` |
| `023d0–02400` | `next_text_shade` | `long(void)`. Raynorpat `hudshade` |
| `02400–02630` | `displayquotes` | Player index in AX. Raynorpat name, `operatefta` in survey |
| `02630–026c0` | `FTA` | `void(short quote)` in existing callers |
| `026c0–02750` | `overlaypal` | `void` proposed, three coordinate/tile registers, shade byte and stacked palette byte |
| `02750–02850` | `gametext` | X/Y in EAX/EDX, text pointer in EBX, shade in CL. Returns final x in EAX |
| `02850–02930` | `minitext` | Same argument registers as `gametext`, returns final x |
| `02930–02a40` | `numtext_relative` † | EAX/EDX xy, signed dword in EBX, shade in CL. Calls the decimal converter and screen-size-adjusted glyph drawing |
| `02a40–02e40` | `strget` | X/Y dwords, shade/limit used as bytes. Return tested in EAX by callers. See candidate declaration |
| `02e40–02e90` | `findlocator` | `short(short lotag, short sector)`. Status 7 on canonical list. `lotag` is at sprite+32 |
| `02e90–02fb0` | `EGS` | Returns a sprite index. See `src/game/gamedef.h` for the four register and ten stack parameters |
| `02fb0–03760` | `cacheit` | Likely `void(void)`. Tile-cache marking and `loadtile` loop |
| `03760–03800` | `loadlookup` | `void(void)`. `genspriteremaps` in original survey |
| `03800–038f0` | `numtext` | EAX/EDX xy, signed dword in EBX, shade in CL. Decimal conversion then glyph drawing |
| `038f0–03970` | `numtext_fixed` | Same argument registers as `numtext`, fixed 6-pixel advance |
| `03970–03a20` | `draw_meter` † | EAX/EDX xy, value in EBX, shade in CL. Draws full five-unit cells then remainder |
| `03a20–03f50` | `animatesprites` | `void(short view_angle)` proposed. Action/frame selection on temporary render sprites |
| `03f50–04050` | `draw_water_distortion` | `void(void)`. Displayrooms tables follow its body |
| `04050–04cd0` | `displayrooms` | `void(short player)` in SECTOR. Named `drawview` in Raynorpat |
| `04cd0–054f0` | `nonsharedkeys` | `void(void)` proposed. Function keys and local actions |
| `054f0–05570` | `show_help` | Format call consumes command-line context. Formal shape open |
| `05570–057c0` | `checkcommandline` | EAX argc, EDX argv through direct callers. Flag parsing |
| `057c0–05b60` | `wait_for_players` † | Likely `void(void)`. Expected count, network handshake and escape check |
| `05b60–07890` | `main` | Watcom CRT `__CMain` direct call at `6477d`, register argc/argv. Return type open |
| `07890–081c0` | `displaystatus` | Player index in AX. Entire body renders status and calls the decimal converter or blitter |
| `081c0–08e30` | `playback_demo` † | Likely `void(void)`. Opens demo, enters a level, consumes recorded input, renders frames |
| `08e30–08ea0` | `record` | `void(short player)`. Records five streams and saturates frame count |
| `08ea0–09650` | `domovethings` | `void(void)` proposed. One tick of simulation |
| `09650–0a420` | `dobonus` | `void(void)` proposed. References bonus tile data, displays results |
| `0a420–0a6f0` | `draw_monitor_overlay` | Likely `void(void)`. Draws a pulsing overlay during view transitions |
| `0a6f0–0a8b0` | `update_menu_cursor` | `void(short tile)` in MENUES |
| `0a8b0–0acb0` | `displayhighscores` | `void(void)`. Renders ordered initialized score records |
| `0acb0–0ae90` | `spawn_random_effects` † | Short sprite and count in AX/DX. Random position and insert loop |
| `0ae90–0b520` | `move_status9` | Likely `void(void)`. Walks status 9, handles paired sector transitions |
| `0b520–0b5d0` | `read_userdefs` | `void(void)`. Settings/score reader and defaults |
| `0b5d0–0b630` | `write_userdefs` | `void(void)`. Writes the same three blocks |
| `0b630–0b700` | `saveplayer` | Reads slot from AL. Current MENUES caller declares long slot |
| `0b700–0b810` | `loadplayer` | Reads slot from AL. Current MENUES caller declares long slot |
| `0b810–0b920` | `read_demo` | Slot/index in AL. Reads header, count, motion/tick/bits streams |
| `0b920–0ba30` | `write_demo` | Likely `void(void)`. Uses the same path buffer and streams |
| `0ba30–0f160` | `moveeffectors` | Likely `void(void)`. Status-list dispatch with private prefix tables |
| `0f160–11380` | `spawn` | `short(short source, short tile_or_index)` in GAMEDEF. Operands branch on negative source |
| `11380–116b0` | `render_camera_tile` | `void(long x,long y,long z,short ang,short sect,short tile)` in SECTOR, `ret 8` |
| `116b0–11a10` | `render_sprite_camera` † | Short sprite index in AX. Builds view coordinates from the sprite and redirects rendering into a tile |
| `11a10–11de0` | `render_mirror` | `void(short wall)` in SECTOR. Mirror view renderer |
| `11de0–11f30` | `resetplayer` | `void(short player)` in GAMEDEF. Initializes player state from an index |
| `11f30–12210` | `reset_player_motion` † | Short player index in AX. Clears motion, copies position and sets player sprite. `ret` at `12197` |
| `12210–148a0` | `prelevel` | Likely `void(void)`. Processes loaded map and prepares stage-wide state |
| `148a0–149b0` | `newgame` | `void(unsigned char episode, unsigned char level, unsigned char skill)` in MENUES |
| `149b0–14e00` | `enterlevel` | `void(unsigned char mode)` in MENUES. Builds map path, loads it, calls prelevel |
| `14e00–14e80` | `check_file_id` | Confirmed candidate `unsigned char(void)`. Low byte sets success/error |
| `14e80–14f80` | `newscore` | Likely `void(void)`. Inserts score among initialized records |
| `14f80–14fad` | `getscorename` | `unsigned char(short row)`. Caller sees strget return normalized to one byte |

† Recovered start absent from `funcmap`. In addition to the timer prologues, the survey can independently place entries after `ret` and instruction fill at `018c0`, `02930`, `03970`, `057c0`, `0acb0`, `116b0` and `11f30`. `081c0` follows an exactly aligned return and has a distinct prologue and complete exit. Raynorpat's `displaystatus` resolves the old survey's name conflict at `07890`: that entry ends at `081bf` and has no demo read or level-entry call. The functions before `02930` and after `03800` share text-rendering logic but have independent exits and save frames.

## Data placement

| Obj2 range | Evidence and ownership |
|---|---|
| `00000–00004` | Unreferenced marker bytes. Segment ownership open. |
| `00004–006a8` | **STRONG** GAME CONST pool. End slice `0069c–006a8` compiled and VERIFIED. Lookup filename/remap diagnostic at `000f8`/`00104` bind as externs in this excerpt. In an isolated two-literal pool the first alignment tail differs at `00103`, following residue from the earlier banner. |
| `02304–02364` | GAME bonus tables used only by `dobonus` at `02304` and `02306`. Last nonzero byte at `0231e`. Remaining extent and historical OMF segment class open. |
| `02364–023dc` | PLAYER group, disjoint reference cluster in `1fe40` and `21410`. Last nonzero byte at `023d1`. |
| `023dc–02568`, `02568–02598` | 12 score-string slots of 33 bytes and 12 dword scores. Both I/O routes transfer the exact ranges. **PROVEN** equal to the two trailing parts of the supplied USERDEFS.TMP. |
| `02598–02738` | GAME FTA strings, 13 slots of 32 bytes. `FTA` indexes this range to determine length, `displayquotes` uses it for drawing. |
| `02738–027b8`, `027b8–02838` | Two 128-byte scan-code maps. Both selected by `strget`, both used by main's inline input path. |
| `02838–0291c` | SOUNDS initialized data, separately verified by worker sounds. Main also reads the device-ID table. |
| `0291c–02aac` | 400-byte compared prefix of initialized file-ID record. Nine markers account for nine CRLF pairs in the supplied file. The check consumes 409 file bytes of 446. Bytes through `02ace` are nonzero beyond the compared prefix. Allocation extent and object declaring this record are open. |
| `0ac50–0ac56` | **PROVEN** six-byte saved IRQ0 pointer, followed by output-path storage at `0ac56`. Verified _BSS placement for the pointer. CRT clear begins at `0ac50` (operand at `1:62ff4`) and the file stores zeros to the end of the page. |
| `0ac60–0ac70`, `0ac70–0ae78`, `0ae78–0ee78` | Stage flags, volume filename table and GAMEDEF actor pointers. Their users include GAME. Ownership of the first cluster and historical segment contributions remain open. |
| `0f6c8–136c8`, `136c8–176c8`, `1d6d8–2d6d8`, `2d6d8–316d8`, `316d8–356d8` | Demo turn, forward, command-bit, tick, and strafe streams. `record` indexes the byte streams by frame and command stream at four bytes per frame. Both file paths transfer the same five arrays. All eight supplied demos satisfy the seven-byte header and eight-bytes-per-frame size rule. The 16,384-entry capacities fit the end anchors and frame cap. Shared declarations currently model them as externs. |
| `a5254`, `a52f8–a5304`, `a5390` | Recording count, three timer velocities and HUD sine phase. Independent references in demo I/O, keyboard/getinput, and several inlined shade calculations respectively. |

**NEGATIVE.** The reference clusters alone cannot establish that the bonus tables, score/FTA/maps and ID record are one contiguous ordinary `_DATA` contribution. The PLAYER group and SOUNDS initialized tables intervene. The BSS bucket rule in `docs/BSS_LAYOUT.md` also applies within one compiler contribution. The 16-KiB demo arrays are separated by GAMEDEF script storage. Reproducing those placements needs evidence of the original declaration/segment split. The verified six-byte case establishes one _BSS anchor.

## Candidate results

The candidate uses natural C, with the historical Watcom DOS intrinsics and register ABI. `outp` is declared as an ordinary library call to suppress the intrinsic in `conio.h` (**COMPILER_STEERED**). Lookup and I/O literals are provisional extern declarations for the linked addresses because the original surrounding pool is missing from the candidate. The source spells the FILE_ID filename directly and treats the original record as an external. The implemented routines are in linked order with gaps. The verifier's `_TEXT` placement conflict is expected for this excerpt and prevents a whole-object placement claim.

| Function | Oracle / candidate bytes | Result |
|---|---:|---|
| `inittimer` | 96 / 96 | STRICT |
| `uninittimer` | 80 / 80 | STRICT |
| `update_timer_input` | 448 / 448 | NEAR_MATCH, 287 byte differences, first `+4`. Early keyboard load changes the register allocation and subsequent fixup sites |
| `level_load_hook` | 16 / 16 | STRICT |
| `next_text_shade` | 48 / 48 | STRICT |
| `findlocator` | 80 / 80 | STRICT |
| `loadlookup` | 160 / 160 | STRICT |
| `draw_water_distortion` | 256 / 192 | NEAR_MATCH, 100 byte differences, first `+44`. Body diverges at an inner-loop branch displacement. The following displayrooms tables are absent |
| `record` | 112 / 112 | STRICT |
| `read_userdefs` | 176 / 176 | STRICT |
| `write_userdefs` | 96 / 96 | STRICT |
| `read_demo` | 272 / 272 | STRICT |
| `write_demo` | 272 / 272 | NEAR_MATCH, 125 byte differences, first `+13`. The decrement and filename selection change code ordering and reference sites |
| `check_file_id` | 128 / 128 | STRICT |
| `getscorename` | 45 / 45 | STRICT |

**NEGATIVE.** Watcom's `conio.h` under the game profile emits inline OUT instructions. The oracle calls the pinned CLIB3R `outp_` public. With the library declaration, `inittimer` matches. `uninittimer` compiled immediately before the empty level-load hook lost its RET to the shared tail, changing three bytes at its end. Inserting the real intervening input routine restores its standalone RET and its match.

**NEGATIVE.** Reading the recording count into a local and incrementing that local changed the initial allocation and moved the increment ahead of stores. Direct global subscripts with the original capped prefix increment reproduce all 112 bytes. Using narrower prototypes at the `strget` call generated extra extensions and changed the save/restore extent. Dword arguments at that call reproduce the observed ABI.

**NEGATIVE.** For `update_timer_input`, the six local initialization orders retained the first divergence at `+4`. Testing the OR of scan codes with `> 0` produced a 464-byte extent. Testing with `!= 0` gives the correct 448-byte extent. Four framebuffer address shapes and four shade expression orders left instruction or table differences in `draw_water_distortion`. The retained shape matches through `+43`.

**NEGATIVE.** `write_demo` with separate filename branches emits 288 bytes. One expression for the digit restores the expected extent. Prefix/postfix count updates gave the same result. A mode-based subtraction and a redundant mode check moved the first difference to `+3`.

**NEGATIVE.** A two-literal lookup excerpt starts its own CONST segment and cannot recover the banner's preceding padding residue. Compiling the lookup and file-ID literals together also concatenates two disjoint oracle pool slices. Keeping the lookup pair external restores its two bindings, and the final 12-byte FILE_ID literal segment verifies independently.

### Reproduce

From the repository root, direct compiler output and the JSON report to the worker directory. The pinned wrapper uses its canonical `build/cc` scratch. Compiling separately avoids `match.py`'s default temporary object under `build/match`.

```powershell
$w = 'build/workers/game2'
python -B "$w/survey.py" tails
python -B "$w/survey.py" tables
python -B "$w/survey.py" data 0 6ac
python -B "$w/survey.py" data 0ac50 0b000
python -B "$w/survey.py" assets
python -B tools/wcc.py --profile game "$w/src/game.c" "-fo=$w/game.obj" "-i=$w/src"
python -B tools/match.py --obj "$w/game.obj" --symbols "$w/overlay.json" --json "$w/results.json"
python -B tools/bindprop.py "$w/results.json"
```

The build is warning-free and the verifier exits 1 for the three NEAR results. The 12 STRICT functions cover 1,309 bytes. The three NEAR candidates compile to 912 bytes against 976 bytes of oracle extents. Bindprop reports six function and twelve data proposals with zero conflicts. The remaining function bodies, historical _DATA placement, and an original-order linked layout remain open.
