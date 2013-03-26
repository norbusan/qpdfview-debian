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
#include <QGraphicsSceneHoverEvent>
#include <qmath.h>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QThreadPool>
#include <QTimer>
#include <QToolTip>

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
    m_presentationMode(presentationMode),
    m_invertColors(false),
    m_highlights(),
    m_rubberBandMode(ModifiersMode),
    m_rubberBand(),
    m_physicalDpiX(72),
    m_physicalDpiY(72),
    m_scaleFactor(1.0),
    m_rotation(RotateBy0),
    m_transform(),
    m_normalizedTransform(),
    m_boundingRect(),
    m_pixmap(),
    m_renderTask(0)
{
    if(s_settings == 0)
    {
        s_settings = Settings::instance();
    }

    s_cache.setMaxCost(s_settings->pageItem().cacheSize());

    setAcceptHoverEvents(true);

    m_renderTask = new RenderTask(this);

    connect(m_renderTask, SIGNAL(finished()), SLOT(on_renderTask_finished()));
    connect(m_renderTask, SIGNAL(imageReady(int,int,qreal,Rotation,bool,bool,QImage)), SLOT(on_renderTask_imageReady(int,int,qreal,Rotation,bool,bool,QImage)));

    m_page = page;

    m_index = index;
    m_size = m_page->size();

    QTimer::singleShot(0, this, SLOT(loadInteractiveElements()));

    prepareGeometry();
}

PageItem::~PageItem()
{
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

void PageItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget* widget)
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

    // page

    if(s_settings->pageItem().decoratePages() && !m_presentationMode)
    {
        QColor paperColor = s_settings->pageItem().paperColor();

        if(m_invertColors)
        {
            paperColor.setRgb(~paperColor.rgb());
        }

        painter->fillRect(m_boundingRect, QBrush(paperColor));
    }

    if(pixmap.isNull())
    {
        qreal extent = qMin(0.1 * m_boundingRect.width(), 0.1 * m_boundingRect.height());
        QRectF rect(m_boundingRect.left() + 0.01 * m_boundingRect.width(), m_boundingRect.top() + 0.01 * m_boundingRect.height(), extent, extent);

        s_settings->pageItem().progressIcon().paint(painter, rect.toRect());
    }
    else
    {
        painter->drawPixmap(m_boundingRect.topLeft(), pixmap);
    }

    if(s_settings->pageItem().decoratePages() && !m_presentationMode)
    {
        painter->setPen(QPen(m_invertColors ? Qt::white : Qt::black));
        painter->drawRect(m_boundingRect);
    }

    // links

    if(s_settings->pageItem().decorateLinks() && !m_presentationMode && !m_links.isEmpty())
    {
        painter->save();

        painter->setTransform(m_normalizedTransform, true);
        painter->setPen(QPen(Qt::red));

        foreach(Model::Link* link, m_links)
        {
            painter->drawPath(link->boundary);
        }

        painter->restore();
    }

    // form fields

    if(s_settings->pageItem().decorateFormFields() && !m_presentationMode && !m_formFields.isEmpty())
    {
        painter->save();

        painter->setTransform(m_normalizedTransform, true);
        painter->setPen(QPen(Qt::blue));

        foreach(Model::FormField* formField, m_formFields)
        {
            painter->drawRect(formField->boundary());
        }

        painter->restore();
    }

    // highlights

    if(!m_highlights.isEmpty())
    {
        painter->save();

        painter->setTransform(m_transform, true);

        QColor highlightColor = widget->palette().color(QPalette::Highlight);

        highlightColor.setAlpha(127);

        foreach(QRectF highlight, m_highlights)
        {
            painter->fillRect(highlight.normalized(), QBrush(highlightColor));
        }

        painter->restore();
    }

    // rubber band

    if(!m_rubberBand.isNull())
    {
        QPen pen;
        pen.setColor(m_invertColors ? Qt::white : Qt::black);
        pen.setStyle(Qt::DashLine);

        painter->setPen(pen);
        painter->drawRect(m_rubberBand);
    }
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

int PageItem::physicalDpiX() const
{
    return m_physicalDpiX;
}

int PageItem::physicalDpiY() const
{
    return m_physicalDpiY;
}

