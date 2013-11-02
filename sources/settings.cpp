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

#include "settings.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QSettings>

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

#include <QStandardPaths>

#else

#include <QDesktopServices>

#endif // QT_VERSION

Settings* Settings::s_instance = 0;

Settings* Settings::instance()
{
    if(s_instance == 0)
    {
        s_instance = new Settings(qApp);
    }

    return s_instance;
}

Settings::~Settings()
{
    s_instance = 0;
}

// page item

void Settings::PageItem::sync()
{
    m_cacheSize = m_settings->value("pageItem/cacheSize", Defaults::PageItem::cacheSize()).toInt();

    m_progressIcon = QIcon::fromTheme("image-loading", QIcon(":/icons/image-loading.svg"));
    m_errorIcon = QIcon::fromTheme("image-missing", QIcon(":icons/image-missing.svg"));

    m_keepObsoletePixmaps = m_settings->value("pageItem/keepObsoletePixmaps", Defaults::PageItem::keepObsoletePixmaps()).toBool();
    m_useDevicePixelRatio = m_settings->value("pageItem/useDevicePixelRatio", Defaults::PageItem::useDevicePixelRatio()).toBool();

    m_decoratePages = m_settings->value("pageItem/decoratePages", Defaults::PageItem::decoratePages()).toBool();
    m_decorateLinks = m_settings->value("pageItem/decorateLinks", Defaults::PageItem::decorateLinks()).toBool();
    m_decorateFormFields = m_settings->value("pageItem/decorateFormFields", Defaults::PageItem::decorateFormFields()).toBool();

    m_backgroundColor = m_settings->value("pageItem/backgroundColor", Defaults::PageItem::backgroundColor()).value< QColor >();
    m_paperColor = m_settings->value("pageItem/paperColor", Defaults::PageItem::paperColor()).value< QColor >();

    m_highlightColor = m_settings->value("pageItem/highlightColor", Defaults::PageItem::highlightColor()).value< QColor >();
}

void Settings::PageItem::setCacheSize(int cacheSize)
{
    if(cacheSize >= 0)
    {
        m_cacheSize = cacheSize;
        m_settings->setValue("pageItem/cacheSize", cacheSize);
    }
}

void Settings::PageItem::setKeepObsoletePixmaps(bool keepObsoletePixmaps)
{
    m_keepObsoletePixmaps = keepObsoletePixmaps;
    m_settings->setValue("pageItem/keepObsoletePixmaps", keepObsoletePixmaps);
}

void Settings::PageItem::setUseDevicePixelRatio(bool useDevicePixelRatio)
{
    m_useDevicePixelRatio = useDevicePixelRatio;
    m_settings->setValue("pageItem/useDevicePixelRatio", useDevicePixelRatio);
}

void Settings::PageItem::setDecoratePages(bool decorate)
{
    m_decoratePages = decorate;
    m_settings->setValue("pageItem/decoratePages", decorate);
}

void Settings::PageItem::setDecorateLinks(bool decorate)
{
    m_decorateLinks = decorate;
    m_settings->setValue("pageItem/decorateLinks", decorate);
}

void Settings::PageItem::setDecorateFormFields(bool decorate)
{
    m_decorateFormFields = decorate;
    m_settings->setValue("pageItem/decorateFormFields", decorate);
}

void Settings::PageItem::setBackgroundColor(const QColor &color)
{
    m_backgroundColor = color;
    m_settings->setValue("pageItem/backgroundColor", color);
}

void Settings::PageItem::setPaperColor(const QColor& color)
{
    m_paperColor = color;
    m_settings->setValue("pageItem/paperColor", color);
}

void Settings::PageItem::setHighlightColor(const QColor& color)
{
    m_highlightColor = color;
    m_settings->setValue("pageItem/highlightColor", color);
}

QColor Settings::PageItem::annotationColor() const
{
    return m_settings->value("pageItem/annotationColor", Defaults::PageItem::annotationColor()).value< QColor >();
}

