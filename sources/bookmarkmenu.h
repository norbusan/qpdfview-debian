/*

Copyright 2014 S. Razi Alavizadeh
Copyright 2012-2014 Adam Reichold

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

#include <QPair>

class QFileInfo;

#include "global.h"

namespace qpdfview
{

class BookmarkMenu : public QMenu
{
    Q_OBJECT

public:
    BookmarkMenu(const QFileInfo& fileInfo, QWidget* parent = 0);

    QString absoluteFilePath() const { return menuAction()->data().toString(); }

    void addJumpToPageAction(int page, const QString& label);
    void removeJumpToPageAction(int page);

signals:
    void openTriggered(const QString& filePath);
    void openInNewTabTriggered(const QString& filePath);
    void jumpToPageTriggered(const QString& filePath, int page);
    void removeBookmarkTriggered(const QString& filePath);

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

} // qpdfview

#endif // BOOKMARKMENU_H
