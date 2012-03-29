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

#ifndef PAGEVIEW_H
#define PAGEVIEW_H

#include <QtCore>
#include <QtGui>

#ifndef QPDFVIEW_ENUMS
#define QPDFVIEW_ENUMS

enum PageLayout { OnePage, TwoPages, OneColumn, TwoColumns };
enum Rotation { RotateBy0, RotateBy90, RotateBy180, RotateBy270 };
enum Scaling { FitToPage, FitToPageWidth, ScaleTo50, ScaleTo75, ScaleTo100, ScaleTo125, ScaleTo150, ScaleTo200, ScaleTo400 };

#endif

struct Link;
class Document;

class PageView : public QGraphicsObject
{
    Q_OBJECT
    Q_PROPERTY(int index READ index NOTIFY indexChanged)
    Q_PROPERTY(qreal scaleFactor READ scaleFactor WRITE setScaleFactor NOTIFY scaleFactorChanged)
    Q_PROPERTY(Rotation rotation READ rotation WRITE setRotation NOTIFY rotationChanged)
    Q_PROPERTY(bool highlightAll READ highlightAll WRITE setHighlightAll NOTIFY highlightAllChanged)

public:
    explicit PageView(Document *document, int index, QGraphicsItem *parent = 0);
    ~PageView();

    // properties

    int index() const;

    qreal scaleFactor() const;
    void setScaleFactor(qreal scaleFactor);

    Rotation rotation() const;
    void setRotation(Rotation rotation);

    bool highlightAll() const;
    void setHighlightAll(bool highlightAll);

    const QList<Link> &links() const;

    const QList<QRectF> &searchResults() const;

    // transforms

    const QSizeF &size() const;

    const QTransform &pageTransform() const;
    const QTransform &linkTransform() const;
    const QTransform &highlightTransform() const;

    // graphics

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*);

public slots:
    void prefetch();

signals:
    void indexChanged(int index);
    void scaleFactorChanged(qreal scaleFactor);
    void rotationChanged(Rotation rotation);
    void highlightAllChanged(bool highlightAll);

    void linkLeftClicked(int page, qreal top);
    void linkLeftClicked(QString url);
    void linkMiddleClicked(int page, qreal top);
    void linkMiddleClicked(QString url);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event);

protected slots:
    void slotPageRendered(int index);
    void slotPageSearched(int index);

private:
    Document *m_document;

    // properties

    int m_index;
    qreal m_scaleFactor;
    Rotation m_rotation;
    bool m_highlightAll;

    QList<Link> m_links;

    QList<QRectF> m_searchResults;

    // graphics

    QRectF m_highlight;
    QRectF m_rubberBand;

    // transforms

    QSizeF m_size;

    QTransform m_pageTransform;
    QTransform m_linkTransform;
    QTransform m_highlightTransform;

    void prepareTransforms();

    // internal methods

    QFuture<void> m_render;
    void render(bool prefetch = false);

};

#endif // PAGEVIEW_H
