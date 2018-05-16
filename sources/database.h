/*

Copyright 2014 S. Razi Alavizadeh
Copyright 2013-2018 Adam Reichold
Copyright 2018 Egor Zenkov

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

class QDateTime;

#include "global.h"

namespace qpdfview
{

class DocumentView;

class Database : public QObject
{
    Q_OBJECT

public:
    static Database* instance();
    ~Database();

    QStringList knownInstanceNames();

    struct RestoreTab
    {
        virtual DocumentView* operator()(const QString& absoluteFilePath) const = 0;
    };

    void restoreTabs(const RestoreTab& restoreTab);
    void saveTabs(const QVector< DocumentView* >& tabs);
    void clearTabs();

    void restoreBookmarks();
    void saveBookmarks();
    void clearBookmarks();

    void restorePerFileSettings(DocumentView* tab);
    void savePerFileSettings(const DocumentView* tab);

private:
    Q_DISABLE_COPY(Database)

    static Database* s_instance;
    Database(QObject* parent = 0);

    static QString instanceName();

#ifdef WITH_SQL

    bool prepareTabs_v5();
    bool prepareBookmarks_v3();
    bool preparePerFileSettings_v4();
    bool preparePerFileSettings_Outline_v1();

    void migrateTabs_v4_v5();
    void migrateTabs_v3_v5();
    void migrateTabs_v2_v5();
    void migrateTabs_v1_v5();
    void migrateBookmarks_v2_v3();
    void migrateBookmarks_v1_v3();
    void migratePerFileSettings_v3_v4();
    void migratePerFileSettings_v2_v4();
    void migratePerFileSettings_v1_v4();

    bool prepareTable(const QString& prepare);
    void migrateTable(const QString& migrate, const QString& prune, const QString& warning);

    void limitPerFileSettings();

    QSqlDatabase m_database;

#endif // WITH_SQL

};

} // qpdfview

#endif // DATABASE_H
