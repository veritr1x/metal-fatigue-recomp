#!/usr/bin/env python3
"""Run the kit's setup tool with this game configuration."""
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "kit/tools/setup.py"
if not TOOL.is_file():
    sys.exit("Missing kit submodule: run git submodule update --init")
sys.exit(subprocess.call([sys.executable, str(TOOL), "--game-dir", str(ROOT), *sys.argv[1:]], cwd=ROOT))
