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
//#include <QtDebug>

#include "settings.h"
#include "model.h"
#include "rendertask.h"
#include "pageitem.h"

namespace qpdfview
{

Settings* TileItem::s_settings = 0;

QCache< QPair< Model::Page*, QString >, QPixmap > TileItem::s_cache;

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
}

QPair< Model::Page*, QString > TileItem::getPixmapKey()
{
    // calculate unique key for the current pixmap

    PageItem* ppi = parentPage(); // parent PageItem
    Model::Page* page = ppi->m_page;
    QString keystr = QString().sprintf("%d,%d,%f,%d,%d,%d,%d,%d,%d",
                                       ppi->m_renderParam.resolution.resolutionX,
                                       ppi->m_renderParam.resolution.resolutionY,
                                       ppi->m_renderParam.scaleFactor,
                                       ppi->m_renderParam.rotation,
                                       ppi->m_renderParam.invertColors,
                                       qRound(m_rect.x()),
                                       qRound(m_rect.y()),
                                       qRound(m_rect.width()),
                                       qRound(m_rect.height()));

    //qDebug() << "rx,ry,sf,rot,inv,l,t,w,h:" << keystr;

    return qMakePair(page, keystr);
}

QPixmap TileItem::takePixmap()
{
    QPixmap pixmap;

    if(s_cache.contains(getPixmapKey()))
    {
        pixmap = *s_cache.object(getPixmapKey());
    }
    else
    {
        if(!m_pixmap.isNull())
        {
            pixmap = m_pixmap;
            m_pixmap = QPixmap();

            int cost = pixmap.width() * pixmap.height() * pixmap.depth() / 8;
            s_cache.insert(getPixmapKey(), new QPixmap(pixmap), cost);
        }
        else
        {
            startRender();
        }
    }

    return pixmap;
}

void TileItem::refresh(bool keepObsoletePixmaps)
{
    if(keepObsoletePixmaps && s_settings->pageItem().keepObsoletePixmaps())
    {
        if(s_cache.contains(getPixmapKey()))
        {
            m_obsoletePixmap = *s_cache.object(getPixmapKey());
        }
    }
    else
    {
        m_obsoletePixmap = QPixmap();
    }

    m_renderTask->cancel(true);

    m_pixmapError = false;
    m_pixmap = QPixmap();

    //update();
    //s_cache.remove(this);
}

int TileItem::startRender(bool prefetch)
{
    if(m_pixmapError || m_renderTask->isRunning() || (prefetch && s_cache.contains(getPixmapKey())))
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
        s_cache.insert(getPixmapKey(), new QPixmap(QPixmap::fromImage(image)), cost);
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

} // qpdfview
