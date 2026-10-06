/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Audacity-CLA-applies
 *
 * Audacity
 * A Digital Audio Editor
 *
 * Copyright (C) 2024 Audacity BVBA and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "applicationactioncontroller.h"

#include "framework/ui/navigationcommands.h"

#include <algorithm>

#include <QApplication>
#include <QCloseEvent>
#include <QFileOpenEvent>
#include <QWindow>
#include <QMimeData>

#include "framework/global/async/async.h"
#include "framework/global/defer.h"
#include "framework/global/translation.h"

#include "project/types/projecttypes.h"

using namespace au::appshell;
using namespace muse::actions;

static const QString TRACK_VIEW_SECTION_NAME("TrackViewSection");
static const QString TIMELINE_SECTION_NAME("TimelineSection");
static const QString VERTICAL_RULER_CONTROL_NAME("VerticalRuler");

void ApplicationActionController::preInit()
{
    qApp->installEventFilter(this);

#ifdef Q_OS_MAC
    // Re-open window when user clicks the dock icon while all windows are closed
    connect(qApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
        if (state != Qt::ApplicationActive) {
            return;
        }
        QWindow* window = mainWindow() ? mainWindow()->qWindow() : nullptr;
        if (window && !window->isVisible()) {
            window->show();
            window->requestActivate();
        }
    });
#endif
}

void ApplicationActionController::init()
{
    dispatcher()->reg(this, "quit", [this](const muse::actions::ActionData& args) {
        muse::io::path_t installerPath = args.count() > 1 ? args.arg<muse::io::path_t>(1) : "";
        quit(installerPath);
    });

    dispatcher()->reg(this, "restart", [this]() {
        restart();
    });

    dispatcher()->reg(this, "fullscreen", this, &ApplicationActionController::toggleFullScreen);

    dispatcher()->reg(this, "about-audacity", this, &ApplicationActionController::openAboutDialog);
    dispatcher()->reg(this, "about-qt", this, &ApplicationActionController::openAboutQtDialog);
    dispatcher()->reg(this, "online-handbook", this, &ApplicationActionController::openOnlineHandbookPage);
    dispatcher()->reg(this, "context-help", this, &ApplicationActionController::openContextHelpPage);
    dispatcher()->reg(this, "ask-help", this, &ApplicationActionController::openAskForHelpPage);
    dispatcher()->reg(this, "preference-dialog", this, &ApplicationActionController::openPreferencesDialog);

    dispatcher()->reg(this, "revert-factory", this, &ApplicationActionController::revertToFactorySettings);

    dispatcher()->reg(this, "audio-settings", this, &ApplicationActionController::openAudioSettingsDialog);
    dispatcher()->reg(this, "shortcuts-preferences", this, &ApplicationActionController::openShortcutsPreferencesDialog);
    dispatcher()->reg(this, "open-command-palette", this, &ApplicationActionController::openCommandPalette);
    dispatcher()->reg(this, "editing-preferences", this, &ApplicationActionController::openEditingPreferencesDialog);
    dispatcher()->reg(this, "spectrogram-preferences", this, &ApplicationActionController::openSpectrogramPreferencesDialog);

    // Global actions
    dispatcher()->reg(this, "action://copy", this, &ApplicationActionController::doGlobalCopy);
    dispatcher()->reg(this, "action://cut", this, &ApplicationActionController::doGlobalCut);
    dispatcher()->reg(this, "action://paste", this, &ApplicationActionController::doGlobalPaste);
    dispatcher()->reg(this, "action://undo", this, &ApplicationActionController::doGlobalUndo);
    dispatcher()->reg(this, "action://redo", this, &ApplicationActionController::doGlobalRedo);
    dispatcher()->reg(this, "action://delete", this, &ApplicationActionController::doGlobalDelete);
    dispatcher()->reg(this, "action://cancel", this, &ApplicationActionController::doGlobalCancel);
    dispatcher()->reg(this, "action://trigger", this, &ApplicationActionController::doGlobalTrigger);
    dispatcher()->reg(this, "action://enter", this, &ApplicationActionController::doGlobalEnter);
    dispatcher()->reg(this, "action://shift-enter", this, &ApplicationActionController::doGlobalShiftEnter);
    dispatcher()->reg(this, "action://context-menu", this, &ApplicationActionController::doGlobalContextMenu);
}

