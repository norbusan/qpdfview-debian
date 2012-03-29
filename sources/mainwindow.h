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

#ifndef QPDFVIEW_ENUMS
#define QPDFVIEW_ENUMS

enum PageLayout { OnePage, TwoPages, OneColumn, TwoColumns };
enum Rotation { RotateBy0, RotateBy90, RotateBy180, RotateBy270 };
enum Scaling { FitToPage, FitToPageWidth, ScaleTo50, ScaleTo75, ScaleTo100, ScaleTo125, ScaleTo150, ScaleTo200, ScaleTo400 };

#endif

class OutlineView;
class ThumbnailsView;
class RecentlyUsedAction;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);

    QSize sizeHint() const
    {
        return QSize(500,700);
    }

    QMenu *createPopupMenu();

public slots:
    bool open(const QString &filePath, int page = 1, qreal top = 0.0);
    bool addTab(const QString &filePath, int page = 1, qreal top = 0.0);

protected:
    void keyPressEvent(QKeyEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void closeEvent(QCloseEvent *event);

protected slots:
    void slotOpen();
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

    void slotPresentation();
    void slotFullscreen();

    void slotAddTab();
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

    void slotHighlightAllCheckBoxToggled();

    void slotRecentyUsedEntrySelected(const QString &filePath);

    void slotPageLayoutTriggered(QAction *action);
    void slotPageLayoutCurrentIndexChanged(int index);
    void slotScalingTriggered(QAction *action);
    void slotScalingCurrentIndexChanged(int index);
    void slotRotationTriggered(QAction *action);
    void slotRotationCurrentIndexChanged(int index);

    void slotNumberOfPagesChanged(int numberOfPages);
    void slotCurrentPageChanged(int currentPage);
    void slotPageLayoutChanged(PageLayout pageLayout);
    void slotScalingChanged(Scaling scaling);
    void slotRotationChanged(Rotation rotation);
    void slotHighlightAllChanged(bool highlightAll);

private:
    // settings

    QSettings m_settings;
    QByteArray m_geometry;

    // actions

    QAction *m_openAction;
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

    QAction *m_presentationAction;
    QAction *m_fullscreenAction;

    QAction *m_addTabAction;
    QAction *m_previousTabAction;
    QAction *m_nextTabAction;
    QAction *m_closeTabAction;

    QAction *m_closeAllTabsAction;
    QAction *m_closeAllTabsButCurrentTabAction;

    QAction *m_contentsAction;
    QAction *m_aboutAction;

    // widgets

    QTabWidget *m_tabWidget;

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
    QTimer *m_searchTimer;
    QCheckBox *m_matchCaseCheckBox;
    QCheckBox *m_highlightAllCheckBox;
    QPushButton *m_findPreviousButton;
    QPushButton *m_findNextButton;

    // toolBars

    QToolBar *m_fileToolBar;
    QToolBar *m_editToolBar;
    QToolBar *m_viewToolBar;

    QToolBar *m_searchToolBar;

    // docks

    QDockWidget *m_outlineDock;
    OutlineView *m_outlineView;

    QDockWidget *m_thumbnailsDock;
    ThumbnailsView *m_thumbnailsView;

    // menus

    QMenu *m_fileMenu;
    QMenu *m_editMenu;
    QMenu *m_viewMenu;
    QMenu *m_tabMenu;
    QMenu *m_helpMenu;

    // internal methods

    void createActions();
    void createWidgets();
    void createToolBars();
    void createDocks();
    void createMenus();

};

#endif // MAINWINDOW_H
