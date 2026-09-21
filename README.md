# Metal Fatigue Recomp

[Build & contribute](CONTRIBUTING.md) · [Port analysis](docs/analysis.md) ·
[Testing](docs/testing.md) · [Changelog](CHANGELOG.md)

Work in progress: a native static recompilation of the GOG Windows release
using [recomp-kit](https://github.com/veritr1x/recomp-kit). Original game files
are required and are never included. See [analysis notes](docs/analysis.md)
for the current evidence and next blockers. Basic mission control is verified;
full campaign playability is not established.

Current checkpoint (2026-09-20): the native macOS app renders the main menu
and campaign selection through the kit's Metal backend. Mission terrain, units,
HUD and minimap render; the user confirmed physical mouse control and a unit
travelling to its ordered destination. The user also confirmed that the rebuilt
host resolves the portrait/HUD flickering. The local kit changes add D3D3
interfaces, cooperative guest-stack resumption, import ABI corrections and
renderer code recovery, an x87 arithmetic destination correction and render
target backpressure for retained HUD updates.
The opening cinematics now play through AVIFile/Indeo 5 and WinMM audio shims.
The user confirmed audible music and effects, plus working campaign robot
animations after correcting colour-key handling in the host renderer. Miles
digital samples, file-backed CD music and cinematic audio use the native device.
Resolution changes now reload the renderer correctly; the user confirmed two
successful changes, followed by a verified clean native exit.

The original Graphics menu now offers 640×480, 800×600, 1024×768, 1920×1080,
2560×1440 and 3840×2160. HD modes use a 1280×720 logical canvas with native
output rendering, keeping the mission HUD readable and camera/input coordinates
consistent. The scaled-font atlas correction is removed so text remains intact.
The user confirmed the corrected HUD, text, camera and F10 settings behavior.
Fonts retain the original bitmap artwork; they are not replacement vector fonts.

Touch places the game's own cursor at the touched point. Menus retain their
640×480 coordinate system at every resolution, while missions use the logical
renderer canvas. Pending relative motion is cleared after placement so it
cannot shift the cursor away from the finger; mouse buttons and wheel remain
independent.

**You need your own copy of the game.** Executables, artwork, audio, movies,
missions and generated game code are prepared locally and are not included.
This repository contains the game configuration, reviewed native adaptations,
tests and tools. The shared runtime and translator live in the public
`kit/` submodule. See [NOTICE](NOTICE) for ownership and dependency credits.

The port selects the original `DirectXRendEng.dll` shipped under
`_unsupported/` in this installation. The executable and renderer are both
hash-pinned in `game.toml`; neither original file is patched.

## Prepare and build on macOS

Requires Apple Silicon, Xcode command-line tools, Python 3.9+, Ghidra 12.1.3
and a compatible JDK (the local analysis uses OpenJDK 26.0.1).

```sh
git clone --recurse-submodules https://github.com/veritr1x/metal-fatigue-recomp.git
cd metal-fatigue-recomp
git submodule update --init
python3 -m venv .venv
.venv/bin/python -m pip install -r kit/requirements-dev.txt
.venv/bin/python tools/setup.py --install /path/to/Metal\ Fatigue --link-only
.venv/bin/python tools/inspect_game.py
.venv/bin/python tools/analyze.py --ghidra-home /path/to/ghidra_12.1.3_PUBLIC
.venv/bin/python tools/build.py --target headless --regenerate --jobs 8
```

`tools/analyze.py` analyzes both images. `--module main` or `--module directx`
exports only one. Outputs go into ignored `analysis/` and `build/`.
The normal kit setup exports without automatic analysis, so use the separate
analysis command above for this game.

`tools/build.py --target app` builds the SDL/Metal desktop app after translation.
Launch with a persistent player profile:

```sh
.venv/bin/python tools/play.py
```

The default profile is `build/profile/`; `--profile /path/to/profile` selects
another. Game options and F10 host settings are saved separately as
`registry.json` and `mod-settings.json`. Fresh profiles receive the supported
DirectX driver defaults; existing choices are retained. F10 opens host settings
and F11 cycles the performance overlay. Resolution stays in the game's Graphics
menu. Original 960×720, 1280×1024 and 1600×1200 slots are replaced by the new list.

## Platform builds

Prepare the private inputs and translation above before building another target.
The kit uses SDL3, Metal on Apple platforms and Vulkan on Linux, Windows and Android.

| Platform | Command | Validation |
| --- | --- | --- |
| macOS | `.venv/bin/python tools/build.py --target app --jobs 8` | Native mission, unit movement, movies, audio, campaign animation, resolution changes and clean exit verified. |
| iOS / iPadOS | `.venv/bin/python tools/build.py --target ios --team <TEAM_ID> --no-install` | Signed app installed on iPad; opening cinematic verified. Mission play unverified. |
| Android | `.venv/bin/python tools/build.py --target android --no-install` | Translated arm64 APK built; device play unverified. |
| Linux | `.venv/bin/python tools/build.py --target app --jobs 8` | Translated arm64 app/package built on Ubuntu 24.04; gameplay unverified. |
| Windows | `.venv/Scripts/python tools/build.py --target app --jobs 8` | Translated x86-64 app/package cross-built; AVI/Ogg decoding checked under Wine; Windows gameplay unverified. |

iOS requires Xcode, an iOS SDK and a development team. Android requires JDK 17+,
SDK platform 36, build tools 37.0.0 and NDK 27.2.12479018; set `JAVA_HOME`,
`ANDROID_HOME` and `ANDROID_NDK_HOME`. `--no-install` leaves attached devices
untouched. Android game data is supplied separately with `--push-game` when
installing on a device. Local iOS bundles include private game data and must
not be uploaded as public artifacts. See the kit's
[contributor guide](kit/CONTRIBUTING.md) for platform prerequisites.

The local Windows cross-build uses `LLVM_MINGW_ROOT` and
`tools/build.py --preset windows-cross --target app`. That preset enables
FFmpeg for AVI/Indeo movies and Ogg CD music and bundles all three media DLLs
with their notice. Cross-building needs a POSIX shell and GNU make; a native
Windows build uses the MSYS2 prerequisites in the kit guide.
Linux packages go under `build/package`; Windows cross-packages go under
`build/windows/package`. Build evidence and local artifact paths are recorded in
[analysis](docs/analysis.md). Public CI uses stub translations on all platforms;
the translated builds above use private local inputs.

## Reproduce the startup probe

```sh
.venv/bin/python tools/run_probe.py --host headless --seconds 10
.venv/bin/python tools/run_probe.py --host app --seconds 600 --frames 0
```

Each probe creates an isolated profile and DirectX registry settings under
`build/runs/`, keeps its logs, and returns the host's exit status. Add
`--profile build/profile` to retain preferences between probes. Normal play uses
`tools/play.py` without a diagnostic timeout. The macOS app bundle is
`build/MetalFatigueRecomp.app`.
The app currently ignores the kit's internal time/frame caps; the probe
terminates it after `--seconds + 45`. Close its window to finish sooner.
`--trace-files`, `--trace-pointer` and `--trace-d3d` add diagnostics;
`--capture-audio` records the mixer output and audio calls. Headless frame buffers
remain uniform because that host does not rasterize the D3D scene; use the
native app for visual validation.

```sh
.venv/bin/python -m pytest -q tests kit/tests/test_game_config.py kit/tests/test_setup.py kit/tests/test_game_literals.py
.venv/bin/python tools/test.py --compile-only --jobs 8
RECOMP_PYTHON="$PWD/.venv/bin/python" RECOMP_PROFILE_DIR="$PWD/build/test-profile" build/recomp/runtime_tests --startup-contracts
```

Validation results and remaining failures are recorded in
[analysis notes](docs/analysis.md).
