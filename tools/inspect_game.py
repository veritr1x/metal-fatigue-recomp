#!/usr/bin/env python3
"""Record reproducible PE evidence without executing or changing game files."""
import argparse
import hashlib
import json
from pathlib import Path
import sys

import pefile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "kit/tools"))
import game_config


def describe(path):
    """Describe headers, imports and exports; addresses remain guest addresses."""
    data = path.read_bytes()
    pe = pefile.PE(data=data)
    header = pe.OPTIONAL_HEADER
    base = header.ImageBase
    return {
        "path": str(path.relative_to(ROOT)),
        "sha256": hashlib.sha256(data).hexdigest(),
        "machine": pe.FILE_HEADER.Machine,
        "image_base": base,
        "image_size": header.SizeOfImage,
        "entry_point": base + header.AddressOfEntryPoint,
        "sections": [{"name": s.Name.rstrip(b"\0").decode(),
                      "address": base + s.VirtualAddress, "virtual_size": s.Misc_VirtualSize,
                      "raw_size": s.SizeOfRawData, "characteristics": s.Characteristics}
                     for s in pe.sections],
        "imports": [{"dll": d.dll.decode(), "name": i.name.decode() if i.name else None,
                     "ordinal": i.ordinal, "iat": i.address}
                    for d in getattr(pe, "DIRECTORY_ENTRY_IMPORT", []) for i in d.imports],
        "exports": [{"name": s.name.decode() if s.name else None,
                     "ordinal": s.ordinal, "address": base + s.address,
                     "forwarder": s.forwarder.decode() if s.forwarder else None}
                    for s in getattr(getattr(pe, "DIRECTORY_ENTRY_EXPORT", None), "symbols", [])],
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "analysis/pe-inventory.json")
    args = parser.parse_args()
    cfg = game_config.load(ROOT)
    main_image = describe(cfg["developer_exe_path"])
    if main_image["sha256"] != cfg["game"]["sha256"]:
        parser.error("Main executable does not match game.toml")
    reports = [main_image]
    exported = {s["name"] for s in main_image["exports"] if s["name"]}
    for module in cfg["aux_modules"]:
        report = describe(module["path"])
        if report["sha256"] != module["sha256"]:
            parser.error(f"{module['name']} does not match game.toml")
        report["imports_from_main"] = [i["name"] for i in report["imports"]
                                       if i["dll"].lower() == cfg["game"]["executable"].lower()]
        report["missing_main_exports"] = [n for n in report["imports_from_main"] if n not in exported]
        reports.append(report)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(reports, indent=2) + "\n")
    for report in reports:
        print(f"{report['path']}: {len(report['imports'])} imports, "
              f"{len(report['exports'])} exports; entry {report['entry_point']:#010x}")
    print(f"Private inventory: {args.output}")


if __name__ == "__main__":
    main()
