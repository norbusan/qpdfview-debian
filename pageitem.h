#ifndef RENDERTHREAD_H
#define RENDERTHREAD_H

#include <QtCore>
#include <QtGui>

#include <poppler-qt4.h>

class PageItem : public QGraphicsItem, public QRunnable
{
public:
    PageItem(QGraphicsItem *parent = 0);

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    qreal m_resolutionX;
    qreal m_resolutionY;

    Poppler::Page *m_page;

    void run();

    QString m_filePath;
    int m_index;

    enum State { Dummy, Running, Finished };
    State m_state;
    QMutex m_stateMutex;

    static const int m_pageCacheCapacity;
    static QMap<int, QImage> m_pageCache;
    static QMutex m_pageCacheMutex;

};

#endif // RENDERTHREAD_H
