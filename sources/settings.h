/*

Copyright 2012-2013 Adam Reichold
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

        inline int cacheSize() const { return m_cacheSize; }
        void setCacheSize(int cacheSize);

        inline const QIcon& progressIcon() const { return m_progressIcon; }
        inline const QIcon& errorIcon() const { return m_errorIcon; }

        inline bool keepObsoletePixmaps() const { return m_keepObsoletePixmaps; }
        void setKeepObsoletePixmaps(bool keepObsoletePixmaps);

        inline bool useDevicePixelRatio() const { return m_useDevicePixelRatio; }
        void setUseDevicePixelRatio(bool useDevicePixelRatio);

        inline bool decoratePages() const { return m_decoratePages; }
        void setDecoratePages(bool decorate);

        inline bool decorateLinks() const { return m_decorateLinks; }
        void setDecorateLinks(bool decorate);

        inline bool decorateFormFields() const { return m_decorateFormFields; }
        void setDecorateFormFields(bool decorate);

        inline const QColor& backgroundColor() const { return m_backgroundColor; }
        void setBackgroundColor(const QColor& color);

        inline const QColor& paperColor() const { return m_paperColor; }
        void setPaperColor(const QColor& color);

        inline const QColor& highlightColor() const { return m_highlightColor; }
        void setHighlightColor(const QColor& color);

        QColor annotationColor() const;
        void setAnnotationColor(const QColor& color);

        Qt::KeyboardModifiers copyToClipboardModifiers() const;
        void setCopyToClipboardModifiers(const Qt::KeyboardModifiers& modifiers);

        Qt::KeyboardModifiers addAnnotationModifiers() const;
        void setAddAnnotationModifiers(const Qt::KeyboardModifiers& modifiers);

        bool annotationOverlay() const;
        void setAnnotationOverlay(bool overlay);

        bool formFieldOverlay() const;
        void setFormFieldOverlay(bool overlay);

    private:
        PageItem(QSettings* settings);
        friend class Settings;

        QSettings* m_settings;

        int m_cacheSize;

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
        bool sync() const;
        void setSync(bool sync);

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

        inline bool prefetch() const { return m_prefetch; }
        void setPrefetch(bool prefetch);

        inline int prefetchDistance() const { return m_prefetchDistance; }
        void setPrefetchDistance(int prefetchDistance);

        int autoRefreshTimeout() const;
        int prefetchTimeout() const;

        inline int pagesPerRow() const { return m_pagesPerRow; }
        void setPagesPerRow(int pagesPerRow);

        inline bool highlightCurrentThumbnail() const { return m_highlightCurrentThumbnail; }
        void setHighlightCurrentThumbnail(bool highlightCurrentThumbnail);

        inline bool limitThumbnailsToResults() const { return m_limitThumbnailsToResults; }
        void setLimitThumbnailsToResults(bool limitThumbnailsToResults);

        inline qreal pageSpacing() const { return m_pageSpacing; }
        void setPageSpacing(qreal pageSpacing);

        inline qreal thumbnailSpacing() const { return m_thumbnailSpacing; }
        void setThumbnailSpacing(qreal thumbnailSpacing);

        inline qreal thumbnailSize() const { return m_thumbnailSize; }
        void setThumbnailSize(qreal thumbnailSize);

        bool matchCase() const;
        void setMatchCase(bool matchCase);

        int highlightDuration() const;
        void setHighlightDuration(int highlightDuration);

        QString sourceEditor() const;
        void setSourceEditor(const QString& sourceEditor);

        Qt::KeyboardModifiers zoomModifiers() const;
        void setZoomModifiers(const Qt::KeyboardModifiers& zoomModifiers);

        Qt::KeyboardModifiers rotateModifiers() const;
        void setRotateModifiers(const Qt::KeyboardModifiers& rotateModifiers);

        Qt::KeyboardModifiers scrollModifiers() const;
        void setScrollModifiers(const Qt::KeyboardModifiers& scrollModifiers);

        // per-tab settings

        bool continuousMode() const;
        void setContinuousMode(bool continuousMode);

        LayoutMode layoutMode() const;
        void setLayoutMode(LayoutMode layoutMode);

        ScaleMode scaleMode() const;
        void setScaleMode(ScaleMode scaleMode);

        qreal scaleFactor() const;
        void setScaleFactor(qreal scaleFactor);

        Rotation rotation() const;
        void setRotation(Rotation rotation);

        bool invertColors() const;
        void setInvertColors(bool invertColors);

        bool highlightAll() const;
        void setHighlightAll(bool highlightAll);

    private:
        DocumentView(QSettings* settings);
        friend class Settings;

        QSettings* m_settings;

        bool m_prefetch;
        int m_prefetchDistance;

        int m_pagesPerRow;

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
        void setTrackRecentlyUsed(bool on);

        int recentlyUsedCount() const;
        void setRecentlyUsedCount(int recentlyUsedCount);

        QStringList recentlyUsed() const;
        void setRecentlyUsed(const QStringList& recentlyUsed);

        bool restoreTabs() const;
        void setRestoreTabs(bool on);

        bool restoreBookmarks() const;
        void setRestoreBookmarks(bool on);

        bool restorePerFileSettings() const;
        void setRestorePerFileSettings(bool on);

        int tabPosition() const;
        void setTabPosition(int tabPosition);

        int tabVisibility() const;
        void setTabVisibility(int tabVisibility);

        bool newTabNextToCurrentTab() const;
        void setNewTabNextToCurrentTab(bool newTabNextToCurrentTab);

        bool currentPageInWindowTitle() const;
        void setCurrentPageInWindowTitle(bool currentPageInWindowTitle);

        bool instanceNameInWindowTitle() const;
        void setInstanceNameInWindowTitle(bool instanceNameInWindowTitle);

        bool synchronizeOutlineView() const;
        void setSynchronizeOutlineView(bool synchronizeOutlineView);

        QStringList fileToolBar() const;
        void setFileToolBar(const QStringList& fileToolBar);

        QStringList editToolBar() const;
        void setEditToolBar(const QStringList& editToolBar);

        QStringList viewToolBar() const;
        void setViewToolBar(const QStringList& viewToolBar);

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

        QSize settingsDialogSize(const QSize& sizeHint) const;
        void setSettingsDialogSize(const QSize& settingsDialogSize);

        QSize fontsDialogSize(const QSize& sizeHint) const;
        void setFontsDialogSize(const QSize& fontsDialogSize);

        QSize contentsDialogSize(const QSize& sizeHint) const;
        void setContentsDialogSize(const QSize& contentsDialogSize);

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

        PrintOptions::PageSet pageSet() const;
        void setPageSet(PrintOptions::PageSet pageSet);

        PrintOptions::NumberUp numberUp() const;
        void setNumberUp(PrintOptions::NumberUp numberUp);

        PrintOptions::NumberUpLayout numberUpLayout() const;
        void setNumberUpLayout(PrintOptions::NumberUpLayout numberUpLayout);

    private:
        PrintDialog(QSettings* settings);
        friend class Settings;

        QSettings* m_settings;

    };

    void sync();

    PageItem& pageItem();
    PresentationView& presentationView();
    DocumentView& documentView();
    MainWindow& mainWindow();
    PrintDialog& printDialog();

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
        static inline int cacheSize() { return 32 * 1024 * 1024; }

        static inline bool keepObsoletePixmaps() { return false; }
        static inline bool useDevicePixelRatio() { return false; }

        static inline bool decoratePages() { return true; }
        static inline bool decorateLinks() { return true; }
        static inline bool decorateFormFields() { return true; }

        static inline QColor backgroundColor() { return Qt::darkGray; }
        static inline QColor paperColor() { return Qt::white; }

        static inline QColor highlightColor() { return Qt::yellow; }
        static inline QColor annotationColor() { return Qt::yellow; }

        static inline Qt::KeyboardModifiers copyToClipboardModifiers() { return Qt::ShiftModifier; }
        static inline Qt::KeyboardModifiers addAnnotationModifiers() { return Qt::ControlModifier; }

        static inline bool annotationOverlay() { return false; }
        static inline bool formFieldOverlay() { return true; }

    private:
        PageItem() {}

    };

    class PresentationView
    {
    public:
        static inline bool sync() { return false; }
        static inline int screen() { return -1; }

        static inline QColor backgroundColor() { return QColor(); }

    private:
        PresentationView() {}

    };

    class DocumentView
    {
    public:
        static inline bool openUrl() { return false; }

        static inline bool autoRefresh() { return false; }

        static inline bool prefetch() { return false; }
        static inline int prefetchDistance() { return 1; }

        static inline int autoRefreshTimeout() { return 750; }
        static inline int prefetchTimeout() { return 250; }

        static inline int pagesPerRow() { return 3; }

        static inline bool highlightCurrentThumbnail() { return false; }
        static inline bool limitThumbnailsToResults() { return false; }

        static inline qreal minimumScaleFactor() { return 0.1; }
        static inline qreal maximumScaleFactor() { return 10.0; }

        static inline qreal zoomBy() { return 0.1; }

        static inline qreal pageSpacing() { return 5.0; }
        static inline qreal thumbnailSpacing() { return 3.0; }

        static inline qreal thumbnailSize() { return 150.0; }

        static inline bool matchCase() { return false; }

        static inline int highlightDuration() { return 5000; }
        static inline QString sourceEditor() { return QString(); }

        static inline Qt::KeyboardModifiers zoomModifiers() { return Qt::ControlModifier; }
        static inline Qt::KeyboardModifiers rotateModifiers() { return Qt::ShiftModifier; }
        static inline Qt::KeyboardModifiers scrollModifiers() { return Qt::AltModifier; }

        // per-tab defaults

        static inline bool continuousMode() { return false; }

        static inline LayoutMode layoutMode() { return SinglePageMode; }

        static inline ScaleMode scaleMode() { return ScaleFactorMode; }

        static inline qreal scaleFactor() { return 1.0; }

        static inline Rotation rotation() { return RotateBy0; }

        static inline bool invertColors() { return false; }

        static inline bool highlightAll() { return false; }

    private:
        DocumentView() {}

    };

    class MainWindow
    {
    public:
        static inline bool trackRecentlyUsed() { return false; }

        static inline int recentlyUsedCount() { return 10; }

        static inline bool restoreTabs() { return false; }
        static inline bool restoreBookmarks() { return false; }
        static inline bool restorePerFileSettings() { return false; }

        static inline int tabPosition() { return 0; }
        static inline int tabVisibility() { return 0; }

        static inline bool newTabNextToCurrentTab() { return true; }

        static inline bool currentPageInWindowTitle() { return false; }
        static inline bool instancfeNameInWindowTitle() { return false; }

        static inline bool synchronizeOutlineView() { return false; }

        static inline QStringList fileToolBar() { return QStringList() << "openInNewTab" << "refresh"; }
        static inline QStringList editToolBar() { return QStringList() << "currentPage" << "previousPage" << "nextPage"; }
        static inline QStringList viewToolBar() { return QStringList() << "scaleFactor" << "zoomIn" << "zoomOut"; }

        static QString path();

    private:
        MainWindow() {}

    };

    class PrintDialog
    {
    public:
        static inline bool collateCopies() { return false; }

        static inline QPrinter::PageOrder pageOrder() { return QPrinter::FirstPageFirst; }

        static inline QPrinter::Orientation orientation() { return QPrinter::Portrait; }

        static inline QPrinter::ColorMode colorMode() { return QPrinter::Color; }

        static inline QPrinter::DuplexMode duplex() { return QPrinter::DuplexNone; }

        static inline bool fitToPage() { return false; }

        static inline PrintOptions::PageSet pageSet() { return PrintOptions::AllPages; }

        static inline PrintOptions::NumberUp numberUp() { return PrintOptions::SinglePage; }
        static inline PrintOptions::NumberUpLayout numberUpLayout() { return PrintOptions::LeftRightTopBottom; }

    private:
        PrintDialog() {}

    };

private:
    Defaults() {}

};

#endif // SETTINGS_H
