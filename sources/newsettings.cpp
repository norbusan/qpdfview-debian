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

#include "newsettings.h"

#include <QApplication>
#include <QSettings>

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

#include <QStandardPaths>

#else

#include <QDesktopServices>

#endif // QT_VERSION

NewSettings* NewSettings::s_instance = 0;

NewSettings* NewSettings::instance()
{
    if(s_instance == 0)
    {
        s_instance = new NewSettings(qApp);
    }

    return s_instance;
}

// page item

void NewSettings::PageItem::sync()
{
    m_cacheSize = m_settings->value("pageItem/cacheSize", Defaults::PageItem::cacheSize()).toInt();

    m_decoratePages = m_settings->value("pageItem/decoratePages", Defaults::PageItem::decoratePages()).toBool();
    m_decorateLinks = m_settings->value("pageItem/decorateLinks", Defaults::PageItem::decorateLinks()).toBool();
    m_decorateFormFields = m_settings->value("pageItem/decorateFormFields", Defaults::PageItem::decorateFormFields()).toBool();

    QColor backgroundColor(m_settings->value("pageItem/backgroundColor", Defaults::PageItem::backgroundColor()).toString());
    m_backgroundColor = backgroundColor.isValid() ? backgroundColor : Defaults::PageItem::backgroundColor();

    QColor paperColor(m_settings->value("pageItem/paperColor", Defaults::PageItem::paperColor()).toString());
    m_paperColor = paperColor.isValid() ? paperColor : Defaults::PageItem::paperColor();

    m_progressIcon = QIcon::fromTheme("image-loading", QIcon(":/icons/image-loading.svg"));
    m_errorIcon = QIcon::fromTheme("image-missing", QIcon(":icons/image-missing.svg"));
}

int NewSettings::PageItem::cacheSize() const
{
    return m_cacheSize;
}

void NewSettings::PageItem::setCacheSize(int cacheSize)
{
    m_cacheSize = cacheSize;
    m_settings->setValue("pageItem/cacheSize", cacheSize);
}

bool NewSettings::PageItem::decoratePages() const
{
    return m_decoratePages;
}

void NewSettings::PageItem::setDecoratePages(bool decorate)
{
    m_decoratePages = decorate;
    m_settings->setValue("pageItem/decoratePages", decorate);
}

bool NewSettings::PageItem::decorateLinks() const
{
    return m_decorateLinks;
}

void NewSettings::PageItem::setDecorateLinks(bool decorate)
{
    m_decorateLinks = decorate;
    m_settings->setValue("pageItem/decorateLinks", decorate);
}

bool NewSettings::PageItem::decorateFormFields() const
{
    return m_decorateFormFields;
}

void NewSettings::PageItem::setDecorateFormFields(bool decorate)
{
    m_decorateFormFields = decorate;
    m_settings->setValue("pageItem/decorateFormFields", decorate);
}

QColor NewSettings::PageItem::backgroundColor() const
{
    return m_backgroundColor.name();
}

void NewSettings::PageItem::setBackgroundColor(const QString& color)
{
    QColor backgroundColor(color);
    m_backgroundColor = backgroundColor.isValid() ? backgroundColor : Defaults::PageItem::backgroundColor();

    m_settings->setValue("pageItem/backgroundColor", color);
}

QColor NewSettings::PageItem::paperColor() const
{
    return m_paperColor.name();
}

void NewSettings::PageItem::setPaperColor(const QString& color)
{
    QColor paperColor(color);
    m_paperColor = paperColor.isValid() ? paperColor : Defaults::PageItem::paperColor();

    m_settings->setValue("pageItem/paperColor", color);
}

const QIcon& NewSettings::PageItem::progressIcon() const
{
    return m_progressIcon;
}

const QIcon& NewSettings::PageItem::errorIcon() const
{
    return m_errorIcon;
}

Qt::KeyboardModifiers NewSettings::PageItem::copyToClipboardModifiers() const
{
    return static_cast< Qt::KeyboardModifiers >(m_settings->value("pageItem/copyToClipboardModifiers", static_cast< int >(Defaults::PageItem::copyToClipboardModifiers())).toInt());
}