const std::vector<muse::actions::ActionCode>& ApplicationActionController::prohibitedActionsWhileRecording() const
{
    static const std::vector<ActionCode> PROHIBITED_WHILE_RECORDING {
        "quit",
        "restart",
    };

    return PROHIBITED_WHILE_RECORDING;
}

void ApplicationActionController::onDragEnterEvent(QDragEnterEvent* event)
{
    onDragMoveEvent(event);
}

void ApplicationActionController::onDragMoveEvent(QDragMoveEvent* event)
{
    const QMimeData* mime = event->mimeData();
    const QList<QUrl> urls = mime->urls();
    if (!urls.isEmpty()) {
        const QUrl& url = urls.front();
        if (url.isLocalFile() && extensionInstaller()->isFileSupported(url.toLocalFile())) {
            event->acceptProposedAction();
            return;
        }
    }

    if (isProjectOpened()) {
        event->ignore();
        return;
    }

    for (const QUrl& url : urls) {
        if (projectFilesController()->isUrlSupported(url)) {
            event->acceptProposedAction();
            return;
        }
    }

    event->ignore();
}

void ApplicationActionController::onDropEvent(QDropEvent* event)
{
    const QMimeData* mime = event->mimeData();
    const QList<QUrl> urls = mime->urls();
    if (urls.isEmpty()) {
        return;
    }

    const QUrl& url = urls.front();
    if (url.isLocalFile() && extensionInstaller()->isFileSupported(url.toLocalFile())) {
        event->accept();
        const muse::io::path_t filePath = url.toLocalFile();
        muse::async::Async::call(this, [this, filePath]() {
            extensionInstaller()->installExtension(filePath);
        });
        return;
    }

    if (isProjectOpened()) {
        event->ignore();
        return;
    }

    QList<QUrl> projectUrls;
    QStringList mediaFiles;

    for (const QUrl& url : urls) {
        if (!projectFilesController()->isUrlSupported(url)) {
            continue;
        }

        if (au::project::isAudacityFile(muse::io::path_t(url))) {
            projectUrls << url;
        } else {
            mediaFiles << url.toLocalFile();
        }
    }

    if (projectUrls.isEmpty() && mediaFiles.isEmpty()) {
        event->ignore();
        return;
    }

    event->accept();

    if (!projectUrls.isEmpty()) {
        muse::async::Async::call(this, [this, projectUrls]() {
            for (const QUrl& url : projectUrls) {
                dispatcher()->dispatch("file-open", ActionData::make_arg1<QUrl>(url));
            }
        });
    }

    if (!mediaFiles.isEmpty()) {
        muse::async::Async::call(this, [this, mediaFiles]() {
            dispatcher()->dispatch("project-import-startup-media",
                                   ActionData::make_arg2<QStringList, bool>(mediaFiles, false));
        });
    }
}

bool ApplicationActionController::canReceiveAction(const ActionCode& code) const
{
    if (recordController()->isRecording()) {
        return !muse::contains(prohibitedActionsWhileRecording(), code);
    }
    return true;
}

bool ApplicationActionController::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::Close && watched == mainWindow()->qWindow()) {
        if (multiwindowsProvider()->windowCount() > 1) {
            if (!projectFilesController()->closeOpenedProject()) {
                event->ignore();
                return true;
            }
            auto provider = multiwindowsProvider();
            auto ctx = iocContext();
            QMetaObject::invokeMethod(qApp, [provider, ctx]() {
                // during the call the window and the context will be destroyed
                // do not capture or use anything that is context-dependent here
                // i.e. dont use Async::call(this instead of invokeMethod
                provider->quitWindow(ctx);
            }, Qt::QueuedConnection);
            event->accept();
            return true;
        }
#ifdef Q_OS_MAC
        // On macos closing the last window does not exit the app
        if (!projectFilesController()->closeOpenedProject()) {
            event->ignore();
            return true;
        }
        // Instead we hide the window, it will be shown when needed
        mainWindow()->qWindow()->setVisible(false);
        event->accept();
        return true;
#else
        const bool accepted = quit();
        event->setAccepted(accepted);
        return true;
#endif
    }

    if (event->type() == QEvent::Quit) {
        const bool accepted = quit();
        event->setAccepted(accepted);
        return true;
    }

    //! on macOS custom URL opened from browser are also passed as QEvent::FileOpen
    if (event->type() == QEvent::FileOpen && watched == qApp) {
        handleFileOpenEvent(static_cast<const QFileOpenEvent*>(event));
        return true;
    }

    return QObject::eventFilter(watched, event);
}

