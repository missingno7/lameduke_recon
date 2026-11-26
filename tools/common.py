"""Shared paths and helpers (stdlib only). Adapted from rayman_recon/tools/common.py (GPLv2)."""
import hashlib
import json
import os
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ASSETS = ROOT / "assets"
BUILD = ROOT / "build"
CONFIG = ROOT / "config"
EXTERNAL = ROOT / "external"
TOOLS_DIR = Path(os.environ.get("LAMEDUKE_TOOLS", "C:/tools"))


def sha256_bytes(data):
    return hashlib.sha256(data).hexdigest()


def sha256_file(path, chunk=1 << 20):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for b in iter(lambda: f.read(chunk), b""):
            h.update(b)
    return h.hexdigest()


def load_json(path):
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def dump_json(path, obj):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_suffix(path.suffix + f".{os.getpid()}.tmp")
    try:
        with open(tmp, "w", encoding="utf-8", newline="\n") as f:
            json.dump(obj, f, indent=1)
            f.write("\n")
        for attempt in range(20):
            try:
                os.replace(tmp, path)
                return
            except PermissionError:
                if attempt == 19:
                    raise
                time.sleep(0.25)
    finally:
        tmp.unlink(missing_ok=True)


def oracle_bytes(name="d3d"):
    """Bytes of a pinned oracle; refuses to continue on a hash mismatch."""
    ent = load_json(CONFIG / "oracles.json")["oracles"][name]
    data = (ROOT / ent["path"]).read_bytes()
    if sha256_bytes(data) != ent["sha256"]:
        raise SystemExit(f"oracle {name}: sha256 mismatch (refusing)")
    return data


_le_cache = {}


def oracle_le(name="d3d"):
    import le
    if name not in _le_cache:
        _le_cache[name] = le.LE(oracle_bytes(name))
    return _le_cache[name]
