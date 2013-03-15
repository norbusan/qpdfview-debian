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

#include "shortcuthandler.h"

#include <QAction>
#include <QSettings>

#include "documentview.h"
#include "settings.h"

ShortcutHandler::ShortcutHandler(QObject* parent) : QAbstractTableModel(parent),
    m_settings(0),
    m_actions(),
    m_shortcuts(),
    m_defaultShortcuts()
{
    m_settings = new QSettings("qpdfview", "shortcuts", this);

    // skip backward shortcut

    m_skipBackwardAction = new QAction(tr("Skip backward"), this);
    m_skipBackwardAction->setObjectName(QLatin1String("skipBackward"));

    m_skipBackwardAction->setShortcut(::DocumentView::skipBackwardShortcut());
    registerAction(m_skipBackwardAction);

    // skip forward shortcut

    m_skipForwardAction = new QAction(tr("Skip forward"), this);
    m_skipForwardAction->setObjectName(QLatin1String("skipForward"));

    m_skipForwardAction->setShortcut(::DocumentView::skipForwardShortcut());
    registerAction(m_skipForwardAction);

    // move up shortcut

    m_moveUpAction = new QAction(tr("Move up"), this);
    m_moveUpAction->setObjectName(QLatin1String("moveUp"));

    m_moveUpAction->setShortcut(::DocumentView::moveUpShortcut());
    registerAction(m_moveUpAction);

    // move down shortcut

    m_moveDownAction = new QAction(tr("Move down"), this);
    m_moveDownAction->setObjectName(QLatin1String("moveDown"));

    m_moveDownAction->setShortcut(::DocumentView::moveDownShortcut());
    registerAction(m_moveDownAction);

    // move left shortcut

    m_moveLeftAction = new QAction(tr("Move left"), this);
    m_moveLeftAction->setObjectName(QLatin1String("moveLeft"));

    m_moveLeftAction->setShortcut(::DocumentView::moveLeftShortcut());
    registerAction(m_moveLeftAction);

    // move right shortcut

    m_moveRightAction = new QAction(tr("Move right"), this);
    m_moveRightAction->setObjectName(QLatin1String("moveRight"));

    m_moveRightAction->setShortcut(::DocumentView::moveRightShortcut());
    registerAction(m_moveRightAction);

    // return to page shortcut

    m_returnToPageAction = new QAction(tr("Return to page"), this);
    m_returnToPageAction->setObjectName(QLatin1String("returnToPage"));

    m_returnToPageAction->setShortcut(::DocumentView::returnToPageShortcut());
    registerAction(m_returnToPageAction);
}

void ShortcutHandler::registerAction(QAction* action)
{
    Q_ASSERT(!action->objectName().isEmpty());

    QKeySequence defaultShortcut = action->shortcut();
    QKeySequence shortcut = m_settings->value(action->objectName(), action->shortcut()).value< QKeySequence >();

    action->setShortcut(shortcut);

    m_actions.append(action);
    m_shortcuts.insert(action, shortcut);
    m_defaultShortcuts.insert(action, defaultShortcut);
}

int ShortcutHandler::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);

    return 2;
}

int ShortcutHandler::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);

    return m_actions.count();
}

Qt::ItemFlags ShortcutHandler::flags(const QModelIndex& index) const
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

QVariant ShortcutHandler::headerData(int section, Qt::Orientation orientation, int role) const
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

QVariant ShortcutHandler::data(const QModelIndex& index, int role) const
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

bool ShortcutHandler::setData(const QModelIndex& index, const QVariant& value, int role)
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

bool ShortcutHandler::submit()
{
    for(QMap< QAction*, QKeySequence >::iterator iterator = m_shortcuts.begin(); iterator != m_shortcuts.end(); ++iterator)
    {
        iterator.key()->setShortcut(iterator.value());
    }

    foreach(QAction* action, m_actions)
    {
        m_settings->setValue(action->objectName(), action->shortcut().toString(QKeySequence::PortableText));
    }

    DocumentView::setSkipBackwardShortcut(m_skipBackwardAction->shortcut());
    DocumentView::setSkipForwardShortcut(m_skipForwardAction->shortcut());

    DocumentView::setMoveUpShortcut(m_moveUpAction->shortcut());
    DocumentView::setMoveDownShortcut(m_moveDownAction->shortcut());
    DocumentView::setMoveLeftShortcut(m_moveLeftAction->shortcut());
    DocumentView::setMoveRightShortcut(m_moveRightAction->shortcut());

    DocumentView::setReturnToPageShortcut(m_returnToPageAction->shortcut());

    return true;
}

void ShortcutHandler::revert()
{
    for(QMap< QAction*, QKeySequence >::iterator iterator = m_shortcuts.begin(); iterator != m_shortcuts.end(); ++iterator)
    {
        iterator.value() = iterator.key()->shortcut();
    }
}

void ShortcutHandler::reset()
{
    m_shortcuts = m_defaultShortcuts;

    emit dataChanged(createIndex(0, 1), createIndex(m_actions.count(), 1));
}
