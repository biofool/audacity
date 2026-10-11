# Technology Underpinnings

This document describes the main features of Audacity 4 and how they are implemented. It references actual files and technologies in the repository.

## High-level architecture

Audacity 4 is a C++20 desktop application built with CMake. It uses the following stack:

- **Qt 6** for the user interface, primarily through **QML**.
- **MuseScore framework** (`muse/`) as the application framework, providing modules for actions, docking, workspaces, extensions, audio plug-ins, and networking.
- **Legacy Audacity 3 engine** (`au3/`) for project files, audio I/O, effects, and import/export. The new code wraps this engine through the `au3wrap` module.
- **CMake** for configuration and build orchestration, with per-module `CMakeLists.txt` files.

The entry point is in `src/app/main.cpp`, which sets up the Qt application, parses command-line options, and instantiates an application object through `src/app/appfactory.cpp`.

## Module system

The application is organized as modules that implement `muse::modularity::IModuleSetup`. The factory in `src/app/appfactory.cpp` registers modules in a specific order:

- Core Muse modules: diagnostics, actions, UI, workspace, dock, cloud, network, extensions.
- Audacity modules: app shell, context, project, project scene, audio, track editing, playback, recording, effects, import/export, spectrogram, automation.

Each module has its own `CMakeLists.txt` under `src/<module>/` and is added in `src/CMakeLists.txt`.

## User interface

The UI is implemented in QML and C++ view models.

- QML sources live under `src/<module>/qml/Audacity/<Module>/`.
- C++ view models expose data and actions to QML. For example, `src/projectscene/view/trackspanel/wavetrackitem.cpp` and `paneltrackslistmodel.cpp` back the track panel views.
- The track canvas and waveform rendering are implemented in `src/projectscene/view/tracksitemsview/`, with waveform painting in `WaveformPainter.cpp` and related files.
- The main application window, toolbars, and docked panels are provided by the Muse framework's `dockwindow` module.

## Project model and track editing

- `src/project/` manages the project lifecycle and current project context.
- `src/trackedit/` provides the domain model for clips, tracks, selections, and edit operations. Key files include `iclipsinteraction.h`, `itracksinteraction.h`, and `itrackeditproject.h`.
- `src/au3wrap/internal/au3project.cpp` maps the new project abstraction onto the legacy `AudacityProject` object in `au3/src/Project.cpp`.
- Undo/redo history is handled through the legacy project history mechanism.

## Audio engine

- `src/audio/` defines the abstract audio configuration and metering interfaces.
- `src/au3audio/` bridges the new code to the Audacity 3 audio engine, including playback and recording paths.
- The legacy engine uses **PortAudio** for host audio I/O, **libsndfile** and **FFmpeg** for file import/export, and project-specific SQLite storage in `au3/src/project-file-io/`.

## Playback and recording

- `src/playback/` implements playback controls, transport state, meters, and toolbars. QML files are in `src/playback/qml/Audacity/Playback/`.
- `src/record/` implements recording controls, meters, and the record controller (`recordmodule.h`, `recordcontroller.cpp`).
- Both modules coordinate with the audio engine through the `au3audio` bridge.

## Effects and plug-ins

The effects system supports several plug-in architectures:

- **Built-in effects**: implemented in `src/effects/builtin/` and `src/effects/builtin_collection/`.
- **Nyquist**: Lisp-based audio effects, enabled with `AU_MODULE_EFFECTS_NYQUIST` in `CMakeLists.txt`.
- **VST3**: enabled with `AU_MODULE_EFFECTS_VST`.
- **LV2**: enabled with `AU_MODULE_EFFECTS_LV2` on Linux.
- **Audio Units**: enabled with `AU_MODULE_EFFECTS_AUDIO_UNIT` on macOS.

Effect dialogs are loaded through QML. The architecture is documented in `docs/effect-view-architecture.md`. Real-time effects are non-destructive and can be reordered per track; destructive effects modify audio on request.

## Spectrogram

- `src/spectrogram/` adds frequency-domain visualization and selection support.
- QML views are in `src/spectrogram/qml/Audacity/Spectrogram/`.
- The feature draws on the legacy track and FFT code in `au3/`.

## Import and export

- `src/importexport/` is split into import, export, and label modules.
- The legacy importer/exporter registry in `au3/src/import-export/` is wrapped and initialized in `src/au3wrap/au3wrapmodule.cpp`.
- Label tracks are handled by `src/importexport/labels/`.

## Cloud integration

- `AU_BUILD_CLOUD_AUDIOCOM` enables integration with the audio.com cloud service. The module lives in `src/au3cloud/`.
- The Muse framework provides the generic cloud and network modules that this builds upon.

## Extensions and automation

- `src/extensions/` hosts Audacity-specific extension support.
- `src/automation/` provides automation APIs for scripting and external control.
- The Muse framework's `extensions` module handles plug-in loading and lifecycle.

## Build configuration

The top-level `CMakeLists.txt` declares:

- Build configurations: `app`, `app-portable`, `utest`.
- Build modes: `dev`, `testing`, `release`.
- Per-module options such as `AU_BUILD_PLAYBACK_MODULE`, `AU_BUILD_RECORD_MODULE`, and `AU_BUILD_EFFECTS_MODULE`.
- Audio libraries for time-stretching and pitch-shifting: `AU_USE_SBSMS` and `AU_USE_SOUNDTOUCH`.

`SetupConfigure.cmake` resolves the selected build mode and platform-specific packaging. Platform setup scripts are in `buildscripts/ci/<os>/` and `buildscripts/packaging/`.

## Testing

Unit tests are enabled with `MUSE_ENABLE_UNIT_TESTS`. Test modules are added per feature (for example, `src/effects/builtin_collection/tests/` and `src/projectscene/tests/`). The CI workflow `au4_check_unit_tests.yml` runs the test suite and publishes the coverage badge shown in `README.md`.
