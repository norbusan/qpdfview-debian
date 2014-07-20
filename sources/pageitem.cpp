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

#include "pageitem.h"

#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <qmath.h>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QTimer>
#include <QToolTip>
#include <QUrl>

#include "settings.h"
#include "model.h"
#include "rendertask.h"
#include "tileitem.h"

namespace qpdfview
{

Settings* PageItem::s_settings = 0;

PageItem::PageItem(Model::Page* page, int index, bool presentationMode, QGraphicsItem* parent) : QGraphicsObject(parent),
    m_page(page),
    m_size(page->size()),
    m_index(index),
    m_presentationMode(presentationMode),
    m_links(),
    m_annotations(),
    m_formFields(),
    m_annotationOverlay(),
    m_formFieldOverlay(),
    m_highlights(),
    m_rubberBandMode(ModifiersMode),
    m_rubberBand(),
    m_renderParam(),
    m_transform(),
    m_normalizedTransform(),
    m_boundingRect(),
    m_tileItems()
{
    if(s_settings == 0)
    {
        s_settings = Settings::instance();
    }

    setAcceptHoverEvents(true);

    setFlag(QGraphicsItem::ItemUsesExtendedStyleOption);

    if(!s_settings->pageItem().useTiling())
    {
        TileItem* tile = new TileItem(this);

        m_tileItems.resize(1);
        m_tileItems.squeeze();

        m_tileItems.replace(0, tile);
    }

    QTimer::singleShot(0, this, SLOT(loadInteractiveElements()));

    prepareGeometry();
}

PageItem::~PageItem()
{
    hideAnnotationOverlay(false);
    hideFormFieldOverlay(false);

    TileItem::dropCachedPixmaps(this);

    qDeleteAll(m_links);
    qDeleteAll(m_annotations);
    qDeleteAll(m_formFields);
}

QRectF PageItem::boundingRect() const
{
    return m_boundingRect;
}

void PageItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*)
{
    paintPage(painter, option->exposedRect);

    paintLinks(painter);
    paintFormFields(painter);

    paintHighlights(painter);
    paintRubberBand(painter);
}

void PageItem::setHighlights(const QList< QRectF >& highlights)
{
    m_highlights = highlights;

    update();
}

void PageItem::setRubberBandMode(RubberBandMode rubberBandMode)
{
    if(m_rubberBandMode != rubberBandMode && rubberBandMode >= 0 && rubberBandMode < NumberOfRubberBandModes)
    {
        m_rubberBandMode = rubberBandMode;

        if(m_rubberBandMode == ModifiersMode)
        {
            unsetCursor();
        }
        else
        {
            setCursor(Qt::CrossCursor);
        }
    }
}

void PageItem::setResolution(int resolutionX, int resolutionY)
{
    if((m_renderParam.resolution.resolutionX != resolutionX || m_renderParam.resolution.resolutionY != resolutionY) && resolutionX > 0 && resolutionY > 0)
    {
        refresh(true);

        m_renderParam.resolution.resolutionX = resolutionX;
        m_renderParam.resolution.resolutionY = resolutionY;

        prepareGeometryChange();
        prepareGeometry();
    }
}

void PageItem::setDevicePixelRatio(qreal devicePixelRatio)
{
    if(!s_settings->pageItem().useDevicePixelRatio())
    {
        return;
    }

    if(!qFuzzyCompare(m_renderParam.resolution.devicePixelRatio, devicePixelRatio) && devicePixelRatio > 0.0)
    {
        refresh(true);

        m_renderParam.resolution.devicePixelRatio = devicePixelRatio;

        prepareGeometryChange();
        prepareGeometry();
    }
}

void PageItem::setScaleFactor(qreal scaleFactor)
{
    if(!qFuzzyCompare(m_renderParam.scaleFactor, scaleFactor) && scaleFactor > 0.0)
    {
        refresh(true);

        m_renderParam.scaleFactor = scaleFactor;

        prepareGeometryChange();
        prepareGeometry();
    }
}