void Settings::PageItem::setAnnotationColor(const QColor &color)
{
    m_settings->setValue("pageItem/annotationColor", color);
}

Qt::KeyboardModifiers Settings::PageItem::copyToClipboardModifiers() const
{
    return static_cast< Qt::KeyboardModifiers >(m_settings->value("pageItem/copyToClipboardModifiers", static_cast< int >(Defaults::PageItem::copyToClipboardModifiers())).toInt());
}

void Settings::PageItem::setCopyToClipboardModifiers(const Qt::KeyboardModifiers& modifiers)
{
    m_settings->setValue("pageItem/copyToClipboardModifiers", static_cast< int >(modifiers));
}

Qt::KeyboardModifiers Settings::PageItem::addAnnotationModifiers() const
{
    return static_cast< Qt::KeyboardModifiers >(m_settings->value("pageItem/addAnnotationModifiers", static_cast< int >(Defaults::PageItem::addAnnotationModifiers())).toInt());
}

void Settings::PageItem::setAddAnnotationModifiers(const Qt::KeyboardModifiers& modifiers)
{
    m_settings->setValue("pageItem/addAnnotationModifiers", static_cast< int >(modifiers));
}

bool Settings::PageItem::annotationOverlay() const
{
    return m_settings->value("pageItem/annotationOverlay", Defaults::PageItem::annotationOverlay()).toBool();
}

void Settings::PageItem::setAnnotationOverlay(bool overlay)
{
    m_settings->setValue("pageItem/annotationOverlay", overlay);
}

bool Settings::PageItem::formFieldOverlay() const
{
    return m_settings->value("pageItem/formFieldOverlay", Defaults::PageItem::formFieldOverlay()).toBool();
}

void Settings::PageItem::setFormFieldOverlay(bool overlay)
{
    m_settings->setValue("pageItem/formFieldOverlay", overlay);
}

Settings::PageItem::PageItem(QSettings* settings) :
    m_settings(settings),
    m_cacheSize(Defaults::PageItem::cacheSize()),
    m_progressIcon(),
    m_errorIcon(),
    m_keepObsoletePixmaps(Defaults::PageItem::keepObsoletePixmaps()),
    m_decoratePages(Defaults::PageItem::decoratePages()),
    m_decorateLinks(Defaults::PageItem::decorateLinks()),
    m_decorateFormFields(Defaults::PageItem::decorateFormFields()),
    m_backgroundColor(Defaults::PageItem::backgroundColor()),
    m_paperColor(Defaults::PageItem::paperColor()),
    m_highlightColor(Defaults::PageItem::highlightColor())
{
}

// presentation view

bool Settings::PresentationView::sync() const
{
    return m_settings->value("presentationView/sync", Defaults::PresentationView::sync()).toBool();
}

void Settings::PresentationView::setSync(bool sync)
{
    m_settings->setValue("presentationView/sync", sync);
}

int Settings::PresentationView::screen() const
{
    int screen = m_settings->value("presentationView/screen", Defaults::PresentationView::screen()).toInt();

    if(screen < -1 || screen >= QApplication::desktop()->screenCount())
    {
        screen = -1;
    }

    return screen;
}

void Settings::PresentationView::setScreen(int screen)
{
    m_settings->setValue("presentationView/screen", screen);
}

QColor Settings::PresentationView::backgroundColor() const
{
    return m_settings->value("presentationView/backgroundColor", Defaults::PresentationView::backgroundColor()).value< QColor >();
}

void Settings::PresentationView::setBackgroundColor(const QColor& backgroundColor)
{
    m_settings->setValue("presentationView/backgroundColor", backgroundColor);
}

Settings::PresentationView::PresentationView(QSettings* settings) :
    m_settings(settings)
{
}

// document view

