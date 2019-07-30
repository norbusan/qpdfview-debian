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

#ifndef THUMBNAILITEM_H
#define THUMBNAILITEM_H

#include <QString>

#if QT_VERSION >= QT_VERSION_CHECK(4,7,0)

#include <QStaticText>

#endif // QT_VERSION

#include "pageitem.h"

namespace qpdfview
{

namespace Model
{
class Page;
}

class ThumbnailItem : public PageItem
{
    Q_OBJECT

public:
    ThumbnailItem(Model::Page* page, const QString& text, int index, QGraphicsItem* parent = 0);

    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

#if QT_VERSION >= QT_VERSION_CHECK(4,7,0)

    QString text() const { return m_text.text(); }
    void setText(const QString& text) { m_text.setText(text); }

#else

    const QString& text() const { return m_text; }
    void setText(const QString& text) { m_text = text; }

#endif // QT_VERSION

    qreal textHeight() const;

    bool isHighlighted() const { return m_isHighlighted; }
    void setHighlighted(bool highlighted);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);

    void contextMenuEvent(QGraphicsSceneContextMenuEvent*);

private:
    Q_DISABLE_COPY(ThumbnailItem)

#if QT_VERSION >= QT_VERSION_CHECK(4,7,0)

    QStaticText m_text;

#else

    QString m_text;

#endif // QT_VERSION

    bool m_isHighlighted;

    void prepareToolTip();

};

} // qpdfview

#endif // THUMBNAILITEM_H
