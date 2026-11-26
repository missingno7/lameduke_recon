"""Pure-Python reader for MZ+LE (DOS/4GW) executables.

    python tools/le.py [EXE]          # print a census of the container

Coordinates: every address is canonical `obj:offset` (1-based LE object, offset inside the object)
or a flat linear address `object.base + offset`.  All LE table offsets are relative to the LE
header except the data-pages offset (+0x80), which is relative to the start of the file.

The parser accounts for every byte of the file (see `regions()`), so a census can show that no
region is silently ignored.
"""
import struct
import sys
from dataclasses import dataclass, field
from pathlib import Path


@dataclass
class Obj:
    index: int            # 1-based
    vsize: int
    base: int
    flags: int
    page_index: int       # 1-based first page-map entry
    page_count: int
    data: bytearray = field(default_factory=bytearray)   # vsize bytes, zero-filled past the file data

    @property
    def is_code(self):
        return bool(self.flags & 0x4)

    @property
    def big(self):
        return bool(self.flags & 0x2000)


@dataclass
class Fixup:
    page: int             # 1-based logical page
    src_type: int         # low nibble of source byte
    flags: int
    src_off: int          # offset within the page (may be negative for page-crossing fixups)
    target_obj: int       # 1-based, internal references only
    target_off: int
    obj: int = 0          # object containing the source
    obj_off: int = 0      # source offset inside the object
    raw: bytes = b""

    SIZES = {0x0: 1, 0x2: 2, 0x3: 4, 0x5: 2, 0x6: 6, 0x7: 4, 0x8: 4}

    @property
    def size(self):
        return self.SIZES.get(self.src_type & 0xF, 0)


