"""Read-only adapters for the local historical binary corpus."""
from collections import defaultdict
from dataclasses import dataclass, field
from datetime import datetime, timezone
import re
import struct

from le import LE, Fixup
import omf


def u16(data, offset):
    return struct.unpack_from("<H", data, offset)[0]


def u32(data, offset):
    return struct.unpack_from("<I", data, offset)[0]


def timestamp(value):
    if value:
        return datetime.fromtimestamp(value, timezone.utc).isoformat()
    return None


def identify(data, suffix=""):
    if data[:2] in (b"MZ", b"ZM"):
        offset = u32(data, 60) if len(data) >= 64 else len(data)
        signature = data[offset:offset + 4]
        if signature == b"PE\0\0":
            return "PE"
        if signature[:2] in (b"LE", b"LX", b"NE") and offset >= 64:
            return signature[:2].decode("ascii")
        return "MZ"
    if data[:2] in (b"LE", b"LX") and data[2:4] == b"\0\0":
        return data[:2].decode("ascii")
    if data.startswith(b"!<arch>\n"):
        return "COFF/ar archive"
    if data[:4] in (b"PK\x03\x04", b"PK\x05\x06", b"PK\x07\x08"):
        return "ZIP"
    if data.startswith(b"7z\xbc\xaf\x27\x1c"):
        return "7z"
    if data.startswith(b"Rar!"):
        return "RAR"
    if data[:2] == b"WA" and suffix in {".a", ".b", ".c"}:
        return "Watcom patch payload"
    if len(data) >= 8 and data[0] in (0x80, 0x82):
        length = u16(data, 1)
        if 2 <= length <= len(data) - 3 and length == data[3] + 2:
            return "OMF object"
    if len(data) >= 16 and data[0] == 0xf0:
        page, directory = u16(data, 1) + 3, u32(data, 3)
        if 16 <= page < directory <= len(data) and page & (page - 1) == 0:
            return "OMF library"
    if len(data) >= 40 and data[:4] == b"\x7fELF":
        return "ELF"
    if len(data) >= 20 and u16(data, 0) in (0x14c, 0x8664, 0x1c0, 0x1c4, 0xaa64):
        count, symbols, opt = u16(data, 2), u32(data, 8), u16(data, 16)
        if 0 < count <= 256 and 20 + opt + 40 * count <= len(data) and (not symbols or symbols < len(data)):
            return "COFF"
    if suffix in {".exe", ".obj", ".lib", ".com", ".dll", ".sys", ".ovl", ".ocx", ".o", ".drv",
                  ".386", ".vxd", ".coff", ".bin"}:
        return "raw DOS COM" if suffix == ".com" else "unknown code artefact"
    return None


MARKERS = {
    "Watcom runtime": rb"watcom c/c\+\+(?:32)? run.time|watcom c run.time",
    "Watcom compiler/tool": rb"watcom (?:c/c\+\+|c\+\+|c32|c compiler|linker|assembler|wlink)|wcc386|open watcom",
    "Watcom linker hint": rb"wlink|wstub|be[g]text",
    "CLIB/MATH library name": rb"clib[36][a-z0-9]*|math387[a-z0-9]*|emu387[a-z0-9]*",
    "Borland/Turbo": rb"borland|turbo (?:assembler|c\+\+|c |pascal|link)",
    "Microsoft/MSC": rb"microsoft \(r\).*?(?:linker|macro assembler|optimizing compiler)|microsoft c(?:/c\+\+| run.time)|mscvrt|msvcrt|msc v",
    "MetaWare": rb"metaware|high c|dosomf",
    "Apogee": rb"apogee sound system",
    "Packed image marker": rb"l[z]exe|pklite|exepack|packed file is corrupt|upx[!0-9]",
}
EXTENDERS = {
    "DOS/4GW": rb"dos/4gw|dos4gw(?:\.exe)?",
    "DOS/4G": rb"dos/4g(?!w)",
    "DOS/32A": rb"dos/32a|dos32a",
    "Phar Lap": rb"phar lap|tnt dos.extender|386\|dos.extender",
    "PMODE/W": rb"pmode/w|pmodew",
    "CauseWay": rb"causeway dos extender|causeway (?:v|dos)",
    "CWSDPMI": rb"cwsdpmi",
}
MARKER_RE = re.compile(b"|".join(b"(?P<m%d>%s)" % (i, p)
                                for i, p in enumerate([*MARKERS.values(), *EXTENDERS.values()])), re.I)
