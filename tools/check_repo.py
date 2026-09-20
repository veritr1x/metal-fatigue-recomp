#!/usr/bin/env python3
"""Apply the kit's source and Markdown-link publication checks to this repo."""
import importlib.util
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("check_repo", ROOT / "kit/tools/check_repo.py")
checker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(checker)
checker.ROOT = ROOT
if __name__ == "__main__":
    checker.main()
