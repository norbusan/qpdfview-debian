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
    TileItem(Model::Page* page, QRectF tile, QGraphicsItem* parent = 0);
    ~TileItem();

    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*);

    inline QRectF tile() const { return m_tile; }
    void setTile(const QRectF& tile);

    inline int resolutionX() const { return m_resolutionX; }
    inline int resolutionY() const { return m_resolutionY; }
    void setResolution(int resolutionX, int resolutionY);

    inline qreal devicePixelRatio() const { return m_devicePixelRatio; }
    void setDevicePixelRatio(qreal devicePixelRatio);

    inline qreal scaleFactor() const { return m_scaleFactor; }
    void setScaleFactor(qreal scaleFactor);

    inline Rotation rotation() const { return m_rotation; }
    void setRotation(Rotation rotation);

    inline bool invertColors() { return m_invertColors; }
    void setInvertColors(bool invertColors);

public slots:
    void refresh();

    void startRender(bool prefetch = false);
    void cancelRender();

protected slots:
    void on_renderTask_finished();
    void on_renderTask_pixmapReady(int resolutionX, int resolutionY, qreal devicePixelRatio,
                                   qreal scaleFactor, Rotation rotation, bool invertColors, bool prefetch,
                                   QPixmap pixmap);

private:
    Q_DISABLE_COPY(TileItem)

    static Settings* s_settings;

    static QCache< TileItem*, QPixmap > s_cache;

    Model::Page* m_page;
    QRectF m_tile;

    int m_resolutionX;
    int m_resolutionY;
    qreal m_devicePixelRatio;

    qreal m_scaleFactor;
    Rotation m_rotation;
    bool m_invertColors;

    bool m_pixmapError;
    QPixmap m_pixmap;

    RenderTask* m_renderTask;

    qreal effectiveDevicePixelRatio();
    QPixmap cachedPixmap();

};

} // qpdfview

#endif // PAGEITEM_H
