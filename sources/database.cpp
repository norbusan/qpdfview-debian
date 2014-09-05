/*

Copyright 2012-2014 Adam Reichold
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
#include <QStandardItemModel>

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

#ifdef WITH_SQL

namespace
{

class Transaction
{
public:
    Transaction(QSqlDatabase& database) :  m_committed(false), m_database(database)
    {
        m_database.transaction();
    }

    ~Transaction()
    {
        if(!m_committed)
        {
            m_database.rollback();
        }
    }

    void commit()
    {
        m_committed = m_database.commit();
    }

private:
    bool m_committed;
    QSqlDatabase& m_database;

};

} // anonymous

#endif // WITH_SQL

namespace qpdfview
{

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
        Transaction transaction(m_database);

        QSqlQuery query(m_database);
        query.exec("SELECT DISTINCT(instanceName) FROM tabs_v2");

        while(query.next())
        {
            if(!query.isActive())
            {
                qDebug() << query.lastError();
                return QStringList();
            }

            if(!query.value(0).toString().isEmpty())
            {
                instanceNames.append(query.value(0).toString());
            }
        }

        transaction.commit();
    }

#endif // WITH_SQL

    return instanceNames;
}

void Database::restoreTabs()
{
#ifdef WITH_SQL

    if(m_database.isOpen())
    {
        Transaction transaction(m_database);

        QSqlQuery query(m_database);
        query.prepare("SELECT filePath,currentPage,continuousMode,layoutMode,rightToLeftMode,scaleMode,scaleFactor,rotation FROM tabs_v3 WHERE instanceName==?");

        query.bindValue(0, instanceName());

        query.exec();

        while(query.next())
        {
            if(!query.isActive())
            {
                qDebug() << query.lastError();
                return;
            }

            emit tabRestored(query.value(0).toString(),
                             static_cast< bool >(query.value(2).toUInt()),
                             static_cast< LayoutMode >(query.value(3).toUInt()),
                             query.value(4).toBool(),
                             static_cast< ScaleMode >(query.value(5).toUInt()),
                             query.value(6).toReal(),
                             static_cast< Rotation >(query.value(7).toUInt()),
                             query.value(1).toInt());
        }

        transaction.commit();
    }

#endif // WITH_SQL
}

void Database::saveTabs(const QList< const DocumentView* >& tabs)
{
#ifdef WITH_SQL

    if(m_database.isOpen())
    {
        Transaction transaction(m_database);

        QSqlQuery query(m_database);

        query.prepare("DELETE FROM tabs_v3 WHERE instanceName==?");

        query.bindValue(0, instanceName());

        query.exec();

        if(!query.isActive())
        {
            qDebug() << query.lastError();
            return;
        }

        query.prepare("INSERT INTO tabs_v3 "
                      "(filePath,instanceName,currentPage,continuousMode,layoutMode,rightToLeftMode,scaleMode,scaleFactor,rotation)"
                      " VALUES (?,?,?,?,?,?,?,?,?)");

        foreach(const DocumentView* tab, tabs)
        {
            query.bindValue(0, tab->fileInfo().absoluteFilePath());
            query.bindValue(1, instanceName());
            query.bindValue(2, tab->currentPage());

            query.bindValue(3, static_cast< uint >(tab->continuousMode()));
            query.bindValue(4, static_cast< uint >(tab->layoutMode()));
            query.bindValue(5, static_cast< uint >(tab->rightToLeftMode()));

            query.bindValue(6, static_cast< uint >(tab->scaleMode()));
            query.bindValue(7, tab->scaleFactor());

            query.bindValue(8, static_cast< uint >(tab->rotation()));

            query.exec();

            if(!query.isActive())
            {
                qDebug() << query.lastError();
                return;
            }
        }

        transaction.commit();
    }

#else

    Q_UNUSED(tabs);

#endif // WITH_SQL
}

void Database::clearTabs()
{
#ifdef WITH_SQL

    if(m_database.isOpen())
    {
        Transaction transaction(m_database);

        QSqlQuery query(m_database);
        query.exec("DELETE FROM tabs_v3");

        if(!query.isActive())
        {
            qDebug() << query.lastError();
            return;
        }

        transaction.commit();
    }

#endif // WITH_SQL
}

void Database::restoreBookmarks()
{
#ifdef WITH_SQL

    if(m_database.isOpen())
    {
        Transaction transaction(m_database);

        QSqlQuery outerQuery(m_database);
        outerQuery.exec("SELECT DISTINCT(filePath) FROM bookmarks_v2");

        while(outerQuery.next())
        {
            if(!outerQuery.isActive())
            {
                qDebug() << outerQuery.lastError();
                return;
            }

            const QString filePath = outerQuery.value(0).toString();
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
                    return;
                }

                const int page = innerQuery.value(0).toInt();
                const QString label = innerQuery.value(1).toString();

                jumps.append(qMakePair(page, label));
            }

            emit bookmarkRestored(filePath, jumps);
        }

        transaction.commit();
    }

#endif // WITH_SQL
}

void Database::saveBookmarks(const QHash< QString, QStandardItemModel* > &bookmarks)
{
#ifdef WITH_SQL

    if(m_database.isOpen())
    {
        Transaction transaction(m_database);

        QSqlQuery query(m_database);
        query.exec("DELETE FROM bookmarks_v2");

        if(!query.isActive())
        {
            qDebug() << query.lastError();
            return;
        }

        if(Settings::instance()->mainWindow().restoreBookmarks())
        {
            query.prepare("INSERT INTO bookmarks_v2 "
                          "(filePath,page,label)"
                          " VALUES (?,?,?)");

            QHash< QString, QStandardItemModel* >::const_iterator iterator = bookmarks.constBegin();
            while (iterator != bookmarks.constEnd())
            {
                QStandardItemModel* bookmarkModel = iterator.value();
                if (bookmarkModel == 0)
                {
                    ++iterator;
                    continue;
                }

                for (int i = 0; i < bookmarkModel->rowCount(); ++i)
                {
                    QStandardItem* item = bookmarkModel->item(i);
                    if (item == 0)
                    {
                        continue;
                    }

                    query.bindValue(0, iterator.key());
                    query.bindValue(1, item->data(Qt::UserRole + 1));
                    query.bindValue(2, item->text());

                    query.exec();

                    if(!query.isActive())
                    {
                        qDebug() << query.lastError();
                        return;
                    }
                }

                ++iterator;
            }
        }

        transaction.commit();
    }

#else

    Q_UNUSED(bookmarks);

#endif // WITH_SQL
}

void Database::clearBookmarks()
{
#ifdef WITH_SQL

    if(m_database.isOpen())
    {
        Transaction transaction(m_database);

        QSqlQuery query(m_database);
        query.exec("DELETE FROM bookmarks_v2");

        if(!query.isActive())
        {
            qDebug() << query.lastError();
            return;
        }

        transaction.commit();
    }

#endif // WITH_SQL
}

void Database::restorePerFileSettings(DocumentView* tab)
{
#ifdef WITH_SQL

    if(Settings::instance()->mainWindow().restorePerFileSettings() && m_database.isOpen() && tab != 0)
    {
        Transaction transaction(m_database);

        QSqlQuery query(m_database);
        query.prepare("SELECT currentPage,continuousMode,layoutMode,rightToLeftMode,scaleMode,scaleFactor,rotation FROM perfilesettings_v2 WHERE filePath==?");

        query.bindValue(0, QCryptographicHash::hash(tab->fileInfo().absoluteFilePath().toUtf8(), QCryptographicHash::Sha1).toBase64());

        query.exec();

        if(query.next())
        {
            tab->setContinuousMode(query.value(1).toBool());
            tab->setLayoutMode(static_cast< LayoutMode >(query.value(2).toUInt()));
            tab->setRightToLeftMode(query.value(3).toBool());

            tab->setScaleMode(static_cast< ScaleMode >(query.value(4).toUInt()));
            tab->setScaleFactor(query.value(5).toReal());

            tab->setRotation(static_cast< Rotation >(query.value(6).toUInt()));

            tab->jumpToPage(query.value(0).toInt(), false);
        }

        if(!query.isActive())
        {
            qDebug() << query.lastError();
            return;
        }

        transaction.commit();
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
        Transaction transaction(m_database);

        QSqlQuery query(m_database);
        query.prepare("INSERT OR REPLACE INTO perfilesettings_v2 "
                      "(lastUsed,filePath,currentPage,continuousMode,layoutMode,rightToLeftMode,scaleMode,scaleFactor,rotation)"
                      " VALUES (?,?,?,?,?,?,?,?,?)");

        query.bindValue(0, QDateTime::currentDateTime().toTime_t());

        query.bindValue(1, QCryptographicHash::hash(tab->fileInfo().absoluteFilePath().toUtf8(), QCryptographicHash::Sha1).toBase64());
        query.bindValue(2, tab->currentPage());

        query.bindValue(3, static_cast< uint >(tab->continuousMode()));
        query.bindValue(4, static_cast< uint >(tab->layoutMode()));
        query.bindValue(5, static_cast< uint >(tab->rightToLeftMode()));

        query.bindValue(6, static_cast< uint >(tab->scaleMode()));
        query.bindValue(7, tab->scaleFactor());

        query.bindValue(8, static_cast< uint >(tab->rotation()));

        query.exec();

        if(!query.isActive())
        {
            qDebug() << query.lastError();
            return;
        }

        transaction.commit();
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
        {
            QSqlQuery query(m_database);
            query.exec("PRAGMA synchronous = OFF");
            query.exec("PRAGMA journal_mode = MEMORY");
        }

        const QStringList tables = m_database.tables();

        // tabs

        if(!tables.contains("tabs_v3"))
        {
            if(prepareTabs_v3())
            {
                if(tables.contains("tabs_v2"))
                {
                    migrateTabs_v2_v3();
                }
                else if(tables.contains("tabs_v1"))
                {
                    migrateTabs_v1_v3();
                }
            }
        }

        // bookmarks

        if(!tables.contains("bookmarks_v2"))
        {
            if(prepareBookmarks_v2() && tables.contains("bookmarks_v1"))
            {
                migrateBookmarks_v1_v2();
            }
        }

        // per-file settings

        if(!tables.contains("perfilesettings_v2"))
        {
            if(preparePerFileSettings_v2() && tables.contains("perfilesettings_v1"))
            {
                migratePerFileSettings_v1_v2();
            }
        }
    }
    else
    {
        qDebug() << m_database.lastError();
    }

#endif // WITH_SQL
}

QString Database::instanceName()
{
    return !qApp->objectName().isNull() ? qApp->objectName() : QString("");
}

#ifdef WITH_SQL

bool Database::prepareTabs_v3()
{
    Transaction transaction(m_database);

    QSqlQuery query(m_database);
    query.exec("CREATE TABLE tabs_v3 "
               "(filePath TEXT"
               ",instanceName TEXT"
               ",currentPage INTEGER"
               ",continuousMode INTEGER"
               ",layoutMode INTEGER"
               ",rightToLeftMode INTEGER"
               ",scaleMode INTEGER"
               ",scaleFactor REAL"
               ",rotation INTEGER)");

    if(!query.isActive())
    {
        qDebug() << query.lastError();
        return false;
    }

    transaction.commit();
    return true;
}

bool Database::prepareBookmarks_v2()
{
    Transaction transaction(m_database);

    QSqlQuery query(m_database);
    query.exec("CREATE TABLE bookmarks_v2 "
               "(filePath TEXT"
               ",page INTEGER"
               ",label TEXT)");

    if(!query.isActive())
    {
        qDebug() << query.lastError();
        return false;
    }

    transaction.commit();
    return true;
}

bool Database::preparePerFileSettings_v2()
{
    Transaction transaction(m_database);

    QSqlQuery query(m_database);

    query.exec("CREATE TABLE perfilesettings_v2 "
               "(lastUsed INTEGER"
               ",filePath TEXT PRIMARY KEY"
               ",currentPage INTEGER"
               ",continuousMode INTEGER"
               ",layoutMode INTEGER"
               ",rightToLeftMode INTEGER"
               ",scaleMode INTEGER"
               ",scaleFactor REAL"
               ",rotation INTEGER)");

    if(!query.isActive())
    {
        qDebug() << query.lastError();
        return false;
    }

    if(Settings::instance()->mainWindow().restorePerFileSettings())
    {
        query.exec("DELETE FROM perfilesettings_v2 WHERE filePath IN (SELECT filePath FROM perfilesettings_v2 ORDER BY lastUsed DESC LIMIT -1 OFFSET 1000)");
    }
    else
    {
        query.exec("DELETE FROM perfilesettings_v2");
    }

    if(!query.isActive())
    {
        qDebug() << query.lastError();
        return false;
    }

    transaction.commit();
    return true;
}

void Database::migrateTabs_v2_v3()
{
    Transaction transaction(m_database);

    QSqlQuery query(m_database);
    query.prepare("INSERT INTO tabs_v3 "
                  "SELECT filePath,instanceName,currentPage,continuousMode,layoutMode,0,scaleMode,scaleFactor,rotation "
                  "FROM tabs_v2");

    query.exec();

    if(!query.isActive())
    {
        qDebug() << query.lastError();
        return;
    }

    qWarning() << "Migrated tabs from v2 to v3, dropping v2.";
    query.exec("DROP TABLE tabs_v2");

    transaction.commit();
}

void Database::migrateTabs_v1_v3()
{
    Transaction transaction(m_database);

    QSqlQuery query(m_database);
    query.prepare("INSERT INTO tabs_v3 "
                  "SELECT filePath,?,currentPage,continuousMode,layoutMode,0,scaleMode,scaleFactor,rotation "
                  "FROM tabs_v1");

    query.bindValue(0, instanceName());

    query.exec();

    if(!query.isActive())
    {
        qDebug() << query.lastError();
        return;
    }

    qWarning() << "Migrated tabs from v1 to v3, dropping v1.";
    query.exec("DROP TABLE tabs_v1");

    transaction.commit();
}

void Database::migrateBookmarks_v1_v2()
{
    Transaction transaction(m_database);

    QSqlQuery outerQuery(m_database);
    outerQuery.exec("SELECT filePath,pages FROM bookmarks_v1");

    while(outerQuery.next())
    {
        if(!outerQuery.isActive())
        {
            qDebug() << outerQuery.lastError();
            return;
        }

        QSqlQuery innerQuery(m_database);
        innerQuery.prepare("INSERT INTO bookmarks_v2 "
                           "(filePath,page,label)"
                           " VALUES (?,?,?)");

        innerQuery.bindValue(0, outerQuery.value(0));

        foreach(QString page, outerQuery.value(1).toString().split(",", QString::SkipEmptyParts))
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

    if(!outerQuery.isActive())
    {
        qDebug() << outerQuery.lastError();
        return;
    }

    qWarning() << "Migrated bookmarks from v1 to v2, dropping v1.";
    outerQuery.exec("DROP TABLE bookmarks_v1");

    transaction.commit();
}

void Database::migratePerFileSettings_v1_v2()
{
    Transaction transaction(m_database);

    QSqlQuery query(m_database);
    query.prepare("INSERT INTO perfilesettings_v2 "
                  "SELECT lastUsed,filePath,currentPage,continuousMode,layoutMode,0,scaleMode,scaleFactor,rotation "
                  "FROM perfilesettings_v1");

    query.exec();

    if(!query.isActive())
    {
        qDebug() << query.lastError();
        return;
    }

    qWarning() << "Migrated per-file settings from v1 to v2, dropping v1.";
    query.exec("DROP TABLE perfilesettings_v1");

    transaction.commit();
}

#endif // WITH_SQL

} // qpdfview
