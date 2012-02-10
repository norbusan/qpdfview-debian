#ifndef PAGEITEM_H
#define PAGEITEM_H

#include <QtCore>
#include <QtGui>

#include "documentmodel.h"

class PageItem : public QGraphicsItem
{
public:
    PageItem(QGraphicsItem *parent = 0, QGraphicsScene *scene = 0);

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

};

#endif // PAGEITEM_H
