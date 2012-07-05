/*

Copyright 2012 Adam Reichold

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

#ifndef PAGEITEM_H
#define PAGEITEM_H

#include <QtCore>
#include <QtGui>

#include <poppler-qt4.h>

class PageItem : public QGraphicsObject
{
    Q_OBJECT

public:
    static int cacheSize();
    static void setCacheSize(int cacheSize);

    static bool decoratePages();
    static void setDecoratePages(bool decoratePages);

    static bool decorateLinks();
    static void setDecorateLinks(bool decorateLinks);

    explicit PageItem(QMutex* mutex, Poppler::Document* document, int index, QGraphicsItem* parent = 0);
    ~PageItem();

    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*);

    int index() const;
    QSizeF size() const;

    const QList< QRectF >& highlights() const;
    void setHighlights(const QList< QRectF >& highlights);

    int physicalDpiX() const;
    int physicalDpiY() const;
    void setPhysicalDpi(int physicalDpiX, int physicalDpiY);

    qreal scaleFactor() const;
    void setScaleFactor(qreal scaleFactor);

    Poppler::Page::Rotation rotation() const;
    void setRotation(Poppler::Page::Rotation rotation);

    const QTransform& transform() const;
    const QTransform& normalizedTransform() const;

    bool isPrefetching() const;

signals:
    void linkClicked(int page, qreal left, qreal top);
    void linkClicked(const QString& url);

public slots:
    void startRender();
    void cancelRender();

    void prefetch();

protected slots:
    void on_render_finished();

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event);
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event);
    void mousePressEvent(QGraphicsSceneMouseEvent* event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);

private:
    static QCache< PageItem*, QImage > s_cache;

    static bool s_decoratePages;
    static bool s_decorateLinks;

    QMutex* m_mutex;
    Poppler::Page* m_page;

    int m_index;
    QSizeF m_size;
    QList< Poppler::Link* > m_links;
    QList< Poppler::Annotation* > m_annotations;

    QList< QRectF > m_highlights;
    QRectF m_rubberBand;

    void copyTextOrImage();

    // geometry

    int m_physicalDpiX;
    int m_physicalDpiY;

    qreal m_scaleFactor;
    Poppler::Page::Rotation m_rotation;

    QTransform m_transform;
    QTransform m_normalizedTransform;
    QRectF m_boundingRect;

    void prepareGeometry();

    // render

    QImage m_image1;
    QImage m_image2;

    bool m_isPrefetching;

    QFutureWatcher< void >* m_render;
    void render(int physicalDpiX, int physicalDpiY, qreal scaleFactor, Poppler::Page::Rotation rotation);

};

class ThumbnailItem : public PageItem
{
    Q_OBJECT

public:
    explicit ThumbnailItem(QMutex* mutex, Poppler::Document* document, int index, QGraphicsItem* parent = 0);

signals:
    void pageClicked(int page);

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event);
    void mousePressEvent(QGraphicsSceneMouseEvent* event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);

};

#endif // PAGEITEM_H
