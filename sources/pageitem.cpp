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

#include "pageitem.h"

QCache< PageItem*, QImage > PageItem::s_cache(32 * 1024 * 1024);

bool PageItem::s_decoratePages = true;
bool PageItem::s_decorateLinks = true;

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

PageItem::PageItem(QMutex* mutex, Poppler::Document* document, int index, QGraphicsItem* parent) : QGraphicsObject(parent),
    m_mutex(0),
    m_page(0),
    m_index(-1),
    m_size(),
    m_links(),
    m_highlights(),
    m_rubberBand(),
    m_physicalDpiX(72),
    m_physicalDpiY(72),
    m_scaleFactor(1.0),
    m_rotation(Poppler::Page::Rotate0),
    m_transform(),
    m_linkTransform(),
    m_boundingRect(),
    m_image1(),
    m_image2(),
    m_prefetch(false),
    m_render(0)
{
    setAcceptHoverEvents(true);

    m_render = new QFutureWatcher< void >(this);
    connect(m_render, SIGNAL(finished()), SLOT(on_render_finished()));

    m_mutex = mutex;
    m_page = document->page(index);

    m_index = index;
    m_size = m_page->pageSizeF();
    m_links = m_page->links();

    prepareGeometry();
}

PageItem::~PageItem()
{
    m_render->cancel();
    m_render->waitForFinished();

    s_cache.remove(this);

    if(m_page != 0)
    {
        delete m_page;
    }

    qDeleteAll(m_links);
}

QRectF PageItem::boundingRect() const
{
    return m_boundingRect;
}

void PageItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    QImage image;

    if(s_cache.contains(this))
    {
        image = *s_cache.object(this);
    }
    else
    {
        if(!m_image1.isNull())
        {
            image = m_image1;

            s_cache.insert(this, new QImage(m_image1), m_image1.byteCount());
            m_image1 = QImage();
        }
        else
        {
            startRender();
        }
    }

    // page

    if(s_decoratePages)
    {
        painter->fillRect(m_boundingRect, QBrush(Qt::white));

        painter->drawImage(m_boundingRect.topLeft(), image);

        painter->setPen(QPen(Qt::black));
        painter->drawRect(m_boundingRect);
    }
    else
    {
        painter->drawImage(m_boundingRect.topLeft(), image);
    }

    // links

    if(s_decorateLinks)
    {
        painter->save();

        painter->setTransform(m_linkTransform, true);
        painter->setPen(QPen(Qt::red));

        foreach(Poppler::Link* link, m_links)
        {
            if(link->linkType() == Poppler::Link::Goto)
            {
                if(!static_cast< Poppler::LinkGoto* >(link)->isExternal())
                {
                    painter->drawRect(link->linkArea().normalized());
                }
            }
            else if(link->linkType() == Poppler::Link::Browse)
            {
                painter->drawRect(link->linkArea().normalized());
            }
        }

        painter->restore();
    }

    // highlights

    painter->save();

    painter->setTransform(m_transform, true);

    QColor highlightColor = QApplication::palette().color(QPalette::Highlight);

    highlightColor.setAlpha(127);

    foreach(QRectF highlight, m_highlights)
    {
        painter->fillRect(highlight, QBrush(highlightColor));
    }

    painter->restore();

    // rubber band

    if(!m_rubberBand.isNull())
    {
        painter->setPen(QPen(Qt::DashLine));
        painter->drawRect(m_rubberBand);
    }
}

int PageItem::index() const
{
    return m_index;
}

