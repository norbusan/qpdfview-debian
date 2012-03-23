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

class DocumentModel;
class PageObject;

class DocumentView : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int currentPage READ currentPage WRITE setCurrentPage NOTIFY currentPageChanged)
    Q_PROPERTY(PageLayout pageLayout READ pageLayout WRITE setPageLayout NOTIFY pageLayoutChanged)
    Q_PROPERTY(Scaling scaling READ scaling WRITE setScaling NOTIFY scalingChanged)
    Q_PROPERTY(Rotation rotation READ rotation WRITE setRotation NOTIFY rotationChanged)
    Q_PROPERTY(bool highlightAll READ highlightAll WRITE setHighlightAll NOTIFY highlightAllChanged)
    Q_ENUMS(PageLayout Scaling Rotation)

public:
    explicit DocumentView(DocumentModel *model, QWidget *parent = 0);

    int currentPage() const;

    enum PageLayout { OnePage, TwoPages, OneColumn, TwoColumns };
    PageLayout pageLayout() const;
    void setPageLayout(const PageLayout &pageLayout);

    enum Scaling { FitToPage, FitToPageWidth, ScaleTo50, ScaleTo75, ScaleTo100, ScaleTo125, ScaleTo150, ScaleTo200, ScaleTo400 };
    Scaling scaling() const;
    void setScaling(const Scaling &scaling);

    enum Rotation { RotateBy0, RotateBy90, RotateBy180, RotateBy270 };
    Rotation rotation() const;
    void setRotation(const Rotation &rotation);

    bool highlightAll() const;
    void setHighlightAll(bool highlightAll);

    DocumentModel *model() const
    {
        return m_model;
    }
    qreal resolutionX() const
    {
        return m_resolutionX;
    }
    qreal resolutionY() const
    {
        return m_resolutionY;
    }

    QAction *makeCurrentTabAction() const
    {
        return m_makeCurrentTabAction;
    }

private:
    DocumentModel *m_model;
    qreal m_resolutionX;
    qreal m_resolutionY;

    QAction *m_makeCurrentTabAction;

    QGraphicsScene *m_scene;
    QGraphicsView *m_view;

    QGraphicsRectItem *m_highlight;

    QMap<int, PageObject*> m_pageToPageObject;
    QMap<int, int> m_heightToPage;

    QMap<int, QRectF> m_results;
    QMap<int, QRectF>::iterator m_currentResult;

    QSettings m_settings;

    int m_currentPage;
    PageLayout m_pageLayout;
    Scaling m_scaling;
    Rotation m_rotation;
    bool m_highlightAll;

    void prepareScene();
    void prepareView();
    void prepareHighlight();

signals:
    void currentPageChanged(int);
    void pageLayoutChanged(DocumentView::PageLayout);
    void scalingChanged(DocumentView::Scaling);
    void rotationChanged(DocumentView::Rotation);
    void highlightAllChanged(bool);

public slots:
    void setCurrentPage(int currentPage);

    void previousPage();
    void nextPage();
    void firstPage();
    void lastPage();

    void findPrevious();
    void findNext();

    void copyText();

private slots:
    void makeCurrentTab();
    void scrollToPage(int value);

    void updateFilePath(const QString &filePath);
    void updateResults();

protected:
    void resizeEvent(QResizeEvent *resizeEvent);
    void wheelEvent(QWheelEvent *wheelEvent);

};

#endif // DOCUMENTVIEW_H