void Settings::DocumentView::sync()
{
    m_prefetch = m_settings->value("documentView/prefetch", Defaults::DocumentView::prefetch()).toBool();
    m_prefetchDistance = m_settings->value("documentView/prefetchDistance", Defaults::DocumentView::prefetchDistance()).toInt();

    m_pagesPerRow = m_settings->value("documentView/pagesPerRow", Defaults::DocumentView::pagesPerRow()).toInt();

    m_highlightCurrentThumbnail = m_settings->value("documentView/highlightCurrentThumbnail", Defaults::DocumentView::highlightCurrentThumbnail()).toBool();
    m_limitThumbnailsToResults = m_settings->value("documentView/limitThumbnailsToResults", Defaults::DocumentView::limitThumbnailsToResults()).toBool();

    m_pageSpacing = m_settings->value("documentView/pageSpacing", Defaults::DocumentView::pageSpacing()).toReal();
    m_thumbnailSpacing = m_settings->value("documentView/thumbnailSpacing", Defaults::DocumentView::thumbnailSpacing()).toReal();

    m_thumbnailSize = m_settings->value("documentView/thumbnailSize", Defaults::DocumentView::thumbnailSize()).toReal();
}

bool Settings::DocumentView::openUrl() const
{
    return m_settings->value("documentView/openUrl", Defaults::DocumentView::openUrl()).toBool();
}

void Settings::DocumentView::setOpenUrl(bool openUrl)
{
    m_settings->setValue("documentView/openUrl", openUrl);
}

bool Settings::DocumentView::autoRefresh() const
{
    return m_settings->value("documentView/autoRefresh", Defaults::DocumentView::autoRefresh()).toBool();
}

void Settings::DocumentView::setAutoRefresh(bool autoRefresh)
{
    m_settings->setValue("documentView/autoRefresh", autoRefresh);
}

void Settings::DocumentView::setPrefetch(bool prefetch)
{
    m_prefetch = prefetch;
    m_settings->setValue("documentView/prefetch", prefetch);
}

void Settings::DocumentView::setPrefetchDistance(int prefetchDistance)
{
    if(prefetchDistance > 0)
    {
        m_prefetchDistance = prefetchDistance;
        m_settings->setValue("documentView/prefetchDistance", prefetchDistance);
    }
}

int Settings::DocumentView::autoRefreshTimeout() const
{
    return m_settings->value("documentView/autoRefreshTimeout", Defaults::DocumentView::autoRefreshTimeout()).toInt();
}

int Settings::DocumentView::prefetchTimeout() const
{
    return m_settings->value("documentView/prefetchTimeout", Defaults::DocumentView::prefetchTimeout()).toInt();
}

void Settings::DocumentView::setPagesPerRow(int pagesPerRow)
{
    if(pagesPerRow > 0)
    {
        m_pagesPerRow = pagesPerRow;
        m_settings->setValue("documentView/pagesPerRow", pagesPerRow);
    }
}

void Settings::DocumentView::setHighlightCurrentThumbnail(bool highlightCurrentThumbnail)
{
    m_highlightCurrentThumbnail = highlightCurrentThumbnail;
    m_settings->setValue("documentView/highlightCurrentThumbnail", highlightCurrentThumbnail);
}

void Settings::DocumentView::setLimitThumbnailsToResults(bool limitThumbnailsToResults)
{
    m_limitThumbnailsToResults = limitThumbnailsToResults;
    m_settings->setValue("documentView/limitThumbnailsToResults", limitThumbnailsToResults);
}

void Settings::DocumentView::setPageSpacing(qreal pageSpacing)
{
    if(pageSpacing >= 0.0)
    {
        m_pageSpacing = pageSpacing;
        m_settings->setValue("documentView/pageSpacing", pageSpacing);
    }
}

void Settings::DocumentView::setThumbnailSpacing(qreal thumbnailSpacing)
{
    if(thumbnailSpacing >= 0.0)
    {
        m_thumbnailSpacing = thumbnailSpacing;
        m_settings->setValue("documentView/thumbnailSpacing", thumbnailSpacing);
    }
}