void NewSettings::PageItem::setCopyToClipboardModifiers(const Qt::KeyboardModifiers& modifiers)
{
    m_settings->setValue("pageItem/copyToClipboardModifiers", static_cast< int >(modifiers));
}

Qt::KeyboardModifiers NewSettings::PageItem::addAnnotationModifiers() const
{
    return static_cast< Qt::KeyboardModifiers >(m_settings->value("pageItem/addAnnotationModifiers", static_cast< int >(Defaults::PageItem::addAnnotationModifiers())).toInt());
}

void NewSettings::PageItem::setAddAnnotationModifiers(const Qt::KeyboardModifiers& modifiers)
{
    m_settings->setValue("pageItem/addAnnotationModifiers", static_cast< int >(modifiers));
}

NewSettings::PageItem::PageItem(QSettings* settings) :
    m_settings(settings),
    m_cacheSize(Defaults::PageItem::cacheSize()),
    m_decoratePages(Defaults::PageItem::decoratePages()),
    m_decorateLinks(Defaults::PageItem::decorateLinks()),
    m_decorateFormFields(Defaults::PageItem::decorateFormFields()),
    m_backgroundColor(Defaults::PageItem::backgroundColor()),
    m_paperColor(Defaults::PageItem::paperColor()),
    m_progressIcon(),
    m_errorIcon()
{
}

// presentation view

void NewSettings::PresentationView::sync()
{
}

NewSettings::PresentationView::PresentationView(QSettings* settings) :
    m_settings(settings)
{
}

bool NewSettings::PresentationView::sync() const
{
    return m_settings->value("presentationView/sync", Defaults::PresentationView::sync()).toBool();
}

void NewSettings::PresentationView::setSync(bool sync)
{
    m_settings->setValue("presentationView/sync", sync);
}

int NewSettings::PresentationView::screen() const
{
    return m_settings->value("presentationView/screen", Defaults::PresentationView::screen()).toInt();
}

void NewSettings::PresentationView::setScreen(int screen)
{
    m_settings->setValue("presentationView/screen", screen);
}

// document view

void NewSettings::DocumentView::sync()
{
    m_prefetch = m_settings->value("documentView/prefetch", Defaults::DocumentView::prefetch()).toBool();
    m_prefetchDistance = m_settings->value("documentView/prefetchDistance", Defaults::DocumentView::prefetchDistance()).toInt();

    m_pagesPerRow = m_settings->value("documentView/pagesPerRow", Defaults::DocumentView::pagesPerRow()).toInt();

    m_limitThumbnailsToResults = m_settings->value("documentView/limitThumbnailsToResults", Defaults::DocumentView::limitThumbnailsToResults()).toBool();

    m_pageSpacing = m_settings->value("documentView/pageSpacing", Defaults::DocumentView::pageSpacing()).toReal();
    m_thumbnailSpacing = m_settings->value("documentView/thumbnailSpacing", Defaults::DocumentView::thumbnailSpacing()).toReal();

    m_thumbnailSize = m_settings->value("documentView/thumbnailSize", Defaults::DocumentView::thumbnailSize()).toReal();
}

bool NewSettings::DocumentView::openUrl() const
{
    return m_settings->value("documentView/openUrl", Defaults::DocumentView::openUrl()).toBool();
}

void NewSettings::DocumentView::setOpenUrl(bool openUrl)
{
    m_settings->setValue("documentView/openUrl", openUrl);
}

bool NewSettings::DocumentView::autoRefresh() const
{
    return m_settings->value("documentView/autoRefresh", Defaults::DocumentView::autoRefresh()).toBool();
}

void NewSettings::DocumentView::setAutoRefresh(bool autoRefresh)
{
    m_settings->setValue("documentView/autoRefresh", autoRefresh);
}

bool NewSettings::DocumentView::prefetch() const
{
    return m_prefetch;
}