DATE_RE = re.compile(
    rb"\b(?:19[789]\d|20[0-2]\d)[-/](?:0?[1-9]|1[0-2])[-/](?:0?[1-9]|[12]\d|3[01])\b|"
    rb"\b(?:jan|feb|mar|apr|may|jun|jul|aug|sep|oct|nov|dec)[a-z]*[ .-]+[ 0-3]?\d[, .-]+(?:19|20)\d\d\b|"
    rb"\b(?:0?[1-9]|1[0-2])[-/](?:0?[1-9]|[12]\d|3[01])[-/](?:(?:19|20)\d\d|[789]\d)\b", re.I)
VERSION_RE = re.compile(rb"\b(?:version|ver\.)\s*\d+(?:\.\d+){1,3}[a-z]?\b", re.I)
PRINTABLE = re.compile(rb"[ -~]{5,}")


def fingerprints(data, path):
    markers, extenders = defaultdict(list), defaultdict(list)
    dates, versions = set(), set()
    labels = [*MARKERS, *EXTENDERS]
    for span in PRINTABLE.finditer(data):
        raw = span.group()
        for hit in MARKER_RE.finditer(raw):
            label = labels[int(hit.lastgroup[1:])]
            dest = extenders if label in EXTENDERS else markers
            evidence = raw[max(0, hit.start() - 25):hit.start() + 105].decode("ascii")
            if evidence not in dest[label] and len(dest[label]) < 12:
                dest[label].append(evidence)
        dates.update(hit.group().decode("ascii") for hit in DATE_RE.finditer(raw))
        versions.update(hit.group().decode("ascii") for hit in VERSION_RE.finditer(raw))
    if re.fullmatch(r"(?:clib|math|emu)\w+\.(?:lib|bak)", path.split("/")[-1], re.I):
        markers["CLIB/MATH library name"].append(path.split("/")[-1])
    return {"toolchain_evidence": dict(markers), "extender_evidence": dict(extenders),
            "embedded_dates": sorted(dates), "embedded_versions": sorted(versions)}


@dataclass(frozen=True)
class Reloc:
    width: int
    kind: str
    target: tuple | None = None
    name: str | None = None
    addend: int = 0


@dataclass
class Region:
    index: int
    name: str
    data: bytes
    bits: int
    executable: bool
    base: int = 0
    virtual_size: int = 0
    coverage: bytes | None = None
    publics: dict = field(default_factory=lambda: defaultdict(list))
    relocs: dict = field(default_factory=dict)


@dataclass
class Program:
    module: str
    member_offset: int | None
    regions: dict
    entry: tuple | None = None
    image: bool = False
    metadata: dict = field(default_factory=dict)
    symbols: dict = field(default_factory=dict)

    def va(self, address):
        for reg in self.regions.values():
            if reg.base <= address < reg.base + max(reg.virtual_size, len(reg.data)):
                return reg.index, address - reg.base
        return None


