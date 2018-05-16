/*

Copyright 2014 Adam Reichold

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

#ifndef TILEITEM_H
#define TILEITEM_H

#include <QCache>
#include <QObject>
#include <QPixmap>

#include "rendertask.h"

namespace qpdfview
{

class PageItem;

class TileItem : public RenderTaskParent
{
public:
    TileItem(PageItem* page);
    ~TileItem();

    const QRect& rect() const { return m_rect; }
    void setRect(QRect rect) { m_rect = rect; }

    const QRectF& cropRect() const { return m_cropRect; }
    void resetCropRect() { m_cropRect = QRectF(); }
    void setCropRect(const QRectF& cropRect);

    void dropPixmap() { m_pixmap = QPixmap(); }
    void dropObsoletePixmap() { m_obsoletePixmap = QPixmap(); }

    static void dropCachedPixmaps(PageItem* page);

    bool paint(QPainter* painter, QPointF topLeft);

public:
    void refresh(bool keepObsoletePixmaps = false);

    int startRender(bool prefetch = false);
    void cancelRender();

    void deleteAfterRender();

private:
    void on_finished(const RenderParam& renderParam,
                     const QRect& rect, bool prefetch,
                     const QImage& image, const QRectF& cropRect);
    void on_canceled();
    void on_finishedOrCanceled();

private:
    Q_DISABLE_COPY(TileItem)

    static Settings* s_settings;

    typedef QPair< PageItem*, QByteArray > CacheKey;
    typedef QPair< QPixmap, QRectF > CacheObject;

    static QCache< CacheKey, CacheObject > s_cache;

    CacheKey cacheKey() const;

    PageItem* m_page;

    QRect m_rect;
    QRectF m_cropRect;

    bool m_pixmapError;
    QPixmap m_pixmap;
    QPixmap m_obsoletePixmap;

    QPixmap takePixmap();

    bool m_deleteAfterRender;
    RenderTask m_renderTask;

};

} // qpdfview

#endif // PAGEITEM_H