void NewSettings::DocumentView::setPrefetch(bool prefetch)
{
    m_prefetch = prefetch;
    m_settings->setValue("documentView/prefetch", prefetch);
}

int NewSettings::DocumentView::prefetchDistance() const
{
    return m_prefetchDistance;
}

void NewSettings::DocumentView::setPrefetchDistance(int prefetchDistance)
{
    m_prefetchDistance = prefetchDistance;
    m_settings->setValue("documentView/prefetchDistance", prefetchDistance);
}

int NewSettings::DocumentView::pagesPerRow() const
{
    return m_pagesPerRow;
}

void NewSettings::DocumentView::setPagesPerRow(int pagesPerRow)
{
    m_pagesPerRow = pagesPerRow;
    m_settings->setValue("documentView/pagesPerRow", pagesPerRow);
}

bool NewSettings::DocumentView::limitThumbnailsToResults() const
{
    return m_limitThumbnailsToResults;
}

void NewSettings::DocumentView::setLimitThumbnailsToResults(bool limitThumbnailsToResults)
{
    m_limitThumbnailsToResults = limitThumbnailsToResults;
    m_settings->setValue("documentView/limitThumbnailsToResults", limitThumbnailsToResults);
}

qreal NewSettings::DocumentView::pageSpacing() const
{
    return m_pageSpacing;
}

void NewSettings::DocumentView::setPageSpacing(qreal pageSpacing)
{
    m_pageSpacing = pageSpacing;
    m_settings->setValue("documentView/pageSpacing", pageSpacing);
}

qreal NewSettings::DocumentView::thumbnailSpacing() const
{
    return m_thumbnailSpacing;
}

void NewSettings::DocumentView::setThumbnailSpacing(qreal thumbnailSpacing)
{
    m_thumbnailSpacing = thumbnailSpacing;
    m_settings->setValue("documentView/thumbnailSpacing", thumbnailSpacing);
}

qreal NewSettings::DocumentView::thumbnailSize() const
{
    return m_thumbnailSize;
}

void NewSettings::DocumentView::setThumbnailSize(qreal thumbnailSize)
{
    m_thumbnailSize = thumbnailSize;
    m_settings->setValue("documentView/thumbnailSize", thumbnailSize);
}

bool NewSettings::DocumentView::matchCase() const
{
    return m_settings->value("documentView/matchCase", Defaults::DocumentView::matchCase()).toBool();
}

void NewSettings::DocumentView::setMatchCase(bool matchCase)
{
    m_settings->setValue("documentView/matchCase", matchCase);
}

int NewSettings::DocumentView::highlightDuration() const
{
    return m_settings->value("documentView/highlightDuration", Defaults::DocumentView::highlightDuration()).toInt();
}

void NewSettings::DocumentView::setHighlightDuration(int highlightDuration)
{
    m_settings->setValue("documentView/highlightDuration", highlightDuration);
}

QString NewSettings::DocumentView::sourceEditor() const
{
    return m_settings->value("documentView/sourceEditor", Defaults::DocumentView::sourceEditor()).toString();
}

void NewSettings::DocumentView::setSourceEditor(const QString& sourceEditor)
{
    m_settings->setValue("documentView/sourceEditor", sourceEditor);
}

Qt::KeyboardModifiers NewSettings::DocumentView::zoomModifiers() const
{
    return static_cast< Qt::KeyboardModifiers >(m_settings->value("documentView/zoomModifiers", static_cast< int >(Defaults::DocumentView::zoomModifiers())).toInt());
}

void NewSettings::DocumentView::setZoomModifiers(const Qt::KeyboardModifiers& zoomModifiers)
{
    m_settings->setValue("documentView/zoomModifiers", static_cast< int >(zoomModifiers));
}

Qt::KeyboardModifiers NewSettings::DocumentView::rotateModifiers() const
{
    return static_cast< Qt::KeyboardModifiers >(m_settings->value("documentView/rotateModifiers", static_cast< int >(Defaults::DocumentView::rotateModifiers())).toInt());
}