void PageItem::setRotation(Rotation rotation)
{
    if(m_renderParam.rotation != rotation && rotation >= 0 && rotation < NumberOfRotations)
    {
        refresh(false);

        m_renderParam.rotation = rotation;

        prepareGeometryChange();
        prepareGeometry();
    }
}

void PageItem::setInvertColors(bool invertColors)
{
    if(m_renderParam.invertColors != invertColors)
    {
        refresh(false);

        m_renderParam.invertColors = invertColors;
    }
}


void PageItem::refresh(bool keepObsoletePixmaps, bool dropCachedPixmaps)
{
    if(!s_settings->pageItem().useTiling())
    {
        m_tileItems.first()->refresh(keepObsoletePixmaps);
    }
    else
    {
        foreach(TileItem* tile, m_tileItems)
        {
            tile->refresh(keepObsoletePixmaps);
        }
    }

    if(dropCachedPixmaps)
    {
        TileItem::dropCachedPixmaps(this);
    }

    update();
}

int PageItem::startRender(bool prefetch)
{
    int cost = 0;

    if(!s_settings->pageItem().useTiling())
    {
        cost += m_tileItems.first()->startRender(prefetch);
    }
    else
    {
        foreach(TileItem* tile, m_tileItems)
        {
            cost += tile->startRender(prefetch);
        }
    }

    return cost;
}

void PageItem::cancelRender()
{
    if(!s_settings->pageItem().useTiling())
    {
        m_tileItems.first()->cancelRender();
    }
    else
    {
        foreach(TileItem* tile, m_tileItems)
        {
            tile->cancelRender();
        }
    }
}

void PageItem::showAnnotationOverlay(Model::Annotation* selectedAnnotation)
{
    if(s_settings->pageItem().annotationOverlay())
    {
        showOverlay(m_annotationOverlay, SLOT(hideAnnotationOverlay()), m_annotations, selectedAnnotation);
    }
    else
    {
        hideAnnotationOverlay(false);

        addProxy(m_annotationOverlay, SLOT(hideAnnotationOverlay()), selectedAnnotation);
        m_annotationOverlay.value(selectedAnnotation)->widget()->setFocus();
    }
}

void PageItem::hideAnnotationOverlay(bool deleteLater)
{
    hideOverlay(m_annotationOverlay, deleteLater);
}

void PageItem::updateAnnotationOverlay()
{
    updateOverlay(m_annotationOverlay);
}

void PageItem::showFormFieldOverlay(Model::FormField* selectedFormField)
{
    if(s_settings->pageItem().formFieldOverlay())
    {
        showOverlay(m_formFieldOverlay, SLOT(hideFormFieldOverlay()), m_formFields, selectedFormField);
    }
    else
    {
        hideFormFieldOverlay(false);

        addProxy(m_formFieldOverlay, SLOT(hideFormFieldOverlay()), selectedFormField);
        m_formFieldOverlay.value(selectedFormField)->widget()->setFocus();
    }
}

void PageItem::updateFormFieldOverlay()
{
    updateOverlay(m_formFieldOverlay);
}

void PageItem::hideFormFieldOverlay(bool deleteLater)
{
    hideOverlay(m_formFieldOverlay, deleteLater);
}

void PageItem::hoverEnterEvent(QGraphicsSceneHoverEvent*)
{
}

void PageItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    if(m_rubberBandMode == ModifiersMode && event->modifiers() == Qt::NoModifier)
    {
        // links

        foreach(const Model::Link* link, m_links)
        {
            if(m_normalizedTransform.map(link->boundary).contains(event->pos()))
            {
                if(link->page != -1 && (link->urlOrFileName.isNull() || !m_presentationMode))
                {
                    setCursor(Qt::PointingHandCursor);

                    if(link->urlOrFileName.isNull())
                    {
                        QToolTip::showText(event->screenPos(), tr("Go to page %1.").arg(link->page));
                    }
                    else
                    {
                        QToolTip::showText(event->screenPos(), tr("Go to page %1 of file '%2'.").arg(link->page).arg(link->urlOrFileName));
                    }

                    return;
                }
                else if(!link->urlOrFileName.isNull() && !m_presentationMode)
                {
                    setCursor(Qt::PointingHandCursor);
                    QToolTip::showText(event->screenPos(), tr("Open '%1'.").arg(link->urlOrFileName));

                    return;
                }
            }
        }

        if(m_presentationMode)
        {
            unsetCursor();
            QToolTip::hideText();

            return;
        }

        // annotations

        foreach(const Model::Annotation* annotation, m_annotations)
        {
            if(m_normalizedTransform.mapRect(annotation->boundary()).contains(event->pos()))
            {
                setCursor(Qt::PointingHandCursor);
                QToolTip::showText(event->screenPos(), annotation->contents());

                return;
            }
        }

        // form fields

        foreach(const Model::FormField* formField, m_formFields)
        {
            if(m_normalizedTransform.mapRect(formField->boundary()).contains(event->pos()))
            {
                setCursor(Qt::PointingHandCursor);
                QToolTip::showText(event->screenPos(), tr("Edit form field '%1'.").arg(formField->name()));

                return;
            }
        }

        unsetCursor();
        QToolTip::hideText();
    }
}

void PageItem::hoverLeaveEvent(QGraphicsSceneHoverEvent*)
{
}

void PageItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    // rubber band

    if(m_rubberBandMode == ModifiersMode && !m_presentationMode
            && (event->modifiers() == Qt::NoModifier
                || event->modifiers() == s_settings->pageItem().copyToClipboardModifiers()
                || event->modifiers() == s_settings->pageItem().addAnnotationModifiers())
            && (event->button() == Qt::LeftButton || event->button() == Qt::MidButton))
    {
        setCursor(Qt::CrossCursor);

        if(event->modifiers() == s_settings->pageItem().copyToClipboardModifiers() && event->button() == Qt::LeftButton)
        {
            m_rubberBandMode = CopyToClipboardMode;
        }
        else if(event->modifiers() == s_settings->pageItem().addAnnotationModifiers() && event->button() == Qt::LeftButton)
        {
            m_rubberBandMode = AddAnnotationMode;
        }
        else if(event->modifiers() == Qt::NoModifier && event->button() == Qt::MidButton)
        {
            m_rubberBandMode = ZoomToSelectionMode;
        }
    }

    if(m_rubberBandMode != ModifiersMode)
    {
        m_rubberBand = QRectF(event->pos(), QSizeF());

        emit rubberBandStarted();

        update();

        event->accept();
        return;
    }

    if(event->modifiers() == Qt::NoModifier
            && (event->button() == Qt::LeftButton || event->button() == Qt::MidButton))
    {
        // links

        foreach(const Model::Link* link, m_links)
        {
            if(m_normalizedTransform.map(link->boundary).contains(event->pos()))
            {
                unsetCursor();

                if(link->page != -1 && (link->urlOrFileName.isNull() || !m_presentationMode))
                {
                    if(link->urlOrFileName.isNull())
                    {
                        emit linkClicked(event->button() == Qt::MidButton, link->page, link->left, link->top);
                    }
                    else
                    {
                        emit linkClicked(link->urlOrFileName, link->page);
                    }

                    event->accept();
                    return;
                }
                else if(!link->urlOrFileName.isNull() && !m_presentationMode)
                {
                    emit linkClicked(link->urlOrFileName);

                    event->accept();
                    return;
                }
            }
        }

        if(event->button() == Qt::MidButton || m_presentationMode)
        {
            event->ignore();
            return;
        }

        // annotations

        foreach(Model::Annotation* annotation, m_annotations)
        {
            if(m_normalizedTransform.mapRect(annotation->boundary()).contains(event->pos()))
            {
                unsetCursor();

                showAnnotationOverlay(annotation);

                event->accept();
                return;
            }
        }

        hideAnnotationOverlay();

        // form fields

        foreach(Model::FormField* formField, m_formFields)
        {
            if(m_normalizedTransform.mapRect(formField->boundary()).contains(event->pos()))
            {
                unsetCursor();

                showFormFieldOverlay(formField);

                event->accept();
                return;
            }
        }

        hideFormFieldOverlay();
    }

    event->ignore();
}

void PageItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    emit editSourceRequested(m_index + 1, m_transform.inverted().map(event->pos()));

    event->accept();
}

void PageItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if(!m_rubberBand.isNull())
    {
        if(m_boundingRect.contains(event->pos()))
        {
            m_rubberBand.setBottomRight(event->pos());

            update();

            event->accept();
            return;
        }
    }

    event->ignore();
}

void PageItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if(!m_rubberBand.isNull())
    {
        unsetCursor();

        m_rubberBand = m_rubberBand.normalized();

        if(m_rubberBandMode == CopyToClipboardMode)
        {
            copyToClipboard(event->screenPos());
        }
        else if(m_rubberBandMode == AddAnnotationMode)
        {
            addAnnotation(event->screenPos());
        }
        else if(m_rubberBandMode == ZoomToSelectionMode)
        {
            emit zoomToSelectionRequested(m_index + 1, m_normalizedTransform.inverted().mapRect(m_rubberBand));
        }

        m_rubberBandMode = ModifiersMode;
        m_rubberBand = QRectF();

        emit rubberBandFinished();

        update();

        event->accept();
        return;
    }

    event->ignore();
}

void PageItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    if(m_presentationMode)
    {
        event->ignore();
        return;
    }

    foreach(Model::Annotation* annotation, m_annotations)
    {
        if(m_normalizedTransform.mapRect(annotation->boundary()).contains(event->pos()))
        {
            unsetCursor();

            removeAnnotation(annotation, event->screenPos());

            event->accept();
            return;
        }
    }

    event->ignore();
}

void PageItem::loadInteractiveElements()
{
    m_links = m_page->links();

    if(!m_presentationMode)
    {
        m_annotations = m_page->annotations();

        foreach(const Model::Annotation* annotation, m_annotations)
        {
            connect(annotation, SIGNAL(wasModified()), SIGNAL(wasModified()));
        }

        m_formFields = m_page->formFields();

        foreach(const Model::FormField* formField, m_formFields)
        {
            connect(formField, SIGNAL(wasModified()), SIGNAL(wasModified()));
        }
    }

    update();
}

void PageItem::copyToClipboard(const QPoint& screenPos)
{
    QMenu menu;

    const QAction* copyTextAction = menu.addAction(tr("Copy &text"));
    QAction* useTextAsSelectionAction = menu.addAction(tr("Use text as &selection"));
    const QAction* copyImageAction = menu.addAction(tr("Copy &image"));
    const QAction* saveImageToFileAction = menu.addAction(tr("Save image to &file..."));

    useTextAsSelectionAction->setVisible(QApplication::clipboard()->supportsSelection());

    const QAction* action = menu.exec(screenPos);

    if(action == copyTextAction || action == useTextAsSelectionAction)
    {
        const QString text = m_page->text(m_transform.inverted().mapRect(m_rubberBand));

        if(!text.isEmpty())
        {
            if(action == copyTextAction)
            {
                QApplication::clipboard()->setText(text);
            }
            else
            {
                QApplication::clipboard()->setText(text, QClipboard::Selection);
            }
        }
    }
    else if(action == copyImageAction || action == saveImageToFileAction)
    {
        const QRect rect = m_rubberBand.translated(-m_boundingRect.topLeft()).toRect();
        const QImage image = m_page->render(m_renderParam.resolution.resolutionX * m_renderParam.scaleFactor,
                                            m_renderParam.resolution.resolutionY * m_renderParam.scaleFactor,
                                            m_renderParam.rotation, rect);

        if(!image.isNull())
        {
            if(action == copyImageAction)
            {
                QApplication::clipboard()->setImage(image);
            }
            else if(action == saveImageToFileAction)
            {
                const QString fileName = QFileDialog::getSaveFileName(0, tr("Save image to file"), QDir::homePath(), "Portable network graphics (*.png)");

                if(!image.save(fileName, "PNG"))
                {
                    QMessageBox::warning(0, tr("Warning"), tr("Could not save image to file '%1'.").arg(fileName));
                }
            }
        }
    }
}

