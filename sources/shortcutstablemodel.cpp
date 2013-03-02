/*

Copyright 2013 Adam Reichold

This file is part of qpdfview.

qpdfview is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

qpdfview is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with qpdfview.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "shortcutstablemodel.h"

#include <QAction>
#include <QSet>

ShortcutsTableModel::ShortcutsTableModel(const QList< QAction* >& actions, const QMap< QAction*, QKeySequence >& defaultShortcuts, QObject* parent) : QAbstractTableModel(parent),
    m_actions(actions),
    m_defaultShortcuts(defaultShortcuts),
    m_shortcuts()
{
    foreach(QAction* action, m_actions)
    {
        m_shortcuts.insert(action, action->shortcut());
    }
}

int ShortcutsTableModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);

    return 2;
}

int ShortcutsTableModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);

    return m_actions.count();
}

Qt::ItemFlags ShortcutsTableModel::flags(const QModelIndex& index) const
{
    switch(index.column())
    {
    case 0:
        return Qt::ItemIsEnabled;
        break;
    case 1:
        return Qt::ItemIsEnabled | Qt::ItemIsEditable;
        break;
    }

    return Qt::NoItemFlags;
}

QVariant ShortcutsTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(orientation);

    if(role == Qt::DisplayRole)
    {
        switch(section)
        {
        case 0:
            return tr("Action");
            break;
        case 1:
            return tr("Key sequence");
            break;
        }
    }

    return QVariant();
}

QVariant ShortcutsTableModel::data(const QModelIndex& index, int role) const
{
    if((role == Qt::DisplayRole || role == Qt::EditRole) && index.row() >= 0 && index.row() < m_actions.count())
    {
        switch(index.column())
        {
        case 0:
            return m_actions.at(index.row())->text().remove(QLatin1Char('&'));
            break;
        case 1:
            return m_shortcuts.value(m_actions.at(index.row()));
            break;
        }

        return QString::number(index.row()) + ":" + QString::number(index.column());
    }

    return QVariant();
}

bool ShortcutsTableModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if(role == Qt::EditRole && index.column() == 1 && index.row() >= 0 && index.row() < m_actions.count())
    {
        QKeySequence shortcut(value.toString());

        if(!shortcut.isEmpty() || value.toString().isEmpty())
        {
            m_shortcuts.insert(m_actions.at(index.row()), shortcut);

            emit dataChanged(index, index);

            return true;
        }
    }

    return false;
}

void ShortcutsTableModel::accept()
{
    for(QMap< QAction*, QKeySequence >::iterator iterator = m_shortcuts.begin(); iterator != m_shortcuts.end(); ++iterator)
    {
        iterator.key()->setShortcut(iterator.value());
    }
}

void ShortcutsTableModel::reset()
{
    m_shortcuts = m_defaultShortcuts;

    emit dataChanged(createIndex(0, 1), createIndex(m_actions.count(), 1));
}