void NewSettings::DocumentView::setRotateModifiers(const Qt::KeyboardModifiers& rotateModifiers)
{
    m_settings->setValue("documentView/rotateModifiers", static_cast< int >(rotateModifiers));
}

Qt::KeyboardModifiers NewSettings::DocumentView::scrollModifiers() const
{
    return static_cast< Qt::KeyboardModifiers >(m_settings->value("documentView/scrollModifiers", static_cast< int >(Defaults::DocumentView::scrollModifiers())).toInt());
}

void NewSettings::DocumentView::setScrollModifiers(const Qt::KeyboardModifiers& scrollModifiers)
{
    m_settings->setValue("documentView/scrollModifiers", static_cast< int >(scrollModifiers));
}

// per-tab settings

bool NewSettings::DocumentView::continuousMode() const
{
    return m_settings->value("documentView/continuousMode", Defaults::DocumentView::continuousMode()).toBool();
}

void NewSettings::DocumentView::setContinuousMode(bool continuousMode)
{
    m_settings->setValue("documentView/continuousMode", continuousMode);
}

LayoutMode NewSettings::DocumentView::layoutMode() const
{
    return static_cast< LayoutMode >(m_settings->value("documentView/layoutMode", static_cast< uint >(Defaults::DocumentView::layoutMode())).toUInt());
}

void NewSettings::DocumentView::setLayoutMode(LayoutMode layoutMode)
{
    m_settings->setValue("documentView/layoutMode", static_cast< uint >(layoutMode));
}

ScaleMode NewSettings::DocumentView::scaleMode() const
{
    return static_cast< ScaleMode >(m_settings->value("documentView/scaleMode", static_cast< uint >(Defaults::DocumentView::scaleMode())).toUInt());
}

void NewSettings::DocumentView::setScaleMode(ScaleMode scaleMode)
{
    m_settings->setValue("documentView/scaleMode", static_cast< uint >(scaleMode));
}

qreal NewSettings::DocumentView::scaleFactor() const
{
    return m_settings->value("documentView/scaleFactor", Defaults::DocumentView::scaleFactor()).toReal();
}

void NewSettings::DocumentView::setScaleFactor(qreal scaleFactor)
{
    m_settings->setValue("documentView/scaleFactor", scaleFactor);
}

Rotation NewSettings::DocumentView::rotation() const
{
    return static_cast< Rotation >(m_settings->value("documentView/rotation", static_cast< uint >(Defaults::DocumentView::rotation())).toUInt());
}

void NewSettings::DocumentView::setRotation(Rotation rotation)
{
    m_settings->setValue("documentView/rotation", static_cast< uint >(rotation));
}

bool NewSettings::DocumentView::invertColors() const
{
    return m_settings->value("documentView/invertColors", Defaults::DocumentView::invertColors()).toBool();
}

void NewSettings::DocumentView::setInvertColors(bool invertColors)
{
    m_settings->setValue("documentView/invertColors", invertColors);
}

bool NewSettings::DocumentView::highlightAll() const
{
    return m_settings->value("documentView/highlightAll", Defaults::DocumentView::highlightAll()).toBool();
}

void NewSettings::DocumentView::setHighlightAll(bool highlightAll)
{
    m_settings->setValue("documentView/highlightAll", highlightAll);
}

NewSettings::DocumentView::DocumentView(QSettings *settings) :
    m_settings(settings),
    m_prefetch(Defaults::DocumentView::prefetch()),
    m_prefetchDistance(Defaults::DocumentView::prefetchDistance()),
    m_pagesPerRow(Defaults::DocumentView::pagesPerRow()),
    m_limitThumbnailsToResults(Defaults::DocumentView::limitThumbnailsToResults()),
    m_pageSpacing(Defaults::DocumentView::pageSpacing()),
    m_thumbnailSpacing(Defaults::DocumentView::thumbnailSpacing()),
    m_thumbnailSize(Defaults::DocumentView::thumbnailSize())
{
}

// main window

void NewSettings::MainWindow::sync()
{
}

