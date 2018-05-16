/*

Copyright 2012-2013 Adam Reichold

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

#ifndef RECENTLYUSEDMENU_H
#define RECENTLYUSEDMENU_H

#include "miscellaneous.h"

class QFileInfo;

namespace qpdfview
{

class RecentlyUsedMenu : public ToolTipMenu
{
    Q_OBJECT

public:
    explicit RecentlyUsedMenu(const QStringList& filePaths, int count, QWidget* parent = 0);

    void addOpenAction(const QFileInfo& fileInfo);
    void removeOpenAction(const QString& filePath);

    QStringList filePaths() const;

signals:
    void openTriggered(const QString& filePath);

protected slots:
    void on_open_triggered(QAction* action);
    void on_clearList_triggered();

private:
    Q_DISABLE_COPY(RecentlyUsedMenu)

    int m_count;

    QActionGroup* m_openActionGroup;
    QAction* m_separatorAction;
    QAction* m_clearListAction;

};

} // qpdfview

#endif // RECENTLYUSEDMENU_H
