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
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

    QSize sizeHint() const { return QSize(500,700); }

private slots:
    void open();
    void addTab();
    void previousTab();
    void nextTab();
    void closeTab();

    void reload();
    void save();
    void print();

    void previousPage();
    void nextPage();
    void firstPage();
    void lastPage();

    void selectDisplayMode(QAction *displayModeAction);
    void selectScaleMode(QAction *scaleModeAction);
    void selectRotationMode(QAction *rotationModeAction);

    void fullscreen();

    void changeCurrent(const int &index);
    void requestTabClose(const int &index);

protected:
    void dropEvent(QDropEvent *dropEvent);
    void closeEvent(QCloseEvent *closeEvent);

private:
    QTabWidget *m_tabWidget;

    QAction *m_openAction;
    QAction *m_addTabAction;
    QAction *m_previousTabAction;
    QAction *m_nextTabAction;
    QAction *m_closeTabAction;

    QAction *m_reloadAction;
    QAction *m_saveAction;
    QAction *m_printAction;

    QAction *m_exitAction;

    QWidget *m_pageWidget;
    QLabel *m_pageLabel;
    QLineEdit *m_pageLineEdit;

    QWidget *m_scalingWidget;
    QLabel *m_scalingLabel;
    QComboBox *m_scalingComboBox;

    QAction *m_previousPageAction;
    QAction *m_nextPageAction;
    QAction *m_firstPageAction;
    QAction *m_lastPageAction;

    QAction *m_pagingAction;
    QAction *m_scrollingAction;
    QAction *m_doublePagingAction;
    QAction *m_doubleScrollingAction;
    QActionGroup *m_displayModeGroup;

    QAction *m_scaleFactorAction;
    QAction *m_fitToPageAction;
    QAction *m_fitToPageWidthAction;
    QActionGroup *m_scaleModeGroup;

    QAction *m_doNotRotateAction;
    QAction *m_rotateBy90Action;
    QAction *m_rotateBy180Action;
    QAction *m_rotateBy270Action;
    QActionGroup *m_rotationModeGroup;

    QAction *m_fullscreenAction;

};

#endif // MAINWINDOW_H
