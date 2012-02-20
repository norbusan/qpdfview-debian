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
    QMenu *m_viewMenu;
    QMenu *m_tabMenu;
    QMenu *m_helpMenu;

    QTabWidget *m_tabWidget;

    QAction *m_openAction;
    QAction *m_refreshAction;
    QAction *m_printAction;

    QAction *m_exitAction;

    QAction *m_previousPageAction;
    QAction *m_nextPageAction;
    QAction *m_firstPageAction;
    QAction *m_lastPageAction;

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

    QAction *m_twoPageSpreadAction;

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

    QWidget *m_scalingWidget;
    QLabel *m_scalingLabel;
    QComboBox *m_scalingComboBox;

    QWidget *m_rotationWidget;
    QLabel *m_rotationLabel;
    QComboBox *m_rotationComboBox;

private slots:
    void open();
    void refresh();
    void print();

    void previousPage();
    void nextPage();
    void firstPage();
    void lastPage();

    void selectScaling(QAction *scalingAction);
    void changeScalingIndex(const int &index);
    void selectRotation(QAction *rotationAction);
    void changeRotationIndex(const int &index);
    void changeTwoPageSpread();

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
    void updateScaling(const DocumentView::Scaling &scaling);
    void updateRotation(const DocumentView::Rotation &rotation);
    void updateTwoPageSpread(const bool &twoPageSpread);

protected:
    void dropEvent(QDropEvent *dropEvent);
    void closeEvent(QCloseEvent *closeEvent);

};

#endif // MAINWINDOW_H
