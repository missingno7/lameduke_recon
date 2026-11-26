# LameDuke cross-source donor scan

Watcom 9.5 GA (wc95b) compiled **160/188** source files and produced **266** successful host/profile builds. The scan evaluated **7,464** named function variants across **4,473** source-file/function pairs.

**165 targets** include 143 game functions and 22 engine NEAR functions. **23** have a strong donor in the top three (9 are NEAR). The strong donor beats the current score by at least 0.08 for **0** targets.

## Most promising NEAR counterparts

Ranked by clear gains, corroboration, positive gain, and donor score among the 86 NEAR targets.

| Target | Donor | Profile | Score | Current | Gain | Evidence | Shared calls |
|---|---|---|---:|---:|---:|---|---|
| 1:47490 `setvesamode` | buildsrc-20190520/19950920/POLYTEX5.C `setvesamode` | engine | 1.000 | 1.000 | +0.000 | Strong | int386x ×2, memset ×2 |
| 1:47699 `setverts` | buildsrc-20190520/19950920/POLYTEX5.C `setverts` | engine | 1.000 | 1.000 | +0.000 | Strong | inp ×5, outp ×20 |
| 1:2eae0 `transnum` | Duke 1.3D tree/gamedef.c `transnum` | game_oe | 0.862 | 1.000 | -0.138 | Strong | atol, strcmp, printf |
| 1:345ac `wallfront` | kenbuild-20241213/BUIL0422/ENGINE.C `wallfront` | engine | 0.825 | 0.834 | -0.009 | Strong | -- |
| 1:3b390 `screencapture` | kenbuild-20260331/build_1995_03_28_partial/ENGINE.C `screencapture` | engine | 0.806 | 0.991 | -0.185 | Strong | getpixel16 ×3, write ×7, close ×2, open |
| 1:4393d `drawline16` | kenbuild-20241213/BUIL0422/BUILD.C `drawline16` | engine | 0.787 | 0.911 | -0.124 | Strong | -- |
| 1:3ca84 `drawsprite` | kenbuild-20260331/build_1995_03_28_partial/ENGINE.C `drawsprite` | engine | 0.755 | 0.938 | -0.183 | Strong | ceilspritescan, spritewallfront, transmaskwallscan, maskwallscan, wallmost ×2 (+6) |
| 1:3ffee `grouvline` | kenbuild-20260331/build_1995_03_28_partial/ENGINE.C `grouvline` | engine | 0.743 | 0.912 | -0.169 | Strong | loadtile ×2 |
| 1:3c322 `drawmaskwall` | kenbuild-20260331/build_1995_03_28_partial/ENGINE.C `drawmaskwall` | engine | 0.721 | 0.975 | -0.254 | Strong | transmaskwallscan, maskwallscan, wallfront, wallmost ×2, animateoffs |
| 1:4445e `draw2dscreen` | kenbuild-20241213/BUIL0422/BUILD.C `drawoverhead` | engine | 0.706 | 0.882 | -0.176 | Weak | drawline16 ×9 |

## Scoring and evidence

Baseline and oracle roster captured at 2026-11-26T20:06:02+00:00. The freshly compiled objects were checked against that verification snapshot before ranking.
- The score blends mnemonic bigram/trigram Dice similarity (40%), operand-shape bigram/trigram Dice similarity (27%), named-call overlap (13%), nonrelocated immediate overlap (13%), and reachable-byte size ratio (7%). Call and constant counts use inverse donor frequency. A component absent on both sides is omitted and its weight redistributed proportionally. Names only break score ties and corroborate hits.
- Reachability follows branches and relocation-backed indexed jump tables. Calls and jump displacements and relocated address values are excluded from immediate evidence. Known direct calls and external tail jumps are scored by name. Unknown call targets and unresolved indirect jumps are counted in the JSON.
- Strong requires 0.72 and either the same function name, two shared call names, a shared call with three shared immediates of magnitude at least eight, or identical normalized instruction shapes over at least 16 instructions. Renamed counterparts need at least 12 instructions on both sides. Incomplete jump-table coverage can suppress the strong label. Clearly closer also requires an improvement of at least 0.08.
- The size columns show full extents. Known verified oracle extents are retained. For NEAR targets, the next bound function, trusted entry or TU boundary supplies the end. Bare code-reference starts can be switch data and are excluded. Regular and local OMF code symbols supply donor boundaries. The scan measures similarity, and the repository verifier determines STRICT status.
- Current-source scores come from fresh DOS-hosted compiles using unit flags. The historical donor matrix uses the NT host with `-bt=dos` for the broad search. DOS-hosted donor controls take precedence for the same source/profile combination. Each donor row records its host and build ID.
- For repeated flag variants of a source-file/function pair the ranking retains its highest score. Historical copies in distinct archives can appear as separate donors. The JSON contains full paths, per-component scores, complete shared features and build logs. Baseline verifier fields record the first non-fixup byte difference and the first reported problem where available.

## Target rankings

### engine

| Oracle function | Oracle B | Current | Rank | Donor | Score | Donor B | Shared calls | Shared immediates |
|---|---:|---:|---:|---|---:|---:|---|---|
| 1:342b8 `scansector` (NEAR_MATCH) | 756 | 1.000 | 1 | kenbuild-20260331/build_1995_03_28_partial/ENGINE.C `scansector` (engine/dos) | 0.669 | 676 | addboxlinum | 48, 1024 ×2, 6 ×2, 7 ×2, 3 ×2 (+3) |
|  |  |  | 2 | buildsrc-20190520/SRC_9505/ENGINE.C `scansector` (engine/dos) | 0.649 | 726 | addboxlinum | 48, 1024 ×2, 6 ×2, 7 ×2, 3 ×3 (+3) |
|  |  |  | 3 | kenbuild-20241213/BUIL0422/ENGINE.C `scansector` (engine/dos) | 0.618 | 422 | addboxlinum | -1 ×3, 0, 4, 1 ×3 |
| 1:345ac `wallfront` (NEAR_MATCH) | 554 | 0.834 | 1 | kenbuild-20241213/BUIL0422/ENGINE.C `wallfront` (engine/dos) | 0.825 **strong** | 529 | -- | -2, 36, 28, 255 ×6, -1 |
|  |  |  | 2 | kenbuild-20241213/buil010294_BUILDOLD/BUILD.C `front` (engine/nt) | 0.813 | 538 | -- | -2, 36, 28, 255 ×6, -1 (+1) |
|  |  |  | 3 | kenbuild-20241213/builsect/BUILD.C `front` (engine/nt) | 0.813 | 538 | -- | -2, 36, 28, 255 ×6, -1 (+1) |
| 1:3493d `calcwalls` (NEAR_MATCH) | 5068 | 1.000 | 1 | kenbuild-20260331/build_1995_03_28_partial/ENGINE.C `drawalls` (engine/dos) | 0.621 | 7994 | grouvline, ceilscan, florscan, wallscan ×8, wallmost ×2 (+1) | 134217728 ×2, -4096 ×2, 2560 ×2, 19 ×2, 16384 ×2 (+20) |
|  |  |  | 2 | buildsrc-20190520/SRC_9505/ENGINE.C `drawalls` (engine/dos) | 0.617 | 7985 | grouvline, ceilscan, florscan, wallscan ×8, wallmost ×2 (+1) | 134217728 ×2, -4096 ×2, 2560 ×2, 19 ×2, 16384 ×2 (+20) |
|  |  |  | 3 | kenbuild-20241213/buil1995/ENGINE.C `drawalls` (engine/nt) | 0.594 | 8286 | grouvline, ceilscan, florscan, wallscan ×8, wallmost ×2 (+1) | -4096 ×2, 19 ×2, 16384 ×2, 192 ×2, 11 ×2 (+17) |
| 1:39a7f `hline` (NEAR_MATCH) | 965 | 1.000 | 1 | kenbuild-20260331/build_1995_03_28_partial/ENGINE.C `hline` (engine/dos) | 0.603 | 881 | hchainasm64, hlineasm64 | 116, 20 ×4, 64, 6, 7 (+7) |
|  |  |  | 2 | kenbuild-20241213/BUIL0422/ENGINE.C `hline` (engine/dos) | 0.398 | 729 | hchainasm64, hlineasm64 | 655360, 20 ×2, 14 ×2, 6 ×2, 31 (+5) |
|  |  |  | 3 | kenbuild-20260331/build_1997_07_19/BEND.C `hbline` (engine/nt) | 0.392 | 620 | hlineasm64 | 36 ×2, 14 ×2, 6, 12 ×4, 8 ×2 (+2) |
| 1:3b390 `screencapture` (NEAR_MATCH) | 1511 | 0.991 | 1 | kenbuild-20260331/build_1995_03_28_partial/ENGINE.C `screencapture` (engine/dos) | 0.806 **strong** | 1375 | getpixel16 ×3, write ×7, close ×2, open | 307200, 64000, 967, 1000, 223 (+33) |
|  |  |  | 2 | kenbuild-20241213/buil1995/ENGINE.C `screencapture` (engine/nt) | 0.647 | 1340 | write ×7, close ×2, open | 967, 1000, 609, 640, 969 (+23) |
|  |  |  | 3 | buildsrc-20190520/SRC_9505/ENGINE.C `screencapture` (engine/dos) | 0.647 | 1337 | write ×7, close ×2, open | 967, 1000, 609, 640, 969 (+23) |
| 1:3c322 `drawmaskwall` (NEAR_MATCH) | 1890 | 0.975 | 1 | kenbuild-20260331/build_1995_03_28_partial/ENGINE.C `drawmaskwall` (engine/dos) | 0.721 **strong** | 1859 | transmaskwallscan, maskwallscan, wallfront, wallmost ×2, animateoffs | 134217728 ×2, -4096, 49152, 32768, 19 (+16) |
|  |  |  | 2 | buildsrc-20190520/SRC_9505/ENGINE.C `drawmaskwall` (engine/dos) | 0.709 | 1776 | transmaskwallscan, maskwallscan, wallfront, wallmost ×2, animateoffs | 134217728 ×2, -4096, 49152, 32768, 19 (+16) |
|  |  |  | 3 | kenbuild-20241213/buil1995/ENGINE.C `drawmaskwall` (engine/nt) | 0.693 | 1777 | transmaskwallscan, maskwallscan, wallfront, wallmost ×2, animateoffs | -4096, 49152, 32768, 19, 16384 ×2 (+15) |
| 1:3ca84 `drawsprite` (NEAR_MATCH) | 9875 | 0.938 | 1 | kenbuild-20260331/build_1995_03_28_partial/ENGINE.C `drawsprite` (engine/dos) | 0.755 **strong** | 10266 | ceilspritescan, spritewallfront, transmaskwallscan, maskwallscan, wallmost ×2 (+6) | 3584, 134217728 ×2, 16843009 ×2, 253, -4096 ×2 (+42) |
|  |  |  | 2 | buildsrc-20190520/SRC_9505/ENGINE.C `drawsprite` (engine/dos) | 0.733 **strong** | 9925 | ceilspritescan, spritewallfront, transmaskwallscan, maskwallscan, wallmost ×2 (+6) | 3584, 134217728 ×2, 16843009 ×2, 253, -4096 ×2 (+40) |
|  |  |  | 3 | kenbuild-20241213/buil1995/ENGINE.C `drawsprite` (engine/nt) | 0.718 | 10054 | ceilspritescan, spritewallfront, transmaskwallscan, maskwallscan, wallmost ×2 (+6) | 16843009 ×2, 253, -4096 ×2, 974, 49152 ×3 (+37) |
| 1:3f679 `overwritesprite` (NEAR_MATCH) | 1063 | 0.918 | 1 | kenbuild-20260331/build_1995_03_28_partial/ENGINE.C `overwritesprite` (engine/dos) | 0.693 | 1313 | tvlineasm1, mvlineasm1, loadtile | 232, 16843009, 16777216, 319, 964 (+15) |
|  |  |  | 2 | buildsrc-20190520/SRC_9505/ENGINE.C `overwritesprite` (engine/dos) | 0.597 | 1174 | tvlineasm1, mvlineasm1, loadtile | 16843009, 16777216, 964, 160, 100 (+9) |
|  |  |  | 3 | kenbuild-20241213/buil1995/ENGINE.C `overwritesprite` (engine/nt) | 0.595 | 1189 | tvlineasm1, mvlineasm1, loadtile | 16843009, 16777216, 964, 160, 100 (+9) |
| 1:3ffee `grouvline` (NEAR_MATCH) | 2169 | 0.912 | 1 | kenbuild-20260331/build_1995_03_28_partial/ENGINE.C `grouvline` (engine/dos) | 0.743 **strong** | 2225 | loadtile ×2 | 134217728 ×2, 16843009 ×2, 964, 640, 252 ×2 (+20) |
|  |  |  | 2 | buildsrc-20190520/SRC_9505/ENGINE.C `grouvline` (engine/dos) | 0.714 | 2163 | loadtile ×2 | 134217728 ×2, 16843009 ×2, 252 ×2, 320 ×4, 63 ×4 (+13) |
|  |  |  | 3 | kenbuild-20241213/buil1995/ENGINE.C `grouvline` (engine/nt) | 0.695 | 2058 | loadtile ×2 | 16843009 ×2, 252 ×2, 63 ×4, 1024, 10 ×5 (+11) |
| 1:411c4 `hitscan` (NEAR_MATCH) | 1745 | 0.959 | 1 | kenbuild-20241213/BUIL0422/ENGINE.C `hitscan` (engine/dos) | 0.637 | 1578 | lintersect | 38, 65535 ×5, 6, 31 ×2, 7 (+4) |
|  |  |  | 2 | kenbuild-20260331/build_1995_03_28_partial/ENGINE.C `hitscan` (engine/dos) | 0.635 | 1647 | -- | 48, 65535 ×5, 64, 31 ×2, 7 (+6) |
|  |  |  | 3 | kenbuild-20260331/build_1995_03_28_partial/ENGINE.C `neartag` (engine/dos) | 0.576 | 1522 | lintersect | 65535 ×3, 7, 32, 3 ×2, -1 ×2 (+3) |
| 1:41895 `neartag` (NEAR_MATCH) | 1644 | 0.968 | 1 | kenbuild-20260331/build_1995_03_28_partial/ENGINE.C `neartag` (engine/dos) | 0.691 | 1522 | lintersect | 14 ×5, 65535 ×3, 24, 2047 ×8, 7 (+5) |
|  |  |  | 2 | buildsrc-20190520/SRC_9505/ENGINE.C `neartag` (engine/dos) | 0.647 | 1411 | lintersect | 14 ×4, 65535 ×3, 24, 2047 ×6, 7 (+4) |
|  |  |  | 3 | kenbuild-20241213/buil1995/ENGINE.C `neartag` (engine/nt) | 0.615 | 1559 | lintersect | 14 ×4, 65535 ×3, 2047 ×6, 7, 3 ×4 (+3) |
| 1:42589 `getcliplines` (NEAR_MATCH) | 3589 | 0.868 | 1 | buildsrc-20190520/SRC_9505/ENGINE.C `getcliplines` (engine/dos) | 0.633 | 3551 | addclipline ×9, clipinsideboxline ×2 | 49152 ×8, 32768 ×5, 254, 1536, 11 (+15) |
|  |  |  | 2 | kenbuild-20241213/buil1995/ENGINE.C `getcliplines` (engine/nt) | 0.626 | 3531 | addclipline ×9, clipinsideboxline ×2 | 124, 49152 ×8, 32768 ×3, 254, 11 (+14) |
|  |  |  | 3 | kenbuild-20241213/buil062896/ENGINE.C `getzrange` (engine_oe/nt) | 0.409 | 3971 | -- | 49152 ×2, 1536, 48, 512, 65535 (+13) |
| 1:43532 `raytrace` (NEAR_MATCH) | 613 | 0.932 | 1 | kenbuild-20241213/buil1995/ENGINE.C `raytrace` (engine/nt) | 0.599 | 524 | -- | 52, 256, -1 |
|  |  |  | 2 | buildsrc-20190520/SRC_9505/ENGINE.C `raytrace` (engine/dos) | 0.599 | 524 | -- | 52, 256, -1 |
|  |  |  | 3 | kenbuild-20241213/BUILDBAK/ENGINE.C `raytrace` (engine/nt) | 0.583 | 519 | -- | 52, 256, -1 |
| 1:4393d `drawline16` (NEAR_MATCH) | 1541 | 0.911 | 1 | kenbuild-20241213/BUIL0422/BUILD.C `drawline16` (engine/dos) | 0.787 **strong** | 1562 | -- | -640, 65408, 639 ×5, 975 ×4, 640 ×9 (+13) |
|  |  |  | 2 | kenbuild-20241213/buil010294_BUILDOLD/BUILD.C `drawline640400` (engine/nt) | 0.600 | 1535 | -- | -640, 639 ×5, 640 ×9, 36, 20 (+7) |
|  |  |  | 3 | kenbuild-20241213/builsect/BUILD.C `drawline640400` (engine/nt) | 0.600 | 1535 | -- | -640, 639 ×5, 640 ×9, 36, 20 (+7) |
| 1:4445e `draw2dscreen` (NEAR_MATCH) | 3202 | 0.882 | 1 | kenbuild-20241213/BUIL0422/BUILD.C `drawoverhead` (engine/dos) | 0.706 | 3469 | drawline16 ×9 | 637 ×2, 974 ×34, 52, 38 ×2, 768 ×3 (+18) |
|  |  |  | 2 | buildsrc-20190520/SRC_9505/ENGINE.C `draw2dscreen` (engine/dos) | 0.559 | 3128 | clear2dscreen, draw2dgrid, drawline16 ×9 | 321 ×2, 637 ×2, 201 ×2, 319 ×2, 199 ×2 (+23) |
|  |  |  | 3 | kenbuild-20241213/buil1995/ENGINE.C `draw2dscreen` (engine/nt) | 0.557 | 3116 | clear2dscreen, draw2dgrid, drawline16 ×9 | 321 ×2, 637 ×2, 201 ×2, 319 ×2, 199 ×2 (+22) |
| 1:45667 `getzrange` (NEAR_MATCH) | 2938 | 0.782 | 1 | kenbuild-20241213/buil1995/ENGINE.C `getzrange` (engine/nt) | 0.517 | 2738 | clipinsideboxline | 49152 ×2, 768 ×2, 16384, 48, 65535 (+13) |
|  |  |  | 2 | kenbuild-20241213/BUILDBAK/ENGINE.C `getzrange` (engine/nt) | 0.506 | 2958 | clipinsideboxline | 49152 ×2, 768 ×2, 16384 ×3, 1536, 48 (+14) |
|  |  |  | 3 | kenbuild-20241213/buil1995/ENGINE.C `getcliplines` (engine/nt) | 0.502 | 3531 | clipinsideboxline ×5 | 49152 ×2, 768, 48, 2047 ×3, 1024 (+10) |
| 1:462c6 `setview` (NEAR_MATCH) | 334 | 0.845 | 1 | buildsrc-20190520/SRC_9505/ENGINE.C `setview` (engine/dos) | 0.534 | 327 | makeradarang | 320 ×2, 31 ×2, 16 ×2, 1 ×3 |
|  |  |  | 2 | kenbuild-20241213/buil1995/ENGINE.C `setview` (engine/nt) | 0.438 | 318 | makeradarang | 6, 31, 16 ×2, 2 ×2, 1 ×3 |
|  |  |  | 3 | kenbuild-20241213/buil062896/ENGINE.C `setview` (engine_oe/nt) | 0.353 | 415 | -- | 320 ×2, 6, 16 ×2, 2 ×2, 1 ×4 |
| 1:46c60 `rotatesprite` (NEAR_MATCH) | 1497 | 0.792 | 1 | buildsrc-20190520/SRC_9505/ENGINE.C `rotatesprite` (engine/dos) | 0.468 | 1752 | spritevline, animateoffs, loadtile | 16843009, 964, 80, 192, 14 ×3 (+10) |
|  |  |  | 2 | kenbuild-20241213/buil1995/ENGINE.C `rotatesprite` (engine/nt) | 0.461 | 2016 | spritevline, animateoffs, loadtile | 16843009, 964, 320, 192, 14 ×3 (+10) |
|  |  |  | 3 | kenbuild-20241213/BUIL0422/ENGINE.C `drawfacesprite` (engine/dos) | 0.403 | 1699 | animateoffs | 319, 964, 655360, 100, 320 ×2 (+13) |
| 1:47239 `makepalookup` (NEAR_MATCH) | 599 | 0.888 | 1 | kenbuild-20241213/buil1995/ENGINE.C `makepalookup` (engine/nt) | 0.571 | 616 | initcache, _nmalloc_ | 2147483647, 63 ×2, 256 ×4, 31, 255 (+4) |
|  |  |  | 2 | buildsrc-20190520/SRC_9505/ENGINE.C `makepalookup` (engine/dos) | 0.561 | 618 | initcache, _nmalloc_ | 2147483647, 63 ×2, 256 ×4, 31, 255 (+4) |
|  |  |  | 3 | kenbuild-20260331/build_1996_11_12_partial/ENGINE.C `makepalookup` (engine_oe/nt) | 0.398 | 655 | -- | 63 ×2, 256 ×5, 32, 255 ×2, 8 ×2 (+2) |
| 1:47490 `setvesamode` (NEAR_MATCH) | 292 | 1.000 | 1 | buildsrc-20190520/19950920/POLYTEX5.C `setvesamode` (engine/dos) | 1.000 **strong** | 292 | int386x ×2, memset ×2 | 20225, 20226, 50, 49 ×2, 79 ×2 (+11) |
|  |  |  | 2 | buildsrc-20190520/SRC_9505/ENGINE.C `setvesamode` (engine/dos) | 1.000 **strong** | 292 | int386x ×2, memset ×2 | 20225, 20226, 50, 49 ×2, 79 ×2 (+11) |
|  |  |  | 3 | buildsrc-20190520/19951017/POLYTEX5.C `setvesamode` (engine/nt) | 1.000 **strong** | 292 | int386x ×2, memset ×2 | 20225, 20226, 50, 49 ×2, 79 ×2 (+11) |
| 1:47699 `setverts` (NEAR_MATCH) | 551 | 1.000 | 1 | buildsrc-20190520/19950920/POLYTEX5.C `setverts` (engine/dos) | 1.000 **strong** | 551 | inp ×5, outp ×20 | 223, 240, 981 ×15, 980 ×10, 21 (+18) |
|  |  |  | 2 | buildsrc-20190520/SRC_9505/ENGINE.C `setverts` (engine/dos) | 1.000 **strong** | 551 | inp ×5, outp ×20 | 223, 240, 981 ×15, 980 ×10, 21 (+18) |
|  |  |  | 3 | buildsrc-20190520/19951017/POLYTEX5.C `setverts` (engine/nt) | 1.000 **strong** | 551 | inp ×5, outp ×20 | 223, 240, 981 ×15, 980 ×10, 21 (+18) |
| 1:479aa `setredbluepalette` (NEAR_MATCH) | 176 | 1.000 | 1 | buildsrc-20190520/SRC_9505/ENGINE.C `setbrightness` (engine/dos) | 0.637 | 324 | -- | 968, 969 ×3, 15 ×2, 256 ×2, 31 (+7) |
|  |  |  | 2 | kenbuild-20241213/buil1995/ENGINE.C `setbrightness` (engine/nt) | 0.637 | 324 | -- | 968, 969 ×3, 15 ×2, 256 ×2, 31 (+7) |
|  |  |  | 3 | buildsrc-20190520/19950920/POLYTEX5.C `setpalette` (engine/dos) | 0.581 | 320 | -- | 968, 969 ×3, 15 ×2, 256 ×2, 31 (+7) |

