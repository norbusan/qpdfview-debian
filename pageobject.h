#ifndef PAGEOBJECT_H
#define PAGEOBJECT_H

#include <QtCore>
#include <QtGui>

#include <poppler-qt4.h>

class PageObject : public QGraphicsObject
{
    Q_OBJECT
    Q_PROPERTY(qreal resolutionX READ resolutionX WRITE setResolutionX NOTIFY resolutionXChanged)
    Q_PROPERTY(qreal resolutionY READ resolutionY WRITE setResolutionY NOTIFY resolutionYChanged)

public:
    explicit PageObject(Poppler::Page *page, QGraphicsItem *parent = 0);
    ~PageObject();


    qreal resolutionX() const;
    void setResolutionX(const qreal &resolutionX);

    qreal resolutionY() const;
    void setResolutionY(const qreal &resolutionY);


    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

private:
    Poppler::Page *m_page;

    qreal m_resolutionX;
    qreal m_resolutionY;

signals:
    void resolutionXChanged(qreal);
    void resolutionYChanged(qreal);

};

/*class PageItem : public QGraphicsItem, public QRunnable
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

};*/

#endif // PAGEOBJECT_H