void Settings::DocumentView::setThumbnailSize(qreal thumbnailSize)
{
    if(thumbnailSize > 0.0)
    {
        m_thumbnailSize = thumbnailSize;
        m_settings->setValue("documentView/thumbnailSize", thumbnailSize);
    }
}

bool Settings::DocumentView::matchCase() const
{
    return m_settings->value("documentView/matchCase", Defaults::DocumentView::matchCase()).toBool();
}

void Settings::DocumentView::setMatchCase(bool matchCase)
{
    m_settings->setValue("documentView/matchCase", matchCase);
}

int Settings::DocumentView::highlightDuration() const
{
    return m_settings->value("documentView/highlightDuration", Defaults::DocumentView::highlightDuration()).toInt();
}

void Settings::DocumentView::setHighlightDuration(int highlightDuration)
{
    if(highlightDuration >= 0)
    {
        m_settings->setValue("documentView/highlightDuration", highlightDuration);
    }
}

QString Settings::DocumentView::sourceEditor() const
{
    return m_settings->value("documentView/sourceEditor", Defaults::DocumentView::sourceEditor()).toString();
}

void Settings::DocumentView::setSourceEditor(const QString& sourceEditor)
{
    m_settings->setValue("documentView/sourceEditor", sourceEditor);
}

Qt::KeyboardModifiers Settings::DocumentView::zoomModifiers() const
{
    return static_cast< Qt::KeyboardModifiers >(m_settings->value("documentView/zoomModifiers", static_cast< int >(Defaults::DocumentView::zoomModifiers())).toInt());
}

void Settings::DocumentView::setZoomModifiers(const Qt::KeyboardModifiers& zoomModifiers)
{
    m_settings->setValue("documentView/zoomModifiers", static_cast< int >(zoomModifiers));
}

Qt::KeyboardModifiers Settings::DocumentView::rotateModifiers() const
{
    return static_cast< Qt::KeyboardModifiers >(m_settings->value("documentView/rotateModifiers", static_cast< int >(Defaults::DocumentView::rotateModifiers())).toInt());
}

void Settings::DocumentView::setRotateModifiers(const Qt::KeyboardModifiers& rotateModifiers)
{
    m_settings->setValue("documentView/rotateModifiers", static_cast< int >(rotateModifiers));
}

Qt::KeyboardModifiers Settings::DocumentView::scrollModifiers() const
{
    return static_cast< Qt::KeyboardModifiers >(m_settings->value("documentView/scrollModifiers", static_cast< int >(Defaults::DocumentView::scrollModifiers())).toInt());
}

void Settings::DocumentView::setScrollModifiers(const Qt::KeyboardModifiers& scrollModifiers)
{
    m_settings->setValue("documentView/scrollModifiers", static_cast< int >(scrollModifiers));
}

// per-tab settings

bool Settings::DocumentView::continuousMode() const
{
    return m_settings->value("documentView/continuousMode", Defaults::DocumentView::continuousMode()).toBool();
}

void Settings::DocumentView::setContinuousMode(bool continuousMode)
{
    m_settings->setValue("documentView/continuousMode", continuousMode);
}

LayoutMode Settings::DocumentView::layoutMode() const
{
    return static_cast< LayoutMode >(m_settings->value("documentView/layoutMode", static_cast< uint >(Defaults::DocumentView::layoutMode())).toUInt());
}

void Settings::DocumentView::setLayoutMode(LayoutMode layoutMode)
{
    m_settings->setValue("documentView/layoutMode", static_cast< uint >(layoutMode));
}

ScaleMode Settings::DocumentView::scaleMode() const
{
    return static_cast< ScaleMode >(m_settings->value("documentView/scaleMode", static_cast< uint >(Defaults::DocumentView::scaleMode())).toUInt());
}

void Settings::DocumentView::setScaleMode(ScaleMode scaleMode)
{
    m_settings->setValue("documentView/scaleMode", static_cast< uint >(scaleMode));
}