void ApplicationActionController::processPendingEvents()
{
    for (const std::unique_ptr<QEvent>& event : applicationEventController()->takePendingEvents()) {
        if (event->type() == QEvent::FileOpen) {
            handleFileOpenEvent(static_cast<const QFileOpenEvent*>(event.get()));
        }
    }
}

void ApplicationActionController::handleFileOpenEvent(const QFileOpenEvent* event)
{
    const QUrl url = event->url();

    // TODO: isUrlSupported - is misleading, as it does not handle audio.com urls
    if (projectFilesController()->isUrlSupported(url)) {
        if (startupScenario()->startupCompleted()) {
            // On macos the main window may be hidden, show and raise it
            // before loading the project
            if (auto mw = mainWindow()) {
                if (QWindow* window = mw->qWindow(); window && !window->isVisible()) {
                    window->setVisible(true);
                }
                mw->requestShowOnFront();
            }
            dispatcher()->dispatch("file-open", ActionData::make_arg1<QUrl>(url));
        } else {
            startupScenario()->setStartupProjectFile(project::ProjectFile { url });
        }

        return;
    }

    const QString urlStr = url.toString(QUrl::FullyEncoded);
    if (startupScenario()->startupCompleted()) {
        if (auto mw = mainWindow()) {
            if (QWindow* window = mw->qWindow(); window && !window->isVisible()) {
                window->setVisible(true);
            }
            mw->requestShowOnFront();
        }
        dispatcher()->dispatch("open-url", ActionData::make_arg1<QString>(urlStr));
    } else {
        startupScenario()->setStartupUrl(urlStr);
    }
}

bool ApplicationActionController::quit(const muse::io::path_t& installerPath)
{
    if (m_quiting) {
        return false;
    }

    m_quiting = true;
    DEFER {
        m_quiting = false;
    };

    auto allContexts = application()->contexts();

    // Close the current window first, then others
    auto thisCtx = iocContext();
    std::stable_partition(allContexts.begin(), allContexts.end(),
                          [&thisCtx](const auto& ctx) { return ctx == thisCtx; });

    for (const auto& ctx : allContexts) {
        auto pfc = muse::modularity::ioc(ctx)->resolve<project::IProjectFilesController>("appshell");
        if (pfc && !pfc->closeOpenedProject()) {
            return false;
        }
    }

    if (!installerPath.empty()) {
        //! NOTE: All windows are quitting to complete the update, apply it
        //! in-place, falling back to handing the package to the user.
        bool applied = false;
        if (appUpdateService()->canAutoInstall()) {
            const muse::RetVal<muse::io::path_t> prepared = appUpdateService()->prepareUpdate(installerPath);
            if (prepared.ret) {
                applied = bool(appUpdateService()->finalizeUpdate(prepared.val));
            }
        }

        if (!applied) {
#if defined(Q_OS_LINUX)
            platformInteractive()->revealInFileBrowser(installerPath);
#else
            platformInteractive()->openUrl(QUrl::fromLocalFile(installerPath.toQString()));
#endif
        }
    }

    QCoreApplication::exit();
    return true;
}

void ApplicationActionController::restart()
{
    if (projectFilesController()->closeOpenedProject(false)) {
        if (multiwindowsProvider()->windowCount() == 1) {
            application()->restart();
        } else {
            multiwindowsProvider()->quitAllAndRestartLast();

            QCoreApplication::exit();
        }
    }
}

void ApplicationActionController::toggleFullScreen()
{
    mainWindow()->toggleFullScreen();
}

void ApplicationActionController::openAboutDialog()
{
    interactive()->open("audacity://about/audacity");
}

void ApplicationActionController::openAboutQtDialog()
{
    QApplication::aboutQt();
}

void ApplicationActionController::openOnlineHandbookPage()
{
    std::string handbookUrl = configuration()->handbookUrl();
    platformInteractive()->openUrl(handbookUrl);
}

void ApplicationActionController::openContextHelpPage()
{
    std::string handbookUrl = configuration()->handbookUrl();
    platformInteractive()->openUrl(handbookUrl + handbookPageForCurrentUri());
}