void PageItem::addAnnotation(const QPoint& screenPos)
{
    if(m_page->canAddAndRemoveAnnotations())
    {
        QMenu menu;

        const QAction* addTextAction = menu.addAction(tr("Add &text"));
        const QAction* addHighlightAction = menu.addAction(tr("Add &highlight"));

        const QAction* action = menu.exec(screenPos);

        if(action == addTextAction || action == addHighlightAction)
        {
            QRectF boundary = m_normalizedTransform.inverted().mapRect(m_rubberBand);

            Model::Annotation* annotation = 0;

            if(action == addTextAction)
            {
                boundary.setWidth(24.0 / m_size.width());
                boundary.setHeight(24.0 / m_size.height());

                annotation = m_page->addTextAnnotation(boundary, s_settings->pageItem().annotationColor());
            }
            else if(action == addHighlightAction)
            {
                annotation = m_page->addHighlightAnnotation(boundary, s_settings->pageItem().annotationColor());
            }

            m_annotations.append(annotation);
            connect(annotation, SIGNAL(wasModified()), SIGNAL(wasModified()));

            refresh(false, true);
            emit wasModified();

            showAnnotationOverlay(annotation);
        }
    }
}

void PageItem::removeAnnotation(Model::Annotation* annotation, const QPoint& screenPos)
{
    if(m_page->canAddAndRemoveAnnotations())
    {
        QMenu menu;

        const QAction* removeAnnotationAction = menu.addAction(tr("&Remove annotation"));

        const QAction* action = menu.exec(screenPos);

        if(action == removeAnnotationAction)
        {
            m_annotations.removeAll(annotation);
            m_page->removeAnnotation(annotation);

            annotation->deleteLater();

            refresh(false, true);
            emit wasModified();
        }
    }
}

template< typename Overlay, typename Element >
void PageItem::showOverlay(Overlay& overlay, const char* hideOverlay, const QList< Element* >& elements, Element* selectedElement)
{
    foreach(Element* element, elements)
    {
        if(!overlay.contains(element))
        {
            addProxy(overlay, hideOverlay, element);
        }

        if(element == selectedElement)
        {
            overlay.value(element)->widget()->setFocus();
        }
    }
}

template< typename Overlay, typename Element >
void PageItem::addProxy(Overlay& overlay, const char* hideOverlay, Element* element)
{
    QGraphicsProxyWidget* proxy = new QGraphicsProxyWidget(this);
    proxy->setWidget(element->createWidget());

#if QT_VERSION >= QT_VERSION_CHECK(4,7,0)

    proxy->setAutoFillBackground(true);

#endif // QT_VERSION

    overlay.insert(element, proxy);
    setProxyGeometry(element, proxy);

    connect(proxy, SIGNAL(visibleChanged()), hideOverlay);
}

template< typename Overlay >
void PageItem::hideOverlay(Overlay& overlay, bool deleteLater)
{
#if QT_VERSION >= QT_VERSION_CHECK(4,8,0)

    Overlay discardedOverlay;
    discardedOverlay.swap(overlay);

#else

    Overlay discardedOverlay(overlay);
    overlay = Overlay();

#endif // QT_VERSION

    if(!discardedOverlay.isEmpty())
    {
        for(typename Overlay::const_iterator i = discardedOverlay.constBegin(); i != discardedOverlay.constEnd(); ++i)
        {
            if(deleteLater)
            {
                i.value()->deleteLater();
            }
            else
            {
                delete i.value();
            }
        }

        refresh(false, true);
    }
}

