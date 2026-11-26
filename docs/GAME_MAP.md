# LameDuke game code survey

Oracle `assets/D3D.EXE`, SHA-256 `698979d97f0f2a16ffdea20ff0059fe49ef9720a7c8933a61601356ca0462701`. All address ranges are hexadecimal and half-open in the named LE object. `names.json` records candidate identities with evidence. Names such as `level_load_hook` and `bootleg_cache_alloc` describe observed roles when the original spelling is unknown. `overlay.json` contains bindings used by the probes.

The Duke 1.5 game sources are under `dukenukem3d/source/`. The sibling `dukenukem3d/SRC/` directory holds Build source. Comparisons use the released game sources and Ken's September 1994 `buildold_12_30_94/GAME.C`. The latter supplies useful antecedents for the parallel demo streams, tick loop, map loading and early input code.

## Translation units

The game contribution is `1:00010–33e5c`. The proposed layout has seven code-bearing objects. Six internal boundaries recur after a terminal `ret` followed by zero fill. Within the game objects, ordinary function starts commonly align to 16 bytes with `lea`/`mov` no-ops, which `wc95b -4r -otexan` reproduces. The fills, ownership of private code tables, and call and data references inform the boundaries below. No original game link map survives in the supplied files.

| Obj1 contribution | Likely counterpart | Obj2 anchors | Evidence |
|---|---|---|---|
| `00010–14fb0` | **GAME**, including later **PREMAP** and some save/menu and actor code | `00004–006a8` literals, `02304`, `023dc–02838`, `0291c` | Main at `05b60` is called by `__CMain` at `6477d`. Timer ISR, graphics, startup, simulation, demos, saves and stage setup group here. `prelevel` and `enterlevel` are at `12210` and `149b0`. Private prefix tables `00010` and `00013` are referenced from `0ba30`. Three zeros follow the last `ret`. |
| `14fb0–1f280` | **SECTOR** | `0eec8`, `a1430–a2830`, `a4c30`, `a5250` | Begins with `checkcursectnums` and `findplayer`. Animation helpers at `150a0`, `15340` and `15380` share a private pointer/goal/velocity cluster. Wall and sector operators dominate the remainder. Two zeros follow the last `ret`. |
| `1f280–25160` | **PLAYER** | `02364–023d2` frame selectors and shared `ps`/input | Five private tables at `1f280`, `1f296`, `1f2aa`, `1f2b4` and `1f2c9` are referenced only by `1fd20` and `1fe40`. First code `1f2e0`, followed by `shoot`, weapon display, `getinput` and `processinput`. Four zeros follow the last `ret`. |
| `25160–2ba30` | **ACTORS** | Shared `hittype`, `sprite` and `headspritestat` | Three private constant blocks at `25160`, `25163` and `25168` are referenced solely by `25480` and `25f10`. First code `25180`. Stage update functions walk status lists and call `execute` at `32260`. Nine zeros follow the last `ret`. |
| `2ba30–2c1c0` | **SOUNDS**, including BootLeg CacheMaster | `006a8–00958`, `02838–0291c`, `356d8–88830`, `a5420`, `a57d0–a59dc` | Contiguous 16-byte compiler fill between cache routines, ASS hooks and sound helpers. Main reaches allocator init and VOC loading from explicit startup diagnostics. ASS calls into both hooks. Ends with `TestCallBack` and five zero bytes after its `ret`. |
| `2c1c0–2e700` | **MENUES**, with additional display/transition helpers | `00958–00b44`, `a5392–a539e`, `a5708–a57a8`, `a59e0–b1df0` | Menu options, keyboard labels, `newgame`/`enterlevel`, saves, input configuration and display helpers. Ripple helpers at `2e120`/`2e200` have a separate data cluster and gameplay callers. Their affiliation with this object is inferred from the same fill through `2e6e0`. Six zeros follow the last `ret`. |
| `2e700–33e5c` | **GAMEDEF** | `00b44–01184`, `02b10–02c00`, `0ae78`, `176cc`, `a5230–a5250`, `b1df0–b1e00` | Lexer, parse diagnostics and recursive includes, compile entry and parallel bytecode dispatchers. `loadefs` at `301e0`, `parse` at `308b0` and `execute` at `32260`. Ends at a `ret` immediately before the engine ISR. |