### game_sounds

| Oracle function | Oracle B | Current | Rank | Donor | Score | Donor B | Shared calls | Shared immediates |
|---|---:|---:|---:|---|---:|---:|---|---|
| 1:2ba30 `bootleg_cache_alloc` (STRICT) | 112 | 1.000 | 1 | kenbuild-20241213/2DRW0628/GAME.C `movethings` (game/nt) | 0.368 | 144 | -- | 127 |
|  |  |  | 2 | kenbuild-20260331/build_1997_07_19/CACHE1D.C `initcache` (engine/nt) | 0.318 | 93 | -- | -- |
|  |  |  | 3 | Duke 1.3D tree/menues.c `saveplayer` (game/nt) | 0.318 | 96 | -- | -- |
| 1:2baa0 `bootleg_cache_pop` (STRICT) | 64 | 1.000 | 1 | kenbuild-20241213/BUIL0422/BUILD.C `initeditor` (engine/dos) | 0.450 | 73 | -- | -- |
|  |  |  | 2 | kenbuild-20260331/FILES_FROM_MATT/2draw/2DRAW.C `uninitvesa` (engine/nt) | 0.415 | 61 | -- | -- |
|  |  |  | 3 | kenbuild-20260331/build_1994_01_09/bui94109.c `insertsprite` (engine/nt) | 0.400 | 64 | -- | -- |
| 1:2bae0 `bootleg_cache_reset` (STRICT) | 80 | 1.000 | 1 | kenbuild-20241213/BUIL1020/GAME.C `dointerpolations` (game/nt) | 0.493 | 96 | -- | 4 |
|  |  |  | 2 | Duke 1.5/ACTORS.C `restoreinterpolations` (game/nt) | 0.486 | 48 | -- | 4 |
|  |  |  | 3 | Duke 1.3D tree/actors.c `restoreinterpolations` (game/nt) | 0.486 | 48 | -- | 4 |
| 1:2bb30 `init_bootleg_cache` (STRICT) | 48 | 1.000 | 1 | buildsrc-20190520/19950920/POLYTEX5.C `initcache` (engine/dos) | 0.501 | 47 | -- | -- |
|  |  |  | 2 | buildsrc-20190520/19951017/POLYTEX5.C `initcache` (engine/nt) | 0.501 | 47 | -- | -- |
|  |  |  | 3 | kenbuild-20241213/buil062896/ENGINE.C `uninitvesa` (engine/nt) | 0.487 | 32 | -- | -- |
| 1:2bb60 `USRHOOKS_GetMem` (STRICT) | 48 | 1.000 | 1 | Duke 1.5/GAME.C `uninittimer` (game/nt) | 0.392 | 48 | -- | -- |
|  |  |  | 2 | Duke 1.3D tree/game.c `uninittimer` (game/nt) | 0.392 | 48 | -- | -- |
|  |  |  | 3 | Duke 1.3D tree/timer.c `uninittimer` (game/nt) | 0.392 | 35 | -- | -- |
| 1:2bb90 `USRHOOKS_FreeMem` (STRICT) | 64 | 1.000 | 1 | kenbuild-20241213/BUIL0422/BUILD.C `initeditor` (engine/dos) | 0.442 | 73 | -- | -- |
|  |  |  | 2 | kenbuild-20260331/FILES_FROM_MATT/2draw/2DRAW.C `uninitvesa` (engine/nt) | 0.424 | 61 | -- | -- |
|  |  |  | 3 | kenbuild-20260331/FILES_FROM_MATT/2draw/2DRAW.C `unloadpics` (engine/nt) | 0.402 | 60 | -- | -- |
| 1:2bbd0 `loadsounds` (STRICT) | 112 | 1.000 | 1 | kenbuild-20260331/build_1997_07_19/WSAY.C `loadtables` (engine/nt) | 0.501 | 91 | read, close, open | 514, 256, 12, -1 |
|  |  |  | 2 | kenbuild-20241213/builold!/BUILD.C `loadtables` (engine/nt) | 0.492 | 81 | read, close, open | 514, 256, 12, -1 |
|  |  |  | 3 | kenbuild-20260331/buildold/VOXEL1.C `loadtables` (engine/nt) | 0.492 | 81 | read, close, open | 514, 256, 12, -1 |
| 1:2bc40 `playmusic` (STRICT) | 112 | 1.000 | 1 | Duke 1.5/GAMEDEF.C `copydefaultcons` (game/nt) | 0.473 | 144 | fopen, fclose | 1 ×2 |
|  |  |  | 2 | kenbuild-20241213/buil112093/BUILD.C `loadtables` (engine/nt) | 0.458 | 120 | -- | 0, 1 |
|  |  |  | 3 | kenbuild-20241213/buil112493/BUILD.C `loadtables` (engine/nt) | 0.458 | 120 | -- | 0, 1 |
| 1:2bcb0 `xyzsound` (STRICT) | 448 | 1.000 | 1 | kenbuild-20241213/BUI1101B/GAME.C `analyzesprites` (game/nt) | 0.341 | 528 | getangle | 6, 7, 5, 16 ×2, 2 (+1) |
|  |  |  | 2 | kenbuild-20241213/BUIL1020/GAME.C `analyzesprites` (game/nt) | 0.341 | 528 | getangle | 6, 7, 5, 16 ×2, 2 (+1) |
|  |  |  | 3 | kenbuild-20241213/buil0723/GAME.C `analyzesprites` (game/dos) | 0.329 | 336 | getangle | 7, 16 ×2, 2, 4 ×2 |
| 1:2be70 `spritesound` (NEAR_MATCH) | 464 | 0.951 | 1 | kenbuild-20241213/BUI1101B/GAME.C `analyzesprites` (game/nt) | 0.342 | 528 | getangle | 6, 7, 5, 2, 4 ×2 |
|  |  |  | 2 | kenbuild-20241213/BUIL1020/GAME.C `analyzesprites` (game/nt) | 0.342 | 528 | getangle | 6, 7, 5, 2, 4 ×2 |
|  |  |  | 3 | Duke 1.3D tree/actors.c `bounce` (game/nt) | 0.334 | 496 | getangle | 17, 20 ×2, 7, 5 |
| 1:2c040 `stopsound` (STRICT) | 80 | 1.000 | 1 | Duke 1.5/SOUNDS.C `stopenvsound` (game/nt) | 0.277 | 128 | fx_stopsound | -- |
|  |  |  | 2 | Duke 1.3D tree/sounds.c `stopenvsound` (game/nt) | 0.277 | 128 | fx_stopsound | -- |
|  |  |  | 3 | Duke 1.5/ACTORS.C `addammo` (game/nt) | 0.244 | 112 | -- | -- |
| 1:2c090 `pan3dsound` (STRICT) | 256 | 1.000 | 1 | Duke 1.5/SECTOR.C `findotherplayer` (game/dos) | 0.352 | 240 | -- | 0, 4 ×3 |
|  |  |  | 2 | kenbuild-20241213/BUILDBAK/BUILD.C `getlinehighlight` (engine/nt) | 0.321 | 240 | -- | -1, 0 |
|  |  |  | 3 | Duke 1.3D tree/sector.c `findotherplayer` (game/nt) | 0.317 | 208 | -- | 0, 4 |
| 1:2c190 `TestCallBack` (STRICT) | 43 | 1.000 | 1 | Duke 1.5/ACTORS.C `addammo` (game/nt) | 0.223 | 112 | -- | -- |
|  |  |  | 2 | Duke 1.3D tree/actors.c `addammo` (game/nt) | 0.223 | 112 | -- | -- |
|  |  |  | 3 | Duke 1.5/ACTORS.C `updateinterpolations` (game/nt) | 0.203 | 48 | -- | -- |

### game_gamedef

| Oracle function | Oracle B | Current | Rank | Donor | Score | Donor B | Shared calls | Shared immediates |
|---|---:|---:|---:|---|---:|---:|---|---|
| 1:2e700 `getincangle_shift` (STRICT) | 64 | 1.000 | 1 | Duke 1.3D tree/gamedef.c `getincangle` (game/dos) | 0.464 | 96 | -- | 1024, 7 ×2 |
|  |  |  | 2 | Duke 1.5/GAMEDEF.C `getincangle` (game/nt) | 0.464 | 96 | -- | 1024, 7 ×2 |
|  |  |  | 3 | buildsrc-20190520/SRC_9505/ENGINE.C `calcksqrt` (engine/dos) | 0.249 | 62 | -- | -- |
| 1:2e740 `ispecial` (STRICT) | 48 | 1.000 | 1 | Duke 1.3D tree/gamedef.c `ispecial` (game/dos) | 1.000 **strong** | 48 | -- | 13, 10, 32, 1 ×2 |
|  |  |  | 2 | Duke 1.5/GAMEDEF.C `ispecial` (game/nt) | 1.000 **strong** | 48 | -- | 13, 10, 32, 1 ×2 |
|  |  |  | 3 | Duke 1.3D tree/gamedef.c `isaltok` (game/dos) | 0.388 | 64 | -- | 1 |
| 1:2e770 `isaltok` (STRICT) | 64 | 1.000 | 1 | Duke 1.3D tree/gamedef.c `isaltok` (game/dos) | 1.000 **strong** | 64 | -- | 125, 45, 42, 123, 47 (+5) |
|  |  |  | 2 | Duke 1.5/GAMEDEF.C `isaltok` (game/nt) | 1.000 **strong** | 64 | -- | 125, 45, 42, 123, 47 (+5) |
|  |  |  | 3 | Duke 1.3D tree/gamedef.c `ispecial` (game/dos) | 0.388 | 48 | -- | 1 |
| 1:2e7b0 `getlabel` (STRICT) | 160 | 1.000 | 1 | Duke 1.3D tree/gamedef.c `getlabel` (game/dos) | 1.000 **strong** | 160 | -- | 224, 13, 10 ×2, 6 ×2, 32 (+2) |
|  |  |  | 2 | Duke 1.5/GAMEDEF.C `getlabel` (game/nt) | 1.000 **strong** | 160 | -- | 224, 13, 10 ×2, 6 ×2, 32 (+2) |
|  |  |  | 3 | Duke 1.5/GAME.C `adduserquote` (game/nt) | 0.347 | 176 | -- | 6, 0 |
| 1:2e850 `keyword` (STRICT) | 208 | 1.000 | 1 | Duke 1.3D tree/gamedef.c `keyword` (game_oe/dos) | 0.992 **strong** | 208 | strcmp | 125 ×2, 45 ×2, 42 ×2, 123 ×2, 47 ×2 (+5) |
|  |  |  | 2 | Duke 1.5/GAMEDEF.C `keyword` (game_oe/nt) | 0.992 **strong** | 208 | strcmp | 125 ×2, 45 ×2, 42 ×2, 123 ×2, 47 ×2 (+5) |
|  |  |  | 3 | Duke 1.3D tree/gamedef.c `transword` (game_oe/dos) | 0.700 | 320 | strcmp | 125 ×2, 45 ×2, 42 ×2, 123 ×2, 47 ×2 (+5) |
| 1:2e920 `transword` (STRICT) | 448 | 1.000 | 1 | Duke 1.3D tree/gamedef.c `transword` (game_oe/dos) | 0.768 **strong** | 320 | strcmp, printf | 125 ×2, 45 ×2, 42 ×2, 123 ×2, 47 ×2 (+7) |
|  |  |  | 2 | Duke 1.3D tree/gamedef.c `transnum` (game_oe/dos) | 0.725 **strong** | 1024 | strcmp ×2, printf | 125 ×2, 45 ×2, 42 ×2, 123 ×2, 47 ×2 (+7) |
|  |  |  | 3 | Duke 1.5/GAMEDEF.C `transnum` (game_oe/nt) | 0.725 **strong** | 1088 | strcmp ×2, printf | 125 ×2, 45 ×2, 42 ×2, 123 ×2, 47 ×2 (+7) |
| 1:2eae0 `transnum` (NEAR_MATCH) | 640 | 1.000 | 1 | Duke 1.3D tree/gamedef.c `transnum` (game_oe/dos) | 0.862 **strong** | 1024 | atol, strcmp, printf | 125 ×2, 45 ×3, 42 ×2, 123 ×2, 47 ×2 (+9) |
|  |  |  | 2 | Duke 1.5/GAMEDEF.C `transnum` (game_oe/nt) | 0.862 **strong** | 1088 | atol, strcmp, printf | 125 ×2, 45 ×3, 42 ×2, 123 ×2, 47 ×2 (+9) |
|  |  |  | 3 | Duke 1.3D tree/gamedef.c `transword` (game_oe/dos) | 0.730 **strong** | 320 | strcmp, printf | 125 ×2, 45 ×2, 42 ×2, 123 ×2, 47 ×2 (+6) |
| 1:2ed60 `parsecommand` (NEAR_MATCH) | 5232 | 0.927 | 1 | Duke 1.5/GAMEDEF.C `parsecommand` (game_oe/nt) | 0.677 | 8784 | ispecial ×6, transnum ×14, transword, parsecommand ×5, isaltok ×18 (+3) | 117, 39 ×2, 125 ×6, 45 ×6, 42 ×7 (+22) |
|  |  |  | 2 | Duke 1.3D tree/gamedef.c `parsecommand` (game/dos) | 0.604 | 5584 | transnum ×14, transword, parsecommand ×5, isaltok ×6, puts (+2) | 117 ×2, 39 ×2, 42, 47, 51 (+15) |
|  |  |  | 3 | Duke 1.3D tree/gamedef.c `passone` (game_oe/dos) | 0.529 | 5280 | transnum ×14, transword, parsecommand ×5, isaltok ×6, puts (+2) | 117 ×2, 39 ×2, 42, 47, 51 (+15) |
| 1:301d0 `passone` (STRICT) | 16 | 1.000 | 1 | Duke 1.3D tree/gamedef.c `passone` (game/dos) | 0.233 | 48 | parsecommand | -- |
|  |  |  | 2 | Duke 1.5/GAMEDEF.C `passone` (game/nt) | 0.233 | 48 | parsecommand | -- |
|  |  |  | 3 | Duke 1.5/SOUNDS.C `SoundShutdown` (game/nt) | 0.118 | 48 | -- | -- |
| 1:301e0 `loadefs` (STRICT) | 464 | 1.000 | 1 | Duke 1.3D tree/gamedef.c `loadefs` (game/dos) | 0.454 | 704 | parsecommand, puts ×4, printf ×3 | 4096, 12 ×2, 8 ×2, 0 ×4, 2 ×2 (+2) |
|  |  |  | 2 | kenbuild-20241213/PTEX0726/GAME.C `initgame` (game/nt) | 0.397 | 368 | read, close, open | 514, 256, 12 ×2, 8, 4 |
|  |  |  | 3 | Duke 1.5/GAMEDEF.C `loadefs` (game/nt) | 0.375 | 800 | puts ×4, printf ×3 | 4096, 12 ×2, 8 ×2, 0 ×5, 2 ×2 (+2) |
| 1:303b0 `furthestangle` (STRICT) | 240 | 1.000 | 1 | Duke 1.3D tree/gamedef.c `furthestangle` (game/dos) | 0.651 | 272 | hitscan | 2048 ×3, 512, 2047, 31, 7 (+2) |
|  |  |  | 2 | Duke 1.5/GAMEDEF.C `furthestangle` (game/nt) | 0.651 | 272 | hitscan | 2048 ×3, 512, 2047, 31, 7 (+2) |
|  |  |  | 3 | Duke 1.5/PLAYER.C `hitawall` (game/nt) | 0.484 | 160 | hitscan | 512, 2047, 7, 0 |
| 1:304a0 `move_actor` (NEAR_MATCH) | 1040 | 0.870 | 1 | Duke 1.5/ACTORS.C `movedummyplayers` (game/nt) | 0.320 | 560 | -- | 8 ×3, 0 ×2, 2, 4 ×3, 1 ×3 |
|  |  |  | 2 | Duke 1.3D tree/actors.c `moveplayers` (game/nt) | 0.315 | 944 | ssp, getangle | 8192, 128, 32, 8 ×3, 0 ×2 (+3) |
|  |  |  | 3 | Duke 1.5/GAMEDEF.C `alterang` (game/nt) | 0.311 | 688 | getangle | 128, 8 ×2, 0, 2 ×2, 4 ×5 (+1) |
| 1:308b0 `parse` (NEAR_MATCH) | 6576 | 0.742 | 1 | Duke 1.3D tree/gamedef.c `parse` (game/dos) | 0.394 | 14672 | ifsquished, parse ×40, ifhitbyweapon, guts, hitradius (+8) | 348, 12288, -8 ×2, -128, 8192 (+21) |
|  |  |  | 2 | Duke 1.5/GAMEDEF.C `parse` (game/nt) | 0.350 | 18752 | ifsquished, parse ×40, ifhitbyweapon, guts, hitradius (+8) | 348, 12288, -8 ×2, -128, 8192 (+21) |
|  |  |  | 3 | Duke 1.5/SECTOR.C `checkhitsprite` (game/dos) | 0.331 | 6064 | guts, shoot, spawn, egs ×2, spritesound (+2) | 12288, -8 ×3, 511, 8192, 2048 (+20) |
| 1:32260 `execute` (NEAR_MATCH) | 7164 | 0.761 | 1 | Duke 1.3D tree/actors.c `movestandables` (game/nt) | 0.348 | 7664 | ifhitbyweapon, hitradius, ssp ×2, spawn, egs ×2 (+4) | 348, 12288, -8 ×3, 38 ×2, 8192 ×2 (+21) |
|  |  |  | 2 | Duke 1.5/ACTORS.C `movestandables` (game/nt) | 0.344 | 8080 | ifhitbyweapon, hitradius, ssp ×2, spawn, egs ×2 (+4) | 348, 12288, -8 ×3, 38 ×2, 8192 ×2 (+20) |
|  |  |  | 3 | Duke 1.5/SECTOR.C `operateactivators` (game_oe/dos) | 0.333 | 5664 | spawn, spritesound | 8192, 127, 11, 64 ×3, 44 ×2 (+12) |

