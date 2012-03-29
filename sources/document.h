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

#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QtCore>
#include <QtXml>
#include <QtGui>

namespace Poppler
{
class Document;
}

struct Link
{
    QRectF area;

    int page;
    qreal top;

    QString url;

    Link() : area(), page(1), top(0.0), url() {}
};

struct Outline
{
    QString text;

    int page;
    qreal top;

    Outline *sibling;
    Outline *child;

    Outline() : text(), page(1), top(0.0), sibling(0), child(0) {}
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

class Document : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString filePath READ filePath NOTIFY filePathChanged)
    Q_PROPERTY(int numberOfPages READ numberOfPages NOTIFY numberOfPagesChanged)

public:
    explicit Document(QObject *parent = 0);
    ~Document();

    // properties

    const QString &filePath() const;
    int numberOfPages() const;

    // settings

    static bool automaticRefresh();
    static void setAutomaticRefresh(bool automaticRefresh);

    static bool openUrl();
    static void setOpenUrl(bool openUrl);

    static bool antialiasing();
    static void setAntialiasing(bool antialiasing);

    static bool textAntialiasing();
    static void setTextAntialiasing(bool textAntialiasing);

    // pages

    QSizeF size(int index);

    QImage thumbnail(int index);

    QString text(int index, QRectF area);

    QList<Link> links(int index);

    // outline

    Outline *outline();

    // page cache

    QImage pullPage(int index, qreal scaleFactor);
    void pushPage(int index, qreal scaleFactor);
    void dropPage(int index, qreal scaleFactor);

    static uint maximumPageCacheSize();
    static void setMaximumPageCacheSize(uint maximumPageCacheSize);

    // search results

    QList<QRectF> searchResults(int index);

public slots:
    bool open(const QString &filePath);
    bool refresh();
    bool saveCopy(const QString &filePath);

    void startRender(int index, qreal scaleFactor);

    void startSearch(const QString &text, bool matchCase = true);
    void cancelSearch();

    void startPrint(QPrinter *printer, int fromPage, int toPage);
    void cancelPrint();

signals:
    void filePathChanged(QString filePath);
    void numberOfPagesChanged(int numberOfPages);

    void documentChanged();

    void pageRendered(int index);
    void pageSearched(int index);

    void searchProgressed(int value);
    void searchCanceled();
    void searchFinished();

    void printProgressed(int value);
    void printCanceled();
    void printFinished();

protected:

protected slots:

private:
    Poppler::Document *m_document;

    // properties

    QString m_filePath;
    int m_numberOfPages;

    QFileSystemWatcher *m_fileSystemWatcher;

    // settings

    static QSettings s_settings;

    static bool s_automaticRefresh;
    static bool s_openUrl;

    static bool s_antialiasing;
    static bool s_textAntialiasing;

    // page cache

    struct PageCacheKey
    {
        QString filePath;
        int index;
        qreal scaleFactor;

        PageCacheKey() : filePath(), index(-1), scaleFactor(1.0) {}
        PageCacheKey(const PageCacheKey &key) : filePath(key.filePath), index(key.index), scaleFactor(key.scaleFactor) {}
        PageCacheKey(const QString &filePath, int index, qreal scaleFactor) : filePath(filePath), index(index), scaleFactor(scaleFactor) {}

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
            else if(filePath == key.filePath && index == key.index && scaleFactor < key.scaleFactor)
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

    static uint s_pageCacheSize;
    static uint s_maximumPageCacheSize;

    // search results

    QMap<int, QRectF> m_searchResults;
    QMutex m_searchResultsMutex;

    // internal methods

    Outline *domNodeToOutline(const QDomNode &domNode);

    QFuture<void> m_render;
    void render(int index, qreal scaleFactor);

    QFuture<void> m_search;
    void search(const QString &text, bool matchCase);

    QFuture<void> m_print;
    void print(QPrinter *printer, int fromPage, int toPage);

};

#endif // DOCUMENT_H
