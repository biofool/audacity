# Audacity — welcome tour fork

This is the `feature/welcome-tour` branch of `biofool/audacity`, a fork of upstream `audacity/audacity` (Audacity 4, `master`). It adds first-run onboarding on top of upstream:

- A six-step welcome tour shown once after first launch, built on the `muse::tours` framework (`muse/framework/tours`).
- Enriched tooltips (title + description + verified shortcut) on primary workspace controls.
- A `.claude/` post-edit lint hook for agent-assisted development.

Everything below the fork section is the upstream README. See `EXECUTIVE-SUMMARY.md` for the non-technical overview and `TECHNOLOGY-UNDERPINNINGS.md` for implementation details.

## Setup

```bash
git clone --recurse-submodules https://github.com/biofool/audacity.git
cd audacity
git checkout feature/welcome-tour
```

The `muse` and `muse_deps` git submodules are required — the tour framework lives in `muse/framework/tours`. In an existing clone, run `git submodule update --init --recursive`.

Requirements: Git, CMake, Ninja (or another generator), a C++20 compiler, and Qt 6.10 with the Qt 5 Compatibility, Network Authorization, Shader Tools, and State Machines add-ons. Full dependency list and platform notes: `BUILDING.md`.

## Build and run

```bash
cmake -S . -B build/          # configure (first build only)
cmake --build build/          # build
cmake --install build/        # install
```

Qt Creator (open `CMakeLists.txt`) gives the best QML debugging experience. On Windows, `generate_sln.bat` produces a Visual Studio solution.

## Using the tour

The tour needs no setup. Build and launch the app, open or create a project, and the six-step tour appears once the project page is the active view. It covers: audio setup, add track, record, select and edit, effects, save/share.

The tour is shown once per user profile. `muse::tours::ToursConfiguration` records shown tours under the `tours/lastShownTours` settings key. To see it again, clear that key in the app's settings store (a `QSettings`-backed file, e.g. under `~/.config/` on Linux), or delete the settings file.

## Caveats

- Audacity 4 `master` is under heavy development. Expect churn; upstream warns the branch is "not particularly friendly to new contributors."
- Tour steps target named navigation controls. If upstream renames a control URI (`audacity://<section>/<panel>/<control>`), that step is skipped silently — check the log for `Control not found` messages.
- This work has not been merged upstream. To submit patches to Audacity 3.x, branch off `audacity3` instead.
- Without the `muse` submodule checked out, the tree does not build — `framework/tours/itoursservice.h` and all other `framework/*` headers come from the submodule.

---

# Audacity (upstream README)

[![Coverage](https://s3.us-east-1.amazonaws.com/extensions.musescore.org/test/code_coverage/au_coverage_badge.svg)](https://github.com/audacity/audacity/actions/workflows/au4_check_unit_tests.yml)

[**Audacity**](https://www.audacityteam.org) is an easy-to-use, multi-track audio editor and recorder for Windows, macOS, GNU/Linux and other operating systems. More info can be found on https://www.audacityteam.org

## This repository is currently undergoing major structural change.

We're currently working on Audacity 4, which means an entirely new UI and also refactorings aplenty. As such, the `master` branch is currently not particularly friendly to new contributors. It is still possible to submit patches to Audacity 3.x; make sure you branch off `audacity3` if you choose to do so. Build instructions for 3.x can be found [here](https://github.com/audacity/audacity/blob/release-3.7.0/BUILDING.md); build instructions for Audacity 4 can be found [here](https://github.com/audacity/audacity/blob/master/BUILDING.md).

You can stay updated with our efforts on [YouTube](https://youtube.com/@audacity), [Discord](https://discord.gg/audacity) and [our blog](https://audacityteam.org/blog).

## License

Audacity is open source software licensed GPLv3. Most code files are GPLv2-or-later, with the notable exceptions being `/au3/lib-src` (which contains third party libraries), as well as VST3-related code. Documentation is licensed CC-by 3.0 unless otherwise noted. Details can be found in the [license file](LICENSE.txt).
