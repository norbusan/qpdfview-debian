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

#ifndef DOCUMENTVIEW_H
#define DOCUMENTVIEW_H

#include <QtCore>
#include <QtGui>

#include "pageobject.h"

class DocumentView : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString filePath READ filePath NOTIFY filePathChanged)
    Q_PROPERTY(int currentPage READ currentPage WRITE setCurrentPage NOTIFY currentPageChanged)
    Q_PROPERTY(int numberOfPages READ numberOfPages NOTIFY numberOfPagesChanged)
    Q_PROPERTY(PageLayout pageLayout READ pageLayout WRITE setPageLayout NOTIFY pageLayoutChanged)
    Q_PROPERTY(Scaling scaling READ scaling WRITE setScaling NOTIFY scalingChanged)
    Q_PROPERTY(Rotation rotation READ rotation WRITE setRotation NOTIFY rotationChanged)
    Q_ENUMS(PageLayout Scaling Rotation)

public:
    explicit DocumentView(QWidget *parent = 0);
    ~DocumentView();


    QString filePath() const;

    int currentPage() const;
    void setCurrentPage(const int &currentPage);

    int numberOfPages() const;

    enum PageLayout { OnePage, TwoPages, OneColumn, TwoColumns };
    PageLayout pageLayout() const;
    void setPageLayout(const PageLayout &pageLayout);

    enum Scaling { FitToPage, FitToPageWidth, ScaleTo25, ScaleTo50, ScaleTo100, ScaleTo200, ScaleTo400 };
    Scaling scaling() const;
    void setScaling(const Scaling &scaling);

    enum Rotation { RotateBy0, RotateBy90, RotateBy180, RotateBy270 };
    Rotation rotation() const;
    void setRotation(const Rotation &rotation);


    bool open(const QString &filePath);
    bool refresh();

    void previousPage();
    void nextPage();
    void firstPage();
    void lastPage();

private:
    QGraphicsScene *m_graphicsScene;
    QGraphicsView *m_graphicsView;

    Poppler::Document *m_document;

    QMap<int, PageObject*> m_numberToObject;
    QMap<qreal, int> m_heightToNumber;
    PageCache *m_pageCache;

    QString m_filePath;
    int m_currentPage;
    int m_numberOfPages;
    PageLayout m_pageLayout;
    Scaling m_scaling;
    Rotation m_rotation;

    void prepareScene();
    void prepareView();
    void prefetch();

signals:
    void filePathChanged(QString);
    void currentPageChanged(int);
    void numberOfPagesChanged(int);
    void pageLayoutChanged(DocumentView::PageLayout);
    void scalingChanged(DocumentView::Scaling);
    void rotationChanged(DocumentView::Rotation);

private slots:
    void changeCurrentPage(const int &value);

protected:
    void resizeEvent(QResizeEvent *resizeEvent);
    void wheelEvent(QWheelEvent *wheelEvent);

};

#endif // DOCUMENTVIEW_H
