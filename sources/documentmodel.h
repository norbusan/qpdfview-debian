/*

Copyright 2012 Adam Reichold

This file is part of qpdfview.

qpdfview is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

qpdfview is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with qpdfview.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef DOCUMENTMODEL_H
#define DOCUMENTMODEL_H

#include <QtCore>
#include <QtXml>
#include <QtGui>

#include <poppler-qt4.h>

class DocumentModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString filePath READ filePath NOTIFY filePathChanged)
    Q_PROPERTY(int pageCount READ pageCount NOTIFY pageCountChanged)

public:
    explicit DocumentModel(QObject *parent = 0);
    ~DocumentModel();

    const QString &filePath() const;

    static bool watchFilePath();
    static void setWatchFilePath(bool watchFilePath);

    // pages

    int pageCount() const;

    QSizeF pageSize(int index);

    // links

    struct Link
    {
        QRectF area;
        int pageNumber;
        qreal top;
        QString url;

        Link() : area(), pageNumber(-1), top(0.0), url() {}
    };

    QList<Link> links(int index);

    // results

    QList<QRectF> results(int index);
    QMap<int, QRectF> results();

    // text

    QString text(int index, QRectF area);

    // outline

    struct Outline
    {
        QString text;
        int pageNumber;
        qreal top;

        bool isOpen;

        Outline *sibling;
        Outline *child;

        Outline() : text(""), pageNumber(-1), top(0.0), isOpen(false), sibling(0), child(0) {}
        ~Outline()
        {
            if(sibling)
            {
                delete sibling;
            }

            if(child)
            {
                delete child;
            }
        }
    };

    Outline *outline();

    // thumbnails

    QImage thumbnail(int index);

    // render hints

    static bool antialiasing();
    static void setAntialiasing(bool antialiasing);

    static bool textAntialiasing();
    static void setTextAntialiasing(bool textAntialiasing);

    static bool textHinting();
    static void setTextHinting(bool textHinting);

    // page cache

    static uint maximumPageCacheSize();
    static void setMaximumPageCacheSize(uint maximumPageCacheSize);

    QImage pullPage(int index, qreal resolutionX, qreal resolutionY);
    void pushPage(int index, qreal resolutionX, qreal resolutionY);
    void dropPage(int index, qreal resolutionX, qreal resolutionY);

signals:
    void filePathChanged(QString, bool);
    void pageCountChanged(int);

    void resultsChanged(int);
    void resultsChanged();

    void searchProgressed(int);
    void searchCanceled();
    void searchFinished();

    void printProgressed(int);
    void printCanceled();
    void printFinished();

public slots:
    bool open(const QString &filePath);
    bool refresh();
    bool saveCopy(const QString &filePath);

    void startSearch(const QString &text, bool matchCase = true);
    void cancelSearch();

    void startPrint(QPrinter *printer, int fromPage, int toPage);
    void cancelPrint();

private:
    Poppler::Document *m_document;

    QString m_filePath;
    QFileSystemWatcher *m_filePathWatcher;

    int m_pageCount;

    struct PageCacheKey
    {
        QString filePath;
        int index;
        qreal resolutionX;
        qreal resolutionY;

        PageCacheKey() : filePath(""), index(-1), resolutionX(72.0), resolutionY(72.0) {}
        PageCacheKey(const PageCacheKey &key) : filePath(key.filePath), index(key.index), resolutionX(key.resolutionX), resolutionY(key.resolutionY) {}
        PageCacheKey(const QString &filePath, int index, qreal resolutionX, qreal resolutionY) : filePath(filePath), index(index), resolutionX(resolutionX), resolutionY(resolutionY) {}

        bool operator<(const PageCacheKey &key) const
        {
            if(filePath < key.filePath)
            {
                return true;
            }
            else if(filePath == key.filePath && index < key.index)
            {
                return true;
            }
            else if(filePath == key.filePath && index == key.index && resolutionX < key.resolutionX)
            {
                return true;
            }
            else if(filePath == key.filePath && index == key.index && resolutionX == key.resolutionX && resolutionY < key.resolutionY)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    };

    static QMap<PageCacheKey, QImage> s_pageCache;
    static QMutex s_pageCacheMutex;

    static QSettings s_settings;

    static bool s_watchFilePath;

    static bool s_antialiasing;
    static bool s_textAntialiasing;
    static bool s_textHinting;

    static uint s_pageCacheSize;
    static uint s_maximumPageCacheSize;

    QMap<int,QRectF> m_results;
    QMutex m_resultsMutex;

    QFuture<void> m_search;
    void search(const QString &text, bool matchCase);

    QFuture<void> m_print;
    void print(QPrinter *printer, int fromPage, int toPage);

};

#endif // DOCUMENTMODEL_H
