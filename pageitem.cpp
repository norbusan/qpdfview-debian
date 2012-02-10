#include "pageitem.h"

PageItem::PageItem(QGraphicsItem *parent, QGraphicsScene *scene) :
    QGraphicsItem(parent, scene)
{    
}

QRectF PageItem::boundingRect() const
{
    return QRectF();
}

void PageItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
}
