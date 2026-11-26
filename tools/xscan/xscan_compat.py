"""Explicit compatibility inputs for incomplete source snapshots."""
import re

from build_corpus import DUKE, KEN, STUB, V13, V15, WORK

ARCH94 = KEN / "kenbuild-20241213/kenbuild-related-archives-20241213"
ARCH97 = KEN / "kenbuild-20260331/kenbuild-related-archives-20260331"
BUILD = KEN / "buildsrc-20190520/buildsrc-20190520"
DUKE_REF = DUKE / "reference/duke3d-reconstructions"

NEARBY_HEADERS = {
    "BUI1101B": [ARCH94 / "BUIL1109", ARCH94 / "BUIL1020"],
    "buil010294_BUILDOLD": [ARCH94 / "builold!"],
    "BUIL0615": [ARCH94 / "BUILD_FROM_MATT", ARCH94 / "BUIL0624"],
    "buil0723": [ARCH94 / "BUIL1020", ARCH94 / "BUILDBAK"],
    "BUIL0906": [ARCH94 / "BUILD_FROM_MATT"],
    "BUIL1109": [ARCH94 / "BUIL1020"],
    "BUIL1121": [ARCH94 / "BUIL1109", ARCH94 / "BUIL1020"],
    "BUIL1211": [ARCH94 / "BUILT", ARCH94 / "BUIL1109"],
    "buildold_12_30_94": [ARCH94 / "BUIL0801"],
    "buildold": [ARCH94 / "builold!"],
    "build_1995_03_28_partial": [ARCH94 / "buil1995"],
    "build_1996_11_12_partial": [BUILD / "19961013", ARCH94 / "buil062896", ARCH97 / "build_1997_07_19"],
    "dmbuild_1995_05_25": [ARCH94 / "buil1995"],
    "19950920": [BUILD / "SRC_9505", ARCH94 / "buil1995"],
    "19960606": [BUILD / "19961013"],
    "19961204": [BUILD / "19961013"],
}


ASM_COMPAT = [
    (r"dword ptr \[ebx\+ecx\*4-4\]", "dword ptr -4[ebx+ecx*4]"),
    (r"loop short beg", "loop beg"),
    (
        r'"mov cx, word ptr shlookup\[ebx\*2\+8192\]"',
        lambda _: '"lea ebx, 4096[ebx]",\\\n\t"mov cx, word ptr shlookup[ebx*2]"',
    ),
]


def adapted_copy(src, changes, prepend=""):
    relative = src.relative_to(KEN)
    destination = WORK / "compat-sources" / relative
    destination.parent.mkdir(parents=True, exist_ok=True)
    data = src.read_bytes().decode("latin-1")
    for pattern, replacement in changes:
        data = re.sub(pattern, replacement, data, flags=re.MULTILINE)
    destination.write_bytes((prepend + data).encode("latin-1"))
    return destination


def compatibility(src, label, inc):
    if label == "duke-1.3d-tree":
        return src, inc + [DUKE_REF, DUKE_REF / "MACT", V15 / "source", V15 / "audiolib/SOURCE"], []
    if label == "duke-1.5":
        return src, inc, []
    folder = src.parent.name
    includes = inc + NEARBY_HEADERS.get(folder, [])
    if folder in {"build_1996_11_12_partial", "build_1997_07_19"}:
        # 9.5 uses pack(n), rather than the later push/pop pragma syntax.
        overlay = WORK / "compat-sources" / src.relative_to(KEN).parent
        overlay.mkdir(parents=True, exist_ok=True)
        available = [src.parent, *includes]
        for name in ("build.h", "ves2.h"):
            orig = next((p / name for p in available if (p / name).exists()), None)
            if orig:
                data = orig.read_bytes()
                data = re.sub(rb"#pragma pack\(push,\s*1\)", b"#pragma pack(1)", data)
                data = re.sub(rb"#pragma pack\(pop\)", b"#pragma pack()", data)
                (overlay / name).write_bytes(data)
        prepend = ""
        if src.stem.lower() in {"snots", "mixtiles"}:
            prepend = '#include "build.h"\r\n'
        if src.stem.lower() == "simple":
            prepend = '#include "editkeys.h"\r\n'
            includes = [*includes, STUB / "ken"]
        changes = ASM_COMPAT if src.stem.lower() == "engine" else []
        return adapted_copy(src, changes, prepend), [src.parent, *includes], ["-d__inline="]
    if folder == "buildold_12_30_94":
        patch = [(r'(#include "names.h"[^\r\n]*)', r'\1\n#include "sep94.h"')]
        return adapted_copy(src, patch), [src.parent, *includes, STUB / "ken"], []
    if folder == "build_1995_03_28_partial":
        # The surviving file ends inside dragpoint. Keep its complete source prefix.
        patch = [
            (r"^dragpoint\(short pointhighlight, long dax, long day\)[\s\S]*", ""),
            (r"\bstruct (spritetype|walltype|sectortype)\b", r"\1"),
        ]
        return adapted_copy(src, patch, '#include "mar95.h"\r\n'), [src.parent, *includes, STUB / "ken"], []
    if folder in {"19950920", "19960606"} and src.stem.lower() == "build":
        return adapted_copy(src, [(r"\bstruct walltype\b", "walltype")], '#include "editkeys.h"\r\n'), [src.parent, *includes, STUB / "ken"], []
    if folder == "buildold" and src.stem.lower() == "game":
        return adapted_copy(src, [], "extern int brightenarea;\r\n"), [src.parent, *includes], []
    if folder in {"build_1993_04_14", "build_1993_06_22", "buildold", "BUILD1"}:
        return src, includes, ["-d_asm=__asm"]
    if folder == "buil010294_BUILDOLD" and (src.parent / "BUILD2.H").exists():
        fix = [(r'(?i)(#include\s+")build\.h(")', r'\1build2.h\2')]
        return adapted_copy(src, fix), [src.parent, *includes], []
    if folder == "BUILDBAK" and src.stem.lower() == "engine":
        return adapted_copy(src, ASM_COMPAT), [src.parent, *includes], []
    return src, includes, []