bool NewSettings::MainWindow::trackRecentlyUsed() const
{
    return m_settings->value("mainWindow/trackRecentlyUsed", Defaults::MainWindow::trackRecentlyUsed()).toBool();
}

void NewSettings::MainWindow::setTrackRecentlyUsed(bool on)
{
    m_settings->setValue("mainWindow/trackRecentlyUsed", on);
}

QStringList NewSettings::MainWindow::recentlyUsed() const
{
    return m_settings->value("mainWindow/recentlyUsed").toStringList();
}

void NewSettings::MainWindow::setRecentlyUsed(const QStringList& recentlyUsed)
{
    m_settings->setValue("mainWindow/recentlyUsed", recentlyUsed);
}

bool NewSettings::MainWindow::restoreTabs() const
{
    return m_settings->value("mainWindow/restoreTabs", Defaults::MainWindow::restoreTabs()).toBool();
}

void NewSettings::MainWindow::setRestoreTabs(bool on)
{
    m_settings->setValue("mainWindow/restoreTabs", on);
}

bool NewSettings::MainWindow::restoreBookmarks() const
{
    return m_settings->value("mainWindow/restoreBookmarks", Defaults::MainWindow::restoreBookmarks()).toBool();
}

void NewSettings::MainWindow::setRestoreBookmarks(bool on)
{
    m_settings->setValue("mainWindow/restoreBookmarks", on);
}

bool NewSettings::MainWindow::restorePerFileSettings() const
{
    return m_settings->value("mainWindow/restorePerFileSettings", Defaults::MainWindow::restorePerFileSettings()).toBool();
}

void NewSettings::MainWindow::setRestorePerFileSettings(bool on)
{
    m_settings->setValue("mainWindow/restorePerFileSettings", on);
}

int NewSettings::MainWindow::tabPosition() const
{
    return m_settings->value("mainWindow/tabPosition", Defaults::MainWindow::tabPosition()).toInt();
}

void NewSettings::MainWindow::setTabPosition(int tabPosition)
{
    m_settings->setValue("mainWindow/tabPosition", tabPosition);
}

int NewSettings::MainWindow::tabVisibility() const
{
    return m_settings->value("mainWindow/tabVisibility", Defaults::MainWindow::tabVisibility()).toInt();
}

void NewSettings::MainWindow::setTabVisibility(int tabVisibility)
{
    m_settings->setValue("mainWindow/tabVisibility", tabVisibility);
}

bool NewSettings::MainWindow::newTabNextToCurrentTab() const
{
    return m_settings->value("mainWindow/newTabNextToCurrentTab", Defaults::MainWindow::newTabNextToCurrentTab()).toBool();
}

void NewSettings::MainWindow::setNewTabNextToCurrentTab(bool newTabNextToCurrentTab)
{
    m_settings->setValue("mainWindow/newTabNextToCurrentTab", newTabNextToCurrentTab);
}

bool NewSettings::MainWindow::currentPageInWindowTitle() const
{
    return m_settings->value("mainWindow/currentPageInWindowTitle", Defaults::MainWindow::currentPageInWindowTitle()).toBool();
}

void NewSettings::MainWindow::setCurrentPageInWindowTitle(bool currentPageInTabText)
{
    m_settings->setValue("mainWindow/currentPageInWindowTitle", currentPageInTabText);
}

static QStringList trimmed(const QStringList& list)
{
    QStringList trimmedList;

    foreach(QString item, list)
    {
        trimmedList.append(item.trimmed());
    }

    return trimmedList;
}

QStringList NewSettings::MainWindow::fileToolBar() const
{
    return m_settings->value("mainWindow/fileToolBar", Defaults::MainWindow::fileToolBar()).toStringList();
}

void NewSettings::MainWindow::setFileToolBar(const QStringList& fileToolBar)
{
    m_settings->setValue("mainWindow/fileToolBar", trimmed(fileToolBar));
}

QStringList NewSettings::MainWindow::editToolBar() const
{
    return m_settings->value("mainWindow/editToolBar", Defaults::MainWindow::editToolBar()).toStringList();
}

