# Metal Fatigue analysis

## Current checkpoint, 2026-09-20

The native SDL/Metal app renders the main menu and campaign selection. Keyboard
navigation works; the user confirmed that the physical mouse moves the green
in-game cursor. Automated CUA mouse events reach SDL at a fixed position, so
campaign selection has been exercised with the user's physical mouse.

Mission terrain, units and movement have user confirmation, as does the HUD
flicker correction. Sustained missions and campaign completion remain unverified.
The opening cinematics now visibly play through the original game's AVIFile /
Indeo 5 path, with WinMM PCM/IMA ADPCM output. The user confirmed audible music
and effects and working campaign animation. Campaign previews use the game's
graphics widgets, separately from AVI; the colour-key correction restores the
content beneath their transparent overlays. The overlay's
displayed refresh rate is not a gameplay benchmark.

## Workspace and inputs

The initial workspace contained only `original/gog/` and local session metadata.
The port uses a separate `kit/` submodule checkout based on `30fb57d`, preserving
the neighboring shared recomp-kit checkout. Runtime changes are local and
uncommitted. Original game files and generated output remain ignored.

| Input | Entry | Base | Image size |
| --- | --- | --- | --- |
| MFatigue.exe | `004c6e5e` | `00400000` | `0017c000` |
| _unsupported/DirectXRendEng.dll | `1002721a` | `10000000` | `000b6000` |

Both PE32 images are SHA-256 pinned in `game.toml`; neither is patched. The main
image has 136 imports and 985 code/data exports. The renderer imports 48 of those
exports. `tools/inspect_game.py` regenerates the private inventory.

The GOG configuration initially chooses Glide. `tools/run_probe.py` seeds an
isolated registry with `driver:0=Direct3D,1,0`, using the game's own driver loader.
The writable profile overlay preserves the supplied installation. `_inmm.dll`
cannot be loaded at its preferred base because it collides with the renderer at
`10000000`; its reached APIs are handled through runtime shims.

## Address and translation discipline

Unidentified hooks and globals remain sentinels in zero-filled `.rsrc` page
padding at `0057be00..0057be73`. They are not recovered game objects. Physical
mouse motion uses the kit's relative-motion fallback; no guessed cursor object
has been installed. The guest arena is `0x11000000` bytes to cover the renderer.

Ghidra 12.1.3 exported 2,640 main functions and 466 renderer functions. The
translator recovers additional code from the pinned images. The latest completed
build including the gameplay entry reported:

| Image | Functions | Dispatch entries | Table sites | Table entries |
| --- | ---: | ---: | ---: | ---: |
| Main | 5,857 / 5,857 | 25,663 | 88 | 1,247 |
| Renderer | 991 / 991 | 3,414 | 46 | 311 |

All counted table sites decode with zero dispatch gaps. This is static coverage,
not proof that every computed runtime target is represented. In particular, the
previously withdrawn speculative method `00490ba0` was subsequently reached
during mission loading and is now a reviewed explicit entry in `game.toml`.
Its complete recovery also merges an overlapping fragment, hence the function
count does not increase. No speculative functions are withdrawn in this build.

`translate.resumable_stacks=true` registers CALL continuations and checks the
returned guest EIP before continuing a native caller. A switch to another guest
stack unwinds to `recomp_run`, which resumes the saved continuation. This is
needed by the original cooperative scheduler at `004c3b40/004c3b77`. Existing
ports retain the default behavior unless they opt in. Generated C is never edited.

The renderer's font vtables require explicit methods `1000d250` and `1000d860`,
observed through the shared virtual call at `1000bad0`. Auxiliary discovery now
records executable section addresses, and each translation filters a mixed
EXE/DLL discovery file to the image it is translating.

## Failure chronology and fixes

1. `RegOpenKeyExA(HKLM,NULL,...)` failed when the registry contained only child
   keys. Root reopening now succeeds for null/empty subkeys, including A/W
   variants. Before fix: `build/runs/headless-74795yh1/host.log`.
2. Renderer imports of main-executable data/code were bound to host shims.
   The loader now resolves actual mapped main exports by name/ordinal and
   validates their bounds. Before fix: `headless-p5vv02az`.
