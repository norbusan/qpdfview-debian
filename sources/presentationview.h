/*

Copyright 2012 Adam Reichold

This file is part of qpdfview.

qpdfview is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
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

#include <poppler-qt4.h>

#include "pageitem.h"

class PresentationView : public QWidget
{
    Q_OBJECT

public:
    PresentationView(QMutex* mutex, Poppler::Document* document);
    ~PresentationView();

    int numberOfPages() const;
    int currentPage() const;
    
public slots:
    void previousPage();
    void nextPage();
    void firstPage();
    void lastPage();

    void jumpToPage(int page, bool returnTo = true);

protected slots:
    void on_render_finished();

protected:
    void resizeEvent(QResizeEvent* event);
    void paintEvent(QPaintEvent* event);

    void keyPressEvent(QKeyEvent* event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent* event);

private:
    QMutex* m_mutex;
    Poppler::Document* m_document;

    int m_numberOfPages;
    int m_currentPage;
    int m_returnToPage;

    QList< Poppler::LinkGoto* > m_links;

    qreal m_scaleFactor;
    QRectF m_boundingRect;
    QTransform m_normalizedTransform;

    void prepareView();

    // render

    QImage m_image1;
    QImage m_image2;

    QFutureWatcher< void >* m_render;
    void render(int index, qreal scaleFactor);
    
};

#endif // PRESENTATIONVIEW_H