### game_menues

| Oracle function | Oracle B | Current | Rank | Donor | Score | Donor B | Shared calls | Shared immediates |
|---|---:|---:|---:|---|---:|---:|---|---|
| 1:2c1c0 `initmenus` (NEAR_MATCH) | 624 | 0.898 | 1 | Duke 1.3D tree/premap.c `resetprestat` (game_oe/nt) | 0.351 | 752 | -- | 6, 5 ×2, 8, 1 ×2 |
|  |  |  | 2 | Duke 1.5/PREMAP.C `resetplayerstats` (game/nt) | 0.349 | 816 | -- | 9, 6, 7, 5, 4 (+1) |
|  |  |  | 3 | Duke 1.5/PREMAP.C `setupbackdrop` (game/nt) | 0.341 | 288 | -- | 4 ×2, 1 ×2 |
| 1:2c430 `draw_menu_slider` (STRICT) | 352 | 1.000 | 1 | buildsrc-20190520/SRC_9505/ENGINE.C `copytilepiece` (engine/dos) | 0.255 | 325 | -- | 255, 0 ×4 |
|  |  |  | 2 | kenbuild-20241213/buil062896/ENGINE.C `copytilepiece` (engine_oe/nt) | 0.255 | 325 | -- | 255, 0 ×4 |
|  |  |  | 3 | kenbuild-20241213/buil1995/ENGINE.C `copytilepiece` (engine/nt) | 0.255 | 325 | -- | 255, 0 ×4 |
| 1:2c590 `drawmenu` (STRICT) | 352 | 1.000 | 1 | Duke 1.5/GAME.C `gamenumber` (game_oe/nt) | 0.309 | 304 | -- | 57, 26, 48, 10, 32 ×2 (+4) |
|  |  |  | 2 | Duke 1.3D tree/menues.c `menutextc` (game/nt) | 0.304 | 544 | -- | 122, 65, 57, 90, 97 (+8) |
|  |  |  | 3 | Duke 1.3D tree/game.c `gamenumber` (game_oe/nt) | 0.303 | 304 | -- | 57, 26, 48, 10, 32 ×2 (+4) |
| 1:2c6f0 `probe` (STRICT) | 400 | 1.000 | 1 | kenbuild-20241213/2DRW0628/GAME.C `checkmasterslaveswitch` (game/nt) | 0.288 | 416 | -- | 16, 0, 2 ×3, 4 |
|  |  |  | 2 | kenbuild-20260331/build_1997_07_19/ADPCM.C `adpcmencode` (engine/nt) | 0.278 | 328 | -- | 31, 2 ×2, 1 ×2 |
|  |  |  | 3 | kenbuild-20260331/buildold/EDITART.C `fillregion` (engine/nt) | 0.264 | 322 | -- | -1, 16, 8 ×4, 0 ×2, 1 ×2 |
| 1:2c880 `draw_and_probe` (NEAR_MATCH) | 1136 | 0.974 | 1 | Duke 1.5/GAME.C `strget` (game_oe/nt) | 0.327 | 976 | rotatesprite | 57, 26, 48, 10, 31 (+8) |
|  |  |  | 2 | Duke 1.3D tree/game.c `strget` (game_oe/nt) | 0.325 | 960 | rotatesprite | 57, 26, 48, 10, 31 (+8) |
|  |  |  | 3 | Duke 1.3D tree/menues.c `menutextc` (game/nt) | 0.309 | 544 | rotatesprite | 122, 65, 57, 90, 97 (+9) |
| 1:2ccf0 `menus` (NEAR_MATCH) | 4976 | 0.949 | 1 | Duke 1.3D tree/game.c `displayrest` (game_oe/nt) | 0.302 | 4272 | gametext ×4, rotatesprite ×10 | 50, 115, 30 ×2, 160, 100 (+18) |
|  |  |  | 2 | Duke 1.5/GAME.C `displayrest` (game_oe/nt) | 0.288 | 3552 | gametext ×2, rotatesprite ×10 | 50, 115, 160, 100, 20 ×3 (+15) |
|  |  |  | 3 | kenbuild-20241213/BUI1101B/GAME.C `drawscreen` (game/nt) | 0.282 | 7088 | rotatesprite ×10 | 10485760 ×9, 60, 160, 40 ×2, 14 ×3 (+16) |
| 1:2e060 `draw_fade_transition` (STRICT) | 112 | 1.000 | 1 | Duke 1.5/GAME.C `myospal` (game/nt) | 0.317 | 112 | rotatesprite | 16, 8 |
|  |  |  | 2 | Duke 1.3D tree/game.c `myospal` (game/nt) | 0.317 | 112 | rotatesprite | 16, 8 |
|  |  |  | 3 | Duke 1.5/GAME.C `myos` (game/nt) | 0.316 | 176 | rotatesprite | 16 |
| 1:2e0d0 `vscrn` (STRICT) | 80 | 1.000 | 1 | kenbuild-20241213/2DRW0628/GAME.C `setviewboundaries` (game/nt) | 0.375 | 80 | setview | -- |
|  |  |  | 2 | kenbuild-20260331/FILES_FROM_MATT/2draw/GAME.C `setviewboundaries` (game/nt) | 0.375 | 80 | setview | -- |
|  |  |  | 3 | Duke 1.3D tree/sector.c `ldist` (game/nt) | 0.315 | 48 | -- | -- |
| 1:2e120 `initlava` (STRICT) | 224 | 1.000 | 1 | kenbuild-20241213/buil0723/GAME.C `initlava` (game/dos) | 1.000 **strong** | 224 | ksqrt ×2 | -16 ×2, 17, 9, 128, 31 (+5) |
|  |  |  | 2 | kenbuild-20241213/BUI1101B/GAME.C `initlava` (game/nt) | 1.000 **strong** | 224 | ksqrt ×2 | -16 ×2, 17, 9, 128, 31 (+5) |
|  |  |  | 3 | kenbuild-20241213/BUIL1020/GAME.C `initlava` (game/nt) | 1.000 **strong** | 224 | ksqrt ×2 | -16 ×2, 17, 9, 128, 31 (+5) |
| 1:2e200 `movelava` (STRICT) | 1248 | 1.000 | 1 | kenbuild-20241213/buil0723/GAME.C `movelava` (game/dos) | 0.988 **strong** | 1248 | rand ×2 | 16256, 16769, 16770, 16896, 130 ×5 (+15) |
|  |  |  | 2 | kenbuild-20241213/buil1995/GAME.C `movelava` (game/nt) | 0.988 **strong** | 1248 | rand ×2 | 16256, 16769, 16770, 16896, 130 ×5 (+15) |
|  |  |  | 3 | kenbuild-20241213/BUI1101B/GAME.C `movelava` (game/nt) | 0.624 | 1168 | rand ×2 | 127 ×4, 9, 10, 128 ×4, 31 (+5) |
| 1:2e6e0 `resetparallax` (STRICT) | 26 | 1.000 | 1 | buildsrc-20190520/SRC_9505/ENGINE.C `resettiming` (engine/dos) | 0.473 | 23 | -- | -- |
|  |  |  | 2 | kenbuild-20260331/build_1995_03_28_partial/ENGINE.C `resettiming` (engine/dos) | 0.473 | 23 | -- | -- |
|  |  |  | 3 | kenbuild-20241213/buil1995/ENGINE.C `resettiming` (engine/nt) | 0.473 | 23 | -- | -- |

### game_sector

| Oracle function | Oracle B | Current | Rank | Donor | Score | Donor B | Shared calls | Shared immediates |
|---|---:|---:|---:|---|---:|---:|---|---|
| 1:14fb0 `checkcursectnums` (STRICT) | 80 | 1.000 | 1 | Duke 1.5/SECTOR.C `checkcursectnums` (game/dos) | 0.779 **strong** | 96 | -- | -1, 4 |
|  |  |  | 2 | Duke 1.3D tree/sector.c `checkcursectnums` (game/nt) | 0.725 **strong** | 80 | -- | -1 |
|  |  |  | 3 | Duke 1.5/ACTORS.C `ifhitsectors` (game/nt) | 0.602 | 80 | -- | -1 |
| 1:15000 `findplayer` (STRICT) | 160 | 1.000 | 1 | Duke 1.3D tree/sector.c `findotherplayer` (game/nt) | 0.649 | 208 | -- | 2147483647, 8 ×2, 4 |
|  |  |  | 2 | Duke 1.5/SECTOR.C `findotherplayer` (game/dos) | 0.639 | 240 | -- | 2147483647, 8 ×2, 4 |
|  |  |  | 3 | Duke 1.3D tree/sector.c `findplayer` (game/nt) | 0.424 | 304 | -- | 2147483647, 4 |
| 1:150a0 `doanimations` (STRICT) | 672 | 1.000 | 1 | buildsrc-20190520/19961013/GAME.C `doanimations` (game_oe/nt) | 0.365 | 448 | -- | 12 ×2, 4 ×3 |
|  |  |  | 2 | kenbuild-20241213/BUI1101B/GAME.C `doanimations` (game_oe/nt) | 0.365 | 448 | -- | 12 ×2, 4 ×3 |
|  |  |  | 3 | kenbuild-20241213/BUIL0615/GAME.C `doanimations` (game_oe/nt) | 0.365 | 448 | -- | 12 ×2, 4 ×3 |
| 1:15340 `getanimationgoal` (STRICT) | 64 | 1.000 | 1 | kenbuild-20241213/BUIL0422/ENGINE.C `getanimationgoal` (engine/dos) | 0.670 | 44 | -- | -1 |
|  |  |  | 2 | Duke 1.5/SECTOR.C `getanimationgoal` (game/dos) | 0.528 | 64 | -- | -1, 4 |
|  |  |  | 3 | Duke 1.3D tree/sector.c `getanimationgoal` (game/nt) | 0.528 | 64 | -- | -1, 4 |
| 1:15380 `setanimation` (STRICT) | 144 | 1.000 | 1 | Duke 1.5/SECTOR.C `setanimation` (game/dos) | 0.550 | 160 | -- | -1, 4 |
|  |  |  | 2 | Duke 1.3D tree/sector.c `setanimation` (game/nt) | 0.550 | 160 | -- | -1, 4 |
|  |  |  | 3 | kenbuild-20241213/buil0723/GAME.C `setanimation` (game/dos) | 0.533 | 128 | -- | -1, 4 |
| 1:15410 `animatecamsprite` (STRICT) | 320 | 1.000 | 1 | Duke 1.5/SECTOR.C `animatecamsprite` (game/dos) | 0.311 | 208 | -- | 2048, 11, 4 |
|  |  |  | 2 | kenbuild-20241213/BUIL0422/ENGINE.C `insertsprite` (engine_oe/dos) | 0.299 | 293 | -- | 38 ×2, -1 |
|  |  |  | 3 | Duke 1.3D tree/sector.c `findplayer` (game/nt) | 0.291 | 304 | -- | 4 ×2 |
| 1:15550 `animatewalls` (STRICT) | 704 | 1.000 | 1 | Duke 1.5/SECTOR.C `animatewalls` (game/dos) | 0.296 | 560 | -- | 254, 128 ×2, 255, 16, 0 (+2) |
|  |  |  | 2 | Duke 1.3D tree/sector.c `animatewalls` (game/nt) | 0.265 | 480 | -- | 254, 128 ×2, 255, 12, 16 (+3) |
|  |  |  | 3 | Duke 1.5/ACTORS.C `movefta` (game/nt) | 0.264 | 896 | -- | -1, 8 ×2, 0, 4, 1 |
| 1:15810 `activatewarpelevators` (NEAR_MATCH) | 464 | 1.000 | 1 | Duke 1.5/SECTOR.C `activatewarpelevators` (game/dos) | 0.687 | 480 | spritesound | 17 ×2, -1, 0 |
|  |  |  | 2 | Duke 1.3D tree/sector.c `activatewarpelevators` (game/nt) | 0.687 | 480 | spritesound | 17 ×2, -1, 0 |
|  |  |  | 3 | Duke 1.5/SECTOR.C `allignwarpelevators` (game/dos) | 0.353 | 368 | -- | 17 ×2 |
| 1:159e0 `operatesectors` (NEAR_MATCH) | 10288 | 0.883 | 1 | Duke 1.3D tree/sector.c `operatesectors` (game_oe/nt) | 0.396 | 18448 | operatesectors ×2, nextsectorneighborz ×11, spritesound ×17 | 21 ×2, 26, 22 ×2, 17 ×9, 20 (+16) |
|  |  |  | 2 | Duke 1.5/SECTOR.C `operatesectors` (game_oe/dos) | 0.395 | 18432 | operatesectors ×2, nextsectorneighborz ×11, spritesound ×17 | 21 ×2, 26, 22 ×2, 17 ×9, 20 (+16) |
|  |  |  | 3 | Duke 1.5/SECTOR.C `operateactivators` (game_oe/dos) | 0.390 | 5664 | operatesectors ×2, nextsectorneighborz ×11, spritesound ×4 | 21 ×2, 26, 18, 22 ×2, 17 (+16) |
| 1:18210 `operateactivators` (NEAR_MATCH) | 4912 | 0.837 | 1 | Duke 1.3D tree/sector.c `operateactivators` (game_oe/nt) | 0.567 | 5408 | activatewarpelevators ×4, getanimationgoal ×4, operatesectors ×2, setanimation ×18, nextsectorneighborz ×11 (+1) | 21 ×2, 26, 22 ×2, 17, 20 (+16) |
|  |  |  | 2 | Duke 1.5/SECTOR.C `operateactivators` (game_oe/dos) | 0.562 | 5664 | activatewarpelevators ×4, getanimationgoal ×4, operatesectors ×2, setanimation ×18, nextsectorneighborz ×11 (+1) | 21 ×2, 26, 18, 22 ×2, 17 (+17) |
|  |  |  | 3 | Duke 1.5/SECTOR.C `activatebysector` (game_oe/dos) | 0.547 | 4752 | activatewarpelevators ×4, getanimationgoal ×4, operatesectors ×2, setanimation ×18, nextsectorneighborz ×11 | 21 ×2, 26, 18, 22 ×2, 17 (+17) |
| 1:19540 `operatemasterswitches` (STRICT) | 80 | 1.000 | 1 | Duke 1.5/SECTOR.C `operatemasterswitches` (game/dos) | 0.900 **strong** | 80 | -- | 0, 1 |
|  |  |  | 2 | Duke 1.3D tree/sector.c `operatemasterswitches` (game/nt) | 0.900 **strong** | 80 | -- | 0, 1 |
|  |  |  | 3 | Duke 1.5/ACTORS.C `ifhitsectors` (game/nt) | 0.613 | 80 | -- | -- |
| 1:19590 `operateforcefields` (NEAR_MATCH) | 400 | 0.925 | 1 | Duke 1.5/SECTOR.C `operateforcefields` (game/dos) | 0.467 | 224 | -- | 85, -1, 0 |
|  |  |  | 2 | Duke 1.3D tree/sector.c `operateforcefields` (game/nt) | 0.459 | 224 | -- | 85, -1, 0 |
|  |  |  | 3 | kenbuild-20241213/BUIL0624/GAME.C `analyzesprites` (game/nt) | 0.304 | 224 | -- | 2 |
| 1:19720 `playerkeys` (NEAR_MATCH) | 7568 | 0.844 | 1 | Duke 1.5/SECTOR.C `operateactivators` (game_oe/dos) | 0.518 | 5664 | activatewarpelevators ×4, getanimationgoal ×4, operatesectors ×2, setanimation ×18, fta (+2) | 191, 21 ×2, 26, 18, 22 ×2 (+21) |
|  |  |  | 2 | Duke 1.3D tree/sector.c `operateactivators` (game_oe/nt) | 0.509 | 5408 | activatewarpelevators ×4, getanimationgoal ×4, operatesectors ×2, setanimation ×18, fta (+2) | 21 ×2, 26, 22 ×2, 17, 20 (+19) |
|  |  |  | 3 | Duke 1.5/SECTOR.C `activatebysector` (game_oe/dos) | 0.476 | 4752 | activatewarpelevators ×4, getanimationgoal ×4, operatesectors ×2, setanimation ×18, fta (+1) | 191, 21 ×2, 26, 18, 22 ×2 (+21) |
| 1:1b4b0 `checkhitswitch` (NEAR_MATCH) | 1632 | 0.934 | 1 | Duke 1.5/SECTOR.C `operateactivators` (game/dos) | 0.315 | 880 | operatesectors, fta ×2 | 64 ×3, -1 ×2, 0, 2, 4 ×2 (+1) |
|  |  |  | 2 | Duke 1.3D tree/sector.c `checkhitswitch` (game_oe/nt) | 0.303 | 4976 | xyzsound ×4, operatesectors, fta ×2 | 395 ×2, 85, 23, 25, 11 (+7) |
|  |  |  | 3 | Duke 1.5/SECTOR.C `checkhitswitch` (game_oe/dos) | 0.296 | 5456 | xyzsound ×4, operatesectors, fta ×2 | 395 ×2, 85, 23, 25, 11 (+7) |
| 1:1bb10 `checkhitwall` (NEAR_MATCH) | 1648 | 0.929 | 1 | Duke 1.3D tree/sector.c `checkhitwall` (game/nt) | 0.353 | 2096 | egs ×2, spritesound, updatesector ×2 | 867, 665, 35, 26, 128 (+6) |
|  |  |  | 2 | Duke 1.3D tree/actors.c `movefallers` (game_oe/nt) | 0.328 | 1808 | egs, deletesprite | 26, 128, 3 ×2, -1 ×3, 5 (+3) |
|  |  |  | 3 | Duke 1.5/ACTORS.C `movefallers` (game_oe/nt) | 0.325 | 2064 | egs, deletesprite | 26, 128, 3 ×2, -1 ×4, 5 (+4) |
| 1:1c180 `checkhitceiling` (NEAR_MATCH) | 1120 | 0.923 | 1 | Duke 1.5/SECTOR.C `checkhitceiling` (game/dos) | 0.340 | 736 | -- | 3, 0, 1 ×2 |
|  |  |  | 2 | Duke 1.3D tree/sector.c `checkhitceiling` (game/nt) | 0.334 | 704 | -- | 3, 0, 1 ×2 |
|  |  |  | 3 | Duke 1.5/SECTOR.C `operateactivators` (game/dos) | 0.333 | 880 | -- | 36, 3, 0, 1 ×2 |
| 1:1c5e0 `checksectors` (NEAR_MATCH) | 11184 | 0.822 | 1 | Duke 1.5/SECTOR.C `checksectors` (game_oe/dos) | 0.428 | 22652 | operatemasterswitches, xyzsound ×2, activatewarpelevators ×7, operateactivators ×2, operateforcefields (+7) | 32767, 168 ×6, 23, 21 ×4, 26 ×2 (+23) |
|  |  |  | 2 | Duke 1.3D tree/sector.c `checksectors` (game_oe/nt) | 0.422 | 22165 | operatemasterswitches, xyzsound ×2, activatewarpelevators ×7, operateactivators ×2, operateforcefields (+7) | 32767, 168 ×6, 23, 21 ×4, 26 ×2 (+23) |
|  |  |  | 3 | Duke 1.5/SECTOR.C `activatebysector` (game_oe/dos) | 0.411 | 4752 | activatewarpelevators ×6, getanimationgoal ×4, operatesectors ×3, setanimation ×18, fta ×2 (+1) | 21 ×2, 26, 18, 22 ×4, 17 (+17) |
| 1:1f190 `allignwarpelevators` (STRICT) | 238 | 1.000 | 1 | Duke 1.5/SECTOR.C `allignwarpelevators` (game/dos) | 0.869 **strong** | 368 | -- | 17 ×2, 16 |
|  |  |  | 2 | Duke 1.3D tree/sector.c `allignwarpelevators` (game/nt) | 0.869 **strong** | 352 | -- | 17 ×2, 16 |
|  |  |  | 3 | Duke 1.5/SECTOR.C `activatewarpelevators` (game/dos) | 0.377 | 480 | -- | 17 ×2 |

