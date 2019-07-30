/*

Copyright 2013, 2016 Adam Reichold

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

#ifndef SHORTCUTHANDLER_H
#define SHORTCUTHANDLER_H

#include <QAbstractTableModel>
#include <QAction>
#include <QKeySequence>

class QSettings;

namespace qpdfview
{

class ShortcutHandler : public QAbstractTableModel
{
    Q_OBJECT

public:
    static ShortcutHandler* instance();
    ~ShortcutHandler();

    void registerAction(QAction* action);

    int columnCount(const QModelIndex& parent) const;
    int rowCount(const QModelIndex& parent) const;

    Qt::ItemFlags flags(const QModelIndex& index) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    QVariant data(const QModelIndex& index, int role) const;
    bool setData(const QModelIndex& index, const QVariant& value, int role);

    bool matchesSkipBackward(const QKeySequence& keySequence) const;
    bool matchesSkipForward(const QKeySequence& keySequence) const;

    bool matchesMoveUp(const QKeySequence& keySequence) const;
    bool matchesMoveDown(const QKeySequence& keySequence) const;
    bool matchesMoveLeft(const QKeySequence& keySequence) const;
    bool matchesMoveRight(const QKeySequence& keySequence) const;

public slots:
    bool submit();
    void revert();

    void reset();

private:
    Q_DISABLE_COPY(ShortcutHandler)

    static ShortcutHandler* s_instance;
    ShortcutHandler(QObject* parent = 0);

    QSettings* m_settings;

    QList< QAction* > m_actions;

    typedef QHash< QAction*, QList< QKeySequence > > Shortcuts;
    Shortcuts m_shortcuts;
    Shortcuts m_defaultShortcuts;

    QAction* m_skipBackwardAction;
    QAction* m_skipForwardAction;

    QAction* m_moveUpAction;
    QAction* m_moveDownAction;
    QAction* m_moveLeftAction;
    QAction* m_moveRightAction;

    QAction* createAction(const QString& text, const QString& objectName, const QList< QKeySequence >& shortcuts);

};

} // qpdfview

#endif // SHORTCUTHANDLER_H
