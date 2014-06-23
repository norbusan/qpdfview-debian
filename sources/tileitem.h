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

#include "global.h"
#include "model.h"

namespace qpdfview
{

class Settings;
class RenderTask;
class PageItem;

class TileItem : public QObject
{
    Q_OBJECT

public:
    TileItem(QObject* parent = 0);
    ~TileItem();

    inline const QRect& rect() const { return m_rect; }
    inline void setRect(const QRect& rect) { m_rect = rect; }

    inline bool pixmapError() const { return m_pixmapError; }

    QPixmap takePixmap();

    inline const QPixmap& obsoletePixmap() const { return m_obsoletePixmap; }
    inline void dropObsoletePixmap() { m_obsoletePixmap = QPixmap(); }

public slots:
    void refresh(bool keepObsoletePixmaps = false);

    int startRender(bool prefetch = false);
    void cancelRender();

    void deleteAfterRender();

protected slots:
    void on_renderTask_finished();
    void on_renderTask_imageReady(const RenderParam& renderParam,
                                  const QRect& rect, bool prefetch,
                                  QImage image);

private:
    Q_DISABLE_COPY(TileItem)

    static Settings* s_settings;

    static QCache< QPair< PageItem*, QString >, QPixmap > s_cache;
    QPair< PageItem*, QString > pixmapKey() const;

    QRect m_rect;

    bool m_pixmapError;
    QPixmap m_pixmap;
    QPixmap m_obsoletePixmap;

    RenderTask* m_renderTask;

    PageItem* parentPage() const;

};

} // qpdfview

#endif // PAGEITEM_H