### game_player

| Oracle function | Oracle B | Current | Rank | Donor | Score | Donor B | Shared calls | Shared immediates |
|---|---:|---:|---:|---|---:|---:|---|---|
| 1:1f2e0 `kickview` (STRICT) | 304 | 1.000 | 1 | kenbuild-20241213/buil0723/GAME.C `analyzesprites` (game/dos) | 0.323 | 336 | getangle | 4 ×2 |
|  |  |  | 2 | kenbuild-20241213/buil1995/GAME.C `analyzesprites` (game/nt) | 0.323 | 336 | getangle | 4 ×2 |
|  |  |  | 3 | Duke 1.3D tree/menues.c `getangplayers` (game/nt) | 0.313 | 272 | getangle | 512 |
| 1:1f410 `spawn_pistol_tracer` (NEAR_MATCH) | 592 | 0.656 | 1 | Duke 1.3D tree/gamedef.c `furthestangle` (game/dos) | 0.300 | 272 | hitscan | 512, 2047, 32 ×2, 0, 4 |
|  |  |  | 2 | Duke 1.5/GAMEDEF.C `furthestangle` (game/nt) | 0.300 | 272 | hitscan | 512, 2047, 32 ×2, 0, 4 |
|  |  |  | 3 | Duke 1.3D tree/actors.c `bounce` (game/nt) | 0.297 | 496 | ksqrt | 512, 2047 ×3, 10, 12, 5 (+2) |
| 1:1f660 `shoot` (NEAR_MATCH) | 1728 | 0.903 | 1 | Duke 1.5/SOUNDS.C `spritesound` (game_oe/nt) | 0.337 | 1776 | -- | 2048, 2047, 1024, 44 ×9, 6 ×2 (+7) |
|  |  |  | 2 | kenbuild-20241213/BUIL0719/GAME.C `shootgun` (game/nt) | 0.334 | 2016 | hitscan | 2560, 38 ×2, 100 ×2, 8192, 2048 ×2 (+13) |
|  |  |  | 3 | kenbuild-20241213/BUIL0624/GAME.C `shootgun` (game/nt) | 0.330 | 1696 | hitscan | 2560, 38 ×2, 100 ×2, 8192, 2048 ×2 (+13) |
| 1:1fd20 `displayraise` (NEAR_MATCH) | 288 | 0.851 | 1 | Duke 1.3D tree/player.c `animateknuckles` (game/nt) | 0.400 | 320 | -- | 180, 160, 9, 31, 3 (+4) |
|  |  |  | 2 | Duke 1.5/PLAYER.C `animateknuckles` (game/nt) | 0.395 | 352 | -- | 180, 160, 9, 31, 3 (+4) |
|  |  |  | 3 | Duke 1.3D tree/player.c `animatetip` (game/nt) | 0.370 | 320 | -- | 9, 31, 3, 0, 4 ×3 (+1) |
| 1:1fe40 `displayweapon` (NEAR_MATCH) | 4128 | 0.800 | 1 | Duke 1.5/GAME.C `cheats` (game/nt) | 0.295 | 4640 | -- | 191, 25, 120, 100, 11 (+12) |
|  |  |  | 2 | Duke 1.3D tree/sector.c `activatebysector` (game_oe/nt) | 0.290 | 4496 | -- | 20, 127 ×2, 11, 9 ×3, 15 (+9) |
|  |  |  | 3 | Duke 1.3D tree/sector.c `operatesectors` (game/nt) | 0.290 | 3984 | -- | 20, 127 ×2, 11, 9 ×3, 15 (+8) |
| 1:20e60 `getinput` (NEAR_MATCH) | 1056 | 1.000 | 1 | kenbuild-20241213/2DRW0719/GAME.C `getinput` (game/nt) | 0.355 | 640 | getmousevalues | -128 ×2, 14, 127 ×4, 11, 9 (+10) |
|  |  |  | 2 | kenbuild-20241213/2DRW0628/GAME.C `getinput` (game_oe/nt) | 0.318 | 928 | getmousevalues | -128 ×2, 13, 14, 127 ×2, 11 (+11) |
|  |  |  | 3 | kenbuild-20260331/FILES_FROM_MATT/2draw/GAME.C `getinput` (game/nt) | 0.295 | 704 | getmousevalues | -128 ×2, 13, 14, 127 ×2, 11 (+11) |
| 1:21280 `update_air` (STRICT) | 400 | 1.000 | 1 | Duke 1.5/ACTORS.C `checkavailweapon` (game_oe/nt) | 0.309 | 512 | spritesound | 10, 0 ×3, 4, 1 |
|  |  |  | 2 | Duke 1.3D tree/premap.c `waitforeverybody` (game/nt) | 0.295 | 432 | -- | 0 ×3, 2, 4 ×4, 1 |
|  |  |  | 3 | Duke 1.5/SECTOR.C `callsound` (game/dos) | 0.288 | 464 | spritesound | 0 ×3, 1 |
| 1:21410 `processinput` (NEAR_MATCH) | 15692 | 0.910 | 1 | Duke 1.5/PLAYER.C `shoot` (game/nt) | 0.403 | 9728 | checkhitswitch ×2, checkhitceiling ×2, checkhitwall ×2, findplayer ×5, spawn (+5) | -127, -2048, 348, -16 ×3, 34 (+38) |
|  |  |  | 2 | Duke 1.5/PLAYER.C `processinput` (game/nt) | 0.403 | 14096 | addweapon, stopsound, spawn, egs, spritesound ×27 (+7) | 2816, 4608, 239, 348 ×2, 3840 (+43) |
|  |  |  | 3 | Duke 1.3D tree/player.c `processinput` (game/nt) | 0.401 | 13840 | addweapon, stopsound, spawn, egs, spritesound ×25 (+7) | 2816, 4608, 239, 348 ×2, 3840 (+43) |

### game_game

