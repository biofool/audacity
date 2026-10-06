/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Audacity-CLA-applies
 *
 * Audacity
 * A Digital Audio Editor
 *
 * Copyright (C) 2026 Audacity BVBA and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
import QtQuick 2.15
import QtQuick.Layouts 1.15

import Muse.Ui 1.0
import Muse.UiComponents

import Audacity.AppShell

StyledDialogView {
    id: root

    title: qsTrc("appshell/command_palette", "Command palette")

    contentWidth: 480
    contentHeight: 440

    CommandPaletteModel {
        id: paletteModel
    }

    function triggerCurrent() {
        var index = view.count > 0 ? view.currentIndex : -1

        root.hide()

        // Dispatch after the dialog is closed so actions that open
        // their own dialogs don't stack over a closing modal
        if (index >= 0) {
            Qt.callLater(function() { paletteModel.trigger(index) })
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        SearchField {
            id: searchField

            Layout.fillWidth: true

            onSearchTextChanged: paletteModel.searchText = searchText

            onAccepted: root.triggerCurrent()

            onEscaped: root.hide()

            Keys.onPressed: function(event) {
                if (event.key === Qt.Key_Down) {
                    view.currentIndex = Math.min(view.currentIndex + 1, view.count - 1)
                    event.accepted = true
                } else if (event.key === Qt.Key_Up) {
                    view.currentIndex = Math.max(view.currentIndex - 1, 0)
                    event.accepted = true
                }
            }
        }

        StyledListView {
            id: view

            Layout.fillWidth: true
            Layout.fillHeight: true

            model: paletteModel
            spacing: 0
            currentIndex: 0

            onCountChanged: currentIndex = 0

            delegate: ListItemBlank {
                id: item

                isSelected: view.currentIndex === model.index

                navigation.name: model.code
                navigation.panel: view.navigation
                navigation.row: model.index
                navigation.accessible.name: model.title
                navigation.accessible.row: model.index

                onClicked: {
                    view.currentIndex = model.index
                    root.triggerCurrent()
                }

                onIsSelectedChanged: {
                    if (isSelected) {
                        scrollIntoView()
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 12
                    spacing: 8

                    StyledIconLabel {
                        iconCode: model.icon ?? IconCode.NONE
                        opacity: model.enabled ? 1.0 : 0.3
                    }

                    StyledTextLabel {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignLeft
                        text: model.title
                        opacity: model.enabled ? 1.0 : 0.5
                    }

                    StyledTextLabel {
                        text: model.checked ? "✓" : ""
                        font: ui.theme.bodyBoldFont
                    }

                    StyledTextLabel {
                        text: model.shortcut
                        opacity: 0.6
                    }
                }
            }
        }

        StyledTextLabel {
            Layout.fillWidth: true

            //: Shown when a command palette search matches no actions
            text: qsTrc("appshell/command_palette", "No matching commands")

            visible: view.count < 1
        }
    }

    onIsOpenedChanged: {
        if (isOpened) {
            Qt.callLater(function() { searchField.forceActiveFocus() })
        }
    }
}
