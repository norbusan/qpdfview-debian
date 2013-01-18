/*

Copyright 2012-2013 Adam Reichold

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

#include <QCache>
#include <QFutureWatcher>
#include <QGraphicsObject>

#include "global.h"

namespace Model
{
struct Link;
class Annotation;
class FormField;
class Page;
}

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

    static bool decorateFormFields();
    static void setDecorateFormFields(bool decorateFormFields);

    static const QColor& backgroundColor();
    static void setBackgroundColor(const QColor& backgroundColor);

    static const QColor& paperColor();
    static void setPaperColor(const QColor& paperColor);

    static bool invertColors();
    static void setInvertColors(bool invertColors);

    static const Qt::KeyboardModifiers& copyToClipboardModifiers();
    static void setCopyToClipboardModifiers(const Qt::KeyboardModifiers& copyToClipboardModifiers);

    static const Qt::KeyboardModifiers& addAnnotationModifiers();
    static void setAddAnnotationModifiers(const Qt::KeyboardModifiers& addAnnotationModifiers);

    PageItem(Model::Page* page, int index, QGraphicsItem* parent = 0);
    ~PageItem();

    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget* widget);

    int index() const;
    const QSizeF& size() const;

    bool presentationMode() const;
    void setPresentationMode(bool presentationMode);

    const QList< QRectF >& highlights() const;
    void setHighlights(const QList< QRectF >& highlights, int duration = 0);

    RubberBandMode rubberBandMode() const;
    void setRubberBandMode(RubberBandMode rubberBandMode);

    int physicalDpiX() const;
    int physicalDpiY() const;
    void setPhysicalDpi(int physicalDpiX, int physicalDpiY);

    qreal scaleFactor() const;
    void setScaleFactor(qreal scaleFactor);

    Rotation rotation() const;
    void setRotation(Rotation rotation);

    const QTransform& transform() const;
    const QTransform& normalizedTransform() const;

signals:
    void imageReady(int physicalDpiX, int physicalDpiY, qreal scaleFactor, Rotation rotation, bool prefetch, QImage image);

    void linkClicked(int page, qreal left = 0.0, qreal top = 0.0);
    void linkClicked(const QString& url);

    void rubberBandStarted();
    void rubberBandFinished();

    void sourceRequested(int page, const QPointF& pos);

public slots:
    void refresh();

    void clearHighlights();

    void startRender(bool prefetch = false);
    void cancelRender();

protected slots:
    void on_render_finished();
    void on_imageReady(int physicalDpiX, int physicalDpiY, qreal scaleFactor, Rotation rotation, bool prefetch, QImage image);

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent*);
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*);

    void mousePressEvent(QGraphicsSceneMouseEvent* event);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);

    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event);

private:
    static QCache< PageItem*, QPixmap > s_cache;

    static bool s_decoratePages;
    static bool s_decorateLinks;
    static bool s_decorateFormFields;

    static QColor s_backgroundColor;
    static QColor s_paperColor;

    static bool s_invertColors;

    static Qt::KeyboardModifiers s_copyToClipboardModifiers;
    static Qt::KeyboardModifiers s_addAnnotationModifiers;

    Model::Page* m_page;

    int m_index;
    QSizeF m_size;

    QList< Model::Link* > m_links;
    QList< Model::Annotation* > m_annotations;
    QList< Model::FormField* > m_formFields;

    bool m_presentationMode;

    QList< QRectF > m_highlights;

    RubberBandMode m_rubberBandMode;
    QRectF m_rubberBand;

    void copyToClipboard(const QPoint& screenPos);

    void addAnnotation(const QPoint& screenPos);
    void removeAnnotation(Model::Annotation* annotation, const QPoint& screenPos);
    void editAnnotation(Model::Annotation* annotation, const QPoint& screenPos);

    void editFormField(Model::FormField* formField, const QPoint& screenPos);

    // geometry

    int m_physicalDpiX;
    int m_physicalDpiY;

    qreal m_scaleFactor;
    Rotation m_rotation;

    QTransform m_transform;
    QTransform m_normalizedTransform;
    QRectF m_boundingRect;

    QPixmap m_pixmap;

    void prepareGeometry();

    // render

    QFutureWatcher< void >* m_render;
    void render(int physicalDpiX, int physicalDpiY, qreal scaleFactor, Rotation rotation, bool prefetch);

};

class ThumbnailItem : public PageItem
{
    Q_OBJECT

public:
    ThumbnailItem(Model::Page* page, int index, QGraphicsItem* parent = 0);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);

};

#endif // PAGEITEM_H
