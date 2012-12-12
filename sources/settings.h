/*

Copyright 2012 Adam Reichold
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

#include <QtCore>
#include <QtGui>

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

#include <QtWidgets>

#endif // QT_VERSION

#include "pageitem.h"
#include "documentview.h"
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

        static inline int defaultCacheSize() { return 32 * 1024 * 1024; }
        int cacheSize() const;
        void setCacheSize(int cacheSize);

        static inline bool defaultDecoratePages() { return true; }
        bool decoratePages() const;
        void setDecoratePages(bool on);

        static inline bool defaultDecorateLinks() { return true; }
        bool decorateLinks() const;
        void setDecorateLinks(bool on);

        static inline bool defaultDecorateFormFields() { return true; }
        bool decorateFormFields() const;
        void setDecorateFormFields(bool on);

        static inline QString defaultBackgroundColor() { return "gray"; }
        QString backgroundColor() const;
        void setBackgroundColor(const QString& color);

        static inline QString defaultPaperColor() { return "white"; }
        QString paperColor() const;
        void setPaperColor(const QString& color);

        static inline bool defaultInvertColors() { return false; }
        bool invertColors() const;
        void setInvertColors(bool on);

        static inline Qt::KeyboardModifiers defaultCopyModifiers() { return Qt::ShiftModifier; }
        Qt::KeyboardModifiers copyModifiers() const;
        void setCopyModifiers(const Qt::KeyboardModifiers& copyModifiers);

        static inline Qt::KeyboardModifiers defaultAnnotateModifiers() { return Qt::ControlModifier; }
        Qt::KeyboardModifiers annotateModifiers() const;
        void setAnnotateModifiers(const Qt::KeyboardModifiers& annotateModifiers);

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

        static inline bool defaultSync() { return false; }
        bool sync() const;
        void setSync(bool on);

        static inline int defaultScreen() { return -1; }
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

        static inline bool defaultOpenUrl() { return false; }
        bool openUrl() const;
        void setOpenUrl(bool on);

        static inline bool defaultAutoRefresh() { return false; }
        bool autoRefresh() const;
        void setAutoRefresh(bool on);

        static inline bool defaultAntialiasing() { return true; }
        bool antialiasing() const;
        void setAntialiasing(bool on);

        static inline bool defaultTextAntialiasing() { return true; }
        bool textAntialiasing() const;
        void setTextAntialiasing(bool on);

        static inline bool defaultTextHinting() { return false; }
        bool textHinting() const;
        void setTextHinting(bool on);

        static inline bool defaultOverprintPreview() { return false; }
        bool overprintPreview() const;
        void setOverprintPreview(bool on);

        static inline bool defaultPrefetch() { return false; }
        bool prefetch() const;
        void setPrefetch(bool on);

        static inline int defaultPagesPerRow() { return 3; }
        int pagesPerRow() const;
        void setPagesPerRow(int pagesPerRow);

        static inline qreal defaultPageSpacing() { return 5.0; }
        qreal pageSpacing() const;
        void setPageSpacing(qreal pageSpacing);

        static inline qreal defaultThumbnailSpacing() { return 3.0; }
        qreal thumbnailSpacing() const;
        void setThumbnailSpacing(qreal thumbnailSpacing);

        static inline qreal defaultThumbnailSize() { return 150.0; }
        qreal thumbnailSize() const;
        void setThumbnailSize(qreal thumbnailSize);

        static inline Qt::KeyboardModifiers defaultZoomModifiers() { return Qt::ControlModifier; }
        Qt::KeyboardModifiers zoomModifiers() const;
        void setZoomModifiers(const Qt::KeyboardModifiers& zoomModifiers);

        static inline Qt::KeyboardModifiers defaultRotateModifiers() { return Qt::ShiftModifier; }
        Qt::KeyboardModifiers rotateModifiers() const;
        void setRotateModifiers(const Qt::KeyboardModifiers& rotateModifiers);

        static inline Qt::KeyboardModifiers defaultHorizontalModifiers() { return Qt::AltModifier; }
        Qt::KeyboardModifiers horizontalModifiers() const;
        void setHorizontalModifiers(const Qt::KeyboardModifiers& horizontalModifiers);

        static inline int defaultHighlightDuration() { return 5000; }
        int highlightDuration() const;
        void setHighlightDuration(int highlightDuration);

        QString sourceEditor() const;
        void setSourceEditor(const QString& sourceEditor);

        // per-tab settings

        static inline bool defaultContinuousMode() { return false; }
        bool continuousMode() const;
        void setContinuousMode(bool on);

        static inline ::DocumentView::LayoutMode defaultLayoutMode() { return ::DocumentView::SinglePageMode; }
        ::DocumentView::LayoutMode layoutMode() const;
        void setLayoutMode(::DocumentView::LayoutMode layoutMode);

        static inline ::DocumentView::ScaleMode defaultScaleMode() { return ::DocumentView::ScaleFactor; }
        ::DocumentView::ScaleMode scaleMode() const;
        void setScaleMode(::DocumentView::ScaleMode scaleMode);

        static inline qreal defaultScaleFactor() { return 1.0; }
        qreal scaleFactor() const;
        void setScaleFactor(qreal scaleFactor);

        static inline Poppler::Page::Rotation defaultRotation() { return Poppler::Page::Rotate0; }
        Poppler::Page::Rotation rotation() const;
        void setRotation(Poppler::Page::Rotation rotation);

        static inline bool defaultHighlightAll() { return false; }
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

        static inline bool defaultTrackRecentlyUsed() { return false; }
        bool trackRecentlyUsed() const;
        void setTrackRecentlyUsed(bool on);

        QStringList recentlyUsed() const;
        void setRecentlyUsed(const QStringList& recentlyUsed);

        static inline bool defaultRestoreTabs() { return false; }
        bool restoreTabs() const;
        void setRestoreTabs(bool on);

        static inline bool defaultRestoreBookmarks() { return false; }
        bool restoreBookmarks() const;
        void setRestoreBookmarks(bool on);

        static inline bool defaultRestorePerFileSettings() { return false; }
        bool restorePerFileSettings() const;
        void setRestorePerFileSettings(bool on);

        static inline QTabWidget::TabPosition defaultTabPosition() { return QTabWidget::North; }
        QTabWidget::TabPosition tabPosition() const;
        void setTabPosition(QTabWidget::TabPosition tabPosition);

        static inline TabWidget::TabBarPolicy defaultTabVisibility() { return TabWidget::TabBarAsNeeded; }
        TabWidget::TabBarPolicy tabVisibility() const;
        void setTabVisibility(TabWidget::TabBarPolicy tabVisibility);

        static inline QStringList defaultFileToolBar() { return QStringList() << "openInNewTab" << "refresh"; }
        QStringList fileToolBar() const;
        void setFileToolBar(const QStringList& fileToolBar);

        static inline QStringList defaultEditToolBar() { return QStringList() << "currentPage" << "previousPage" << "nextPage"; }
        QStringList editToolBar() const;
        void setEditToolBar(const QStringList& editToolBar);

        static inline QStringList defaultViewToolBar() { return QStringList() << "scaleFactor" << "zoomIn" << "zoomOut"; }
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

        static QString defaultPath();

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

#endif // SETTINGS_H
