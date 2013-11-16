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
#include <QTimer>
#include <QToolTip>
#include <QUrl>

#include "settings.h"
#include "model.h"
#include "rendertask.h"

Settings* PageItem::s_settings = 0;

QCache< PageItem*, QPixmap > PageItem::s_cache;

PageItem::PageItem(Model::Page* page, int index, bool presentationMode, QGraphicsItem* parent) : QGraphicsObject(parent),
    m_page(0),
    m_index(-1),
    m_size(),
    m_links(),
    m_annotations(),
    m_formFields(),
    m_annotationOverlay(),
    m_formFieldOverlay(),
    m_presentationMode(presentationMode),
    m_invertColors(false),
    m_highlights(),
    m_rubberBandMode(ModifiersMode),
    m_rubberBand(),
    m_resolutionX(72),
    m_resolutionY(72),
    m_devicePixelRatio(1.0),
    m_scaleFactor(1.0),
    m_rotation(RotateBy0),
    m_transform(),
    m_normalizedTransform(),
    m_boundingRect(),
    m_pixmap(),
    m_renderTask(0),
    m_obsoletePixmap(),
    m_obsoleteTransform()
{
    if(s_settings == 0)
    {
        s_settings = Settings::instance();
    }

    s_cache.setMaxCost(s_settings->pageItem().cacheSize());

    setAcceptHoverEvents(true);

    m_renderTask = new RenderTask(this);

    connect(m_renderTask, SIGNAL(finished()), SLOT(on_renderTask_finished()));
    connect(m_renderTask, SIGNAL(imageReady(int,int,qreal,qreal,Rotation,bool,bool,QImage)), SLOT(on_renderTask_imageReady(int,int,qreal,qreal,Rotation,bool,bool,QImage)));

    m_page = page;

    m_index = index;
    m_size = m_page->size();

    QTimer::singleShot(0, this, SLOT(loadInteractiveElements()));

    prepareGeometry();
}

PageItem::~PageItem()
{
    hideAnnotationOverlay(false);
    hideFormFieldOverlay(false);

    m_renderTask->cancel();
    m_renderTask->wait();

    s_cache.remove(this);

    qDeleteAll(m_links);
    qDeleteAll(m_annotations);
    qDeleteAll(m_formFields);
}

QRectF PageItem::boundingRect() const
{
    return m_boundingRect;
}

void PageItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    paintPage(painter, cachedPixmap());

    paintLinks(painter);
    paintFormFields(painter);

    paintHighlights(painter);
    paintRubberBand(painter);
}

int PageItem::index() const
{
    return m_index;
}

const QSizeF& PageItem::size() const
{
    return m_size;
}

bool PageItem::invertColors()
{
    return m_invertColors;
}

void PageItem::setInvertColors(bool invertColors)
{
    m_invertColors = invertColors;

    refresh();
}

const QList< QRectF >& PageItem::highlights() const
{
    return m_highlights;
}

void PageItem::setHighlights(const QList< QRectF >& highlights)
{
    m_highlights = highlights;

    update();
}

