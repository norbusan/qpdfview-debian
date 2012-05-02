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
#include <QtDBus>

#include "documentview.h"
#include "miscellaneous.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);

    QSize sizeHint() const
    {
        return QSize(500, 700);
    }

    QMenu *createPopupMenu()
    {
        QMenu *menu = new QMenu();

        menu->addAction(m_fileToolBar->toggleViewAction());
        menu->addAction(m_editToolBar->toggleViewAction());
        menu->addAction(m_viewToolBar->toggleViewAction());
        menu->addSeparator();
        menu->addAction(m_outlineDock->toggleViewAction());
        menu->addAction(m_metaInformationDock->toggleViewAction());
        menu->addAction(m_thumbnailsDock->toggleViewAction());

        return menu;
    }

public slots:
    bool open(const QString &filePath, int page = 1, qreal top = 0.0);
    bool openInNewTab(const QString &filePath, int page = 1, qreal top = 0.0);

protected:
    void closeEvent(QCloseEvent*);
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void keyPressEvent(QKeyEvent *event);

protected slots:
    void slotOpen();
    void slotOpenInNewTab();
    void slotRefresh();
    void slotSaveCopy();
    void slotPrint();

    void slotPreviousPage();
    void slotNextPage();
    void slotFirstPage();
    void slotLastPage();

    void slotSearch();

    void slotStartSearch();
    void slotCancelSearch();

    void slotSearchProgressed(int value);
    void slotSearchCanceled();
    void slotSearchFinished();

    void slotFindPrevious();
    void slotFindNext();

    void slotSettings();

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

    void slotFilePathChanged(const QString &filePath);
    void slotNumberOfPagesChanged(int numberOfPages);

    void slotCurrentPageLineEditReturnPressed();
    void slotCurrentPageChanged(int currentPage);

    void slotPageLayoutTriggered(QAction *action);
    void slotPageLayoutCurrentIndexChanged(int index);
    void slotPageLayoutChanged(DocumentView::PageLayout pageLayout);

    void slotScalingTriggered(QAction *action);
    void slotScalingCurrentIndexChanged(int index);
    void slotScalingChanged(DocumentView::Scaling scaling);

    void slotRotationTriggered(QAction *action);
    void slotRotationCurrentIndexChanged(int index);
    void slotRotationChanged(DocumentView::Rotation rotation);

    void slotHighlightAllCheckBoxClicked(bool checked);
    void slotHighlightAllChanged(bool highlightAll);

    void slotRecentyUsedActionEntrySelected(const QString &filePath);

private:
    // actions

    QAction *m_openAction;
    QAction *m_openInNewTabAction;
    RecentlyUsedAction *m_recentlyUsedAction;
    QAction *m_refreshAction;
    QAction *m_saveCopyAction;
    QAction *m_printAction;

    QAction *m_exitAction;

    QAction *m_previousPageAction;
    QAction *m_nextPageAction;
    QAction *m_firstPageAction;
    QAction *m_lastPageAction;

    QAction *m_searchAction;
    QAction *m_findPreviousAction;
    QAction *m_findNextAction;
    QAction *m_cancelSearchAction;

    QAction *m_settingsAction;

    QAction *m_onePageAction;
    QAction *m_twoPagesAction;
    QAction *m_oneColumnAction;
    QAction *m_twoColumnsAction;
    QActionGroup *m_pageLayoutGroup;

    QAction *m_fitToPageAction;
    QAction *m_fitToPageWidthAction;
    QAction *m_scaleTo50Action;
    QAction *m_scaleTo75Action;
    QAction *m_scaleTo100Action;
    QAction *m_scaleTo125Action;
    QAction *m_scaleTo150Action;
    QAction *m_scaleTo200Action;
    QAction *m_scaleTo400Action;
    QActionGroup *m_scalingGroup;

    QAction *m_rotateBy0Action;
    QAction *m_rotateBy90Action;
    QAction *m_rotateBy180Action;
    QAction *m_rotateBy270Action;
    QActionGroup *m_rotationGroup;

    QAction *m_fullscreenAction;
    QAction *m_presentationAction;

    QAction *m_previousTabAction;
    QAction *m_nextTabAction;

    QAction *m_closeTabAction;
    QAction *m_closeAllTabsAction;
    QAction *m_closeAllTabsButCurrentTabAction;

    QAction *m_contentsAction;
    QAction *m_aboutAction;

    void createActions();

    // widgets

    QTabWidget *m_tabWidget;

    QLineEdit *m_currentPageLineEdit;
    QIntValidator *m_currentPageValidator;
    QLabel *m_numberOfPagesLabel;

    QWidget *m_pageLayoutWidget;
    QComboBox *m_pageLayoutComboBox;

    QWidget *m_scalingWidget;
    QComboBox *m_scalingComboBox;

    QWidget *m_rotationWidget;
    QComboBox *m_rotationComboBox;

    QWidget *m_searchWidget;
    QLineEdit *m_searchLineEdit;
    QTimer *m_searchTimer;
    QCheckBox *m_matchCaseCheckBox;
    QCheckBox *m_highlightAllCheckBox;

    void createWidgets();

    // toolbars

    QToolBar *m_fileToolBar;
    QToolBar *m_editToolBar;
    QToolBar *m_viewToolBar;

    QToolBar *m_searchToolBar;

    void createToolBars();

    // docks

    QDockWidget *m_outlineDock;
    QDockWidget *m_metaInformationDock;
    QDockWidget *m_thumbnailsDock;

    void createDocks();

    // menus

    QMenu *m_fileMenu;
    QMenu *m_editMenu;
    QMenu *m_viewMenu;
    QMenu *m_tabMenu;
    QMenu *m_helpMenu;

    void createMenus();

    // settings

    QSettings m_settings;
    QByteArray m_geometry;

    // D-Bus

    friend class MainWindowAdaptor;
};

class MainWindowAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "net.launchpad.qpdfview.MainWindow")

public:
    MainWindowAdaptor(MainWindow *mainWindow);

public slots:
    bool open(const QString &filePath, int page, qreal top);
    bool openInNewTab(const QString &filePath, int page, qreal top);

    Q_NOREPLY void refresh(const QString &filePath, int page, qreal top);

};

#endif // MAINWINDOW_H
