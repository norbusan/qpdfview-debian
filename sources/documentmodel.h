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
    int pageCount() const;

    struct Outline
    {
        QString text;
        bool isOpen;
        QString destinationName;

        Outline *sibling;
        Outline *child;

        Outline() : text(""), isOpen(false), destinationName(""), sibling(0), child(0) {}
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

    int destination(const QString &destinationName);

    QSizeF pageSize(int index);

    QImage thumbnail(int index);

    struct Link
    {
        QRectF area;
        int index;
    };

    QList<Link> links(int index);

    QString text(int index, QRectF area);

    QList<QRectF> results(int index);
    QMap<int, QRectF> results();

    static uint pageCacheSize()
    {
        return s_maximumPageCacheSize;
    }
    static void setPageCacheSize(uint pageCacheSize)
    {
        s_maximumPageCacheSize = pageCacheSize;
    }

    QImage pullPage(int index, qreal resolutionX, qreal resolutionY);
    void pushPage(int index, qreal resolutionX, qreal resolutionY);
    void dropPage(int index, qreal resolutionX, qreal resolutionY);

signals:
    void filePathChanged(QString);
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