void NewSettings::MainWindow::setEditToolBar(const QStringList& editToolBar)
{
    m_settings->setValue("mainWindow/editToolBar", trimmed(editToolBar));
}

QStringList NewSettings::MainWindow::viewToolBar() const
{
    return m_settings->value("mainWindow/viewToolBar", Defaults::MainWindow::viewToolBar()).toStringList();
}

void NewSettings::MainWindow::setViewToolBar(const QStringList& viewToolBar)
{
    m_settings->setValue("mainWindow/viewToolBar", trimmed(viewToolBar));
}

bool NewSettings::MainWindow::hasIconTheme() const
{
    return m_settings->contains("mainWindow/iconTheme");
}

QString NewSettings::MainWindow::iconTheme() const
{
    return m_settings->value("mainWindow/iconTheme").toString();
}

bool NewSettings::MainWindow::hasStyleSheet() const
{
    return m_settings->contains("mainWindow/styleSheet");
}

QString NewSettings::MainWindow::styleSheet() const
{
    return m_settings->value("mainWindow/styleSheet").toString();
}

QByteArray NewSettings::MainWindow::geometry() const
{
    return m_settings->value("mainWindow/geometry").toByteArray();
}

void NewSettings::MainWindow::setGeometry(const QByteArray& geometry)
{
    m_settings->setValue("mainWindow/geometry", geometry);
}

QByteArray NewSettings::MainWindow::state() const
{
    return m_settings->value("mainWindow/state").toByteArray();
}

void NewSettings::MainWindow::setState(const QByteArray& state)
{
    m_settings->setValue("mainWindow/state", state);
}

QString NewSettings::MainWindow::openPath() const
{
    return m_settings->value("mainWindow/openPath", Defaults::MainWindow::path()).toString();
}

void NewSettings::MainWindow::setOpenPath(const QString& openPath)
{
    m_settings->setValue("mainWindow/openPath", openPath);
}

QString NewSettings::MainWindow::savePath() const
{
    return m_settings->value("mainWindow/savePath", Defaults::MainWindow::path()).toString();
}

void NewSettings::MainWindow::setSavePath(const QString& savePath)
{
    m_settings->setValue("mainWindow/savePath", savePath);
}

QSize NewSettings::MainWindow::fontsDialogSize(const QSize& sizeHint) const
{
    return m_settings->value("mainWindow/fontsDialogSize", sizeHint).toSize();
}

void NewSettings::MainWindow::setFontsDialogSize(const QSize& fontsDialogSize)
{
    m_settings->setValue("mainWindow/fontsDialogSize", fontsDialogSize);
}

QSize NewSettings::MainWindow::contentsDialogSize(const QSize& sizeHint) const
{
    return m_settings->value("mainWindow/contentsDialogSize", sizeHint).toSize();
}

void NewSettings::MainWindow::setContentsDialogSize(const QSize& contentsDialogSize)
{
    m_settings->setValue("mainWindow/contentsDialogSize", contentsDialogSize);
}

NewSettings::MainWindow::MainWindow(QSettings* settings) :
    m_settings(settings)
{
}

// print dialog

void NewSettings::PrintDialog::sync()
{
}

NewSettings::PrintDialog::PrintDialog(QSettings* settings) :
    m_settings(settings)
{
}

bool NewSettings::PrintDialog::collateCopies()
{
    return m_settings->value("printDialog/collateCopies", Defaults::PrintDialog::collateCopies()).toBool();
}

void NewSettings::PrintDialog::setCollateCopies(bool collateCopies)
{
    m_settings->setValue("printDialog/collateCopies", collateCopies);
}

QPrinter::PageOrder NewSettings::PrintDialog::pageOrder()
{
    return static_cast< QPrinter::PageOrder >(m_settings->value("printDialog/pageOrder", static_cast< int >(Defaults::PrintDialog::pageOrder())).toInt());
}