std::string ApplicationActionController::handbookPageForCurrentUri() const
{
    //! NOTE If a menu or other popup is open, the highlighted item is the active
    //! navigation control and its name is the menu-item id: the action code plus
    //! a numeric index suffix (see AppMenuModel::makeId). Map the action to the
    //! handbook page for the item; unknown items fall through to the URI map.
    static const std::vector<std::pair<std::string, std::string> > ACTION_TO_HANDBOOK_PAGE {
        // File menu
        { "file-new", "man/file_menu.html#New_.C2.A0Ctrl_.2B_N" },
        { "file-open", "man/file_menu.html#Open_.C2.A0Ctrl_.2B_O" },
        { "file-open-recent", "man/file_menu.html#Recent_Files_.28.22Open_Recent.22_on_Mac.29" },
        { "project-import", "man/file_menu_import.html" },
        { "file-save", "man/file_menu_save_project.html" },
        { "file-save-as", "man/file_menu_save_project.html" },
        { "file-save-to-cloud", "man/file_menu.html#Cloud_use" },
        { "action://cloud/update-audio-preview", "man/file_menu.html#Cloud_use" },
        { "export-audio", "man/file_export_dialog.html" },
        { "export-labels", "man/file_menu_export_other.html" },
        { "file-share-audio", "man/file_menu.html#Cloud_use" },
        { "file-close", "man/file_menu.html#Close_Project_.C2.A0Ctrl_.2B_W" },
        { "quit", "man/file_menu.html#Quit_Audacity_.C2.A0Ctrl_.2B_Q" },

        // Edit menu (incl. Clip and Label submenus)
        { "action://trackedit/undo", "man/edit_menu.html#Undo_.C2.A0Ctrl_.2B_Z" },
        { "action://trackedit/redo", "man/edit_menu.html#Redo_.C2.A0Ctrl_.2B_Y" },
        { "action://cut", "man/edit_menu.html#Cut_.C2.A0Ctrl_.2B_X" },
        { "action://copy", "man/edit_menu.html#Copy_.C2.A0Ctrl_.2B_C" },
        { "action://paste", "man/edit_menu.html#Paste_.C2.A0Ctrl_.2B_V" },
        { "action://delete", "man/edit_menu.html#Delete_.C2.A0Ctrl_.2B_K" },
        { "duplicate", "man/edit_menu.html#Duplicate_.C2.A0Ctrl_.2B_D" },
        { "delete-per-track-ripple", "man/edit_menu.html#Remove_Special" },
        { "delete-leave-gap", "man/edit_menu.html#Remove_Special" },
        { "delete-per-clip-ripple", "man/edit_menu.html#Remove_Special" },
        { "delete-all-tracks-ripple", "man/edit_menu.html#Remove_Special" },
        { "cut-per-track-ripple", "man/edit_menu.html#Remove_Special" },
        { "cut-per-clip-ripple", "man/edit_menu.html#Remove_Special" },
        { "cut-all-tracks-ripple", "man/edit_menu.html#Remove_Special" },
        { "cut-leave-gap", "man/edit_menu.html#Remove_Special" },
        { "trim-clip", "man/edit_menu.html#Audio_Clips" },
        { "split", "man/edit_menu.html#Audio_Clips" },
        { "split-into-new-track", "man/edit_menu.html#Audio_Clips" },
        { "disjoin", "man/edit_menu.html#Audio_Clips" },
        { "join", "man/edit_menu.html#Audio_Clips" },
        { "group-clips", "man/edit_menu.html#Audio_Clips" },
        { "ungroup-clips", "man/edit_menu.html#Audio_Clips" },
        { "silence-audio-selection", "man/edit_menu.html#Remove_Special" },
        { "open-metadata-editor", "man/edit_menu.html#Metadata_Editor" },
        { "preference-dialog", "man/edit_menu.html#Preferences_.C2.A0Ctrl_.2B_P" },
        { "label-add", "man/label_tracks.html" },
        { "paste-new-label", "man/label_tracks.html" },
        { "open-label-editor", "man/label_tracks.html" },

        // Select menu (incl. Region and Looping submenus)
        { "select-all", "man/select_menu.html#All_.C2.A0Ctrl_.2B_A" },
        { "clear-selection", "man/select_menu.html#None_.C2.A0Ctrl_.2B_Shift_.2B_A_.C2.A0_Extra" },
        { "select-all-tracks", "man/select_menu.html#Tracks" },
        { "select-left-of-playback-position", "man/select_menu.html#Region" },
        { "select-right-of-playback-position", "man/select_menu.html#Region" },
        { "select-track-start-to-cursor", "man/select_menu.html#Region" },
        { "select-cursor-to-track-end", "man/select_menu.html#Region" },
        { "select-track-start-to-end", "man/select_menu.html#Region" },
        { "select-previous-clip-boundary-to-cursor", "man/select_menu.html#Audio_Clips" },
        { "select-cursor-to-next-clip-boundary", "man/select_menu.html#Audio_Clips" },
        { "select-previous-clip", "man/select_menu.html#Audio_Clips" },
        { "select-next-clip", "man/select_menu.html#Audio_Clips" },
        { "skip-to-selection-start", "man/select_menu.html" },
        { "skip-to-selection-end", "man/select_menu.html" },
        { "toggle-loop-region", "man/select_menu.html#Region" },
        { "clear-loop-region", "man/select_menu.html#Region" },
        { "set-loop-region-to-selection", "man/select_menu.html#Region" },
        { "set-loop-region-in-out", "man/select_menu.html#Region" },
        { "set-selection-to-loop", "man/select_menu.html#Region" },
        { "zero-cross", "man/select_menu.html#At_Zero_Crossings_.C2.A0Z" },

        // View menu (incl. Zoom submenu)
        { "toggle-effects", "man/view_menu.html" },
        { "toggle-history", "man/view_menu.html#History" },
        { "fullscreen", "man/view_menu.html#Enter_.2F_Exit_Full_Screen_.28Mac_only.29" },
        { "toggle-clipping-in-waveform", "man/view_menu.html#Show_Clipping_in_Waveform" },
        { "toggle-rms-in-waveform", "man/view_menu.html#Show_RMS_in_Waveform" },
        { "toggle-vertical-rulers", "man/view_menu.html" },
        { "dock-restore-default-layout", "man/view_menu.html" },
        { "zoom-in", "man/view_menu.html#Zoom" },
        { "zoom-out", "man/view_menu.html#Zoom" },
        { "zoom-default", "man/view_menu.html#Zoom" },
        { "zoom-to-selection", "man/view_menu.html#Zoom" },
        { "zoom-toggle", "man/view_menu.html#Zoom" },
        { "zoom-to-fit-project", "man/view_menu.html#Zoom" },
        { "collapse-all-tracks", "man/view_menu.html#Track_Size" },
        { "expand-all-tracks", "man/view_menu.html#Track_Size" },

        // Record menu (no record_menu page in the manual — use the recording guide)
        { "command://record/on-current-track", "man/recording.html#Recording_on_the_same_track" },
        { "command://record/on-new-track", "man/recording.html#Recording_a_new_track" },
        { "action://record/lead-in-recording", "man/recording.html#Continuing_recording_in_a_new_track" },
        { "set-up-timed-recording", "man/recording.html#Recording_for_a_specific_length_of_time" },
        { "toggle-sound-activated-recording", "man/recording.html" },
        { "set-sound-activation-level", "man/recording.html" },

        // Tracks menu
        { "new-mono-track", "man/tracks_menu.html#Add_New_.C2.A0" },
        { "new-stereo-track", "man/tracks_menu.html#Add_New_.C2.A0" },
        { "new-label-track", "man/tracks_menu.html#Add_New_.C2.A0" },
        { "track-duplicate", "man/tracks_menu.html" },
        { "align-end-to-end", "man/tracks_menu.html#Align_Tracks_.C2.A0" },
        { "align-together", "man/tracks_menu.html#Align_Tracks_.C2.A0" },
        { "align-start-to-zero", "man/tracks_menu.html#Align_Tracks_.C2.A0" },
        { "align-start-to-playhead", "man/tracks_menu.html#Align_Tracks_.C2.A0" },
        { "align-start-to-selection-end", "man/tracks_menu.html#Align_Tracks_.C2.A0" },
        { "align-end-to-playhead", "man/tracks_menu.html#Align_Tracks_.C2.A0" },
        { "align-end-to-selection-end", "man/tracks_menu.html#Align_Tracks_.C2.A0" },

        // Effect menu
        { "add-realtime-effects", "man/effect_menu.html#Add_Realtime_Effects" },
        { "repeat-last-effect", "man/effect_menu.html#Repeat_Last_Effect_.C2.A0Ctrl_.2BR" },

        // Analyze menu
        { "contrast-analyzer", "man/analyze_menu.html#Contrast_.C2.A0Ctrl_.2B_Shift_.2B_T_.C2.A0_Extra" },
        { "plot-spectrum", "man/analyze_menu.html#Plot_Spectrum" },

        // Tools menu
        { "plugin-manager", "man/manage_effects_generators_and_analyzers.html" },
        { "manage-macros", "man/tools_menu.html#Macro_Manager_.C2.A0" },
        { "raw-data-import", "man/tools_menu.html#Sample_Data_Import" },
        { "reset-configuration", "man/tools_menu.html#Reset_Configuration" },

        // Extra menu
        { "prev-window", "man/window_menu.html" },
        { "next-window", "man/window_menu.html" },
        { "benchmark", "man/tools_menu.html#Run_Benchmark_.C2.A0" },
        { "regular-interval-labels", "man/tools_menu.html#Regular_Interval_Labels" },
        { "sort-by-time", "man/extra_menu.html" },

        // Help menu
        { "tutorials", "man/help_menu.html" },
        { "context-help", "man/help_menu.html#Quick_Help_.C2.A0" },
        { "online-handbook", "man/help_menu.html#Manual" },
        { "shortcuts-preferences", "man/keyboard_preferences.html" },
        { "link-account", "man/help_menu.html#Link_audio.com_account_.C2.A0" },
        { "about-audacity", "man/help_menu.html#About_Audacity_.C2.A0" },
        { "about-qt", "man/help_menu.html#About_Audacity_.C2.A0" },
        { "revert-factory", "man/help_menu.html" },
        { "check-update", "man/help_menu.html#Check_for_Updates_.C2.A0" },

        // Timeline and track-panel context menus
        { "audio-settings", "man/preferences.html" },
        { "rescan-devices", "man/preferences.html" },
        { "beats-measures-ruler", "man/timeline.html" },
        { "minutes-seconds-ruler", "man/timeline.html" },
        { "toggle-pinned-play-head", "man/transport_menu.html" },
        { "toggle-playback-on-ruler-click-enabled", "man/transport_menu.html" },
        { "toggle-selection-follows-loop-region", "man/select_menu.html#Region" },
        { "toggle-update-display-while-playing", "man/transport_menu.html" },
        { "realtimeeffect-remove", "man/index_of_effects_generators_and_analyzers.html" },
    };

    //! NOTE Prefix rules for parameterized or dynamically-generated action codes
    //!      (menu item ids end in the numeric index suffix, which strips cleanly,
    //!      but parameter values can also end in digits — prefixes are safer).
    static const std::vector<std::pair<std::string, std::string> > ACTION_PREFIX_TO_HANDBOOK_PAGE {
        { "action://trackedit/cut", "man/edit_menu.html#Cut_.C2.A0Ctrl_.2B_X" },
        { "action://trackedit/copy", "man/edit_menu.html#Copy_.C2.A0Ctrl_.2B_C" },
        { "action://trackedit/paste", "man/edit_menu.html#Paste_.C2.A0Ctrl_.2B_V" },
        { "action://trackedit/delete", "man/edit_menu.html#Remove_Special" },
        { "action://trackedit/clip/", "man/audio_tracks.html" },
        { "action://trackedit/track/", "man/audio_tracks.html" },
        { "action://projectscene/track-view", "man/audio_tracks.html" },
        { "action://effects/", "man/index_of_effects_generators_and_analyzers.html" },
    };

    //! NOTE The active control covers highlighted menu items (popups use
    //! Exclusive sections) and keyboard-focused toolbar/page controls alike —
    //! either is a valid context for F1. Names that are not action codes
    //! simply miss the map and fall through to the URI map.
    const muse::ui::INavigationControl* activeControl = navigationController()->activeControl();
    if (activeControl) {
        std::string actionId = activeControl->name().toStdString();
        while (!actionId.empty() && actionId.back() >= '0' && actionId.back() <= '9') {
            actionId.pop_back();
        }

        for (const auto& entry : ACTION_TO_HANDBOOK_PAGE) {
            if (entry.first == actionId) {
                return entry.second;
            }
        }

        for (const auto& entry : ACTION_PREFIX_TO_HANDBOOK_PAGE) {
            if (actionId.find(entry.first) == 0) {
                return entry.second;
            }
        }
    }

    //! NOTE Maps the currently-open top-level page or dialog URI to a page in
    //! the online handbook (https://manual.audacityteam.org). Entries are
    //! matched by URI prefix so sub-dialogs share their parent's page.
    //! Unmapped contexts fall back to the handbook root.
    static const std::vector<std::pair<std::string, std::string> > URI_TO_HANDBOOK_PAGE {
        { "audacity://project/export", "man/file_export_dialog.html" },
        { "audacity://effects", "man/index_of_effects_generators_and_analyzers.html" },
        { "audacity://projectscene/geteffects", "man/index_of_effects_generators_and_analyzers.html" },
        { "muse://extensions/viewer", "man/index_of_effects_generators_and_analyzers.html" },
        { "audacity://projectscene/openlabeleditor", "man/label_tracks.html" },
        { "audacity://projectscene/addnewlabeltrack", "man/label_tracks.html" },
        { "audacity://preferences", "man/preferences.html" },
    };

    const std::string currentUri = interactive()->currentUri().val.toString();
    for (const auto& entry : URI_TO_HANDBOOK_PAGE) {
        if (currentUri.find(entry.first) == 0) {
            return entry.second;
        }
    }

    return "";
}

