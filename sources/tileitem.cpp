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

#include "settings.h"
#include "model.h"
#include "rendertask.h"
#include "pageitem.h"

namespace
{

qreal effectiveDevicePixelRatio(qpdfview::Settings* settings, qreal devicePixelRatio)
{
#if QT_VERSION >= QT_VERSION_CHECK(5,1,0)

    return settings->pageItem().useDevicePixelRatio() ? devicePixelRatio : 1.0;

#else

    return 1.0;

#endif // QT_VERSION
}

} // anonymous

namespace qpdfview
{

Settings* TileItem::s_settings = 0;

QCache< TileItem*, QPixmap > TileItem::s_cache;

TileItem::TileItem(QGraphicsItem* parent) : QGraphicsObject(parent),
    m_tile(),
    m_pixmapError(false),
    m_pixmap(),
    m_renderTask(0)
{
    if(s_settings == 0)
    {
        s_settings = Settings::instance();
    }

    s_cache.setMaxCost(s_settings->pageItem().cacheSize());

    m_renderTask = new RenderTask(this);

    connect(m_renderTask, SIGNAL(finished()), SLOT(on_renderTask_finished()));
    connect(m_renderTask, SIGNAL(pixmapReady(int,int,qreal,qreal,Rotation,bool,QRect,bool,QPixmap)), SLOT(on_renderTask_pixmapReady(int,int,qreal,qreal,Rotation,bool,QRect,bool,QPixmap)));
}

TileItem::~TileItem()
{
    m_renderTask->cancel(true);
    m_renderTask->wait();

    s_cache.remove(this);
}

QRectF TileItem::boundingRect() const
{
    return m_tile;
}

void TileItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    const QPixmap& pixmap = cachedPixmap();

    if(!pixmap.isNull())
    {
        // pixmap

        painter->drawPixmap(m_tile.topLeft(), pixmap);
    }
    else
    {
        const qreal extent = qMin(0.1 * m_tile.width(), 0.1 * m_tile.height());
        const QRectF rect(m_tile.left() + 0.01 * m_tile.width(), m_tile.top() + 0.01 * m_tile.height(), extent, extent);

        if(!m_pixmapError)
        {
            // progress icon

            s_settings->pageItem().progressIcon().paint(painter, rect.toRect());
        }
        else
        {
            // error icon

            s_settings->pageItem().errorIcon().paint(painter, rect.toRect());
        }
    }

}

void TileItem::setTile(const QRectF& tile)
{
    if(m_tile != tile)
    {
        refresh();

        prepareGeometryChange();

        m_tile = tile;
    }
}

void TileItem::refresh()
{
    m_renderTask->cancel();

    m_pixmapError = false;
    m_pixmap = QPixmap();
    s_cache.remove(this);

    update();
}

void TileItem::startRender(bool prefetch)
{
    if(prefetch && s_cache.contains(this))
    {
        return;
    }

    if(!m_pixmapError && !m_renderTask->isRunning())
    {
        PageItem* parent = qobject_cast< PageItem* >(parentObject());

        m_renderTask->start(parent->m_page,
                            parent->m_resolutionX, parent->m_resolutionY, effectiveDevicePixelRatio(s_settings, parent->m_devicePixelRatio),
                            parent->m_scaleFactor, parent->m_rotation, parent->m_invertColors,
                            m_tile.toRect(), prefetch);
    }
}

void TileItem::cancelRender()
{
    m_renderTask->cancel();

    m_pixmap = QPixmap();
}

void TileItem::on_renderTask_finished()
{
    update();
}

void TileItem::on_renderTask_pixmapReady(int resolutionX, int resolutionY, qreal devicePixelRatio,
                                         qreal scaleFactor, Rotation rotation, bool invertColors,
                                         const QRect& tile, bool prefetch,
                                         QPixmap pixmap)
{
    PageItem* parent = qobject_cast< PageItem* >(parentObject());

    if(parent->m_resolutionX != resolutionX || parent->m_resolutionY != resolutionY || !qFuzzyCompare(effectiveDevicePixelRatio(s_settings, parent->m_devicePixelRatio), devicePixelRatio)
            || !qFuzzyCompare(parent->m_scaleFactor, scaleFactor) || parent->m_rotation != rotation || parent->m_invertColors != invertColors
            || m_tile != tile)
    {
        return;
    }

    if(pixmap.isNull())
    {
        m_pixmapError = true;

        return;
    }

    if(prefetch && !m_renderTask->forceCancelleation())
    {
        int cost = pixmap.width() * pixmap.height() * pixmap.depth() / 8;
        s_cache.insert(this, new QPixmap(pixmap), cost);
    }
    else if(!m_renderTask->wasCanceled())
    {
        m_pixmap = pixmap;
    }
}

QPixmap TileItem::cachedPixmap()
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
