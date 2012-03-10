#ifndef PAGEOBJECT_H
#define PAGEOBJECT_H

#include <QtCore>
#include <QtGui>

#include <poppler-qt4.h>

class PageObject;

#include "documentview.h"

class PageObject : public QGraphicsObject
{
    Q_OBJECT

public:
    explicit PageObject(Poppler::Page *page, int index, DocumentView *view, QGraphicsItem *parent = 0);
    ~PageObject();

    int index() const;
    void setIndex(const int &index);

    QString filePath() const;
    qreal resolutionX() const;
    qreal resolutionY() const;
    Poppler::Page::Rotation rotation() const;


    bool findNext(const QString &text);

    void clearHighlight();
    QRectF highlightedArea() const;
    QString highlightedText() const;

    void prefetch();
    void updateScene();

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

private:
    Poppler::Page *m_page;
    int m_index;
    DocumentView *m_view;

    QList<Poppler::LinkGoto*> m_links;
    QRectF m_highlight;
    QRectF m_selection;

    QFutureWatcher<void> *m_renderWatcher;
    void renderPage(bool prefetch);

    static QMutex s_mutex;
    static QMap<QPair<QString, int>, QImage> s_pageCache;
    static int s_maximumPageCacheSize;

signals:
    void indexChanged(int);

    void linkClicked(int gotoPage);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

};

#endif // PAGEOBJECT_H
