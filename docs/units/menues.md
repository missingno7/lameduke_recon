# LameDuke MENUES reconstruction

The candidate covers eleven recovered functions in original order. Watcom 9.5b with the pinned
game profile reports **6 STRICT functions and 1,146 bytes**, **0 CODE_BYTES_MATCH** and
**5 NEAR_MATCH functions covering 8,384 oracle bytes**. The five candidate extents total
8,320 bytes. Runtime behavior remains untested.

## Boundaries

**PROVEN.** SOUNDS returns at `1:2c1ba`. Five zero bytes and zero relocations precede
the initializer at `1:2c1c0`, independently called from main at `1:5bdd`. The parallax
helper returns at `1:2e6f9`. Six zero bytes and zero relocations separate `1:2e6fa` from
the verified GAMEDEF entry at `1:2e700`. The six bytes lie outside the function extent.

**PROVEN.** The census merged two uncalled routines with preceding functions. The return at
`1:2c424` is followed by compiler alignment and a standalone slider at `1:2c430`. The second
return in `probe` ends at `1:2c876`. Alignment precedes the combined title, text and input
routine at `1:2c880`. Both entries have complete prologues and returns. The binary also
contains the slider inlined three times and the title/input pattern specialized on each
menu page.

**PROVEN.** Six switch tables for `menus` occupy `1:2cc00–2ccf0`, with 11, 8, 8, 12, 12 and 9
LE dword relocations. Their starts are `2cc00`, `2cc2c`, `2cc4c`, `2cc6c`, `2cc9c` and
`2cccc`. The verifier includes those 240 bytes in the extent of `draw_and_probe`. The
last instruction of that function ends at `1:2cbff`, followed by an alignment NOP.

**STRONG.** The continuous Watcom alignment, internal call graph, exclusive MENUES literal pool
and code tables support one contribution `1:2c1c0–2e6fa`. The ripple functions and parallax helper
have callers in GAME/SECTOR. Their association with MENUES comes from placement through normal
alignment. The historical object filename and linker map remain open.

## Function results

Byte counts are oracle then candidate, including compiler padding.

| Function | Obj1 extent | Bytes | Status and first difference |
| --- | --- | ---: | --- |
| initmenus | 2c1c0–2c430 | 624 / 624 | NEAR_MATCH. 253 differences. At +73, the immediate feeding the second scheduling group differs. At +6d, the candidate also loads a literal where the oracle loads an integer. |
| draw_menu_slider | 2c430–2c590 | 352 / 352 | STRICT |
| drawmenu | 2c590–2c6f0 | 352 / 352 | STRICT |
| probe | 2c6f0–2c880 | 400 / 400 | NEAR_MATCH. 54 differences. At +4c, Watcom loads the −2 return before clearing Escape. Down and wraparound blocks also differ. |
| draw_and_probe | 2c880–2ccf0 | 1,136 / 1,136 | NEAR_MATCH. 636 differences. At +4f, the title call prepares x sooner. The inlined regions and following switch targets differ. |
| menus | 2ccf0–2e060 | 4,976 / 4,912 | NEAR_MATCH. 2,989 differences. At +98, the jump to Instructions has a different displacement. Entry initialization and the 252-byte frame match. |
| draw_fade_transition | 2e060–2e0d0 | 112 / 112 | STRICT |
| vscrn | 2e0d0–2e120 | 80 / 80 | STRICT |
| init_water_ripples | 2e120–2e200 | 224 / 224 | STRICT |
| animate_water_ripples | 2e200–2e6e0 | 1,248 / 1,248 | NEAR_MATCH. 625 differences. At +7d, the outer index spill uses esp+18 in the candidate and esp+14 in the oracle. Ring traversal, workspace setup and filtering schedules differ. |
| resetparallax | 2e6e0–2e6fa | 26 / 26 | STRICT |

The object has 9,466 bytes of `_TEXT`. Publics through `menus` imply code base `1:2c1c0`.
The dispatcher deficit makes the five following publics imply `1:2c200`. Contiguous
compiled placement remains open.

## Data and bindings

- **PROVEN.** `CONST` verifies at `2:00958–00b44`, all 492 bytes with complete
  emitted coverage. Watcom emits different stale padding after the two identical slot-list
  literals, preserving both pointers at `2:00a6c` and `2:00aa4`. Plain `""` for the
  score descriptor and `"Open"` for the final label gave a 489-byte contribution.
  `"\0"` and `"Open\0\0\0"` supply the required zero coverage. The compiler emitted
  every nonzero padding byte.
- **PROVEN.** The candidate emits zero `_DATA` bytes. No initialized relocations in
  the image point into the MENUES literal pool. `initmenus` writes the literal pointers
  at runtime. Ownership of the file-backed zero interval `2:02aac–02b10` remains open.
- **PROVEN.** The descriptor ABI is 15 bytes. By-value copies use three dwords, a word
  and a byte. Fields used by the routines are x, y and line count at offsets 0, 2 and 4,
  title tile at 8, text pointer at 10 and flags at 14. The field at 6 has no identified use.
  Nine descriptors occupy 16-byte slots from `2:a5708` to `2:a57a8`, with an unassigned
  slot at `2:a5778`. A tenth descriptor at `2:a5410` supplies the High Scores title.
  Packed player and sector records retain independently established 176- and 37-byte strides.
