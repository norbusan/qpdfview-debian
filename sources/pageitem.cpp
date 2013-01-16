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
#include <QtConcurrentRun>
#include <QFileDialog>
#include <QGraphicsSceneHoverEvent>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QTimer>
#include <QToolTip>

#include "model.h"

QCache< PageItem*, QPixmap > PageItem::s_cache(32 * 1024 * 1024);

bool PageItem::s_decoratePages = true;
bool PageItem::s_decorateLinks = true;
bool PageItem::s_decorateFormFields = true;

QColor PageItem::s_backgroundColor(Qt::darkGray);
QColor PageItem::s_paperColor(Qt::white);

bool PageItem::s_invertColors = false;

Qt::KeyboardModifiers PageItem::s_copyToClipboardModifiers(Qt::ShiftModifier);
Qt::KeyboardModifiers PageItem::s_addAnnotationModifiers(Qt::ControlModifier);

int PageItem::cacheSize()
{
    return s_cache.totalCost();
}

void PageItem::setCacheSize(int cacheSize)
{
    s_cache.setMaxCost(cacheSize);
}

bool PageItem::decoratePages()
{
    return s_decoratePages;
}

void PageItem::setDecoratePages(bool decoratePages)
{
    s_decoratePages = decoratePages;
}

bool PageItem::decorateLinks()
{
    return s_decorateLinks;
}

void PageItem::setDecorateLinks(bool decorateLinks)
{
    s_decorateLinks = decorateLinks;
}

bool PageItem::decorateFormFields()
{
    return s_decorateFormFields;
}

void PageItem::setDecorateFormFields(bool decorateFormFields)
{
    s_decorateFormFields = decorateFormFields;
}

const QColor& PageItem::backgroundColor()
{
    return s_backgroundColor;
}

void PageItem::setBackgroundColor(const QColor& backgroundColor)
{
    if(backgroundColor.isValid())
    {
        s_backgroundColor = backgroundColor;
    }
}

const QColor& PageItem::paperColor()
{
    return s_paperColor;
}

void PageItem::setPaperColor(const QColor& paperColor)
{
    if(paperColor.isValid())
    {
        s_paperColor = paperColor;
    }
}

bool PageItem::invertColors()
{
    return s_invertColors;
}

void PageItem::setInvertColors(bool invertColors)
{
    s_invertColors = invertColors;
}

const Qt::KeyboardModifiers& PageItem::copyToClipboardModifiers()
{
    return s_copyToClipboardModifiers;
}

void PageItem::setCopyToClipboardModifiers(const Qt::KeyboardModifiers& copyToClipboardModifiers)
{
    s_copyToClipboardModifiers = copyToClipboardModifiers;
}

const Qt::KeyboardModifiers& PageItem::addAnnotationModifiers()
{
    return s_addAnnotationModifiers;
}

void PageItem::setAddAnnotationModifiers(const Qt::KeyboardModifiers& addAnnotationModifiers)
{
    s_addAnnotationModifiers = addAnnotationModifiers;
}

PageItem::PageItem(Model::Page* page, int index, QGraphicsItem* parent) : QGraphicsObject(parent),
    m_page(0),
    m_index(-1),
    m_size(),
    m_links(),
    m_annotations(),
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
    m_render(0)
{
    setAcceptHoverEvents(true);

    m_render = new QFutureWatcher< void >(this);
    connect(m_render, SIGNAL(finished()), SLOT(on_render_finished()));

    connect(this, SIGNAL(imageReady(int,int,qreal,Rotation,bool,QImage)), SLOT(on_imageReady(int,int,qreal,Rotation,bool,QImage)));

    m_page = page;

    m_index = index;
    m_size = m_page->size();

    m_links = m_page->links();
    m_annotations = m_page->annotations();
    m_formFields = m_page->formFields();

    prepareGeometry();
}