RubberBandMode PageItem::rubberBandMode() const
{
    return m_rubberBandMode;
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

bool PageItem::showsAnnotationOverlay() const
{
    return !m_annotationOverlay.isEmpty();
}

bool PageItem::showsFormFieldOverlay() const
{
    return !m_formFieldOverlay.isEmpty();
}

int PageItem::resolutionX() const
{
    return m_resolutionX;
}

int PageItem::resolutionY() const
{
    return m_resolutionY;
}

void PageItem::setResolution(int resolutionX, int resolutionY)
{
    if((m_resolutionX != resolutionX || m_resolutionY != resolutionY) && resolutionX > 0 && resolutionY > 0)
    {
        refresh();

        m_resolutionX = resolutionX;
        m_resolutionY = resolutionY;

        prepareGeometryChange();
        prepareGeometry();
    }
}

qreal PageItem::devicePixelRatio() const
{
    return m_devicePixelRatio;
}

void PageItem::setDevicePixelRatio(qreal devicePixelRatio)
{
    if(!qFuzzyCompare(m_devicePixelRatio, devicePixelRatio) && devicePixelRatio > 0.0)
    {
        refresh();

        m_devicePixelRatio = devicePixelRatio;

        prepareGeometryChange();
        prepareGeometry();
    }
}

qreal PageItem::scaleFactor() const
{
    return m_scaleFactor;
}

void PageItem::setScaleFactor(qreal scaleFactor)
{
    if(!qFuzzyCompare(m_scaleFactor, scaleFactor) && scaleFactor > 0.0)
    {
        refresh();

        m_scaleFactor = scaleFactor;

        prepareGeometryChange();
        prepareGeometry();
    }
}

Rotation PageItem::rotation() const
{
    return m_rotation;
}

void PageItem::setRotation(Rotation rotation)
{
    if(m_rotation != rotation && rotation >= 0 && rotation < NumberOfRotations)
    {
        refresh();

        m_rotation = rotation;

        prepareGeometryChange();
        prepareGeometry();
    }
}

const QTransform& PageItem::transform() const
{
    return m_transform;
}

const QTransform& PageItem::normalizedTransform() const
{
    return m_normalizedTransform;
}

void PageItem::refresh()
{
    m_renderTask->cancel();

    if(s_settings->pageItem().keepObsoletePixmaps() && s_cache.contains(this))
    {
        m_obsoletePixmap = *s_cache.object(this);
        m_obsoleteTopLeft = m_boundingRect.topLeft();
        m_obsoleteTransform = m_transform.inverted();
    }

    m_pixmap = QPixmap();
    s_cache.remove(this);

    update();
}

void PageItem::startRender(bool prefetch)
{
    if(prefetch && s_cache.contains(this))
    {
        return;
    }

    if(!m_renderTask->isRunning())
    {
        m_renderTask->start(m_page, m_resolutionX, m_resolutionY, effectiveDevicePixelRatio(), m_scaleFactor, m_rotation, m_invertColors, prefetch);
    }
}

void PageItem::cancelRender()
{
    m_renderTask->cancel();

    m_pixmap = QPixmap();
    m_obsoletePixmap = QPixmap();
}

void PageItem::on_renderTask_finished()
{
    update();
}

void PageItem::on_renderTask_imageReady(int resolutionX, int resolutionY, qreal devicePixelRatio, qreal scaleFactor, Rotation rotation, bool invertColors, bool prefetch, QImage image)
{
    if(m_resolutionX != resolutionX || m_resolutionY != resolutionY || !qFuzzyCompare(effectiveDevicePixelRatio(), devicePixelRatio) || !qFuzzyCompare(m_scaleFactor, scaleFactor) || m_rotation != rotation || m_invertColors != invertColors)
    {
        return;
    }

    if(image.isNull())
    {
        // error icon

        const qreal extent = qMin(0.1 * m_boundingRect.width(), 0.1 * m_boundingRect.height());
        const QRectF rect(0.01 * m_boundingRect.width(), 0.01 * m_boundingRect.height(), extent, extent);

        image = QImage(qFloor(0.01 * m_boundingRect.width() + extent), qFloor(0.01 * m_boundingRect.height() + extent), QImage::Format_ARGB32);
        image.fill(Qt::transparent);

        QPainter painter(&image);
        s_settings->pageItem().errorIcon().paint(&painter, rect.toRect());
    }

    if(prefetch)
    {
        QPixmap* pixmap = new QPixmap(QPixmap::fromImage(image));

        int cost = pixmap->width() * pixmap->height() * pixmap->depth() / 8;
        s_cache.insert(this, pixmap, cost);
    }
    else
    {
        if(!m_renderTask->wasCanceled())
        {
            m_pixmap = QPixmap::fromImage(image);
        }
    }

    m_obsoletePixmap = QPixmap();
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
            && (event->modifiers() == s_settings->pageItem().copyToClipboardModifiers() || event->modifiers() == s_settings->pageItem().addAnnotationModifiers())
            && event->button() == Qt::LeftButton)
    {
        setCursor(Qt::CrossCursor);

        if(event->modifiers() == s_settings->pageItem().copyToClipboardModifiers())
        {
            m_rubberBandMode = CopyToClipboardMode;
        }
        else if(event->modifiers() == s_settings->pageItem().addAnnotationModifiers())
        {
            m_rubberBandMode = AddAnnotationMode;
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

    if(event->modifiers() == Qt::NoModifier && event->button() == Qt::LeftButton)
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
                        emit linkClicked(link->page, link->left, link->top);
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

        if(m_presentationMode)
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
    emit sourceRequested(m_index + 1, m_transform.inverted().map(event->pos()));
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
    const QAction* copyImageAction = menu.addAction(tr("Copy &image"));
    const QAction* saveImageToFileAction = menu.addAction(tr("Save image to &file..."));

    const QAction* action = menu.exec(screenPos);

    if(action == copyTextAction)
    {
        const QString text = m_page->text(m_transform.inverted().mapRect(m_rubberBand));

        if(!text.isEmpty())
        {
            QApplication::clipboard()->setText(text);
        }
    }
    else if(action == copyImageAction || action == saveImageToFileAction)
    {
        const QRect rect = m_rubberBand.translated(-m_boundingRect.topLeft()).toRect();
        const QImage image = s_cache.contains(this) ? s_cache.object(this)->copy(rect).toImage() : m_page->render(m_resolutionX * m_scaleFactor, m_scaleFactor * m_resolutionY, m_rotation, rect);

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

            refresh();
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

            refresh();
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
    Overlay discardedOverlay;

#if QT_VERSION >= QT_VERSION_CHECK(4,8,0)

    discardedOverlay.swap(overlay);

#else

    discardedOverlay = overlay;
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

        refresh();
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

    switch(m_rotation)
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

    width /= m_scaleFactor;
    height /= m_scaleFactor;

    proxy->setScale(m_scaleFactor);

    proxy->setGeometry(QRectF(x - proxyPadding, y - proxyPadding, width + proxyPadding, height + proxyPadding));
}

qreal PageItem::effectiveDevicePixelRatio()
{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

    return s_settings->pageItem().useDevicePixelRatio() ? m_devicePixelRatio : 1.0;

#else

    return 1.0;

#endif // QT_VERSION
}

void PageItem::prepareGeometry()
{
    m_transform.reset();
    m_normalizedTransform.reset();

    switch(m_rotation)
    {
    default:
    case RotateBy0:
        break;
    case RotateBy90:
        m_transform.rotate(90.0);
        m_normalizedTransform.rotate(90.0);
        break;
    case RotateBy180:
        m_transform.rotate(180.0);
        m_normalizedTransform.rotate(180.0);
        break;
    case RotateBy270:
        m_transform.rotate(270.0);
        m_normalizedTransform.rotate(270.0);
        break;
    }

    switch(m_rotation)
    {
    default:
    case RotateBy0:
    case RotateBy180:
        m_transform.scale(m_scaleFactor * m_resolutionX / 72.0, m_scaleFactor * m_resolutionY / 72.0);
        m_normalizedTransform.scale(m_scaleFactor * m_resolutionX / 72.0 * m_size.width(), m_scaleFactor * m_resolutionY / 72.0 * m_size.height());
        break;
    case RotateBy90:
    case RotateBy270:
        m_transform.scale(m_scaleFactor * m_resolutionY / 72.0, m_scaleFactor * m_resolutionX / 72.0);
        m_normalizedTransform.scale(m_scaleFactor * m_resolutionY / 72.0 * m_size.width(), m_scaleFactor * m_resolutionX / 72.0 * m_size.height());
        break;
    }

    m_boundingRect = m_transform.mapRect(QRectF(QPointF(), m_size));

    m_boundingRect.setWidth(qRound(m_boundingRect.width()));
    m_boundingRect.setHeight(qRound(m_boundingRect.height()));

    updateAnnotationOverlay();
    updateFormFieldOverlay();
}

QPixmap PageItem::cachedPixmap()
{
    QPixmap pixmap;

    if(s_cache.contains(this))
    {
        pixmap = *s_cache.object(this);
    }
    else
    {
        if(!m_pixmap.isNull())
        {
            pixmap = m_pixmap;
            m_pixmap = QPixmap();

            int cost = pixmap.width() * pixmap.height() * pixmap.depth() / 8;
            s_cache.insert(this, new QPixmap(pixmap), cost);
        }
        else
        {
            startRender();
        }
    }

    return pixmap;
}

void PageItem::paintPage(QPainter* painter, const QPixmap& pixmap) const
{
    if(s_settings->pageItem().decoratePages() && !m_presentationMode)
    {
        QColor paperColor = s_settings->pageItem().paperColor();

        if(m_invertColors)
        {
            paperColor.setRgb(~paperColor.rgb());
        }

        painter->fillRect(m_boundingRect, QBrush(paperColor));
    }

    if(!pixmap.isNull())
    {
        painter->drawPixmap(m_boundingRect.topLeft(), pixmap);
    }
    else if(!m_obsoletePixmap.isNull())
    {
        painter->save();

        painter->setTransform(m_obsoleteTransform, true);
        painter->setTransform(m_transform, true);

        painter->drawPixmap(m_obsoleteTopLeft, m_obsoletePixmap);

        painter->restore();
    }
    else
    {
        // progess icon

        const qreal extent = qMin(0.1 * m_boundingRect.width(), 0.1 * m_boundingRect.height());
        const QRectF rect(m_boundingRect.left() + 0.01 * m_boundingRect.width(), m_boundingRect.top() + 0.01 * m_boundingRect.height(), extent, extent);

        s_settings->pageItem().progressIcon().paint(painter, rect.toRect());
    }

    if(s_settings->pageItem().decoratePages() && !m_presentationMode)
    {
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

ThumbnailItem::ThumbnailItem(Model::Page* page, int index, QGraphicsItem* parent) : PageItem(page, index, false, parent),
#if QT_VERSION >= QT_VERSION_CHECK(4,7,0)
    m_text(QString::number(index + 1)),
#endif // QT_VERSION
    m_current(false)
{
    setAcceptHoverEvents(false);
}

QRectF ThumbnailItem::boundingRect() const
{
#if QT_VERSION >= QT_VERSION_CHECK(4,7,0)

    return PageItem::boundingRect().adjusted(0.0, 0.0, 0.0, 2.0 * m_text.size().height());

#else

    return PageItem::boundingRect().adjusted(0.0, 0.0, 0.0, 2.0 * QFontMetrics(QFont()).height());

#endif // QT_VERSION
}

void ThumbnailItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    PageItem::paint(painter, option, widget);

    const QRectF boundingRect = PageItem::boundingRect();

#if QT_VERSION >= QT_VERSION_CHECK(4,7,0)

    const QSizeF textSize = m_text.size();

    QPointF pos = boundingRect.bottomLeft();
    pos.rx() += 0.5 * (boundingRect.width() - textSize.width());
    pos.ry() += 0.5 * textSize.height();

    painter->drawStaticText(pos, m_text);

#else

    const QString text = QString::number(index() + 1);
    const QFontMetrics fontMetrics = QFontMetrics(QFont());

    QPointF pos = boundingRect.bottomLeft();
    pos.rx() += 0.5 * (boundingRect.width() - fontMetrics.width(text));
    pos.ry() += fontMetrics.height();

    painter->drawText(pos, text);

#endif // QT_VERSION

    if(m_current)
    {
        painter->save();

        painter->setCompositionMode(QPainter::CompositionMode_Multiply);
        painter->fillRect(boundingRect, widget->palette().highlight());

        painter->restore();
    }
}

bool ThumbnailItem::isCurrent() const
{
    return m_current;
}

void ThumbnailItem::setCurrent(bool current)
{
    if(m_current != current)
    {
        m_current = current;

        update();
    }
}

void ThumbnailItem::mousePressEvent(QGraphicsSceneMouseEvent*)
{
    emit linkClicked(index() + 1);
}

void ThumbnailItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent*)
{
}

void ThumbnailItem::mouseMoveEvent(QGraphicsSceneMouseEvent*)
{
}

void ThumbnailItem::mouseReleaseEvent(QGraphicsSceneMouseEvent*)
{
}

void ThumbnailItem::contextMenuEvent(QGraphicsSceneContextMenuEvent*)
{
}

void ThumbnailItem::loadInteractiveElements()
{
    const qreal width = size().width() / 72.0 * 25.4;
    const qreal height = size().height() / 72.0 * 25.4;

    const qreal longEdge = qMax(width, height);
    const qreal shortEdge = qMin(width, height);

    QString paperSize;

    if(qAbs(longEdge - 279.4) <= 1.0 && qAbs(shortEdge - 215.9) <= 1.0)
    {
        paperSize = QLatin1String(" (Letter)");
    }
    else
    {
        qreal longEdgeA = 1189.0;
        qreal shortEdgeA = 841.0;

        qreal longEdgeB = 1414.0;
        qreal shortEdgeB = 1000.0;

        for(int i = 0; i <= 10; ++i)
        {
            if(qAbs(longEdge - longEdgeA) <= 1.0 && qAbs(shortEdge - shortEdgeA) <= 1.0)
            {
                paperSize = QString(" (A%1)").arg(i);
                break;
            }
            else if(qAbs(longEdge - longEdgeB) <= 1.0 && qAbs(shortEdge - shortEdgeB) <= 1.0)
            {
                paperSize = QString(" (B%1)").arg(i);
                break;
            }

            longEdgeA = shortEdgeA;
            shortEdgeA /= qSqrt(2.0);

            longEdgeB = shortEdgeB;
            shortEdgeB /= qSqrt(2.0);
        }
    }

    setToolTip(QString("%1 mm x %2 mm%3").arg(width, 0, 'f', 1).arg(height, 0, 'f', 1).arg(paperSize));
}
