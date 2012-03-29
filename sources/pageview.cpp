/*

Copyright 2012 Adam Reichold

This file is part of qpdfview.

qpdfview is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

qpdfview is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with qpdfview.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "document.h"
#include "pageview.h"

PageView::PageView(Document *document, int index, QGraphicsItem *parent) : QGraphicsObject(parent),
    m_document(document),
    m_index(index),
    m_scaleFactor(1.0),
    m_rotation(RotateBy0),
    m_highlightAll(false),
    m_links(),
    m_searchResults(),
    m_highlight(),
    m_rubberBand(),
    m_size(),
    m_pageTransform(),
    m_linkTransform(),
    m_highlightTransform(),
    m_render()
{
    m_links = m_document->links(m_index);

    m_searchResults = m_document->searchResults(m_index);

    m_size = m_document->size(m_index);

    this->prepareTransforms();

    connect(m_document, SIGNAL(pageRendered(int)), this, SLOT(slotPageRendered(int)));
    connect(m_document, SIGNAL(pageSearched(int)), this, SLOT(slotPageSearched(int)));

    this->setAcceptHoverEvents(true);
}

PageView::~PageView()
{
    if(m_render.isRunning())
    {
        m_render.waitForFinished();
    }
}

int PageView::index() const
{
    return m_index;
}

qreal PageView::scaleFactor() const
{
    return m_scaleFactor;
}

void PageView::setScaleFactor(qreal scaleFactor)
{
    if(m_scaleFactor != scaleFactor)
    {
        m_scaleFactor = scaleFactor;

        this->prepareTransforms();

        update(boundingRect());

        emit scaleFactorChanged(m_scaleFactor);
    }
}

Rotation PageView::rotation() const
{
    return m_rotation;
}

void PageView::setRotation(Rotation rotation)
{
    if(m_rotation != rotation)
    {
        m_rotation = rotation;

        this->prepareTransforms();

        update(boundingRect());

        emit rotationChanged(m_rotation);
    }
}

bool PageView::highlightAll() const
{
    return m_highlightAll;
}

void PageView::setHighlightAll(bool highlightAll)
{
    if(m_highlightAll != highlightAll)
    {
        m_highlightAll = highlightAll;

        emit highlightAllChanged(m_highlightAll);

        update(boundingRect());
    }
}

const QList<Link> &PageView::links() const
{
    return m_links;
}

const QList<QRectF> &PageView::searchResults() const
{
    return m_searchResults;
}

const QSizeF &PageView::size() const
{
    return m_size;
}

const QTransform &PageView::pageTransform() const
{
    return m_pageTransform;
}

const QTransform &PageView::linkTransform() const
{
    return m_linkTransform;
}

const QTransform &PageView::highlightTransform() const
{
    return m_highlightTransform;
}

QRectF PageView::boundingRect() const
{
    QRectF result;

    switch(m_rotation)
    {
    case RotateBy0:
    case RotateBy180:
        result = QRectF(0.0, 0.0, qCeil(m_scaleFactor * m_size.width()), qCeil(m_scaleFactor * m_size.height()));

        break;
    case RotateBy90:
    case RotateBy270:
        result = QRectF(0.0, 0.0, qCeil(m_scaleFactor * m_size.height()), qCeil(m_scaleFactor * m_size.width()));

        break;
    }

    return result;
}

void PageView::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    // draw page

    painter->fillRect(boundingRect(), QBrush(Qt::white));

    QImage image = m_document->pullPage(m_index, m_scaleFactor);

    if(image.isNull())
    {
        if(!m_render.isRunning())
        {
            m_render = QtConcurrent::run(this, &PageView::render, false);
        }
    }
    else
    {
        painter->setTransform(m_pageTransform, true);
        painter->drawImage(QPointF(0.0, 0.0), image);
        painter->setTransform(m_pageTransform.inverted(), true);
    }

    painter->setPen(QPen(Qt::black));
    painter->drawRect(boundingRect());

    // draw links

    painter->setPen(QPen(QColor(255,0,0,127)));
    painter->setTransform(m_linkTransform, true);

    foreach(Link link, m_links)
    {
        painter->drawRect(link.area);
    }

    painter->setTransform(m_linkTransform.inverted(), true);

    // draw search result highlights

    painter->setTransform(m_highlightTransform, true);

    if(m_highlightAll)
    {
        foreach(QRectF searchResult, m_searchResults)
        {
            painter->fillRect(searchResult.adjusted(-1.0, -1.0, 1.0, 1.0), QBrush(QColor(0,255,0,127)));
        }
    }

    // draw highlight

    if(!m_highlight.isNull())
    {
        painter->fillRect(m_highlight, QBrush(QColor(0,0,255,127)));
    }

    painter->setTransform(m_highlightTransform.inverted(), true);

    // draw rubber band

    if(!m_rubberBand.isNull())
    {
        QPen pen;
        pen.setColor(Qt::black);
        pen.setStyle(Qt::DashLine);
        painter->setPen(pen);

        painter->drawRect(m_rubberBand);
    }
}

void PageView::prefetch()
{
    QImage image = m_document->pullPage(m_index, m_scaleFactor);

    if(image.isNull())
    {
        if(!m_render.isRunning())
        {
            m_render = QtConcurrent::run(this, &PageView::render, true);
        }
    }
}

void PageView::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        foreach(Link link, m_links)
        {
            if(m_linkTransform.mapRect(link.area).contains(event->pos()))
            {
                if(!link.url.isEmpty())
                {
                    emit linkLeftClicked(link.url);
                }
                else
                {
                    emit linkLeftClicked(link.page, link.top);
                }

                return;
            }
        }

        m_rubberBand = QRectF(event->pos(), QSizeF());
    }
    else if(event->button() == Qt::MiddleButton)
    {
        foreach(Link link, m_links)
        {
            if(m_linkTransform.mapRect(link.area).contains(event->pos()))
            {
                if(!link.url.isEmpty())
                {
                    emit linkMiddleClicked(link.url);
                }
                else
                {
                    emit linkMiddleClicked(link.page, link.top);
                }

                return;
            }
        }

        if(!m_highlight.isNull())
        {
            m_highlight = QRectF();

            update(boundingRect());
        }
    }
}

void PageView::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if(!m_rubberBand.isNull())
    {
        m_rubberBand.setBottomRight(event->pos());

        update(boundingRect());
    }
}

void PageView::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if(!m_rubberBand.isNull())
    {
        m_rubberBand.setBottomRight(event->pos());

        m_highlight = m_highlightTransform.inverted().mapRect(m_rubberBand).adjusted(-1.0, -1.0, 1.0, 1.0);

        m_rubberBand = QRectF();

        QString text = m_document->text(m_index, m_highlight);

        if(!text.isEmpty())
        {
            QApplication::clipboard()->setText(text);
        }

        update(boundingRect());
    }
}

void PageView::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    QApplication::restoreOverrideCursor();

    foreach(Link link, m_links)
    {
        if(m_linkTransform.mapRect(link.area).contains(event->pos()))
        {
            QApplication::setOverrideCursor(Qt::PointingHandCursor);

            if(!link.url.isEmpty())
            {
                QToolTip::showText(event->screenPos(), tr("Open URL \"%1\".").arg(link.url));
            }
            else
            {
                QToolTip::showText(event->screenPos(), tr("Go to page %1.").arg(link.page));
            }

            return;
        }
    }

    QToolTip::hideText();
}

void PageView::slotPageRendered(int index)
{
    if(m_index == index)
    {
        update(boundingRect());
    }
}

void PageView::slotPageSearched(int index)
{
    if(m_index == index)
    {
        m_searchResults = m_document->searchResults(index);

        update(boundingRect());
    }
}

void PageView::prepareTransforms()
{
    this->prepareGeometryChange();

    switch(m_rotation)
    {
    case RotateBy0:
        m_pageTransform = QTransform(1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
        m_linkTransform = QTransform(m_scaleFactor * m_size.width(), 0.0, 0.0, m_scaleFactor * m_size.height(), 0.0, 0.0);
        m_highlightTransform = QTransform(m_scaleFactor, 0.0, 0.0, m_scaleFactor, 0.0, 0.0);

        break;
    case RotateBy90:
        m_pageTransform = QTransform(0.0, 1.0, -1.0, 0.0, m_scaleFactor * m_size.height(), 0.0);
        m_linkTransform = QTransform(0.0, m_scaleFactor * m_size.width(), -m_scaleFactor * m_size.height(), 0.0, m_scaleFactor * m_size.height(), 0.0);
        m_highlightTransform = QTransform(0.0, m_scaleFactor, -m_scaleFactor, 0.0, m_scaleFactor * m_size.height(), 0.0);

        break;
    case RotateBy180:
        m_pageTransform = QTransform(-1.0, 0.0, 0.0, -1.0, m_scaleFactor * m_size.width(), m_scaleFactor * m_size.height());
        m_linkTransform = QTransform(-m_scaleFactor * m_size.width(), 0.0, 0.0, -m_scaleFactor * m_size.height(), m_scaleFactor * m_size.width(), m_scaleFactor * m_size.height());
        m_highlightTransform = QTransform(-m_scaleFactor, 0.0, 0.0, -m_scaleFactor, m_scaleFactor * m_size.width(), m_scaleFactor * m_size.height());

        break;
    case RotateBy270:
        m_pageTransform = QTransform(0.0, -1.0, 1.0, 0.0, 0.0, m_scaleFactor * m_size.width());
        m_linkTransform = QTransform(0.0, -m_scaleFactor * m_size.width(), m_scaleFactor * m_size.height(), 0.0, 0.0, m_scaleFactor * m_size.width());
        m_highlightTransform = QTransform(0.0, -m_scaleFactor, m_scaleFactor, 0.0, 0.0, m_scaleFactor * m_size.width());

        break;
    }
}

void PageView::render(bool prefetch)
{
    bool visible = false;

    QRectF pageRect = boundingRect().translated(pos());

    foreach(QGraphicsView *view, scene()->views())
    {
       QRectF viewRect = view->mapToScene(view->rect()).boundingRect();

       visible = visible || viewRect.intersects(pageRect);
    }

    if(visible || prefetch)
    {
       m_document->pushPage(m_index, m_scaleFactor);
    }
}
