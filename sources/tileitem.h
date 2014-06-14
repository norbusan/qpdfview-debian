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
#include <QGraphicsObject>
#include <QIcon>

#include "global.h"

namespace qpdfview
{

namespace Model
{
class Page;
}

class Settings;
class RenderTask;

class TileItem : public QGraphicsObject
{
    Q_OBJECT

public:
    TileItem(QGraphicsItem* parent = 0);
    ~TileItem();

    QRectF boundingRect() const;
    void setBoundingRect(const QRectF& boundingRect);

    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*);

    void dropObsoletePixmaps();

public slots:
    void refresh(bool canKeepObsoletePixmaps = false);

    void startRender(bool prefetch = false);
    void cancelRender();

    void deleteAfterRender();

protected slots:
    void on_renderTask_finished();
    void on_renderTask_imageReady(int resolutionX, int resolutionY, qreal devicePixelRatio,
                                  qreal scaleFactor, Rotation rotation, bool invertColors,
                                  const QRect& tile, bool prefetch,
                                  QImage image);

private:
    Q_DISABLE_COPY(TileItem)

    static Settings* s_settings;

    static QCache< TileItem*, QPixmap > s_cache;

    QRectF m_boundingRect;

    bool m_pixmapError;
    QPixmap m_pixmap;
    QPixmap m_obsoletePixmap;

    RenderTask* m_renderTask;

    QPixmap takePixmap();

};

} // qpdfview

#endif // PAGEITEM_H
