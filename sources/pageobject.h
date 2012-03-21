#ifndef PAGEOBJECT_H
#define PAGEOBJECT_H

#include <QtCore>
#include <QtGui>

#include <poppler-qt4.h>

class PageObject;

#include "documentview.h"

class PageObject : public QGraphicsObject
{
    Q_OBJECT
    Q_PROPERTY(int index READ index WRITE setIndex NOTIFY indexChanged)

public:
    explicit PageObject(Poppler::Page *page, int index, DocumentView *view, QGraphicsItem *parent = 0);
    ~PageObject();

    int index() const;
    void setIndex(const int &index);


    QRectF highlightedArea() const;
    QString highlightedText() const;


    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);


    static bool pageCacheThreading();
    static void setPageCacheThreading(bool threading);

    static uint pageCacheSize();
    static void setPageCacheSize(uint size);

private:
    Poppler::Page *m_page;
    int m_index;

    DocumentView *m_view;

    QMatrix m_linkTransform;
    QMatrix m_highlightTransform;

    struct Link
    {
        QRectF area;
        int pageNumber;

        Link(const QRectF &_area, const int &_pageNumber) : area(_area), pageNumber(_pageNumber) {}
    };
    QList<Link> m_links;

    struct ExternalLink
    {
        QRectF area;
        QString fileName;
        int pageNumber;

        ExternalLink(const QRectF &_area, const QString &_fileName, const int &_pageNumber) : area(_area), fileName(_fileName), pageNumber(_pageNumber) {}
    };
    QList<ExternalLink> m_externalLinks;

    QRectF m_highlight;
    QRectF m_rubberBand;


    QFutureWatcher<void> *m_renderWatcher;

    void renderPage();
    void updateScene();
    static void updatePageCache(QPair<QString, int> key, QImage image);

    static bool s_concurrentPageCache;

    static QMutex s_pageCacheMutex;
    static QMap<QPair<QString, int>, QImage> s_pageCache;

    static uint s_pageCacheByteCount;
    static uint s_maximumPageCacheByteCount;

signals:
    void indexChanged(int);

    void linkClicked(int pageNumber);
    void linkClicked(QString fileName, int pageNumber);

protected:
    QString filePath() const;
    qreal resolutionX() const;
    qreal resolutionY() const;
    Poppler::Page::Rotation rotation() const;

    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

};

#endif // PAGEOBJECT_H