qreal Settings::DocumentView::scaleFactor() const
{
    return m_settings->value("documentView/scaleFactor", Defaults::DocumentView::scaleFactor()).toReal();
}

void Settings::DocumentView::setScaleFactor(qreal scaleFactor)
{
    m_settings->setValue("documentView/scaleFactor", scaleFactor);
}

Rotation Settings::DocumentView::rotation() const
{
    return static_cast< Rotation >(m_settings->value("documentView/rotation", static_cast< uint >(Defaults::DocumentView::rotation())).toUInt());
}

void Settings::DocumentView::setRotation(Rotation rotation)
{
    m_settings->setValue("documentView/rotation", static_cast< uint >(rotation));
}

bool Settings::DocumentView::invertColors() const
{
    return m_settings->value("documentView/invertColors", Defaults::DocumentView::invertColors()).toBool();
}

void Settings::DocumentView::setInvertColors(bool invertColors)
{
    m_settings->setValue("documentView/invertColors", invertColors);
}

bool Settings::DocumentView::highlightAll() const
{
    return m_settings->value("documentView/highlightAll", Defaults::DocumentView::highlightAll()).toBool();
}

void Settings::DocumentView::setHighlightAll(bool highlightAll)
{
    m_settings->setValue("documentView/highlightAll", highlightAll);
}

Settings::DocumentView::DocumentView(QSettings *settings) :
    m_settings(settings),
    m_prefetch(Defaults::DocumentView::prefetch()),
    m_prefetchDistance(Defaults::DocumentView::prefetchDistance()),
    m_pagesPerRow(Defaults::DocumentView::pagesPerRow()),
    m_highlightCurrentThumbnail(Defaults::DocumentView::highlightCurrentThumbnail()),
    m_limitThumbnailsToResults(Defaults::DocumentView::limitThumbnailsToResults()),
    m_pageSpacing(Defaults::DocumentView::pageSpacing()),
    m_thumbnailSpacing(Defaults::DocumentView::thumbnailSpacing()),
    m_thumbnailSize(Defaults::DocumentView::thumbnailSize())
{
}

// main window

bool Settings::MainWindow::trackRecentlyUsed() const
{
    return m_settings->value("mainWindow/trackRecentlyUsed", Defaults::MainWindow::trackRecentlyUsed()).toBool();
}

void Settings::MainWindow::setTrackRecentlyUsed(bool on)
{
    m_settings->setValue("mainWindow/trackRecentlyUsed", on);
}

int Settings::MainWindow::recentlyUsedCount() const
{
    return m_settings->value("mainWindow/recentlyUsedCount", Defaults::MainWindow::recentlyUsedCount()).toInt();
}

void Settings::MainWindow::setRecentlyUsedCount(int recentlyUsedCount)
{
    m_settings->setValue("mainWindow/recentlyUsedCount", recentlyUsedCount);
}

QStringList Settings::MainWindow::recentlyUsed() const
{
    return m_settings->value("mainWindow/recentlyUsed").toStringList();
}

void Settings::MainWindow::setRecentlyUsed(const QStringList& recentlyUsed)
{
    m_settings->setValue("mainWindow/recentlyUsed", recentlyUsed);
}

bool Settings::MainWindow::restoreTabs() const
{
    return m_settings->value("mainWindow/restoreTabs", Defaults::MainWindow::restoreTabs()).toBool();
}

void Settings::MainWindow::setRestoreTabs(bool on)
{
    m_settings->setValue("mainWindow/restoreTabs", on);
}

bool Settings::MainWindow::restoreBookmarks() const
{
    return m_settings->value("mainWindow/restoreBookmarks", Defaults::MainWindow::restoreBookmarks()).toBool();
}

void Settings::MainWindow::setRestoreBookmarks(bool on)
{
    m_settings->setValue("mainWindow/restoreBookmarks", on);
}

bool Settings::MainWindow::restorePerFileSettings() const
{
    return m_settings->value("mainWindow/restorePerFileSettings", Defaults::MainWindow::restorePerFileSettings()).toBool();
}