void PageItem::setPhysicalDpi(int physicalDpiX, int physicalDpiY)
{
    if((m_physicalDpiX != physicalDpiX || m_physicalDpiY != physicalDpiY) && physicalDpiX > 0 && physicalDpiY > 0)
    {
        refresh();

        m_physicalDpiX = physicalDpiX;
        m_physicalDpiY = physicalDpiY;

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
    if(m_scaleFactor != scaleFactor && scaleFactor > 0.0)
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
    if(m_rotation != rotation && rotation >= 0 && rotation < NumberOfDirections)
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
    cancelRender();

    s_cache.remove(this);

    update();
}

void PageItem::clearHighlights()
{
    m_highlights.clear();

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
        m_renderTask->start(m_page, m_physicalDpiX, m_physicalDpiY, m_scaleFactor, m_rotation, m_invertColors, prefetch);
    }
}

void PageItem::cancelRender()
{
    m_renderTask->cancel();

    m_pixmap = QPixmap();
}

void PageItem::on_renderTask_finished()
{
    update();
}

void PageItem::on_renderTask_imageReady(int physicalDpiX, int physicalDpiY, qreal scaleFactor, Rotation rotation, bool invertColors, bool prefetch, QImage image)
{
    if(m_physicalDpiX != physicalDpiX || m_physicalDpiY != physicalDpiY || !qFuzzyCompare(m_scaleFactor, scaleFactor) || m_rotation != rotation || m_invertColors != invertColors)
    {
        return;
    }

    if(image.isNull())
    {
        qreal extent = qMin(0.1 * m_boundingRect.width(), 0.1 * m_boundingRect.height());
        QRectF rect(0.01 * m_boundingRect.width(), 0.01 * m_boundingRect.height(), extent, extent);

        image = QImage(qFloor(0.01 * m_boundingRect.width() + extent), qFloor(0.01 * m_boundingRect.height() + extent), QImage::Format_ARGB32);
        image.fill(Qt::transparent);

        QPainter painter(&image);
        s_settings->pageItem().errorIcon().paint(&painter, rect.toRect());
    }

    if(prefetch)
    {
        QPixmap pixmap = QPixmap::fromImage(image);

        int cost = pixmap.width() * pixmap.height() * pixmap.depth() / 8;
        s_cache.insert(this, new QPixmap(pixmap), cost);
    }
    else
    {
        if(!m_renderTask->wasCanceled())
        {
            m_pixmap = QPixmap::fromImage(image);
        }
    }
}

void PageItem::hoverEnterEvent(QGraphicsSceneHoverEvent*)
{
}

void PageItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    if(m_rubberBandMode == ModifiersMode && event->modifiers() == Qt::NoModifier)
    {
        // links

        foreach(Model::Link* link, m_links)
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

        foreach(Model::Annotation* annotation, m_annotations)
        {
            if(m_normalizedTransform.mapRect(annotation->boundary()).contains(event->pos()))
            {
                setCursor(Qt::PointingHandCursor);
                QToolTip::showText(event->screenPos(), annotation->contents());

                return;
            }
        }

        // form fields

        foreach(Model::FormField* formField, m_formFields)
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

        foreach(Model::Link* link, m_links)
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

                editAnnotation(annotation, event->screenPos());

                event->accept();
                return;
            }
        }

        // form fields

        foreach(Model::FormField* formField, m_formFields)
        {
            if(m_normalizedTransform.mapRect(formField->boundary()).contains(event->pos()))
            {
                unsetCursor();

                editFormField(formField, event->screenPos());

                event->accept();
                return;
            }
        }
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
        m_formFields = m_page->formFields();
    }

    update();
}

