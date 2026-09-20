# Working on Metal Fatigue Recomp

Read README.md and docs/analysis.md first. The game configuration and reviewed
symbols live here; runtime and translator changes belong in the kit submodule.
Read kit/AGENTS.md before changing kit sources. Preserve the neighboring shared
recomp-kit checkout and other games.

- Keep original/, analysis/, build/, .venv/ and run profiles ignored.
- Never modify the supplied game files or bypass executable/module hashes.
- Guest pointers are 32-bit addresses, never host pointers. Unidentified hooks
  remain documented sentinels until verified from the pinned images.
- Edit translator rules, never generated C. Build through tools/build.py.
- Keep docs/analysis.md current with exact commands and observed failure points.
- Distinguish translation, compilation, startup, displayed frames, input and
  gameplay. None alone proves the next stage or a performance target.