class LinearExecutable(LE):
    """Reuse the checked repository LE reader for standard LE. Extend page/fixup handling here."""

    def __init__(self, data, kind):
        self.kind = kind
        if kind == "LE" and data.startswith(b"MZ"):
            super().__init__(data)
            return
        # Table offsets remain header-relative. The base reader recognizes the LE
        # signature and expects an MZ header. Use a synthetic wrapper only for parsing.
        if data[:2] in (b"LE", b"LX"):
            stub = bytearray(64)
            stub[:2] = b"MZ"
            struct.pack_into("<I", stub, 60, 64)
            data = stub + data
            for offset in (0x80, 0x88, 0x98):
                value = u32(data, 64 + offset)
                if value:
                    struct.pack_into("<I", data, 64 + offset, value + 64)
        header = u32(data, 60)
        if kind == "LX":
            # The base reader walks 4-byte entries before calling our page loader.
            data = data[:header] + b"LE" + data[header + 2:]
        super().__init__(data)

    def _load_pages(self):
        data, h, hdr = self.file, self.h, self.hdr
        size = hdr["page_size"]
        if data[h + 2:h + 4] != b"\0\0" or not 0 < size <= 65536 or hdr["obj_count"] > 4096:
            raise ValueError("unsupported linear image byte order or page layout")
        if sum(obj.vsize for obj in self.objects) > 512 * 1024**2:
            raise ValueError("linear image exceeds address-space limit")
        for obj in self.objects:
            image = bytearray()
            for k in range(obj.page_count):
                page = obj.page_index + k
                if self.kind == "LX":
                    loc = h + hdr["page_map"] + (page - 1) * 8
                    number, length, flags = struct.unpack_from("<IHH", data, loc)
                    offset = hdr["data_pages"] + (number << hdr["last_page_size"])
                    length = length or size
                else:
                    number, flags = self.page_map[page - 1]
                    length = (hdr["last_page_size"] or size) if number == hdr["pages"] else size
                    offset = hdr["data_pages"] + (number - 1) * size
                if flags in (2, 3) or (not number and self.kind == "LE"):
                    chunk = b""
                elif flags == 0 and offset >= 0 and offset + length <= len(data):
                    chunk = data[offset:offset + length]
                else:
                    raise ValueError(f"linear page {page}: unsupported flags or range ({flags:#x})")
                image.extend((chunk + bytes(size - len(chunk))) if len(chunk) <= size else chunk[:size])
            obj.data = image[:obj.vsize]

    def _load_fixups(self):
        # Internal, import ordinal/name and internal-entry records, including additive
        # and source lists. Internal chaining is surfaced as unsupported evidence.
        data, h, hdr = self.file, self.h, self.hdr
        self.fixups, self.import_fixups = [], []
        if not hdr["fixup_section_size"] or not hdr["fixup_page_table"]:
            return
        self.fixup_page_table = [u32(data, h + hdr["fixup_page_table"] + 4 * i)
                                 for i in range(hdr["pages"] + 1)]
        imports = [None]
        pos = h + hdr["import_modules"]
        for _ in range(hdr["import_module_count"]):
            length = data[pos]
            imports.append(data[pos + 1:pos + 1 + length].decode("latin-1"))
            pos += length + 1
        record_base = h + hdr["fixup_record_table"]
        for page in range(hdr["pages"]):
            pos = record_base + self.fixup_page_table[page]
            end = record_base + self.fixup_page_table[page + 1]
            if pos == end:
                continue
            obj, objoff = self.page_object(page + 1)
            while pos < end:
                start = pos
                source, flags = data[pos:pos + 2]
                pos += 2
                if flags & 8:
                    raise ValueError("linear image uses internal chaining")
                if source & 32:
                    count = data[pos]
                    pos += 1
                    offsets = None
                else:
                    count = 1
                    offsets = [struct.unpack_from("<h", data, pos)[0]]
                    pos += 2
                typ = flags & 3
                index_width = 2 if flags & 64 else 1
                index = int.from_bytes(data[pos:pos + index_width], "little")
                pos += index_width
                target_width = 4 if flags & 16 else 2
                if typ == 3 or (typ == 0 and source & 15 == 2):
                    target_width = 0
                elif typ == 1 and flags & 128:
                    target_width = 1
                target = int.from_bytes(data[pos:pos + target_width], "little")
                pos += target_width
                if flags & 4:
                    width = 4 if flags & 32 else 2
                    target += int.from_bytes(data[pos:pos + width], "little")
                    pos += width
                if offsets is None:
                    offsets = struct.unpack_from(f"<{count}h", data, pos)
                    pos += count * 2
                for offset in offsets:
                    if typ == 0:
                        self.fixups.append(Fixup(page + 1, source, flags, offset, index, target,
                                                 obj.index, objoff + offset, data[start:pos]))
                    else:
                        label = f"entry#{index}"
                        if typ in (1, 2):
                            mod = imports[index] if 0 < index < len(imports) else f"import#{index}"
                            name = f"#{target}"
                            if typ == 2:
                                where = h + hdr["import_procs"] + target
                                name = data[where + 1:where + 1 + data[where]].decode("latin-1")
                            label = f"{mod}!{name}"
                        self.import_fixups.append((obj.index, objoff + offset, source & 15, label))
            if pos != end:
                raise ValueError(f"linear fixup table overrun at page {page + 1}")


def linear_exports(image):
    data, hdr, header = image.file, image.hdr, image.h
    start = hdr["entry_table"]
    if not start:
        return {}
    entries, ordinal = {}, 1
    pos = header + start
    while pos < len(data) and data[pos]:
        count, typ = data[pos:pos + 2]
        pos += 2
        if not typ:
            ordinal += count
            continue
        width = {1: 2, 2: 2, 3: 4, 4: 0}.get(typ)
        if width is None:
            break
        index = u16(data, pos)
        pos += 2
        for _ in range(count):
            if width:
                offset = int.from_bytes(data[pos + 1:pos + 1 + width], "little")
                entries[ordinal] = index, offset
            pos += {1: 3, 2: 5, 3: 5, 4: 7}[typ]
            ordinal += 1
    return entries


