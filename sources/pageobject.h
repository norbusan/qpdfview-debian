#ifndef PAGEOBJECT_H
#define PAGEOBJECT_H

#include <QtCore>
#include <QtGui>

#include <poppler-qt4.h>

#include "documentmodel.h"
#include "documentview.h"

class PageObject : public QGraphicsObject
{
    Q_OBJECT
    Q_PROPERTY(int index READ index WRITE setIndex NOTIFY indexChanged)

public:
    explicit PageObject(DocumentModel *model, DocumentView *view, int index, QGraphicsItem *parent = 0);
    ~PageObject();

    int index() const;
    void setIndex(const int &index);

    qreal pageWidth() const;
    qreal pageHeight() const;

    QRectF selectedArea() const;
    QString selectedText() const;

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

private:
    int m_index;

    DocumentModel *m_model;
    DocumentView *m_view;

    QSizeF m_size;
    QList<DocumentModel::Link> m_links;
    QList<QRectF> m_results;

    QTransform m_pageTransform;
    QTransform m_linkTransform;
    QTransform m_resultsTransform;

    QRectF m_selection;
    QRectF m_rubberBand;

    QFutureWatcher<void> m_render;
    void render();

signals:
    void indexChanged(int);

    void linkClicked(int pageNumber);

private slots:
    void updatePage();
    void updateResults(int index);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

};

#endif // PAGEOBJECT_H
