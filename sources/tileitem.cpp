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

#include "tileitem.h"

#include <QPainter>
#include <QTimer>

#include "settings.h"
#include "rendertask.h"
#include "pageitem.h"

namespace qpdfview
{

Settings* TileItem::s_settings = 0;

QCache< TileItem*, QPixmap > TileItem::s_cache;

TileItem::TileItem(QObject* parent) : QObject(parent),
    m_rect(),
    m_pixmapError(false),
    m_pixmap(),
    m_obsoletePixmap(),
    m_renderTask(0)
{
    if(s_settings == 0)
    {
        s_settings = Settings::instance();
    }

    s_cache.setMaxCost(s_settings->pageItem().cacheSize());

    m_renderTask = new RenderTask(this);

    connect(m_renderTask, SIGNAL(finished()), SLOT(on_renderTask_finished()));
    connect(m_renderTask, SIGNAL(imageReady(RenderParam,QRect,bool,QImage)), SLOT(on_renderTask_imageReady(RenderParam,QRect,bool,QImage)));
}

TileItem::~TileItem()
{
    m_renderTask->cancel(true);
    m_renderTask->wait();

    s_cache.remove(this);
}

void TileItem::paint(QPainter* painter, const QPointF& topLeft)
{
    const QPixmap& pixmap = takePixmap();

    if(!pixmap.isNull())
    {
        // pixmap

        painter->drawPixmap(m_rect.topLeft() + topLeft, pixmap);
    }
    else if(!m_obsoletePixmap.isNull())
    {
        // obsolete pixmap

        painter->drawPixmap(QRectF(m_rect).translated(topLeft), m_obsoletePixmap, QRectF());
    }
    else
    {
        const qreal iconExtent = qMin(0.1 * m_rect.width(), 0.1 * m_rect.height());
        const QRect iconRect(topLeft.x() + m_rect.left() + 0.01 * m_rect.width(),
                             topLeft.y() + m_rect.top() + 0.01 * m_rect.height(),
                             iconExtent, iconExtent);

        if(!m_pixmapError)
        {
            // progress icon

            s_settings->pageItem().progressIcon().paint(painter, iconRect);
        }
        else
        {
            // error icon

            s_settings->pageItem().errorIcon().paint(painter, iconRect);
        }
    }
}

void TileItem::refresh(bool keepObsoletePixmaps)
{
    if(keepObsoletePixmaps && s_settings->pageItem().keepObsoletePixmaps())
    {
        if(s_cache.contains(this))
        {
            m_obsoletePixmap = *s_cache.object(this);
        }
    }
    else
    {
        m_obsoletePixmap = QPixmap();
    }

    m_renderTask->cancel(true);

    m_pixmapError = false;
    m_pixmap = QPixmap();
    s_cache.remove(this);
}

int TileItem::startRender(bool prefetch)
{
    if(m_pixmapError || m_renderTask->isRunning() || (prefetch && s_cache.contains(this)))
    {
        return 0;
    }

    m_renderTask->start(parentPage()->m_page,
                        parentPage()->m_renderParam,
                        m_rect, prefetch);

    return 1;
}

void TileItem::cancelRender()
{
    m_renderTask->cancel();

    m_pixmap = QPixmap();
    m_obsoletePixmap = QPixmap();
}

void TileItem::deleteAfterRender()
{
    cancelRender();

    if(!m_renderTask->isRunning())
    {
        delete this;
    }
    else
    {
        QTimer::singleShot(0, this, SLOT(deleteAfterRender()));
    }
}

void TileItem::on_renderTask_finished()
{
    parentPage()->update();
}

void TileItem::on_renderTask_imageReady(const RenderParam& renderParam,
                                        const QRect& rect, bool prefetch,
                                        QImage image)
{
    if(parentPage()->m_renderParam != renderParam || m_rect != rect)
    {
        return;
    }

    m_obsoletePixmap = QPixmap();

    if(image.isNull())
    {
        m_pixmapError = true;

        return;
    }

    if(prefetch && !m_renderTask->wasCanceledForcibly())
    {
        int cost = image.width() * image.height() * image.depth() / 8;
        s_cache.insert(this, new QPixmap(QPixmap::fromImage(image)), cost);
    }
    else if(!m_renderTask->wasCanceled())
    {
        m_pixmap = QPixmap::fromImage(image);
    }
}

PageItem* TileItem::parentPage() const
{
    return qobject_cast< PageItem* >(parent());
}

QPixmap TileItem::takePixmap()
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

} // qpdfview