The engine starts at `1:33e5c`. Engine initialization at `1:3a263` takes the interrupt-handler address there, and `1:33e5c–33f18` implements IRQ1 for the engine's shared `keystatus`. Ken's March 1995 `ENGINE.C` likewise begins with `keyhandler` and `drawrooms`. The latter starts at `1:33f18` here and is called repeatedly by `displayrooms`. The game IRQ0 handler is instead at `1:002a0`. Engine loadboard at `1:39127` and saveboard at `1:39337` take the game map paths and position/sector pointers. These anchors complement the independently scanned `BUIL0801/ENGINE.OBJ` publics such as `ksqrt`, `krand` and `getmousevalues`.

The function census merges uncalled and inlined routines with adjacent entries. `1:00020`, `00080`, `2bae0`, `2e7b0` and `2e850` have separately recoverable starts. `1:00010`, `00013`, `1f280–1f2d4` and `25160–25177` have data-reference edges and contain private initializers. Jump-table labels and these tables are excluded from the function names. Physical boundaries are better supported than the historical filenames. A globals-only object like later `GLOBAL.C` could contribute no `_TEXT`. There is no independent object-level boundary for later `PREMAP`, `CONFIG` or `RTS` in the survey.

## File-backed data ownership

References can establish use and infer segment ownership. For mixed or weakly referenced ranges, the table reports a cluster and leaves precise contribution cuts open. `0xb000` is the loader's file-backed boundary. It includes zero-filled game storage.

| Obj2 range | Owner or use | Evidence |
|---|---|---|
| `00000–006a8` | GAME/PREMAP literal pool | Beta banners at `00004` and `0032c`, LOOKUP and help, warp and startup messages, bonus/score UI, save/demo paths and level-entry text. Final explicit string at `0069c` is FILE_ID.DIZ. Initial marker bytes at `00000–00004` are unassigned. |
| `006a8–00958` | SOUNDS and allocator literals | 39 VOC names at `006a8–008a4`, nine device names at `008a4–00928`, allocator/hook diagnostics and `playmusic` file mode at `00928–00958`. The filenames and device names are pointed to from initialized tables. |
| `00958–00b44` | MENUES literals | `initmenus` references the menu text lists and `menus` references the binding labels. |
| `00b44–01184` | GAMEDEF literals | `00b44–00d8c` contains 60 keyword strings, starting with `end`. `00d8c–01184` contains compilation diagnostics, game.con path and result messages. |
| `01184–01248` | ENGINE literals | TABLES.DAT, SETUP.DAT and palette, screenshot, timing and network diagnostics. |
| `01248–01ef0` | ASS literals | Device-specific error/driver strings, including final AdLib FX messages at `01e58–01eea`. Padding or unused bytes at the edges are uncertain. |
| `01ef0–02304` | Watcom C/math constants | Error strings, FP functions' names and constants, and `__IsTable` at `02164–02265`. The latter uniquely matches all 257 bytes of pinned 9.5b CLIB3R `istable:CONST2` and is referenced by game tokenizers. |
| `02304–02364` | GAME bonus tables | Uses by `dobonus` at `09650`, including tile/short data at `02304` and `02306`. Unreferenced gaps remain open. |
| `02364–023dc` | PLAYER frame data | Indexed by `displayweapon` and `processinput`. The last observed nonzero frame data ends at `023d2`. |
| `023dc–02838` | Shared GAME data | Score entries `023dc–02568` and values `02568–02598` exactly equal USERDEFS.TMP bytes 21–416 and 417–464. FTA has 32-byte strings at `02598–02738`. Two 128-byte scancode maps at `02738–02838`. |
| `02838–0291c` | SOUNDS/shared startup data | 39 VOC pointers `02838–028d4`, nine device name pointers `028d4–028f8`, nine ASS device-ID dwords `028f8–0291c`. |
| `0291c–02b10` | GAME integrity text and nearby zero storage | `check_file_id` compares a 400-byte record starting at `0291c`. Extent and owner of intervening zero storage are open. |
| `02b10–02c00` | GAMEDEF keyword pointers | 60 consecutive off32 relocations into `00b44–00d8c`. `keyword` traverses 0xf0 bytes. Opcode ordering and vocabulary differ from Duke 1.5's 112-keyword table. |
| `02c00–02e80` | ENGINE initialized and shared data | Renderer defaults and setup tables, keyboard defaults at `02cda–02ceb`, a network ISR template near `02d10`, connection data at `02e78–02e7c`. Game callers read `myconnectindex` at `02e7a`. |
| `02e80–04734` | Audio device/music cluster, provisional ASS attribution | Code from `49c8c` through `4def4` references the region densely. ASS routines at `623dc` and `62468` also use its banks and tables. Exact module cuts are open. |
| `04734–08e06` | Audio device/bank data | Early sound initialization references `04734–049c8`. ASS sound-bank loader `623dc` and its peers reference large subranges through `08de4`. |
| `08e06–09d54` | ASS driver and mixer data | Heavy driver/mixer references, `095cc` contains ULTRAMID.INI, and BLASTER data begins near `0964c`. Boundaries within this cluster are provisional. |
| `09d54–0a6fc` | ASS bank/synthesis data | Bank selection and conversion routines use low references and large indexed spans. Exact cuts are open. |
| `0a6fc–0a71c` | ASS tail / AdLib FX | References come from `62778–62cb8`, including the AdLib FX error routine. |
| `0a71c–0ac50` | Watcom runtime data and linker lists | Dense CRT references and code/data fixups. Contains the stack overflow diagnostic and startup records. |
| `0ac50–0b000` | File-backed zero storage used by GAME/GAMEDEF | Old timer far pointer `0ac50`, output directory `0ac56`, spawn-related flags near `0ac60–0ac70`, and first `0x188` bytes of the actor-script pointer table starting at `0ae78`. Entire range reads as zero in the oracle. |

