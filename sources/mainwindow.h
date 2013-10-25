/*

Copyright 2012-2013 Adam Reichold
Copyright 2012 Michał Trybus
Copyright 2012 Alexander Volkov

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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#ifdef WITH_DBUS

#include <QDBusAbstractAdaptor>

#endif // WITH_DBUS

class QCheckBox;
class QGraphicsView;
class QModelIndex;
class QShortcut;
class QTableView;
class QWidgetAction;

#include "global.h"

class Settings;
class DocumentView;
class TabWidget;
class TreeView;
class ComboBox;
class SpinBox;
class SearchLineEdit;
class RecentlyUsedMenu;
class BookmarkMenu;
class Database;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget* parent = 0);

    QSize sizeHint() const;
    QMenu* createPopupMenu();

public slots:
    bool open(const QString& filePath, int page = -1, const QRectF& highlight = QRectF(), bool quiet = false);
    bool openInNewTab(const QString& filePath, int page = -1, const QRectF& highlight = QRectF(), bool quiet = false);

    bool jumpToPageOrOpenInNewTab(const QString& filePath, int page = -1, bool refreshBeforeJump = false, const QRectF& highlight = QRectF(), bool quiet = false);

    void startSearch(const QString& text);

protected slots:
    void on_tabWidget_currentChanged(int index);
    void on_tabWidget_tabCloseRequested(int index);
    void on_tabWidget_tabContextMenuRequested(const QPoint& globalPos, int index);

    void on_currentTab_documentChanged();

    void on_currentTab_filePathChanged(const QString& filePath);
    void on_currentTab_numberOfPagesChaned(int numberOfPages);
    void on_currentTab_currentPageChanged(int currentPage);

    void on_currentTab_canJumpChanged(bool backward, bool forward);

    void on_currentTab_continuousModeChanged(bool continuousMode);
    void on_currentTab_layoutModeChanged(LayoutMode layoutMode);
    void on_currentTab_scaleModeChanged(ScaleMode scaleMode);
    void on_currentTab_scaleFactorChanged(qreal scaleFactor);
    void on_currentTab_rotationChanged(Rotation rotation);

    void on_currentTab_linkClicked(const QString& filePath, int page);

    void on_currentTab_invertColorsChanged(bool invertColors);
    void on_currentTab_highlightAllChanged(bool highlightAll);
    void on_currentTab_rubberBandModeChanged(RubberBandMode rubberBandMode);

    void on_currentTab_searchFinished();
    void on_currentTab_searchProgressChanged(int progress);

    void on_currentTab_customContextMenuRequested(const QPoint& pos);

    void on_currentPage_editingFinished();
    void on_currentPage_returnPressed();

    void on_scaleFactor_activated(int index);
    void on_scaleFactor_editingFinished();
    void on_scaleFactor_returnPressed();

    void on_open_triggered();
    void on_openInNewTab_triggered();
    void on_openContainingFolder_triggered();
    void on_refresh_triggered();
    void on_saveCopy_triggered();
    void on_saveAs_triggered();
    void on_print_triggered();

    void on_recentlyUsed_openTriggered(const QString& filePath);

    void on_previousPage_triggered();
    void on_nextPage_triggered();
    void on_firstPage_triggered();
    void on_lastPage_triggered();

    void on_jumpToPage_triggered();

    void on_jumpBackward_triggered();
    void on_jumpForward_triggered();

    void on_search_triggered();
    void on_findPrevious_triggered();
    void on_findNext_triggered();
    void on_cancelSearch_triggered();

    void on_copyToClipboardMode_triggered(bool checked);
    void on_addAnnotationMode_triggered(bool checked);

    void on_settings_triggered();

    void on_continuousMode_triggered(bool checked);
    void on_twoPagesMode_triggered(bool checked);
    void on_twoPagesWithCoverPageMode_triggered(bool checked);
    void on_multiplePagesMode_triggered(bool checked);

    void on_zoomIn_triggered();
    void on_zoomOut_triggered();
    void on_originalSize_triggered();
    void on_fitToPageWidthMode_triggered(bool checked);
    void on_fitToPageSizeMode_triggered(bool checked);

    void on_rotateLeft_triggered();
    void on_rotateRight_triggered();

    void on_invertColors_triggered(bool checked);

    void on_fonts_triggered();

    void on_fullscreen_triggered(bool checked);
    void on_presentation_triggered();

    void on_previousTab_triggered();
    void on_nextTab_triggered();
    void on_closeTab_triggered();
    void on_closeAllTabs_triggered();
    void on_closeAllTabsButCurrentTab_triggered();

    void on_tabAction_triggered();
    void on_tabShortcut_activated();

    void on_previousBookmark_triggered();
    void on_nextBookmark_triggered();
    void on_addBookmark_triggered();
    void on_removeBookmark_triggered();
    void on_removeAllBookmarks_triggered();

    void on_bookmark_openTriggered(const QString& filePath);
    void on_bookmark_openInNewTabTriggered(const QString& filePath);
    void on_bookmark_jumpToPageTriggered(const QString& filePath, int page);

    void on_contents_triggered();
    void on_about_triggered();

    void on_focusCurrentPage_activated();
    void on_focusScaleFactor_activated();

    void on_searchInitiated(const QString& text, bool allTabs);
    void on_highlightAll_clicked(bool checked);

    void on_outline_clicked(const QModelIndex& index);

    void on_thumbnails_dockLocationChanged(Qt::DockWidgetArea area);
    void on_thumbnails_verticalScrollBar_valueChanged(int value);

    void on_database_tabRestored(const QString& filePath, bool continousMode, LayoutMode layoutMode, ScaleMode scaleMode, qreal scaleFactor, Rotation rotation, int currentPage);
    void on_database_bookmarkRestored(const QString& filePath, const JumpList& pages);

protected:
    void closeEvent(QCloseEvent* event);

    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent* event);

private:
    Q_DISABLE_COPY(MainWindow)

    static Settings* s_settings;
    static Database* s_database;

    TabWidget* m_tabWidget;

    DocumentView* currentTab() const;
    DocumentView* tab(int index) const;
    QList< DocumentView* > tabs() const;

    bool senderIsCurrentTab() const;

    void setWindowTitleForCurrentTab();

    BookmarkMenu* bookmarkForCurrentTab() const;

    void saveTabs() const;
    void saveBookmarks() const;

    bool saveModifications(DocumentView* tab);

    SpinBox* m_currentPageSpinBox;
    QWidgetAction* m_currentPageAction;

    ComboBox* m_scaleFactorComboBox;
    QWidgetAction* m_scaleFactorAction;

    SearchLineEdit* m_searchLineEdit;
    QCheckBox* m_matchCaseCheckBox;
    QCheckBox* m_highlightAllCheckBox;

    void createWidgets();

    QAction* m_openAction;
    QAction* m_openInNewTabAction;
    QAction* m_openContainingFolderAction;
    QAction* m_refreshAction;
    QAction* m_saveCopyAction;
    QAction* m_saveAsAction;
    QAction* m_printAction;
    QAction* m_exitAction;

    QAction* m_previousPageAction;
    QAction* m_nextPageAction;
    QAction* m_firstPageAction;
    QAction* m_lastPageAction;

    QAction* m_jumpToPageAction;

    QAction* m_jumpBackwardAction;
    QAction* m_jumpForwardAction;

    QAction* m_searchAction;
    QAction* m_findPreviousAction;
    QAction* m_findNextAction;
    QAction* m_cancelSearchAction;

    QAction* m_copyToClipboardModeAction;
    QAction* m_addAnnotationModeAction;

    QAction* m_settingsAction;

    QAction* m_continuousModeAction;
    QAction* m_twoPagesModeAction;
    QAction* m_twoPagesWithCoverPageModeAction;
    QAction* m_multiplePagesModeAction;

    QAction* m_zoomInAction;
    QAction* m_zoomOutAction;
    QAction* m_originalSizeAction;

    QAction* m_fitToPageWidthModeAction;
    QAction* m_fitToPageSizeModeAction;

    QAction* m_rotateLeftAction;
    QAction* m_rotateRightAction;

    QAction* m_invertColorsAction;

    QAction* m_fontsAction;

    QAction* m_fullscreenAction;
    QAction* m_presentationAction;

    QAction* m_previousTabAction;
    QAction* m_nextTabAction;

    QAction* m_closeTabAction;
    QAction* m_closeAllTabsAction;
    QAction* m_closeAllTabsButCurrentTabAction;

    QShortcut* m_tabShortcuts[9];

    QAction* m_previousBookmarkAction;
    QAction* m_nextBookmarkAction;

    QAction* m_addBookmarkAction;
    QAction* m_removeBookmarkAction;
    QAction* m_removeAllBookmarksAction;

    QAction* m_contentsAction;
    QAction* m_aboutAction;

    QAction* createAction(const QString& text, const QString& objectName, const QIcon& icon, const QKeySequence& shortcut, const char* member, bool checkable = false);
    QAction* createAction(const QString& text, const QString& objectName, const QString& iconName, const QKeySequence& shortcut, const char* member, bool checkable = false);

    void createActions();

    QToolBar* m_fileToolBar;
    QToolBar* m_editToolBar;
    QToolBar* m_viewToolBar;

    QShortcut* m_focusCurrentPageShortcut;
    QShortcut* m_focusScaleFactorShortcut;

    QToolBar* createToolBar(const QString& text, const QString& objectName, const QStringList& actionNames, const QList< QAction* >& actions);

    void createToolBars();

    QDockWidget* m_outlineDock;
    TreeView* m_outlineView;

    QDockWidget* m_propertiesDock;
    QTableView* m_propertiesView;

    QDockWidget* m_thumbnailsDock;
    QGraphicsView* m_thumbnailsView;

    QDockWidget* m_searchDock;
    QWidget* m_searchWidget;

    QDockWidget* createDock(const QString& text, const QString& objectName, const QKeySequence& toggleViewShortcut);

    void createDocks();

    QMenu* m_fileMenu;
    RecentlyUsedMenu* m_recentlyUsedMenu;
    QMenu* m_editMenu;
    QMenu* m_viewMenu;
    QMenu* m_tabsMenu;
    QMenu* m_bookmarksMenu;
    QMenu* m_helpMenu;

    void createMenus();

};

#ifdef WITH_DBUS

class MainWindowAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "local.qpdfview.MainWindow")

public:
    explicit MainWindowAdaptor(MainWindow* mainWindow);

public slots:
    bool open(const QString& filePath, int page = -1, const QRectF& highlight = QRectF(), bool quiet = false);
    bool openInNewTab(const QString& filePath, int page = -1, const QRectF& highlight = QRectF(), bool quiet = false);

    bool jumpToPageOrOpenInNewTab(const QString& filePath, int page = -1, bool refreshBeforeJump = false, const QRectF& highlight = QRectF(), bool quiet = false);

    Q_NOREPLY void startSearch(const QString& text);

    Q_NOREPLY void raiseAndActivate();

private:
    MainWindow* mainWindow() const;

};

#endif // WITH_DBUS

#endif // MAINWINDOW_H
