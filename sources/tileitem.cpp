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
#include "pageitem.h"

namespace qpdfview
{

namespace
{

inline int cacheCost(const QPixmap& pixmap)
{
    return qMax(1, pixmap.width() * pixmap.height() * pixmap.depth() / 8 / 1024);
}

} // anonymous

Settings* TileItem::s_settings = 0;

QCache< TileItem::CacheKey, TileItem::CacheObject > TileItem::s_cache;

TileItem::TileItem(PageItem* page) : RenderTaskParent(),
    m_page(page),
    m_rect(),
    m_cropRect(),
    m_pixmapError(false),
    m_pixmap(),
    m_obsoletePixmap(),
    m_deleteAfterRender(false),
    m_renderTask(m_page->m_page, this)
{
    if(s_settings == 0)
    {
        s_settings = Settings::instance();
    }

    s_cache.setMaxCost(s_settings->pageItem().cacheSize());
}

TileItem::~TileItem()
{
    m_renderTask.cancel(true);
    m_renderTask.wait();
}

void TileItem::setCropRect(const QRectF& cropRect)
{
    if(!m_page->m_renderParam.trimMargins())
    {
        return;
    }

    if(m_cropRect.isNull() && !cropRect.isNull())
    {
        m_cropRect = cropRect;

        m_page->updateCropRect();
    }
}

void TileItem::dropCachedPixmaps(PageItem* page)
{
    foreach(const CacheKey& key, s_cache.keys())
    {
        if(key.first == page)
        {
            s_cache.remove(key);
        }
    }
}

bool TileItem::paint(QPainter* painter, QPointF topLeft)
{
    const QPixmap& pixmap = takePixmap();

    if(!pixmap.isNull())
    {
        // pixmap

        painter->drawPixmap(m_rect.topLeft() + topLeft, pixmap);

        return true;
    }
    else if(!m_obsoletePixmap.isNull())
    {
        // obsolete pixmap

        painter->drawPixmap(QRectF(m_rect).translated(topLeft), m_obsoletePixmap, QRectF());

        return false;
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

            return false;
        }
        else
        {
            // error icon

            s_settings->pageItem().errorIcon().paint(painter, iconRect);

            return true;
        }
    }
}

void TileItem::refresh(bool keepObsoletePixmaps)
{
    if(keepObsoletePixmaps && s_settings->pageItem().keepObsoletePixmaps())
    {
        if(const CacheObject* object = s_cache.object(cacheKey()))
        {
            m_obsoletePixmap = object->first;
        }
    }
    else
    {
        m_obsoletePixmap = QPixmap();
    }

    if(!keepObsoletePixmaps)
    {
        m_cropRect = QRectF();
    }

    m_renderTask.cancel(true);

    m_pixmapError = false;
    m_pixmap = QPixmap();
}

int TileItem::startRender(bool prefetch)
{
    m_page->startLoadInteractiveElements();

    if(m_pixmapError || m_renderTask.isRunning() || (prefetch && s_cache.contains(cacheKey())))
    {
        return 0;
    }

    m_renderTask.start(m_page->m_renderParam, m_rect, prefetch);

    return 1;
}

void TileItem::cancelRender()
{
    m_renderTask.cancel();

    m_pixmap = QPixmap();
    m_obsoletePixmap = QPixmap();
}

void TileItem::deleteAfterRender()
{
    if(!m_renderTask.isRunning())
    {
        m_renderTask.deleteParentLater();
    }
    else
    {
        m_renderTask.cancel(true);

        m_deleteAfterRender = true;
    }
}

void TileItem::on_finished(const RenderParam& renderParam,
                           const QRect& rect, bool prefetch,
                           const QImage& image, const QRectF& cropRect)
{
    if(m_page->m_renderParam != renderParam || m_rect != rect)
    {
        on_finishedOrCanceled();
        return;
    }

    m_obsoletePixmap = QPixmap();

    if(image.isNull())
    {
        m_pixmapError = true;

        on_finishedOrCanceled();
        return;
    }

    if(prefetch && !m_renderTask.wasCanceledForcibly())
    {
        const QPixmap pixmap = QPixmap::fromImage(image);

        s_cache.insert(cacheKey(), new CacheObject(pixmap, cropRect), cacheCost(pixmap));

        setCropRect(cropRect);
    }
    else if(!m_renderTask.wasCanceled())
    {
        m_pixmap = QPixmap::fromImage(image);

        setCropRect(cropRect);
    }

    on_finishedOrCanceled();
}

void TileItem::on_canceled()
{
    on_finishedOrCanceled();
}

void TileItem::on_finishedOrCanceled()
{
    if(m_deleteAfterRender)
    {
        m_renderTask.deleteParentLater();
    }
    else if(!m_page->useTiling() || m_page->m_exposedTileItems.contains(this))
    {
        m_page->update();
    }
}

inline TileItem::CacheKey TileItem::cacheKey() const
{
    QByteArray key;
    QDataStream stream(&key, QIODevice::WriteOnly);

    stream << m_page->m_renderParam << m_rect;

    return qMakePair(m_page, key);
}

QPixmap TileItem::takePixmap()
{
    const CacheKey key = cacheKey();

    if(const CacheObject* object = s_cache.object(key))
    {
        m_obsoletePixmap = QPixmap();

        setCropRect(object->second);
        return object->first;
    }

    QPixmap pixmap;

    if(!m_pixmap.isNull())
    {
        s_cache.insert(key, new CacheObject(m_pixmap, m_cropRect), cacheCost(m_pixmap));

        pixmap = m_pixmap;
    }
    else
    {
        startRender();
    }

    return pixmap;
}

} // qpdfview
