# Build and contribute

Start with the [README](README.md), [port analysis](docs/analysis.md) and
[testing guide](docs/testing.md). Use the pinned public `kit/` submodule; game
addresses and native adaptations belong here, shared runtime changes in the kit.

Supply your own supported game installation, then run the setup, analysis and
build commands in the README. Neither original images nor generated translations
belong in Git. Never bypass the executable or renderer hash to run another build.

Change translator rules in `kit/tools/recomp/translate.py`, never generated C.
Native overrides in `native/` must document their address provenance and preserve
the guest calling convention. Add focused regressions for callback, renderer or
settings changes; distinguish a successful build from a rendered and playable game.

Build through `tools/build.py` and `tools/test.py`. Format native source with the
kit's `.clang-format`. Keep private inputs, build products, logs, profiles and saves
under ignored directories. Run `git diff --check`, the relevant tests and the
kit's publication checks before committing. Update the changelog and analysis
with exact commands, observed results and unresolved failures.

Public CI never receives the game or its generated code. Its stub builds check
host compilation and linking, not game behavior. Do not upload locally packaged
game assets or iOS bundles to Actions artifacts or public releases.