void ApplicationActionController::openAskForHelpPage()
{
    std::string askForHelpUrl = configuration()->askForHelpUrl();
    platformInteractive()->openUrl(askForHelpUrl);
}

void ApplicationActionController::openPreferencesDialog()
{
    //! TODO AU4
    // if (multiwindowsProvider()->isPreferencesAlreadyOpened()) {
    //     multiwindowsProvider()->activateWindowWithOpenedPreferences();
    //     return;
    // }

    interactive()->open("audacity://preferences");
}

void ApplicationActionController::openAudioSettingsDialog()
{
    muse::UriQuery preferencesUri("audacity://preferences");
    preferencesUri.addParam("currentPageId", muse::Val("audio-settings"));

    interactive()->open(preferencesUri);
}

void ApplicationActionController::openShortcutsPreferencesDialog()
{
    muse::UriQuery preferencesUri("audacity://preferences");
    preferencesUri.addParam("currentPageId", muse::Val("shortcuts"));

    interactive()->open(preferencesUri);
}

void ApplicationActionController::openCommandPalette()
{
    interactive()->open("audacity://command-palette");
}

void ApplicationActionController::openEditingPreferencesDialog()
{
    muse::UriQuery preferencesUri("audacity://preferences");
    preferencesUri.addParam("currentPageId", muse::Val("editing"));

    interactive()->open(preferencesUri);
}