class LE:
    def __init__(self, path_or_bytes):
        if isinstance(path_or_bytes, (bytes, bytearray)):
            self.file = bytes(path_or_bytes)
        else:
            self.file = Path(path_or_bytes).read_bytes()
        f = self.file
        if f[:2] != b"MZ":
            raise ValueError("not an MZ executable")
        self.mz = dict(zip(
            ["cblp", "cp", "crlc", "cparhdr", "minalloc", "maxalloc", "ss", "sp", "csum", "ip", "cs", "lfarlc", "ovno"],
            struct.unpack_from("<13H", f, 2)))
        self.le_off = struct.unpack_from("<I", f, 0x3C)[0]
        h = self.le_off
        if f[h:h + 2] != b"LE":
            raise ValueError(f"no LE signature at {h:#x}")
        self.h = h
        u = lambda o: struct.unpack_from("<I", f, h + o)[0]
        self.hdr = {
            "byte_order": f[h + 2], "word_order": f[h + 3], "format_level": u(4),
            "cpu": struct.unpack_from("<H", f, h + 8)[0], "os": struct.unpack_from("<H", f, h + 10)[0],
            "module_version": u(0x0C), "module_flags": u(0x10), "pages": u(0x14),
            "eip_obj": u(0x18), "eip": u(0x1C), "esp_obj": u(0x20), "esp": u(0x24),
            "page_size": u(0x28), "last_page_size": u(0x2C), "fixup_section_size": u(0x30),
            "fixup_section_checksum": u(0x34), "loader_section_size": u(0x38), "loader_section_checksum": u(0x3C),
            "obj_table": u(0x40), "obj_count": u(0x44), "page_map": u(0x48), "iter_pages": u(0x4C),
            "resource_table": u(0x50), "resource_count": u(0x54), "resident_names": u(0x58),
            "entry_table": u(0x5C), "module_directives": u(0x60), "module_directives_count": u(0x64),
            "fixup_page_table": u(0x68), "fixup_record_table": u(0x6C), "import_modules": u(0x70),
            "import_module_count": u(0x74), "import_procs": u(0x78), "per_page_checksum": u(0x7C),
            "data_pages": u(0x80), "preload_pages": u(0x84), "nonres_names": u(0x88),
            "nonres_names_size": u(0x8C), "nonres_names_checksum": u(0x90), "auto_ds_obj": u(0x94),
            "debug_info": u(0x98), "debug_info_size": u(0x9C), "instance_preload": u(0xA0),
            "instance_demand": u(0xA4), "heap_size": u(0xA8), "stack_size": u(0xAC),
        }
        H = self.hdr
        self.objects = []
        for i in range(H["obj_count"]):
            o = h + H["obj_table"] + 24 * i
            vsize, base, flags, pidx, pcnt, _res = struct.unpack_from("<6I", f, o)
            self.objects.append(Obj(i + 1, vsize, base, flags, pidx, pcnt))
        # page map: LE uses 4-byte entries (24-bit page number big-endian-ish + flags)
        self.page_map = []
        for i in range(H["pages"]):
            o = h + H["page_map"] + 4 * i
            b = f[o:o + 4]
            num = (b[0] << 16) | (b[1] << 8) | b[2]
            self.page_map.append((num, b[3]))
        self._load_pages()
        self._load_fixups()

    # -- pages -------------------------------------------------------------------------------
    def page_file_range(self, logical_page):
        """File (offset, length) of a 1-based logical page."""
        H = self.hdr
        num, flags = self.page_map[logical_page - 1]
        if flags != 0:
            raise ValueError(f"page {logical_page}: unsupported page flags {flags:#x}")
        length = H["page_size"] if num != H["pages"] else H["last_page_size"]
        return H["data_pages"] + (num - 1) * H["page_size"], length

    def page_object(self, logical_page):
        for o in self.objects:
            if o.page_index <= logical_page < o.page_index + o.page_count:
                return o, (logical_page - o.page_index) * self.hdr["page_size"]
        raise ValueError(f"page {logical_page} belongs to no object")

    def _load_pages(self):
        for o in self.objects:
            o.data = bytearray(o.vsize)
            for k in range(o.page_count):
                off, ln = self.page_file_range(o.page_index + k)
                pos = k * self.hdr["page_size"]
                chunk = self.file[off:off + ln]
                n = min(len(chunk), max(0, o.vsize - pos))
                o.data[pos:pos + n] = chunk[:n]
                if len(chunk) > n and any(chunk[n:]):
                    raise ValueError(f"object {o.index}: nonzero page bytes past vsize")

    # -- fixups ------------------------------------------------------------------------------
    def _load_fixups(self):
        f, H, h = self.file, self.hdr, self.h
        fpt = [struct.unpack_from("<I", f, h + H["fixup_page_table"] + 4 * i)[0] for i in range(H["pages"] + 1)]
        self.fixup_page_table = fpt
        rec_base = h + H["fixup_record_table"]
        self.fixups = []
        for p in range(H["pages"]):
            pos, end = rec_base + fpt[p], rec_base + fpt[p + 1]
            obj, page_off = self.page_object(p + 1)
            while pos < end:
                start = pos
                src, flg = f[pos], f[pos + 1]
                pos += 2
                if src & 0x20:
                    count = f[pos]; pos += 1
                    offs = None
                else:
                    count = 1
                    offs = [struct.unpack_from("<h", f, pos)[0]]
                    pos += 2
                tt = flg & 3
                if tt != 0:
                    raise ValueError(f"page {p+1}: non-internal fixup target type {tt} at {start:#x}")
                if flg & 0x40:
                    tobj = struct.unpack_from("<H", f, pos)[0]; pos += 2
                else:
                    tobj = f[pos]; pos += 1
                if (src & 0xF) == 0x2:     # 16-bit selector fixup: no offset field
                    toff = 0
                elif flg & 0x10:
                    toff = struct.unpack_from("<I", f, pos)[0]; pos += 4
                else:
                    toff = struct.unpack_from("<H", f, pos)[0]; pos += 2
                if flg & 0x04:
                    raise ValueError("additive fixups unsupported")
                if src & 0x20:
                    offs = [struct.unpack_from("<h", f, pos + 2 * i)[0] for i in range(count)]
                    pos += 2 * count
                raw = f[start:pos]
                for so in offs:
                    self.fixups.append(Fixup(p + 1, src, flg, so, tobj, toff, obj.index, page_off + so, raw))
            if pos != end:
                raise ValueError(f"page {p+1}: fixup records overran ({pos:#x} != {end:#x})")

    # -- convenience -------------------------------------------------------------------------
    def obj(self, index):
        return self.objects[index - 1]

    def linear(self, obj_index, off):
        return self.obj(obj_index).base + off

    def to_objoff(self, linear):
        for o in self.objects:
            if o.base <= linear < o.base + max(o.vsize, 1):
                return o.index, linear - o.base
        return None

    def fixup_map(self):
        """{(obj, off): Fixup} keyed by source location; asserts no duplicate source sites."""
        m = {}
        for fx in self.fixups:
            k = (fx.obj, fx.obj_off)
            if k in m and (m[k].target_obj, m[k].target_off, m[k].src_type) != (fx.target_obj, fx.target_off, fx.src_type):
                raise ValueError(f"conflicting fixups at {k}")
            m[k] = fx
        return m

    def entry(self):
        return self.hdr["eip_obj"], self.hdr["eip"]

    def regions(self):
        """Ordered (start, end, label) file regions covering the whole file."""
        H, h, f = self.hdr, self.h, self.file
        r = []
        mz_hdr = self.mz["cparhdr"] * 16
        mz_img_end = (self.mz["cp"] - 1) * 512 + (self.mz["cblp"] or 512) if self.mz["cp"] else 0
        r.append((0, mz_hdr, "MZ header"))
        r.append((mz_hdr, min(mz_img_end, h), "MZ stub image"))
        if mz_img_end < h:
            r.append((mz_img_end, h, "gap MZ->LE"))
        loader_end = h + H["fixup_page_table"]
        r.append((h, loader_end, "LE header + loader section"))
        fix_end = h + H["fixup_record_table"] + self.fixup_page_table[-1]
        r.append((loader_end, fix_end, "fixup section"))
        if fix_end < H["data_pages"]:
            r.append((fix_end, H["data_pages"], "gap fixups->pages (import tables etc.)"))
        last = max(n for n, _ in self.page_map)
        pages_end = H["data_pages"] + (last - 1) * H["page_size"] + H["last_page_size"]
        r.append((H["data_pages"], pages_end, "data pages"))
        if H["nonres_names"]:
            r.append((pages_end, H["nonres_names"], "gap pages->nonres names"))
            r.append((H["nonres_names"], H["nonres_names"] + H["nonres_names_size"], "nonresident names"))
            pages_end = H["nonres_names"] + H["nonres_names_size"]
        if H["debug_info"]:
            r.append((H["debug_info"], H["debug_info"] + H["debug_info_size"], "debug info"))
            pages_end = max(pages_end, H["debug_info"] + H["debug_info_size"])
        if pages_end < len(f):
            r.append((pages_end, len(f), "overlay / tail"))
        return r

    def resident_names(self):
        H, h, f = self.hdr, self.h, self.file
        out, pos = [], h + H["resident_names"]
        while f[pos]:
            n = f[pos]
            out.append((f[pos + 1:pos + 1 + n].decode("latin-1"), struct.unpack_from("<H", f, pos + 1 + n)[0]))
            pos += 3 + n
        return out


