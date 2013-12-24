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

#include "recentlyclosedmenu.h"

#include "documentview.h"

static inline DocumentView* storedTab(QAction* action)
{
    return reinterpret_cast< DocumentView* >(action->data().toULongLong());
}

RecentlyClosedMenu::RecentlyClosedMenu(int count, QWidget* parent) : QMenu(parent),
    m_count(count)
{
    menuAction()->setText(tr("Recently closed"));

    m_restoreActionGroup = new QActionGroup(this);
    connect(m_restoreActionGroup, SIGNAL(triggered(QAction*)), SLOT(on_restore_triggered(QAction*)));

    m_separatorAction = addSeparator();

    m_clearListAction = addAction(tr("&Clear list"));
    connect(m_clearListAction, SIGNAL(triggered()), SLOT(on_clearList_triggered()));
}

void RecentlyClosedMenu::addRestoreAction(DocumentView* tab)
{
    // TODO
}

void RecentlyClosedMenu::on_restore_triggered(QAction* action)
{
    emit restoreTriggered(storedTab(action));

    action->deleteLater();
}

void RecentlyClosedMenu::on_clearList_triggered()
{
    foreach(QAction* action, m_restoreActionGroup->actions())
    {
        delete storedTab(action);

        delete action;
    }
}