void ApplicationActionController::openSpectrogramPreferencesDialog()
{
    muse::UriQuery preferencesUri("audacity://preferences");
    preferencesUri.addParam("currentPageId", muse::Val("spectrogram"));

    interactive()->open(preferencesUri);
}

void ApplicationActionController::revertToFactorySettings()
{
    std::string title = muse::trc("appshell", "Are you sure you want to revert to factory settings?");
    std::string question = muse::trc("appshell",
                                     "This action will reset all your app preferences and custom UI configurations. "
                                     "It also deletes your custom workspaces and shortcuts. "
                                     "You will also need to scan all third party plugins again.\n\n"
                                     "This action will not delete any of your projects.");

    muse::IInteractive::ButtonData cancelBtn = interactive()->buttonData(muse::IInteractive::Button::Cancel);
    cancelBtn.accent = true;

    int revertBtn = int(muse::IInteractive::Button::Apply);
    auto promise = interactive()->warning(title, question,
                                          { cancelBtn,
                                            muse::IInteractive::ButtonData(revertBtn, muse::trc("appshell", "Revert")) },
                                          cancelBtn.btn, { muse::IInteractive::Option::WithIcon },
                                          muse::trc("appshell", "Revert to factory settings"));

    promise.onResolve(this, [this](const muse::IInteractive::Result& res) {
        if (res.isButton(muse::IInteractive::Button::Cancel)) {
            return;
        }

        static constexpr bool KEEP_DEFAULT_SETTINGS = false;
        static constexpr bool NOTIFY_ABOUT_CHANGES = false;
        static constexpr bool NOTIFY_OTHER_INSTANCES = false;
        configuration()->revertToFactorySettings(KEEP_DEFAULT_SETTINGS, NOTIFY_ABOUT_CHANGES, NOTIFY_OTHER_INSTANCES);

        std::string title = muse::trc("appshell", "Would you like to restart Audacity now?");
        std::string question = muse::trc("appshell", "Audacity needs to be restarted for these changes to take effect.");

        int restartBtn = int(muse::IInteractive::Button::Apply);
        auto promise = interactive()->question(title, question,
                                               { interactive()->buttonData(muse::IInteractive::Button::Cancel),
                                                 muse::IInteractive::ButtonData(restartBtn,
                                                                                muse::trc("appshell", "Restart"), true) },
                                               restartBtn, {},
                                               muse::trc("appshell", "Restart Audacity"));

        promise.onResolve(this, [this](const muse::IInteractive::Result& res) {
            if (!res.isButton(muse::IInteractive::Button::Cancel)) {
                restart();
            }
        });
    });
}