template< typename Overlay >
void PageItem::updateOverlay(const Overlay& overlay) const
{
    for(typename Overlay::const_iterator i = overlay.constBegin(); i != overlay.constEnd(); ++i)
    {
        setProxyGeometry(i.key(), i.value());
    }
}

void PageItem::setProxyGeometry(Model::Annotation* annotation, QGraphicsProxyWidget* proxy) const
{
    const QPointF center = m_normalizedTransform.map(annotation->boundary().center());

    qreal x = center.x() - 0.5 * proxy->preferredWidth();
    qreal y = center.y() - 0.5 * proxy->preferredHeight();
    qreal width = proxy->preferredWidth();
    qreal height = proxy->preferredHeight();

    x = qMax(x, m_boundingRect.left() + proxyPadding);
    y = qMax(y, m_boundingRect.top() + proxyPadding);

    width = qMin(width, m_boundingRect.right() - proxyPadding - x);
    height = qMin(height, m_boundingRect.bottom() - proxyPadding - y);

    proxy->setGeometry(QRectF(x, y, width, height));
}

void PageItem::setProxyGeometry(Model::FormField* formField, QGraphicsProxyWidget* proxy) const
{
    QRectF rect = m_normalizedTransform.mapRect(formField->boundary());

    qreal x = rect.x();
    qreal y = rect.y();
    qreal width = rect.width();
    qreal height = rect.height();

    switch(m_renderParam.rotation)
    {
    default:
    case RotateBy0:
        proxy->setRotation(0.0);
        break;
    case RotateBy90:
        x += width;
        qSwap(width, height);

        proxy->setRotation(90.0);
        break;
    case RotateBy180:
        x += width;
        y += height;

        proxy->setRotation(180.0);
        break;
    case RotateBy270:
        y += height;
        qSwap(width, height);

        proxy->setRotation(270.0);
        break;
    }

    width /= m_renderParam.scaleFactor;
    height /= m_renderParam.scaleFactor;

    proxy->setScale(m_renderParam.scaleFactor);

    proxy->setGeometry(QRectF(x - proxyPadding, y - proxyPadding, width + proxyPadding, height + proxyPadding));
}

void PageItem::prepareGeometry()
{
    m_transform.reset();

    m_transform.scale(m_renderParam.resolution.resolutionX * m_renderParam.scaleFactor / 72.0,
                      m_renderParam.resolution.resolutionY * m_renderParam.scaleFactor / 72.0);

    switch(m_renderParam.rotation)
    {
    default:
    case RotateBy0:
        break;
    case RotateBy90:
        m_transform.rotate(90.0);
        break;
    case RotateBy180:
        m_transform.rotate(180.0);
        break;
    case RotateBy270:
        m_transform.rotate(270.0);
        break;
    }

    m_normalizedTransform = m_transform;
    m_normalizedTransform.scale(m_size.width(), m_size.height());


    m_boundingRect = m_transform.mapRect(QRectF(QPointF(), m_size));

    m_boundingRect.setWidth(qRound(m_boundingRect.width()));
    m_boundingRect.setHeight(qRound(m_boundingRect.height()));


    prepareTiling();

    updateAnnotationOverlay();
    updateFormFieldOverlay();
}

