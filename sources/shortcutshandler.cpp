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

#include "shortcutshandler.h"

#include <QAction>
#include <QSet>

#include "documentview.h"
#include "settings.h"

ShortcutsHandler::ShortcutsHandler(Settings* settings, QObject* parent) : QAbstractTableModel(parent),
    m_settings(settings),
    m_actions(),
    m_shortcuts(),
    m_defaultShortcuts()
{
    // skip backward shortcut

    m_skipBackwardAction = new QAction(tr("Skip backward"), this);
    m_skipBackwardAction->setObjectName(QLatin1String("skipBackward"));

    m_skipBackwardAction->setShortcut(::DocumentView::skipBackwardShortcut());
    addAction(m_skipBackwardAction);

    // skip forward shortcut

    m_skipForwardAction = new QAction(tr("Skip forward"), this);
    m_skipForwardAction->setObjectName(QLatin1String("skipForward"));

    m_skipForwardAction->setShortcut(::DocumentView::skipForwardShortcut());
    addAction(m_skipForwardAction);

    // movement shortcuts

    m_moveUpAction = new QAction(tr("Move up"), this);
    m_moveUpAction->setObjectName(QLatin1String("moveUp"));

    m_moveUpAction->setShortcut(::DocumentView::movementShortcuts(::DocumentView::MoveUp));
    addAction(m_moveUpAction);

    m_moveDownAction = new QAction(tr("Move down"), this);
    m_moveDownAction->setObjectName(QLatin1String("moveDown"));

    m_moveDownAction->setShortcut(::DocumentView::movementShortcuts(::DocumentView::MoveDown));
    addAction(m_moveDownAction);

    m_moveLeftAction = new QAction(tr("Move left"), this);
    m_moveLeftAction->setObjectName(QLatin1String("moveLeft"));

    m_moveLeftAction->setShortcut(::DocumentView::movementShortcuts(::DocumentView::MoveLeft));
    addAction(m_moveLeftAction);

    m_moveRightAction = new QAction(tr("Move right"), this);
    m_moveRightAction->setObjectName(QLatin1String("moveRight"));

    m_moveRightAction->setShortcut(::DocumentView::movementShortcuts(::DocumentView::MoveRight));
    addAction(m_moveRightAction);

    // return to page shortcut

    m_returnToPageAction = new QAction(tr("Return to page"), this);
    m_returnToPageAction->setObjectName(QLatin1String("returnToPage"));

    m_returnToPageAction->setShortcut(::DocumentView::returnToPageShortcut());
    addAction(m_returnToPageAction);
}

void ShortcutsHandler::addAction(QAction* action)
{
    if(!action->objectName().isEmpty())
    {
        QKeySequence defaultShortcut = action->shortcut();
        QKeySequence shortcut = m_settings->shortcut(action->objectName(), action->shortcut());

        action->setShortcut(shortcut);

        m_actions.append(action);
        m_shortcuts.insert(action, shortcut);
        m_defaultShortcuts.insert(action, defaultShortcut);
    }
}

void ShortcutsHandler::removeAction(QAction* action)
{
    m_actions.removeAll(action);
    m_shortcuts.remove(action);
    m_defaultShortcuts.remove(action);
}

int ShortcutsHandler::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);

    return 2;
}

int ShortcutsHandler::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);

    return m_actions.count();
}

Qt::ItemFlags ShortcutsHandler::flags(const QModelIndex& index) const
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

QVariant ShortcutsHandler::headerData(int section, Qt::Orientation orientation, int role) const
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

QVariant ShortcutsHandler::data(const QModelIndex& index, int role) const
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

bool ShortcutsHandler::setData(const QModelIndex& index, const QVariant& value, int role)
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

void ShortcutsHandler::accept()
{
    for(QMap< QAction*, QKeySequence >::iterator iterator = m_shortcuts.begin(); iterator != m_shortcuts.end(); ++iterator)
    {
        iterator.key()->setShortcut(iterator.value());
    }

    foreach(QAction* action, m_actions)
    {
        m_settings->setShortcut(action->objectName(), action->shortcut());
    }

    DocumentView::setSkipBackwardShortcut(m_skipBackwardAction->shortcut());
    DocumentView::setSkipForwardShortcut(m_skipForwardAction->shortcut());

    DocumentView::setMovementShortcuts(DocumentView::MoveUp, m_moveUpAction->shortcut());
    DocumentView::setMovementShortcuts(DocumentView::MoveDown, m_moveDownAction->shortcut());
    DocumentView::setMovementShortcuts(DocumentView::MoveLeft, m_moveLeftAction->shortcut());
    DocumentView::setMovementShortcuts(DocumentView::MoveRight, m_moveRightAction->shortcut());

    DocumentView::setReturnToPageShortcut(m_returnToPageAction->shortcut());
}

void ShortcutsHandler::reset()
{
    m_shortcuts = m_defaultShortcuts;

    emit dataChanged(createIndex(0, 1), createIndex(m_actions.count(), 1));
}