| Oracle function | Oracle B | Current | Rank | Donor | Score | Donor B | Shared calls | Shared immediates |
|---|---:|---:|---:|---|---:|---:|---|---|
| 1:00020 `inittimer` (STRICT) | 96 | 1.000 | 1 | kenbuild-20241213/buil0723/GAME.C `inittimer` (game/dos) | 1.000 **strong** | 96 | _dos_getvect_, _dos_setvect_, outp ×3 | 214, 54, 38, 67, 64 ×2 (+1) |
|  |  |  | 2 | kenbuild-20241213/2DRW0628/GAME.C `inittimer` (game/nt) | 1.000 **strong** | 96 | _dos_getvect_, _dos_setvect_, outp ×3 | 214, 54, 38, 67, 64 ×2 (+1) |
|  |  |  | 3 | kenbuild-20241213/BUI1101B/GAME.C `inittimer` (game/nt) | 1.000 **strong** | 96 | _dos_getvect_, _dos_setvect_, outp ×3 | 214, 54, 38, 67, 64 ×2 (+1) |
| 1:00080 `uninittimer` (STRICT) | 80 | 1.000 | 1 | kenbuild-20241213/buil0723/GAME.C `uninittimer` (game/dos) | 1.000 **strong** | 80 | _dos_setvect_, outp ×3 | 54, 67, 64 ×2, 255 ×2, 8 |
|  |  |  | 2 | kenbuild-20241213/2DRW0628/GAME.C `uninittimer` (game/nt) | 1.000 **strong** | 80 | _dos_setvect_, outp ×3 | 54, 67, 64 ×2, 255 ×2, 8 |
|  |  |  | 3 | kenbuild-20241213/BUI1101B/GAME.C `uninittimer` (game/nt) | 1.000 **strong** | 80 | _dos_setvect_, outp ×3 | 54, 67, 64 ×2, 255 ×2, 8 |
| 1:000d0 `update_timer_input` (STRICT) | 448 | 1.000 | 1 | buildsrc-20190520/19950920/BUILD.C `keytimerstuff` (engine/nt) | 0.336 | 493 | -- | -128 ×6, 127 ×6, 16 ×2, 8 ×4, 0 |
|  |  |  | 2 | buildsrc-20190520/19960606/BUILD.C `keytimerstuff` (engine/nt) | 0.336 | 493 | -- | -128 ×6, 127 ×6, 16 ×2, 8 ×4, 0 |
|  |  |  | 3 | buildsrc-20190520/SRC_9505/BUILD.C `keytimerstuff` (engine/nt) | 0.336 | 493 | -- | -128 ×6, 127 ×6, 16 ×2, 8 ×4, 0 |
| 1:00290 `level_load_hook` (STRICT) | 16 | 1.000 | 1 | Duke 1.5/GAME.C `copyprotect` (game/nt) | 1.000 | 16 | -- | -- |
|  |  |  | 2 | Duke 1.5/GAME.C `showtwoscreens` (game/nt) | 1.000 | 16 | -- | -- |
|  |  |  | 3 | Duke 1.5/MENUES.C `dummyfunc` (game/nt) | 1.000 | 16 | -- | -- |
| 1:002a0 `timerhandler` (STRICT) | 2496 | 0.940 | 1 | kenbuild-20241213/buil1995/GAME.C `timerhandler` (game_oe/nt) | 0.277 | 1136 | outp | -128 ×16, 127 ×16, 32, 16 ×5, 8 ×10 (+2) |
|  |  |  | 2 | kenbuild-20241213/buil0723/GAME.C `getinput` (game/dos) | 0.246 | 3072 | -- | -128 ×22, 127 ×20, 10 ×2, 255 ×2, 5 (+3) |
|  |  |  | 3 | kenbuild-20241213/BUI1101B/GAME.C `getinput` (game/nt) | 0.245 | 3072 | -- | -128 ×22, 127 ×20, 10 ×2, 255 ×2, 5 (+3) |
| 1:00c60 `gameexit` (STRICT) | 176 | 1.000 | 1 | kenbuild-20241213/buil0723/GAME.C `uninittimer` (game/dos) | 0.518 | 80 | _dos_setvect_, outp ×3 | 54, 67, 64 ×2, 255 ×2, 8 |
|  |  |  | 2 | kenbuild-20241213/2DRW0628/GAME.C `uninittimer` (game/nt) | 0.518 | 80 | _dos_setvect_, outp ×3 | 54, 67, 64 ×2, 255 ×2, 8 |
|  |  |  | 3 | kenbuild-20241213/BUI1101B/GAME.C `uninittimer` (game/nt) | 0.518 | 80 | _dos_setvect_, outp ×3 | 54, 67, 64 ×2, 255 ×2, 8 |
| 1:00d10 `domovethings_multiplayer` (NEAR_MATCH) | 1040 | 0.983 | 1 | kenbuild-20241213/buildold_12_30_94/GAME.C `getpackets` (game/dos) | 0.583 | 672 | getpacket | 7, 255, 3 ×5, -1, 16 ×2 (+4) |
|  |  |  | 2 | kenbuild-20241213/BUIL0719/GAME.C `getpackets` (game/nt) | 0.521 | 1120 | getpacket | 7, 255, 3 ×5, -1, 5 (+5) |
|  |  |  | 3 | kenbuild-20241213/BUIL0801/GAME.C `getpackets` (game/nt) | 0.521 | 1120 | getpacket | 7, 255, 3 ×5, -1, 5 (+5) |
| 1:01120 `moveloop` (NEAR_MATCH) | 1952 | 0.958 | 1 | kenbuild-20241213/buildold_12_30_94/GAME.C `sync` (game_oe/dos) | 0.538 | 2512 | getpacket ×2, sendpacket ×2 | 7 ×6, 255 ×4, 3 ×10, 16 ×7, 8 ×11 (+3) |
|  |  |  | 2 | kenbuild-20241213/BUIL0719/GAME.C `getpackets` (game_oe/nt) | 0.469 | 1488 | getpacket, sendpacket ×2 | 7 ×4, 255, 3 ×5, 5 ×2, 16 ×2 (+4) |
|  |  |  | 3 | kenbuild-20241213/BUIL0801/GAME.C `getpackets` (game_oe/nt) | 0.469 | 1488 | getpacket, sendpacket ×2 | 7 ×4, 255, 3 ×5, 5 ×2, 16 ×2 (+4) |
| 1:018c0 `network_move_loop` (NEAR_MATCH) | 2832 | 0.972 | 1 | kenbuild-20241213/buildold_12_30_94/GAME.C `sync` (game_oe/dos) | 0.633 | 2512 | getinput ×2, doanimations, processinput, getpacket ×2, sendpacket ×2 | 7 ×6, 255 ×7, 12 ×2, 3 ×16, -1 (+5) |
|  |  |  | 2 | kenbuild-20241213/BUIL0719/GAME.C `sync` (game_oe/nt) | 0.560 | 3392 | getinput ×2, doanimations, processinput, getpacket ×2, sendpacket ×2 | 7 ×6, 255 ×6, 12 ×2, 3 ×16, -1 (+6) |
|  |  |  | 3 | kenbuild-20241213/BUIL0801/GAME.C `sync` (game_oe/nt) | 0.560 | 3392 | getinput ×2, doanimations, processinput, getpacket ×2, sendpacket ×2 | 7 ×6, 255 ×6, 12 ×2, 3 ×16, -1 (+6) |
| 1:023d0 `next_text_shade` (STRICT) | 48 | 1.000 | 1 | kenbuild-20241213/buil062896/ENGINE.C `plotpixel` (engine/nt) | 0.191 | 34 | -- | -- |
|  |  |  | 2 | kenbuild-20260331/build_1996_11_12_partial/ENGINE.C `plotpixel` (engine/nt) | 0.191 | 34 | -- | -- |
|  |  |  | 3 | kenbuild-20260331/build_1997_07_19/ENGINE.C `plotpixel` (engine/nt) | 0.191 | 34 | -- | -- |
| 1:02400 `displayquotes` (NEAR_MATCH) | 560 | 0.968 | 1 | Duke 1.3D tree/menues.c `menutextc` (game/nt) | 0.271 | 544 | -- | 122 ×2, 97 ×2, 32 ×2, 5, 8 (+2) |
|  |  |  | 2 | kenbuild-20241213/BUIL0624/GAME.C `drawscreen` (game_oe/nt) | 0.270 | 656 | overwritesprite | 8 ×2, 0, 2 ×5, 4 ×4 |
|  |  |  | 3 | Duke 1.5/MENUES.C `menutextc` (game/nt) | 0.268 | 560 | -- | 122 ×2, 97 ×2, 32 ×2, 5, 8 (+2) |
| 1:02630 `FTA` (STRICT) | 144 | 1.000 | 1 | kenbuild-20241213/BUIL1020/GAME.C `stopinterpolation` (game/nt) | 0.376 | 112 | -- | 4 ×2 |
|  |  |  | 2 | Duke 1.5/ACTORS.C `stopinterpolation` (game/nt) | 0.373 | 128 | -- | 4 ×2 |
|  |  |  | 3 | Duke 1.3D tree/actors.c `stopinterpolation` (game/nt) | 0.373 | 128 | -- | 4 ×2 |
| 1:026c0 `draw_weapon_tile` (STRICT) | 144 | 1.000 | 1 | Duke 1.5/GAME.C `myos` (game/nt) | 0.334 | 176 | -- | 255, 4 ×4 |
|  |  |  | 2 | Duke 1.3D tree/game.c `myos` (game/nt) | 0.305 | 160 | -- | 255, 4 ×4 |
|  |  |  | 3 | Duke 1.5/GAME.C `myospal` (game/nt) | 0.287 | 112 | -- | 255, 4 ×3 |
| 1:02750 `gametext` (NEAR_MATCH) | 256 | 0.982 | 1 | Duke 1.3D tree/game.c `gametext` (game/nt) | 0.301 | 272 | -- | 57, 26, 48, 32 ×2, 5 (+2) |
|  |  |  | 2 | Duke 1.5/GAME.C `gametext` (game/nt) | 0.292 | 272 | -- | 57, 48, 32 ×2, 5, 16 ×2 (+2) |
|  |  |  | 3 | Duke 1.3D tree/game.c `gamenumber` (game_oe/nt) | 0.290 | 304 | -- | 57, 26, 48, 10, 32 ×2 (+3) |
| 1:02850 `minitext` (STRICT) | 224 | 1.000 | 1 | Duke 1.5/GAME.C `minitextshade` (game/nt) | 0.296 | 144 | -- | 32, 8, 0 ×3, 4 |
|  |  |  | 2 | Duke 1.5/GAME.C `minitext` (game/nt) | 0.287 | 144 | -- | 32, 0 ×4, 4 |
|  |  |  | 3 | Duke 1.3D tree/game.c `minitext` (game/nt) | 0.287 | 144 | -- | 32, 0 ×4, 4 |
| 1:02930 `numtext_relative` (NEAR_MATCH) | 272 | 1.000 | 1 | Duke 1.3D tree/game.c `gamenumber` (game_oe/nt) | 0.375 | 304 | ltoa | 57, 26, 48, 10, 32 ×2 (+3) |
|  |  |  | 2 | Duke 1.5/GAME.C `gamenumber` (game_oe/nt) | 0.372 | 304 | ltoa | 57, 26, 48, 10, 32 ×2 (+3) |
|  |  |  | 3 | Duke 1.5/GAME.C `digitalnumber` (game/nt) | 0.320 | 256 | ltoa | 10, 32 ×2, 16 ×2, 0 ×2, 4 |
| 1:02a40 `strget` (NEAR_MATCH) | 1024 | 0.883 | 1 | kenbuild-20241213/BUIL0801/GAME.C `drawscreen` (game_oe/nt) | 0.286 | 1392 | overwritesprite ×2 | 40 ×2, 10, 3 ×2, 5, 8 (+4) |
|  |  |  | 2 | Duke 1.5/GAME.C `strget` (game_oe/nt) | 0.285 | 976 | -- | 57, 26, 127, 48, 10 (+8) |
|  |  |  | 3 | kenbuild-20241213/BUIL0719/GAME.C `drawscreen` (game_oe/nt) | 0.282 | 1376 | overwritesprite ×2 | 40 ×2, 10, 3 ×2, 8, 0 ×3 (+3) |
| 1:02e40 `findlocator` (STRICT) | 80 | 1.000 | 1 | Duke 1.5/GAME.C `LocateTheLocator` (game/nt) | 0.906 **strong** | 80 | -- | -1 ×2 |
|  |  |  | 2 | Duke 1.3D tree/game.c `LocateTheLocator` (game/nt) | 0.906 **strong** | 80 | -- | -1 ×2 |
|  |  |  | 3 | Duke 1.5/ACTORS.C `ifhitsectors` (game/nt) | 0.547 | 80 | -- | -1 |
| 1:02e90 `EGS` (STRICT) | 288 | 1.000 | 1 | Duke 1.3D tree/game.c `EGS` (game/nt) | 0.446 | 544 | insertsprite | -- |
|  |  |  | 2 | kenbuild-20241213/buildold_12_30_94/GAME.C `initplayersprites` (game/dos) | 0.365 | 224 | insertsprite | -- |
|  |  |  | 3 | kenbuild-20241213/BUIL0624/GAME.C `initplayersprites` (game/nt) | 0.365 | 223 | insertsprite | -- |
| 1:02fb0 `cacheit` (STRICT) | 1968 | 1.000 | 1 | Duke 1.3D tree/premap.c `cacheit` (game_oe/nt) | 0.474 | 2384 | loadtile ×36 | 573, 925, 1420, 1820, 1680 (+9) |
|  |  |  | 2 | Duke 1.3D tree/premap.c `cachespritenum` (game/nt) | 0.365 | 1424 | loadtile ×12 | 573, 925, 1420, 1820, 1680 (+9) |
|  |  |  | 3 | Duke 1.5/PREMAP.C `cacheit` (game_oe/nt) | 0.341 | 2592 | -- | 573, 925, 1420, 1820, 1680 (+10) |
| 1:03760 `loadlookup` (STRICT) | 160 | 1.000 | 1 | kenbuild-20260331/build_1997_07_19/SPHERE.C `loadtables` (engine/nt) | 0.559 | 135 | read ×3, close, open | 514, 256, 12, -1, 1 |
|  |  |  | 2 | kenbuild-20260331/FILES_FROM_MATT/2draw/2DRAW.C `loadtables` (engine/nt) | 0.526 | 157 | read ×3, close, open | 514, 256, 12, -1 |
|  |  |  | 3 | kenbuild-20241213/buil112093/BUILD.C `showpositioncapture` (engine/nt) | 0.495 | 198 | read ×3, close, open | 514, 256, 12, -1 |
| 1:03800 `numtext` (STRICT) | 240 | 1.000 | 1 | Duke 1.3D tree/game.c `gamenumber` (game_oe/nt) | 0.360 | 304 | ltoa | 10, 32, 8, 0 ×3 |
|  |  |  | 2 | Duke 1.5/GAME.C `gamenumber` (game_oe/nt) | 0.357 | 304 | ltoa | 10, 32, 8, 0 ×3 |
|  |  |  | 3 | Duke 1.5/GAME.C `digitalnumber` (game/nt) | 0.349 | 256 | ltoa | 10, 32, 0 ×3, 4 |
| 1:038f0 `numtext_fixed` (STRICT) | 128 | 1.000 | 1 | Duke 1.5/GAME.C `digitalnumber` (game/nt) | 0.366 | 256 | ltoa | 10, 0 ×2 |
|  |  |  | 2 | Duke 1.3D tree/game.c `digitalnumber` (game/nt) | 0.366 | 256 | ltoa | 10, 0 ×2 |
|  |  |  | 3 | Duke 1.3D tree/game.c `gamenumber` (game/nt) | 0.339 | 48 | ltoa | 10, 255 |
| 1:03970 `draw_meter` (STRICT) | 176 | 1.000 | 1 | Duke 1.5/GAME.C `minitextshade` (game/nt) | 0.289 | 144 | -- | 5, 0 ×3 |
|  |  |  | 2 | Duke 1.5/GAME.C `minitext` (game/nt) | 0.289 | 144 | -- | 5, 0 ×4 |
|  |  |  | 3 | Duke 1.3D tree/game.c `minitext` (game/nt) | 0.289 | 144 | -- | 5, 0 ×4 |
| 1:03a20 `animatesprites` (STRICT) | 1328 | 1.000 | 1 | Duke 1.5/ACTORS.C `ssp` (game_oe/nt) | 0.256 | 1312 | -- | 192, 2047 ×2, 7 ×2, 3, 5 (+5) |
|  |  |  | 2 | Duke 1.3D tree/actors.c `ssp` (game_oe/nt) | 0.253 | 1248 | -- | 192, 2047 ×2, 7, 3, 5 (+5) |
|  |  |  | 3 | kenbuild-20241213/BUIL0719/GAME.C `drawscreen` (game_oe/nt) | 0.249 | 1376 | -- | 3200, 251, 2047 ×2, 31 ×3, 7 (+6) |
| 1:03f50 `draw_water_distortion` (NEAR_MATCH) | 256 | 1.000 | 1 | kenbuild-20260331/build_1997_07_19/LAVA.C `drawpic` (engine/nt) | 0.344 | 93 | -- | 320, 200 |
|  |  |  | 2 | kenbuild-20241213/BUIL0719/GAME.C `analyzesprites` (game/nt) | 0.318 | 240 | -- | 8 ×2 |
|  |  |  | 3 | kenbuild-20241213/BUIL0801/GAME.C `analyzesprites` (game/nt) | 0.318 | 240 | -- | 8 ×2 |
| 1:04050 `displayrooms` (NEAR_MATCH) | 3200 | 0.943 | 1 | Duke 1.3D tree/game.c `displayrooms` (game/nt) | 0.286 | 2752 | animatecamsprite, drawmasks, drawrooms ×2 | 199 ×2, 320, 200, 2047 ×3, 31 (+9) |
|  |  |  | 2 | Duke 1.5/GAME.C `displayrooms` (game/nt) | 0.278 | 2800 | animatecamsprite, drawmasks, drawrooms ×2 | 199 ×2, 320, 200, 2047 ×3, 255 ×2 (+9) |
|  |  |  | 3 | kenbuild-20241213/BUIL0719/GAME.C `processinput` (game/nt) | 0.271 | 3520 | -- | 38, 320 ×3, 63, 200, 2047 ×4 (+16) |
| 1:04cd0 `nonsharedkeys` (NEAR_MATCH) | 2080 | 0.993 | 1 | Duke 1.5/GAME.C `displayrest` (game/nt) | 0.329 | 2048 | vscrn, fx_stopallsounds, setbrightness | 50, 90 ×2, 199, 26 ×2, 100 (+11) |
|  |  |  | 2 | kenbuild-20241213/BUIL0719/GAME.C `getinput` (game/nt) | 0.325 | 2144 | qsetmode640350, screencapture | 10, 15, 7, 3 ×2, -1 ×2 (+7) |
|  |  |  | 3 | kenbuild-20241213/BUIL0801/GAME.C `getinput` (game/nt) | 0.317 | 2144 | screencapture | 10, 15, 7, 3 ×2, -1 ×2 (+7) |
| 1:054f0 `show_help` (STRICT) | 128 | 1.000 | 1 | Duke 1.3D tree/game.c `comlinehelp` (game/nt) | 0.722 **strong** | 432 | puts ×10, printf | 8 |
|  |  |  | 2 | Duke 1.5/GAME.C `comlinehelp` (game/nt) | 0.582 | 560 | puts ×10, printf | 8 |
|  |  |  | 3 | Duke 1.5/RTS.C `RTS_SoundLength` (game/nt) | 0.346 | 64 | -- | 8 |
| 1:05570 `checkcommandline` (NEAR_MATCH) | 592 | 0.979 | 1 | Duke 1.3D tree/game.c `checkcommandline` (game/nt) | 0.311 | 1072 | puts, atol ×3, exit ×2, printf ×4 | 47, 83, 49, 115, 63 (+5) |
|  |  |  | 2 | Duke 1.3D tree/menues.c `menutextc` (game/nt) | 0.297 | 544 | -- | 63, 8, 0, 4, 1 |
|  |  |  | 3 | Duke 1.5/MENUES.C `menutextc` (game/nt) | 0.287 | 560 | -- | 63, 8, 0, 4, 1 |
| 1:057c0 `wait_for_players` (NEAR_MATCH) | 928 | 0.945 | 1 | kenbuild-20241213/buildold_12_30_94/GAME.C `getpackets` (game/dos) | 0.597 | 672 | getpacket | 7, 255, 3 ×6, 16 ×2, 8 ×4 (+3) |
|  |  |  | 2 | kenbuild-20241213/BUIL0719/GAME.C `getpackets` (game/nt) | 0.512 | 1120 | getpacket | 7, 255, 3 ×5, 5, 16 ×2 (+4) |
|  |  |  | 3 | kenbuild-20241213/BUIL0801/GAME.C `getpackets` (game/nt) | 0.512 | 1120 | getpacket | 7, 255, 3 ×5, 5, 16 ×2 (+4) |
| 1:05b60 `main` (NEAR_MATCH) | 7472 | 0.966 | 1 | Duke 1.5/GAME.C `main` (game_oe/nt) | 0.377 | 9472 | checkcommandline, nonsharedkeys, animatecamsprite, animatesprites ×3, enterlevel ×2 (+10) | 122, 57 ×2, 191, 199 ×2, 119 (+29) |
|  |  |  | 2 | kenbuild-20241213/BUIL0801/GAME.C `main` (game_oe/nt) | 0.375 | 7056 | overwritesprite ×6, draw2dscreen ×3, getinput ×3, doanimations, drawmasks ×3 (+6) | 57, 360, 160 ×8, 120 ×2, 40 ×2 (+14) |
|  |  |  | 3 | Duke 1.3D tree/game.c `main` (game_oe/nt) | 0.366 | 7920 | checkcommandline, nonsharedkeys, animatecamsprite, animatesprites ×3, enterlevel ×2 (+10) | 122, 57 ×2, 191, 199 ×2, 119 (+27) |
| 1:07890 `displaystatus` (STRICT) | 2352 | 1.000 | 1 | Duke 1.5/GAME.C `displayrest` (game_oe/nt) | 0.245 | 3552 | -- | 90, 160 ×4, 192, 14, 24 (+7) |
|  |  |  | 2 | Duke 1.5/GAME.C `Logo` (game_oe/nt) | 0.243 | 1808 | -- | 90, 160, 120 ×2, 14, 10 ×6 (+6) |
|  |  |  | 3 | kenbuild-20241213/BUILDBAK/GAME.C `main` (game/nt) | 0.234 | 2448 | -- | 120, 127, 24 ×2, 15, 64 (+7) |
| 1:081c0 `playback_demo` (NEAR_MATCH) | 3184 | 0.993 | 1 | Duke 1.5/GAME.C `displayrest` (game_oe/nt) | 0.309 | 3552 | displayweapon, menus | 90, 199, 160 ×4, 20, 192 (+14) |
|  |  |  | 2 | kenbuild-20241213/buil1995/GAME.C `main` (game_oe/nt) | 0.287 | 3536 | doanimations, processinput, nextpage | 18, 38, 28, 160, 120 (+15) |
|  |  |  | 3 | Duke 1.5/GAME.C `domovethings` (game_oe/nt) | 0.287 | 3264 | animatewalls, checksectors, moveactors, moveeffectors, moveplayers (+6) | 18, 10 ×3, 15, 128, 6 (+8) |
| 1:08e30 `record` (STRICT) | 112 | 1.000 | 1 | kenbuild-20260331/build_1997_07_19/CACHE1D.C `initcache` (engine/nt) | 0.382 | 93 | -- | -- |
|  |  |  | 2 | kenbuild-20241213/buil062896/ENGINE.C `setaspect` (engine/nt) | 0.362 | 101 | -- | -- |
|  |  |  | 3 | kenbuild-20241213/BUILDBAK/ENGINE.C `setaspect` (engine/nt) | 0.362 | 101 | -- | -- |
| 1:08ea0 `domovethings` (STRICT) | 1968 | 1.000 | 1 | Duke 1.3D tree/game.c `domovethings` (game_oe/nt) | 0.366 | 1600 | checksectors, moveactors, moveeffectors, moveplayers, movestandables (+4) | 10 ×2, -1, 5, 0 ×4, 2 ×3 (+2) |
|  |  |  | 2 | Duke 1.5/GAME.C `domovethings` (game/nt) | 0.358 | 1808 | checksectors, moveactors, moveeffectors, moveplayers, movestandables (+4) | 10, -1, 5, 0 ×5, 2 ×3 (+2) |
|  |  |  | 3 | Duke 1.3D tree/actors.c `movetransports` (game/nt) | 0.357 | 3456 | spawn ×6, spritesound ×2, krand | 1536 ×2, 127, 4096, 10, -1 (+5) |
| 1:09650 `dobonus` (NEAR_MATCH) | 3536 | 0.986 | 1 | Duke 1.5/GAME.C `typemode` (game_oe/nt) | 0.311 | 2992 | -- | 65, 57 ×2, 50, 97, 26 ×2 (+15) |
|  |  |  | 2 | Duke 1.5/GAME.C `displayrest` (game_oe/nt) | 0.294 | 3552 | -- | 50, 90 ×2, 26 ×2, 160, 100 (+11) |
|  |  |  | 3 | Duke 1.5/GAME.C `domovethings` (game_oe/nt) | 0.290 | 3264 | -- | 122 ×4, 26 ×2, 100 ×4, 10 ×4, 128 (+9) |
| 1:0a420 `draw_monitor_overlay` (NEAR_MATCH) | 720 | 0.995 | 1 | kenbuild-20241213/BUIL0624/GAME.C `drawscreen` (game_oe/nt) | 0.240 | 656 | overwritesprite | 160 ×4, 7, 3, 2 ×5, 4 ×5 |
|  |  |  | 2 | Duke 1.5/GAME.C `displayfragbar` (game_oe/nt) | 0.227 | 864 | -- | 3 ×2, 16 ×2, 4 ×5 |
|  |  |  | 3 | Duke 1.5/GAME.C `cameratext` (game/nt) | 0.225 | 496 | -- | 16, 2 ×5, 4 ×3 |
| 1:0a6f0 `update_menu_cursor` (NEAR_MATCH) | 448 | 0.873 | 1 | kenbuild-20241213/2DRW0719/GAME.C `getinput` (game/nt) | 0.287 | 640 | getmousevalues | 9, 10, 128, 7, 255 ×2 (+5) |
|  |  |  | 2 | Duke 1.5/ACTORS.C `movecyclers` (game/nt) | 0.280 | 288 | -- | 10, 7, 4, 1 |
|  |  |  | 3 | Duke 1.3D tree/actors.c `movecyclers` (game/nt) | 0.280 | 288 | -- | 10, 7, 4, 1 |
| 1:0a8b0 `displayhighscores` (NEAR_MATCH) | 1024 | 0.985 | 1 | Duke 1.5/GAME.C `typemode` (game/nt) | 0.302 | 1280 | minitext ×2 | 50, 97, 30, 7, 5 (+5) |
|  |  |  | 2 | Duke 1.5/GAME.C `strget` (game_oe/nt) | 0.292 | 976 | -- | 11, 10, 7, 32 ×4, 5 (+4) |
|  |  |  | 3 | kenbuild-20241213/2DRW0719/GAME.C `playback` (game_oe/nt) | 0.287 | 973 | -- | 50, 32 ×3, 5, 8 ×2, 0 ×3 (+2) |
| 1:0acb0 `spawn_random_effects` (NEAR_MATCH) | 480 | 0.829 | 1 | Duke 1.3D tree/game.c `vglass` (game_oe/nt) | 0.445 | 704 | insertsprite, krand ×3 | 224, 32 ×2, 3, 5, 1 |
|  |  |  | 2 | Duke 1.3D tree/game.c `spriteglass` (game_oe/nt) | 0.438 | 752 | insertsprite, krand ×3 | 2047, 32, 3, 5, 1 |
|  |  |  | 3 | Duke 1.3D tree/game.c `ceilingglass` (game_oe/nt) | 0.429 | 832 | insertsprite, krand ×3 | 224, 3, 5, 1 |
| 1:0ae90 `move_status9` (NEAR_MATCH) | 1680 | 1.000 | 1 | Duke 1.5/SECTOR.C `doanimations` (game_oe/dos) | 0.357 | 1200 | spritesound ×2 | 20 ×2, 3, 5, 0 ×2, 2 (+2) |
|  |  |  | 2 | Duke 1.3D tree/sector.c `doanimations` (game_oe/nt) | 0.355 | 1184 | spritesound ×2 | 20 ×2, 3, 5, 0 ×2, 2 (+2) |
|  |  |  | 3 | Duke 1.3D tree/actors.c `movetransports` (game/nt) | 0.347 | 3456 | spawn ×6, spritesound ×2 | 1536 ×2, 127, 4096, 10, 5 (+4) |
| 1:0b520 `read_userdefs` (STRICT) | 176 | 1.000 | 1 | kenbuild-20241213/buil112093/BUILD.C `loadtables` (engine/nt) | 0.551 | 120 | read ×3, close, open | 514, 256, 12, -1, 1 |
|  |  |  | 2 | kenbuild-20241213/buil112493/BUILD.C `loadtables` (engine/nt) | 0.551 | 120 | read ×3, close, open | 514, 256, 12, -1, 1 |
|  |  |  | 3 | kenbuild-20241213/buil120693/BUILD.C `loadtables` (engine/nt) | 0.551 | 120 | read ×3, close, open | 514, 256, 12, -1, 1 |
| 1:0b5d0 `write_userdefs` (STRICT) | 96 | 1.000 | 1 | kenbuild-20260331/build_1997_07_19/SETUP.C `savesetup` (engine/nt) | 0.580 | 102 | write ×2, close, open | 12 |
|  |  |  | 2 | kenbuild-20241213/builold!/BUILD.C `loadtables` (engine/nt) | 0.552 | 81 | close, open | 12 |
|  |  |  | 3 | kenbuild-20260331/buildold/VOXEL1.C `loadtables` (engine/nt) | 0.552 | 81 | close, open | 12 |
| 1:0b630 `saveplayer` (STRICT) | 208 | 1.000 | 1 | kenbuild-20241213/BUIL0422/ENGINE.C `loadtables` (engine/dos) | 0.482 | 153 | close, open | 4096 ×2, 12 |
|  |  |  | 2 | buildsrc-20190520/SRC_9505/ENGINE.C `loadtables` (engine/dos) | 0.464 | 204 | close, open | 4096 ×2, 12 |
|  |  |  | 3 | kenbuild-20260331/build_1997_07_19/CONVMAP7.C `savenewboard` (engine/nt) | 0.460 | 270 | write ×5, close, open | 12 |
| 1:0b700 `loadplayer` (STRICT) | 272 | 1.000 | 1 | buildsrc-20190520/SRC_9505/ENGINE.C `loadtables` (engine/dos) | 0.569 | 204 | read ×5, close, open | 514, 4096 ×2, 256, 12, -1 |
|  |  |  | 2 | kenbuild-20241213/BUIL0422/ENGINE.C `loadtables` (engine/dos) | 0.568 | 153 | read ×5, close, open | 514, 4096 ×2, 256, 12, -1 |
|  |  |  | 3 | buildsrc-20190520/19950920/POLYTEX5.C `loadtables` (engine/dos) | 0.521 | 221 | read ×5, close, open | 514, 4096, 256, 12, -1 ×2 |
| 1:0b810 `read_demo` (STRICT) | 272 | 1.000 | 1 | kenbuild-20260331/build_1997_07_19/CONVMAP7.C `loadoldboard` (engine/nt) | 0.631 | 297 | read ×9, close, open | 514, 256, 12, -1, 4 |
|  |  |  | 2 | kenbuild-20260331/build_1997_07_19/FIXSPRBT.C `loadoldboard` (engine/nt) | 0.631 | 297 | read ×9, close, open | 514, 256, 12, -1, 4 |
|  |  |  | 3 | kenbuild-20260331/FILES_FROM_MATT/BUILD/FIXSPRBT.C `loadoldboard` (engine/nt) | 0.631 | 297 | read ×9, close, open | 514, 256, 12, -1, 4 |
| 1:0b920 `write_demo` (NEAR_MATCH) | 272 | 0.849 | 1 | kenbuild-20260331/build_1997_07_19/CONVMAP7.C `savenewboard` (engine/nt) | 0.621 | 270 | write ×9, close, open | 12, -1, 4 |
|  |  |  | 2 | kenbuild-20260331/build_1997_07_19/CONVMAP6.C `savenewboard` (engine/nt) | 0.615 | 272 | write ×9, close, open | 12, -1, 4 |
|  |  |  | 3 | kenbuild-20260331/build_1997_07_19/FIXSPRBT.C `savenewboard` (engine/nt) | 0.615 | 272 | write ×9, close, open | 12, -1, 4 |
| 1:0ba30 `moveeffectors` (NEAR_MATCH) | 14128 | 0.943 | 1 | Duke 1.3D tree/actors.c `moveeffectors` (game/nt) | 0.461 | 18625 | activatewarpelevators, ifhitbyweapon, getanimationgoal ×2, findplayer ×2, stopsound (+10) | -511, -20, 174 ×2, 35, 27 (+46) |
|  |  |  | 2 | Duke 1.5/ACTORS.C `moveeffectors` (game/nt) | 0.436 | 22549 | activatewarpelevators, ifhitbyweapon, getanimationgoal ×2, findplayer ×2, stopsound (+10) | -511, -20, 174 ×2, 35, 27 (+47) |
|  |  |  | 3 | Duke 1.5/ACTORS.C `movestandables` (game_oe/nt) | 0.434 | 14640 | findplayer ×2, operatesectors, egs, spritesound, setsprite ×9 (+3) | -32 ×2, 12288, 223, 224 ×3, 26 (+35) |