## Large zero-filled arrays and shared state

| Obj2 range | Identity / owner | Evidence |
|---|---|---|
| `0ae78–0ee78` | GAMEDEF `actorscrptr[4096]` | `loadefs` clears 4096 dwords. Parser assigns entries by tile ID, and `spawn`/`execute` read by tile ID. |
| `0eec8–0f6c8` | SECTOR `animatevel[512]` | Four-byte entries paired with `animatesect[512]` at `a1430`, `animateptr[512]` at `a1830` and `animategoal[512]` at `a2030`. `setanimation` refuses a new slot when count is 511. |
| `0f6c8–176c8` | GAME demo motion arrays | Two 16 KiB byte streams starting at `0f6c8` and `136c8`. `record` also writes motion stream `316d8–356d8`, timing stream `2d6d8–316d8` and command dwords `1d6d8–2d6d8`. `reccnt` at `a5254` is capped at 16383. All eight demos satisfy `length = 7 + 8 × frame_count`, with three mode bytes and a little-endian dword count. Individual motion names are provisional. |
| `176c8–1d6d8` | GAMEDEF compiled script and preceding end pointer | Start at `176cc`, end pointer stored at `176c8`. Compiler reports their difference. `1d6d8` is the next independently identified base and gives an upper limit for the script allocation. The actual allocated size is open. |
| `356d8–35810` | SOUNDS file table | 39 eight-byte records. First dword supplied to the audio player. |
| `35810–3f450` | SOUNDS MIDI buffer interval | `playmusic` reads the file into `35810`. The next bound object is the cache arena. Exact MIDI allocation size is open. |
| `3f450–88830` | BootLeg CacheMaster arena | Init sets base and 300000-byte capacity. SECTOR/GAMEDEF/SOUNDS can use cache-backed storage. `loadefs` reserves file length, 32768 label-name bytes and 2048 label-value bytes, then pops all three. The persistent arena accounting sits at `a57d0–a59dc`. |
| `88830–893e0` | Shared `ps[17]` | 176-byte stride. Seen in position, current sector, sprite index, quote and inventory access. 17 slots fit the exact interval to `hittype`. |
| `893e0–9d3e0` | Shared `hittype[4096]` | 20-byte stride. `spawn`, sector movement, sprite updates and script execution use this metadata. Fits to next bound address. |
| `a4830–a4c30` | Shared scratch interval | Used by command line, file-ID check and all CON lexical routines. The read of 450 bytes sets a proven minimum. Exact allocation size is open. |
| `a4c30–a5230` | Shared animated-wall interval | Six-byte stride. `prelevel` populates entries and `animatewalls` consumes them. Interval fits 256 records pending confirmation of the source declaration. |
| `a5420–a5705` | SOUNDS voice records | 39 × 19 bytes. Position/handle/count writes and packed callback probe support fields at +0, +4, +12, +14, +18. |
| `a5708–a57a8` | MENUES descriptors | Initialized and indexed in 16-byte steps by menu code. Additional descriptor near `a5410`. |
| `a59e0–b1df0` | Ripple renderer state | Random ripple centers and ages, paired coordinate-ring tables around `a5ae0`/`a9ae0`, palette ramp at `a5a20`. Exact array limits within this cluster are open. |
| `b1df0–b1e00` | GAMEDEF local compile/interpreter state | Last source pointer, line and total-line counters, parser flags. |
| `11eb90–122b90` | ENGINE `waloff[4096]` | Four-byte tile pointers used in cache and display code. |
| `12ed90–130d90` | ENGINE `nextspritestat[4096]` | Two-byte links used by each actor-list traversal and initialized by the engine. Companion `headspritestat` at `1bd63c`. |
| `134d90–134f90`, `13ba10–13cb10` | ENGINE shared bitmap, sine and key arrays | `gotpic` 512 bytes, `sintable` 2048 shorts at `13ba10`, `keystatus` 256 bytes at `13ca10`. |
| `13cf10–146f10`, `146f10–16cf10`, `16cf10–176310`, `176310–1bc310` | ENGINE `extra`, `sprite`, `sector`, `wall` | Consecutive arrays of 4096 × 10, 4096 × 38, 1024 × 37 and 8192 × 35 bytes. Direct v5 loadboard reads at `39303`, `392ad`, `3922a` and `39271` independently establish the record sizes. |