PageItem::~PageItem()
{
    m_render->cancel();
    m_render->waitForFinished();

    s_cache.remove(this);

    qDeleteAll(m_links);
    qDeleteAll(m_annotations);
    qDeleteAll(m_formFields);

    delete m_page;
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

    if(s_decoratePages)
    {
        QColor paperColor = s_paperColor;

        if(s_invertColors)
        {
            paperColor.setRgb(~paperColor.rgb());
        }

        painter->fillRect(m_boundingRect, QBrush(paperColor));

        painter->drawPixmap(m_boundingRect.topLeft(), pixmap);

        painter->setPen(QPen(s_invertColors ? Qt::white : Qt::black));
        painter->drawRect(m_boundingRect);
    }
    else
    {
        painter->drawPixmap(m_boundingRect.topLeft(), pixmap);
    }

    // links

    if(s_decorateLinks)
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

    if(s_decorateFormFields)
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

    painter->save();

    painter->setTransform(m_transform, true);

    QColor highlightColor = widget->palette().color(QPalette::Highlight);

    highlightColor.setAlpha(127);

    foreach(QRectF highlight, m_highlights)
    {
        painter->fillRect(highlight.normalized(), QBrush(highlightColor));
    }

    painter->restore();

    // rubber band

    if(!m_rubberBand.isNull())
    {
        QPen pen;
        pen.setColor(s_invertColors ? Qt::white : Qt::black);
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

const QList< QRectF >& PageItem::highlights() const
{
    return m_highlights;
}

void PageItem::setHighlights(const QList< QRectF >& highlights, int duration)
{
    m_highlights = highlights;

    update();

    if(duration > 0)
    {
        QTimer::singleShot(duration, this, SLOT(clearHighlights()));
    }
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
    if(!m_render->isRunning())
    {
        m_render->setFuture(QtConcurrent::run(this, &PageItem::render, m_physicalDpiX, m_physicalDpiY, m_scaleFactor, m_rotation, prefetch));
    }
}

void PageItem::cancelRender()
{
    m_render->cancel();

    m_pixmap = QPixmap();
}

void PageItem::on_render_finished()
{
    update();
}

void PageItem::on_imageReady(int physicalDpiX, int physicalDpiY, qreal scaleFactor, Rotation rotation, bool prefetch, QImage image)
{
    if(m_physicalDpiX != physicalDpiX || m_physicalDpiY != physicalDpiY || !qFuzzyCompare(m_scaleFactor, scaleFactor) || m_rotation != rotation)
    {
        return;
    }

    if(prefetch)
    {
        QPixmap pixmap = QPixmap::fromImage(image);

        int cost = pixmap.width() * pixmap.height() * pixmap.depth() / 8;
        s_cache.insert(this, new QPixmap(pixmap), cost);
    }
    else
    {
        if(!m_render->isCanceled())
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
                if(link->page != -1)
                {
                    setCursor(Qt::PointingHandCursor);
                    QToolTip::showText(event->screenPos(), tr("Go to page %1.").arg(link->page));

                    return;
                }
                else if(!link->url.isNull())
                {
                    setCursor(Qt::PointingHandCursor);
                    QToolTip::showText(event->screenPos(), tr("Open %1.").arg(link->url));

                    return;
                }
            }
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

    if(m_rubberBandMode == ModifiersMode && (event->modifiers() == s_copyToClipboardModifiers || event->modifiers() == s_addAnnotationModifiers) && event->button() == Qt::LeftButton)
    {
        setCursor(Qt::CrossCursor);

        if(event->modifiers() == s_copyToClipboardModifiers)
        {
            m_rubberBandMode = CopyToClipboardMode;
        }
        else if(event->modifiers() == s_addAnnotationModifiers)
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

                if(link->page != -1)
                {
                    emit linkClicked(link->page, link->left, link->top);

                    event->accept();
                    return;
                }
                else if(!link->url.isNull())
                {
                    emit linkClicked(link->url);

                    event->accept();
                    return;
                }
            }
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
        }
    }
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

void PageItem::copyToClipboard(const QPoint& screenPos)
{
    QMenu* menu = new QMenu();

    QAction* copyTextAction = menu->addAction(tr("Copy &text"));
    QAction* copyImageAction = menu->addAction(tr("Copy &image"));
    QAction* saveImageToFileAction = menu->addAction(tr("Save image to &file..."));

    QAction* action = menu->exec(screenPos);

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

    delete menu;
}

void PageItem::addAnnotation(const QPoint& screenPos)
{
    if(m_page->canAddAndRemoveAnnotations())
    {
        QMenu* menu = new QMenu();

        QAction* addTextAction = menu->addAction(tr("Add &text"));
        QAction* addHighlightAction = menu->addAction(tr("Add &highlight"));

        QAction* action = menu->exec(screenPos);

        if(action == addTextAction || action == addHighlightAction)
        {
            QRectF boundary = m_normalizedTransform.inverted().mapRect(m_rubberBand);

            Model::Annotation* annotation = 0;

            if(action == addTextAction)
            {
                boundary.setWidth(24.0 / m_size.width());
                boundary.setHeight(24.0 / m_size.height());

                annotation = m_page->addTextAnnotation(boundary);
            }
            else if(action == addHighlightAction)
            {
                annotation = m_page->addHighlightAnnotation(boundary);
            }

            m_annotations.append(annotation);

            refresh();

            editAnnotation(annotation, screenPos);
        }

        delete menu;
    }
}

void PageItem::removeAnnotation(Model::Annotation* annotation, const QPoint& screenPos)
{
    if(m_page->canAddAndRemoveAnnotations())
    {
        QMenu* menu = new QMenu();

        QAction* removeAnnotationAction = menu->addAction(tr("&Remove annotation"));

        QAction* action = menu->exec(screenPos);

        if(action == removeAnnotationAction)
        {
            m_annotations.removeAll(annotation);
            m_page->removeAnnotation(annotation);

            refresh();
        }

        delete menu;
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

void PageItem::render(int physicalDpiX, int physicalDpiY, qreal scaleFactor, Rotation rotation, bool prefetch)
{
    if(m_render->isCanceled() && !prefetch)
    {
        return;
    }

    QImage image = m_page->render(physicalDpiX * scaleFactor, physicalDpiY * scaleFactor, rotation);

    if(m_render->isCanceled() && !prefetch)
    {
        return;
    }

    if(image.isNull())
    {
        image = QImage(1, 1, QImage::Format_Mono);
        image.fill(Qt::color0);
    }

    if(s_invertColors)
    {
        image.invertPixels();
    }

    emit imageReady(physicalDpiX, physicalDpiY, scaleFactor, rotation, prefetch, image);
}

ThumbnailItem::ThumbnailItem(Model::Page* page, int index, QGraphicsItem* parent) : PageItem(page, index, parent)
{
    setAcceptHoverEvents(false);
}

void ThumbnailItem::mousePressEvent(QGraphicsSceneMouseEvent*)
{
    emit linkClicked(index() + 1);
}

void ThumbnailItem::mouseMoveEvent(QGraphicsSceneMouseEvent*)
{
}

void ThumbnailItem::mouseReleaseEvent(QGraphicsSceneMouseEvent*)
{
}
