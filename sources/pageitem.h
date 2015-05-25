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
#include <QSet>

class QGraphicsProxyWidget;

#include "renderparam.h"

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

    friend class TileItem;

public:
    enum PaintMode
    {
        DefaultMode,
        PresentationMode,
        ThumbnailMode
    };

    PageItem(Model::Page* page, int index, PaintMode paintMode = DefaultMode, QGraphicsItem* parent = 0);
    ~PageItem();

    const QRectF& uncroppedBoundingRect() const { return m_boundingRect; }

    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*);

    int index() const { return m_index; }

    const QSizeF& size() const { return m_size; }

    qreal displayedWidth() const { return displayedWidth(renderParam()); }
    qreal displayedHeight() const { return displayedHeight(renderParam()); }

    qreal displayedWidth(const RenderParam& renderParam) const;
    qreal displayedHeight(const RenderParam& renderParam) const;

    const QList< QRectF >& highlights() const { return m_highlights; }
    void setHighlights(const QList< QRectF >& highlights);

    RubberBandMode rubberBandMode() const { return m_rubberBandMode; }
    void setRubberBandMode(RubberBandMode rubberBandMode);

    bool showsAnnotationOverlay() const { return !m_annotationOverlay.isEmpty(); }
    bool showsFormFieldOverlay() const { return !m_formFieldOverlay.isEmpty(); }

    const RenderParam& renderParam() const { return m_renderParam; }
    void setRenderParam(const RenderParam& renderParam);

    int resolutionX() const { return m_renderParam.resolutionX(); }
    int resolutionY() const { return m_renderParam.resolutionY(); }
    void setResolution(int resolutionX, int resolutionY);

    qreal devicePixelRatio() const { return m_renderParam.devicePixelRatio(); }
    void setDevicePixelRatio(qreal devicePixelRatio);

    qreal scaleFactor() const { return m_renderParam.scaleFactor(); }
    void setScaleFactor(qreal scaleFactor);

    Rotation rotation() const { return m_renderParam.rotation(); }
    void setRotation(Rotation rotation);

    bool invertColors() const { return m_renderParam.invertColors(); }
    void setInvertColors(bool invertColors);

    bool convertToGrayscale() const { return m_renderParam.convertToGrayscale(); }
    void setConvertToGrayscale(bool convertToGrayscale);

    bool trimMargins() const { return m_renderParam.trimMargins(); }
    void setTrimMargins(bool trimMargins);

    const QTransform& transform() const { return m_transform; }
    const QTransform& normalizedTransform() const { return m_normalizedTransform; }

signals:
    void cropRectChanged();

    void linkClicked(bool newTab, int page, qreal left = qQNaN(), qreal top = qQNaN());
    void linkClicked(bool newTab, const QString& fileName, int page);
    void linkClicked(const QString& url);

    void rubberBandStarted();
    void rubberBandFinished();

    void editSourceRequested(int page, const QPointF& pos);
    void zoomToSelectionRequested(int page, const QRectF& rect);

    void wasModified();

public slots:
    void refresh(bool keepObsoletePixmaps = false, bool dropCachedPixmaps = false);

    int startRender(bool prefetch = false);
    void cancelRender();

protected slots:
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

    Model::Page* m_page;
    QSizeF m_size;

    QRectF m_cropRect;

    void prepareCropRect();
    void updateCropRect();

    int m_index;
    PaintMode m_paintMode;

    bool presentationMode() const;
    bool thumbnailMode() const;

    bool useTiling() const;

    QList< QRectF > m_highlights;

    // interactive elements

    QList< Model::Link* > m_links;
    QList< Model::Annotation* > m_annotations;
    QList< Model::FormField* > m_formFields;

    RubberBandMode m_rubberBandMode;
    QRectF m_rubberBand;

    void copyToClipboard(const QPoint& screenPos);
    void addAnnotation(const QPoint& screenPos);

    void showLinkContextMenu(Model::Link* link, const QPoint& screenPos);
    void showAnnotationContextMenu(Model::Annotation* annotation, const QPoint& screenPos);

    // overlay

    typedef QMap< Model::Annotation*, QGraphicsProxyWidget* > AnnotationOverlay;
    AnnotationOverlay m_annotationOverlay;

    typedef QMap< Model::FormField*, QGraphicsProxyWidget* > FormFieldOverlay;
    FormFieldOverlay m_formFieldOverlay;

    template< typename Overlay, typename Element > void showOverlay(Overlay& overlay, const char* hideOverlay, const QList< Element* >& elements, Element* selectedElement);
    template< typename Overlay, typename Element > void addProxy(Overlay& overlay, const char* hideOverlay, Element* element);
    template< typename Overlay > void hideOverlay(Overlay& overlay, bool deleteLater);
    template< typename Overlay > void updateOverlay(const Overlay& overlay) const;

    void setProxyGeometry(Model::Annotation* annotation, QGraphicsProxyWidget* proxy) const;
    void setProxyGeometry(Model::FormField* formField, QGraphicsProxyWidget* proxy) const;

    // geometry

    RenderParam m_renderParam;

    QTransform m_transform;
    QTransform m_normalizedTransform;
    QRectF m_boundingRect;

    void prepareGeometry();

    QVector< TileItem* > m_tileItems;
    mutable QSet< TileItem* > m_exposedTileItems;

    void prepareTiling();

    // paint

    void paintPage(QPainter* painter, const QRectF& exposedRect) const;

    void paintLinks(QPainter* painter) const;
    void paintFormFields(QPainter* painter) const;

    void paintHighlights(QPainter* painter) const;
    void paintRubberBand(QPainter* painter) const;

};

} // qpdfview

#endif // PAGEITEM_H