3. `IDirect3D3` was unavailable. Versioned D3D3/Device3/Viewport3/Material3
   vtables now implement the reached paths, FVF input, texture ownership,
   single-stage texture state and Clear2. Unsupported methods return errors
   with correct ABI cleanup. Initial blocker: `headless-acoiq3s5`, `app-pph66mzv`.
4. Missing stdcall implementations left their arguments on the guest stack.
   Decorated `_name@bytes` imports now retain stack cleanup; startup shims add
   system locale, bounded GetUserNameA, CloseWindow/IsIconic and honest AVI/MCI
   failure behavior. Audio implementations are still incomplete.
5. Cooperative guest-stack switching diverged from native call frames, causing
   null virtual calls. Resumable translation fixes the observed startup path.
   `headless-6903lm62` reached 100 frames, 92 scenes and 35,272 D3D draw calls,
   then guest ExitProcess(0). Headless returned 4 because its captured buffers
   were uniform: it does not rasterize these D3D scenes.
6. Native app runs displayed the main menu and campaign selection. Further
   interaction exposed the missing font entries and `_INMM!mciSendCommandA`
   cleanup. Those are fixed. Evidence: `app-xh1jqy7t`, `app-epqt0i0a`,
   `app-tuf62dbp`.
7. `app-_p9gduo7` reached renderer jump `10026582` into omitted `100265af`.
   Its unaligned table at `100390ca` is indexed by pre-scaled byte offsets,
   rather than `index*4`. Table recovery now recognizes that shape with
   consecutive executable-pointer evidence. Eight additional table sites
   and 64 table entries were recovered. Build: `build/build-byte-tables.log`.
8. `app-ag4p7sla` loaded `TBD/Z_01/MissionInfo.tbd`, objects, toolbar, tech tree
   and background, then reached `00490ba0` 256 times from the virtual call
   returning to `004b1434`. Its missing RET-8 cleanup corrupts subsequent
   execution, ending at a null call returning to `004a22ff`. The verified
   gameplay method is now an explicit main entry and the rebuilt app includes
   both branches. Mission validation is in progress.
   Log: `build/build-gameplay-entry.log`.
9. `app-5tz79ido` runs the mission timer, minimap and HUD, but the main world
   is black. LLDB captured 120 draw calls in `build/black-draws.json`; sampled
   textured world vertices already had screen x around -3,000 before host
   rendering. The renderer projection at `100264a0` matches original x86
   execution for the captured inputs (`build/projection.json`). Tracing its
   matrix identified `004c3cc7`, encoded `DC CB`: multiply ST3 by ST0. Ghidra
   prints this as `FMUL ST3`, which the translator incorrectly treated as the
   D8 form writing ST0. The captured matrix multiply's translation component
   was -1399 in native execution versus -95 in original x86 execution with
   identical inputs (`build/multiply.json`). Instruction bytes now distinguish
   both destinations, including prefixed forms. Thirteen synthetic cases cover
   add, multiply, subtract and divide directions against Unicorn. The rebuilt
   `app-xdpp3bws` displays terrain and units. The user confirmed selecting a red
   unit and moving it to nearby ground. Portrait/HUD flicker remains; the user's
   screenshot also shows stale cursor arrows along the bottom toolbar.
10. Frame captures in `build/seals.json`, `build/buffers-states.json` and
    `build/hud-dirty.json` show primary Flip boundaries, two guest buffers and
    incremental HUD redraws. The host can discard all drawing for a frame when
    its target pool is full, losing retained-surface updates even though the
    guest advances its dirty-rectangle ring. `app-xdpp3bws/host.log` reports
    target exhaustion. First-write acquisition now waits up to 250 ms for
    completion before reporting a stalled-pool fault; the renderer's slots
    cover all seven presenter targets plus a spare. The rebuilt `app-6uqvzki0`
    reached a mission and the user confirmed: "the flickering is fixed".
    Its inspected log has no target-pool fault. The overlay's drop counter also
    counts discarded completed mailbox frames, so that counter alone does not
    mean guest draws were lost. Cursor-trail clearing was not separately
    confirmed. Build and checks: `build/build-hud-pressure.log`,
    `build/build-tests-hud.log`, `build/presenter-hud.log`,
    `build/presenter-gpu-hud.log`, `build/dx-hud.log` and
    `build/port-hud-tests.log`.

