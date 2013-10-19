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

#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>

#ifdef WITH_SQL

#include <QSqlDatabase>

#endif // WITH_SQL

#include "global.h"

class DocumentView;
class BookmarkMenu;

class Database : public QObject
{
    Q_OBJECT

public:
    static Database* instance();
    ~Database();

    QStringList loadInstanceNames();

    void restoreTabs();
    void saveTabs(const QList< const DocumentView* >& tabs);

    void restoreBookmarks();
    void saveBookmarks(const QList< const BookmarkMenu* >& bookmarks);

    void restorePerFileSettings(DocumentView* tab);
    void savePerFileSettings(const DocumentView* tab);

signals:
    void tabRestored(const QString& filePath, bool continousMode, LayoutMode layoutMode, ScaleMode scaleMode, qreal scaleFactor, Rotation rotation, int currentPage);
    void bookmarkRestored(const QString& filePath, const JumpList& pages);

private:
    Q_DISABLE_COPY(Database)

    static Database* s_instance;
    Database(QObject* parent = 0);

#ifdef WITH_SQL

    void migrateTabs_v1_v2();
    void migrateBookmarks_v1_v2();

    QSqlDatabase m_database;

#endif // WITH_SQL

};

#endif // DATABASE_H