def linear_program(data, kind):
    image = LinearExecutable(data, kind)
    try:
        names = image.resident_names() if image.hdr["resident_names"] else []
    except (IndexError, struct.error):
        names = []
    regions = {obj.index: Region(obj.index, f"obj{obj.index}", bytes(obj.data), 32 if obj.big else 16,
                                 obj.is_code, obj.base, obj.vsize) for obj in image.objects}
    exports = {}
    try:
        exports = linear_exports(image)
        pos = image.hdr["nonres_names"]
        limit = min(len(data), pos + image.hdr["nonres_names_size"])
        all_names = list(names)
        while 0 < pos < limit and image.file[pos]:
            length = image.file[pos]
            name = image.file[pos + 1:pos + 1 + length].decode("latin-1")
            all_names.append((name, u16(image.file, pos + 1 + length)))
            pos += length + 3
        for name, ordinal in all_names:
            if ordinal in exports:
                idx, offset = exports[ordinal]
                if idx in regions:
                    regions[idx].publics[offset].append(name)
    except (ValueError, IndexError, struct.error):
        pass
    for (src, offset), fx in image.fixup_map().items():
        regions[src].relocs[offset] = Reloc(fx.size, "relative" if fx.src_type & 15 == 8 else "absolute",
                                             (fx.target_obj, fx.target_off))
    for src, offset, typ, name in image.import_fixups:
        addr = exports.get(int(name[6:])) if name.startswith("entry#") else None
        regions[src].relocs[offset] = Reloc(Fixup.SIZES.get(typ, 0), "relative" if typ == 8 else "absolute",
                                             addr, name=name)
    return Program(names[0][0] if names else "", None, regions, image.entry(), True,
                   {"header_offset": image.h, "resident_names": names, "module_version": image.hdr["module_version"],
                    "object_count": len(regions), "fixups": len(image.fixups) + len(image.import_fixups)})


def omf_program(module, member_offset):
    regions, symbols = {}, {}
    for index, seg in enumerate(module.segments[1:], 1):
        executable = seg.cls.upper() in {"CODE", "TEXT"} or seg.name.upper().endswith("_TEXT")
        regions[index] = Region(index, seg.name, bytes(seg.data), 32 if seg.use32 else 16,
                                executable, virtual_size=seg.size, coverage=bytes(seg.covered))
    for name, seg, offset, _ in module.publics:
        if seg in regions:
            regions[seg].publics[offset].append(name)
            symbols[name] = (seg, offset)
    for fix in module.fixups:
        region = regions[fix.seg]
        width = fix.size()
        method, index = fix.target
        method &= 3
        value = 0
        if width and fix.loc not in (0, 2, 4):
            offset_width = width if fix.loc not in (3, 11) else width - 2
            value = int.from_bytes(region.data[fix.off:fix.off + offset_width], "little", signed=True)
        addend = fix.disp + value
        target = (index, addend) if method == 0 else None
        name = module.externs[index] if method == 2 and 0 < index < len(module.externs) else None
        if name in symbols:
            seg, off = symbols[name]
            target = (seg, off + addend)
        region.relocs[fix.off] = Reloc(width, "relative" if fix.self_rel else "absolute", target, name, addend)
    metadata = {"name": module.name, "offset": member_offset, "dialect": "Easy OMF" if module.easy else module.dialect,
                "comments": module.comments, "unsupported_records": {f"{k:02x}": v for k, v in module.unsupported.items()},
                "fixups": len(module.fixups), "publics": len(module.publics)}
    return Program(module.name or "", member_offset, regions, metadata=metadata, symbols=symbols)


