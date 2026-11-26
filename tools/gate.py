"""Integration gate: verifier controls + zero-regression rebuild of every unit. Run before every commit.

    python tools/gate.py
"""
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
steps = [[sys.executable, "-m", "unittest", "discover", "-s", "tests", "-t", "."],
         [sys.executable, "tools/verify.py"]]
for cmd in steps:
    r = subprocess.run(cmd, cwd=ROOT)
    if r.returncode:
        print(f"GATE FAILED: {' '.join(cmd[1:])}")
        sys.exit(1)
print("GATE OK")
