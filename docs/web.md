# Browser build and optional streaming

Use this repository's pinned `kit/`, your own hash-matched installation, and
the preparation/analysis steps in [CONTRIBUTING.md](../CONTRIBUTING.md).
Install the kit's hashed Python requirements and activate Emscripten 6.0.11.
On Windows, use `.venv/Scripts/python.exe` instead of `.venv/bin/python`.

```sh
.venv/bin/python tools/build.py --target web --regenerate --jobs 4
.venv/bin/python kit/tools/web_launcher.py --game-dir "$PWD" --out build/web-stream-site \
    --web-build metal-fatigue=build/web/recomp --stream metal-fatigue --serve 2028
```

After the first translation, omit `--regenerate` when translator inputs have
not changed. Open `http://127.0.0.1:2028/` and choose **Stream game**.
The command stages a private installation and manifest into the site. It
preserves executable/module hash checks, bundle exclusions and prepared
auxiliary modules. Original installation files are unchanged.

If analysis uses a minimal executable copy, select the full installation
explicitly: replace `--stream metal-fatigue` with
`--stream "metal-fatigue=/path/to/full installation"`. That directory must contain
the configured executable with its pinned hash and required data folders.
This changes the staging source only, preserving configured analysis inputs.
Keep generated game data and all local evidence private.

The worker mounts game files read-only at `/data`, using 4 MiB HTTP ranges
and a 64 MiB download cache. This is a cache bound, not total browser memory.
The kit supplies a corrected fetch backend without modifying the installed
SDK. Saves and settings use a separate writable OPFS profile.

**Import folder**, ZIP import and **Play** remain available. Play uses an
imported installation; Stream game selects hosting explicitly. Direct player
URLs prefer imported data when available. Both modes share saves on the same
origin. For games with prepared auxiliary modules, import a prepared bundle
with the pinned modules at their configured names; the staged `data/` folder
is such a bundle. A raw install may keep those modules elsewhere.

To prepare an import-only site, use a separate `--out` directory and omit
`--stream`. Previously staged assets are not removed by omitting the flag.
The server supplies HEAD, byte ranges, Wasm MIME and COOP/COEP. Other hosting
must preserve those semantics and same-origin asset URLs; files larger than
64 MiB require ranges. Streaming has no persistent offline asset cache.

Keep the hostname and port consistent to reuse saves. Export saves from
**Manage** before clearing site storage. Browser compatibility results are
recorded below; compilation alone does not establish gameplay.

## Local validation

Updated to recomp-kit `9dff77f71928ce55933c6354ee763a8077d17909`. The real
Emscripten 6.0.11 game build succeeds. Shared changes implement worker-safe
texture/buffer readback, preserve transfer rectangles across proxy calls,
handle self-copies and restore depth checkpoints with a depth-only draw.
The real browser GPU fixture passes 1,004 checks, including device loss,
padded rows and depth restore values. See the kit's testing guide to rerun it.

On Windows/Chromium 145 with hardware WebGPU, the shared transfer fixes
restore menu labels and the first RimTech mission's terrain, units, HUD,
minimap and selected MissileCar panel. The previous large triangles, stale
briefing remnants and missing labels are absent in the observed mission.
Streamed startup verifies the prepared renderer module, and the OPFS game
directory stays empty.

The final rebuilt player also imports the prepared 879-file bundle through
the normal Import folder control, boots source `import`, and reaches the same
mission with unit-selection panel updates and zero streamed asset GETs.
Neither mode shows JavaScript exceptions or WebGPU validation errors.

These are bounded rendering and interaction checks. Direct movement-order
round trips, save/load, full missions, audible output and other browsers are
not established. Frame rates remain low (roughly 4-10 FPS in this run); the
rendering fixes are not a playable-performance claim.

Portable project tests pass (6 tests). Private screenshots and diagnostics
remain under ignored build directories. Historical native/device results
were not all rerun for this browser update. The 64 MiB streaming bound applies
to the download cache, not total browser memory.