def easy_lidata16(data, start):
    """Some Watcom patch members use 32-bit Easy OMF offsets and 16-bit A2 repeat counts."""
    pos, easy, iterated = start, False, set()
    while pos + 3 <= len(data):
        kind, length = data[pos], u16(data, pos + 1)
        end = pos + 3 + length
        if not length or end > len(data):
            raise ValueError("invalid record boundary in Easy OMF member")
        if kind == 0x88 and data[pos + 4:pos + 5] == b"\xaa":
            easy = True
        if kind in (0xa2, 0xa3):
            iterated.add(kind)
        if kind in (0x8a, 0x8b):
            break
        pos = end
    if not easy or iterated != {0xa2}:
        raise ValueError("no Easy OMF A2-only member")

    def expand(reader):
        repeats, branches = reader.u16(), reader.u16()
        if branches:
            value = b"".join(expand(reader) for _ in range(branches))
        else:
            length = reader.u8()
            if reader.p + length > len(reader.b):
                raise ValueError("truncated Easy OMF iterated data")
            value = reader.b[reader.p:reader.p + length]
            reader.p += length
        if repeats * len(value) > 32 * 1024**2:
            raise ValueError("oversized Easy OMF iterated data")
        return value * repeats

    old = omf._expand_lidata
    omf._expand_lidata = expand
    try:
        module, end = omf.parse_module(data, start)
    finally:
        omf._expand_lidata = old
    if any(offset + count > module.segments[seg].size for seg, offset, count in module.chunks):
        raise ValueError("Easy OMF record extends beyond its segment")
    return module, end


def omf_members(data, library):
    """Isolate member failures at OMF library page boundaries."""
    pos = u16(data, 1) + 3 if library else 0
    page = pos
    end = u32(data, 3) if library else len(data)
    while pos < end and data[pos] not in (0, 0xf1):
        start = pos
        try:
            module, pos = omf.parse_module(data, start)
            yield omf_program(module, start), None
        except (ValueError, IndexError, KeyError, TypeError, struct.error, RecursionError) as exc:
            try:
                module, pos = easy_lidata16(data, start)
                program = omf_program(module, start)
                program.metadata["parser_extension"] = "Easy OMF A2 with 16-bit iteration counts"
                yield program, None
            except (ValueError, IndexError, KeyError, TypeError, struct.error, RecursionError):
                yield None, f"OMF member at {start:#x}: {type(exc).__name__}: {exc}"
                if not library:
                    return
                pos = start + page
                while pos < end:
                    if data[pos] == 0xf1 or (data[pos] in (0x80, 0x82) and
                                             pos + 3 < end and u16(data, pos + 1) == data[pos + 3] + 2):
                        break
                    pos += page
        if library:
            pos = (pos + page - 1) // page * page


def coff_program(data, kind, member="", member_offset=None):
    pe = kind == "PE"
    header = u32(data, 60) + 4 if pe else 0
    machine, count, time, table, symbols, opts, _ = struct.unpack_from("<HHIIIHH", data, header)
    if machine not in (0x14c, 0x8664):
        raise ValueError(f"non-x86 COFF machine {machine:#x}")
    bits = 64 if machine == 0x8664 else 32
    opt = header + 20
    base = (struct.unpack_from("<Q", data, opt + 24)[0] if bits == 64 else u32(data, opt + 28)) if pe else 0
    regions, relocation_tables = {}, {}
    for i in range(1, count + 1):
        at = opt + opts + 40 * (i - 1)
        name, vs, va, size, ptr, rel, _, nrel, _, flags = struct.unpack_from("<8s6I2HI", data, at)
        name = name.split(b"\0")[0].decode("latin-1")
        code = bool(flags & 0x20 or flags & 0x20000000)
        backing = data[ptr:ptr + size] if ptr else b""
        if pe and vs:
            backing = backing[:vs]
        regions[i] = Region(i, name, backing, bits, code, base + va, vs or size)
        relocation_tables[i] = rel, nrel
    prog = Program(member, member_offset, regions, image=pe,
                   metadata={"machine": hex(machine), "coff_timestamp": timestamp(time), "offset": member_offset})
    if pe:
        prog.entry = prog.va(base + u32(data, opt + 16))
        prog.metadata["linker_header_version"] = f"{data[opt + 2]}.{data[opt + 3]}"
    string_table = table + 18 * symbols
    symbol_refs = {}
    i = 0
    while table and i < symbols:
        where = table + i * 18
        raw, value, sec, typ, cls, aux = struct.unpack_from("<8sIhHBB", data, where)
        if raw[:4] == b"\0" * 4:
            start = string_table + u32(raw, 4)
            raw = data[start:data.find(b"\0", start)]
        name = raw.split(b"\0")[0].decode("latin-1")
        if sec in regions:
            if name and cls in (2, 3, 101) and (typ & 32 or not regions[sec].executable):
                regions[sec].publics[value].append(name)
            symbol_refs[i] = (sec, value), name
        else:
            symbol_refs[i] = None, name
        i += 1 + aux
    for sec, (start, count) in relocation_tables.items():
        reg = regions[sec]
        for i in range(count):
            where, symbol, typ = struct.unpack_from("<IIH", data, start + i * 10)
            widths = {1: 8, 2: 4, 3: 4, 4: 4, 5: 4, 6: 4, 7: 4, 8: 4, 9: 4, 10: 2, 11: 4} if bits == 64 else {
                1: 2, 2: 2, 6: 4, 7: 4, 10: 2, 11: 4, 20: 4}
            relative = typ in (4, 5, 6, 7, 8, 9) if bits == 64 else typ in (2, 20)
            target, name = symbol_refs.get(symbol, (None, None))
            width = widths.get(typ, 0)
            addend = int.from_bytes(reg.data[where:where + width], "little", signed=True)
            if target:
                target = target[0], target[1] + addend
            reg.relocs[where] = Reloc(width, "relative" if relative else "absolute", target, name, addend)
    if pe:
        pe_directories(data, prog, opt, bits, base, opts)
    return prog


