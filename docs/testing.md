# Testing

Prepare the private game inputs using the [README](../README.md). Build via the
wrappers so the correct game configuration and isolated build directories apply.

```sh
.venv/bin/python tools/test.py
.venv/bin/python -m pytest -q tests
.venv/bin/python tools/test.py --compile-only --jobs 8
build/recomp/display_tests
build/recomp/host_tests --presenter-only
build/recomp/host_tests --presenter-gpu
RECOMP_MOD_TEST_ONLY=hook_guest_call_import_preserves_registers,settings_host_display_survives_page_reinit_and_profile_reload,settings_declare_and_round_trip build/recomp/mods_tests
RECOMP_PROFILE_DIR="$PWD/build/test-profile" build/recomp/runtime_tests --startup-contracts
```

The display suite exercises physical mode enumeration through the real guest
callback return driver, repeated renderer initialization and font-state restoration.
Presenter suites check output target sizes, retained frame lifetimes and GPU pixels.
Private-input tests skip when the original images are absent; CI uses stub
translations and does not establish game playability.

For interactive checks, use `tools/play.py`, or a logged probe with
`tools/run_probe.py --host app --seconds 1200 --frames 0 --profile build/profile`.
Check movie playback/skipping, music/effects, campaign robot previews, mission
terrain/HUD, selecting and moving a unit, camera edge scrolling, and repeated
resolution changes. Change both F10 display rows, change resolution, restart with
the same profile, and verify the choices remain applied. Exit normally afterward.

The [analysis record](analysis.md) separates live confirmations, automated checks,
known failures and platform build results. Full campaign completion, long sessions
and a sustained 4K frame-rate target remain unverified.