void Settings::MainWindow::setRestorePerFileSettings(bool on)
{
    m_settings->setValue("mainWindow/restorePerFileSettings", on);
}

int Settings::MainWindow::tabPosition() const
{
    return m_settings->value("mainWindow/tabPosition", Defaults::MainWindow::tabPosition()).toInt();
}

void Settings::MainWindow::setTabPosition(int tabPosition)
{
    if(tabPosition >= 0 && tabPosition < 4)
    {
        m_settings->setValue("mainWindow/tabPosition", tabPosition);
    }
}

int Settings::MainWindow::tabVisibility() const
{
    return m_settings->value("mainWindow/tabVisibility", Defaults::MainWindow::tabVisibility()).toInt();
}

void Settings::MainWindow::setTabVisibility(int tabVisibility)
{
    if(tabVisibility >= 0 && tabVisibility < 3)
    {
        m_settings->setValue("mainWindow/tabVisibility", tabVisibility);
    }
}

bool Settings::MainWindow::newTabNextToCurrentTab() const
{
    return m_settings->value("mainWindow/newTabNextToCurrentTab", Defaults::MainWindow::newTabNextToCurrentTab()).toBool();
}

void Settings::MainWindow::setNewTabNextToCurrentTab(bool newTabNextToCurrentTab)
{
    m_settings->setValue("mainWindow/newTabNextToCurrentTab", newTabNextToCurrentTab);
}

bool Settings::MainWindow::currentPageInWindowTitle() const
{
    return m_settings->value("mainWindow/currentPageInWindowTitle", Defaults::MainWindow::currentPageInWindowTitle()).toBool();
}

void Settings::MainWindow::setCurrentPageInWindowTitle(bool currentPageInTabText)
{
    m_settings->setValue("mainWindow/currentPageInWindowTitle", currentPageInTabText);
}

bool Settings::MainWindow::instanceNameInWindowTitle() const
{
    return m_settings->value("mainWindow/instanceNameInWindowTitle", Defaults::MainWindow::instancfeNameInWindowTitle()).toBool();
}

void Settings::MainWindow::setInstanceNameInWindowTitle(bool instanceNameInWindowTitle)
{
    m_settings->setValue("mainWindow/instanceNameInWindowTitle", instanceNameInWindowTitle);
}

bool Settings::MainWindow::synchronizeOutlineView() const
{
    return m_settings->value("mainWindow/synchronizeOutlineView", Defaults::MainWindow::synchronizeOutlineView()).toBool();
}

void Settings::MainWindow::setSynchronizeOutlineView(bool synchronizeOutlineView)
{
    m_settings->setValue("mainWindow/synchronizeOutlineView", synchronizeOutlineView);
}

static QStringList trimmed(const QStringList& list)
{
    QStringList trimmedList;

    foreach(const QString& item, list)
    {
        trimmedList.append(item.trimmed());
    }

    return trimmedList;
}

QStringList Settings::MainWindow::fileToolBar() const
{
    return m_settings->value("mainWindow/fileToolBar", Defaults::MainWindow::fileToolBar()).toStringList();
}

void Settings::MainWindow::setFileToolBar(const QStringList& fileToolBar)
{
    m_settings->setValue("mainWindow/fileToolBar", trimmed(fileToolBar));
}

QStringList Settings::MainWindow::editToolBar() const
{
    return m_settings->value("mainWindow/editToolBar", Defaults::MainWindow::editToolBar()).toStringList();
}

void Settings::MainWindow::setEditToolBar(const QStringList& editToolBar)
{
    m_settings->setValue("mainWindow/editToolBar", trimmed(editToolBar));
}

QStringList Settings::MainWindow::viewToolBar() const
{
    return m_settings->value("mainWindow/viewToolBar", Defaults::MainWindow::viewToolBar()).toStringList();
}