def pe_directories(data, prog, opt, bits, base, opts):
    directory = opt + (112 if bits == 64 else 96)

    def read(rva, length):
        address = prog.va(base + rva)
        if not address:
            return b""
        idx, offset = address
        return prog.regions[idx].data[offset:offset + length]

    def table(index):
        if directory + index * 8 + 8 <= opt + opts:
            return struct.unpack_from("<II", data, directory + index * 8)
        return 0, 0

    rva, size = table(5)
    reloc_data = read(rva, size)
    offset = 0
    while offset + 8 <= len(reloc_data):
        page, length = struct.unpack_from("<II", reloc_data, offset)
        if not 8 <= length <= len(reloc_data) - offset:
            break
        for i in range(offset + 8, offset + length - 1, 2):
            field = u16(reloc_data, i)
            width = {3: 4, 10: 8}.get(field >> 12)
            if width and (source := prog.va(base + page + (field & 4095))):
                region, where = source
                target = prog.va(int.from_bytes(prog.regions[region].data[where:where + width], "little"))
                prog.regions[region].relocs[where] = Reloc(width, "absolute", target)
        offset += length
    rva, size = table(0)
    exports = read(rva, 40) if size >= 40 else b""
    if len(exports) == 40:
        _, _, _, _, _, nfunc, nnames, funcs, names, ords = struct.unpack_from("<IIHHIIIIIII", exports)
        for i in range(min(nnames, 100_000)):
            where, ordinal = read(names + i * 4, 4), read(ords + i * 2, 2)
            if len(where) != 4 or len(ordinal) != 2 or u16(ordinal, 0) >= nfunc:
                break
            addr = read(funcs + u16(ordinal, 0) * 4, 4)
            if len(addr) != 4 or rva <= u32(addr, 0) < rva + size:
                continue
            address = prog.va(base + u32(addr, 0))
            if address:
                idx, offset = address
                name = read(u32(where, 0), 256).split(b"\0")[0].decode("latin-1")
                prog.regions[idx].publics[offset].append(name)
    if bits == 64:
        rva, size = table(3)
        pdata = read(rva, size)
        for offset in range(0, len(pdata) - 11, 12):
            begin, end, _ = struct.unpack_from("<III", pdata, offset)
            first, last = prog.va(base + begin), prog.va(base + end - 1)
            if first and last and first[0] == last[0]:
                prog.metadata.setdefault("unwind_extents", {})[first] = last[1] + 1


def ar_members(data):
    """SysV/GNU/BSD archive names and Unix member timestamps."""
    pos, strings = 8, b""
    while pos + 60 <= len(data):
        header = data[pos:pos + 60]
        if header[-2:] != b"`\n":
            raise ValueError(f"invalid ar member at {pos:#x}")
        length = int(header[48:58].strip())
        offset = pos + 60
        payload = data[offset:offset + length]
        if len(payload) != length:
            raise ValueError("truncated ar member")
        pos = offset + length + (length & 1)
        name = header[:16].decode("latin-1").rstrip()
        if name == "//":
            strings = payload
            continue
        if name in ("/", "/SYM64/"):
            continue
        if name.startswith("#1/"):
            n = int(name[3:])
            name, payload = payload[:n].rstrip(b"\0").decode("latin-1"), payload[n:]
        elif name.startswith("/") and name[1:].isdigit():
            name = strings[int(name[1:]):].split(b"\n", 1)[0].rstrip(b"/\0").decode("latin-1")
        else:
            name = name.rstrip("/")
        yield name, offset, timestamp(int(header[16:28].strip() or b"0")), payload
