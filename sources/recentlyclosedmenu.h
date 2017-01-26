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

#ifndef RECENTLYCLOSEDMENU_H
#define RECENTLYCLOSEDMENU_H

#include "miscellaneous.h"

namespace qpdfview
{

class RecentlyClosedMenu : public ToolTipMenu
{
    Q_OBJECT

public:
    explicit RecentlyClosedMenu(int count, QWidget *parent = 0);

    void addTabAction(QAction* tabAction);

signals:
    void tabActionTriggered(QAction* tabAction);

public slots:
    void triggerFirstTabAction();
    void triggerLastTabAction();

protected slots:
    void on_tabAction_triggered(QAction* tabAction);
    void on_clearList_triggered();

private:
    Q_DISABLE_COPY(RecentlyClosedMenu)

    int m_count;

    QActionGroup* m_tabActionGroup;
    QAction* m_separatorAction;
    QAction* m_clearListAction;

};

} // qpdfview

#endif // RECENTLYCLOSEDMENU_H
