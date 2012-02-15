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

    enum States { Dummy, Running, Finished };
    States m_state;
    QMutex m_stateMutex;

    static QMap<int, QImage> s_pageCache;
    static QMutex s_pageCacheMutex;

    static const int maximumPageCacheSize;

};

#endif // RENDERTHREAD_H
