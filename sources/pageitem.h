/*

Copyright 2012-2014 Adam Reichold

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

#if QT_VERSION >= QT_VERSION_CHECK(4,7,0)

#include <QStaticText>

#endif // QT_VERSION

class QGraphicsProxyWidget;

#include "global.h"

namespace qpdfview
{

namespace Model
{
struct Link;
class Annotation;
class FormField;
class Page;
}

class Settings;
class RenderTask;
class TileItem;

class PageItem : public QGraphicsObject
{
    Q_OBJECT

public:
    PageItem(Model::Page* page, int index, bool presentationMode = false, QGraphicsItem* parent = 0);
    ~PageItem();

    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*);

    inline int index() const { return m_index; }
    inline bool presentationMode() const { return m_presentationMode; }

    inline const QSizeF& size() const { return m_size; }

    inline qreal unscaledWidth() const
    {
        switch(m_rotation)
        {
        default:
        case RotateBy0:
        case RotateBy180:
            return m_resolutionX / 72.0 * m_size.width();
        case RotateBy90:
        case RotateBy270:
            return m_resolutionX / 72.0 * m_size.height();
        }
    }

    inline qreal unscaledHeight() const
    {
        switch(m_rotation)
        {
        default:
        case RotateBy0:
        case RotateBy180:
            return m_resolutionY / 72.0 * m_size.height();
        case RotateBy90:
        case RotateBy270:
            return m_resolutionY / 72.0 * m_size.width();
        }
    }

    inline const QList< QRectF >& highlights() const { return m_highlights; }
    void setHighlights(const QList< QRectF >& highlights);

    inline RubberBandMode rubberBandMode() const { return m_rubberBandMode; }
    void setRubberBandMode(RubberBandMode rubberBandMode);

    inline bool showsAnnotationOverlay() const { return !m_annotationOverlay.isEmpty(); }
    inline bool showsFormFieldOverlay() const { return !m_formFieldOverlay.isEmpty(); }

    inline int resolutionX() const { return m_resolutionX; }
    inline int resolutionY() const { return m_resolutionY; }
    void setResolution(int resolutionX, int resolutionY);

    inline qreal devicePixelRatio() const { return m_devicePixelRatio; }
    void setDevicePixelRatio(qreal devicePixelRatio);

    inline qreal scaleFactor() const { return m_scaleFactor; }
    void setScaleFactor(qreal scaleFactor);

    inline Rotation rotation() const { return m_rotation; }
    void setRotation(Rotation rotation);

    inline bool invertColors() { return m_invertColors; }
    void setInvertColors(bool invertColors);

    inline const QTransform& transform() const { return m_transform; }
    inline const QTransform& normalizedTransform() const { return m_normalizedTransform; }

signals:
    void linkClicked(bool newTab, int page, qreal left = 0.0, qreal top = 0.0);
    void linkClicked(const QString& url);
    void linkClicked(const QString& fileName, int page);

    void rubberBandStarted();
    void rubberBandFinished();

    void editSourceRequested(int page, const QPointF& pos);
    void zoomToSelectionRequested(int page, const QRectF& rect);

    void wasModified();

public slots:
    void refresh();

    void startRender(bool prefetch = false);
    void cancelRender();

protected slots:
    void on_renderTask_finished();
    void on_renderTask_pixmapReady(int resolutionX, int resolutionY, qreal devicePixelRatio,
                                   qreal scaleFactor, Rotation rotation, bool invertColors, bool prefetch,
                                   QPixmap pixmap);

    void showAnnotationOverlay(Model::Annotation* selectedAnnotation);
    void hideAnnotationOverlay(bool deleteLater = true);
    void updateAnnotationOverlay();

    void showFormFieldOverlay(Model::FormField* selectedFormField);
    void hideFormFieldOverlay(bool deleteLater = true);
    void updateFormFieldOverlay();

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
    Q_DISABLE_COPY(PageItem)

    static Settings* s_settings;

    static QCache< PageItem*, QPixmap > s_cache;

    Model::Page* m_page;
    QSizeF m_size;

    int m_index;
    bool m_presentationMode;

    QList< Model::Link* > m_links;
    QList< Model::Annotation* > m_annotations;
    QList< Model::FormField* > m_formFields;

    typedef QMap< Model::Annotation*, QGraphicsProxyWidget* > AnnotationOverlay;
    AnnotationOverlay m_annotationOverlay;

    typedef QMap< Model::FormField*, QGraphicsProxyWidget* > FormFieldOverlay;
    FormFieldOverlay m_formFieldOverlay;

    QList< QRectF > m_highlights;

    RubberBandMode m_rubberBandMode;
    QRectF m_rubberBand;

    void copyToClipboard(const QPoint& screenPos);

    void addAnnotation(const QPoint& screenPos);
    void removeAnnotation(Model::Annotation* annotation, const QPoint& screenPos);

    template< typename Overlay, typename Element > void showOverlay(Overlay& overlay, const char* hideOverlay, const QList< Element* >& elements, Element* selectedElement);
    template< typename Overlay, typename Element > void addProxy(Overlay& overlay, const char* hideOverlay, Element* element);
    template< typename Overlay > void hideOverlay(Overlay& overlay, bool deleteLater);
    template< typename Overlay > void updateOverlay(const Overlay& overlay) const;

    static const qreal proxyPadding = 2.0;

    void setProxyGeometry(Model::Annotation* annotation, QGraphicsProxyWidget* proxy) const;
    void setProxyGeometry(Model::FormField* formField, QGraphicsProxyWidget* proxy) const;

    // geometry

    int m_resolutionX;
    int m_resolutionY;
    qreal m_devicePixelRatio;

    qreal m_scaleFactor;
    Rotation m_rotation;
    bool m_invertColors;

    QTransform m_transform;
    QTransform m_normalizedTransform;
    QRectF m_boundingRect;

    qreal effectiveDevicePixelRatio();

    bool m_pixmapError;
    QPixmap m_pixmap;

    void prepareGeometry();
    void prepareTiling();

    RenderTask* m_renderTask;

    QVector< TileItem* > m_tileItems;

    // obsolete pixmap

    QPixmap m_obsoletePixmap;
    QPointF m_obsoleteTopLeft;
    QTransform m_obsoleteTransform;

    // paint

    QPixmap cachedPixmap();
    void paintPage(QPainter* painter, const QPixmap& pixmap) const;

    void paintLinks(QPainter* painter) const;
    void paintFormFields(QPainter* painter) const;

    void paintHighlights(QPainter* painter) const;
    void paintRubberBand(QPainter* painter) const;

};

class ThumbnailItem : public PageItem
{
    Q_OBJECT

public:
    ThumbnailItem(Model::Page* page, int index, QGraphicsItem* parent = 0);

    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

    inline bool isCurrent() const { return m_current; }
    void setCurrent(bool current);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);

    void contextMenuEvent(QGraphicsSceneContextMenuEvent*);

private slots:
    void loadInteractiveElements();

private:
    Q_DISABLE_COPY(ThumbnailItem)

#if QT_VERSION >= QT_VERSION_CHECK(4,7,0)

    QStaticText m_text;

#else

    QString m_text;

#endif // QT_VERSION

    bool m_current;

};

} // qpdfview

#endif // PAGEITEM_H