def census(path):
    le = LE(path)
    H = le.hdr
    print(f"file {path}: {len(le.file)} bytes; LE header at {le.h:#x}")
    print(f"MZ: {le.mz}")
    for k in ("cpu", "os", "module_flags", "pages", "page_size", "last_page_size", "eip_obj", "eip", "esp_obj",
              "esp", "obj_count", "data_pages", "fixup_section_size", "loader_section_size", "auto_ds_obj",
              "debug_info", "debug_info_size", "nonres_names", "nonres_names_size", "stack_size", "heap_size",
              "import_module_count", "entry_table", "resident_names"):
        v = H[k]
        print(f"  {k:24s} {v:#x} ({v})")
    for o in le.objects:
        filled = o.page_count * H["page_size"]
        print(f"  obj{o.index}: base={o.base:#x} vsize={o.vsize:#x} flags={o.flags:#06x} pages={o.page_index}..{o.page_index+o.page_count-1}"
              f" ({'code' if o.is_code else 'data'}{', big' if o.big else ''}) file-backed<= {filled:#x}")
    from collections import Counter
    c = Counter((fx.src_type, fx.flags, fx.obj, fx.target_obj) for fx in le.fixups)
    print(f"  fixups: {len(le.fixups)}")
    for k, v in sorted(c.items()):
        print(f"    src={k[0]:#04x} flg={k[1]:#04x} obj{k[2]}->obj{k[3]}: {v}")
    print("  resident names:", le.resident_names())
    print("  regions:")
    for s, e, lab in le.regions():
        print(f"    {s:#08x}-{e:#08x} {e-s:8d} {lab}")
    return le


if __name__ == "__main__":
    census(sys.argv[1] if len(sys.argv) > 1 else str(Path(__file__).resolve().parent.parent / "assets" / "D3D.EXE"))