void Settings::MainWindow::setViewToolBar(const QStringList& viewToolBar)
{
    m_settings->setValue("mainWindow/viewToolBar", trimmed(viewToolBar));
}

bool Settings::MainWindow::hasIconTheme() const
{
    return m_settings->contains("mainWindow/iconTheme");
}

QString Settings::MainWindow::iconTheme() const
{
    return m_settings->value("mainWindow/iconTheme").toString();
}

bool Settings::MainWindow::hasStyleSheet() const
{
    return m_settings->contains("mainWindow/styleSheet");
}

QString Settings::MainWindow::styleSheet() const
{
    return m_settings->value("mainWindow/styleSheet").toString();
}

QByteArray Settings::MainWindow::geometry() const
{
    return m_settings->value("mainWindow/geometry").toByteArray();
}

void Settings::MainWindow::setGeometry(const QByteArray& geometry)
{
    m_settings->setValue("mainWindow/geometry", geometry);
}

QByteArray Settings::MainWindow::state() const
{
    return m_settings->value("mainWindow/state").toByteArray();
}

void Settings::MainWindow::setState(const QByteArray& state)
{
    m_settings->setValue("mainWindow/state", state);
}

QString Settings::MainWindow::openPath() const
{
    return m_settings->value("mainWindow/openPath", Defaults::MainWindow::path()).toString();
}

void Settings::MainWindow::setOpenPath(const QString& openPath)
{
    m_settings->setValue("mainWindow/openPath", openPath);
}

QString Settings::MainWindow::savePath() const
{
    return m_settings->value("mainWindow/savePath", Defaults::MainWindow::path()).toString();
}

void Settings::MainWindow::setSavePath(const QString& savePath)
{
    m_settings->setValue("mainWindow/savePath", savePath);
}

QSize Settings::MainWindow::settingsDialogSize(const QSize& sizeHint) const
{
    return m_settings->value("mainWindow/settingsDialogSize", sizeHint).toSize();
}

void Settings::MainWindow::setSettingsDialogSize(const QSize& settingsDialogSize)
{
    m_settings->setValue("mainWindow/settingsDialogSize", settingsDialogSize);
}

QSize Settings::MainWindow::fontsDialogSize(const QSize& sizeHint) const
{
    return m_settings->value("mainWindow/fontsDialogSize", sizeHint).toSize();
}

void Settings::MainWindow::setFontsDialogSize(const QSize& fontsDialogSize)
{
    m_settings->setValue("mainWindow/fontsDialogSize", fontsDialogSize);
}

QSize Settings::MainWindow::contentsDialogSize(const QSize& sizeHint) const
{
    return m_settings->value("mainWindow/contentsDialogSize", sizeHint).toSize();
}

void Settings::MainWindow::setContentsDialogSize(const QSize& contentsDialogSize)
{
    m_settings->setValue("mainWindow/contentsDialogSize", contentsDialogSize);
}

Settings::MainWindow::MainWindow(QSettings* settings) :
    m_settings(settings)
{
}

// print dialog

Settings::PrintDialog::PrintDialog(QSettings* settings) :
    m_settings(settings)
{
}

bool Settings::PrintDialog::collateCopies() const
{
    return m_settings->value("printDialog/collateCopies", Defaults::PrintDialog::collateCopies()).toBool();
}

void Settings::PrintDialog::setCollateCopies(bool collateCopies)
{
    m_settings->setValue("printDialog/collateCopies", collateCopies);
}

QPrinter::PageOrder Settings::PrintDialog::pageOrder() const
{
    return static_cast< QPrinter::PageOrder >(m_settings->value("printDialog/pageOrder", static_cast< int >(Defaults::PrintDialog::pageOrder())).toInt());
}

void Settings::PrintDialog::setPageOrder(QPrinter::PageOrder pageOrder)
{
    m_settings->setValue("printDialog/pageOrder", static_cast< int >(pageOrder));
}

