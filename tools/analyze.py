#!/usr/bin/env python3
"""Hash-check and export the main image and renderer using Ghidra analysis.

Unlike the kit's annotation-based setup, this port needs default analyzers.
Original binaries are read only; exports and disposable projects are ignored.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
KIT = ROOT / "kit"
sys.path.insert(0, str(KIT / "tools"))
import game_config

GHIDRA_VERSION = "12.1.3"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ghidra-home", type=Path, default=os.environ.get("GHIDRA_HOME"))
    parser.add_argument("--java-home", type=Path, default=os.environ.get("JAVA_HOME"))
    parser.add_argument("--max-memory", default="8G")
    parser.add_argument("--module", help="Analyze only 'main' or one configured auxiliary module")
    args = parser.parse_args()
    if not args.ghidra_home:
        parser.error("Set --ghidra-home or GHIDRA_HOME")
    ghidra = args.ghidra_home.expanduser().resolve()
    properties = ghidra / "Ghidra/application.properties"
    if not properties.is_file() or f"application.version={GHIDRA_VERSION}\n" not in properties.read_text():
        parser.error(f"Use Ghidra {GHIDRA_VERSION}")
    cfg = game_config.load(ROOT)
    modules = [{"key": "main", "path": cfg["developer_exe_path"],
                "sha256": cfg["game"]["sha256"], "listings_path": cfg["listings_path"]},
               *cfg["aux_modules"]]
    if args.module:
        modules = [module for module in modules if module["key"] == args.module]
        if not modules:
            parser.error(f"Unknown module: {args.module}")
    # Check all inputs before producing any outputs.
    for module in modules:
        digest = hashlib.sha256(module["path"].read_bytes()).hexdigest()
        if digest != module["sha256"]:
            parser.error(f"Unsupported {module['path'].name}: SHA-256 {digest}")
    env = dict(os.environ, MAXMEM=args.max_memory)
    if args.java_home:
        env["JAVA_HOME"] = str(args.java_home.expanduser().resolve())
        env["PATH"] = str(Path(env["JAVA_HOME"]) / "bin") + os.pathsep + env.get("PATH", "")
    for module in modules:
        listings = module["listings_path"]
        output = listings.parent
        project = ROOT / "analysis/ghidra"
        project.mkdir(parents=True, exist_ok=True)
        output.mkdir(parents=True, exist_ok=True)
        subprocess.run([
            str(ghidra / "support/analyzeHeadless"), str(project),
            cfg["game"]["app_name"] + "-" + module["key"],
            "-import", str(module["path"]), "-deleteProject",
            "-scriptPath", str(KIT / "tools"),
            "-postScript", "ExportProgram.java", str(output),
        ], cwd=ROOT, env=env, check=True)
        index = listings / "functions.tsv"
        if not index.is_file() or len(index.read_text().splitlines()) < 2:
            sys.exit(f"No function index exported for {module['key']}")
        (listings / "inputs.json").write_text(json.dumps({
            "executable_sha256": module["sha256"],
            "ghidra_version": GHIDRA_VERSION,
            "ghidra_analysis": "default analyzers",
        }, indent=2) + "\n")
        print(f"Listings ready: {listings}", flush=True)


if __name__ == "__main__":
    main()
