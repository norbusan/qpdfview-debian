/*

Copyright 2015 S. Razi Alavizadeh
Copyright 2012-2015, 2018 Adam Reichold
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

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QColor>
#include <QIcon>
#include <QKeySequence>
#include <QObject>
#include <QPrinter>

class QSettings;

#include "global.h"
#include "printoptions.h"

namespace qpdfview
{

class Settings : public QObject
{
    Q_OBJECT

public:
    static Settings* instance();
    ~Settings();

    // page item

    class PageItem
    {
    public:
        void sync();

        int cacheSize() const { return m_cacheSize; }
        void setCacheSize(int cacheSize);

        bool useTiling() const { return m_useTiling; }
        void setUseTiling(bool useTiling);

        int tileSize() const { return m_tileSize; }

        const QIcon& progressIcon() const { return m_progressIcon; }
        void setProgressIcon(const QIcon& progressIcon) { m_progressIcon = progressIcon; }

        const QIcon& errorIcon() const { return m_errorIcon; }
        void setErrorIcon(const QIcon& errorIcon) { m_errorIcon = errorIcon; }

        bool keepObsoletePixmaps() const { return m_keepObsoletePixmaps; }
        void setKeepObsoletePixmaps(bool keepObsoletePixmaps);

        bool useDevicePixelRatio() const { return m_useDevicePixelRatio; }
        void setUseDevicePixelRatio(bool useDevicePixelRatio);

        bool decoratePages() const { return m_decoratePages; }
        void setDecoratePages(bool decoratePages);

        bool decorateLinks() const { return m_decorateLinks; }
        void setDecorateLinks(bool decorateLinks);

        bool decorateFormFields() const { return m_decorateFormFields; }
        void setDecorateFormFields(bool decorateFormFields);

        const QColor& backgroundColor() const { return m_backgroundColor; }
        void setBackgroundColor(const QColor& backgroundColor);

        const QColor& paperColor() const { return m_paperColor; }
        void setPaperColor(const QColor& paperColor);

        const QColor& highlightColor() const { return m_highlightColor; }
        void setHighlightColor(const QColor& highlightColor);

        QColor annotationColor() const;
        void setAnnotationColor(const QColor& annotationColor);

        Qt::KeyboardModifiers copyToClipboardModifiers() const;
        void setCopyToClipboardModifiers(Qt::KeyboardModifiers modifiers);

        Qt::KeyboardModifiers addAnnotationModifiers() const;
        void setAddAnnotationModifiers(Qt::KeyboardModifiers modifiers);

        Qt::KeyboardModifiers zoomToSelectionModifiers() const;
        void setZoomToSelectionModifiers(Qt::KeyboardModifiers modifiers);

        Qt::KeyboardModifiers openInSourceEditorModifiers() const;
        void setOpenInSourceEditorModifiers(Qt::KeyboardModifiers modifiers);

        bool annotationOverlay() const;
        void setAnnotationOverlay(bool overlay);

        bool formFieldOverlay() const;
        void setFormFieldOverlay(bool overlay);

    private:
        PageItem(QSettings* settings);
        friend class Settings;

        QSettings* m_settings;

        int m_cacheSize;

        bool m_useTiling;
        int m_tileSize;

        QIcon m_progressIcon;
        QIcon m_errorIcon;

        bool m_keepObsoletePixmaps;
        bool m_useDevicePixelRatio;

        bool m_decoratePages;
        bool m_decorateLinks;
        bool m_decorateFormFields;

        QColor m_backgroundColor;
        QColor m_paperColor;

        QColor m_highlightColor;

    };

    // presentation view

    class PresentationView
    {
    public:
        bool synchronize() const;
        void setSynchronize(bool synchronize);

        int screen() const;
        void setScreen(int screen);

        QColor backgroundColor() const;
        void setBackgroundColor(const QColor& backgroundColor);

    private:
        PresentationView(QSettings* settings);
        friend class Settings;

        QSettings* m_settings;

    };

    // document view

    class DocumentView
    {
    public:
        void sync();

        bool openUrl() const;
        void setOpenUrl(bool openUrl);

        bool autoRefresh() const;
        void setAutoRefresh(bool autoRefresh);

        int autoRefreshTimeout() const;

        bool prefetch() const { return m_prefetch; }
        void setPrefetch(bool prefetch);

        int prefetchDistance() const { return m_prefetchDistance; }
        void setPrefetchDistance(int prefetchDistance);

        int prefetchTimeout() const;

        int pagesPerRow() const { return m_pagesPerRow; }
        void setPagesPerRow(int pagesPerRow);

        bool minimalScrolling() const { return m_minimalScrolling; }
        void setMinimalScrolling(bool minimalScrolling);

        bool highlightCurrentThumbnail() const { return m_highlightCurrentThumbnail; }
        void setHighlightCurrentThumbnail(bool highlightCurrentThumbnail);

        bool limitThumbnailsToResults() const { return m_limitThumbnailsToResults; }
        void setLimitThumbnailsToResults(bool limitThumbnailsToResults);

        qreal minimumScaleFactor() const;
        qreal maximumScaleFactor() const;

        qreal zoomFactor() const;
        void setZoomFactor(qreal zoomFactor);

        qreal pageSpacing() const { return m_pageSpacing; }
        void setPageSpacing(qreal pageSpacing);

        qreal thumbnailSpacing() const { return m_thumbnailSpacing; }
        void setThumbnailSpacing(qreal thumbnailSpacing);

        qreal thumbnailSize() const { return m_thumbnailSize; }
        void setThumbnailSize(qreal thumbnailSize);

        bool matchCase() const;
        void setMatchCase(bool matchCase);

        bool wholeWords() const;
        void setWholeWords(bool wholeWords);

        bool parallelSearchExecution() const;
        void setParallelSearchExecution(bool parallelSearchExecution);

        int highlightDuration() const;
        void setHighlightDuration(int highlightDuration);

        QString sourceEditor() const;
        void setSourceEditor(const QString& sourceEditor);

        Qt::KeyboardModifiers zoomModifiers() const;
        void setZoomModifiers(Qt::KeyboardModifiers zoomModifiers);

        Qt::KeyboardModifiers rotateModifiers() const;
        void setRotateModifiers(Qt::KeyboardModifiers rotateModifiers);

        Qt::KeyboardModifiers scrollModifiers() const;
        void setScrollModifiers(Qt::KeyboardModifiers scrollModifiers);

        // per-tab settings

        bool continuousMode() const;
        void setContinuousMode(bool continuousMode);

        LayoutMode layoutMode() const;
        void setLayoutMode(LayoutMode layoutMode);

        bool rightToLeftMode() const;
        void setRightToLeftMode(bool rightToLeftMode);

        ScaleMode scaleMode() const;
        void setScaleMode(ScaleMode scaleMode);

        qreal scaleFactor() const;
        void setScaleFactor(qreal scaleFactor);

        Rotation rotation() const;
        void setRotation(Rotation rotation);

        bool invertColors() const;
        void setInvertColors(bool invertColors);

        bool convertToGrayscale() const;
        void setConvertToGrayscale(bool convertToGrayscale);

        bool trimMargins() const;
        void setTrimMargins(bool trimMargins);

        CompositionMode compositionMode() const;
        void setCompositionMode(CompositionMode compositionMode);

        bool highlightAll() const;
        void setHighlightAll(bool highlightAll);

    private:
        DocumentView(QSettings* settings);
        friend class Settings;

        QSettings* m_settings;

        bool m_prefetch;
        int m_prefetchDistance;

        int m_pagesPerRow;

        bool m_minimalScrolling;

        bool m_highlightCurrentThumbnail;
        bool m_limitThumbnailsToResults;

        qreal m_pageSpacing;
        qreal m_thumbnailSpacing;

        qreal m_thumbnailSize;

    };

    // main window

    class MainWindow
    {
    public:
        bool trackRecentlyUsed() const;
        void setTrackRecentlyUsed(bool trackRecentlyUsed);

        int recentlyUsedCount() const;
        void setRecentlyUsedCount(int recentlyUsedCount);

        QStringList recentlyUsed() const;
        void setRecentlyUsed(const QStringList& recentlyUsed);

        bool keepRecentlyClosed() const;
        void setKeepRecentlyClosed(bool keepRecentlyClosed);

        int recentlyClosedCount() const;
        void setRecentlyClosedCount(int recentlyClosedCount);

        bool restoreTabs() const;
        void setRestoreTabs(bool restoreTabs);

        bool restoreBookmarks() const;
        void setRestoreBookmarks(bool restoreBookmarks);

        bool restorePerFileSettings() const;
        void setRestorePerFileSettings(bool restorePerFileSettings);

        int perFileSettingsLimit() const;

        int saveDatabaseInterval() const;
        void setSaveDatabaseInterval(int saveDatabaseInterval);

        int currentTabIndex() const;
        void setCurrentTabIndex(int currentTabIndex);

        int tabPosition() const;
        void setTabPosition(int tabPosition);

        int tabVisibility() const;
        void setTabVisibility(int tabVisibility);

        bool spreadTabs() const;
        void setSpreadTabs(bool spreadTabs);

        bool newTabNextToCurrentTab() const;
        void setNewTabNextToCurrentTab(bool newTabNextToCurrentTab);

        bool exitAfterLastTab() const;
        void setExitAfterLastTab(bool exitAfterLastTab);

        bool documentTitleAsTabTitle() const;
        void setDocumentTitleAsTabTitle(bool documentTitleAsTabTitle);

        bool currentPageInWindowTitle() const;
        void setCurrentPageInWindowTitle(bool currentPageInWindowTitle);

        bool instanceNameInWindowTitle() const;
        void setInstanceNameInWindowTitle(bool instanceNameInWindowTitle);

        bool extendedSearchDock() const;
        void setExtendedSearchDock(bool extendedSearchDock);

        bool usePageLabel() const;
        void setUsePageLabel(bool usePageLabel);

        bool synchronizeOutlineView() const;
        void setSynchronizeOutlineView(bool synchronizeOutlineView);

        bool synchronizeSplitViews() const;
        void setSynchronizeSplitViews(bool synchronizeSplitViews);

        QStringList fileToolBar() const;
        void setFileToolBar(const QStringList& fileToolBar);

        QStringList editToolBar() const;
        void setEditToolBar(const QStringList& editToolBar);

        QStringList viewToolBar() const;
        void setViewToolBar(const QStringList& viewToolBar);

        QStringList documentContextMenu() const;
        void setDocumentContextMenu(const QStringList& documentContextMenu);

        QStringList tabContextMenu() const;
        void setTabContextMenu(const QStringList& tabContextMenu);

        bool scrollableMenus() const;
        void setScrollableMenus(bool scrollableMenus);

        bool searchableMenus() const;
        void setSearchableMenus(bool searchableMenus);

        bool toggleToolAndMenuBarsWithFullscreen() const;
        void setToggleToolAndMenuBarsWithFullscreen(bool toggleToolAndMenuBarsWithFullscreen) const;

        bool hasIconTheme() const;
        QString iconTheme() const;

        bool hasStyleSheet() const;
        QString styleSheet() const;

        QByteArray geometry() const;
        void setGeometry(const QByteArray& geometry);

        QByteArray state() const;
        void setState(const QByteArray& state);

        QString openPath() const;
        void setOpenPath(const QString& openPath);

        QString savePath() const;
        void setSavePath(const QString& savePath);

        QSize settingsDialogSize(QSize sizeHint) const;
        void setSettingsDialogSize(QSize settingsDialogSize);

        QSize fontsDialogSize(QSize sizeHint) const;
        void setFontsDialogSize(QSize fontsDialogSize);

        QSize contentsDialogSize(QSize sizeHint) const;
        void setContentsDialogSize(QSize contentsDialogSize);

    private:
        MainWindow(QSettings* settings);
        friend class Settings;

        QSettings* m_settings;

    };

    // print dialog

    class PrintDialog
    {
    public:
        bool collateCopies() const;
        void setCollateCopies(bool collateCopies);

        QPrinter::PageOrder pageOrder() const;
        void setPageOrder(QPrinter::PageOrder pageOrder);

        QPrinter::Orientation orientation() const;
        void setOrientation(QPrinter::Orientation orientation);

        QPrinter::ColorMode colorMode() const;
        void setColorMode(QPrinter::ColorMode colorMode);

        QPrinter::DuplexMode duplex() const;
        void setDuplex(QPrinter::DuplexMode duplex);

        bool fitToPage() const;
        void setFitToPage(bool fitToPage);

#if QT_VERSION < QT_VERSION_CHECK(5,2,0)

        PrintOptions::PageSet pageSet() const;
        void setPageSet(PrintOptions::PageSet pageSet);

        PrintOptions::NumberUp numberUp() const;
        void setNumberUp(PrintOptions::NumberUp numberUp);

        PrintOptions::NumberUpLayout numberUpLayout() const;
        void setNumberUpLayout(PrintOptions::NumberUpLayout numberUpLayout);

#endif // QT_VERSION

    private:
        PrintDialog(QSettings* settings);
        friend class Settings;

        QSettings* m_settings;

    };

    void sync();

    PageItem& pageItem() { return m_pageItem; }
    PresentationView& presentationView() { return m_presentationView; }
    DocumentView& documentView() { return m_documentView; }
    MainWindow& mainWindow() { return m_mainWindow; }
    PrintDialog& printDialog() { return m_printDialog; }

private:
    Q_DISABLE_COPY(Settings)

    static Settings* s_instance;
    Settings(QObject* parent = 0);

    QSettings* m_settings;

    PageItem m_pageItem;
    PresentationView m_presentationView;
    DocumentView m_documentView;
    MainWindow m_mainWindow;
    PrintDialog m_printDialog;

};

// defaults

class Defaults
{
public:
    class PageItem
    {
    public:
        static int cacheSize() { return 32 * 1024; }

        static bool useTiling() { return false; }
        static int tileSize() { return 1024; }

        static bool keepObsoletePixmaps() { return false; }
        static bool useDevicePixelRatio() { return false; }

        static bool decoratePages() { return true; }
        static bool decorateLinks() { return true; }
        static bool decorateFormFields() { return true; }

        static QColor backgroundColor() { return Qt::darkGray; }
        static QColor paperColor() { return Qt::white; }

        static QColor highlightColor() { return Qt::yellow; }
        static QColor annotationColor() { return Qt::yellow; }

        static Qt::KeyboardModifiers copyToClipboardModifiers() { return Qt::ShiftModifier; }
        static Qt::KeyboardModifiers addAnnotationModifiers() { return Qt::ControlModifier; }
        static Qt::KeyboardModifiers zoomToSelectionModifiers() { return Qt::ShiftModifier | Qt::ControlModifier; }
        static Qt::KeyboardModifiers openInSourceEditorModifiers() { return Qt::NoModifier; }

        static bool annotationOverlay() { return false; }
        static bool formFieldOverlay() { return true; }

    private:
        PageItem() {}

    };

    class PresentationView
    {
    public:
        static bool synchronize() { return false; }
        static int screen() { return -1; }

        static QColor backgroundColor() { return QColor(); }

    private:
        PresentationView() {}

    };

    class DocumentView
    {
    public:
        static bool openUrl() { return false; }

        static bool autoRefresh() { return false; }

        static int autoRefreshTimeout() { return 750; }

        static bool prefetch() { return false; }
        static int prefetchDistance() { return 1; }

        static int prefetchTimeout() { return 250; }

        static int pagesPerRow() { return 3; }

        static bool minimalScrolling() { return false; }

        static bool highlightCurrentThumbnail() { return false; }
        static bool limitThumbnailsToResults() { return false; }

        static qreal minimumScaleFactor() { return 0.1; }
        static qreal maximumScaleFactor() { return 50.0; }

        static qreal zoomFactor() { return 1.1; }

        static qreal pageSpacing() { return 5.0; }
        static qreal thumbnailSpacing() { return 3.0; }

        static qreal thumbnailSize() { return 150.0; }

        static CompositionMode compositionMode() { return DefaultCompositionMode; }

        static bool matchCase() { return false; }
        static bool wholeWords() { return false; }
        static bool parallelSearchExecution() { return false; }

        static int highlightDuration() { return 5 * 1000; }
        static QString sourceEditor() { return QString(); }

        static Qt::KeyboardModifiers zoomModifiers() { return Qt::ControlModifier; }
        static Qt::KeyboardModifiers rotateModifiers() { return Qt::ShiftModifier; }
        static Qt::KeyboardModifiers scrollModifiers() { return Qt::AltModifier; }

        // per-tab defaults

        static bool continuousMode() { return false; }
        static LayoutMode layoutMode() { return SinglePageMode; }
        static bool rightToLeftMode();

        static ScaleMode scaleMode() { return ScaleFactorMode; }
        static qreal scaleFactor() { return 1.0; }
        static Rotation rotation() { return RotateBy0; }

        static bool invertColors() { return false; }
        static bool convertToGrayscale() { return false; }
        static bool trimMargins() { return false; }

        static bool highlightAll() { return false; }

    private:
        DocumentView() {}

    };

    class MainWindow
    {
    public:
        static bool trackRecentlyUsed() { return false; }
        static int recentlyUsedCount() { return 10; }

        static bool keepRecentlyClosed() { return false; }
        static int recentlyClosedCount() { return 5; }

        static bool restoreTabs() { return false; }
        static bool restoreBookmarks() { return false; }
        static bool restorePerFileSettings() { return false; }

        static int perFileSettingsLimit() { return 1000; }

        static int saveDatabaseInterval() { return 5 * 60 * 1000; }

        static int tabPosition() { return 0; }
        static int tabVisibility() { return 0; }

        static bool spreadTabs() { return false; }

        static bool newTabNextToCurrentTab() { return true; }
        static bool exitAfterLastTab() { return false; }

        static bool documentTitleAsTabTitle() { return true; }

        static bool currentPageInWindowTitle() { return false; }
        static bool instanceNameInWindowTitle() { return false; }

        static bool extendedSearchDock() { return false; }

        static bool usePageLabel() { return true; }

        static bool synchronizeOutlineView() { return false; }
        static bool synchronizeSplitViews() { return true; }

        static QStringList fileToolBar() { return QStringList() << "openInNewTab" << "refresh"; }
        static QStringList editToolBar() { return QStringList() << "currentPage" << "previousPage" << "nextPage"; }
        static QStringList viewToolBar() { return QStringList() << "scaleFactor" << "zoomIn" << "zoomOut"; }

        static QStringList documentContextMenu() { return QStringList() << "previousPage" << "nextPage" << "firstPage" << "lastPage" << "separator" << "jumpToPage" << "jumpBackward" << "jumpForward" << "separator" << "setFirstPage" << "separator" << "findPrevious" << "findNext" << "cancelSearch"; }
        static QStringList tabContexntMenu() { return QStringList() << "openCopyInNewTab" << "openCopyInNewWindow" << "openContainingFolder" << "separator" << "splitViewHorizontally" << "splitViewVertically" << "closeCurrentView" << "separator" << "closeAllTabs" << "closeAllTabsButThisOne" << "closeAllTabsToTheLeft" << "closeAllTabsToTheRight"; }

        static bool scrollableMenus() { return false; }
        static bool searchableMenus() { return false; }

        static bool toggleToolAndMenuBarsWithFullscreen() { return false; }

        static QString path();

    private:
        MainWindow() {}

    };

    class PrintDialog
    {
    public:
        static bool collateCopies() { return false; }

        static QPrinter::PageOrder pageOrder() { return QPrinter::FirstPageFirst; }

        static QPrinter::Orientation orientation() { return QPrinter::Portrait; }

        static QPrinter::ColorMode colorMode() { return QPrinter::Color; }

        static QPrinter::DuplexMode duplex() { return QPrinter::DuplexNone; }

        static bool fitToPage() { return false; }

#if QT_VERSION < QT_VERSION_CHECK(5,2,0)

        static PrintOptions::PageSet pageSet() { return PrintOptions::AllPages; }

        static PrintOptions::NumberUp numberUp() { return PrintOptions::SinglePage; }
        static PrintOptions::NumberUpLayout numberUpLayout() { return PrintOptions::LeftRightTopBottom; }

#endif // QT_VERSION

    private:
        PrintDialog() {}

    };

private:
    Defaults() {}

};

} // qpdfview

#endif // SETTINGS_H
