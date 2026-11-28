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


GAME = ROOT / "build/verify/game_game.obj"


@unittest.skipUnless(GAME.exists() and (ROOT / "assets/D3D.EXE").exists(), "needs a built game_game.obj and assets/")
class JumpTableControls(unittest.TestCase):
    """draw_water_distortion (1:03f50) ends with displayrooms' two switch tables plus alignment padding; its own
    code ends at the table start and the tables are verified with displayrooms."""

    @classmethod
    def setUpClass(cls):
        cls.img = Image()
        cls.mod = omf.load(GAME)[0]

    def results(self, mod, moved=None):
        match.Bindings._cache.clear()
        if moved:          # a modified copy of the canonical registry (overlays may not contradict it)
            sym = json.load(open(ROOT / "config/symbols.json", encoding="utf-8"))
            for name, addr in moved.items():
                sym["d3d"]["functions"][name]["addr"] = addr
            with tempfile.NamedTemporaryFile("w", suffix=".json", delete=False) as f:
                json.dump(sym, f)
            match.Bindings.registries = [Path(f.name), ROOT / "config/library_symbols.json"]
        try:
            return {r["function"]: r for r in match.verify_object(self.img, mod, {"draw_water_distortion",
                                                                                 "displayrooms"})}
        finally:
            match.Bindings.registries = None
            match.Bindings._cache.clear()

    def test_table_attribution_positive(self):
        r = self.results(self.mod)
        w = r["draw_water_distortion"]
        self.assertEqual(w["status"], "STRICT", w)
        self.assertGreater(w["trailing_table_of_next"], 8)
        self.assertEqual(r["displayrooms"]["leading_table"], w["trailing_table_of_next"])

    def test_table_not_attributed_without_entries(self):
        # remove the table relocations: no run of fixups -> the span is not shortened and cannot verify
        mod = copy.deepcopy(self.mod)
        pubs = {n: (s, o) for n, s, o, _ in mod.publics}
        s, o = pubs["displayrooms_"]
        mod.fixups = [f for f in mod.fixups if not (f.seg == s and o - 0x50 <= f.off < o)]
        r = self.results(mod)
        self.assertNotIn("trailing_table_of_next", r["draw_water_distortion"])
        self.assertNotEqual(r["draw_water_distortion"]["status"], "STRICT")

    def test_table_end_needs_oracle_boundary(self):
        # binding displayrooms elsewhere breaks the oracle confirmation of the table boundary
        r = self.results(self.mod, {"displayrooms": "1:04060"})
        self.assertNotEqual(r["draw_water_distortion"]["status"], "STRICT")


ENGINE = ROOT / "build/verify/engine.obj"
PITCH = ROOT / "build/verify/ass_pitch.obj"


