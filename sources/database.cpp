/*

Copyright 2012-2013 Adam Reichold
Copyright 2012 Michał Trybus

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

#include "database.h"

#include <QApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QDir>

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

#include <QStandardPaths>

#else

#include <QDesktopServices>

#endif // QT_VERSION

#ifdef WITH_SQL

#include <QSqlError>
#include <QSqlQuery>

#endif // WITH_SQL

#include "settings.h"
#include "documentview.h"
#include "bookmarkmenu.h"

Database* Database::s_instance = 0;

Database* Database::instance()
{
    if(s_instance == 0)
    {
        s_instance = new Database(qApp);
    }

    return s_instance;
}

Database::~Database()
{
    s_instance = 0;
}

QStringList Database::loadInstanceNames()
{
    QStringList instanceNames;

#ifdef WITH_SQL

    if(m_database.isOpen())
    {
        m_database.transaction();

        QSqlQuery query(m_database);
        query.exec("SELECT DISTINCT(instanceName) FROM tabs_v2");

        while(query.next())
        {
            if(!query.isActive())
            {
                qDebug() << query.lastError();
                break;
            }

            if(!query.value(0).toString().isEmpty())
            {
                instanceNames.append(query.value(0).toString());
            }
        }

        m_database.commit();
    }

#endif // WITH_SQL

    return instanceNames;
}

void Database::restoreTabs()
{
#ifdef WITH_SQL

    if(m_database.isOpen())
    {
        m_database.transaction();

        QSqlQuery query(m_database);
        query.prepare("SELECT filePath,currentPage,continuousMode,layoutMode,scaleMode,scaleFactor,rotation FROM tabs_v2 WHERE instanceName==?");

        query.bindValue(0, qApp->objectName());

        query.exec();

        while(query.next())
        {
            if(!query.isActive())
            {
                qDebug() << query.lastError();
                break;
            }

            emit tabRestored(query.value(0).toString(),
                             static_cast< bool >(query.value(2).toUInt()),
                             static_cast< LayoutMode >(query.value(3).toUInt()),
                             static_cast< ScaleMode >(query.value(4).toUInt()),
                             query.value(5).toReal(),
                             static_cast< Rotation >(query.value(6).toUInt()),
                             query.value(1).toInt());
        }

        m_database.commit();
    }

#endif // WITH_SQL
}

void Database::saveTabs(const QList< const DocumentView* >& tabs)
{
#ifdef WITH_SQL

    if(m_database.isOpen())
    {
        m_database.transaction();

        QSqlQuery query(m_database);

        if(Settings::instance()->mainWindow().restoreTabs())
        {
            query.prepare("DELETE FROM tabs_v2 WHERE instanceName==?");

            query.bindValue(0, qApp->objectName());

            query.exec();

            if(!query.isActive())
            {
                qDebug() << query.lastError();
            }

            query.prepare("INSERT INTO tabs_v2 "
                          "(filePath,instanceName,currentPage,continuousMode,layoutMode,scaleMode,scaleFactor,rotation)"
                          " VALUES (?,?,?,?,?,?,?,?)");

            foreach(const DocumentView* tab, tabs)
            {
                query.bindValue(0, QFileInfo(tab->filePath()).absoluteFilePath());
                query.bindValue(1, qApp->objectName());
                query.bindValue(2, tab->currentPage());

                query.bindValue(3, static_cast< uint >(tab->continousMode()));
                query.bindValue(4, static_cast< uint >(tab->layoutMode()));

                query.bindValue(5, static_cast< uint >(tab->scaleMode()));
                query.bindValue(6, tab->scaleFactor());

                query.bindValue(7, static_cast< uint >(tab->rotation()));

                query.exec();

                if(!query.isActive())
                {
                    qDebug() << query.lastError();
                }
            }
        }
        else
        {
            query.exec("DELETE FROM tabs_v2");

            if(!query.isActive())
            {
                qDebug() << query.lastError();
            }
        }

        m_database.commit();
    }

#else

    Q_UNUSED(tabs);

#endif // WITH_SQL
}

void Database::restoreBookmarks()
{
#ifdef WITH_SQL

    if(m_database.isOpen())
    {
        m_database.transaction();

        QSqlQuery query(m_database);
        query.exec("SELECT DISTINCT(filePath) FROM bookmarks_v2");

        while(query.next())
        {
            if(!query.isActive())
            {
                qDebug() << query.lastError();
                break;
            }

            const QString filePath = query.value(0).toString();
            QList< QPair< int, QString > > jumps;

            QSqlQuery innerQuery(m_database);
            innerQuery.prepare("SELECT page,label FROM bookmarks_v2 WHERE filePath==?");

            innerQuery.bindValue(0, filePath);

            innerQuery.exec();

            while(innerQuery.next())
            {
                if(!innerQuery.isActive())
                {
                    qDebug() << innerQuery.lastError();
                }

                const int page = innerQuery.value(0).toInt();
                const QString label = innerQuery.value(1).toString();

                jumps.append(qMakePair(page, label));
            }

            emit bookmarkRestored(filePath, jumps);
        }

        m_database.commit();
    }

#endif // WITH_SQL
}

void Database::saveBookmarks(const QList< const BookmarkMenu* >& bookmarks)
{
#ifdef WITH_SQL

    if(m_database.isOpen())
    {
        m_database.transaction();

        QSqlQuery query(m_database);
        query.exec("DELETE FROM bookmarks_v2");

        if(!query.isActive())
        {
            qDebug() << query.lastError();
        }

        if(Settings::instance()->mainWindow().restoreBookmarks())
        {
            query.prepare("INSERT INTO bookmarks_v2 "
                          "(filePath,page,label)"
                          " VALUES (?,?,?)");

            foreach(const BookmarkMenu* bookmark, bookmarks)
            {
                foreach(const Jump jump, bookmark->jumps())
                {
                    query.bindValue(0, QFileInfo(bookmark->filePath()).absoluteFilePath());
                    query.bindValue(1, jump.first);
                    query.bindValue(2, jump.second);

                    query.exec();

                    if(!query.isActive())
                    {
                        qDebug() << query.lastError();
                    }
                }
            }
        }

        m_database.commit();
    }

#else

    Q_UNUSED(bookmarks);

#endif // WITH_SQL
}

void Database::restorePerFileSettings(DocumentView* tab)
{
#ifdef WITH_SQL

    if(Settings::instance()->mainWindow().restorePerFileSettings() && m_database.isOpen() && tab != 0)
    {
        m_database.transaction();

        QSqlQuery query(m_database);
        query.prepare("SELECT currentPage,continuousMode,layoutMode,scaleMode,scaleFactor,rotation FROM perfilesettings_v1 WHERE filePath==?");

        query.bindValue(0, QCryptographicHash::hash(QFileInfo(tab->filePath()).absoluteFilePath().toUtf8(), QCryptographicHash::Sha1).toBase64());

        query.exec();

        if(query.next())
        {
            tab->setContinousMode(query.value(1).toBool());
            tab->setLayoutMode(static_cast< LayoutMode >(query.value(2).toUInt()));

            tab->setScaleMode(static_cast< ScaleMode >(query.value(3).toUInt()));
            tab->setScaleFactor(query.value(4).toReal());

            tab->setRotation(static_cast< Rotation >(query.value(5).toUInt()));

            tab->jumpToPage(query.value(0).toInt(), false);
        }

        if(!query.isActive())
        {
            qDebug() << query.lastError();
        }

        m_database.commit();
    }

#else

    Q_UNUSED(tab);

#endif // WITH_SQL
}

void Database::savePerFileSettings(const DocumentView* tab)
{
#ifdef WITH_SQL

    if(Settings::instance()->mainWindow().restorePerFileSettings() && m_database.isOpen() && tab != 0)
    {
        m_database.transaction();

        QSqlQuery query(m_database);
        query.prepare("INSERT OR REPLACE INTO perfilesettings_v1 "
                      "(lastUsed,filePath,currentPage,continuousMode,layoutMode,scaleMode,scaleFactor,rotation)"
                      " VALUES (?,?,?,?,?,?,?,?)");

        query.bindValue(0, QDateTime::currentDateTime().toTime_t());

        query.bindValue(1, QCryptographicHash::hash(QFileInfo(tab->filePath()).absoluteFilePath().toUtf8(), QCryptographicHash::Sha1).toBase64());
        query.bindValue(2, tab->currentPage());

        query.bindValue(3, static_cast< uint >(tab->continousMode()));
        query.bindValue(4, static_cast< uint >(tab->layoutMode()));

        query.bindValue(5, static_cast< uint >(tab->scaleMode()));
        query.bindValue(6, tab->scaleFactor());

        query.bindValue(7, static_cast< uint >(tab->rotation()));

        query.exec();

        if(!query.isActive())
        {
            qDebug() << query.lastError();
        }

        m_database.commit();
    }

#else

    Q_UNUSED(tab);

#endif // WITH_SQL
}

Database::Database(QObject* parent) : QObject(parent)
{
#ifdef WITH_SQL

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

    const QString path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);

#else

    const QString path = QDesktopServices::storageLocation(QDesktopServices::DataLocation);

#endif // QT_VERSION

    QDir().mkpath(path);

    m_database = QSqlDatabase::addDatabase("QSQLITE");
    m_database.setDatabaseName(QDir(path).filePath("database"));
    m_database.open();

    if(m_database.isOpen())
    {
        m_database.transaction();

        const QStringList tables = m_database.tables();
        QSqlQuery query(m_database);

        // tabs

        if(!tables.contains("tabs_v2"))
        {
            query.exec("CREATE TABLE tabs_v2 "
                       "(filePath TEXT"
                       ",instanceName TEXT"
                       ",currentPage INTEGER"
                       ",continuousMode INTEGER"
                       ",layoutMode INTEGER"
                       ",scaleMode INTEGER"
                       ",scaleFactor REAL"
                       ",rotation INTEGER)");

            if(!query.isActive())
            {
                qDebug() << query.lastError();
            }

            if(tables.contains("tabs_v1"))
            {
                migrateTabs_v1_v2();
            }
        }

        // bookmarks

        if(!tables.contains("bookmarks_v2"))
        {
            query.exec("CREATE TABLE bookmarks_v2 "
                       "(filePath TEXT"
                       ",page INTEGER"
                       ",label TEXT)");

            if(!query.isActive())
            {
                qDebug() << query.lastError();
            }

            if(tables.contains("bookmarks_v1"))
            {
                migrateBookmarks_v1_v2();
            }
        }

        // per-file settings

        if(!tables.contains("perfilesettings_v1"))
        {
            query.exec("CREATE TABLE perfilesettings_v1 "
                       "(lastUsed INTEGER"
                       ",filePath TEXT PRIMARY KEY"
                       ",currentPage INTEGER"
                       ",continuousMode INTEGER"
                       ",layoutMode INTEGER"
                       ",scaleMode INTEGER"
                       ",scaleFactor REAL"
                       ",rotation INTEGER)");

            if(!query.isActive())
            {
                qDebug() << query.lastError();
            }
        }

        if(Settings::instance()->mainWindow().restorePerFileSettings())
        {
            query.exec("DELETE FROM perfilesettings_v1 WHERE filePath IN (SELECT filePath FROM perfilesettings_v1 ORDER BY lastUsed DESC LIMIT -1 OFFSET 1000)");
        }
        else
        {
            query.exec("DELETE FROM perfilesettings_v1");
        }

        if(!query.isActive())
        {
            qDebug() << query.lastError();
        }

        m_database.commit();
    }
    else
    {
        qDebug() << m_database.lastError();
    }

#endif // WITH_SQL
}

#ifdef WITH_SQL

void Database::migrateTabs_v1_v2()
{
    QSqlQuery query(m_database);
    query.prepare("INSERT INTO tabs_v2 "
                  "SELECT filePath,?,currentPage,continuousMode,layoutMode,scaleMode,scaleFactor,rotation "
                  "FROM tabs_v1");

    query.bindValue(0, qApp->objectName());

    query.exec();

    if(!query.isActive())
    {
        qDebug() << query.lastError();
        return;
    }

    qWarning() << "Migrated tabs from v1 to v2, dropping v1.";
    query.exec("DROP TABLE tabs_v1");
}

void Database::migrateBookmarks_v1_v2()
{
    QSqlQuery query(m_database);
    query.exec("SELECT filePath,pages FROM bookmarks_v1");

    while(query.next())
    {
        if(!query.isActive())
        {
            qDebug() << query.lastError();
            return;
        }

        QSqlQuery innerQuery(m_database);
        innerQuery.prepare("INSERT INTO bookmarks_v2 "
                           "(filePath,page,label)"
                           " VALUES (?,?,?)");

        innerQuery.bindValue(0, query.value(0));

        foreach(QString page, query.value(1).toString().split(",", QString::SkipEmptyParts))
        {
            innerQuery.bindValue(1, page);
            innerQuery.bindValue(2, tr("Jump to page %1").arg(page));

            innerQuery.exec();

            if(!innerQuery.isActive())
            {
                qDebug() << innerQuery.lastError();
                return;
            }
        }
    }

    if(!query.isActive())
    {
        qDebug() << query.lastError();
        return;
    }

    qWarning() << "Migrated bookmarks from v1 to v2, dropping v1.";
    query.exec("DROP TABLE bookmarks_v1");
}

#endif // WITH_SQL