Twenty-nine supplied MAPs have version 5 and parse to their exact file size using a 20-byte header and counted sector, wall, sprite and extra arrays of 37, 35, 38 and 10 bytes respectively. `L9.MAP` and `M8.MAP` have version 4. This corrects the all-maps claim in `docs/FINDINGS.md`. The v5 sizes establish that target structs differ from later Build definitions. The `8192` wall count agrees with the December 23 engine change in the local `BUILD.TXT`.

## Compiler findings

**PROVEN reproducing profiles** `wc95b -4r -s -otexan`, `wc95b -4r -s -otexa`, `wc95b -4r -otexan`, and `wc95b -4r -otexa` pass the nine full-extent probes. These controls leave the historical use of the `n` suboption and explicit `-s` switch open. The observed image has no direct game-region calls to the stack check entry at `1:646ef` or its nearby variants `646ff` and `64702`. Default `wc95b -4r` emits a leading `push` and call to `__CHK` in the lexer probe.

**STRONG historical hypothesis** Watcom 9.5-family, register-call 486 code generation and speed-oriented optimization with the alias assumption enabled. `USRHOOKS_GetMem` distinguishes the tested 9.5b build from all four 10.x installs under the same flags. The matching 9.5-family CRT provides independent historical evidence. Most individual probes are too small to distinguish some releases and switches. The original compiler's exact patch level and full command line remain open.

| Probe | Target extent | Status with `wc95b -4r -s -otexan` |
|---|---|---|
| `level_load_hook` | `00290–002a0`, 16 B | STRICT |
| `getanimationgoal` | `15340–15380`, 64 B | STRICT |
| `init_bootleg_cache` | `2bb30–2bb60`, 48 B | STRICT |
| `USRHOOKS_GetMem` | `2bb60–2bb90`, 48 B | STRICT. Both rel32 calls bound, 14-byte CONST segment verified at `2:00944`. |
| `USRHOOKS_FreeMem` | `2bb90–2bbd0`, 64 B | STRICT |
| `TestCallBack` | `2c190–2c1bb`, 43 B | STRICT. Five zeros after the `ret` are between TUs. |
| `ispecial` | `2e740–2e770`, 48 B | STRICT |
| `isaltok` | `2e770–2e7b0`, 64 B | STRICT |
| `getlabel` | `2e7b0–2e850`, 160 B | STRICT |
| `getincangle_shift` | `2e700–2e740`, 64 B | NEAR_MATCH. Candidate 96 B, 88 differing bytes, first at `+1`. Its save set adds `esi` and its extent overruns into the next function's two fixups. |