bool ApplicationActionController::isProjectOpened() const
{
    bool hasProject = globalContext()->currentProject() != nullptr;
    bool isOpened = uiContextResolver()->matchWithCurrent(context::UiCtxProjectOpened);
    return hasProject && isOpened;
}

bool ApplicationActionController::isProjectOpenedAndFocused() const
{
    bool isOpened = isProjectOpened();
    bool isFocused = uiContextResolver()->matchWithCurrent(context::UiCtxProjectFocused);
    return isOpened && isFocused;
}

void ApplicationActionController::doGlobalCopy()
{
    if (isProjectOpenedAndFocused()) {
        dispatcher()->dispatch("action://trackedit/copy");
    } else {
        // resolve other actions
    }
}

void ApplicationActionController::doGlobalCut()
{
    if (isProjectOpenedAndFocused()) {
        dispatcher()->dispatch("action://trackedit/cut");
    } else {
        // resolve other actions
    }
}

void ApplicationActionController::doGlobalPaste()
{
    if (isProjectOpened()) {
        dispatcher()->dispatch("action://trackedit/paste-default");
    } else {
        // resolve other actions
    }
}

void ApplicationActionController::doGlobalUndo()
{
    if (isProjectOpened()) {
        dispatcher()->dispatch("action://trackedit/undo");
    } else {
        // resolve other actions
    }
}

