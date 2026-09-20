#!/usr/bin/env python3
"""Launch the desktop game with a persistent player profile."""
import argparse
import os
from pathlib import Path
import platform
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "kit/tools"))
import game_config


def app_path(root, name, system):
    if system == "Darwin":
        return root / "build" / (name + ".app") / "Contents/MacOS" / name
    return root / "build/recomp" / (name + (".exe" if system == "Windows" else ""))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--profile", type=Path, default=ROOT / "build/profile")
    args = parser.parse_args()
    cfg = game_config.load(ROOT)
    binary = app_path(ROOT, cfg["game"]["app_name"], platform.system())
    if not binary.is_file():
        parser.error(f"Missing {binary}; run tools/build.py --target app first")
    profile = args.profile.resolve()
    profile.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, RECOMP_PROFILE_DIR=str(profile),
               RECOMP_REGISTRY=str(profile / "registry.json"))
    print(f"Player profile: {profile}", flush=True)
    return subprocess.call([str(binary)], cwd=ROOT, env=env)


if __name__ == "__main__":
    sys.exit(main())