void PageItem::prepareTiling()
{
    if(!s_settings->pageItem().useTiling())
    {
        return;
    }


    const qreal pageWidth = m_boundingRect.width();
    const qreal pageHeight = m_boundingRect.height();

    const int tileSize = s_settings->pageItem().tileSize();

    int tileWidth = pageWidth < pageHeight ? tileSize * pageWidth / pageHeight : tileSize;
    int tileHeight = pageHeight < pageWidth ? tileSize * pageHeight / pageWidth : tileSize;

    const int columnCount = qCeil(pageWidth / tileWidth);
    const int rowCount = qCeil(pageHeight / tileHeight);

    tileWidth = qCeil(pageWidth / columnCount);
    tileHeight = qCeil(pageHeight / rowCount);


    const int newCount = columnCount * rowCount;
    const int oldCount = m_tileItems.count();

    for(int index = newCount; index < oldCount; ++index)
    {
        m_tileItems.at(index)->deleteAfterRender();
    }

    m_tileItems.resize(newCount);

    for(int index = oldCount; index < newCount; ++index)
    {
        m_tileItems.replace(index, new TileItem(this));
    }

    if(oldCount != newCount)
    {
        foreach(TileItem* tile, m_tileItems)
        {
            tile->dropObsoletePixmap();
        }
    }


    for(int column = 0; column < columnCount; ++column)
    {
        for(int row = 0; row < rowCount; ++row)
        {
            const int left = column > 0 ? column * tileWidth : 0.0;
            const int top = row > 0 ? row * tileHeight : 0.0;

            const int width = column < (columnCount - 1) ? tileWidth : pageWidth - left;
            const int height = row < (rowCount - 1) ? tileHeight : pageHeight - top;

            TileItem* tile = m_tileItems.at(column * rowCount + row);

            tile->setRect(QRect(left, top, width, height));
        }
    }
}

void PageItem::paintPage(QPainter* painter, const QRectF& exposedRect) const
{
    if(s_settings->pageItem().decoratePages() && !m_presentationMode)
    {
        // background

        QColor paperColor = s_settings->pageItem().paperColor();

        if(m_renderParam.invertColors)
        {
            paperColor.setRgb(~paperColor.rgb());
        }

        painter->fillRect(m_boundingRect, QBrush(paperColor));
    }

    // tiles

    if(!s_settings->pageItem().useTiling())
    {
        m_tileItems.first()->paint(painter, m_boundingRect.topLeft());
    }
    else
    {
        const QRectF& translatedExposedRect = exposedRect.translated(-m_boundingRect.topLeft());

        foreach(TileItem* tile, m_tileItems)
        {
            if(translatedExposedRect.intersects(tile->rect()))
            {
                tile->paint(painter, m_boundingRect.topLeft());
            }
            else
            {
                tile->cancelRender();
            }
        }
    }

    if(s_settings->pageItem().decoratePages() && !m_presentationMode)
    {
        // border

        painter->drawRect(m_boundingRect);
    }
}

void PageItem::paintLinks(QPainter* painter) const
{
    if(s_settings->pageItem().decorateLinks() && !m_presentationMode && !m_links.isEmpty())
    {
        painter->save();

        painter->setTransform(m_normalizedTransform, true);
        painter->setPen(QPen(Qt::red, 0.0));

        foreach(const Model::Link* link, m_links)
        {
            painter->drawPath(link->boundary);
        }

        painter->restore();
    }
}

void PageItem::paintFormFields(QPainter* painter) const
{
    if(s_settings->pageItem().decorateFormFields() && !m_presentationMode && !m_formFields.isEmpty())
    {
        painter->save();

        painter->setTransform(m_normalizedTransform, true);
        painter->setPen(QPen(Qt::blue, 0.0));

        foreach(const Model::FormField* formField, m_formFields)
        {
            painter->drawRect(formField->boundary());
        }

        painter->restore();
    }
}

void PageItem::paintHighlights(QPainter* painter) const
{
    if(!m_highlights.isEmpty())
    {
        painter->save();

        painter->setTransform(m_transform, true);
        painter->setPen(QPen(s_settings->pageItem().highlightColor(), 0.0));
        painter->setBrush(QBrush(s_settings->pageItem().highlightColor()));
        painter->setCompositionMode(QPainter::CompositionMode_Multiply);

        foreach(const QRectF highlight, m_highlights)
        {
            painter->drawRect(highlight.normalized());
        }

        painter->restore();
    }
}

void PageItem::paintRubberBand(QPainter* painter) const
{
    if(!m_rubberBand.isNull())
    {
        painter->save();

        painter->setPen(QPen(Qt::white, 0.0, Qt::DashLine));
        painter->setCompositionMode(QPainter::CompositionMode_Difference);

        painter->drawRect(m_rubberBand);

        painter->restore();
    }
}

} // qpdfview
