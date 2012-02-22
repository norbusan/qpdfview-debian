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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore>
#include <QtGui>

#include "documentview.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    QSize sizeHint() const;
private:
    QMenu *m_fileMenu;
    QMenu *m_editMenu;
    QMenu *m_viewMenu;
    QMenu *m_tabMenu;
    QMenu *m_helpMenu;

    QToolBar *m_fileToolBar;
    QToolBar *m_editToolBar;
    QToolBar *m_searchToolBar;
    QToolBar *m_viewToolBar;

    QTabWidget *m_tabWidget;

    QAction *m_openAction;
    QAction *m_refreshAction;
    QAction *m_printAction;

    QAction *m_exitAction;

    QAction *m_previousPageAction;
    QAction *m_nextPageAction;
    QAction *m_firstPageAction;
    QAction *m_lastPageAction;

    QAction *m_searchAction;
    QAction *m_findNextAction;

    QAction *m_onePageAction;
    QAction *m_twoPagesAction;
    QAction *m_oneColumnAction;
    QAction *m_twoColumnsAction;
    QActionGroup *m_pageLayoutGroup;

    QAction *m_fitToPageAction;
    QAction *m_fitToPageWidthAction;
    QAction *m_scaleTo25Action;
    QAction *m_scaleTo50Action;
    QAction *m_scaleTo100Action;
    QAction *m_scaleTo200Action;
    QAction *m_scaleTo400Action;
    QActionGroup *m_scalingGroup;

    QAction *m_rotateBy0Action;
    QAction *m_rotateBy90Action;
    QAction *m_rotateBy180Action;
    QAction *m_rotateBy270Action;
    QActionGroup *m_rotationGroup;

    QAction *m_fullscreenAction;

    QAction *m_addTabAction;
    QAction *m_previousTabAction;
    QAction *m_nextTabAction;
    QAction *m_closeTabAction;

    QAction *m_aboutAction;

    QWidget *m_currentPageWidget;
    QLabel *m_currentPageLabel;
    QLineEdit *m_currentPageLineEdit;
    QIntValidator *m_currentPageValidator;
    QLabel *m_numberOfPagesLabel;

    QWidget *m_pageLayoutWidget;
    QLabel *m_pageLayoutLabel;
    QComboBox *m_pageLayoutComboBox;

    QWidget *m_scalingWidget;
    QLabel *m_scalingLabel;
    QComboBox *m_scalingComboBox;

    QWidget *m_rotationWidget;
    QLabel *m_rotationLabel;
    QComboBox *m_rotationComboBox;

    QWidget *m_searchWidget;
    QLabel *m_searchLabel;
    QLineEdit *m_searchLineEdit;
    QPushButton *m_findNextButton;

    QSettings m_settings;
    QByteArray m_normalGeometry;

private slots:
    void open();
    void refresh();
    void print();

    void previousPage();
    void nextPage();
    void firstPage();
    void lastPage();
    void changeCurrentPage();

    void search();
    void findNext();

    void selectPageLayout(QAction *pageLayoutAction);
    void changePageLayoutIndex(const int &index);
    void selectScaling(QAction *scalingAction);
    void changeScalingIndex(const int &index);
    void selectRotation(QAction *rotationAction);
    void changeRotationIndex(const int &index);

    void changeFullscreen();

    void addTab();
    void previousTab();
    void nextTab();
    void closeTab();

    void changeCurrentTab(const int &index);
    void requestTabClose(const int &index);

    void about();

    void updateFilePath(const QString &filePath);
    void updateCurrentPage(const int &currentPage);
    void updateNumberOfPages(const int &numberOfPages);
    void updatePageLayout(const DocumentView::PageLayout &pageLayout);
    void updateScaling(const DocumentView::Scaling &scaling);
    void updateRotation(const DocumentView::Rotation &rotation);

protected:
    void dragEnterEvent(QDragEnterEvent *dragEnterEvent);
    void dropEvent(QDropEvent *dropEvent);
    void closeEvent(QCloseEvent *closeEvent);

};

#endif // MAINWINDOW_H
