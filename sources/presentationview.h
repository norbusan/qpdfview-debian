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


#ifndef PRESENTATIONVIEW_H
#define PRESENTATIONVIEW_H

#include <QtCore>
#include <QtGui>

#include "documentmodel.h"

class PresentationView : public QWidget
{
    Q_OBJECT

public:
    explicit PresentationView(DocumentModel *model, int currentPage);
    ~PresentationView();

private:
    DocumentModel *m_model;

    int m_currentPage;

    QSizeF m_size;
    qreal m_resolutionX;
    qreal m_resolutionY;
    QRectF m_boundingRect;

    QList<DocumentModel::Link> m_links;
    QTransform m_linkTransform;

    QTimer *m_prefetchTimer;

    QFuture<void> m_render;
    void render();

    QFuture<void> m_prefetch;
    void prefetch();

    void preparePage();
    
public slots:
    void previousPage();
    void nextPage();
    void firstPage();
    void lastPage();

    void gotoPage(int pageNumber);

private slots:
    void prefetchTimeout();

protected:
    void paintEvent(QPaintEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    
};

#endif // PRESENTATIONVIEW_H
