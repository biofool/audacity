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
#ifndef AU_APPSHELL_COMMANDPALETTEMODEL_H
#define AU_APPSHELL_COMMANDPALETTEMODEL_H

#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h>

#include "framework/global/modularity/ioc.h"
#include "framework/global/async/asyncable.h"
#include "framework/actions/iactionsdispatcher.h"
#include "framework/shortcuts/ishortcutsregister.h"
#include "framework/ui/iuiactionsregister.h"

namespace au::appshell {
class CommandPaletteModel : public QAbstractListModel, public muse::async::Asyncable, public muse::Contextable
{
    Q_OBJECT
    QML_ELEMENT

    muse::ContextInject<muse::ui::IUiActionsRegister> uiActionsRegister { this };
    muse::ContextInject<muse::actions::IActionsDispatcher> dispatcher { this };
    muse::ContextInject<muse::shortcuts::IShortcutsRegister> shortcutsRegister { this };

    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged FINAL)

public:
    explicit CommandPaletteModel(QObject* parent = nullptr);

    QString searchText() const;
    void setSearchText(const QString& text);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void trigger(int index);

signals:
    void searchTextChanged();

private:
    enum Roles {
        CodeRole = Qt::UserRole + 1,
        TitleRole,
        DescriptionRole,
        ShortcutRole,
        IconRole,
        EnabledRole,
        CheckedRole
    };

    void loadActions();
    void updateFilter();

    muse::ui::UiActionList m_actions;
    std::vector<size_t> m_filtered;
    QString m_searchText;
};
}

#endif // AU_APPSHELL_COMMANDPALETTEMODEL_H
