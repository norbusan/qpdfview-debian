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
#include <QGraphicsObject>
#include <QIcon>
#include <QStaticText>

#include "global.h"

namespace Model
{
struct Link;
class Annotation;
class FormField;
class Page;
}

class Settings;
class RenderTask;

class PageItem : public QGraphicsObject
{
    Q_OBJECT

public:
    PageItem(Model::Page* page, int index, bool presentationMode = false, QGraphicsItem* parent = 0);
    ~PageItem();

    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*);

    int index() const;
    const QSizeF& size() const;

    bool invertColors();
    void setInvertColors(bool invertColors);

    const QList< QRectF >& highlights() const;
    void setHighlights(const QList< QRectF >& highlights);

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
    void linkClicked(int page, qreal left = 0.0, qreal top = 0.0);
    void linkClicked(const QString& url);
    void linkClicked(const QString& fileName, int page);

    void rubberBandStarted();
    void rubberBandFinished();

    void sourceRequested(int page, const QPointF& pos);

public slots:
    void refresh();

    void startRender(bool prefetch = false);
    void cancelRender();

protected slots:
    void on_renderTask_finished();
    void on_renderTask_imageReady(int physicalDpiX, int physicalDpiY, qreal scaleFactor, Rotation rotation, bool invertColors, bool prefetch, QImage image);

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent*);
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*);

    void mousePressEvent(QGraphicsSceneMouseEvent* event);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);

    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event);

private slots:
    virtual void loadInteractiveElements();

private:
    static Settings* s_settings;

    static QCache< PageItem*, QPixmap > s_cache;

    Model::Page* m_page;

    int m_index;
    QSizeF m_size;

    QList< Model::Link* > m_links;
    QList< Model::Annotation* > m_annotations;
    QList< Model::FormField* > m_formFields;

    bool m_presentationMode;
    bool m_invertColors;

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

    RenderTask* m_renderTask;

};

class ThumbnailItem : public PageItem
{
    Q_OBJECT

public:
    ThumbnailItem(Model::Page* page, int index, QGraphicsItem* parent = 0);

    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

    bool isCurrent() const;
    void setCurrent(bool current);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);

    void contextMenuEvent(QGraphicsSceneContextMenuEvent*);

private slots:
    void loadInteractiveElements();

private:
    QStaticText m_text;
    bool m_current;

};

#endif // PAGEITEM_H
