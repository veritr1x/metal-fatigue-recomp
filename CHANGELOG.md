# Changelog

## Unreleased

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
