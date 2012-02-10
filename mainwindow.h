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

    QLineEdit *m_pageLineEdit;

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

    QAction *m_fullscreenAction;

};

#endif // MAINWINDOW_H
