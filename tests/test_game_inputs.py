"""Pin native address assumptions to the user's actual PE images."""
import hashlib
from pathlib import Path
import sys
import struct

import pefile
import pytest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "kit/tools"))
import game_config


@pytest.fixture
def config():
    return game_config.load(ROOT)


def load_pe(path):
    if not path.is_file():
        pytest.skip(f"Private game input missing: {path}")
    return pefile.PE(str(path))


def test_image_identity_and_memory_map(config):
    pe = load_pe(config["developer_exe_path"])
    game = config["game"]
    assert hashlib.sha256(pe.__data__).hexdigest() == game["sha256"]
    assert pe.FILE_HEADER.Machine == 0x14C
    assert pe.OPTIONAL_HEADER.ImageBase == game["image_base"]
    assert game["entry_point"] == game["image_base"] + pe.OPTIONAL_HEADER.AddressOfEntryPoint
    assert game["image_base"] + pe.OPTIONAL_HEADER.SizeOfImage <= game["heap_base"]


def test_renderer_identity_and_arena(config):
    for module in config["aux_modules"]:
        pe = load_pe(module["path"])
        assert hashlib.sha256(pe.__data__).hexdigest() == module["sha256"]
        assert pe.OPTIONAL_HEADER.ImageBase == module["base"]
        assert pe.OPTIONAL_HEADER.SizeOfImage == module["size"]
        assert module["base"] >= 0x10000000
        assert module["base"] + module["size"] <= config["game"]["guest_size"]


def test_placeholder_hooks_are_outside_original_sections(config):
    pe = load_pe(config["developer_exe_path"])
    base = pe.OPTIONAL_HEADER.ImageBase
    end = base + pe.OPTIONAL_HEADER.SizeOfImage
    section_end = max(base + s.VirtualAddress + s.Misc_VirtualSize for s in pe.sections)
    addresses = [config["translate"]["animation_counter"]]
    for value in config["hooks"].values():
        addresses.extend(value if isinstance(value, list) else [value])
    addresses.extend(v["addr"] for v in config["globals"].values())
    assert all(section_end <= address and address + 4 <= end for address in addresses)


def test_renderer_imports_match_main_exports(config):
    main = load_pe(config["developer_exe_path"])
    exports = {s.name: s.address for s in main.DIRECTORY_ENTRY_EXPORT.symbols}
    renderer = load_pe(config["aux_modules"][0]["path"])
    descriptor = next(d for d in renderer.DIRECTORY_ENTRY_IMPORT
                      if d.dll.decode().lower() == config["game"]["executable"].lower())
    assert len(descriptor.imports) == 48
    assert all(i.name in exports for i in descriptor.imports)
    # Both executable code and shared data must resolve directly into the PE.
    executable = [bool(main.get_section_by_rva(exports[i.name]).Characteristics & 0x20000000)
                  for i in descriptor.imports]
    assert any(executable) and not all(executable)


def test_display_adaptation_addresses(config):
    renderer = load_pe(config["aux_modules"][0]["path"])
    data = lambda address, size: renderer.get_data(address - 0x10000000, size)
    assert struct.unpack("<12I", data(0x100380c0, 48)) == (
        640, 480, 800, 600, 960, 720, 1024, 768, 1280, 1024, 1600, 1200)
    assert struct.unpack("<f", data(0x10031428, 4))[0] == 1 / 32
    # Both scaled-font UV corrections read this constant.
    assert data(0x1000d876, 6) == bytes.fromhex("d80d28140310")
    assert data(0x1000d8ae, 6) == bytes.fromhex("d80d28140310")
    main = load_pe(config["developer_exe_path"])
    imports = {i.address: i.name for d in main.DIRECTORY_ENTRY_IMPORT for i in d.imports}
    assert {address: imports[address] for address in (
        0x004d2000, 0x004d2008, 0x004d200c, 0x004d2014, 0x004d2018)} == {
        0x004d2000: b"RegSetValueExA", 0x004d2008: b"RegFlushKey",
        0x004d200c: b"RegCloseKey", 0x004d2014: b"RegCreateKeyExA",
        0x004d2018: b"RegQueryValueExA"}


def test_touch_cursor_layouts(config):
    pe = load_pe(config["developer_exe_path"])
    data = lambda address, size: pe.get_data(address - 0x400000, size)
    # Menu constructor, global publication, input member and float X/Y integrator.
    assert data(0x0041b109, 18) == bytes.fromhex(
        "8d9084000000 c700704e4d00 898890000000")
    assert data(0x0040c67a, 5) == bytes.fromhex("a3002a5200")
    assert data(0x0041b201, 12) == bytes.fromhex("8b8790000000 8b4814 8b5018")
    assert data(0x0041b219, 6) == bytes.fromhex("d88784000000")
    assert data(0x0041b229, 6) == bytes.fromhex("d88788000000")
    assert struct.unpack("<2f", data(0x004d4b8c, 8)) == (630, 470)
    # Mission constructor/global and renderer-pixel cursor integrator.
    assert data(0x0047dc8a, 6) == bytes.fromhex("c706b81a4e00")
    assert data(0x0047de0b, 6) == bytes.fromhex("8935f8295200")
    assert data(0x00480e38, 11) == bytes.fromhex("a1c0295200 8b4814 8b4018")
    assert data(0x00480e47, 6) == bytes.fromhex("8d8ff0000000")
    assert data(0x00480e5d, 6) == bytes.fromhex("d887f4000000")
    # Input Read: active flag +0x10, mouse interface +8, DIMOUSESTATE at +0x14.
    assert data(0x00408303, 3) == bytes.fromhex("8b4610")
    assert data(0x0040830e, 7) == bytes.fromhex("8b4608 57 8d7e14")
