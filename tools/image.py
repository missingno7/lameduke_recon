"""Oracle image view used by the matcher: object bytes and the LE fixup map in canonical obj:offset coordinates."""
import bisect
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from common import oracle_le  # noqa: E402


def parse_addr(s):
    o, a = s.split(":")
    return int(o), int(a, 16)


def fmt_addr(a):
    return f"{a[0]}:{a[1]:05x}"


class Image:
    def __init__(self, name=None):
        self.name = name or "d3d"
        le = oracle_le(self.name)
        self.le = le
        self.obj = {o.index: bytes(o.data) for o in le.objects}
        # file-backed (initialised) length per object; beyond it the loader zero-fills (BSS)
        self.init_len = {o.index: min(o.vsize, o.page_count * le.hdr["page_size"]) for o in le.objects}
        self.fixups = {k: (fx.src_type, fx.target_obj, fx.target_off) for k, fx in le.fixup_map().items()}
        self._sites = {}
        for (o, s) in self.fixups:
            self._sites.setdefault(o, []).append(s)
        for v in self._sites.values():
            v.sort()

    def sites_in(self, obj, start, end):
        v = self._sites.get(obj, [])
        return v[bisect.bisect_left(v, start):bisect.bisect_left(v, end)]

    _starts = None

    def is_boundary(self, obj, off, binds=None):
        """True when obj:off is an independently known function start or the end of the object."""
        if off == len(self.obj[obj]):
            return True
        if Image._starts is None:
            import funcmap
            _le, funcs, _r, _u, _jt = funcmap.analyse()
            Image._starts = {f["start"] for f in funcs}
        if obj == 1 and off in Image._starts:
            return True
        if binds is not None:
            for table in (binds.game, binds.lib):
                if (obj, off) in table.values():
                    return True
        return False
