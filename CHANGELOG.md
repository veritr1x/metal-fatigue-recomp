# Changelog

## Unreleased

- Pin kit `9dff77f` for browser GPU readback and preserved transfer
  regions; resolve the previous browser rendering/startup blocker and record
  first-level checks and remaining performance limits in `docs/web.md`.

- Update recomp-kit to `1cbd81c` for optional browser asset streaming, the
  corrected bounded fetch backend and fixed-function WebGPU shader fixes.
  Preserve folder import and OPFS profiles; document browser validation.

- Publish `.recomp.json` project metadata for discovery tools, with a
  contributor reminder to keep its status aligned with the README.

- Pin the shared kit to `a18ccaf`, including the reviewed runtime, renderer
  and translator fixes and bounded GetDIBits conversion. Use the hashed
  contributor dependency lock in setup and CI.

- Refresh the fullscreen DirectDraw window after movie mode changes through
  the updated shared runtime.

- Merge the port's runtime history into the shared kit and load its hash-pinned
  DirectX renderer from `_unsupported/` inside relocated iPad installations.
  The signed iPad build now reaches the opening cinematic.

- Update the shared runtime with iPad touch-coordinate, stick-motion and frame-color
  fixes, plus regression coverage for simultaneous keys and mapped pad controls.

- Preserve visible gamepad sticks and buttons when switching from a
  collapsed keyboard.

- Hide the on-screen keyboard HIDE/KEYS tabs when a hardware keyboard or
  controller auto-hides the controls; retain the layout switch and saved visibility.

- Include AVI/Indeo movies and Ogg music support in Windows cross-builds, with
  the three FFmpeg DLLs and dependency notice in the desktop package.
- Initial native Metal Fatigue port using the GOG executable and DirectX renderer.
- Render terrain, units, HUD, minimap and campaign previews; physical unit movement verified.
- Play AVI/Indeo movies, Miles music/effects and waveform audio through the host.
- Preserve incremental HUD drawing and restore renderer DLL state on resolution changes.
- Add 1080p, 1440p and 4K modes with a stable logical HUD/camera canvas and corrected font UVs.
- Keep player preferences in a reusable profile and seed missing DirectX defaults on first run.
- Add hash-pinned address checks, callback regressions and cross-platform source build checks.