void PageItem::copyToClipboard(const QPoint& screenPos)
{
    QMenu menu;

    QAction* copyTextAction = menu.addAction(tr("Copy &text"));
    QAction* copyImageAction = menu.addAction(tr("Copy &image"));
    QAction* saveImageToFileAction = menu.addAction(tr("Save image to &file..."));

    QAction* action = menu.exec(screenPos);

    if(action == copyTextAction)
    {
        QString text = m_page->text(m_transform.inverted().mapRect(m_rubberBand));

        if(!text.isEmpty())
        {
            QApplication::clipboard()->setText(text);
        }
    }
    else if(action == copyImageAction || action == saveImageToFileAction)
    {
        QImage image;

        if(s_cache.contains(this))
        {
            image = s_cache.object(this)->copy(m_rubberBand.translated(-m_boundingRect.topLeft()).toRect()).toImage();
        }
        else
        {
            image = m_page->render(m_physicalDpiX * m_scaleFactor,  m_scaleFactor * m_physicalDpiY, m_rotation, m_rubberBand.translated(-m_boundingRect.topLeft()).toRect());
        }

        if(!image.isNull())
        {
            if(action == copyImageAction)
            {
                QApplication::clipboard()->setImage(image);
            }
            else if(action == saveImageToFileAction)
            {
                QString fileName = QFileDialog::getSaveFileName(0, tr("Save image to file"), QDir::homePath(), "Portable network graphics (*.png)");

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

        QAction* addTextAction = menu.addAction(tr("Add &text"));
        QAction* addHighlightAction = menu.addAction(tr("Add &highlight"));

        QAction* action = menu.exec(screenPos);

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

            refresh();

            editAnnotation(annotation, screenPos);
        }
    }
}

void PageItem::removeAnnotation(Model::Annotation* annotation, const QPoint& screenPos)
{
    if(m_page->canAddAndRemoveAnnotations())
    {
        QMenu menu;

        QAction* removeAnnotationAction = menu.addAction(tr("&Remove annotation"));

        QAction* action = menu.exec(screenPos);

        if(action == removeAnnotationAction)
        {
            m_annotations.removeAll(annotation);
            m_page->removeAnnotation(annotation);

            refresh();
        }
    }
}

void PageItem::editAnnotation(Model::Annotation* annotation, const QPoint& screenPos)
{
    annotation->showDialog(screenPos);
}

void PageItem::editFormField(Model::FormField* formField, const QPoint& screenPos)
{
    QDialog* formFieldDialog = formField->showDialog(screenPos);

    if(formFieldDialog != 0)
    {
        connect(formFieldDialog, SIGNAL(destroyed()), SLOT(refresh()));
    }
    else
    {
        refresh();
    }
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
        m_transform.scale(m_scaleFactor * m_physicalDpiX / 72.0, m_scaleFactor * m_physicalDpiY / 72.0);
        m_normalizedTransform.scale(m_scaleFactor * m_physicalDpiX / 72.0 * m_size.width(), m_scaleFactor * m_physicalDpiY / 72.0 * m_size.height());
        break;
    case RotateBy90:
    case RotateBy270:
        m_transform.scale(m_scaleFactor * m_physicalDpiY / 72.0, m_scaleFactor * m_physicalDpiX / 72.0);
        m_normalizedTransform.scale(m_scaleFactor * m_physicalDpiY / 72.0 * m_size.width(), m_scaleFactor * m_physicalDpiX / 72.0 * m_size.height());
        break;
    }

    m_boundingRect = m_transform.mapRect(QRectF(QPointF(), m_size));

    m_boundingRect.setWidth(qRound(m_boundingRect.width()));
    m_boundingRect.setHeight(qRound(m_boundingRect.height()));
}

ThumbnailItem::ThumbnailItem(Model::Page* page, int index, QGraphicsItem* parent) : PageItem(page, index, false, parent)
{
    setAcceptHoverEvents(false);

    QFontMetrics fontMetrics = QFontMetrics(QFont());

    m_textWidth = fontMetrics.width(QString::number(index + 1));
    m_textHeight = fontMetrics.height();
}

QRectF ThumbnailItem::boundingRect() const
{
    return PageItem::boundingRect().adjusted(0.0, 0.0, 0.0, 2.0 * m_textHeight);
}

void ThumbnailItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    PageItem::paint(painter, option, widget);

    QRectF boundingRect = PageItem::boundingRect();

    QPointF pos = boundingRect.bottomLeft();
    pos.rx() += 0.5 * (boundingRect.width() - m_textWidth);
    pos.ry() += m_textHeight;

    painter->drawText(pos, QString::number(index() + 1));
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
}