void ApplicationActionController::doGlobalRedo()
{
    if (isProjectOpened()) {
        dispatcher()->dispatch("action://trackedit/redo");
    } else {
        // resolve other actions
    }
}

void ApplicationActionController::doGlobalDelete()
{
    if (isProjectOpenedAndFocused()) {
        dispatcher()->dispatch("action://trackedit/delete");
    } else {
        // resolve other actions
    }
}

void ApplicationActionController::doGlobalCancel()
{
    if (isProjectOpenedAndFocused()) {
        dispatcher()->dispatch("action://trackedit/cancel");
        return;
    }

    commandDispatcher()->dispatch(muse::ui::ESCAPE_COMMAND);
}

void ApplicationActionController::doGlobalTrigger()
{
    if (isProjectOpened()) {
        dispatcher()->dispatch("action://playback/toggle-play-stop");
        return;
    }

    commandDispatcher()->dispatch(muse::ui::TRIGGER_CONTROL_COMMAND);
}

void ApplicationActionController::doGlobalEnter()
{
    const muse::ui::INavigationSection* activeSection = navigationController()->activeSection();
    if (activeSection && activeSection->name() != TRACK_VIEW_SECTION_NAME) {
        commandDispatcher()->dispatch(muse::ui::TRIGGER_CONTROL_COMMAND);
        return;
    }

    if (isProjectOpenedAndFocused()) {
        dispatcher()->dispatch("track-view-replace-selection");
        return;
    }

    commandDispatcher()->dispatch(muse::ui::TRIGGER_CONTROL_COMMAND);
}

void ApplicationActionController::doGlobalShiftEnter()
{
    const muse::ui::INavigationSection* activeSection = navigationController()->activeSection();
    if (activeSection && activeSection->name() != TRACK_VIEW_SECTION_NAME) {
        commandDispatcher()->dispatch(muse::ui::TRIGGER_CONTROL_COMMAND);
        return;
    }

    if (isProjectOpenedAndFocused()) {
        dispatcher()->dispatch("track-view-range-selection");
        return;
    }

    commandDispatcher()->dispatch(muse::ui::TRIGGER_CONTROL_COMMAND);
}

void ApplicationActionController::doGlobalContextMenu()
{
    const muse::ui::INavigationSection* activeSection = navigationController()->activeSection();
    if (!activeSection) {
        return;
    }

    if (activeSection->name() == TRACK_VIEW_SECTION_NAME) {
        const muse::ui::INavigationControl* activeControl = navigationController()->activeControl();
        if (activeControl && activeControl->name() == VERTICAL_RULER_CONTROL_NAME) {
            dispatcher()->dispatch("track-view-ruler-context-menu");
            return;
        }

        dispatcher()->dispatch("track-view-item-context-menu");
    } else if (activeSection->name() == TIMELINE_SECTION_NAME) {
        dispatcher()->dispatch("timeline-context-menu");
    }
}