void NewSettings::PrintDialog::setPageOrder(QPrinter::PageOrder pageOrder)
{
    m_settings->setValue("printDialog/pageOrder", static_cast< int >(pageOrder));
}

QPrinter::Orientation NewSettings::PrintDialog::orientation()
{
    return static_cast< QPrinter::Orientation >(m_settings->value("printDialog/orientation", static_cast< int >(Defaults::PrintDialog::orientation())).toInt());
}

void NewSettings::PrintDialog::setOrientation(QPrinter::Orientation orientation)
{
    m_settings->setValue("printDialog/orientation", static_cast< int >(orientation));
}

QPrinter::ColorMode NewSettings::PrintDialog::colorMode()
{
    return static_cast< QPrinter::ColorMode >(m_settings->value("printDialog/colorMode", static_cast< int >(Defaults::PrintDialog::colorMode())).toInt());
}

void NewSettings::PrintDialog::setColorMode(QPrinter::ColorMode colorMode)
{
    m_settings->setValue("printDialog/colorMode", static_cast< int >(colorMode));
}

QPrinter::DuplexMode NewSettings::PrintDialog::duplex()
{
    return static_cast< QPrinter::DuplexMode >(m_settings->value("printDialog/duplex", static_cast< int >(Defaults::PrintDialog::duplex())).toInt());
}

void NewSettings::PrintDialog::setDuplex(QPrinter::DuplexMode duplex)
{
    m_settings->setValue("printDialog/duplex", static_cast< int >(duplex));
}

bool NewSettings::PrintDialog::fitToPage()
{
    return m_settings->value("printDialog/fitToPage", Defaults::PrintDialog::fitToPage()).toBool();
}

void NewSettings::PrintDialog::setFitToPage(bool fitToPage)
{
    m_settings->setValue("printDialog/fitToPage", fitToPage);
}

PrintOptions::PageSet NewSettings::PrintDialog::pageSet()
{
    return static_cast< PrintOptions::PageSet >(m_settings->value("printDialog/pageSet", static_cast< uint >(Defaults::PrintDialog::pageSet())).toUInt());
}

void NewSettings::PrintDialog::setPageSet(PrintOptions::PageSet pageSet)
{
    m_settings->setValue("printDialog/pageSet", static_cast< uint >(pageSet));
}

PrintOptions::NumberUp NewSettings::PrintDialog::numberUp()
{
    return static_cast< PrintOptions::NumberUp >(m_settings->value("printDialog/numberUp", static_cast< uint >(Defaults::PrintDialog::numberUp())).toUInt());
}

void NewSettings::PrintDialog::setNumberUp(PrintOptions::NumberUp numberUp)
{
    m_settings->setValue("printDialog/numberUp", static_cast< uint >(numberUp));
}

PrintOptions::NumberUpLayout NewSettings::PrintDialog::numberUpLayout()
{
    return static_cast< PrintOptions::NumberUpLayout >(m_settings->value("printDialog/numberUpLayout", static_cast< uint >(Defaults::PrintDialog::numberUpLayout())).toUInt());
}

void NewSettings::PrintDialog::setNumberUpLayout(PrintOptions::NumberUpLayout numberUpLayout)
{
    m_settings->setValue("printDialog/numberUpLayout", static_cast< uint >(numberUpLayout));
}

void NewSettings::sync()
{
    m_settings->sync();

    m_pageItem.sync();
    m_presentationView.sync();
    m_documentView.sync();
    m_mainWindow.sync();
    m_printDialog.sync();
}

NewSettings::PageItem& NewSettings::pageItem()
{
    return m_pageItem;
}

NewSettings::PresentationView& NewSettings::presentationView()
{
    return m_presentationView;
}

NewSettings::DocumentView& NewSettings::documentView()
{
    return m_documentView;
}

NewSettings::MainWindow& NewSettings::mainWindow()
{
    return m_mainWindow;
}

NewSettings::PrintDialog& NewSettings::printDialog()
{
    return m_printDialog;
}

NewSettings::NewSettings(QObject* parent) : QObject(parent),
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
