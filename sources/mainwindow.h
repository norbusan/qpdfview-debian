/*

Copyright 2012 Adam Reichold

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

#include <QtCore>
#include <QtXml>
#include <QtGui>

#ifdef WITH_DBUS

#include <QtDBus>

#endif // WITH_DBUS

#include "documentview.h"
#include "miscellaneous.h"
#include "settingsdialog.h"
#include "bookmarkmenu.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget* parent = 0);

    QSize sizeHint() const;
    QMenu* createPopupMenu();

public slots:
    bool open(const QString& filePath, int page = 0);
    bool openInNewTab(const QString& filePath, int page = 0);

    bool refreshOrOpenInNewTab(const QString& filePath, int page = 0);

protected slots:
    void on_tabWidget_currentChanged(int index);
    void on_tabWidget_tabCloseRequested(int index);

    void on_currentTab_filePathChanged(const QString& filePath);
    void on_currentTab_numberOfPagesChaned(int numberOfPages);
    void on_currentTab_currentPageChanged(int currentPage);

    void on_currentTab_continuousModeChanged(bool continuousMode);
    void on_currentTab_twoPagesModeChanged(bool twoPagesMode);
    void on_currentTab_scaleModeChanged(DocumentView::ScaleMode scaleMode);
    void on_currentTab_scaleFactorChanged(qreal scaleFactor);
    void on_currentTab_rotationChanged(Poppler::Page::Rotation rotation);

    void on_currentTab_highlightAllChanged(bool highlightAll);

    void on_currentTab_searchProgressed(int progress);
    void on_currentTab_searchFinished();
    void on_currentTab_searchCanceled();

    void on_currentPage_valueChanged(int value);
    void on_currentPage_returnPressed();

    void on_scaleFactor_currentIndexChanged(int index);
    void on_scaleFactor_editingFinished();
    void on_scaleFactor_returnPressed();

    void on_open_triggered();
    void on_openInNewTab_triggered();
    void on_refresh_triggered();
    void on_saveCopy_triggered();
    void on_print_triggered();

    void on_previousPage_triggered();
    void on_nextPage_triggered();
    void on_firstPage_triggered();
    void on_lastPage_triggered();

    void on_jumpToPage_triggered();

    void on_search_triggered();
    void on_findPrevious_triggered();
    void on_findNext_triggered();
    void on_cancelSearch_triggered();

    void on_search_timeout();

    void on_settings_triggered();

    void on_continuousMode_triggered(bool checked);
    void on_twoPagesMode_triggered(bool checked);

    void on_zoomIn_triggered();
    void on_zoomOut_triggered();
    void on_originalSize_triggered();
    void on_fitToPageWidth_triggered(bool checked);
    void on_fitToPageSize_triggered(bool checked);

    void on_rotateLeft_triggered();
    void on_rotateRight_triggered();

    void on_fonts_triggered();

    void on_fullscreen_triggered(bool checked);
    void on_presentation_triggered();

    void on_previousTab_triggered();
    void on_nextTab_triggered();
    void on_closeTab_triggered();
    void on_closeAllTabs_triggered();
    void on_closeAllTabsButCurrentTab_triggered();

    void on_tab_triggered();

    void on_addBookmark_triggered();
    void on_removeAllBookmarks_triggered();

    void on_bookmark_openTriggered(const QString& filePath);
    void on_bookmark_openInNewTabTriggered(const QString& filePath);
    void on_bookmark_jumpToPageTriggered(const QString& filePath, int page);

    void on_contents_triggered();
    void on_about_triggered();

    void on_highlightAll_clicked(bool checked);

    void on_outline_clicked(const QModelIndex& index);

protected:
    void closeEvent(QCloseEvent* event);

    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent* event);

private:
    QSettings* m_settings;

    TabWidget* m_tabWidget;

    DocumentView* currentTab() const;
    DocumentView* tab(int index) const;

    bool senderIsCurrentTab() const;

    SpinBox* m_currentPageSpinBox;
    ComboBox* m_scaleFactorComboBox;

    ProgressLineEdit* m_searchLineEdit;
    QTimer* m_searchTimer;
    QCheckBox* m_matchCaseCheckBox;
    QCheckBox* m_highlightAllCheckBox;

    void createWidgets();

    QAction* m_openAction;
    QAction* m_openInNewTabAction;
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

    QAction* m_continuousModeAction;
    QAction* m_twoPagesModeAction;

    QAction* m_zoomInAction;
    QAction* m_zoomOutAction;
    QAction* m_originalSizeAction;
    QAction* m_fitToPageWidthAction;
    QAction* m_fitToPageSizeAction;

    QAction* m_rotateLeftAction;
    QAction* m_rotateRightAction;

    QAction* m_fontsAction;

    QAction* m_fullscreenAction;
    QAction* m_presentationAction;

    QAction* m_previousTabAction;
    QAction* m_nextTabAction;
    QAction* m_closeTabAction;
    QAction* m_closeAllTabsAction;
    QAction* m_closeAllTabsButCurrentTabAction;

    QAction* m_addBookmarkAction;
    QAction* m_removeAllBookmarksAction;

    QAction* m_contentsAction;
    QAction* m_aboutAction;

    void createActions();

    QToolBar* m_fileToolBar;
    QToolBar* m_editToolBar;
    QToolBar* m_viewToolBar;

    QToolBar* m_searchToolBar;

    void createToolBars();

    QDockWidget* m_outlineDock;
    QTreeView* m_outlineView;

    QDockWidget* m_propertiesDock;
    QTableView* m_propertiesView;

    QDockWidget* m_thumbnailsDock;
    QGraphicsView* m_thumbnailsView;

    void createDocks();

    QMenu* m_fileMenu;
    QMenu* m_editMenu;
    QMenu* m_viewMenu;
    QMenu* m_tabsMenu;
    QMenu* m_bookmarksMenu;
    QMenu* m_helpMenu;

    void createMenus();

    void restoreTabs();
    void saveTabs();

    void restoreBookmarks();
    void saveBookmarks();

};

#ifdef WITH_DBUS

class MainWindowAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "local.qpdfview.MainWindow")

public:
    MainWindowAdaptor(MainWindow* mainWindow);

    MainWindow* mainWindow() const;

public slots:
    bool open(const QString& filePath, int page = 0);
    bool openInNewTab(const QString& filePath, int page = 0);

    bool refreshOrOpenInNewTab(const QString& filePath, int page = 0);

};

#endif // WITH_DBUS

#endif // MAINWINDOW_H