### game_premap

| Oracle function | Oracle B | Current | Rank | Donor | Score | Donor B | Shared calls | Shared immediates |
|---|---:|---:|---:|---|---:|---:|---|---|
| 1:0f160 `spawn` (NEAR_MATCH) | 8736 | 0.914 | 1 | Duke 1.3D tree/game.c `spawn` (game/nt) | 0.374 | 16672 | nextsectorneighborz ×2, egs, setsprite, updatesector, changespritestat ×28 (+1) | 1174, 686, 940, 1360, 1123 (+52) |
|  |  |  | 2 | Duke 1.5/GAME.C `spawn` (game/nt) | 0.370 | 17792 | nextsectorneighborz ×2, egs, setsprite, updatesector, changespritestat ×28 (+1) | 1174, 686, 940, 1359, 1360 (+56) |
|  |  |  | 3 | Duke 1.5/SECTOR.C `checkhitsprite` (game/dos) | 0.366 | 6064 | egs, updatesector, changespritestat, krand ×6 | 686, 1359, 12288, 26, 3072 (+30) |
| 1:11380 `render_camera_tile` (NEAR_MATCH) | 816 | 0.873 | 1 | Duke 1.5/GAME.C `se40code` (game_oe/nt) | 0.280 | 1312 | animatesprites, drawmasks, drawrooms | 8 |
|  |  |  | 2 | Duke 1.5/GAME.C `SE40_Draw` (game/nt) | 0.276 | 928 | animatesprites, drawmasks, drawrooms | -- |
|  |  |  | 3 | kenbuild-20241213/PTEX0726/GAME.C `drawscreen` (game/nt) | 0.238 | 928 | -- | 1920, 8 |
| 1:116b0 `render_sprite_camera` (NEAR_MATCH) | 864 | 0.895 | 1 | Duke 1.5/GAME.C `SE40_Draw` (game/nt) | 0.274 | 928 | animatesprites, drawmasks, drawrooms | 28 ×2 |
|  |  |  | 2 | Duke 1.5/GAME.C `se40code` (game_oe/nt) | 0.273 | 1312 | animatesprites, drawmasks, drawrooms | -- |
|  |  |  | 3 | kenbuild-20241213/PTEX0726/GAME.C `drawscreen` (game/nt) | 0.269 | 928 | -- | 1920, 28 ×2 |
| 1:11a10 `render_mirror` (NEAR_MATCH) | 976 | 0.884 | 1 | Duke 1.5/GAME.C `se40code` (game_oe/nt) | 0.292 | 1312 | animatesprites, drawmasks, drawrooms | 31 ×2, 4, 1 ×2 |
|  |  |  | 2 | kenbuild-20241213/PTEX0726/GAME.C `drawscreen` (game/nt) | 0.279 | 928 | -- | 1920, 31 ×2, 4, 1 |
|  |  |  | 3 | Duke 1.5/GAME.C `SE40_Draw` (game/nt) | 0.273 | 928 | animatesprites, drawmasks, drawrooms | 31 ×2, 1 ×2 |
| 1:11de0 `resetplayer` (NEAR_MATCH) | 336 | 0.924 | 1 | Duke 1.5/PREMAP.C `pickrandomspot` (game/nt) | 0.388 | 192 | krand | 4 ×2, 1 |
|  |  |  | 2 | Duke 1.3D tree/gamedef.c `dodge` (game/dos) | 0.387 | 336 | krand | 2, 4, 1 |
|  |  |  | 3 | Duke 1.5/GAMEDEF.C `dodge` (game/nt) | 0.387 | 336 | krand | 2, 4, 1 |
| 1:11f30 `reset_player_motion` (NEAR_MATCH) | 736 | 0.865 | 1 | Duke 1.3D tree/game.c `vglass` (game_oe/nt) | 0.379 | 704 | krand | 3, -1 ×2, 5, 4, 1 |
|  |  |  | 2 | Duke 1.3D tree/game.c `spriteglass` (game_oe/nt) | 0.370 | 752 | krand | 3, -1, 5, 1 |
|  |  |  | 3 | kenbuild-20241213/2DRW0628/GAME.C `prepareboard` (game/nt) | 0.368 | 880 | krand | 17, 100, -1 ×3, 5, 2 ×2 (+2) |
| 1:12210 `prelevel` (NEAR_MATCH) | 9872 | 0.905 | 1 | Duke 1.5/ACTORS.C `movestandables` (game_oe/nt) | 0.355 | 14640 | setsprite, loadtile, deletesprite, krand ×6 | 12288, -2, 224, 26, 25 (+36) |
|  |  |  | 2 | Duke 1.3D tree/game.c `animatesprites` (game/nt) | 0.350 | 7104 | rand | 1359, 1360, 4608, 1280, 27 (+38) |
|  |  |  | 3 | Duke 1.5/GAME.C `SE40_Draw` (game_oe/nt) | 0.347 | 8464 | rand | 1359, 1360, 4608, 1280, 27 (+37) |
| 1:148a0 `newgame` (NEAR_MATCH) | 272 | 0.957 | 1 | Duke 1.5/PREMAP.C `setupbackdrop` (game/nt) | 0.515 | 288 | -- | 2, 1 |
|  |  |  | 2 | Duke 1.3D tree/premap.c `setupbackdrop` (game/nt) | 0.468 | 288 | -- | 2, 1 |
|  |  |  | 3 | Duke 1.5/PREMAP.C `resetmys` (game/nt) | 0.411 | 256 | -- | -- |
| 1:149b0 `enterlevel` (STRICT) | 1104 | 1.000 | 1 | Duke 1.3D tree/premap.c `enterlevel` (game/nt) | 0.346 | 1401 | prelevel, allignwarpelevators, cacheit, playmusic, vscrn (+4) | 13, 7 ×2, 12, -1, 0 ×2 (+3) |
|  |  |  | 2 | Duke 1.5/PREMAP.C `resetpspritevars` (game/nt) | 0.343 | 1520 | gameexit, egs, updatesector | 42, 17, 10, 12 ×2, 3 (+4) |
|  |  |  | 3 | Duke 1.5/PREMAP.C `enterlevel` (game/nt) | 0.340 | 1272 | prelevel, allignwarpelevators, cacheit, playmusic, vscrn (+3) | 13, 7 ×2, 12, -1, 0 (+3) |
| 1:14e00 `check_file_id` (STRICT) | 128 | 1.000 | 1 | kenbuild-20260331/build_1997_07_19/SPHERE.C `loadtables` (engine/nt) | 0.470 | 135 | read, close, open | 514, 256, 12, -1, 1 |
|  |  |  | 2 | kenbuild-20260331/build_1997_07_19/BEND.C `loadtables` (engine/nt) | 0.467 | 64 | read, close, open | 514, 256, 12, -1 |
|  |  |  | 3 | kenbuild-20260331/buildold/EDITART.C `loadtables` (engine/nt) | 0.467 | 64 | read, close, open | 514, 256, 12, -1 |
| 1:14e80 `newscore` (STRICT) | 256 | 1.000 | 1 | Duke 1.5/MENUES.C `getfilenames` (game/nt) | 0.301 | 240 | -- | -1, 2, 4 ×2 |
|  |  |  | 2 | Duke 1.3D tree/menues.c `getfilenames` (game/nt) | 0.301 | 240 | -- | -1, 2, 4 ×2 |
|  |  |  | 3 | Duke 1.5/ACTORS.C `movecyclers` (game/nt) | 0.295 | 288 | -- | 10, 5 ×2, 2, 4 ×2 |
| 1:14f80 `getscorename` (STRICT) | 45 | 1.000 | 1 | Duke 1.5/GAME.C `inittimer` (game/nt) | 0.328 | 48 | -- | -- |
|  |  |  | 2 | Duke 1.3D tree/game.c `inittimer` (game/nt) | 0.328 | 48 | -- | -- |
|  |  |  | 3 | Duke 1.3D tree/timer.c `inittimer` (game/nt) | 0.328 | 48 | -- | -- |

### game_actors

| Oracle function | Oracle B | Current | Rank | Donor | Score | Donor B | Shared calls | Shared immediates |
|---|---:|---:|---:|---|---:|---:|---|---|
| 1:25180 `addweapon` (STRICT) | 768 | 1.000 | 1 | Duke 1.5/ACTORS.C `ifhitbyweapon` (game/nt) | 0.240 | 1024 | -- | 0 ×5, 4 ×4, 1 |
|  |  |  | 2 | Duke 1.3D tree/actors.c `ifhitbyweapon` (game/nt) | 0.233 | 864 | -- | 0 ×4 |
|  |  |  | 3 | Duke 1.5/PREMAP.C `resetprestat` (game_oe/nt) | 0.220 | 864 | -- | 5 ×3, 0 ×5, 4 ×5, 1 ×5 |
| 1:25480 `hitradius` (NEAR_MATCH) | 304 | 1.000 | 1 | Duke 1.3D tree/menues.c `getangplayers` (game/nt) | 0.334 | 272 | getangle | 0 ×4 |
|  |  |  | 2 | Duke 1.5/PLAYER.C `tracers` (game/nt) | 0.330 | 336 | egs | 3, 0 ×5, 4 |
|  |  |  | 3 | Duke 1.3D tree/player.c `tracers` (game/nt) | 0.330 | 336 | egs | 3, 0 ×5, 4 |
| 1:255b0 `spawn_gib` (STRICT) | 80 | 1.000 | 1 | kenbuild-20241213/buil0723/GAME.C `operatesprite` (game/dos) | 0.281 | 80 | -- | -- |
|  |  |  | 2 | buildsrc-20190520/19961013/GAME.C `operatesprite` (game/nt) | 0.281 | 80 | -- | -- |
|  |  |  | 3 | kenbuild-20241213/BUI1101B/GAME.C `operatesprite` (game/nt) | 0.281 | 80 | -- | -- |
| 1:25600 `ssp` (STRICT) | 144 | 1.000 | 1 | Duke 1.5/ACTORS.C `ssp` (game/nt) | 0.690 | 112 | movesprite | 14 ×2, 512, 2047, 7 |
|  |  |  | 2 | Duke 1.3D tree/actors.c `ssp` (game/nt) | 0.690 | 112 | movesprite | 14 ×2, 512, 2047, 7 |
|  |  |  | 3 | Duke 1.5/PLAYER.C `hitawall` (game/nt) | 0.263 | 160 | -- | 512, 2047, 7 |
| 1:25690 `nuke` (STRICT) | 80 | 1.000 | 1 | Duke 1.5/ACTORS.C `lotsofmoney` (game/nt) | 0.359 | 128 | egs | 5, 0 ×3 |
|  |  |  | 2 | Duke 1.3D tree/actors.c `lotsofmoney` (game/nt) | 0.359 | 128 | egs | 5, 0 ×3 |
|  |  |  | 3 | Duke 1.5/ACTORS.C `lotsofmail` (game/nt) | 0.359 | 128 | egs | 5, 0 ×3 |
| 1:256e0 `spawn_blood` (STRICT) | 112 | 1.000 | 1 | Duke 1.5/ACTORS.C `lotsofmoney` (game/nt) | 0.394 | 128 | egs, krand ×3 | 31, 5, 0 |
|  |  |  | 2 | Duke 1.3D tree/actors.c `lotsofmoney` (game/nt) | 0.394 | 128 | egs, krand ×3 | 31, 5, 0 |
|  |  |  | 3 | Duke 1.5/ACTORS.C `lotsofmail` (game/nt) | 0.393 | 128 | egs, krand ×3 | 31, 5, 0 |
| 1:25750 `movesector` (NEAR_MATCH) | 368 | 0.822 | 1 | Duke 1.5/ACTORS.C `ms` (game/nt) | 0.574 | 288 | dragpoint, rotatepoint | 20 ×2, 14 ×2, 512, 2047 ×2, 7 (+1) |
|  |  |  | 2 | Duke 1.3D tree/actors.c `ms` (game/nt) | 0.574 | 288 | dragpoint, rotatepoint | 20 ×2, 14 ×2, 512, 2047 ×2, 7 (+1) |
|  |  |  | 3 | Duke 1.3D tree/actors.c `movedummyplayers` (game/nt) | 0.342 | 496 | setsprite | -- |
| 1:258c0 `wakeactors` (NEAR_MATCH) | 416 | 0.934 | 1 | Duke 1.3D tree/actors.c `movefta` (game/nt) | 0.325 | 848 | findplayer, cansee, changespritestat | 6, 1 ×2 |
|  |  |  | 2 | Duke 1.5/ACTORS.C `movefta` (game/nt) | 0.310 | 896 | findplayer, cansee, changespritestat | 6, 4, 1 ×2 |
|  |  |  | 3 | Duke 1.5/ACTORS.C `checkavailweapon` (game_oe/nt) | 0.287 | 512 | spritesound ×3 | 6, 4, 1 ×2 |
| 1:25a60 `ifsquished` (STRICT) | 384 | 1.000 | 1 | Duke 1.5/ACTORS.C `ifsquished` (game/nt) | 0.386 | 304 | fta | 23, 10, 128, 4 |
|  |  |  | 2 | Duke 1.3D tree/actors.c `ifsquished` (game/nt) | 0.362 | 288 | fta | 1536, 10, 128 |
|  |  |  | 3 | Duke 1.5/SECTOR.C `check_activator_motion` (game/dos) | 0.285 | 336 | -- | 4 ×2 |
| 1:25be0 `effectinsect` (STRICT) | 80 | 1.000 | 1 | Duke 1.5/ACTORS.C `ifhitsectors` (game/nt) | 0.640 | 80 | -- | -1 |
|  |  |  | 2 | Duke 1.3D tree/actors.c `ifhitsectors` (game/nt) | 0.640 | 80 | -- | -1 |
|  |  |  | 3 | Duke 1.3D tree/sector.c `checkcursectnums` (game/nt) | 0.536 | 80 | -- | -1 |
| 1:25c30 `ifhitbyweapon` (STRICT) | 288 | 1.000 | 1 | Duke 1.3D tree/sector.c `findotherplayer` (game/nt) | 0.383 | 208 | -- | 8 |
|  |  |  | 2 | Duke 1.5/SECTOR.C `findotherplayer` (game/dos) | 0.362 | 240 | -- | 8 |
|  |  |  | 3 | Duke 1.3D tree/menues.c `getangplayers` (game/nt) | 0.337 | 272 | -- | 768, 8, 1 |
| 1:25d50 `guts` (NEAR_MATCH) | 448 | 1.000 | 1 | Duke 1.5/GAME.C `cameratext` (game/nt) | 0.271 | 496 | -- | 48, 512 ×2, 6, 0 ×5, 2 |
|  |  |  | 2 | Duke 1.3D tree/game.c `cameratext` (game/nt) | 0.271 | 496 | -- | 48, 512 ×2, 6, 0 ×5, 2 |
|  |  |  | 3 | Duke 1.5/MENUES.C `dispnames` (game/nt) | 0.255 | 576 | -- | 48, 512, 24 ×5, 1024, 0 ×5 (+1) |
| 1:25f10 `moveactors` (NEAR_MATCH) | 12320 | 0.812 | 1 | Duke 1.3D tree/actors.c `moveactors` (game/nt) | 0.502 | 11392 | hitradius, addweapon, execute, shoot ×2, findplayer (+8) | -3328, -3072, 579, 1680, -3 ×2 (+60) |
|  |  |  | 2 | Duke 1.5/ACTORS.C `moveactors` (game/nt) | 0.495 | 12704 | operatemasterswitches, hitradius, addweapon, operateactivators, execute (+10) | -3328, -3072, 579, 1680, -3 ×2 (+59) |
|  |  |  | 3 | Duke 1.5/ACTORS.C `movestandables` (game_oe/nt) | 0.478 | 14640 | checkcursectnums, operatemasterswitches, hitradius, operateactivators, execute (+10) | 5120, 348, -32, 225, 12288 (+44) |
| 1:28f30 `moveweapons` (NEAR_MATCH) | 1456 | 0.962 | 1 | Duke 1.5/ACTORS.C `moveplayers` (game_oe/nt) | 0.311 | 1152 | execute, movesprite | 14 ×2, 512, 65535, 2047 ×2, 1024 (+7) |
|  |  |  | 2 | Duke 1.3D tree/actors.c `movefallers` (game_oe/nt) | 0.309 | 1808 | movesprite | 191, 20 ×2, 14 ×2, 48 ×2, 512 ×3 (+9) |
|  |  |  | 3 | Duke 1.3D tree/actors.c `moveplayers` (game_oe/nt) | 0.304 | 1088 | execute, movesprite | 14 ×2, 512, 65535, 2047 ×2, 1024 (+7) |
| 1:294e0 `moveeffects` (NEAR_MATCH) | 3984 | 0.822 | 1 | Duke 1.3D tree/actors.c `moveexplosions` (game_oe/nt) | 0.417 | 4560 | execute, findplayer, spawn, movesprite ×4, deletesprite ×9 (+1) | 14 ×12, 127, 4096 ×3, 48, 512 ×10 (+17) |
|  |  |  | 2 | Duke 1.5/ACTORS.C `moveexplosions` (game_oe/nt) | 0.390 | 5136 | execute, findplayer, spawn, movesprite ×4, deletesprite ×9 (+1) | 13, 14 ×12, 127, 4096 ×3, 48 (+18) |
|  |  |  | 3 | Duke 1.5/ACTORS.C `moveweapons` (game/nt) | 0.375 | 3680 | execute, findplayer, spawn ×2, movesprite, egs ×3 (+2) | 246, 1360, 96, 192, 14 ×2 (+19) |
| 1:2a470 `moveplayers` (NEAR_MATCH) | 1008 | 1.000 | 1 | Duke 1.3D tree/actors.c `moveplayers` (game_oe/nt) | 0.378 | 1088 | execute, movesprite, setsprite, deletesprite | 14 ×2, 512, 2047 ×2, 1024, 128 (+5) |
|  |  |  | 2 | Duke 1.5/ACTORS.C `moveplayers` (game_oe/nt) | 0.366 | 1152 | execute, movesprite, setsprite, deletesprite | 14 ×2, 512, 2047 ×2, 1024, 128 (+5) |
|  |  |  | 3 | Duke 1.5/ACTORS.C `movedummyplayers` (game/nt) | 0.346 | 560 | setsprite, deletesprite | 8 ×2, 0 ×2, 2, 4 ×3 |
| 1:2a860 `movemusic` (NEAR_MATCH) | 816 | 0.949 | 1 | Duke 1.5/ACTORS.C `movefx` (game/nt) | 0.289 | 1264 | spritesound | 1000 ×2, 999, 12, 0 ×2, 4 ×4 (+1) |
|  |  |  | 2 | Duke 1.5/SECTOR.C `doanimations` (game_oe/dos) | 0.285 | 1200 | stopsound, spritesound | 1000, 3, 0 ×3, 4 ×3, 1 |
|  |  |  | 3 | Duke 1.3D tree/sector.c `doanimations` (game_oe/nt) | 0.281 | 1184 | stopsound, spritesound | 1000, 3, 0 ×3, 4 ×2, 1 |
| 1:2ab90 `movestandables` (NEAR_MATCH) | 3735 | 0.866 | 1 | Duke 1.5/ACTORS.C `moveweapons` (game/nt) | 0.432 | 3680 | execute, findplayer, spawn ×6, movesprite, egs ×3 (+3) | 8704, 12288, 16384, 192, 14 ×2 (+20) |
|  |  |  | 2 | Duke 1.3D tree/actors.c `moveweapons` (game/nt) | 0.426 | 3408 | execute, findplayer, spawn ×6, movesprite, egs ×3 (+3) | 8704, 12288, 16384, 192, 14 ×2 (+19) |
|  |  |  | 3 | Duke 1.3D tree/actors.c `movetransports` (game_oe/nt) | 0.385 | 3584 | checkcursectnums ×2, spawn ×6, movesprite | -2, 1536, 14 ×2, 4096 ×2, 48 (+12) |