Totals for the primary profile are **9 STRICT / 555 bytes**, **0 CODE_BYTES_MATCH** and **1 NEAR_MATCH** among the named probes. The C probes are independent excerpts. Their synthetic following publics serve to make the compiler emit its own alignment. In the cache and lexer excerpts, their bound consecutive real publics also agree on placement. Complete original C objects and a natural link remain open.

### Negative and discriminating results

- `wc95b -4r`, `-4r -s` and `-4r -s -ot` fail `ispecial` at `+0`. The CPU/register model alone does not reproduce the game. `-4r -s -ox` can match shortened code prefixes. With a real following routine it gives 40 bytes at `ispecial` instead of 48, and `getanimationgoal` first differs at `+13` with a 53-byte candidate instead of 64.
- `wc95b -3r -s -otexan` and `-5r -s -otexan` produced 28- and 40-byte `ispecial` extents in the two-routine sweep. `-5r` also first differs in `init_bootleg_cache` at `+26`, in the padding, with a 40-byte extent. Neither reproduces the observed 16-byte layout across probes.
- `wc100`, `wc100a`, `wc105` and `wc106` under `-4r -s -otexan` emit a 32-byte `USRHOOKS_GetMem` and first differ at `+1`. The candidate moves the output pointer to EBX where the oracle pushes ECX, changing the rel32 and literal sites. The versions still match `getanimationgoal`, `init_bootleg_cache` and `USRHOOKS_FreeMem`. All four also differ on `isaltok` and `getlabel` when compiled with their pinned headers. `wc100a` differs at `+1` on the header-independent `ispecial`.
- `wc95b -4r -s -otx` matches the 48-byte allocator hook and cache initializer, but `USRHOOKS_FreeMem` first differs at `+5`, with an 80-byte candidate and different cache-size reads. `-otexn` produces the same free-hook mismatch and changes `getlabel` at `+4`. `-otax` retains the free hook but changes `getlabel` at `+13`.
- `TestCallBack` matches with the compiler default and `-zp1`. The `-zp2` and `-zp4` variants produce 20- and 24-byte record strides and first differ at `+13` and `+a`, respectively. This establishes one-byte effective packing for this declaration. A source pragma and an explicit compiler packing flag can both produce it.
- An early verifier run reported `STRICT` for a 39-byte `ispecial` because its candidate public extent ended before the final oracle `ret`. The final suite checks each reported size independently against the next known start or TU return. No short candidate is counted. Outer verification uses unchanged `tools/match.py`.

## Reproduction

Run from the repository root. The harness calls the pinned compiler wrapper with scratch rooted inside the worker directory and invokes `match.py --obj` on each result. It runs one compile at a time and checks independently measured extents.

```powershell
python -B build/workers/game/calibrate.py --verify
python -B build/workers/game/calibrate.py --verify -- -4r -s -otexa
python -B build/workers/game/calibrate.py --verify -- -4r -otexan
python -B build/workers/game/calibrate.py --source cache_probe.c --install wc100 --functions USRHOOKS_GetMem -- -4r -s -otexan
python -B build/workers/game/calibrate.py --source sound_probe.c --functions TestCallBack -- -4r -s -otexan -zp2
python -B build/workers/game/calibrate.py --source angle_probe.c --functions getincangle_shift -- -4r -s -otexan
python -B build/workers/game/survey.py dis 2e740-2e850 33e5c
python -B build/workers/game/survey.py strings 0 1248
```

The next integration candidate is the contiguous `ispecial`/`isaltok`/`getlabel` block. It has stable boundaries, parser data references with independent identities, and a self-reference inside the lexer probe. Promote only after rerunning the verifier against any new TU declarations and bindings.
