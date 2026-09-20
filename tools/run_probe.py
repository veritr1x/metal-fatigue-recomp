#!/usr/bin/env python3
"""Boot the translated entry point with an isolated profile and DirectX settings.

This is a diagnostic probe, not a gameplay pass. Return the host's exit status
and retain logs, discovered addresses and frame captures for inspection.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "kit/tools"))
import game_config


def registry_seed(guest_root):
    """Use the game's own driver selection (0042d4a0 -> 0042da30).

    Driver strings are '<description>,<renderer type>,<device index>'. Type 1
    selects DirectXRendEng.dll; these settings never modify the original PE.
    """
    values = {"RegVersion": 0x101, "Driver": 0, "NumDrivers": 1,
              "Resolution": 0, "ColorDepth": 0, "Rendering": 8,
              "Particles": 1, "ScrollRate": 5, "PlayVoice": 1, "PlayMusic": 1,
              "SoundLevel": 8, "MusicLevel": 5, "VoiceLevel": 8,
              "Gamma": 0x3f800000}
    values = {key: {"type": 4, "data": value} for key, value in values.items()}
    values["driver:0"] = {"type": 1, "data": "Direct3D,1,0"}
    values["CDPath"] = {"type": 1, "data": guest_root + "\\"}
    return {r"HKEY_LOCAL_MACHINE\SOFTWARE\Psygnosis\Metal Fatigue": values}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--seconds", type=int, default=20)
    parser.add_argument("--host", choices=("headless", "app"), default="headless")
    parser.add_argument("--frames", type=int, default=120, help="Frame cap; zero disables it")
    parser.add_argument("--frame-every", type=int, default=1)
    parser.add_argument("--trace-files", action="store_true")
    parser.add_argument("--trace-pointer", action="store_true")
    parser.add_argument("--trace-d3d", action="store_true")
    parser.add_argument("--capture-audio", action="store_true", help="Capture mixer output and audio calls")
    parser.add_argument("--profile", type=Path,
                        help="Reuse this profile and its registry; otherwise use an isolated profile")
    args = parser.parse_args()
    if args.seconds <= 0:
        parser.error("--seconds must be positive")
    if args.frames < 0 or args.frame_every <= 0:
        parser.error("--frames must be nonnegative and --frame-every positive")
    cfg = game_config.load(ROOT)
    binary = (ROOT / "build/recomp/pop_headless" if args.host == "headless" else
              ROOT / "build" / (cfg["game"]["app_name"] + ".app") /
              "Contents/MacOS" / cfg["game"]["app_name"])
    if not binary.is_file():
        parser.error(f"Missing {binary}: run tools/build.py --target {args.host}")
    runs = ROOT / "build/runs"
    runs.mkdir(parents=True, exist_ok=True)
    case = Path(tempfile.mkdtemp(prefix=args.host + "-", dir=runs))
    profile = args.profile.resolve() if args.profile else case / "profile"
    profile.mkdir(parents=True, exist_ok=True)
    registry = profile / "registry.json" if args.profile else case / "registry.json"
    if not registry.exists():
        registry.write_text(json.dumps(registry_seed(cfg["game"]["guest_root"]), indent=2) + "\n")
    env = {key: value for key, value in os.environ.items() if not key.startswith("RECOMP_")}
    env.update(RECOMP_PROFILE_DIR=str(profile), RECOMP_REGISTRY=str(registry),
               RECOMP_LOG="1", RECOMP_IMPORT_STATS="1",
               RECOMP_MAX_SECONDS=str(args.seconds), RECOMP_MAX_FRAMES=str(args.frames),
               RECOMP_FRAMES=str(case / "frames"), RECOMP_FRAME_EVERY=str(args.frame_every),
               RECOMP_DISCOVERY=str(case / "discovered.txt"), RECOMP_RUN_RECORD=str(case / "run.json"))
    if args.capture_audio:
        env["RECOMP_HOST_AUDIO_CAPTURE"] = str(case / "audio.wav")
        env["RECOMP_HOST_TRACE_AUDIO"] = "1"
    if args.trace_d3d:
        env["RECOMP_HOST_TRACE_D3D"] = "1"
        env["RECOMP_HOST_DUMP_EVERY"] = str(args.frame_every)
        env["RECOMP_HOST_DUMP_DIR"] = str(case / "frames")
    if args.trace_files:
        env["RECOMP_TRACE_FILES"] = "1"
    if args.trace_pointer:
        env["RECOMP_TRACE_POINTER"] = "1"
    print(f"Probe artifacts: {case}", flush=True)
    print(f"Player profile: {profile}", flush=True)
    with (case / "host.log").open("w") as log:
        try:
            result = subprocess.run([str(binary)], cwd=ROOT, env=env,
                                    stdout=log, stderr=subprocess.STDOUT, timeout=args.seconds + 45)
            code = result.returncode
        except subprocess.TimeoutExpired:
            code = 124
    (case / "result.json").write_text(json.dumps({"exit_code": code, "host": args.host,
                                                  "seconds_cap": args.seconds}, indent=2) + "\n")
    print(f"Host exit: {code}; inspect {case / 'host.log'}")
    return code


if __name__ == "__main__":
    sys.exit(main())
