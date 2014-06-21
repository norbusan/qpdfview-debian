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
#include "model.h"
#include "rendertask.h"
#include "pageitem.h"

namespace
{

using namespace qpdfview;

RenderResolution effectiveResolution(int resolutionX, int resolutionY,
                                     Settings* settings, qreal devicePixelRatio)
{
    RenderResolution resolution(resolutionX, resolutionY);

#if QT_VERSION >= QT_VERSION_CHECK(5,1,0)

    if(settings->pageItem().useDevicePixelRatio())
    {
        resolution.devicePixelRatio = devicePixelRatio;
    }

#else

    Q_UNUSED(settings);
    Q_UNUSED(devicePixelRatio);

#endif // QT_VERSION

    return resolution;
}

} // anonymous

namespace qpdfview
{

Settings* TileItem::s_settings = 0;

QCache< TileItem*, QPixmap > TileItem::s_cache;

TileItem::TileItem(QGraphicsItem* parent) : QGraphicsObject(parent),
    m_tile(),
    m_boundingRect(),
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
    connect(m_renderTask, SIGNAL(imageReady(RenderResolution,qreal,Rotation,bool,QRect,bool,QImage)), SLOT(on_renderTask_imageReady(RenderResolution,qreal,Rotation,bool,QRect,bool,QImage)));
}

TileItem::~TileItem()
{
    m_renderTask->cancel(true);
    m_renderTask->wait();

    s_cache.remove(this);
}

QRectF TileItem::boundingRect() const
{
    return m_boundingRect;
}

void TileItem::setBoundingRect(const QRectF& boundingRect)
{
    prepareGeometryChange();

    m_boundingRect = boundingRect;
}

void TileItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    const QPixmap& pixmap = takePixmap();

    if(!pixmap.isNull())
    {
        // pixmap

        painter->drawPixmap(m_boundingRect.topLeft(), pixmap);
    }
    else if(!m_obsoletePixmap.isNull())
    {
        // obsolete pixmap

        painter->drawPixmap(m_boundingRect, m_obsoletePixmap, QRectF());
    }
    else
    {
        const qreal extent = qMin(0.1 * m_boundingRect.width(), 0.1 * m_boundingRect.height());
        const QRectF rect(m_boundingRect.left() + 0.01 * m_boundingRect.width(), m_boundingRect.top() + 0.01 * m_boundingRect.height(), extent, extent);

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

void TileItem::dropObsoletePixmaps()
{
    m_obsoletePixmap = QPixmap();
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

    update();
}

int TileItem::startRender(bool prefetch)
{
    if(m_pixmapError || m_renderTask->isRunning() || (prefetch && s_cache.contains(this)))
    {
        return 0;
    }

    const PageItem* parentPage = qobject_cast< PageItem* >(parentObject());

    const RenderResolution& effectiveResolution = ::effectiveResolution(parentPage->m_resolutionX, parentPage->m_resolutionY,
                                                                        s_settings, parentPage->m_devicePixelRatio);

    m_renderTask->start(parentPage->m_page,
                        effectiveResolution,
                        parentPage->m_scaleFactor, parentPage->m_rotation, parentPage->m_invertColors,
                        m_tile, prefetch);

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
        setVisible(false);

        QTimer::singleShot(0, this, SLOT(deleteAfterRender()));
    }
}

void TileItem::on_renderTask_finished()
{
    update();
}

void TileItem::on_renderTask_imageReady(const RenderResolution& resolution,
                                        qreal scaleFactor, Rotation rotation, bool invertColors,
                                        const QRect& tile, bool prefetch,
                                        QImage image)
{
    const PageItem* parentPage = qobject_cast< PageItem* >(parentObject());

    const RenderResolution& effectiveResolution = ::effectiveResolution(parentPage->m_resolutionX, parentPage->m_resolutionY,
                                                                        s_settings, parentPage->m_devicePixelRatio);

    if(effectiveResolution != resolution
            || !qFuzzyCompare(parentPage->m_scaleFactor, scaleFactor) || parentPage->m_rotation != rotation || parentPage->m_invertColors != invertColors
            || m_tile != tile)
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
