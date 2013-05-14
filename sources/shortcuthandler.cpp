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

#include <QApplication>
#include <QSettings>

#include "documentview.h"

static QList< QKeySequence > toShortcuts(const QStringList& stringList)
{
    QList< QKeySequence > shortcuts;

    foreach(const QString& string, stringList)
    {
        QKeySequence shortcut(string.trimmed());

        if(!shortcut.isEmpty())
        {
            shortcuts.append(shortcut);
        }
    }

    return shortcuts;
}

static QStringList toStringList(const QList< QKeySequence >& shortcuts, QKeySequence::SequenceFormat format = QKeySequence::PortableText)
{
    QStringList stringList;

    foreach(const QKeySequence& shortcut, shortcuts)
    {
        stringList.append(shortcut.toString(format));
    }

    return stringList;
}

static bool matches(const QKeySequence& keySequence, const QList< QKeySequence >& shortcuts)
{
    foreach(const QKeySequence& shortcut, shortcuts)
    {
        if(keySequence.matches(shortcut) == QKeySequence::ExactMatch)
        {
            return true;
        }
    }

    return false;
}

ShortcutHandler* ShortcutHandler::s_instance = 0;

ShortcutHandler* ShortcutHandler::instance()
{
    if(s_instance == 0)
    {
        s_instance = new ShortcutHandler(qApp);
    }

    return s_instance;
}

ShortcutHandler::~ShortcutHandler()
{
    s_instance = 0;
}

void ShortcutHandler::registerAction(QAction* action)
{
    Q_ASSERT(!action->objectName().isEmpty());

    const QList< QKeySequence > defaultShortcuts = action->shortcuts();
    const QList< QKeySequence > shortcuts = toShortcuts(m_settings->value(action->objectName(), toStringList(defaultShortcuts)).toStringList());

    action->setShortcuts(shortcuts);

    m_actions.append(action);
    m_shortcuts.insert(action, shortcuts);
    m_defaultShortcuts.insert(action, defaultShortcuts);
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
        QAction* action = m_actions.at(index.row());

        switch(index.column())
        {
        case 0:
            return action->text().remove(QLatin1Char('&'));
            break;
        case 1:
            return toStringList(m_shortcuts.value(action), QKeySequence::NativeText).join(";");
            break;
        }
    }

    return QVariant();
}

bool ShortcutHandler::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if(role == Qt::EditRole && index.column() == 1 && index.row() >= 0 && index.row() < m_actions.count())
    {
        QList< QKeySequence > shortcuts = toShortcuts(value.toString().split(";", QString::SkipEmptyParts));

        if(!shortcuts.isEmpty() || value.toString().isEmpty())
        {
            m_shortcuts.insert(m_actions.at(index.row()), shortcuts);

            emit dataChanged(index, index);

            return true;
        }
    }

    return false;
}

bool ShortcutHandler::matchesSkipBackward(const QKeySequence& keySequence) const
{
    return matches(keySequence, m_skipBackwardAction->shortcuts());
}

bool ShortcutHandler::matchesSkipForward(const QKeySequence& keySequence) const
{
    return matches(keySequence, m_skipForwardAction->shortcuts());
}

bool ShortcutHandler::matchesMoveUp(const QKeySequence& keySequence) const
{
    return matches(keySequence, m_moveUpAction->shortcuts());
}

bool ShortcutHandler::matchesMoveDown(const QKeySequence& keySequence) const
{
    return matches(keySequence, m_moveDownAction->shortcuts());
}

bool ShortcutHandler::matchesMoveLeft(const QKeySequence& keySequence) const
{
    return matches(keySequence, m_moveLeftAction->shortcuts());
}

bool ShortcutHandler::matchesMoveRight(const QKeySequence& keySequence) const
{
    return matches(keySequence, m_moveRightAction->shortcuts());
}

bool ShortcutHandler::submit()
{
    for(QMap< QAction*, QList< QKeySequence > >::iterator iterator = m_shortcuts.begin(); iterator != m_shortcuts.end(); ++iterator)
    {
        iterator.key()->setShortcuts(iterator.value());
    }

    foreach(const QAction* action, m_actions)
    {
        m_settings->setValue(action->objectName(), toStringList(action->shortcuts()));
    }

    return true;
}

void ShortcutHandler::revert()
{
    for(QMap< QAction*, QList< QKeySequence > >::iterator iterator = m_shortcuts.begin(); iterator != m_shortcuts.end(); ++iterator)
    {
        iterator.value() = iterator.key()->shortcuts();
    }
}

void ShortcutHandler::reset()
{
    for(QMap< QAction*, QList< QKeySequence > >::iterator iterator = m_defaultShortcuts.begin(); iterator != m_defaultShortcuts.end(); ++iterator)
    {
        m_shortcuts.insert(iterator.key(), iterator.value());
    }

    emit dataChanged(createIndex(0, 1), createIndex(m_actions.count(), 1));
}

ShortcutHandler::ShortcutHandler(QObject* parent) : QAbstractTableModel(parent),
    m_settings(new QSettings("qpdfview", "shortcuts", this)),
    m_actions(),
    m_shortcuts(),
    m_defaultShortcuts()
{
    // skip backward shortcut

    m_skipBackwardAction = new QAction(tr("Skip backward"), this);
    m_skipBackwardAction->setObjectName(QLatin1String("skipBackward"));
    m_skipBackwardAction->setShortcut(QKeySequence(Qt::Key_PageUp));
    registerAction(m_skipBackwardAction);

    // skip forward shortcut

    m_skipForwardAction = new QAction(tr("Skip forward"), this);
    m_skipForwardAction->setObjectName(QLatin1String("skipForward"));
    m_skipForwardAction->setShortcut(QKeySequence(Qt::Key_PageDown));
    registerAction(m_skipForwardAction);

    // move up shortcut

    m_moveUpAction = new QAction(tr("Move up"), this);
    m_moveUpAction->setObjectName(QLatin1String("moveUp"));
    m_moveUpAction->setShortcut(QKeySequence(Qt::Key_Up));
    registerAction(m_moveUpAction);

    // move down shortcut

    m_moveDownAction = new QAction(tr("Move down"), this);
    m_moveDownAction->setObjectName(QLatin1String("moveDown"));
    m_moveDownAction->setShortcut(QKeySequence(Qt::Key_Down));
    registerAction(m_moveDownAction);

    // move left shortcut

    m_moveLeftAction = new QAction(tr("Move left"), this);
    m_moveLeftAction->setObjectName(QLatin1String("moveLeft"));
    m_moveLeftAction->setShortcut(QKeySequence(Qt::Key_Left));
    registerAction(m_moveLeftAction);

    // move right shortcut

    m_moveRightAction = new QAction(tr("Move right"), this);
    m_moveRightAction->setObjectName(QLatin1String("moveRight"));
    m_moveRightAction->setShortcut(QKeySequence(Qt::Key_Right));
    registerAction(m_moveRightAction);
}