QPrinter::Orientation Settings::PrintDialog::orientation() const
{
    return static_cast< QPrinter::Orientation >(m_settings->value("printDialog/orientation", static_cast< int >(Defaults::PrintDialog::orientation())).toInt());
}

void Settings::PrintDialog::setOrientation(QPrinter::Orientation orientation)
{
    m_settings->setValue("printDialog/orientation", static_cast< int >(orientation));
}

QPrinter::ColorMode Settings::PrintDialog::colorMode() const
{
    return static_cast< QPrinter::ColorMode >(m_settings->value("printDialog/colorMode", static_cast< int >(Defaults::PrintDialog::colorMode())).toInt());
}

void Settings::PrintDialog::setColorMode(QPrinter::ColorMode colorMode)
{
    m_settings->setValue("printDialog/colorMode", static_cast< int >(colorMode));
}

QPrinter::DuplexMode Settings::PrintDialog::duplex() const
{
    return static_cast< QPrinter::DuplexMode >(m_settings->value("printDialog/duplex", static_cast< int >(Defaults::PrintDialog::duplex())).toInt());
}

void Settings::PrintDialog::setDuplex(QPrinter::DuplexMode duplex)
{
    m_settings->setValue("printDialog/duplex", static_cast< int >(duplex));
}

bool Settings::PrintDialog::fitToPage() const
{
    return m_settings->value("printDialog/fitToPage", Defaults::PrintDialog::fitToPage()).toBool();
}

void Settings::PrintDialog::setFitToPage(bool fitToPage)
{
    m_settings->setValue("printDialog/fitToPage", fitToPage);
}

PrintOptions::PageSet Settings::PrintDialog::pageSet() const
{
    return static_cast< PrintOptions::PageSet >(m_settings->value("printDialog/pageSet", static_cast< uint >(Defaults::PrintDialog::pageSet())).toUInt());
}

void Settings::PrintDialog::setPageSet(PrintOptions::PageSet pageSet)
{
    m_settings->setValue("printDialog/pageSet", static_cast< uint >(pageSet));
}

PrintOptions::NumberUp Settings::PrintDialog::numberUp() const
{
    return static_cast< PrintOptions::NumberUp >(m_settings->value("printDialog/numberUp", static_cast< uint >(Defaults::PrintDialog::numberUp())).toUInt());
}

void Settings::PrintDialog::setNumberUp(PrintOptions::NumberUp numberUp)
{
    m_settings->setValue("printDialog/numberUp", static_cast< uint >(numberUp));
}

PrintOptions::NumberUpLayout Settings::PrintDialog::numberUpLayout() const
{
    return static_cast< PrintOptions::NumberUpLayout >(m_settings->value("printDialog/numberUpLayout", static_cast< uint >(Defaults::PrintDialog::numberUpLayout())).toUInt());
}

void Settings::PrintDialog::setNumberUpLayout(PrintOptions::NumberUpLayout numberUpLayout)
{
    m_settings->setValue("printDialog/numberUpLayout", static_cast< uint >(numberUpLayout));
}

void Settings::sync()
{
    m_settings->sync();

    m_pageItem.sync();
    m_documentView.sync();
}

Settings::PageItem& Settings::pageItem()
{
    return m_pageItem;
}

Settings::PresentationView& Settings::presentationView()
{
    return m_presentationView;
}

Settings::DocumentView& Settings::documentView()
{
    return m_documentView;
}

Settings::MainWindow& Settings::mainWindow()
{
    return m_mainWindow;
}

Settings::PrintDialog& Settings::printDialog()
{
    return m_printDialog;
}

Settings::Settings(QObject* parent) : QObject(parent),
    m_settings(new QSettings("qpdfview", "qpdfview", this)),
    m_pageItem(m_settings),
    m_presentationView(m_settings),
    m_documentView(m_settings),
    m_mainWindow(m_settings),
    m_printDialog(m_settings)
{
}

// defaults

QString Defaults::MainWindow::path()
{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

    return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

#else

    return QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);

#endif // QT_VERSION
}