QSizeF PageItem::size() const
{
    return m_size;
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
    if(m_physicalDpiX != physicalDpiX || m_physicalDpiY != physicalDpiY)
    {
        cancelRender();

        s_cache.remove(this);
        m_image1 = QImage();

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
    if(m_scaleFactor != scaleFactor)
    {
        cancelRender();

        s_cache.remove(this);
        m_image1 = QImage();

        m_scaleFactor = scaleFactor;

        prepareGeometryChange();
        prepareGeometry();
    }
}

Poppler::Page::Rotation PageItem::rotation() const
{
    return m_rotation;
}

void PageItem::setRotation(Poppler::Page::Rotation rotation)
{
    if(m_rotation != rotation)
    {
        cancelRender();

        s_cache.remove(this);
        m_image1 = QImage();

        m_rotation = rotation;

        prepareGeometryChange();
        prepareGeometry();
    }
}

const QTransform& PageItem::transform() const
{
    return m_transform;
}

const QTransform& PageItem::linkTransform() const
{
    return m_linkTransform;
}

bool PageItem::isPrefetching() const
{
    return m_prefetch;
}

void PageItem::startRender()
{
    if(!m_render->isRunning())
    {
        m_render->setFuture(QtConcurrent::run(this, &PageItem::render, m_physicalDpiX, m_physicalDpiY, m_scaleFactor, m_rotation));
    }
}

void PageItem::cancelRender()
{
    m_render->cancel();
}

void PageItem::prefetch()
{
    if(!s_cache.contains(this) && m_image1.isNull())
    {
        m_prefetch = true;

        startRender();
    }
}

void PageItem::on_render_finished()
{
    if(!m_render->isCanceled())
    {
        m_image1 = m_image2;
    }

    if(!m_render->isRunning())
    {
        m_image2 = QImage();
    }

    m_prefetch = false;

    update();
}

void PageItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    Q_UNUSED(event);
}

void PageItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    if(event->modifiers() == Qt::NoModifier)
    {
        QApplication::restoreOverrideCursor();

        foreach(Poppler::Link* link, m_links)
        {
            if(m_linkTransform.mapRect(link->linkArea().normalized()).contains(event->pos()))
            {
                if(link->linkType() == Poppler::Link::Goto)
                {
                    if(!static_cast< Poppler::LinkGoto* >(link)->isExternal())
                    {
                        QApplication::setOverrideCursor(Qt::PointingHandCursor);
                        QToolTip::showText(event->screenPos(), tr("Go to page %1.").arg(static_cast< Poppler::LinkGoto* >(link)->destination().pageNumber()));

                        return;
                    }
                }
                else if(link->linkType() == Poppler::Link::Browse)
                {
                    QApplication::setOverrideCursor(Qt::PointingHandCursor);
                    QToolTip::showText(event->screenPos(), tr("Open %1.").arg(static_cast< Poppler::LinkBrowse* >(link)->url()));

                    return;
                }
            }
        }

        QToolTip::hideText();
    }
}

void PageItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    Q_UNUSED(event);
}

void PageItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if(event->modifiers() == Qt::NoModifier && event->button() == Qt::LeftButton)
    {
        foreach(Poppler::Link* link, m_links)
        {
            if(m_linkTransform.mapRect(link->linkArea().normalized()).contains(event->pos()))
            {
                if(link->linkType() == Poppler::Link::Goto)
                {
                    if(!static_cast< Poppler::LinkGoto* >(link)->isExternal())
                    {
                        event->accept();
                        return;
                    }
                }
                else if(link->linkType() == Poppler::Link::Browse)
                {
                    event->accept();
                    return;
                }
            }
        }

        event->ignore();
    }
    else if(event->modifiers() == Qt::ShiftModifier && event->button() == Qt::LeftButton)
    {
        m_rubberBand = QRectF(event->pos(), QSizeF());

        update();

        event->accept();
    }
    else
    {
        event->ignore();
    }
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
    foreach(Poppler::Link* link, m_links)
    {
        if(m_linkTransform.mapRect(link->linkArea().normalized()).contains(event->pos()))
        {
            if(link->linkType() == Poppler::Link::Goto)
            {
                Poppler::LinkGoto* linkGoto = static_cast< Poppler::LinkGoto* >(link);

                if(!linkGoto->isExternal())
                {
                    int page = linkGoto->destination().pageNumber();
                    qreal left = linkGoto->destination().isChangeLeft() ? linkGoto->destination().left() : 0.0;
                    qreal top = linkGoto->destination().isChangeTop() ? linkGoto->destination().top() : 0.0;

                    emit linkClicked(page, left, top);

                    return;
                }
            }
            else if(link->linkType() == Poppler::Link::Browse)
            {
                emit linkClicked(static_cast< Poppler::LinkBrowse* >(link)->url());

                return;
            }
        }
    }

    if(!m_rubberBand.isNull())
    {
        QMenu* menu = new QMenu();

        QAction* copyTextAction = menu->addAction(tr("Copy &text"));
        QAction* copyImageAction = menu->addAction(tr("Copy &image"));

        QAction* action = menu->exec(event->screenPos());

        if(action == copyTextAction)
        {
            QString text;

            m_mutex->lock();

            text = m_page->text(m_transform.inverted().mapRect(m_rubberBand));

            m_mutex->unlock();

            if(!text.isEmpty())
            {
                QApplication::clipboard()->setText(text);
            }
        }
        else if(action == copyImageAction)
        {
            QImage image;

            m_mutex->lock();

            switch(m_rotation)
            {
            case Poppler::Page::Rotate0:
            case Poppler::Page::Rotate90:
                image = m_page->renderToImage(m_scaleFactor * m_physicalDpiX, m_scaleFactor * m_physicalDpiY, m_rubberBand.x(), m_rubberBand.y(), m_rubberBand.width(), m_rubberBand.height(), m_rotation);
                break;
            case Poppler::Page::Rotate180:
            case Poppler::Page::Rotate270:
                image = m_page->renderToImage(m_scaleFactor * m_physicalDpiY, m_scaleFactor * m_physicalDpiX, m_rubberBand.x(), m_rubberBand.y(), m_rubberBand.width(), m_rubberBand.height(), m_rotation);
                break;
            }

            m_mutex->lock();

            if(!image.isNull())
            {
                QApplication::clipboard()->setImage(image);
            }
        }

        delete menu;

        m_rubberBand = QRectF();

        update();
    }
}

