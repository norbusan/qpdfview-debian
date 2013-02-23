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

#ifndef MY_SETTINGS_H
#define MY_SETTINGS_H

class QSettings;

#include "global.h"
#include "miscellaneous.h"

class Settings : public QObject
{
    Q_OBJECT

public:
    explicit Settings(QObject* parent = 0);
    ~Settings();

    QString fileName() const;

    // page item

    class PageItem
    {
    public:
        PageItem(QSettings* settings);

        int cacheSize() const;
        void setCacheSize(int cacheSize);

        bool decoratePages() const;
        void setDecoratePages(bool on);

        bool decorateLinks() const;
        void setDecorateLinks(bool on);

        bool decorateFormFields() const;
        void setDecorateFormFields(bool on);

        QString backgroundColor() const;
        void setBackgroundColor(const QString& color);

        QString paperColor() const;
        void setPaperColor(const QString& color);

        Qt::KeyboardModifiers copyToClipboardModifiers() const;
        void setCopyToClipboardModifiers(const Qt::KeyboardModifiers& copyToClipboardModifiers);

        Qt::KeyboardModifiers addAnnotationModifiers() const;
        void setAddAnnotationModifiers(const Qt::KeyboardModifiers& addAnnotationModifiers);

    private:
        QSettings* m_settings;

    };

    PageItem* pageItem();
    const PageItem* pageItem() const;

    // presentation view

    class PresentationView
    {
    public:
        PresentationView(QSettings* settings);

        bool sync() const;
        void setSync(bool on);

        int screen() const;
        void setScreen(int screen);

    private:
        QSettings* m_settings;

    };

    PresentationView* presentationView();
    const PresentationView* presentationView() const;

    // document view

    class DocumentView
    {
    public:
        DocumentView(QSettings* settings);

        bool openUrl() const;
        void setOpenUrl(bool on);

        bool autoRefresh() const;
        void setAutoRefresh(bool on);

        bool prefetch() const;
        void setPrefetch(bool on);

        int prefetchDistance() const;
        void setPrefetchDistance(int prefetchDistance);

        int pagesPerRow() const;
        void setPagesPerRow(int pagesPerRow);

        qreal pageSpacing() const;
        void setPageSpacing(qreal pageSpacing);

        qreal thumbnailSpacing() const;
        void setThumbnailSpacing(qreal thumbnailSpacing);

        qreal thumbnailSize() const;
        void setThumbnailSize(qreal thumbnailSize);

        Qt::KeyboardModifiers zoomModifiers() const;
        void setZoomModifiers(const Qt::KeyboardModifiers& zoomModifiers);

        Qt::KeyboardModifiers rotateModifiers() const;
        void setRotateModifiers(const Qt::KeyboardModifiers& rotateModifiers);

        Qt::KeyboardModifiers scrollModifiers() const;
        void setScrollModifiers(const Qt::KeyboardModifiers& scrollModifiers);

        int highlightDuration() const;
        void setHighlightDuration(int highlightDuration);

        QString sourceEditor() const;
        void setSourceEditor(const QString& sourceEditor);

        // per-tab settings

        bool continuousMode() const;
        void setContinuousMode(bool on);

        LayoutMode layoutMode() const;
        void setLayoutMode(LayoutMode layoutMode);

        ScaleMode scaleMode() const;
        void setScaleMode(ScaleMode scaleMode);

        qreal scaleFactor() const;
        void setScaleFactor(qreal scaleFactor);

        Rotation rotation() const;
        void setRotation(Rotation rotation);

        bool invertColors() const;
        void setInvertColors(bool on);

        bool highlightAll() const;
        void setHighlightAll(bool on);

    private:
        QSettings* m_settings;

    };

    DocumentView* documentView();
    const DocumentView* documentView() const;

    // main window

    class MainWindow
    {
    public:
        MainWindow(QSettings* settings);

        bool trackRecentlyUsed() const;
        void setTrackRecentlyUsed(bool on);

        QStringList recentlyUsed() const;
        void setRecentlyUsed(const QStringList& recentlyUsed);

        bool restoreTabs() const;
        void setRestoreTabs(bool on);

        bool restoreBookmarks() const;
        void setRestoreBookmarks(bool on);

        bool restorePerFileSettings() const;
        void setRestorePerFileSettings(bool on);

        QTabWidget::TabPosition tabPosition() const;
        void setTabPosition(QTabWidget::TabPosition tabPosition);

        TabWidget::TabBarPolicy tabVisibility() const;
        void setTabVisibility(TabWidget::TabBarPolicy tabVisibility);

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

        QSize fontsDialogSize(const QSize& sizeHint) const;
        void setFontsDialogSize(const QSize& fontsDialogSize);

        QSize contentsDialogSize(const QSize& sizeHint) const;
        void setContentsDialogSize(const QSize& contentsDialogSize);

    private:
        QSettings* m_settings;

        QStringList trimmed(const QStringList& list);

    };

    MainWindow* mainWindow();
    const MainWindow* mainWindow() const;

public slots:
    void refresh();

private:
    QSettings* m_settings;

    PageItem* m_pageItem;
    PresentationView* m_presentationView;
    DocumentView* m_documentView;
    MainWindow* m_mainWindow;

};

class Defaults
{
public:
    class PageItem
    {
    public:
        static inline int cacheSize() { return 32 * 1024 * 1024; }

        static inline bool decoratePages() { return true; }
        static inline bool decorateLinks() { return true; }
        static inline bool decorateFormFields() { return true; }

        static inline QString backgroundColor() { return "gray"; }
        static inline QString paperColor() { return "white"; }

        static inline Qt::KeyboardModifiers copyToClipboardModifiers() { return Qt::ShiftModifier; }
        static inline Qt::KeyboardModifiers addAnnotationModifiers() { return Qt::ControlModifier; }

    private:
        PageItem() {}

    };

    class PresentationView
    {
    public:
        static inline bool sync() { return false; }
        static inline int screen() { return -1; }

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

        static inline int pagesPerRow() { return 3; }

        static inline qreal pageSpacing() { return 5.0; }
        static inline qreal thumbnailSpacing() { return 3.0; }

        static inline qreal thumbnailSize() { return 150.0; }

        static inline Qt::KeyboardModifiers zoomModifiers() { return Qt::ControlModifier; }
        static inline Qt::KeyboardModifiers rotateModifiers() { return Qt::ShiftModifier; }
        static inline Qt::KeyboardModifiers scrollModifiers() { return Qt::AltModifier; }

        static inline int highlightDuration() { return 5000; }

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

        static inline bool restoreTabs() { return false; }
        static inline bool restoreBookmarks() { return false; }
        static inline bool restorePerFileSettings() { return false; }

        static inline QTabWidget::TabPosition tabPosition() { return QTabWidget::North; }
        static inline TabWidget::TabBarPolicy tabVisibility() { return TabWidget::TabBarAsNeeded; }

        static inline QStringList fileToolBar() { return QStringList() << "openInNewTab" << "refresh"; }
        static inline QStringList editToolBar() { return QStringList() << "currentPage" << "previousPage" << "nextPage"; }
        static inline QStringList viewToolBar() { return QStringList() << "scaleFactor" << "zoomIn" << "zoomOut"; }

        static QString path();

    private:
        MainWindow() {}

    };

private:
    Defaults() {}

};

#endif // SETTINGS_H