## Compile inventory

DOS-hosted versus DOS-targeted NT controls had identical CODE bytes, named fixups, and code symbols for 17/17 pairs.
The earlier default NT target used a larger `dirent` in BUIL0422/BUILD.C, changing `getfilenames` from 180 to 186 bytes under both engine profiles. Explicit DOS targeting restored the 180-byte body and the full code layout in the control.

| Source group | Files | Compiled | Successful builds |
|---|---:|---:|---:|
| buildsrc-20190520 | 23 | 23 | 42 |
| duke-1.3d-tree | 11 | 11 | 22 |
| duke-1.5 | 12 | 12 | 24 |
| kenbuild-20241213 | 45 | 44 | 94 |
| kenbuild-20260331 | 97 | 70 | 84 |

The Duke 1.3D tree includes reconstructed or backported code and borrowed declarations. The Duke 1.5 sources use the declarations supplied with that release. Compatibility builds for incomplete Ken snapshots use the header sources and edits recorded in the build logs and in `tools_proposed/xscan_compat.py`. The March 1995 ENGINE.C ends inside dragpoint, so the compile uses the complete prefix through neartag. The later engine adaptations adjust inline assembler syntax for Watcom 9.5. Failed attempts include old 16-bit inline assembly, incomplete files and inliner timeouts.