void PageItem::prepareGeometry()
{
    m_transform.reset();
    m_linkTransform.reset();

    switch(m_rotation)
    {
    case Poppler::Page::Rotate0:
        break;
    case Poppler::Page::Rotate90:
        m_transform.rotate(90.0);
        m_linkTransform.rotate(90.0);
        break;
    case Poppler::Page::Rotate180:
        m_transform.rotate(180.0);
        m_linkTransform.rotate(180.0);
        break;
    case Poppler::Page::Rotate270:
        m_transform.rotate(270.0);
        m_linkTransform.rotate(270.0);
        break;
    }

    switch(m_rotation)
    {
    case Poppler::Page::Rotate0:
    case Poppler::Page::Rotate90:
        m_transform.scale(m_scaleFactor * m_physicalDpiX / 72.0, m_scaleFactor * m_physicalDpiY / 72.0);
        m_linkTransform.scale(m_scaleFactor * m_physicalDpiX / 72.0 * m_size.width(), m_scaleFactor * m_physicalDpiY / 72.0 * m_size.height());
        break;
    case Poppler::Page::Rotate180:
    case Poppler::Page::Rotate270:
        m_transform.scale(m_scaleFactor * m_physicalDpiY / 72.0, m_scaleFactor * m_physicalDpiX / 72.0);
        m_linkTransform.scale(m_scaleFactor * m_physicalDpiY / 72.0 * m_size.width(), m_scaleFactor * m_physicalDpiX / 72.0 * m_size.height());
        break;
    }

    m_boundingRect = m_transform.mapRect(QRectF(QPointF(), m_size));
}

void PageItem::render(int physicalDpiX, int physicalDpiY, qreal scaleFactor, Poppler::Page::Rotation rotation)
{
    QMutexLocker mutexLocker(m_mutex);

    if(m_render->isCanceled())
    {
        return;
    }

    switch(rotation)
    {
    case Poppler::Page::Rotate0:
    case Poppler::Page::Rotate90:
        m_image2 = m_page->renderToImage(scaleFactor * physicalDpiX, scaleFactor * physicalDpiY, -1, -1, -1, -1, rotation);
        break;
    case Poppler::Page::Rotate180:
    case Poppler::Page::Rotate270:
        m_image2 = m_page->renderToImage(scaleFactor * physicalDpiY, scaleFactor * physicalDpiX, -1, -1, -1, -1, rotation);
        break;
    }
}

ThumbnailItem::ThumbnailItem(QMutex* mutex, Poppler::Document* document, int index, QGraphicsItem* parent) : PageItem(mutex, document, index, parent)
{
}

void ThumbnailItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    Q_UNUSED(event);
}

void ThumbnailItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    emit pageClicked(index() + 1);

    event->accept();
}

void ThumbnailItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    Q_UNUSED(event);
}

void ThumbnailItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    Q_UNUSED(event);
}