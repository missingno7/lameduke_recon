"""Attach locally shipped Watcom linker maps whose entry and object ends agree."""
from datetime import datetime
from hashlib import sha256
from pathlib import Path
import re
import struct

from binary_formats import identify, u32


IMAGE = re.compile(r"^Executable Image:\s+([^\r\n]+)", re.M | re.I)
ENTRY = re.compile(r"^Entry point address:\s*([a-f\d]+):([a-f\d]+)", re.M | re.I)
SEGMENT = re.compile(r"^\S+[ \t]+\S+[ \t]+(?:\S+[ \t]+)?([a-f\d]{4}):([a-f\d]{8})[ \t]+([a-f\d]{8})[ \t]*$", re.I)
PUBLIC = re.compile(r"^([a-f\d]{4}):([a-f\d]{8})[+* ]+[ \t]+(\S+)", re.I)
VERSION = re.compile(r"^WATCOM Linker Version\s+([\d.]+)", re.I)
CREATED = re.compile(r"^Created on:\s+(\d\d/\d\d/\d\d \d\d:\d\d:\d\d)", re.M | re.I)


def map_index(paths, roots, display):
    layouts, rejected = {}, []
    for path in paths:
        if path.suffix.casefold() != ".map":
            continue
        try:
            with path.open("rb") as inp:
                first = inp.read(128)
                if not VERSION.match(first.decode("latin-1")):
                    continue
                raw = first + inp.read()
        except OSError:
            continue
        content = raw.decode("latin-1")
        image, entry = IMAGE.search(content), ENTRY.search(content)
        if not image or not entry:
            continue
        prefix, _, suffix = content.partition("Memory Map")
        ends, names = {}, {}
        for line in prefix.splitlines():
            if match := SEGMENT.match(line):
                obj, offset, size = (int(n, 16) for n in match.groups())
                ends[obj] = max(ends.get(obj, 0), offset + size)
        for line in suffix.splitlines():
            if match := PUBLIC.match(line):
                obj, offset = (int(n, 16) for n in match.groups()[:2])
                names.setdefault((obj, offset), []).append(match[3])
        if not (ends and names):
            continue
        declared = Path(image[1].strip().replace("\\", "/"))
        candidates = {(path.parent / declared).resolve(), path.with_suffix(".EXE").resolve()}
        status = "no matching nearby LE/LX image"
        for exe in sorted(candidates):
            if not any(exe.is_relative_to(root) for root in roots):
                continue
            try:
                binary = exe.read_bytes()
                if identify(binary, exe.suffix.casefold()) not in {"LE", "LX"}:
                    continue
                header = u32(binary, 60) if binary.startswith(b"MZ") else 0
                obj_table = header + u32(binary, header + 0x40)
                sizes = {i + 1: u32(binary, obj_table + 24 * i)
                         for i in range(u32(binary, header + 0x44))}
                expected_entry = u32(binary, header + 0x18), u32(binary, header + 0x1c)
            except (OSError, struct.error, OverflowError):
                continue
            if ends != sizes or expected_entry != tuple(int(n, 16) for n in entry.groups()):
                status = "entry or object extents disagree"
                continue
            date = CREATED.search(content)
            metadata = {"path": display(path), "sha256": sha256(raw).hexdigest(),
                        "linker_version": VERSION.match(content)[1],
                        "created": datetime.strptime(date[1], "%y/%m/%d %H:%M:%S").isoformat() if date else None,
                        "public_count": sum(map(len, names.values())),
                        "basis": "matching entry point and every LE/LX object extent"}
            layouts[sha256(binary).hexdigest()] = metadata, names
            break
        else:
            rejected.append({"path": display(path), "reason": status})
    return layouts, rejected


def apply_symbols(program, names):
    for (obj, offset), aliases in names.items():
        region = program.regions.get(obj)
        if region and 0 <= offset < region.virtual_size:
            for name in aliases:
                if name not in region.publics[offset]:
                    region.publics[offset].append(name)