| Source | Successful profiles | Failed attempts by host/profile |
|---|---|---|
| kenbuild-20241213/BUIL0422/BUILD.C | [engine/nt](logs-nt/0ba6bde3978b-2.txt), [engine_oe/nt](logs-nt/ca61b834bdef-2.txt), [engine/dos](logs-dos/0ba6bde3978b-1.txt), [engine_oe/dos](logs-dos/ca61b834bdef-1.txt) | -- |
| kenbuild-20241213/BUIL0422/ENGINE.C | [engine/nt](logs-nt/321bec66613c-2.txt), [engine_oe/nt](logs-nt/301eef94d172-2.txt), [engine/dos](logs-dos/321bec66613c-1.txt), [engine_oe/dos](logs-dos/301eef94d172-1.txt) | -- |
| kenbuild-20241213/BUIL0422/GAME.C | [game/nt](logs-nt/8fa23916bdd0-2.txt), [game_oe/nt](logs-nt/009b0ab85bf2-2.txt), [game/dos](logs-dos/8fa23916bdd0-1.txt), [game_oe/dos](logs-dos/009b0ab85bf2-1.txt) | -- |
| kenbuild-20241213/2DRW0628/GAME.C | [game/nt](logs-nt/2f2faa18a8a7-2.txt), [game_oe/nt](logs-nt/d3f57d52aae8-2.txt) | -- |
| kenbuild-20241213/2DRW0719/GAME.C | [game/nt](logs-nt/9ac6df23b2f3-2.txt), [game_oe/nt](logs-nt/f32fd294416f-2.txt) | -- |
| kenbuild-20241213/BUI1101B/GAME.C | [game/nt](logs-nt/3e50cb02cd2c-3.txt), [game_oe/nt](logs-nt/d092172b671d-3.txt) | -- |
| kenbuild-20241213/buil010294_BUILDOLD/BUILD.C | [engine/nt](logs-nt/b2f53f36529e-2.txt) | [engine_oe/nt](logs-nt/a589f0050dac-2.txt) |
| kenbuild-20241213/buil010294_BUILDOLD/GAME.C | [game/nt](logs-nt/e2356c1d5f7f-3.txt), [game_oe/nt](logs-nt/6ef1783e2c7a-3.txt) | -- |
| kenbuild-20241213/BUIL0408/GAME.C | [game/nt](logs-nt/00b686768c6f-2.txt), [game_oe/nt](logs-nt/5e16684608c0-2.txt) | -- |
| kenbuild-20241213/BUIL0615/GAME.C | [game/nt](logs-nt/7e59eb360f5d-5.txt), [game_oe/nt](logs-nt/f91f3b4c6847-5.txt) | -- |
| kenbuild-20241213/BUIL0624/GAME.C | [game/nt](logs-nt/3572b903df81-2.txt), [game_oe/nt](logs-nt/321db9681e17-2.txt) | -- |
| kenbuild-20241213/buil062896/ENGINE.C | [engine/nt](logs-nt/1584d5cec97c-2.txt), [engine_oe/nt](logs-nt/e86ccc16c7c0-2.txt) | -- |
| kenbuild-20241213/BUIL0719/GAME.C | [game/nt](logs-nt/32c8a03b71dc-2.txt), [game_oe/nt](logs-nt/c31ed966f1f6-2.txt) | -- |
| kenbuild-20241213/buil0723/GAME.C | [game/nt](logs-nt/f01d09070f95-4.txt), [game_oe/nt](logs-nt/fd6a2dcff349-4.txt), [game/dos](logs-dos/f01d09070f95-1.txt), [game_oe/dos](logs-dos/fd6a2dcff349-1.txt) | -- |
| kenbuild-20241213/BUIL0801/GAME.C | [game/nt](logs-nt/d754cb5071b7-2.txt), [game_oe/nt](logs-nt/f9608ad97b9f-2.txt) | -- |
| kenbuild-20241213/BUIL0906/GAME.C | [game/nt](logs-nt/c14874efb5d7-5.txt), [game_oe/nt](logs-nt/ff45aabcacc7-5.txt) | -- |
| kenbuild-20241213/BUIL1020/GAME.C | [game/nt](logs-nt/fd7b73011280-2.txt), [game_oe/nt](logs-nt/9d6731178235-2.txt) | -- |
| kenbuild-20241213/BUIL1109/GAME.C | [game/nt](logs-nt/6b69586d695d-3.txt), [game_oe/nt](logs-nt/eff553a29b01-3.txt) | -- |
| kenbuild-20241213/buil112093/BUILD.C | [engine/nt](logs-nt/b344d8decf57-2.txt), [engine_oe/nt](logs-nt/b6d135040328-2.txt) | -- |
| kenbuild-20241213/buil112093/GAME.C | [game/nt](logs-nt/893e95b1de7d-2.txt), [game_oe/nt](logs-nt/aa257a4a56dc-2.txt) | -- |
| kenbuild-20241213/BUIL1121/GAME.C | [game/nt](logs-nt/fcd5f4d5d5e8-3.txt), [game_oe/nt](logs-nt/07dfa022e426-3.txt) | -- |
| kenbuild-20241213/buil112493/BUILD.C | [engine/nt](logs-nt/186cab17a28a-2.txt), [engine_oe/nt](logs-nt/c41c58aedbb2-2.txt) | -- |
| kenbuild-20241213/buil112493/GAME.C | [game/nt](logs-nt/9e9eef229e77-2.txt), [game_oe/nt](logs-nt/e1fc70f37eaa-2.txt) | -- |
| kenbuild-20241213/buil120693/BUILD.C | [engine/nt](logs-nt/9ba44ae89d5a-2.txt), [engine_oe/nt](logs-nt/fcada7edd92e-2.txt) | -- |
| kenbuild-20241213/buil120693/GAME.C | [game/nt](logs-nt/929309e9be85-2.txt), [game_oe/nt](logs-nt/14c2910fa9c9-2.txt) | -- |
| kenbuild-20241213/buil120893/BUILD.C | [engine/nt](logs-nt/18f67e2d7254-2.txt), [engine_oe/nt](logs-nt/248526a1b632-2.txt) | -- |
| kenbuild-20241213/buil120893/GAME.C | [game/nt](logs-nt/88937fb42623-2.txt), [game_oe/nt](logs-nt/d546bb6edd41-2.txt) | -- |
| kenbuild-20241213/BUIL1211/GAME.C | [game/nt](logs-nt/52c83f0f4a02-4.txt), [game_oe/nt](logs-nt/3d4d5c9e7797-4.txt) | -- |
| kenbuild-20241213/buil121393/BUILD.C | [engine/nt](logs-nt/ff1d10771f8a-2.txt), [engine_oe/nt](logs-nt/978bed973ba8-2.txt) | -- |
| kenbuild-20241213/buil121393/GAME.C | [game/nt](logs-nt/7df86d19c413-2.txt), [game_oe/nt](logs-nt/8d9e136546e5-2.txt) | -- |
| kenbuild-20241213/buil1995/BUILD.C | [engine/nt](logs-nt/e516298541af-2.txt), [engine_oe/nt](logs-nt/45612a33330b-2.txt) | -- |
| kenbuild-20241213/buil1995/ENGINE.C | [engine/nt](logs-nt/bcb1f070579f-2.txt) | [engine_oe/nt](logs-nt/037e323fc098-2.txt) |
| kenbuild-20241213/buil1995/GAME.C | [game/nt](logs-nt/ebaff45e571a-2.txt), [game_oe/nt](logs-nt/7a3871024e87-2.txt) | -- |
| kenbuild-20241213/BUILD1/BUILD.C | -- | [engine/nt](logs-nt/36b0fa2f83bb-4.txt), [engine_oe/nt](logs-nt/0bf42d7c758c-2.txt) |
| kenbuild-20241213/BUILD_FROM_MATT/GAME.C | [game/nt](logs-nt/f64bc6766eb7-2.txt), [game_oe/nt](logs-nt/48c1d5e9dafe-2.txt) | -- |
| kenbuild-20241213/BUILD_FROM_SW_TREE/GAME.C | [game/nt](logs-nt/7f5703d705f0-2.txt), [game_oe/nt](logs-nt/8df9d8cf146a-2.txt) | -- |
| kenbuild-20241213/BUILDBAK/BUILD.C | [engine/nt](logs-nt/4384ee65d113-2.txt), [engine_oe/nt](logs-nt/254570f8a7a4-2.txt) | -- |
| kenbuild-20241213/BUILDBAK/ENGINE.C | [engine/nt](logs-nt/cef04a7e30ce-6.txt) | [engine_oe/nt](logs-nt/849c48104bc8-2.txt) |
| kenbuild-20241213/BUILDBAK/GAME.C | [game/nt](logs-nt/ea90b27731be-2.txt), [game_oe/nt](logs-nt/c8e82afa8f91-2.txt) | -- |
| kenbuild-20241213/buildold_12_30_94/GAME.C | [game/nt](logs-nt/665ea3110f96-5.txt), [game_oe/nt](logs-nt/f262f61a170e-5.txt), [game/dos](logs-dos/665ea3110f96-1.txt), [game_oe/dos](logs-dos/f262f61a170e-1.txt) | -- |
| kenbuild-20241213/builold!/BUILD.C | [engine/nt](logs-nt/3bf93b6824ca-2.txt), [engine_oe/nt](logs-nt/85e6fd96118e-2.txt) | -- |
| kenbuild-20241213/builold!/GAME.C | [game/nt](logs-nt/3936704d65bd-2.txt), [game_oe/nt](logs-nt/5e6b78bfa22d-2.txt) | -- |
| kenbuild-20241213/builsect/BUILD.C | [engine/nt](logs-nt/fcf02077dc51-2.txt) | [engine_oe/nt](logs-nt/17db83ca5aa3-2.txt) |
| kenbuild-20241213/BUILT/GAME.C | [game/nt](logs-nt/4b254f8f2df9-2.txt), [game_oe/nt](logs-nt/115a925377cb-2.txt) | -- |
| kenbuild-20241213/PTEX0726/GAME.C | [game/nt](logs-nt/79515e2e1a3c-2.txt), [game_oe/nt](logs-nt/04f76f1c8930-2.txt) | -- |
| kenbuild-20260331/build_1993_04_14/BUI41593.C | -- | [engine/nt](logs-nt/da69007587cf-4.txt) |
| kenbuild-20260331/build_1993_04_14/BUILD.C | -- | [engine/nt](logs-nt/0e298ccc0c67-4.txt), [engine_oe/nt](logs-nt/bf6bae2fb4d4-2.txt) |
| kenbuild-20260331/build_1993_04_14/HLINE.C | -- | [engine/nt](logs-nt/84ce397db970-4.txt) |
| kenbuild-20260331/build_1993_04_14/pic33093.c | -- | [engine/nt](logs-nt/fca96c981ff6-4.txt) |
| kenbuild-20260331/build_1993_04_14/pic40693.c | -- | [engine/nt](logs-nt/77864349e8e3-4.txt) |
| kenbuild-20260331/build_1993_06_22/BUI53193.C | -- | [engine/nt](logs-nt/31c0ad6dbfdf-4.txt) |
| kenbuild-20260331/build_1993_06_22/bui61593.c | -- | [engine/nt](logs-nt/dea847bd19cf-4.txt) |
| kenbuild-20260331/build_1993_06_22/bui62293.c | -- | [engine/nt](logs-nt/6a0652e327cc-4.txt) |
| kenbuild-20260331/build_1993_06_22/bui71593.c | -- | [engine/nt](logs-nt/c0943c010f6d-4.txt) |
| kenbuild-20260331/build_1993_06_22/EDITBAD.C | -- | [engine/nt](logs-nt/496f6203119b-4.txt) |
| kenbuild-20260331/build_1993_06_22/HLINE.C | -- | [engine/nt](logs-nt/9b0291c8c1b2-4.txt) |
| kenbuild-20260331/build_1993_06_22/MAKTABLE.C | [engine/nt](logs-nt/141e50202a33-2.txt) | -- |
| kenbuild-20260331/build_1993_06_22/RAYTRACE.C | -- | [engine/nt](logs-nt/28fecbe861d6-4.txt) |
| kenbuild-20260331/build_1994_01_09/BUI94101.C | [engine/nt](logs-nt/625c3090e766-2.txt) | -- |
| kenbuild-20260331/build_1994_01_09/BUI94102.C | [engine/nt](logs-nt/9a5e9cb67eef-2.txt) | -- |
| kenbuild-20260331/build_1994_01_09/bui94109.c | [engine/nt](logs-nt/0cf3a5eef3b1-2.txt) | -- |
| kenbuild-20260331/build_1994_10_17_partial/CONVMAP5.C | [engine/nt](logs-nt/4a5e2f23d488-2.txt) | -- |
| kenbuild-20260331/build_1995_03_28_partial/ENGINE.C | [engine/nt](logs-nt/d9a98d389af7-8.txt), [engine/dos](logs-dos/d9a98d389af7-1.txt) | [engine_oe/nt](logs-nt/1746c71231ee-3.txt) |
| kenbuild-20260331/build_1996_11_12_partial/BSTUB.C | [game/nt](logs-nt/cb55903a25ac-3.txt), [game_oe/nt](logs-nt/48356d2ecd01-3.txt) | -- |
| kenbuild-20260331/build_1996_11_12_partial/ENGINE.C | [engine/nt](logs-nt/a0441c7499f4-6.txt), [engine_oe/nt](logs-nt/ed1e1a584b7a-2.txt) | -- |
| kenbuild-20260331/build_1996_11_12_partial/GAME.C | [game/nt](logs-nt/9668eef1deb3-3.txt), [game_oe/nt](logs-nt/442e20c59e53-3.txt) | -- |
| kenbuild-20260331/build_1996_11_12_partial/GETPOINT.C | -- | [engine/nt](logs-nt/5f6f7fa22df0-4.txt) |
| kenbuild-20260331/build_1996_11_12_partial/SNOTS.C | [engine/nt](logs-nt/a1e2035ec414-3.txt) | -- |
| kenbuild-20260331/build_1997_07_19/ADPCM.C | [engine/nt](logs-nt/25cbfdb78c14-2.txt) | -- |
| kenbuild-20260331/build_1997_07_19/BEND.C | [engine/nt](logs-nt/551435b61b8d-2.txt) | -- |
| kenbuild-20260331/build_1997_07_19/BICOMC.C | [engine/nt](logs-nt/6e4ef4d11767-2.txt) | -- |
| kenbuild-20260331/build_1997_07_19/BSTUB.C | [game/nt](logs-nt/53d0824c4d49-3.txt), [game_oe/nt](logs-nt/96627692a1c4-3.txt) | -- |
| kenbuild-20260331/build_1997_07_19/BUILD.C | [engine/nt](logs-nt/7f995df4dc6b-3.txt), [engine_oe/nt](logs-nt/a3c404721f51-2.txt) | -- |
| kenbuild-20260331/build_1997_07_19/CACHE1D.C | [engine/nt](logs-nt/164a177d696c-2.txt) | -- |
| kenbuild-20260331/build_1997_07_19/CHESS.C | [engine/nt](logs-nt/947726623b9a-3.txt) | -- |
| kenbuild-20260331/build_1997_07_19/CHESSBK.C | [engine/nt](logs-nt/1ab5d4436d29-3.txt) | -- |
| kenbuild-20260331/build_1997_07_19/CONVART0.C | [engine/nt](logs-nt/c3d1f83f26d7-2.txt) | -- |
| kenbuild-20260331/build_1997_07_19/CONVART1.C | [engine/nt](logs-nt/e4d465ae8d5c-2.txt) | -- |
| kenbuild-20260331/build_1997_07_19/CONVMAP0.C | [engine/nt](logs-nt/daf8a5bc5d24-2.txt) | -- |
| kenbuild-20260331/build_1997_07_19/CONVMAP1.C | [engine/nt](logs-nt/b6e4743e12af-2.txt) | -- |
| kenbuild-20260331/build_1997_07_19/CONVMAP2.C | [engine/nt](logs-nt/6c8c551e38a4-2.txt) | -- |
| kenbuild-20260331/build_1997_07_19/CONVMAP3.C | [engine/nt](logs-nt/c176cb9d9f0d-2.txt) | -- |
| kenbuild-20260331/build_1997_07_19/CONVMAP4.C | [engine/nt](logs-nt/827395235834-2.txt) | -- |
| kenbuild-20260331/build_1997_07_19/CONVMAP5.C | [engine/nt](logs-nt/87842f9e36bc-2.txt) | -- |
| kenbuild-20260331/build_1997_07_19/CONVMAP6.C | [engine/nt](logs-nt/41d7eb7ff99f-2.txt) | -- |
| kenbuild-20260331/build_1997_07_19/CONVMAP7.C | [engine/nt](logs-nt/ef0a6e298bb7-2.txt) | -- |
| kenbuild-20260331/build_1997_07_19/EDITART.C | [engine/nt](logs-nt/e6d2ad48fedd-2.txt) | -- |
| kenbuild-20260331/build_1997_07_19/ENGINE.C | [engine/nt](logs-nt/f2c698c4ed6f-6.txt), [engine_oe/nt](logs-nt/4b2fc4d23860-2.txt) | -- |
| kenbuild-20260331/build_1997_07_19/FIXSPRBT.C | [engine/nt](logs-nt/a5072c02728d-2.txt) | -- |
| kenbuild-20260331/build_1997_07_19/GAME.C | [game/nt](logs-nt/f2bdaf91df13-3.txt), [game_oe/nt](logs-nt/dd090b0fef49-3.txt) | -- |
| kenbuild-20260331/build_1997_07_19/GETPOINT.C | -- | [engine/nt](logs-nt/44a7d33af984-4.txt) |
| kenbuild-20260331/build_1997_07_19/JOY.C | [engine/nt](logs-nt/ff5c9c831e90-2.txt) | -- |
| kenbuild-20260331/build_1997_07_19/KCODE.C | [engine/nt](logs-nt/51892301b454-2.txt) | -- |
| kenbuild-20260331/build_1997_07_19/LAVA.C | [engine/nt](logs-nt/47f10524a6ff-2.txt) | -- |
| kenbuild-20260331/build_1997_07_19/MAKTABLE.C | [engine/nt](logs-nt/01bf782077c0-2.txt) | -- |
| kenbuild-20260331/build_1997_07_19/MAKTEXT.C | [engine/nt](logs-nt/db238475eb54-2.txt) | -- |
| kenbuild-20260331/build_1997_07_19/MIXTILES.C | -- | [engine/nt](logs-nt/0c2cfed015f1-4.txt) |
| kenbuild-20260331/build_1997_07_19/NAMES.C | [engine/nt](logs-nt/4714a3c72f26-2.txt) | -- |
| kenbuild-20260331/build_1997_07_19/RSIZEART.C | [engine/nt](logs-nt/eeb5e12b7e4b-2.txt) | -- |
| kenbuild-20260331/build_1997_07_19/SETUP.C | [engine/nt](logs-nt/cce2ab12b397-2.txt) | -- |
| kenbuild-20260331/build_1997_07_19/SIMPLE.C | [engine/nt](logs-nt/e08c7e093098-5.txt) | -- |
| kenbuild-20260331/build_1997_07_19/SIMPLNET.C | -- | [engine/nt](logs-nt/e6bea906ab67-4.txt) |
| kenbuild-20260331/build_1997_07_19/SNOTS.C | [engine/nt](logs-nt/24dc69660bba-3.txt) | -- |
| kenbuild-20260331/build_1997_07_19/SPHERE.C | [engine/nt](logs-nt/ca59d54ffc49-2.txt) | -- |
| kenbuild-20260331/build_1997_07_19/SPHERE2.C | [engine/nt](logs-nt/ca69015e8e3c-2.txt) | -- |
| kenbuild-20260331/build_1997_07_19/SPRILINK.C | [engine/nt](logs-nt/29800e698d6e-2.txt) | -- |
| kenbuild-20260331/build_1997_07_19/SPRITE.C | [engine/nt](logs-nt/903ccda33bdd-2.txt) | -- |
| kenbuild-20260331/build_1997_07_19/TERM.C | [engine/nt](logs-nt/0cee1d864e33-2.txt) | -- |
| kenbuild-20260331/build_1997_07_19/TRANSPAL.C | [engine/nt](logs-nt/c632409390f9-2.txt) | -- |
| kenbuild-20260331/build_1997_07_19/WKSM.C | [engine/nt](logs-nt/7eb1e8d55959-2.txt) | -- |
| kenbuild-20260331/build_1997_07_19/WSAY.C | [engine/nt](logs-nt/3ade4416cf44-2.txt) | -- |
| kenbuild-20260331/build_1997_07_19/WSND.C | [engine/nt](logs-nt/f897fbf17d75-2.txt) | -- |
| kenbuild-20260331/buildold/BUILD.C | [engine/nt](logs-nt/a07c49967531-2.txt) | [engine_oe/nt](logs-nt/c0b90dbf163e-2.txt) |
| kenbuild-20260331/buildold/BUILD2.C | -- | [engine/nt](logs-nt/2078db0bf72d-4.txt) |
| kenbuild-20260331/buildold/BUILD3.C | -- | [engine/nt](logs-nt/a0fcda48c482-4.txt) |
| kenbuild-20260331/buildold/BUILD4.C | -- | [engine/nt](logs-nt/0c93413e7b16-4.txt) |
| kenbuild-20260331/buildold/BUILDREC.C | -- | [engine/nt](logs-nt/d3043fdcb832-4.txt) |
| kenbuild-20260331/buildold/EDITART.C | [engine/nt](logs-nt/1cb1074d4d57-2.txt) | -- |
| kenbuild-20260331/buildold/EDITMAP.C | -- | [engine/nt](logs-nt/c44f4177ea99-4.txt) |
| kenbuild-20260331/buildold/GAME.C | [game/nt](logs-nt/b196a53f45e9-4.txt), [game_oe/nt](logs-nt/bc4292be9146-4.txt) | -- |
| kenbuild-20260331/buildold/GRAPHW.C | -- | [engine/nt](logs-nt/d09816ed3d04-4.txt) |
| kenbuild-20260331/buildold/GRAPHX.C | -- | [engine/nt](logs-nt/e188dd1210ca-4.txt) |
| kenbuild-20260331/buildold/HLINE.C | -- | [engine/nt](logs-nt/9a54650091ea-4.txt) |
| kenbuild-20260331/buildold/MAKTABLE.C | [engine/nt](logs-nt/321e08ef0a8d-2.txt) | -- |
| kenbuild-20260331/buildold/PIXSCREW.C | [engine/nt](logs-nt/311b7917b6d0-2.txt) | -- |
| kenbuild-20260331/buildold/PIXSCRW2.C | [engine/nt](logs-nt/3f0d45f57533-2.txt) | -- |
| kenbuild-20260331/buildold/RAYTRACE.C | -- | [engine/nt](logs-nt/08369d3a4305-4.txt) |
| kenbuild-20260331/buildold/SIMPLE.C | [engine/nt](logs-nt/51d2e8e7c7c3-3.txt) | -- |
| kenbuild-20260331/buildold/VOXEL1.C | [engine/nt](logs-nt/2c0cf6cece74-2.txt) | -- |
| kenbuild-20260331/buildold/VOXEL2.C | [engine/nt](logs-nt/19f48093f209-2.txt) | -- |
| kenbuild-20260331/dmbuild_1995_05_25/WAD2ART.C | [engine/nt](logs-nt/cc033eba20d8-2.txt) | -- |
| kenbuild-20260331/dmbuild_1995_05_25/WAD2MAP.C | -- | [engine/nt](logs-nt/a12f259459ff-4.txt) |
| kenbuild-20260331/FILES_FROM_MATT/2draw/2DRAW.C | [engine/nt](logs-nt/6cf3f39ff6d7-2.txt) | -- |
| kenbuild-20260331/FILES_FROM_MATT/2draw/GAME.C | [game/nt](logs-nt/1eef5bcf6080-2.txt), [game_oe/nt](logs-nt/ec2dc2d596bc-2.txt) | -- |
| kenbuild-20260331/FILES_FROM_MATT/2draw/MMULTI.C | [multi/nt](logs-nt/154c1f50ec4a-2.txt) | -- |
| kenbuild-20260331/FILES_FROM_MATT/BUILD/BSTUB.C | [game/nt](logs-nt/1a1aa3422ebd-2.txt), [game_oe/nt](logs-nt/b039bdede8e0-2.txt) | -- |
| kenbuild-20260331/FILES_FROM_MATT/BUILD/CACHE1D.C | [engine/nt](logs-nt/ec6ed6791671-2.txt) | -- |
| kenbuild-20260331/FILES_FROM_MATT/BUILD/FIXSPRBT.C | [engine/nt](logs-nt/8fa35a834156-2.txt) | -- |
| kenbuild-20260331/FILES_FROM_MATT/BUILD/GAME.C | [game/nt](logs-nt/809067e3a666-2.txt), [game_oe/nt](logs-nt/abd21ec82e07-2.txt) | -- |
| kenbuild-20260331/FILES_FROM_MATT/BUILD/GETPOINT.C | -- | [engine/nt](logs-nt/4ffa6d2168c9-4.txt) |
| kenbuild-20260331/FILES_FROM_SW_TREE/build/BSTUB.C | [game/nt](logs-nt/d44981979caa-2.txt), [game_oe/nt](logs-nt/a4633e4c9345-2.txt) | -- |
| kenbuild-20260331/FILES_FROM_SW_TREE/build/GAME.C | [game/nt](logs-nt/de21ac02c46e-2.txt), [game_oe/nt](logs-nt/a719beab110d-2.txt) | -- |
| buildsrc-20190520/19950920/BSTUB.C | [game/nt](logs-nt/6410bcd1ed04-2.txt), [game_oe/nt](logs-nt/8f89734ad6fb-2.txt) | -- |
| buildsrc-20190520/19950920/BUILD.C | [engine/nt](logs-nt/b642c8a567f7-5.txt), [engine_oe/nt](logs-nt/d8acf6f9b83f-2.txt) | -- |
| buildsrc-20190520/19950920/CONVART1.C | [engine/nt](logs-nt/6ab2b9ba2966-2.txt) | -- |
| buildsrc-20190520/19950920/POLYTEX5.C | [engine/nt](logs-nt/90527cd93d38-2.txt), [engine/dos](logs-dos/90527cd93d38-1.txt) | -- |
| buildsrc-20190520/19951017/POLYTEX5.C | [engine/nt](logs-nt/68e1e34b5064-2.txt) | -- |
| buildsrc-20190520/19951025/BSTUB.C | [game/nt](logs-nt/6da1e774d69c-2.txt), [game_oe/nt](logs-nt/d11fb4e1ed92-2.txt) | -- |
| buildsrc-20190520/19951208/BSTUB.C | [game/nt](logs-nt/d71619db59f0-2.txt), [game_oe/nt](logs-nt/99067c9cc098-2.txt) | -- |
| buildsrc-20190520/19960213/BSTUB.C | [game/nt](logs-nt/5275bf7ed32c-2.txt), [game_oe/nt](logs-nt/2ada94d6d75f-2.txt) | -- |
| buildsrc-20190520/19960319/BSTUB.C | [game/nt](logs-nt/6f3241a3b505-2.txt), [game_oe/nt](logs-nt/16101f3de72f-2.txt) | -- |
| buildsrc-20190520/19960320/BSTUB.C | [game/nt](logs-nt/8bd01a4dd9b7-2.txt), [game_oe/nt](logs-nt/cc17a486a8d0-2.txt) | -- |
| buildsrc-20190520/19960606/BSTUB.C | [game/nt](logs-nt/250e6e482491-2.txt), [game_oe/nt](logs-nt/03e924e2c6d0-2.txt) | -- |
| buildsrc-20190520/19960606/BUILD.C | [engine/nt](logs-nt/39f50f382371-5.txt), [engine_oe/nt](logs-nt/4203d789af90-2.txt) | -- |
| buildsrc-20190520/19961013/BSTUB.C | [game/nt](logs-nt/ed49c6f904fb-2.txt), [game_oe/nt](logs-nt/3caaab22c803-2.txt) | -- |
| buildsrc-20190520/19961013/GAME.C | [game/nt](logs-nt/6ef604dce7cc-2.txt), [game_oe/nt](logs-nt/58f2d3813af2-2.txt) | -- |
| buildsrc-20190520/19961204/TRANSPAL.C | [engine/nt](logs-nt/ff808f7b9b62-4.txt) | -- |
| buildsrc-20190520/19961213/BSTUB.C | [game/nt](logs-nt/74185273775e-2.txt), [game_oe/nt](logs-nt/71cfe08243cb-2.txt) | -- |
| buildsrc-20190520/19970221/BSTUB.C | [game/nt](logs-nt/79d32ce1ea7b-2.txt), [game_oe/nt](logs-nt/1febff8f2fbd-2.txt) | -- |
| buildsrc-20190520/19970305/BSTUB.C | [game/nt](logs-nt/602d042ae05c-2.txt), [game_oe/nt](logs-nt/7aa354e3d4a2-2.txt) | -- |
| buildsrc-20190520/19970522/BSTUB.C | [game/nt](logs-nt/ececca9634cd-2.txt), [game_oe/nt](logs-nt/b1deb188bfb8-2.txt) | -- |
| buildsrc-20190520/SRC_9505/BSTUB.C | [game/nt](logs-nt/39554370c970-2.txt), [game_oe/nt](logs-nt/c684d7d6a712-2.txt) | -- |
| buildsrc-20190520/SRC_9505/BUILD.C | [engine/nt](logs-nt/b7c31708800c-2.txt), [engine_oe/nt](logs-nt/d8ac936a5869-2.txt) | -- |
| buildsrc-20190520/SRC_9505/CACHE1D.C | [engine/nt](logs-nt/2521f178cab4-2.txt) | -- |
| buildsrc-20190520/SRC_9505/ENGINE.C | [engine/nt](logs-nt/be778b85c02e-2.txt), [engine/dos](logs-dos/be778b85c02e-1.txt) | [engine_oe/nt](logs-nt/f6a3e2da8d5e-2.txt) |
| Duke 1.3D tree/actors.c | [game/nt](logs-nt/55485ea904a8-3.txt), [game_oe/nt](logs-nt/5f85736fab8a-3.txt) | -- |
| Duke 1.3D tree/config.c | [game/nt](logs-nt/62691bd06079-3.txt), [game_oe/nt](logs-nt/61bef1533308-3.txt) | -- |
| Duke 1.3D tree/game.c | [game/nt](logs-nt/7135ab756ddb-3.txt), [game_oe/nt](logs-nt/b56a3446ad13-3.txt) | -- |
| Duke 1.3D tree/gamedef.c | [game/nt](logs-nt/5ac7c17a5748-3.txt), [game_oe/nt](logs-nt/f95ee67abe60-3.txt), [game/dos](logs-dos/5ac7c17a5748-1.txt), [game_oe/dos](logs-dos/f95ee67abe60-1.txt) | -- |
| Duke 1.3D tree/global.c | [game/nt](logs-nt/22b36f7b34dd-3.txt), [game_oe/nt](logs-nt/144624959d75-3.txt) | -- |
| Duke 1.3D tree/menues.c | [game/nt](logs-nt/4a0bad1aaa65-3.txt) | [game_oe/nt](logs-nt/e84bbc1daaa3-3.txt) |
| Duke 1.3D tree/player.c | [game/nt](logs-nt/79185d144cd0-3.txt) | [game_oe/nt](logs-nt/1cdd4035b8d7-3.txt) |
| Duke 1.3D tree/premap.c | [game/nt](logs-nt/eeb44ecbb5f5-3.txt), [game_oe/nt](logs-nt/656bb348cb9a-3.txt) | -- |
| Duke 1.3D tree/sector.c | [game/nt](logs-nt/5658f696db4e-3.txt), [game_oe/nt](logs-nt/2c574b92529d-3.txt) | -- |
| Duke 1.3D tree/sounds.c | [game/nt](logs-nt/bbbd0401a08d-3.txt), [game_oe/nt](logs-nt/8cf782fa8150-3.txt) | -- |
| Duke 1.3D tree/timer.c | [game/nt](logs-nt/721ef0f6dddb-2.txt), [game_oe/nt](logs-nt/c5d9dbe79423-2.txt) | -- |
| Duke 1.5/ACTORS.C | [game/nt](logs-nt/54b98eff1092-2.txt), [game_oe/nt](logs-nt/03b0af78fee0-2.txt) | -- |
| Duke 1.5/ANIMLIB.C | [game/nt](logs-nt/12ff6bba21b0-2.txt), [game_oe/nt](logs-nt/06bca8b55683-2.txt) | -- |
| Duke 1.5/CONFIG.C | [game/nt](logs-nt/dc9124074246-2.txt), [game_oe/nt](logs-nt/6216d6a143b0-2.txt) | -- |
| Duke 1.5/GAME.C | [game/nt](logs-nt/e8bd456b05b5-2.txt), [game_oe/nt](logs-nt/e4cd38933598-2.txt) | -- |
| Duke 1.5/GAMEDEF.C | [game/nt](logs-nt/35b44cb40386-2.txt), [game_oe/nt](logs-nt/442415ec824f-2.txt) | -- |
| Duke 1.5/GLOBAL.C | [game/nt](logs-nt/a7b6b722c1be-2.txt), [game_oe/nt](logs-nt/8c1b22d39515-2.txt) | -- |
| Duke 1.5/MENUES.C | [game/nt](logs-nt/4f8ad77bc29b-2.txt) | [game_oe/nt](logs-nt/300aed5c34e1-3.txt), [game_oe/dos](logs-dos/300aed5c34e1-1.txt) |
| Duke 1.5/PLAYER.C | [game/nt](logs-nt/7320b027974e-2.txt) | [game_oe/nt](logs-nt/3d898b2c0d7e-3.txt) |
| Duke 1.5/PREMAP.C | [game/nt](logs-nt/f04b0908ebbb-2.txt), [game_oe/nt](logs-nt/e2e34495967a-2.txt) | -- |
| Duke 1.5/RTS.C | [game/nt](logs-nt/3f13de712f1c-2.txt), [game_oe/nt](logs-nt/d47c7bbda11c-2.txt) | -- |
| Duke 1.5/SECTOR.C | [game/nt](logs-nt/2ddad82553e8-2.txt), [game_oe/nt](logs-nt/09955e4a74aa-2.txt), [game/dos](logs-dos/2ddad82553e8-1.txt), [game_oe/dos](logs-dos/09955e4a74aa-1.txt) | -- |
| Duke 1.5/SOUNDS.C | [game/nt](logs-nt/338fe35c0450-2.txt), [game_oe/nt](logs-nt/5c83cb4e2e40-2.txt) | -- |

## Reproduce

```powershell
python -B build/workers/xscan/tools_proposed/build_corpus.py --host nt
python -B build/workers/xscan/tools_proposed/build_corpus.py --host nt --retry
python -B build/workers/xscan/tools_proposed/build_corpus.py --host dos --replay-nt --probe BUIL0422 --probe buildold_12_30_94 --probe build_1995_03_28_partial --probe buil0723 --probe 19950920/POLYTEX5.C --probe /SRC_9505/ENGINE.C --probe /src/game/gamedef.c --probe /source/SECTOR.C
python -B build/workers/xscan/tools_proposed/build_corpus.py --host dos --replay-nt --timeouts-only --probe /source/MENUES.C
python -B build/workers/xscan/tools_proposed/build_corpus.py --baseline
python -B build/workers/xscan/tools_proposed/xscan.py
```
