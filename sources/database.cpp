/*

Copyright 2014 S. Razi Alavizadeh
Copyright 2012-2018 Adam Reichold
Copyright 2012 Michał Trybus
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
#include "bookmarkmodel.h"

#ifdef WITH_SQL

namespace
{

using namespace qpdfview;

class Transaction
{
public:
    Transaction(QSqlDatabase& database) :
        m_database(database),
        m_committed(false)
    {
        if(s_current != 0)
        {
            return;
        }

        if(!m_database.transaction())
        {
            throw m_database.lastError();
        }

        s_current = this;
    }

    ~Transaction() throw()
    {
        if(s_current != this)
        {
            return;
        }

        if(!m_committed)
        {
            m_database.rollback();
        }

        s_current = 0;
    }

    void commit()
    {
        if(s_current != this)
        {
            return;
        }

        if(!m_database.commit())
        {
            throw m_database.lastError();
        }

        m_committed = true;
    }

private:
    Q_DISABLE_COPY(Transaction)

    QSqlDatabase& m_database;
    bool m_committed;

    static Transaction* s_current;

};

Transaction* Transaction::s_current = 0;

class Query
{
private:
    template< typename T >
    class BindValue
    {
    private:
        friend class Query;

        explicit BindValue(const T& value) : m_value(value) {}

    public:
        inline operator QVariant() const
        {
            return Conversion< T >::convert(m_value);
        }

    private:
        const T& m_value;

        template< typename S, bool Defined = QMetaTypeId2< S >::Defined >
        struct Conversion
        {
            static inline QVariant convert(const S& value)
            {
                return QVariant(value);
            }
        };

        template< typename S >
        struct Conversion< S, false >
        {
            static inline QVariant convert(const S& value)
            {
                return QVariant(static_cast< uint >(value));
            }
        };

    };

public:
    class Value
    {
    private:
        friend class Query;

        explicit Value(const QVariant& value) : m_value(value) {}

    public:
        template< typename T >
        inline operator T() const
        {
            return Conversion< T >::convert(m_value);
        }

    private:
        const QVariant m_value;

        template< typename T, bool Defined = QMetaTypeId2< T >::Defined >
        struct Conversion
        {
            static inline T convert(const QVariant& value)
            {
                return value.value< T >();
            }
        };

        template< typename T >
        struct Conversion< T, false >
        {
            static inline T convert(const QVariant& value)
            {
                return static_cast< T >(value.value< uint >());
            }
        };

    };

public:
    Query(QSqlDatabase& database) :
        m_query(database),
        m_bindValueIndex(0),
        m_valueIndex(0)
    {
    }

    void prepare(const QString& query)
    {
        if(!m_query.prepare(query))
        {
            throw m_query.lastError();
        }

        m_bindValueIndex = 0;
    }

    void exec()
    {
        if(!m_query.exec())
        {
            throw m_query.lastError();
        }

        m_bindValueIndex = 0;
    }

    void exec(const QString& query)
    {
        if(!m_query.exec(query))
        {
            throw m_query.lastError();
        }

        m_bindValueIndex = 0;
    }

    Query& operator <<(const QVariant& value)
    {
        m_query.bindValue(m_bindValueIndex++, value);

        return *this;
    }

    template< typename T >
    Query& operator <<(const T& value)
    {
        m_query.bindValue(m_bindValueIndex++, BindValue< T >(value));

        return *this;
    }

    bool nextRecord()
    {
        if(!m_query.isActive())
        {
            throw m_query.lastError();
        }

        m_valueIndex = 0;

        return m_query.next();
    }

    Value nextValue()
    {
        return Value(m_query.value(m_valueIndex++));
    }

private:
    Q_DISABLE_COPY(Query)

    QSqlQuery m_query;
    int m_bindValueIndex;
    int m_valueIndex;

};

inline QByteArray hashFilePath(const QString& filePath)
{
    return QCryptographicHash::hash(filePath.toUtf8(), QCryptographicHash::Sha1).toBase64();
}

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

QStringList Database::knownInstanceNames()
{
    QStringList instanceNames;

#ifdef WITH_SQL

    try
    {
        Transaction transaction(m_database);
        Query query(m_database);

        query.exec("SELECT DISTINCT(instanceName) FROM tabs_v5");

        while(query.nextRecord())
        {
            const QString instanceName = query.nextValue();

            if(!instanceName.isEmpty())
            {
                instanceNames.append(instanceName);
            }
        }

        transaction.commit();
    }
    catch(QSqlError& error)
    {
        qDebug() << error;
    }

#endif // WITH_SQL

    return instanceNames;
}

void Database::restoreTabs(const RestoreTab& restoreTab)
{
#ifdef WITH_SQL

    try
    {
        Transaction transaction(m_database);
        Query query(m_database);

        query.prepare("SELECT filePath,currentPage,continuousMode,layoutMode,rightToLeftMode,scaleMode,scaleFactor,rotation,renderFlags,firstPage"
                      " FROM tabs_v5 WHERE instanceName==? ORDER BY tabIndex");

        query << instanceName();

        query.exec();

        while(query.nextRecord())
        {
            if(DocumentView* newTab = restoreTab(query.nextValue()))
            {
                const int page = query.nextValue();

                newTab->setContinuousMode(query.nextValue());
                newTab->setLayoutMode(query.nextValue());
                newTab->setRightToLeftMode(query.nextValue());

                newTab->setScaleMode(query.nextValue());
                newTab->setScaleFactor(query.nextValue());

                newTab->setRotation(query.nextValue());
                newTab->setRenderFlags(query.nextValue());

                newTab->setFirstPage(query.nextValue());

                newTab->jumpToPage(page, false);
            }
        }

        transaction.commit();
    }
    catch(QSqlError& error)
    {
        qDebug() << error;
    }

#else

    Q_UNUSED(restoreTab);

#endif // WITH_SQL
}

void Database::saveTabs(const QVector< DocumentView* >& tabs)
{
#ifdef WITH_SQL

    try
    {
        Transaction transaction(m_database);
        Query query(m_database);

        query.prepare("DELETE FROM tabs_v5 WHERE instanceName==?");

        query << instanceName();

        query.exec();

        query.prepare("INSERT INTO tabs_v5"
                      " (filePath,instanceName,tabIndex,currentPage,continuousMode,layoutMode,rightToLeftMode,scaleMode,scaleFactor,rotation,renderFlags,firstPage)"
                      " VALUES (?,?,?,?,?,?,?,?,?,?,?,?)");

        for(int tabIndex = 0; tabIndex < tabs.size(); ++tabIndex)
        {
            const DocumentView* const tab = tabs.at(tabIndex);

            query << tab->fileInfo().absoluteFilePath()
                  << instanceName()
                  << tabIndex
                  << tab->currentPage()

                  << tab->continuousMode()
                  << tab->layoutMode()
                  << tab->rightToLeftMode()

                  << tab->scaleMode()
                  << tab->scaleFactor()

                  << tab->rotation()
                  << tab->renderFlags()

                  << tab->firstPage();

            query.exec();
        }

        transaction.commit();
    }
    catch(QSqlError& error)
    {
        qDebug() << error;
    }

#else

    Q_UNUSED(tabs);

#endif // WITH_SQL
}

void Database::clearTabs()
{
#ifdef WITH_SQL

    try
    {
        Transaction transaction(m_database);
        Query query(m_database);

        query.exec("DELETE FROM tabs_v5");

        transaction.commit();
    }
    catch(QSqlError& error)
    {
        qDebug() << error;
    }

#endif // WITH_SQL
}

void Database::restoreBookmarks()
{
#ifdef WITH_SQL

    try
    {
        Transaction transaction(m_database);
        Query outerQuery(m_database);
        Query innerQuery(m_database);

        outerQuery.exec("SELECT DISTINCT(filePath) FROM bookmarks_v3");

        innerQuery.prepare("SELECT page,label,comment,modified"
                           " FROM bookmarks_v3 WHERE filePath==?");

        while(outerQuery.nextRecord())
        {
            const QString filePath = outerQuery.nextValue();

            innerQuery << filePath;

            innerQuery.exec();

            BookmarkModel* model = BookmarkModel::fromPath(filePath, true);

            while(innerQuery.nextRecord())
            {
                const int page = innerQuery.nextValue();
                const QString label = innerQuery.nextValue();
                const QString comment = innerQuery.nextValue();
                const QDateTime modified = innerQuery.nextValue();

                model->addBookmark(BookmarkItem(page, label, comment, modified));
            }
        }

        transaction.commit();
    }
    catch(QSqlError& error)
    {
        qDebug() << error;
    }

#endif // WITH_SQL
}

void Database::saveBookmarks()
{
#ifdef WITH_SQL

    try
    {
        Transaction transaction(m_database);
        Query query(m_database);

        query.exec("DELETE FROM bookmarks_v3");

        if(Settings::instance()->mainWindow().restoreBookmarks())
        {
            query.prepare("INSERT INTO bookmarks_v3"
                          " (filePath,page,label,comment,modified)"
                          " VALUES (?,?,?,?,?)");

            foreach(const QString& filePath, BookmarkModel::paths())
            {
                const BookmarkModel* model = BookmarkModel::fromPath(filePath);

                for(int row = 0, rowCount = model->rowCount(); row < rowCount; ++row)
                {
                    const QModelIndex index = model->index(row);

                    query << filePath

                          << index.data(BookmarkModel::PageRole)
                          << index.data(BookmarkModel::LabelRole)
                          << index.data(BookmarkModel::CommentRole)
                          << index.data(BookmarkModel::ModifiedRole);

                    query.exec();
                }
            }
        }

        transaction.commit();
    }
    catch(QSqlError& error)
    {
        qDebug() << error;
    }

#endif // WITH_SQL
}

void Database::clearBookmarks()
{
#ifdef WITH_SQL

    try
    {
        Transaction transaction(m_database);
        Query query(m_database);

        query.exec("DELETE FROM bookmarks_v3");

        transaction.commit();
    }
    catch(QSqlError& error)
    {
        qDebug() << error;
    }

#endif // WITH_SQL
}

void Database::restorePerFileSettings(DocumentView* tab)
{
#ifdef WITH_SQL

    if(!Settings::instance()->mainWindow().restorePerFileSettings())
    {
        return;
    }

    try
    {
        const QByteArray filePath = hashFilePath(tab->fileInfo().absoluteFilePath());

        Transaction transaction(m_database);
        Query query(m_database);

        query.prepare("SELECT currentPage,continuousMode,layoutMode,rightToLeftMode,scaleMode,scaleFactor,rotation,renderFlags,firstPage"
                      " FROM perfilesettings_v4 WHERE filePath==?");

        query << filePath;

        query.exec();

        if(query.nextRecord())
        {
            const int page = query.nextValue();

            tab->setContinuousMode(query.nextValue());
            tab->setLayoutMode(query.nextValue());
            tab->setRightToLeftMode(query.nextValue());

            tab->setScaleMode(query.nextValue());
            tab->setScaleFactor(query.nextValue());

            tab->setRotation(query.nextValue());

            tab->setRenderFlags(query.nextValue());

            tab->setFirstPage(query.nextValue());

            tab->jumpToPage(page, false);
        }

        query.prepare("SELECT expandedPath FROM perfilesettings_outline_v1 WHERE filePath==?");

        query << filePath;

        query.exec();

        QSet< QByteArray > expandedPaths;

        while(query.nextRecord())
        {
            expandedPaths.insert(query.nextValue());
        }

        tab->restoreExpandedPaths(expandedPaths);

        transaction.commit();
    }
    catch(QSqlError& error)
    {
        qDebug() << error;
    }

#else

    Q_UNUSED(tab);

#endif // WITH_SQL
}

void Database::savePerFileSettings(const DocumentView* tab)
{
#ifdef WITH_SQL

    if(!Settings::instance()->mainWindow().restorePerFileSettings())
    {
        return;
    }

    try
    {
        const QByteArray filePath = hashFilePath(tab->fileInfo().absoluteFilePath());

        Transaction transaction(m_database);
        Query query(m_database);

        query.prepare("INSERT OR REPLACE INTO perfilesettings_v4"
                      " (lastUsed,filePath,currentPage,continuousMode,layoutMode,rightToLeftMode,scaleMode,scaleFactor,rotation,renderFlags,firstPage)"
                      " VALUES (strftime('%s','now'),?,?,?,?,?,?,?,?,?,?)");

        query << filePath
              << tab->currentPage()

              << tab->continuousMode()
              << tab->layoutMode()
              << tab->rightToLeftMode()

              << tab->scaleMode()
              << tab->scaleFactor()

              << tab->rotation()
              << tab->renderFlags()

              << tab->firstPage();

        query.exec();

        query.prepare("DELETE FROM perfilesettings_outline_v1 WHERE filePath==?");

        query << filePath;

        query.exec();

        query.prepare("INSERT INTO perfilesettings_outline_v1"
                      " (filePath,expandedPath)"
                      " VALUES (?,?)");

        foreach(const QByteArray& expandedPath, tab->saveExpandedPaths())
        {
            query << filePath << expandedPath;

            query.exec();
        }

        transaction.commit();
    }
    catch(QSqlError& error)
    {
        qDebug() << error;
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

    if(!m_database.open())
    {
        qDebug() << m_database.lastError();
        return;
    }

    try
    {
        Query query(m_database);

        query.exec("PRAGMA synchronous = OFF");
        query.exec("PRAGMA journal_mode = MEMORY");
    }
    catch(QSqlError& error)
    {
        qDebug() << error;
    }

    const QStringList tables = m_database.tables();

    // tabs

    if(!tables.contains("tabs_v5"))
    {
        if(prepareTabs_v5())
        {
            if(tables.contains("tabs_v4"))
            {
                migrateTabs_v4_v5();
            }
            else if(tables.contains("tabs_v3"))
            {
                migrateTabs_v3_v5();
            }
            else if(tables.contains("tabs_v2"))
            {
                migrateTabs_v2_v5();
            }
            else if(tables.contains("tabs_v1"))
            {
                migrateTabs_v1_v5();
            }
        }
    }

    // bookmarks

    if(!tables.contains("bookmarks_v3"))
    {
        if(prepareBookmarks_v3())
        {
            if(tables.contains("bookmarks_v2"))
            {
                migrateBookmarks_v2_v3();
            }
            else if(tables.contains("bookmarks_v1"))
            {
                migrateBookmarks_v1_v3();
            }
        }
    }

    // per-file settings

    if(!tables.contains("perfilesettings_v4"))
    {
        if(preparePerFileSettings_v4())
        {
            if(tables.contains("perfilesettings_v3"))
            {
                migratePerFileSettings_v3_v4();
            }
            else if(tables.contains("perfilesettings_v2"))
            {
                migratePerFileSettings_v2_v4();
            }
            else if(tables.contains("perfilesettings_v1"))
            {
                migratePerFileSettings_v1_v4();
            }
        }
    }

    if(!tables.contains("perfilesettings_outline_v1"))
    {
        preparePerFileSettings_Outline_v1();
    }

    limitPerFileSettings();

#endif // WITH_SQL
}

QString Database::instanceName()
{
    QString instanceName = qApp->objectName();

    if(instanceName.isNull())
    {
        instanceName = QLatin1String("");
    }

    return instanceName;
}

#ifdef WITH_SQL

bool Database::prepareTabs_v5()
{
    return prepareTable("CREATE TABLE tabs_v5 ("
                        " filePath TEXT"
                        " ,instanceName TEXT"
                        " ,tabIndex INTEGER"
                        " ,currentPage INTEGER"
                        " ,continuousMode INTEGER"
                        " ,layoutMode INTEGER"
                        " ,rightToLeftMode INTEGER"
                        " ,scaleMode INTEGER"
                        " ,scaleFactor REAL"
                        " ,rotation INTEGER"
                        " ,renderFlags INTEGER"
                        " ,firstPage INTEGER"
                        " ,PRIMARY KEY (instanceName, tabIndex)"
                        " )");
}

bool Database::prepareBookmarks_v3()
{
    return prepareTable("CREATE TABLE bookmarks_v3 ("
                        " filePath TEXT"
                        " ,page INTEGER"
                        " ,label TEXT"
                        " ,comment TEXT"
                        " ,modified DATETIME"
                        " )");
}

bool Database::preparePerFileSettings_v4()
{
    return prepareTable("CREATE TABLE perfilesettings_v4 ("
                        " lastUsed INTEGER"
                        " ,filePath TEXT PRIMARY KEY"
                        " ,currentPage INTEGER"
                        " ,continuousMode INTEGER"
                        " ,layoutMode INTEGER"
                        " ,rightToLeftMode INTEGER"
                        " ,scaleMode INTEGER"
                        " ,scaleFactor REAL"
                        " ,rotation INTEGER"
                        " ,renderFlags INTEGER"
                        " ,firstPage INTEGER"
                        " )");
}

bool Database::preparePerFileSettings_Outline_v1()
{
    return prepareTable("CREATE TABLE perfilesettings_outline_v1 ("
                        " filePath TEXT"
                        " ,expandedPath TEXT"
                        " ,FOREIGN KEY (filePath) REFERENCES perfilesettings_v4 (filePath) ON DELETE CASCADE"
                        " )");
}

void Database::migrateTabs_v4_v5()
{
    migrateTable("INSERT INTO tabs_v5"
                 " SELECT filePath,instanceName,(SELECT MAX(tabIndex)+1 FROM tabs_v5 WHERE tabs_v5.instanceName=tabs_v4.instanceName),currentPage,continuousMode,layoutMode,rightToLeftMode,scaleMode,scaleFactor,rotation,0,-1"
                 " FROM tabs_v4",

                 "DROP TABLE tabs_v4",

                 "Migrated tabs from v4 to v5, dropping v4.");
}

void Database::migrateTabs_v3_v5()
{
    migrateTable("INSERT INTO tabs_v5"
                 " SELECT filePath,instanceName,(SELECT MAX(tabIndex)+1 FROM tabs_v5 WHERE tabs_v5.instanceName=tabs_v3.instanceName),currentPage,continuousMode,layoutMode,rightToLeftMode,scaleMode,scaleFactor,rotation,0,-1"
                 " FROM tabs_v3",

                 "DROP TABLE tabs_v3",

                 "Migrated tabs from v3 to v5, dropping v3.");
}

void Database::migrateTabs_v2_v5()
{
    migrateTable("INSERT INTO tabs_v5"
                 " SELECT filePath,instanceName,(SELECT MAX(tabIndex)+1 FROM tabs_v5 WHERE tabs_v5.instanceName=tabs_v2.instanceName),currentPage,continuousMode,layoutMode,0,scaleMode,scaleFactor,rotation,0,-1"
                 " FROM tabs_v2",

                 "DROP TABLE tabs_v2"  ,

                 "Migrated tabs from v2 to v5, dropping v2.");
}

void Database::migrateTabs_v1_v5()
{
    migrateTable("INSERT INTO tabs_v5"
                 " SELECT filePath,'',(SELECT MAX(tabIndex)+1 FROM tabs_v5 WHERE tabs_v5.instanceName=''),currentPage,continuousMode,layoutMode,0,scaleMode,scaleFactor,rotation,0,-1"
                 " FROM tabs_v1",

                 "DROP TABLE tabs_v1",

                 "Migrated tabs from v1 to v5, dropping v1.");
}

void Database::migrateBookmarks_v2_v3()
{
    migrateTable("INSERT INTO bookmarks_v3"
                 " SELECT filePath,page,label,'',datetime('now')"
                 " FROM bookmarks_v2",

                 "DROP TABLE bookmarks_v2",

                 "Migrated bookmarks from v2 to v3, dropping v2.");
}

void Database::migrateBookmarks_v1_v3()
{
    try
    {
        Transaction transaction(m_database);
        Query outerQuery(m_database);
        Query innerQuery(m_database);

        outerQuery.exec("SELECT filePath,pages FROM bookmarks_v1");

        innerQuery.prepare("INSERT INTO bookmarks_v3"
                           " (filePath,page,label,comment,modified)"
                           " VALUES (?,?,?,'',datetime('now'))");

        while(outerQuery.nextRecord())
        {
            const QString filePath = outerQuery.nextValue();
            const QString pages = outerQuery.nextValue();

            foreach(const QString& page, pages.split(",", QString::SkipEmptyParts))
            {
                innerQuery << filePath
                           << page
                           << tr("Jump to page %1").arg(page);

                innerQuery.exec();
            }
        }

        qWarning() << "Migrated bookmarks from v1 to v3, dropping v1.";

        outerQuery.exec("DROP TABLE bookmarks_v1");

        transaction.commit();
    }
    catch(QSqlError& error)
    {
        qDebug() << error;
    }
}

void Database::migratePerFileSettings_v3_v4()
{
    migrateTable("INSERT INTO perfilesettings_v4"
                 " SELECT lastUsed,filePath,currentPage,continuousMode,layoutMode,rightToLeftMode,scaleMode,scaleFactor,rotation,0,firstPage"
                 " FROM perfilesettings_v3",

                 "DROP TABLE perfilesettings_v3",

                 "Migrated per-file settings from v3 to v4, dropping v3.");
}

void Database::migratePerFileSettings_v2_v4()
{
    migrateTable("INSERT INTO perfilesettings_v4"
                 " SELECT lastUsed,filePath,currentPage,continuousMode,layoutMode,rightToLeftMode,scaleMode,scaleFactor,rotation,0,-1"
                 " FROM perfilesettings_v2",

                 "DROP TABLE perfilesettings_v2",

                 "Migrated per-file settings from v2 to v4, dropping v2.");
}

void Database::migratePerFileSettings_v1_v4()
{
    migrateTable("INSERT INTO perfilesettings_v4"
                 " SELECT lastUsed,filePath,currentPage,continuousMode,layoutMode,0,scaleMode,scaleFactor,rotation,0,-1"
                 " FROM perfilesettings_v1",

                 "DROP TABLE perfilesettings_v1",

                 "Migrated per-file settings from v1 to v4, dropping v1.");
}

bool Database::prepareTable(const QString& prepare)
{
    try
    {
        Transaction transaction(m_database);
        Query query(m_database);

        query.exec(prepare);

        transaction.commit();
        return true;
    }
    catch(QSqlError& error)
    {
        qDebug() << error;
        return false;
    }
}

void Database::migrateTable(const QString& migrate, const QString& prune, const QString& warning)
{
    try
    {
        Transaction transaction(m_database);
        Query query(m_database);

        query.exec(migrate);

        qWarning() << warning;

        query.exec(prune);

        transaction.commit();
    }
    catch(QSqlError& error)
    {
        qDebug() << error;
    }
}

void Database::limitPerFileSettings()
{
    try
    {
        Transaction transaction(m_database);
        Query query(m_database);

        if(Settings::instance()->mainWindow().restorePerFileSettings())
        {
            query.prepare("DELETE FROM perfilesettings_v4"
                          " WHERE filePath NOT IN ("
                          "  SELECT filePath FROM perfilesettings_v4"
                          "  ORDER BY lastUsed DESC LIMIT ?"
                          " )");

            query << Settings::instance()->mainWindow().perFileSettingsLimit();

            query.exec();
        }
        else
        {
            query.exec("DELETE FROM perfilesettings_v4");
        }

        transaction.commit();
    }
    catch(QSqlError& error)
    {
        qDebug() << error;
    }
}

#endif // WITH_SQL

} // qpdfview