- **PROVEN.** The ripple initializer and update share `2:a59e0–b1dec`. The cluster
  contains 16 ages, a 128-byte ramp, 16 dword strengths, two 32-by-128 dword ring
  tables, 32 ring counts, two 16-dword center arrays, a 130-by-130 byte workspace
  and two dword counters. The caller at `1:15587` passes `waloff[49]` under `gotpic` bit 49.
  The renderer wraps both axes of the 128-by-128 source into the border and sums seven
  sampled neighbors plus one ramp value for each output pixel.
- **STRONG.** The zoom at `2:a5392` is local to menu code. `2:a5394`, `2:a5398`,
  `2:a539a` and `2:a539c` are shared mouse action, drag and cursor fields used by GAME.
  `2:a5396` has GAME cursor-updater references. Main and gameplay use the separate
  transition zoom at `2:a57b3`. The BSS ownership of shared fields remains open.
- **PROVEN.** The sliders adjust `2:a57af`, `2:a57b1` and `2:a57b7` within the
  21-byte settings block of `USERDEFS.TMP`. The first two words go to FX and MIDI
  volume calls. The third scales mouse input. The 23-byte `SETUP.DAT` contains
  the gameplay scan codes, including `c8`, `d0`, `cb` and `cd`. The four MENUES
  literals `Jump`, `Duck`, `Shoot` and `Open` label live indicators on the
  Instructions page. The page tests the low four input bits. Page 40 is Ordering Info,
  which accepts Back.
- **OPEN.** The candidate defines the observed descriptors, menu zoom and ripple state.
  Watcom emits one 50,351-byte `_BSS` contribution whose public order implies
  incompatible bases. The original compiler and linker allocation order remains open,
  including the four zero-filled bytes at `2:b1dec–b1df0`.

The overlay gives independent call-site, data-use and extent evidence. The final
`bindprop.py --min-funcs 2` run yields one function and three data proposals with zero
conflicts. `menu_drag` has two agreeing STRICT relocation votes and separate GAME cursor
update evidence. `overwritesprite`, `ps` and `screenpeek` agree with the canonical registry.

## Source provenance and compiler findings

The routines are **BINARY_DERIVED**. Duke 1.5 `MENUES.C` supplies the probe and menu
concepts, `GAME.C` supplies glyph drawing, and `PREMAP.C` supplies viewport and stage-transition
context. Ken's `buildold_12_30_94/GAME.C` supplies earlier map-loading and viewport idioms.
The prototype's page numbering, fixed margins, glyph IDs and settings layout follow
the executable. The header includes Build's `klabs` and `copybuf` from `BUIL0615/PRAGMAS.H`,
both **ORIGINAL_ASM** intrinsics.

`initmenus`, `probe`, `draw_fade_transition`, `vscrn` and `resetparallax` use **NATURAL**
body shapes. The two extra trailing NUL expressions are **COMPILER_STEERED**.
`draw_menu_slider` and `drawmenu` retain the promoted blitter arguments, literal-left
addition and explicit shade widening required by the matching ABI. `init_water_ripples`
writes both coordinates before incrementing the count. These choices, the by-value
copies in `draw_and_probe` and `menus`, and the two-row copy and four-byte intensity
group in `animate_water_ripples` are **COMPILER_STEERED**.

The historical spellings of `draw_menu_slider`, `draw_and_probe`,
`draw_fade_transition`, `init_water_ripples`, `animate_water_ripples` and `resetparallax`
are unproven. The `menu_*`, `ripple_*` and `ring_*` storage names, helper macros,
partial record tags and unknown fields are descriptive. `next_text_shade`,
`update_menu_cursor` and `displayhighscores` are provisional GAME names.

### Negative results with wc95b -4r -s -otexan

- A byte glyph-shade formal with `base_shade = shade0` produced a 352-byte extent
  with correct relocations and six displacement differences. A short formal explicitly
  truncated to unsigned byte before widening reproduces `drawmenu`.
- A signed-byte blitter shade prototype emits a byte reload in `draw_menu_slider` where
  the oracle reloads a signed word. Reordering or narrowing the local shade changes
  the 12-byte frame's spill layout.
- Postincrementing a local ring slot before writing the coordinates added four frame
  bytes and changed index addressing. Two indexed stores followed by increment match.
- In `animate_water_ripples`, `memcpy` emitted generic byte-count copy code in a
  1,008-byte candidate. `copybuf` emits the dword copy. A single-byte mask loop
  unrolled ten times. Pairing row copies and grouping four intensity bytes produces
  the 1,248-byte extent. The first instruction difference is at +7d.
- A local `probe` cursor copy used a different save register and omitted the oracle's
  cursor store at exits. Direct use of the shared cursor reduced the differences to
  54 bytes. Alternate key-clear, return, and cursor-alias forms failed to improve it.
- Tests of `initmenus` assignment order and hoisted literal pointers left code and
  relocation differences. The grouped source preserves the extent and CONST. Staging
  menu arguments and short slider parameters brought `menus` from 4,624 to 4,912
  bytes and reproduced the frame. The dispatcher remains 64 bytes short.

## Reproduce

From the repository root in PowerShell

```powershell
$w = 'build/workers/menues'
python -B "$w/run.py" match --profile game --symbols "$w/overlay.json" "-i=$w/src" "$w/src/menues.c" --json "$w/results.json"
python -B tools/bindprop.py "$w/results.json" --min-funcs 2
```

The wrapper invokes the unchanged verifier and compiler wrapper with worker-local
object and temporary output. The verifier exits with status 1 for this partial
reconstruction. `verify.txt` includes the data verdicts and first differences.
The oracle is `assets/D3D.EXE`, SHA-256
`698979d97f0f2a16ffdea20ff0059fe49ef9720a7c8933a61601356ca0462701`.
