"""Positive and negative controls for tools/match.py on the LE oracle.

Control object: Ken's Aug-94 BUIL0801/ENGINE.OBJ (external reference). Its functions ksqrt/krand/getcrc are
byte-identical to the target. Bindings come from an overlay written into a temporary file.
Each negative control mutates one property and must never yield STRICT/CODE_BYTES_MATCH.
"""
import copy
import json
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import match  # noqa: E402
import omf  # noqa: E402
from image import Image  # noqa: E402

OBJ = ROOT / "external/x/kenbuild-20241213/kenbuild-related-archives-20241213/BUIL0801/ENGINE.OBJ"
FUNCS = {"ksqrt": "1:3bb6c", "krand": "1:45647", "getcrc": "1:4965b"}
DATA = {"sqrtable": "2:100e90", "randomseed": "2:1bce1c", "crctable": "2:1be6d4"}


@unittest.skipUnless(OBJ.exists() and (ROOT / "assets/D3D.EXE").exists(), "needs external/ and assets/")
class MatchControls(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.img = Image()
        cls.mod = omf.load(OBJ)[0]

    def run_with(self, funcs, data, mod=None, only=("ksqrt", "krand", "getcrc")):
        ov = {"d3d": {"functions": {k: {"addr": v} for k, v in funcs.items()},
                      "data": {k: {"addr": v} for k, v in data.items()}}}
        with tempfile.NamedTemporaryFile("w", suffix=".json", delete=False) as f:
            json.dump(ov, f)
        match.Bindings.extra = f.name
        match.Bindings.registries = []          # isolate the controls from the canonical registries
        match.Bindings._cache.clear()
        try:
            res = match.verify_object(self.img, mod or self.mod, set(only))
        finally:
            match.Bindings.extra = None
            match.Bindings.registries = None
            match.Bindings._cache.clear()
            Path(f.name).unlink()
        return {r["function"]: r for r in res}

    def test_positive_strict(self):
        r = self.run_with(FUNCS, DATA)
        for n in FUNCS:
            self.assertEqual(r[n]["status"], "STRICT", r[n])

    def test_unbound_data_is_not_strict(self):
        r = self.run_with(FUNCS, {})
        self.assertEqual(r["ksqrt"]["status"], "CODE_BYTES_MATCH")

    def test_wrong_function_address(self):
        r = self.run_with({**FUNCS, "ksqrt": "1:3bb6d"}, DATA)
        self.assertEqual(r["ksqrt"]["status"], "NEAR_MATCH")

    def test_wrong_global_binding(self):
        r = self.run_with(FUNCS, {**DATA, "randomseed": "2:1bce20"})
        self.assertEqual(r["krand"]["status"], "NEAR_MATCH")

    def test_wrong_instruction_byte(self):
        mod = copy.deepcopy(self.mod)
        pubs = {n: o for n, s, o, _ in mod.publics}
        off = pubs["krand_"] + 1                  # inside the function, not a fixup field? check below
        fixed = {f.off + k for f in mod.fixups if f.seg == 1 for k in range(f.size())}
        while off in fixed:
            off += 1
        mod.segments[1].data[off] ^= 0x01
        r = self.run_with(FUNCS, DATA, mod)
        self.assertEqual(r["krand"]["status"], "NEAR_MATCH")

    def test_missing_relocation(self):
        mod = copy.deepcopy(self.mod)
        pubs = {n: o for n, s, o, _ in mod.publics}
        start = pubs["krand_"]
        victim = next(f for f in mod.fixups if f.seg == 1 and start <= f.off < start + 32 and not f.self_rel)
        mod.fixups.remove(victim)
        r = self.run_with(FUNCS, DATA, mod)
        self.assertEqual(r["krand"]["status"], "NEAR_MATCH")

    def test_wrong_relocation_target(self):
        mod = copy.deepcopy(self.mod)
        pubs = {n: o for n, s, o, _ in mod.publics}
        start = pubs["krand_"]
        for f in mod.fixups:
            if f.seg == 1 and start <= f.off < start + 32 and not f.self_rel:
                f.disp += 4
                break
        r = self.run_with(FUNCS, DATA, mod)
        self.assertEqual(r["krand"]["status"], "NEAR_MATCH")

    def test_wrong_boundary(self):
        # ksqrt extent grown by moving the next public: extra bytes must be compared and fail or extend into a
        # region whose fixups/bytes differ
        mod = copy.deepcopy(self.mod)
        pubs = sorted((o, n) for n, s, o, _ in mod.publics if s == 1)
        i = next(k for k, (o, n) in enumerate(pubs) if n == "ksqrt_")
        nxt = pubs[i + 1][1]
        mod.publics = [p for p in mod.publics if p[0] != nxt]
        r = self.run_with(FUNCS, DATA, mod, only=("ksqrt",))
        self.assertNotIn(r["ksqrt"]["status"], ("STRICT", "CODE_BYTES_MATCH"))

    def test_truncated_candidate(self):
        # a candidate whose public extent ends early (prefix of the real function) must not be STRICT
        mod = copy.deepcopy(self.mod)
        pubs = {n: o for n, s, o, _ in mod.publics}
        mod.publics.append(("fake_cut_", 1, pubs["krand_"] + 0x10, False))
        r = self.run_with(FUNCS, DATA, mod, only=("krand",))
        self.assertNotEqual(r["krand"]["status"], "STRICT")

    def test_data_hole_model(self):
        # holes in DATA segments are linker zero fill; holes in CODE are never accepted
        seg = omf.Segment("CONST", "DATA", 5, 2, 4, True, None, bytearray(b"ab\x00\x00"), bytearray(b"\x01\x01\x00\x00"))
        self.assertTrue(match.data_covered(seg))
        self.assertEqual(bytes(seg.data[2:]), b"\x00\x00")       # a hole compares as zero against the oracle
        code = omf.Segment("_TEXT", "CODE", 1, 2, 4, True, None, bytearray(4), bytearray(b"\x01\x01\x00\x00"))
        self.assertFalse(match.data_covered(code))


DPMI = ROOT / "build/verify/ass_dpmi.obj"      # fresh build of src/audiolib/dpmi.c by tools/verify.py


@unittest.skipUnless(DPMI.exists() and (ROOT / "assets/D3D.EXE").exists(), "needs a built ass_dpmi.obj and assets/")
class ObjectEndControls(unittest.TestCase):
    """DPMI_UnlockMemory is the last public of DPMI.C's _TEXT (1:5eb24, 86 bytes); the oracle has two zero
    padding bytes and then the next module at the 4-aligned 1:5eb7c."""

    @classmethod
    def setUpClass(cls):
        cls.img = Image()
        cls.mod = omf.load(DPMI)[0]

    def ext_unverified(self, mod):
        match.Bindings._cache.clear()
        r = {x["function"]: x for x in match.verify_object(self.img, mod, {"DPMI_UnlockMemory"})}["DPMI_UnlockMemory"]
        return r, [u for u in r["unverified"] if u.startswith("extent end")]

    def test_object_end_positive(self):
        r, ext = self.ext_unverified(self.mod)
        self.assertEqual(ext, [])
        self.assertEqual(r["status"], "STRICT", r)

    def test_object_end_truncated(self):
        # candidate segment cut by two bytes: the gap now holds real code bytes and must not be accepted
        mod = copy.deepcopy(self.mod)
        seg = mod.segments[1]
        seg.size -= 2
        del seg.data[-2:], seg.covered[-2:]
        r, ext = self.ext_unverified(mod)
        self.assertNotEqual(r["status"], "STRICT")
        self.assertTrue(ext, r)

    def test_object_end_helper(self):
        cand = bytes(self.img.obj[1][0x5eb24:0x5eb7a])
        self.assertTrue(match.object_end_confirmed(self.img, 1, 0x5eb7a, cand))
        # absorbing one padding byte (candidate ending in 00) is refused
        self.assertFalse(match.object_end_confirmed(self.img, 1, 0x5eb7b, cand + b"\x00"))
        # an end inside the function, followed by non-zero bytes, is refused
        self.assertFalse(match.object_end_confirmed(self.img, 1, 0x5eb70, cand[:-10]))


if __name__ == "__main__":
    unittest.main()
