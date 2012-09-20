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

#include "annotationdialog.h"

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

    static bool invertColors();
    static void setInvertColors(bool invertColors);

    static const Qt::KeyboardModifiers& copyModifiers();
    static void setCopyModifiers(const Qt::KeyboardModifiers& copyModifiers);

    static const Qt::KeyboardModifiers& annotateModifiers();
    static void setAnnotateModifiers(const Qt::KeyboardModifiers& annotateModifiers);

    PageItem(QMutex* mutex, Poppler::Page* page, int index, QGraphicsItem* parent = 0);
    ~PageItem();

    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*);

    int index() const;
    QSizeF size() const;

    const QList< QRectF >& highlights() const;
    void setHighlights(const QList< QRectF >& highlights, int duration = 0);

    enum RubberBandMode
    {
        ModifiersMode = 0,
        CopyToClipboardMode = 1,
        AddAnnotationMode = 2
    };

    RubberBandMode rubberBandMode() const;
    void setRubberBandMode(RubberBandMode rubberBandMode);

    int physicalDpiX() const;
    int physicalDpiY() const;
    void setPhysicalDpi(int physicalDpiX, int physicalDpiY);

    qreal scaleFactor() const;
    void setScaleFactor(qreal scaleFactor);

    Poppler::Page::Rotation rotation() const;
    void setRotation(Poppler::Page::Rotation rotation);

    const QTransform& transform() const;
    const QTransform& normalizedTransform() const;

signals:
    void imageReady(int physicalDpiX, int physicalDpiY, qreal scaleFactor, Poppler::Page::Rotation rotation, bool prefetch, QImage image);

    void linkClicked(int page, qreal left, qreal top);
    void linkClicked(const QString& url);

    void rubberBandStarted();
    void rubberBandFinished();

public slots:
    void refresh();

    void clearHighlights();

    void startRender(bool prefetch = false);
    void cancelRender();

protected slots:
    void on_render_finished();
    void on_imageReady(int physicalDpiX, int physicalDpiY, qreal scaleFactor, Poppler::Page::Rotation rotation, bool prefetch, QImage image);

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event);
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event);
    void mousePressEvent(QGraphicsSceneMouseEvent* event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event);

private:
    static QCache< PageItem*, QImage > s_cache;

    static bool s_decoratePages;
    static bool s_decorateLinks;

    static bool s_invertColors;

    static Qt::KeyboardModifiers s_copyModifiers;
    static Qt::KeyboardModifiers s_annotateModifiers;

    QMutex* m_mutex;
    Poppler::Page* m_page;

    int m_index;
    QSizeF m_size;
    QList< Poppler::Link* > m_links;
    QList< Poppler::Annotation* > m_annotations;

    QList< QRectF > m_highlights;

    RubberBandMode m_rubberBandMode;
    QRectF m_rubberBand;

    void copyToClipboard(const QPoint& screenPos);
    void addAnnotation(const QPoint& screenPos);
    void removeAnnotation(Poppler::Annotation* annotation, const QPoint& screenPos);
    void editAnnotation(Poppler::Annotation* annotation, const QPoint& screenPos);

    // geometry

    int m_physicalDpiX;
    int m_physicalDpiY;

    qreal m_scaleFactor;
    Poppler::Page::Rotation m_rotation;

    QTransform m_transform;
    QTransform m_normalizedTransform;
    QRectF m_boundingRect;

    QImage m_image;

    void prepareGeometry();

    // render

    QFutureWatcher< void >* m_render;
    void render(int physicalDpiX, int physicalDpiY, qreal scaleFactor, Poppler::Page::Rotation rotation, bool prefetch);

};

class ThumbnailItem : public PageItem
{
    Q_OBJECT

public:
    ThumbnailItem(QMutex* mutex, Poppler::Page* page, int index, QGraphicsItem* parent = 0);

signals:
    void pageClicked(int page);

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event);
    void mousePressEvent(QGraphicsSceneMouseEvent* event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);

};

#endif // PAGEITEM_H
