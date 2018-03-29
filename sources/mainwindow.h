/*

Copyright 2014 S. Razi Alavizadeh
Copyright 2012-2018 Adam Reichold
Copyright 2018 Pavel Sanda
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

#include <QPointer>

#ifdef WITH_DBUS

#include <QDBusAbstractAdaptor>

class QDBusInterface;

#endif // WITH_DBUS

class QCheckBox;
class QDateTime;
class QGraphicsView;
class QFileInfo;
class QModelIndex;
class QShortcut;
class QTableView;
class QToolButton;
class QTreeView;
class QWidgetAction;

#include "renderparam.h"

namespace qpdfview
{

class Settings;
class DocumentView;
class TabWidget;
class TreeView;
class ComboBox;
class MappingSpinBox;
class SearchLineEdit;
class SearchableMenu;
class RecentlyUsedMenu;
class RecentlyClosedMenu;
class BookmarkModel;
class Database;
class ShortcutHandler;
class SearchModel;
class HelpDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

    friend class MainWindowAdaptor;

public:
    explicit MainWindow(QWidget* parent = 0);

    QSize sizeHint() const;
    QMenu* createPopupMenu();

public slots:
    void show();

    bool open(const QString& filePath, int page = -1, const QRectF& highlight = QRectF(), bool quiet = false);
    bool openInNewTab(const QString& filePath, int page = -1, const QRectF& highlight = QRectF(), bool quiet = false);

    bool jumpToPageOrOpenInNewTab(const QString& filePath, int page = -1, bool refreshBeforeJump = false, const QRectF& highlight = QRectF(), bool quiet = false);

    void startSearch(const QString& text);

    void saveDatabase();

protected slots:
    void on_tabWidget_currentChanged();
    void on_tabWidget_tabCloseRequested(int index);
    void on_tabWidget_tabDragRequested(int index);
    void on_tabWidget_tabContextMenuRequested(QPoint globalPos, int index);

    void on_currentTab_documentChanged();
    void on_currentTab_documentModified();

    void on_currentTab_numberOfPagesChaned(int numberOfPages);
    void on_currentTab_currentPageChanged(int currentPage);

    void on_currentTab_canJumpChanged(bool backward, bool forward);

    void on_currentTab_continuousModeChanged(bool continuousMode);
    void on_currentTab_layoutModeChanged(LayoutMode layoutMode);
    void on_currentTab_rightToLeftModeChanged(bool rightToLeftMode);
    void on_currentTab_scaleModeChanged(ScaleMode scaleMode);
    void on_currentTab_scaleFactorChanged(qreal scaleFactor);
    void on_currentTab_rotationChanged(Rotation rotation);

    void on_currentTab_linkClicked(int page);
    void on_currentTab_linkClicked(bool newTab, const QString& filePath, int page);

    void on_currentTab_renderFlagsChanged(qpdfview::RenderFlags renderFlags);

    void on_currentTab_invertColorsChanged(bool invertColors);
    void on_currentTab_convertToGrayscaleChanged(bool convertToGrayscale);
    void on_currentTab_trimMarginsChanged(bool trimMargins);

    void on_currentTab_compositionModeChanged(CompositionMode compositionMode);

    void on_currentTab_highlightAllChanged(bool highlightAll);
    void on_currentTab_rubberBandModeChanged(RubberBandMode rubberBandMode);

    void on_currentTab_searchFinished();
    void on_currentTab_searchProgressChanged(int progress);

    void on_currentTab_customContextMenuRequested(QPoint pos);

    void on_splitView_splitHorizontally_triggered();
    void on_splitView_splitVertically_triggered();
    void on_splitView_split_triggered(Qt::Orientation orientation, int index);
    void on_splitView_closeCurrent_triggered();
    void on_splitView_closeCurrent_triggered(int index);
    void on_splitView_currentWidgetChanged(QWidget* currentWidget);

    void on_currentPage_editingFinished();
    void on_currentPage_returnPressed();

    void on_scaleFactor_activated(int index);
    void on_scaleFactor_editingFinished();
    void on_scaleFactor_returnPressed();

    void on_open_triggered();
    void on_openInNewTab_triggered();
    void on_openCopyInNewTab_triggered();
    void on_openCopyInNewTab_triggered(const DocumentView* tab);
    void on_openCopyInNewWindow_triggered();
    void on_openCopyInNewWindow_triggered(const DocumentView* tab);
    void on_openContainingFolder_triggered();
    void on_openContainingFolder_triggered(const DocumentView* tab);
    void on_moveToInstance_triggered();
    void on_moveToInstance_triggered(DocumentView* tab);
    void on_refresh_triggered();
    void on_save_triggered();
    void on_saveAs_triggered();
    void on_saveCopy_triggered();
    void on_print_triggered();

    void on_recentlyUsed_openTriggered(const QString& filePath);

    void on_previousPage_triggered();
    void on_nextPage_triggered();
    void on_firstPage_triggered();
    void on_lastPage_triggered();

    void on_setFirstPage_triggered();

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

    void on_rightToLeftMode_triggered(bool checked);

    void on_zoomIn_triggered();
    void on_zoomOut_triggered();
    void on_originalSize_triggered();

    void on_fitToPageWidthMode_triggered(bool checked);
    void on_fitToPageSizeMode_triggered(bool checked);

    void on_rotateLeft_triggered();
    void on_rotateRight_triggered();

    void on_invertColors_triggered(bool checked);
    void on_convertToGrayscale_triggered(bool checked);
    void on_trimMargins_triggered(bool checked);
    void on_darkenWithPaperColor_triggered(bool checked);
    void on_lightenWithPaperColor_triggered(bool checked);

    void on_fonts_triggered();

    void on_fullscreen_triggered(bool checked);
    void on_presentation_triggered();

    void on_previousTab_triggered();
    void on_nextTab_triggered();
    void on_closeTab_triggered();
    void on_closeAllTabs_triggered();
    void on_closeAllTabsButCurrentTab_triggered();
    void on_closeAllTabsButThisOne_triggered(int thisIndex);
    void on_closeAllTabsToTheLeft_triggered(int ofIndex);
    void on_closeAllTabsToTheRight_triggered(int ofIndex);
    void on_closeTabs_triggered(const QVector< DocumentView* >& tabs);

    void on_restoreMostRecentlyClosedTab_triggered();

    void on_recentlyClosed_tabActionTriggered(QAction* tabAction);

    void on_tabAction_triggered();
    void on_tabShortcut_activated();

    void on_previousBookmark_triggered();
    void on_nextBookmark_triggered();
    void on_addBookmark_triggered();
    void on_removeBookmark_triggered();
    void on_removeAllBookmarks_triggered();

    void on_bookmarksMenu_aboutToShow();

    void on_bookmark_openTriggered(const QString& absoluteFilePath);
    void on_bookmark_openInNewTabTriggered(const QString& absoluteFilePath);
    void on_bookmark_jumpToPageTriggered(const QString& absoluteFilePath, int page);
    void on_bookmark_removeBookmarkTriggered(const QString& absoluteFilePath);

    void on_contents_triggered();
    void on_about_triggered();

    void on_focusCurrentPage_activated();
    void on_focusScaleFactor_activated();

    void on_toggleToolBars_triggered(bool checked);
    void on_toggleMenuBar_triggered(bool checked);

    void on_searchInitiated(const QString& text, bool modified);
    void on_highlightAll_clicked(bool checked);

    void on_dock_dockLocationChanged(Qt::DockWidgetArea area);

    void on_outline_sectionCountChanged();
    void on_outline_clicked(const QModelIndex& index);

    void on_properties_sectionCountChanged();

    void on_thumbnails_dockLocationChanged(Qt::DockWidgetArea area);
    void on_thumbnails_verticalScrollBar_valueChanged(int value);

    void on_bookmarks_sectionCountChanged();
    void on_bookmarks_clicked(const QModelIndex& index);
    void on_bookmarks_contextMenuRequested(QPoint pos);

    void on_search_sectionCountChanged();
    void on_search_dockLocationChanged(Qt::DockWidgetArea area);
    void on_search_visibilityChanged(bool visible);
    void on_search_clicked(const QModelIndex& index);
    void on_search_rowsInserted(const QModelIndex& parent, int first, int last);

    void on_saveDatabase_timeout();

protected:
    bool eventFilter(QObject* target, QEvent* event);

    void closeEvent(QCloseEvent* event);

    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent* event);

private:
    Q_DISABLE_COPY(MainWindow)

    static Settings* s_settings;
    static Database* s_database;
    static ShortcutHandler* s_shortcutHandler;
    static SearchModel* s_searchModel;

    void prepareStyle();

    TabWidget* m_tabWidget;

    DocumentView* currentTab() const;
    DocumentView* currentTab(int index) const;
    QVector< DocumentView* > allTabs(int index) const;
    QVector< DocumentView* > allTabs() const;

    bool senderIsCurrentTab() const;

    bool m_currentTabChangedBlocked;

    class CurrentTabChangeBlocker;

    void addTab(DocumentView* tab);
    void addTabAction(DocumentView* tab);
    void connectTab(DocumentView* tab);

    void restorePerFileSettings(DocumentView* tab);

    bool saveModifications(DocumentView* tab);
    void closeTab(DocumentView* tab);

    void setWindowTitleForCurrentTab();
    void setCurrentPageSuffixForCurrentTab();

    BookmarkModel* bookmarkModelForCurrentTab(bool create = false);

    QAction* sourceLinkActionForCurrentTab(QObject* parent, QPoint pos);

    class RestoreTab;

    QTimer* m_saveDatabaseTimer;

    void prepareDatabase();

    void scheduleSaveDatabase();
    void scheduleSaveTabs();
    void scheduleSaveBookmarks();
    void scheduleSavePerFileSettings();

    class TextValueMapper;

    MappingSpinBox* m_currentPageSpinBox;
    QWidgetAction* m_currentPageAction;

    ComboBox* m_scaleFactorComboBox;
    QWidgetAction* m_scaleFactorAction;

    SearchLineEdit* m_searchLineEdit;
    QCheckBox* m_matchCaseCheckBox;
    QCheckBox* m_wholeWordsCheckBox;
    QCheckBox* m_highlightAllCheckBox;
    QToolButton* m_findPreviousButton;
    QToolButton* m_findNextButton;
    QToolButton* m_cancelSearchButton;

    void createWidgets();

    QAction* m_openAction;
    QAction* m_openInNewTabAction;
    QAction* m_refreshAction;
    QAction* m_saveAction;
    QAction* m_saveAsAction;
    QAction* m_saveCopyAction;
    QAction* m_printAction;
    QAction* m_exitAction;

    QAction* m_previousPageAction;
    QAction* m_nextPageAction;
    QAction* m_firstPageAction;
    QAction* m_lastPageAction;

    QAction* m_setFirstPageAction;

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

    QAction* m_rightToLeftModeAction;

    QAction* m_zoomInAction;
    QAction* m_zoomOutAction;
    QAction* m_originalSizeAction;

    QAction* m_fitToPageWidthModeAction;
    QAction* m_fitToPageSizeModeAction;

    QAction* m_rotateLeftAction;
    QAction* m_rotateRightAction;

    QAction* m_invertColorsAction;
    QAction* m_convertToGrayscaleAction;
    QAction* m_trimMarginsAction;
    QAction* m_darkenWithPaperColorAction;
    QAction* m_lightenWithPaperColorAction;

    QAction* m_fontsAction;

    QAction* m_fullscreenAction;
    QAction* m_presentationAction;

    QAction* m_previousTabAction;
    QAction* m_nextTabAction;

    QAction* m_closeTabAction;
    QAction* m_closeAllTabsAction;
    QAction* m_closeAllTabsButCurrentTabAction;

    QAction* m_restoreMostRecentlyClosedTabAction;

    QShortcut* m_tabShortcuts[9];

    QAction* m_previousBookmarkAction;
    QAction* m_nextBookmarkAction;

    QAction* m_addBookmarkAction;
    QAction* m_removeBookmarkAction;
    QAction* m_removeAllBookmarksAction;

    QAction* m_contentsAction;
    QAction* m_aboutAction;

    QAction* m_openCopyInNewTabAction;
    QAction* m_openCopyInNewWindowAction;
    QAction* m_openContainingFolderAction;
    QAction* m_moveToInstanceAction;
    QAction* m_splitViewHorizontallyAction;
    QAction* m_splitViewVerticallyAction;
    QAction* m_closeCurrentViewAction;

    QAction* createAction(const QString& text, const QString& objectName, const QIcon& icon, const QList< QKeySequence >& shortcuts, const char* member, bool checkable = false, bool checked = false);
    QAction* createAction(const QString& text, const QString& objectName, const QIcon& icon, const QKeySequence& shortcut, const char* member, bool checkable = false, bool checked = false);
    QAction* createAction(const QString& text, const QString& objectName, const QString& iconName, const QList< QKeySequence >& shortcuts, const char* member, bool checkable = false, bool checked = false);
    QAction* createAction(const QString& text, const QString& objectName, const QString& iconName, const QKeySequence& shortcut, const char* member, bool checkable = false, bool checked = false);

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

    QDockWidget* m_bookmarksDock;
    QTableView* m_bookmarksView;

    QDockWidget* m_searchDock;
    QTreeView* m_searchView;
    QWidget* m_searchWidget;

    QDockWidget* createDock(const QString& text, const QString& objectName, const QKeySequence& toggleViewShortcut);

    void createSearchDock();

    void createDocks();

    QMenu* m_fileMenu;
    RecentlyUsedMenu* m_recentlyUsedMenu;
    QMenu* m_editMenu;
    QMenu* m_viewMenu;
    QMenu* m_compositionModeMenu;
    SearchableMenu* m_tabsMenu;
    RecentlyClosedMenu* m_recentlyClosedMenu;
    SearchableMenu* m_bookmarksMenu;
    QMenu* m_helpMenu;

    bool m_bookmarksMenuIsDirty;

    void createMenus();

    int m_tabBarHadPolicy;

    bool m_fileToolBarWasVisible;
    bool m_editToolBarWasVisible;
    bool m_viewToolBarWasVisible;

    QAction* m_toggleToolBarsAction;
    QAction* m_toggleMenuBarAction;

    QPointer< HelpDialog > m_helpDialog;

};

#ifdef WITH_DBUS

class MainWindowAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "local.qpdfview.MainWindow")

    explicit MainWindowAdaptor(MainWindow* mainWindow);

public:
    static QDBusInterface* createInterface(const QString& instanceName = QString());
    static MainWindowAdaptor* createAdaptor(MainWindow* mainWindow);

public slots:
    Q_NOREPLY void raiseAndActivate();

    bool open(const QString& absoluteFilePath, int page = -1, const QRectF& highlight = QRectF(), bool quiet = false);
    bool openInNewTab(const QString& absoluteFilePath, int page = -1, const QRectF& highlight = QRectF(), bool quiet = false);

    bool jumpToPageOrOpenInNewTab(const QString& absoluteFilePath, int page = -1, bool refreshBeforeJump = false, const QRectF& highlight = QRectF(), bool quiet = false);

    Q_NOREPLY void startSearch(const QString& text);

    Q_NOREPLY void saveDatabase();


    int currentPage() const;
    Q_NOREPLY void jumpToPage(int page);

    Q_NOREPLY void previousPage();
    Q_NOREPLY void nextPage();
    Q_NOREPLY void firstPage();
    Q_NOREPLY void lastPage();

    Q_NOREPLY void previousBookmark();
    Q_NOREPLY void nextBookmark();

    bool jumpToBookmark(const QString& label);


    Q_NOREPLY void continuousMode(bool checked);
    Q_NOREPLY void twoPagesMode(bool checked);
    Q_NOREPLY void twoPagesWithCoverPageMode(bool checked);
    Q_NOREPLY void multiplePagesMode(bool checked);

    Q_NOREPLY void fitToPageWidthMode(bool checked);
    Q_NOREPLY void fitToPageSizeMode(bool checked);

    Q_NOREPLY void invertColors(bool checked);
    Q_NOREPLY void convertToGrayscale(bool checked);
    Q_NOREPLY void trimMargins(bool checked);

    Q_NOREPLY void fullscreen(bool checked);
    Q_NOREPLY void presentation();


    Q_NOREPLY void closeTab();
    Q_NOREPLY void closeAllTabs();
    Q_NOREPLY void closeAllTabsButCurrentTab();

    bool closeTab(const QString& absoluteFilePath);

private:
    MainWindow* mainWindow() const;

    static QString serviceName(QString instanceName = QString());

};

#endif // WITH_DBUS

} // qpdfview

#endif // MAINWINDOW_H
