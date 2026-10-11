# Audacity

[![Coverage](https://s3.us-east-1.amazonaws.com/extensions.musescore.org/test/code_coverage/au_coverage_badge.svg)](https://github.com/audacity/audacity/actions/workflows/au4_check_unit_tests.yml)

[Audacity](https://www.audacityteam.org) is a free, open-source, multi-track audio editor and recorder for Windows, macOS, GNU/Linux, and other operating systems. More information is available at [audacityteam.org](https://www.audacityteam.org).

This repository is the source for **Audacity 4**. Audacity 4 is an active rewrite with a new Qt-based user interface, while the underlying audio engine remains derived from Audacity 3.x. Because of this rewrite, the `master` branch is currently in flux and not recommended for end-user production work.

## Release branches

- Use branch **`master`** for Audacity 4 development. Build instructions are in [BUILDING.md](./BUILDING.md) and summarized below.
- Use branch **`audacity3`** to patch Audacity 3.x. Build instructions for 3.x are in the [release-3.7.0 `BUILDING.md`](https://github.com/audacity/audacity/blob/release-3.7.0/BUILDING.md).

End users who need a stable editor should download a release build from [audacityteam.org](https://www.audacityteam.org) rather than building from this branch.

## Requirements

- Git
- CMake 3.24 or later
- A CMake generator such as Ninja
- A C++ compiler that supports C++20 (tested: MSVC 2022 on Windows, g++ on Linux, Clang on macOS)
- Qt 6.10 with these additional libraries:
  - Qt 5 Compatibility Module
  - Qt Network Authorization
  - Qt Shader Tools
  - Qt State Machines
- A system package manager (tested: Chocolatey on Windows, Homebrew on macOS)

Older Qt versions are available through the Qt Online Installer by selecting **Show > Archive**.

## Get the source

```bash
git clone --recurse-submodules https://github.com/audacity/audacity.git
cd audacity
```

The `--recurse-submodules` flag is required because the MuseScore framework and other dependencies are included as submodules.

## Build from the command line

```bash
# Configure (run once)
cmake -S . -B build/ [options]

# Build
cmake --build build/

# Install
cmake --install build/
```

Common optional CMake arguments:

- `-DCMAKE_BUILD_TYPE=Release` or `Debug`
- `-DAU4_BUILD_MODE=dev|testing|release`
- `-DAU4_BUILD_CONFIGURATION=app|app-portable|utest`
- `-DMUSE_ENABLE_UNIT_TESTS=ON` to build unit tests

## Build with Qt Creator

1. Open `CMakeLists.txt` as a project in Qt Creator.
2. Select the auto-detected Qt kit.
3. Click **Build**.

This is the recommended workflow for QML-heavy development because it gives the best completion and debugging support.

## Build with Visual Studio (Windows)

Double-click `generate_sln.bat` in the repository root. It generates a Visual Studio solution in the `build/` directory, builds the `install` target, and opens `build/audacity.sln`. Press **F5** to run.

## Build with Visual Studio Code (Windows)

1. Open `.vscode/audacity.code-workspace`.
2. Install the recommended extensions when prompted.
3. Press **Ctrl+Shift+P** and run **CMake: Configure**.
4. Press **F5** to build and run.

## Run tests

If the project was configured with tests enabled:

```bash
cmake --build build/ --target test
```

## Deployment and packaging

The build system can create platform-specific packages:

- Linux / BSD: AppImage packaging via `buildscripts/packaging/Linux+BSD/SetupAppImagePackaging.cmake`
- Windows: installer and portable setup via `buildscripts/packaging/Windows/SetupWindowsPackaging.cmake`
- macOS: packaging is handled through the CMake install step and associated bundle scripts

Run `cmake --install build/` first, then invoke the packaging target if one is available for your platform.

## Contributing

See [CONTRIBUTING.md](./CONTRIBUTING.md) for ways to help. Code contributions require signing the [Contributor License Agreement](https://www.audacityteam.org/cla/). Development discussion happens on [Discord](https://discord.gg/audacity), and the developer guide is at [audacity.gitbook.io/dev](https://audacity.gitbook.io/dev/).

## License

Audacity is open-source software licensed under GPLv3. Most source files are GPLv2-or-later. Third-party libraries in `/au3/lib-src` and VST3-related code have their own licenses. Documentation is licensed under CC BY 3.0 unless otherwise stated. See [LICENSE.txt](./LICENSE.txt) for full details.

## Important caveats

- This `master` branch is in major structural change for Audacity 4. It is not a stable end-user release.
- Build instructions are a work in progress and will be finalized closer to the Audacity 4 release.
- The dependency list is currently long because MuseScore dependencies have not yet been cleaned up.
