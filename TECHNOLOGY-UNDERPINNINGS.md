# Technology Underpinnings

## Base

- **Codebase**: Audacity 4 `master` (upstream `audacity/audacity`), forked at `biofool/audacity`, branch `feature/welcome-tour`. Audacity 4 is a Qt/QML rewrite of the editor; the legacy Audacity 3 audio engine lives under `au3/` and is wrapped by `src/au3wrap/`.
- **Language/toolchain**: C++20 (`CMakeLists.txt:13`), Qt 6.10 + QML, CMake + Ninja. Build instructions: `BUILDING.md`.
- **Framework**: the `muse` git submodule (MuseScore's `muse_framework`, see `.gitmodules`) provides the UI framework, dependency injection (`muse::ContextInject`), actions, navigation, toasts, and the tours module. `muse_deps` holds third-party dependencies. Both must be checked out to build.

## Feature: first-run welcome tour

Two files in this repo plus the `muse::tours` framework module:

- `src/appshell/qml/Audacity/AppShell/AppWindow.qml` — imports `Muse.Tours 1.0` and instantiates `ToursProvider {}`, the QML component that renders tour step popups over the app window.
- `src/project/internal/projectactionscontroller.cpp` — `registerTours()` builds a `muse::tours::Tour` (id `"welcome"`) with six `TourStep`s and registers it against the event code `"project-opened"` via `IToursService::registerTour`. `init()` then calls `toursService()->onEvent(...)` when `interactive()->currentUri()` becomes `audacity://project` while a project is loaded — i.e. after first-launch and welcome dialogs close.
- `src/project/internal/projectactionscontroller.h` — injects `muse::tours::IToursService` via `muse::ContextInject` and declares `registerTours()`.

Inside the framework (`muse/framework/tours/`):

- `itoursservice.h` — interface: `registerTour(eventCode, tour)` + `onEvent(eventCode)`.
- `internal/toursservice.cpp` — keeps an `eventCode → Tour` map; on event, compares the tour id to `ToursConfiguration::lastShownTourIdForEvent` and shows it only if different.
- `internal/toursprovider.cpp` — resolves each step's `controlUri` (`audacity://<section>/<panel>/<control>`) through `ui::INavigationController::findControl`, anchors the popup to that control, and skips steps whose control is missing (`LOGD: Control not found … skipping step`).
- `internal/toursconfiguration.cpp` — persists shown tours under the `Settings` key `tours/lastShownTours`, which makes the tour a once-per-install experience.
- `tourstypes.h` — `Tour { id, steps }` and `TourStep { title, description, previewImageOrGifUrl, videoExplanationUrl, controlUri }`. The welcome tour leaves image/video fields empty; text only.
- `qml/Muse/Tours/ToursProvider.qml` + `internal/TourStepPopup.qml` — the popup UI, backed by `toursprovidermodel.cpp`.

Build wiring: `MUSE_MODULE_TOURS` defaults ON via `declare_muse_module_opt(TOURS ON)` in `muse/framework/cmake/MuseDeclareOptions.cmake`; Audacity's `CMakeLists.txt` does not override it. `src/app/CMakeLists.txt` links `muse::tours` with `add_to_link_if_exists`, and `src/appshell/qml/Audacity/AppShell/CMakeLists.txt` lists `muse_tours_qml` as a QML import. If the module were disabled, stubs under `muse/framework/stubs/tours/` (no-op service, empty `ToursProvider` QtObject) take its place — the code compiles and the tour silently does nothing.

Step targets use real navigation names: toolbar items are named after their action code (`audio-setup`, `get-effects`, `file-share-audio`, `toggle-loop-region`, `split-tool`); the tracks panel title bar exposes `AddTrack` (`src/projectscene/qml/Audacity/ProjectScene/trackspanel/TracksTitleBar.qml`, `navigation.name: "AddTrack"`).

## Feature: enriched tooltips

Twelve QML files gain `toolTipTitle` / `toolTipDescription` (and `toolTipShortcut` where a default binding is verified in `src/app/configs/data/shortcuts.xml`), using the Muse three-part tooltip API:

- `src/playback/qml/Audacity/Playback/panels/PlaybackMeterPanel.qml`, `toolbars/PlaybackLevel.qml`
- `src/projectscene/qml/Audacity/ProjectScene/trackspanel/` — `AddNewTrackPopup.qml`, `LabelTrackItem.qml`, `RealtimeEffectListItem.qml`, `TrackEffectsSection.qml`, `TrackItem.qml`, `TracksTitleBar.qml`, `WaveTrackItem.qml`
- `src/effects/effects_base/qml/Audacity/Effects/` — `BypassEffectButton.qml`, `EffectPresetsBar.qml`
- `src/effects/builtin_collection/filtercurveeq/FilterCurveEqView.qml`

Shortcuts are only written where the binding is confirmed: `E` (add-realtime-effects), `Shift+U` (track-mute), `Shift+S` (track-solo), `Ctrl+B` (label-add), `Ctrl+Shift+N` (new-mono-track). Strings use `qsTrc`, so they flow into the normal Qt translation pipeline (`tools/translations`, `.tx/config`).

## Supporting change: post-edit lint hook

`.claude/hooks/post_edit_lint.py` + `.claude/settings.json` — a Claude Code `PostToolUse` hook that runs fast deterministic checks (ruff/py_compile, `php -l`, `node --check`, `bash -n`/shellcheck, `json.tool`, gofmt, pyyaml) on every file an agent edits, so mistakes surface immediately. Vendored from `biofool/magic-compact-devin`; stdlib-only; each check no-ops when its tool is absent.
