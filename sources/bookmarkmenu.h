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

#ifndef BOOKMARKMENU_H
#define BOOKMARKMENU_H

#include <QMenu>

#include "global.h"

class BookmarkMenu : public QMenu
{
    Q_OBJECT

public:
    BookmarkMenu(const QString& filePath, QWidget* parent = 0);

    void addJumpToPageAction(int page, const QString& label);
    void removeJumpToPageAction(int page);

    QString filePath() const;
    JumpList jumps() const;

signals:
    void openTriggered(const QString& filePath);
    void openInNewTabTriggered(const QString& filePath);
    void jumpToPageTriggered(const QString& filePath, int page);

protected slots:
    void on_open_triggered();
    void on_openInNewTab_triggered();
    void on_jumpToPage_triggered(QAction* action);
    void on_removeBookmark_triggered();

private:
    Q_DISABLE_COPY(BookmarkMenu)

    QAction* m_openAction;
    QAction* m_openInNewTabAction;
    QActionGroup* m_jumpToPageActionGroup;
    QAction* m_separatorAction;
    QAction* m_removeBookmarkAction;

};

#endif // BOOKMARKMENU_H
