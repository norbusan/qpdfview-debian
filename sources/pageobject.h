/*

Copyright 2012 Adam Reichold

This file is part of qpdfview.

qpdfview is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

qpdfview is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with qpdfview.  If not, see <http://www.gnu.org/licenses/>.

*/

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

    const QTransform &pageTransform() const;
    const QTransform &linkTransform() const;
    const QTransform &resultsTransform() const;

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
    void render(bool prefetch = false);

signals:
    void indexChanged(int);

public slots:
    void prefetch();

private slots:
    void updatePage();
    void updateResults(int index);
    void updateHighlights();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

};

#endif // PAGEOBJECT_H