@unittest.skipUnless(ENGINE.exists() and (ROOT / "assets/D3D.EXE").exists(), "needs a built ENGINE and assets/")
class BssReferenceControls(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.img = Image()
        cls.mod = omf.load(ENGINE)[0]
        cls.si = next(i for i, seg in enumerate(cls.mod.segments) if seg and seg.name == "_BSS")

    def check_function(self, mod, name="setvesamode"):
        segments = []
        r, = match.verify_object(self.img, mod, {name}, segments)
        layout, = [s for s in segments if s["segment"] == "_BSS"]
        refs = [ref for ref in r["references"] if ref["target"] == "seg:_BSS"]
        return r, layout, refs

    def test_rmi_addresses(self):
        r, layout, refs = self.check_function(self.mod)
        self.assertEqual((layout["status"], layout["oracle"], layout["size"]), ("VERIFIED", "2:b1e00", 1097937))
        self.assertEqual((r["status"], r["verified_bytes"]), ("STRICT", 292))
        sites = {0x43: 0, 0x63: 0x22, 0x6e: 0, 0x74: 0x1c, 0x79: 0x18, 0x7f: 0, 0x99: 0, 0xb9: 0x1c}
        self.assertEqual({f["site"]: (f["oracle_target"], f.get("verified_by")) for f in refs},
                         {site: (f"2:{0x1bde9f + off:x}", "BSS layout") for site, off in sites.items()})

    def test_unbound_public(self):
        mod = copy.deepcopy(self.mod)
        mod.publics = [("_unknown" if n == "_startang" else n, s, o, local) for n, s, o, local in mod.publics]
        r, layout, refs = self.check_function(mod)
        self.assertEqual(layout["status"], "BSS")
        self.assertIn("_unknown", layout["problem"])
        self.assertEqual(r["status"], "CODE_BYTES_MATCH")
        self.assertEqual(sum("verified_by" not in ref for ref in refs), 8)

    def test_conflicting_public(self):
        mod = copy.deepcopy(self.mod)
        mod.publics = [(n, s, o + 1 if n == "_startang" else o, local) for n, s, o, local in mod.publics]
        r, layout, refs = self.check_function(mod)
        self.assertEqual((layout["status"], r["status"]), ("MISMATCH", "CODE_BYTES_MATCH"))
        self.assertTrue(all("verified_by" not in ref for ref in refs))

    def test_foreign_global_before_first_public(self):
        mod = copy.deepcopy(self.mod)
        mod.publics = [p for p in mod.publics if p[0] != "_localtilestart"]
        r, layout, refs = self.check_function(mod)
        self.assertEqual((layout["status"], r["status"]), ("MISMATCH", "CODE_BYTES_MATCH"))
        self.assertIn("localtilestart", layout["problem"])
        self.assertTrue(all("verified_by" not in ref for ref in refs))

    def test_consistent_wrong_base(self):
        # Each anchor still implies the same base. The additional prefix byte lies in oracle padding.
        mod = copy.deepcopy(self.mod)
        mod.publics = [(n, s, o + 1 if s == self.si else o, local) for n, s, o, local in mod.publics]
        r, layout, refs = self.check_function(mod)
        self.assertEqual((layout["status"], layout["oracle"]), ("VERIFIED", "2:b1dff"))
        self.assertEqual(r["status"], "NEAR_MATCH")
        self.assertEqual(sum("oracle 2:" in p for p in r["problems"]), 8)
        self.assertTrue(all("verified_by" not in ref for ref in refs))

    def test_one_past_segment_with_equal_oracle_target(self):
        # Retain all public anchors and shorten only the private tail, exactly to the ES field.
        mod = copy.deepcopy(self.mod)
        seg = mod.segments[self.si]
        seg.size = 0x10c0c1
        del seg.covered[seg.size:], seg.data[seg.size:]
        r, layout, refs = self.check_function(mod)
        self.assertEqual(layout["status"], "VERIFIED")
        self.assertEqual(r["status"], "NEAR_MATCH")
        self.assertEqual([f["site"] for f in refs if "verified_by" not in f], [0x63])
        self.assertIn("outside segment", r["problems"][0])

    @unittest.skipUnless(PITCH.exists(), "needs a built ass_pitch.obj")
    def test_other_module_requires_its_own_bss_proof(self):
        self.assertEqual(self.check_function(self.mod)[0]["status"], "STRICT")
        # PITCH uses the same SEGDEF ordinal/name. Express its two named PitchTable references
        # as equivalent segment-relative fixups (the public is at offset zero).
        mod = omf.load(PITCH)[0]
        idx, offset = next((s, o) for n, s, o, _ in mod.publics if n == "_PitchTable")
        self.assertEqual((idx, offset), (self.si, 0))
        for f in mod.fixups:
            if mod.target_name(f) == "_PitchTable":
                f.target = (4, idx)
        r, layout, refs = self.check_function(mod, "PITCH_GetScale")
        self.assertEqual((r["status"], layout["status"], len(refs)), ("STRICT", "VERIFIED", 2))
        mod.publics = [p for p in mod.publics if p[1] != idx]
        r, layout, refs = self.check_function(mod, "PITCH_GetScale")
        self.assertEqual((r["status"], layout["status"]), ("CODE_BYTES_MATCH", "BSS"))
        self.assertEqual(sum("verified_by" not in ref for ref in refs), 2)

    def test_wrong_fixup_kind_same_width(self):
        mod = copy.deepcopy(self.mod)
        entry = next(o for n, _, o, _ in mod.publics if n == "setvesamode_")
        fx = next(f for f in mod.fixups if f.seg == 1 and f.off == entry + 0x43)
        fx.loc = 3  # OMF ptr16:16 occupies four bytes too
        r, layout, refs = self.check_function(mod)
        self.assertEqual((layout["status"], r["status"]), ("VERIFIED", "NEAR_MATCH"))
        self.assertEqual([f["site"] for f in refs if "verified_by" not in f], [0x43])

    def test_initialised_bss_is_not_a_layout_proof(self):
        mod = copy.deepcopy(self.mod)
        mod.segments[self.si].covered[-1] = 1
        r, layout, refs = self.check_function(mod)
        self.assertEqual((layout["status"], r["status"]), ("MISMATCH", "CODE_BYTES_MATCH"))
        self.assertTrue(all("verified_by" not in ref for ref in refs))


if __name__ == "__main__":
    unittest.main()
