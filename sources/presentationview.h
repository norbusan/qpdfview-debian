/*

Copyright 2012-2013 Adam Reichold

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

#include <QGraphicsView>

#include "global.h"

namespace Model
{
class Page;
}

class Settings;
class PageItem;

class PresentationView : public QGraphicsView
{
    Q_OBJECT

public:
    PresentationView(const QList< Model::Page* >& pages, QWidget* parent = 0);
    ~PresentationView();

    int numberOfPages() const;
    int currentPage() const;

    Rotation rotation() const;
    void setRotation(Rotation rotation);

    bool invertColors() const;
    void setInvertColors(bool invertColors);

signals:
    void currentPageChanged(int currentPage, bool trackChange = false);

    void rotationChanged(Rotation rotation);

    void invertColorsChanged(bool invertColors);
    
public slots:
    void show();

    void previousPage();
    void nextPage();
    void firstPage();
    void lastPage();

    void jumpToPage(int page, bool trackChange = true);

    void jumpBackward();
    void jumpForward();

    void rotateLeft();
    void rotateRight();

protected slots:
    void on_prefetch_timeout();

    void on_pages_linkClicked(int page, qreal left, qreal top);

protected:
    void resizeEvent(QResizeEvent* event);

    void keyPressEvent(QKeyEvent* event);
    void wheelEvent(QWheelEvent* event);

private:
    Q_DISABLE_COPY(PresentationView)

    static Settings* s_settings;

    QTimer* m_prefetchTimer;

    QList< Model::Page* > m_pages;

    int m_currentPage;

    QList< int > m_past;
    QList< int > m_future;

    Rotation m_rotation;

    bool m_invertColors;

    QVector< PageItem* > m_pageItems;

    void preparePages();
    void prepareBackground();

    void prepareScene();
    void prepareView();
    
};

#endif // PRESENTATIONVIEW_H