11. The reached media imports were incomplete. Miles `_AIL_waveOutOpen@16`
    now writes a real driver handle; digital preferences/configuration, master
    gain and named MP3 effects are implemented. `[media].cd_tracks` maps the
    original data track plus `MUSIC/Track02.ogg` through `Track23.ogg` to the
    Miles redbook API. WinMM and `_INMM` waveform output decode PCM/IMA ADPCM
    and finish WAVEHDRs according to the device's played-byte clock. Callback
    modes and loop headers remain explicitly unsupported; MCI remains unavailable.
    AVIFile reads original compressed samples and stream metadata; Video for
    Windows decodes IV50 through packaged FFmpeg. `ICDecompress` is cdecl,
    unlike the other reached codec entry points, and IV50 uses YUV410P here.
    `app-q1i0fhpa` and `app-99vs1pld` visibly play the logo and opening scenes.
    The latter trace reports a running 48 kHz stereo device. The previous run
    also queued menu music continuously. The user subsequently confirmed
    "Music and effects are audible" and "Animation and audio both work".
    The private-asset probes decode 150 frames each: logo changes 147 times,
    opening changes 55 times, both nonblack. No original media was modified.
12. The campaign robot widgets are separate from the AVI player. Read-only
    LLDB inspection found the live robot children and their graphics descriptors.
    Draw capture `build/black-draws.json` recorded COLORKEYENABLE=1 while alpha
    testing and blending were disabled. Texture upload already converted key
    pixels to zero alpha, but the host ignored the required implicit test and
    painted the black background over other graphics. The host now supplies
    GREATER/0 alpha testing for keyed RGB textures when requested by that state
    and when the guest has not set an explicit alpha test. A Metal pixel test
    verifies both enabled and disabled states. This follows the legacy behaviour
    described in [NVIDIA's colour-key reference](https://developer.download.nvidia.com/assets/gamedev/docs/ColorKey.pdf).
    The user verified campaign selection/animation in `app-99vs1pld`; CUA also
    observed its mission terrain and unit portrait. A subsequent log review
    found an empty AVI sample near the end of the opening cinematic. Empty
    video samples and ICDECOMPRESS_NULLFRAME now retain the previous image
    instead of reporting a codec error or flushing FFmpeg. This last edge-case
    correction is included in the rebuilt app.
    Full private-asset decode now passes all six original AVIs (15,656 frames),
    including 55 repeated empty frames in OpeningCinematic and 449 in Finale.
    This is complete decode evidence, separate from the live opening playback
    and user-confirmed campaign animation/audio.
13. `app-99vs1pld` reached `ExitProcess(0)` after 481 seconds, then aborted during
    static waveform destruction after the SDL audio mutex had been destroyed.
    `waveout_shutdown()` now releases abandoned handles at the shared boot
    teardown seam, after guest workers stop and before host audio teardown.
    A regression checks an open, playing handle is reclaimed exactly once and
    repeated shutdown is harmless. The rebuilt `app-ukhqr_sr` reached live movie
    audio, but a settings interaction hit the older zero guest call /
    `0040b6d0` failure before normal teardown. This was initially described as
    a close failure; the user subsequently identified resolution changes as
    the trigger. The resolution fix and native clean-exit evidence follow below.

The earlier HUD run `app-6uqvzki0` later exited with `Error in timeline triggers`,
a zero-address guest call returning to `0040b6d0`, and unsupported exception
continuation. That occurred before the media build. Its return address matches
the renderer-reload failure identified below, although its original trigger
was not recorded. Sustained mission stability remains a separate check.

## Resolution-change crash (2026-09-20)

The user identified resolution as the setting that crashed. The guest loop at
`0040ca80` handles `RebootApp` by calling renderer teardown `0040a800`, rereading
settings at `0042d4a0`, then initialization `0040b4d0`. Teardown clears exported
`pRendEng` and calls FreeLibrary. Previously that shim did nothing, and the next
LoadLibrary skipped the already-attached renderer's entry point. The call at
`0040b6cd`, returning to `0040b6d0`, consequently used the cleared renderer.
The renderer's actual DllMain at `1000aed0` creates and publishes that object
on DLL_PROCESS_ATTACH; its CRT wrapper also owns mutable initialization state.

Auxiliary DLLs now count LoadLibrary references and call DLL_PROCESS_DETACH on
the final release. A fresh load restores a retained snapshot of the verified,
IAT-patched image before running DLL_PROCESS_ATTACH again. This restores CRT
globals and preserves code/data import bindings without modifying originals or
bypassing hashes. GetModuleHandle does not acquire a reference or initialize
an unloaded auxiliary DLL. Failed attach detaches and reports error 1114. The
main executable cannot be unloaded or initialized as a DLL; shim-only libraries
retain their existing process lifetime.

A synthetic translated DllMain regression exercises the actual pinned module
mapping and Win32 import dispatch: failed attach, three reload cycles, multiple
references, final detach, double-release rejection, pristine image/import bytes,
and preservation of the caller's stack and return address. Commands:

```sh
.venv/bin/python kit/tools/format.py --write
.venv/bin/python tools/test.py --compile-only --jobs 8
RECOMP_PYTHON="$PWD/.venv/bin/python" RECOMP_PROFILE_DIR="$PWD/build/test-profile" build/recomp/runtime_tests --startup-contracts
RECOMP_PYTHON="$PWD/.venv/bin/python" RECOMP_PROFILE_DIR="$PWD/build/test-profile" build/recomp/runtime_tests
.venv/bin/python tools/build.py --target app --jobs 8
.venv/bin/python kit/tools/check_game_literals.py
.venv/bin/python tools/run_probe.py --host app --seconds 3600 --frames 0 --capture-audio
```

Native run `app-qukr003r` restarted its renderer twice and continued rendering;
the first restart changed the logged mode from 640x480 to 1024x768, and the
second recreated 1024x768. CUA observed a rendered mission. The user confirmed
that changing resolution and changing it back both worked. This establishes
repeated renderer restart, not support for every advertised display mode.
The same process then called ExitProcess(0), and the probe returned exit code
0, also verifying native clean teardown after the earlier waveform fix. No
zero-target call or unsupported exception continuation occurred in this run.
The game still prints `Error in timeline triggers` during some transitions;
that message alone does not indicate the former resolution crash.

Logs: `build/build-tests-resolution.log`, `build/startup-resolution.log`,
`build/runtime-resolution.log`, `build/build-app-resolution.log`, and
`build/runs/app-qukr003r/host.log` / `result.json`. The run has ended cleanly.

Every probe folder named above is under ignored `build/runs/`. Actual app
screens were inspected through CUA. No menu/counter result establishes a
playable mission, audio fidelity or a frame-rate target.

## Reproduce

See README for setup, analysis and build commands. For interactive validation:

```sh
.venv/bin/python tools/build.py --target app --regenerate --jobs 8
.venv/bin/python tools/run_probe.py --host app --seconds 1200 --frames 0 --trace-files
```

The app currently disables the internal time/frame caps. The probe's outer
process timeout is `--seconds + 45`; closing its window exits sooner. Each run gets isolated registry/profile state by default; `--profile` reuses a
player profile. Logs and missing-address discovery remain per run. The
headless host honors its frame cap but cannot provide D3D pixel evidence.

## Validation

| Check | Latest result |
| --- | --- |
| Port input + kit config/setup/literal tests | 35 passed |
| Portable kit suite | 410 passed, 3 skipped |
| Native test build through tools/test.py | Passed |
| Startup contracts including stack switch and DLL reload | 141 checks, 0 failures, 1 skipped |
| DirectX suite | 141,126 checks, 0 failures |
| Host offscreen Metal suite after colour-key fix | 3,737,891 checks, 0 failures |
| All six original AVI files, full decode | 15,656 frames, no decode failures |
| Presenter queue tests after HUD change | 294 checks, 0 failures |
| Presenter Metal pixel tests after HUD change | 131 checks, 0 failures |
| Full runtime suite | 1,271 checks, 8 failures, 2 skipped |
| Native resolution changes and subsequent exit | User confirmed both changes work; process exit 0 |

Logs: `build/portable-x87-direction.log`, `build/build-tests-stacks.log`,
`build/startup-checkpoint.log`, `build/dx-checkpoint.log`, and
`build/runtime-checkpoint.log`. The new cooperative-stack test executes the
worker and restored CALL continuation in order, verifies the original stack
return and preserves the suspended stack. Translator tests check emitted
continuation entries/guards and byte-offset table recovery.

Seven full-runtime failures assume a VERSIONINFO resource absent from this
executable. The eighth now reports six remaining imports without argument metadata;
reached startup paths were corrected individually. Full runtime success has not
been claimed. The focused skip is the absence of imported main-image data;
renderer imports of exported main data are tested separately.

Media/colour-key checks: `build/build-media-final.log`,
`build/build-tests-media-final.log`, `build/dx-media-final.log`,
`build/host-gpu-media.log`, `build/portable-media-final.log`,
`build/avi-logo-final.log` and `build/avi-opening-final.log`.
Full-length checks are in `build/avi-full-*.log`.
The generated IMA fixture checks decoded sample values and that completion
uses 18 decoded bytes rather than eight compressed bytes. To rerun the private
asset probe without redistributing it:

```sh
RECOMP_TEST_AVI="$PWD/original/gog/TBD/LogoCinematic.avi" build/recomp/dx_tests
RECOMP_TEST_AVI="$PWD/original/gog/TBD/OpeningCinematic.avi" build/recomp/dx_tests
RECOMP_TEST_AVI="$PWD/original/gog/TBD/Finale.avi" RECOMP_TEST_AVI_FRAMES=0 build/recomp/dx_tests
build/recomp/host_tests --gpu-only
```

## Remaining work

- Continue sustained mission checks, including cursor-trail clearing across
  the toolbar, combat and mission transitions. Terrain, movement and the
  portrait/HUD flicker correction now have native user confirmation.
- Continue following real missing-code/API failures with saved run evidence.
- Broaden resolution checks to additional display modes and mission contexts;
  repeated renderer restart and a clean native exit are now verified.
- Continue sustained mission and cinematic transition checks. Music/effects
  and campaign animation now have user confirmation; this does not establish
  every scripted event, audio fidelity or completion of a campaign.
- Identify genuine game hooks where required; keep unknown sentinels explicit.
- Establish simulation timing, stable rendering and input before benchmarking.


## High resolutions, persistent settings and publication (2026-09-20)

The original six Graphics buttons now select 640×480, 800×600, 1024×768,
1920×1080, 2560×1440 and 3840×2160. The game-specific core plugin changes the
reviewed mode table at `100380c0` during the renderer's attach, at the main
executable's `rendmalloc` call (`0040d470`, return PC `1000aee6`). It updates
button labels after the Graphics panel constructor at `00415560`.

Native adaptations live in `native/display.c`, selected through the configured
override header. Mode enumeration (`10002e60`) validates physical modes and then
uses 1280×720 logical dimensions for HD slots. Renderer initialization
(`1000b620`) requests the selected physical output size from the host. This
keeps HUD layout, camera limits and input in one logical coordinate space while
the GPU rasterizes at the requested resolution. The callback must complete its
work before the guest return sentinel; wrapping the original function and doing
work after its return did not run reliably through the callback driver.

The scaled-font routine (`1000d860`) used `(scale - 1) / 32` to inset each glyph's
atlas coordinates. At enlarged menu scales that collapsed or reversed the UV
span. Its reviewed `1/32` constant at `10031428` is temporarily zeroed only while
the original leaf font routine runs, then restored. Other renderer users of the
constant retain their original behavior. Text still uses the original bitmap
artwork; it has not been replaced by vector fonts.

The presenter accepts a requested output size for subsequent frame targets;
leased frames retain their size. F10 choices are loaded once per player profile
and survive renderer recreation. `tools/play.py` uses `build/profile` by default,
with registry and host settings stored separately. The display plugin fills
missing first-run registry values through the pinned imported APIs before
`0042d4a0` reads options. Existing values are preserved. Hook `guest_call` now
accepts allocated import trampolines as well as translated functions.

Live verification: run `app-kr3kjmoc` used a 1280×720 guest canvas and requested
3840×2160 output. The user confirmed that HUD, text, camera, unit control and
F10 rows 1 and 2 all worked, including resolution changes. The earlier small-HUD
report referred to a previous build. This run exited with status 0. A separate
restart through `tools/play.py --profile build/profile` visually retained
fullscreen and the disabled performance overlay; it was quit normally and no
game process remained. These observations do not establish campaign completion
or sustained 4K/120 performance.

Fresh-profile headless startup loaded `metal-fatigue.display`, created the
DirectX registry defaults, attached the renderer and selected 640×480 without
the probe's registry seeding. The headless diagnostic stop required the host's
bounded unwind; it is not an additional clean-exit claim.

Focused checks after the final display changes:

| Check | Result |
| --- | --- |
| Portable tooling | 415 passed, 3 skipped |
| Pinned private input assertions | 5 passed |
| Native display adaptation | 96 checks, 0 failures |
| Presenter queues and output size | 304 checks, 0 failures |
| Presenter Metal pixels | 131 checks, 0 failures |
| F10 persistence and hook import delegation | 39 checks, 0 failures |

The hook regression checks stdcall arguments, return values, complete register
restoration and rejection of unallocated/misaligned trampoline addresses.
The display regression exercises real guest callback unwinding over repeated
renderer initialization. The broad mods suite also contains Populous-specific
settings fixtures; those fixtures are not applicable to this game. Use the
focused command in `docs/testing.md` instead of treating that suite as a complete
Metal Fatigue gameplay check.

The translated iOS arm64 application and Android arm64 APK built successfully.
iOS signing verification passed; both binaries contain the compiled display
plugin, and the Android APK contains its manifest. No mobile device was installed
or used to establish gameplay. iOS packaging now falls back to the original
same-named sibling ICO when the valid executable has no embedded icon. Android
unpacks packaged core manifests before guest startup. Static libraries now live
in each preset's own build directory, preventing one platform's build from
replacing another's libraries.

Logs: `build/build-app-first-run.log`, `build/build-headless-first-run.log`,
`build/build-tests-first-run.log`, `build/mods-first-run.log`,
`build/portable-publication-final.log`, `build/game-inputs-final.log`,
`build/presenter-highres.log`, `build/presenter-gpu-highres.log`,
`build/restart-profile.log`, `build/build-ios-first-run.log` and
`build/build-android-first-run.log`. All artifacts, generated translations,
original game inputs and player profiles remain local and ignored. Public CI
builds link the host against a stub translation; they do not run the game.

### Final cross-platform build check

The final translated Linux arm64 and Windows x86-64 applications both linked and
packaged successfully. Linux used Ubuntu 24.04 with clang 18 inside a local
container; Windows used llvm-mingw 20260908 UCRT from the same container. The
toolchain archive SHA-256 was
`c907dd2302a292b663add18752a55ce9d544ca0fd19cf33c086ca242d5994ea6`.
Generated code remained on this machine. Linux includes FFmpeg shared libraries.
At this checkpoint the Windows cross preset had FFmpeg disabled, so that
artifact could not play AVI movies or Ogg CD tracks. This limitation is fixed in
the media cross-build check below. Neither build establishes gameplay on its
target OS.

The cross-build found missing `<cstdlib>` includes and a duplicate default core
plugin table on COFF. The empty table now lives in a fallback archive, loaded only
when the host has no compiled plugin table. Linux, Windows, macOS, iOS and Android
were rebuilt after this change. The focused hook/settings regression still passed
all 39 checks. Both desktop packages contain the core display manifest, and the
apps compile its plugin directly. The cross-build packager now stages the Windows
output directory rather than looking for the host OS's binary.

Private local artifacts:

| Target | Artifact |
| --- | --- |
| macOS arm64 | `build/MetalFatigueRecomp.app` |
| iOS arm64 | `build/ios/Release/MetalFatigueRecomp.app` |
| Android arm64 | `build/android/app/build/outputs/apk/debug/app-debug.apk` |
| Linux arm64 | `build/platforms/linux/build/package/MetalFatigueRecomp-linux-aarch64.tar.gz` |
| Windows x86-64 | `build/platforms/windows/build/windows/package/MetalFatigueRecomp/` |

Final build logs: `build/build-app-platform-final.log`,
`build/build-ios-platform-final.log`, `build/build-android-platform-final.log`,
`build/platforms/linux-final.log` and `build/platforms/windows-final.log`.
The reused isolated startup profile retained `Resolution=5` and `MusicLevel=7`,
confirming that first-run defaults do not overwrite existing preferences. The
interactive game remained closed during packaging and publication.


### Windows movies and Ogg cross-build (2026-09-21)

The `windows-cross` and inherited stub preset now enable FFmpeg. Dependency
configuration distinguishes the build host from the Windows target: Linux/macOS
use their POSIX shell and GNU make, with an explicit Windows architecture and
llvm-mingw compiler, binutils, resource compiler and cross prefix. The prefix is
required for FFmpeg's `dlltool` invocation; selecting only the compiler leaves
import-library installation broken. The AVI adapter now includes `<cstdlib>`
for `std::abs`, and the GDI test includes `<algorithm>` for `std::fill`, which
llvm-mingw's libc++ does not supply through unrelated headers.

Using the same Ubuntu arm64 container and llvm-mingw 20260908 toolchain recorded
above, the full translated Windows app and every native test binary built:

```sh
docker run --rm \
  -v "$PWD/build/platforms/windows:/work" \
  -v "$PWD/build/platforms/toolchain:/toolchain:ro" \
  -e LLVM_MINGW_ROOT=/toolchain -w /work metal-fatigue-builder:local \
  python tools/build.py --preset windows-cross --target app --jobs 6

docker run --rm \
  -v "$PWD/build/platforms/windows:/work" \
  -v "$PWD/build/platforms/toolchain:/toolchain:ro" \
  -e LLVM_MINGW_ROOT=/toolchain -w /work metal-fatigue-builder:local \
  python kit/tools/test.py --game-dir /work --preset windows-cross --compile-only --jobs 6
```

The package at `build/platforms/windows/build/windows/package/MetalFatigueRecomp/`
contains `MetalFatigueRecomp.exe`, `avformat-61.dll`, `avcodec-61.dll`,
`avutil-59.dll`, the core display manifest and `resources/ffmpeg-NOTICE.md`.
PE inspection confirms AMD64 throughout: the executable imports these DLLs,
and their remaining imports are Windows system/UCRT libraries. No additional
compiler runtime DLL is required. FFmpeg's `config_components.h` enables
Indeo 5, Vorbis, AVI and Ogg.

For execution checks, `dx_tests.exe` and only the three DLLs copied from the
package were placed in `build/platforms/media-check/`. CrossOver ran them using
a new isolated `media-check` bottle under `build/platforms/wine-bottles/`, leaving
existing bottles untouched. Environment variables supplied Windows `Z:` paths
to the local original assets:

| Probe | Result |
| --- | --- |
| `RECOMP_TEST_AVI`, `TBD/LogoCinematic.avi` | 150 frames decoded through AVIFile/Indeo imports; 147 changing images, nonblack output, exit 0. |
| `RECOMP_TEST_AUDIO`, `MUSIC/Track02.ogg` | 355,584 interleaved samples, 44,100 Hz stereo, peak 10,291, exit 0. |
| Portable tooling | 415 passed, 3 skipped. |
| Game-specific assertions | 5 passed. |

The audio probe uses the same `mf::Media` decoder as file-backed CD music.
These are headless Windows-binary decoder checks under Wine, not audible output
or native Windows gameplay. The game stayed closed. A new public CI job builds
the Windows stub with media, checks codec configuration, and decodes a generated
Ogg sine tone with isolated copies of the Windows DLLs. No game assets are used
or uploaded by CI.

Local evidence: `build/platforms/windows-media-build.log`,
`build/platforms/windows-media-tests-build.log`, `build/platforms/windows-avi-probe.log`,
`build/platforms/windows-ogg-probe.log`, `build/platforms/media-portable-tests.log`
and `build/platforms/media-game-tests.log`.
