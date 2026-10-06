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
#include "commandpalettemodel.h"

#include <algorithm>

#include "framework/global/translation.h"
#include "framework/shortcuts/shortcutstypes.h"

using namespace au::appshell;
using namespace muse::actions;
using namespace muse::ui;

CommandPaletteModel::CommandPaletteModel(QObject* parent)
    : QAbstractListModel(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
    loadActions();

    uiActionsRegister()->actionsChanged().onReceive(this, [this](const UiActionList&) {
        loadActions();
    });

    uiActionsRegister()->actionStateChanged().onReceive(this, [this](const ActionCodeList& codes) {
        for (const ActionCode& code : codes) {
            for (size_t row = 0; row < m_filtered.size(); ++row) {
                if (m_actions[m_filtered[row]].code == code) {
                    emit dataChanged(index(static_cast<int>(row)), index(static_cast<int>(row)), { EnabledRole, CheckedRole });
                    break;
                }
            }
        }
    });
}

void CommandPaletteModel::loadActions()
{
    m_actions.clear();

    for (const UiAction& action : uiActionsRegister()->actionList()) {
        if (action.title.qTranslatedWithoutMnemonic().isEmpty()) {
            continue;
        }

        if (action.code == "open-command-palette") {
            continue;
        }

        m_actions.push_back(action);
    }

    updateFilter();
}

QString CommandPaletteModel::searchText() const
{
    return m_searchText;
}

void CommandPaletteModel::setSearchText(const QString& text)
{
    if (m_searchText == text) {
        return;
    }

    m_searchText = text;
    emit searchTextChanged();

    updateFilter();
}

void CommandPaletteModel::updateFilter()
{
    beginResetModel();
    m_filtered.clear();

    QString key = m_searchText.trimmed().toLower();

    for (size_t i = 0; i < m_actions.size(); ++i) {
        const UiAction& action = m_actions[i];

        if (!key.isEmpty()) {
            QString haystack = QString::fromStdString(action.code)
                               + u' ' + action.title.qTranslatedWithoutMnemonic()
                               + u' ' + action.description.qTranslated();

            if (!haystack.toLower().contains(key)) {
                continue;
            }
        }

        m_filtered.push_back(i);
    }

    endResetModel();
}

int CommandPaletteModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_filtered.size());
}

QVariant CommandPaletteModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return QVariant();
    }

    const UiAction& action = m_actions[m_filtered[static_cast<size_t>(index.row())]];

    switch (role) {
    case CodeRole:
        return QString::fromStdString(action.code);
    case TitleRole:
        return action.title.qTranslatedWithoutMnemonic();
    case DescriptionRole:
        return action.description.qTranslated();
    case ShortcutRole: {
        const muse::shortcuts::Shortcut& shortcut = shortcutsRegister()->shortcut(action.code);
        return muse::shortcuts::sequencesToNativeText(shortcut.sequences);
    }
    case IconRole:
        return static_cast<int>(action.iconCode);
    case EnabledRole:
        return uiActionsRegister()->actionState(action.code).enabled;
    case CheckedRole:
        return uiActionsRegister()->actionState(action.code).checked;
    }

    return QVariant();
}

QHash<int, QByteArray> CommandPaletteModel::roleNames() const
{
    static const QHash<int, QByteArray> roles = {
        { CodeRole, "code" },
        { TitleRole, "title" },
        { DescriptionRole, "description" },
        { ShortcutRole, "shortcut" },
        { IconRole, "icon" },
        { EnabledRole, "enabled" },
        { CheckedRole, "checked" }
    };

    return roles;
}

void CommandPaletteModel::trigger(int index)
{
    if (index < 0 || index >= rowCount()) {
        return;
    }

    const UiAction& action = m_actions[m_filtered[static_cast<size_t>(index)]];

    if (!uiActionsRegister()->actionState(action.code).enabled) {
        return;
    }

    dispatcher()->dispatch(action.code);
}
