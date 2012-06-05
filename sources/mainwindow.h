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

#ifdef WITH_DBUS

#include <QtDBus>

#endif

#include "documentview.h"
#include "miscellaneous.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = 0);

    QSize sizeHint() const;
    QMenu* createPopupMenu();

public slots:
    bool open(const QString& filePath, int page = 1, qreal top = 0.0);
    bool openInNewTab(const QString& filePath, int page = 1, qreal top = 0.0);

protected:
    void closeEvent(QCloseEvent*);

    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent* event);

    void keyPressEvent(QKeyEvent* event);

protected slots:
    void slotOpen();
    void slotOpenInNewTab();
    void slotRecentyUsedActionEntrySelected(const QString& filePath);
    void slotRefresh();
    void slotSaveCopy();
    void slotPrint();

    void slotPreviousPage();
    void slotNextPage();
    void slotFirstPage();
    void slotLastPage();
    void slotJumpToPage();

    void slotSearch();

    void slotStartSearch();

    void slotSearchProgressed(int value);
    void slotSearchCanceled();
    void slotSearchFinished();

    void slotFindPrevious();
    void slotFindNext();
    void slotCancelSearch();

    void slotSettings();

    void slotPageLayoutGroupTriggered(QAction* action);
    void slotScaleModeGroupTriggered(QAction* action);

    void slotZoomIn();
    void slotZoomOut();

    void slotRotateLeft();
    void slotRotateRight();

    void slotFullscreen();
    void slotPresentation();

    void slotPreviousTab();
    void slotNextTab();
    void slotCloseTab();

    void slotCloseAllTabs();
    void slotCloseAllTabsButCurrentTab();

    void slotContents();
    void slotAbout();

    void slotTabWidgetCurrentChanged(int index);
    void slotTabWidgetTabCloseRequested(int index);

    void slotCurrentPageLineEditReturnPressed();

    void slotScaleFactorComboBoxCurrentIndexChanged(int index);
    void slotScaleFactorComboBoxEditingFinished();

    void slotHighlightAllCheckBoxClicked(bool checked);

    void slotFilePathChanged(const QString& filePath);
    void slotNumberOfPagesChanged(int numberOfPages);

    void slotCurrentPageChanged(int currentPage);
    void slotPageLayoutChanged(DocumentView::PageLayout pageLayout);
    void slotScaleModeChanged(DocumentView::ScaleMode scaleMode);
    void slotScaleFactorChanged(qreal scaleFactor);

    void slotHighlightAllChanged(bool highlightAll);

private:
    // actions

    QAction* m_openAction;
    QAction* m_openInNewTabAction;
    RecentlyUsedAction* m_recentlyUsedAction;
    QAction* m_refreshAction;
    QAction* m_saveCopyAction;
    QAction* m_printAction;

    QAction* m_exitAction;

    QAction* m_previousPageAction;
    QAction* m_nextPageAction;
    QAction* m_firstPageAction;
    QAction* m_lastPageAction;
    QAction* m_jumpToPageAction;

    QAction* m_searchAction;
    QAction* m_findPreviousAction;
    QAction* m_findNextAction;
    QAction* m_cancelSearchAction;

    QAction* m_settingsAction;

    QAction* m_onePageAction;
    QAction* m_twoPagesAction;
    QAction* m_oneColumnAction;
    QAction* m_twoColumnsAction;
    QActionGroup* m_pageLayoutGroup;

    QAction* m_fitToPageAction;
    QAction* m_fitToPageWidthAction;
    QAction* m_doNotScaleAction;
    QActionGroup* m_scaleModeGroup;

    QAction* m_zoomInAction;
    QAction* m_zoomOutAction;

    QAction* m_rotateLeftAction;
    QAction* m_rotateRightAction;

    QAction* m_fullscreenAction;
    QAction* m_presentationAction;

    QAction* m_previousTabAction;
    QAction* m_nextTabAction;

    QAction* m_closeTabAction;
    QAction* m_closeAllTabsAction;
    QAction* m_closeAllTabsButCurrentTabAction;

    QAction* m_contentsAction;
    QAction* m_aboutAction;

    void createActions();

    // widgets

    TabWidget* m_tabWidget;

    LineEdit* m_currentPageLineEdit;
    QIntValidator* m_currentPageValidator;

    QLabel* m_numberOfPagesLabel;

    ComboBox* m_scaleFactorComboBox;

    QWidget* m_searchWidget;
    QLineEdit* m_searchLineEdit;
    QTimer* m_searchTimer;
    QCheckBox* m_matchCaseCheckBox;
    QCheckBox* m_highlightAllCheckBox;

    void createWidgets();

    // toolbars

    QToolBar* m_fileToolBar;
    QToolBar* m_editToolBar;
    QToolBar* m_viewToolBar;

    QToolBar* m_searchToolBar;

    void createToolBars();

    // docks

    QDockWidget* m_outlineDock;
    QDockWidget* m_metaInformationDock;
    QDockWidget* m_thumbnailsDock;

    void createDocks();

    // menus

    QMenu* m_fileMenu;
    QMenu* m_editMenu;
    QMenu* m_viewMenu;
    QMenu* m_tabMenu;
    QMenu* m_helpMenu;

    void createMenus();

    // settings

    QSettings m_settings;
    QByteArray m_geometry;

#ifdef WITH_DBUS

    friend class MainWindowAdaptor;
};

class MainWindowAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "local.qpdfview.MainWindow")

public:
    MainWindowAdaptor(MainWindow* mainWindow);

public slots:
    bool open(const QString& filePath, int page = 1, qreal top = 0.0);
    bool openInNewTab(const QString& filePath, int page = 1, qreal top = 0.0);

    Q_NOREPLY void refresh(const QString& filePath, int page = 1, qreal top = 0.0);

};

#else

};

#endif // WITH_DBUS

#endif // MAINWINDOW_H
