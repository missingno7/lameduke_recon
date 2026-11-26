"""tools/blockdiff.py is diagnostic, but it must agree with the verifier on STRICT functions: every STRICT function
scores 1.0 on every measure (content, loose, fuzzy, skeleton, layout, regs) and has the oracle's size. Uses the
objects tools/verify.py left in build/verify/ (A.ASM: relocated MASM jumps; DPMI: module-last extent; GAME:
inlining, jump tables and Watcom filler padding)."""
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import blockdiff  # noqa: E402

UNITS = ["a_asm", "ass_dpmi", "game_game"]


@unittest.skipUnless((ROOT / "assets/D3D.EXE").exists() and
                     all((ROOT / f"build/verify/{u}.obj").exists() for u in UNITS), "needs assets/ and build/verify/")
class BlockdiffAgreesWithStrict(unittest.TestCase):
    def test_strict_functions_score_one(self):
        az = blockdiff.Analyzer()
        checked = 0
        for unit in UNITS:
            for p in az.object_pairs(blockdiff.unit_obj(unit), all_funcs=True):
                if p.status != "STRICT":
                    continue
                checked += 1
                r = blockdiff.metrics(p.orig, p.cand)
                for k in ("content", "loose", "fuzzy", "skeleton", "layout", "regs"):
                    self.assertEqual(r[k], 1.0, f"{unit} {p.name} {k}: {r}")
                self.assertEqual(p.cand_size, p.orig_size, f"{unit} {p.name}")
        self.assertGreater(checked, 40)

    def test_near_function_is_not_one(self):
        az = blockdiff.Analyzer()
        near = [p for p in az.object_pairs(blockdiff.unit_obj("game_game")) if p.status == "NEAR_MATCH"]
        self.assertTrue(near)
        self.assertTrue(any(blockdiff.metrics(p.orig, p.cand)["content"] < 1.0 for p in near))


if __name__ == "__main__":
    unittest.main()
