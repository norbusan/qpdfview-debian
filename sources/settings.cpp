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

#include <QKeySequence>
#include <QSettings>

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

#include <QStandardPaths>

#else

#include <QDesktopServices>

#endif // QT_VERSION

#include "pageitem.h"
#include "documentview.h"

Settings::Settings(QObject* parent) : QObject(parent)
{
    m_settings = new QSettings(this);

    m_pageItem = new PageItem(m_settings);
    m_presentationView = new PresentationView(m_settings);
    m_documentView = new DocumentView(m_settings);
    m_mainWindow = new MainWindow(m_settings);
    m_printDialog = new PrintDialog(m_settings);
}

Settings::~Settings()
{
    delete m_pageItem;
    delete m_presentationView;
    delete m_documentView;
    delete m_mainWindow;
    delete m_printDialog;
}

Settings::PageItem* Settings::pageItem()
{
    return m_pageItem;
}

const Settings::PageItem* Settings::pageItem() const
{
    return m_pageItem;
}

Settings::PresentationView* Settings::presentationView()
{
    return m_presentationView;
}

const Settings::PresentationView* Settings::presentationView() const
{
    return m_presentationView;
}

Settings::DocumentView* Settings::documentView()
{
    return m_documentView;
}

const Settings::DocumentView* Settings::documentView() const
{
    return m_documentView;
}

Settings::MainWindow* Settings::mainWindow()
{
    return m_mainWindow;
}

const Settings::MainWindow* Settings::mainWindow() const
{
    return m_mainWindow;
}

Settings::PrintDialog* Settings::printDialog()
{
    return m_printDialog;
}

const Settings::PrintDialog* Settings::printDialog() const
{
    return m_printDialog;
}

QKeySequence Settings::shortcut(const QString& objectName, const QKeySequence& defaultShortcut)
{
    return m_settings->value("shortcuts/" + objectName, defaultShortcut).value< QKeySequence >();
}

void Settings::setShortcut(const QString& objectName, const QKeySequence& shortcut)
{
    m_settings->setValue("shortcuts/" + objectName, shortcut.toString(QKeySequence::PortableText));
}

void Settings::sync()
{
    m_settings->sync();

    ::PageItem::setCacheSize(pageItem()->cacheSize());

    ::PageItem::setDecoratePages(pageItem()->decoratePages());
    ::PageItem::setDecorateLinks(pageItem()->decorateLinks());
    ::PageItem::setDecorateFormFields(pageItem()->decorateFormFields());

    ::PageItem::setBackgroundColor(pageItem()->backgroundColor());
    ::PageItem::setPaperColor(pageItem()->paperColor());

    ::PageItem::setCopyToClipboardModifiers(pageItem()->copyToClipboardModifiers());
    ::PageItem::setAddAnnotationModifiers(pageItem()->addAnnotationModifiers());

    ::PageItem::setProgressIcon(QIcon::fromTheme("image-loading", QIcon(":/icons/image-loading.svg")));
    ::PageItem::setErrorIcon(QIcon::fromTheme("image-missing", QIcon(":icons/image-missing.svg")));

    ::DocumentView::setOpenUrl(documentView()->openUrl());

    ::DocumentView::setAutoRefresh(documentView()->autoRefresh());

    ::DocumentView::setPrefetch(documentView()->prefetch());
    ::DocumentView::setPrefetchDistance(documentView()->prefetchDistance());

    ::DocumentView::setPagesPerRow(documentView()->pagesPerRow());

    ::DocumentView::setLimitThumbnailsToResults(documentView()->limitThumbnailsToResults());

    ::DocumentView::setPageSpacing(documentView()->pageSpacing());
    ::DocumentView::setThumbnailSpacing(documentView()->thumbnailSpacing());

    ::DocumentView::setThumbnailSize(documentView()->thumbnailSize());

    ::DocumentView::setZoomModifiers(documentView()->zoomModifiers());
    ::DocumentView::setRotateModifiers(documentView()->rotateModifiers());
    ::DocumentView::setScrollModifiers(documentView()->scrollModifiers());

    ::DocumentView::setHighlightDuration(documentView()->highlightDuration());

    ::DocumentView::setSourceEditor(documentView()->sourceEditor());
}

// page item

Settings::PageItem::PageItem(QSettings* settings) : m_settings(settings) {}

int Settings::PageItem::cacheSize() const
{
    return m_settings->value("pageItem/cacheSize", Defaults::PageItem::cacheSize()).toInt();
}

void Settings::PageItem::setCacheSize(int cacheSize)
{
    m_settings->setValue("pageItem/cacheSize", cacheSize);
}

bool Settings::PageItem::decoratePages() const
{
    return m_settings->value("pageItem/decoratePages", Defaults::PageItem::decoratePages()).toBool();
}

void Settings::PageItem::setDecoratePages(bool on)
{
    m_settings->setValue("pageItem/decoratePages", on);
}

bool Settings::PageItem::decorateLinks() const
{
    return m_settings->value("pageItem/decorateLinks", Defaults::PageItem::decorateLinks()).toBool();
}

void Settings::PageItem::setDecorateLinks(bool on)
{
    m_settings->setValue("pageItem/decorateLinks", on);
}

bool Settings::PageItem::decorateFormFields() const
{
    return m_settings->value("pageItem/decorateFormFields", Defaults::PageItem::decorateFormFields()).toBool();
}

void Settings::PageItem::setDecorateFormFields(bool on)
{
    m_settings->setValue("pageItem/decorateFormFields", on);
}

QString Settings::PageItem::backgroundColor() const
{
    return m_settings->value("pageItem/backgroundColor", Defaults::PageItem::backgroundColor()).toString();
}

void Settings::PageItem::setBackgroundColor(const QString& color)
{
#if QT_VERSION >= QT_VERSION_CHECK(4,7,0)

    m_settings->setValue("pageItem/backgroundColor", QColor::isValidColor(color) ? color : Defaults::PageItem::backgroundColor());

#else

    m_settings->setValue("pageItem/backgroundColor", QColor(color).isValid() ? color : Defaults::PageItem::backgroundColor());

#endif // QT_VERSION
}

QString Settings::PageItem::paperColor() const
{
    return m_settings->value("pageItem/paperColor", Defaults::PageItem::paperColor()).toString();
}

void Settings::PageItem::setPaperColor(const QString& color)
{
#if QT_VERSION >= QT_VERSION_CHECK(4,7,0)

    m_settings->setValue("pageItem/paperColor", QColor::isValidColor(color) ? color : Defaults::PageItem::paperColor());

#else

    m_settings->setValue("pageItem/paperColor", QColor(color).isValid() ? color : Defaults::PageItem::paperColor());

#endif // QT_VERSION
}

Qt::KeyboardModifiers Settings::PageItem::copyToClipboardModifiers() const
{
    return static_cast< Qt::KeyboardModifiers >(m_settings->value("pageItem/copyToClipboardModifiers", static_cast< int >(Defaults::PageItem::copyToClipboardModifiers())).toInt());
}

void Settings::PageItem::setCopyToClipboardModifiers(const Qt::KeyboardModifiers& copyToClipboardModifiers)
{
    m_settings->setValue("pageItem/copyToClipboardModifiers", static_cast< int >(copyToClipboardModifiers));
}

Qt::KeyboardModifiers Settings::PageItem::addAnnotationModifiers() const
{
    return static_cast< Qt::KeyboardModifiers >(m_settings->value("pageItem/addAnnotationModifiers", static_cast< int >(Defaults::PageItem::addAnnotationModifiers())).toInt());
}

void Settings::PageItem::setAddAnnotationModifiers(const Qt::KeyboardModifiers& addAnnotationModifiers)
{
    m_settings->setValue("pageItem/addAnnotationModifiers", static_cast< int >(addAnnotationModifiers));
}

// presentation view

Settings::PresentationView::PresentationView(QSettings* settings) : m_settings(settings) {}

bool Settings::PresentationView::sync() const
{
    return m_settings->value("presentationView/sync", Defaults::PresentationView::sync()).toBool();
}

void Settings::PresentationView::setSync(bool on)
{
    m_settings->setValue("presentationView/sync", on);
}

int Settings::PresentationView::screen() const
{
    return m_settings->value("presentationView/screen", Defaults::PresentationView::screen()).toInt();
}

void Settings::PresentationView::setScreen(int screen)
{
    m_settings->setValue("presentationView/screen", screen);
}

// document view

Settings::DocumentView::DocumentView(QSettings* settings) : m_settings(settings) {}

bool Settings::DocumentView::openUrl() const
{
    return m_settings->value("documentView/openUrl", Defaults::DocumentView::openUrl()).toBool();
}

void Settings::DocumentView::setOpenUrl(bool on)
{
    m_settings->setValue("documentView/openUrl", on);
}

bool Settings::DocumentView::autoRefresh() const
{
    return m_settings->value("documentView/autoRefresh", Defaults::DocumentView::autoRefresh()).toBool();
}

void Settings::DocumentView::setAutoRefresh(bool on)
{
    m_settings->setValue("documentView/autoRefresh", on);
}

bool Settings::DocumentView::prefetch() const
{
    return m_settings->value("documentView/prefetch", Defaults::DocumentView::prefetch()).toBool();
}

void Settings::DocumentView::setPrefetch(bool on)
{
    m_settings->setValue("documentView/prefetch", on);
}

int Settings::DocumentView::prefetchDistance() const
{
    return m_settings->value("documentView/prefetchDistance", Defaults::DocumentView::prefetchDistance()).toInt();
}

void Settings::DocumentView::setPrefetchDistance(int prefetchDistance)
{
    m_settings->setValue("documentView/prefetchDistance", prefetchDistance);
}

int Settings::DocumentView::pagesPerRow() const
{
    return m_settings->value("documentView/pagesPerRow", Defaults::DocumentView::pagesPerRow()).toInt();
}

void Settings::DocumentView::setPagesPerRow(int pagesPerRow)
{
    m_settings->setValue("documentView/pagesPerRow", pagesPerRow);
}

bool Settings::DocumentView::limitThumbnailsToResults() const
{
    return m_settings->value("documentView/limitThumbnailsToResults", Defaults::DocumentView::limitThumbnailsToResults()).toBool();
}

void Settings::DocumentView::setLimitThumbnailsToResults(bool limitThumbnailsToResults)
{
    m_settings->setValue("documentView/limitThumbnailsToResults", limitThumbnailsToResults);
}

qreal Settings::DocumentView::pageSpacing() const
{
    return m_settings->value("documentView/pageSpacing", Defaults::DocumentView::pageSpacing()).toReal();
}

void Settings::DocumentView::setPageSpacing(qreal pageSpacing)
{
    m_settings->setValue("documentView/pageSpacing", pageSpacing);
}

qreal Settings::DocumentView::thumbnailSpacing() const
{
    return m_settings->value("documentView/thumbnailSpacing", Defaults::DocumentView::thumbnailSpacing()).toReal();
}

void Settings::DocumentView::setThumbnailSpacing(qreal thumbnailSpacing)
{
    m_settings->setValue("documentView/thumbnailSpacing", thumbnailSpacing);
}

qreal Settings::DocumentView::thumbnailSize() const
{
    return m_settings->value("documentView/thumbnailSize", Defaults::DocumentView::thumbnailSize()).toReal();
}

void Settings::DocumentView::setThumbnailSize(qreal thumbnailSize)
{
    m_settings->setValue("documentView/thumbnailSize", thumbnailSize);
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

int Settings::DocumentView::highlightDuration() const
{
    return m_settings->value("documentView/highlightDuration", Defaults::DocumentView::highlightDuration()).toInt();
}

void Settings::DocumentView::setHighlightDuration(int highlightDuration)
{
    m_settings->setValue("documentView/highlightDuration", highlightDuration);
}

QString Settings::DocumentView::sourceEditor() const
{
    return m_settings->value("documentView/sourceEditor").toString();
}

void Settings::DocumentView::setSourceEditor(const QString& sourceEditor)
{
    m_settings->setValue("documentView/sourceEditor", sourceEditor);
}

// per-tab settings

bool Settings::DocumentView::continuousMode() const
{
    return m_settings->value("documentView/continuousMode", Defaults::DocumentView::continuousMode()).toBool();
}

void Settings::DocumentView::setContinuousMode(bool on)
{
    m_settings->setValue("documentView/continuousMode", on);
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

void Settings::DocumentView::setInvertColors(bool on)
{
    m_settings->setValue("documentView/invertColors", on);
}

bool Settings::DocumentView::highlightAll() const
{
    return m_settings->value("documentView/highlightAll", Defaults::DocumentView::highlightAll()).toBool();
}

void Settings::DocumentView::setHighlightAll(bool on)
{
    m_settings->setValue("documentView/highlightAll", on);
}

// main window

Settings::MainWindow::MainWindow(QSettings* settings) : m_settings(settings) {}

bool Settings::MainWindow::trackRecentlyUsed() const
{
    return m_settings->value("mainWindow/trackRecentlyUsed", Defaults::MainWindow::trackRecentlyUsed()).toBool();
}

void Settings::MainWindow::setTrackRecentlyUsed(bool on)
{
    m_settings->setValue("mainWindow/trackRecentlyUsed", on);
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

QTabWidget::TabPosition Settings::MainWindow::tabPosition() const
{
    return static_cast< QTabWidget::TabPosition >(m_settings->value("mainWindow/tabPosition", static_cast< uint >(Defaults::MainWindow::tabPosition())).toUInt());
}

void Settings::MainWindow::setTabPosition(QTabWidget::TabPosition tabPosition)
{
    m_settings->setValue("mainWindow/tabPosition", static_cast< uint >(tabPosition));
}

TabWidget::TabBarPolicy Settings::MainWindow::tabVisibility() const
{
    return static_cast< TabWidget::TabBarPolicy >(m_settings->value("mainWindow/tabVisibility", static_cast< uint >(Defaults::MainWindow::tabVisibility())).toUInt());
}

void Settings::MainWindow::setTabVisibility(TabWidget::TabBarPolicy tabVisibility)
{
    m_settings->setValue("mainWindow/tabVisibility", static_cast< uint >(tabVisibility));
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

static QStringList trimmed(const QStringList& list)
{
    QStringList trimmedList;

    foreach(QString item, list)
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

// print dialog

Settings::PrintDialog::PrintDialog(QSettings* settings) : m_settings(settings) {}

bool Settings::PrintDialog::collateCopies()
{
    return m_settings->value("printDialog/collateCopies", Defaults::PrintDialog::collateCopies()).toBool();
}

void Settings::PrintDialog::setCollateCopies(bool collateCopies)
{
    m_settings->setValue("printDialog/collateCopies", collateCopies);
}

QPrinter::PageOrder Settings::PrintDialog::pageOrder()
{
    return static_cast< QPrinter::PageOrder >(m_settings->value("printDialog/pageOrder", static_cast< int >(Defaults::PrintDialog::pageOrder())).toInt());
}

void Settings::PrintDialog::setPageOrder(QPrinter::PageOrder pageOrder)
{
    m_settings->setValue("printDialog/pageOrder", static_cast< int >(pageOrder));
}

QPrinter::Orientation Settings::PrintDialog::orientation()
{
    return static_cast< QPrinter::Orientation >(m_settings->value("printDialog/orientation", static_cast< int >(Defaults::PrintDialog::orientation())).toInt());
}

void Settings::PrintDialog::setOrientation(QPrinter::Orientation orientation)
{
    m_settings->setValue("printDialog/orientation", static_cast< int >(orientation));
}

QPrinter::ColorMode Settings::PrintDialog::colorMode()
{
    return static_cast< QPrinter::ColorMode >(m_settings->value("printDialog/colorMode", static_cast< int >(Defaults::PrintDialog::colorMode())).toInt());
}

void Settings::PrintDialog::setColorMode(QPrinter::ColorMode colorMode)
{
    m_settings->setValue("printDialog/colorMode", static_cast< int >(colorMode));
}

QPrinter::DuplexMode Settings::PrintDialog::duplex()
{
    return static_cast< QPrinter::DuplexMode >(m_settings->value("printDialog/duplex", static_cast< int >(Defaults::PrintDialog::duplex())).toInt());
}

void Settings::PrintDialog::setDuplex(QPrinter::DuplexMode duplex)
{
    m_settings->setValue("printDialog/duplex", static_cast< int >(duplex));
}

bool Settings::PrintDialog::fitToPage()
{
    return m_settings->value("printDialog/fitToPage", Defaults::PrintDialog::fitToPage()).toBool();
}

void Settings::PrintDialog::setFitToPage(bool fitToPage)
{
    m_settings->setValue("printDialog/fitToPage", fitToPage);
}

PrintOptions::PageSet Settings::PrintDialog::pageSet()
{
    return static_cast< PrintOptions::PageSet >(m_settings->value("printDialog/pageSet", static_cast< uint >(Defaults::PrintDialog::pageSet())).toUInt());
}

void Settings::PrintDialog::setPageSet(PrintOptions::PageSet pageSet)
{
    m_settings->setValue("printDialog/pageSet", static_cast< uint >(pageSet));
}

PrintOptions::NumberUp Settings::PrintDialog::numberUp()
{
    return static_cast< PrintOptions::NumberUp >(m_settings->value("printDialog/numberUp", static_cast< uint >(Defaults::PrintDialog::numberUp())).toUInt());
}

void Settings::PrintDialog::setNumberUp(PrintOptions::NumberUp numberUp)
{
    m_settings->setValue("printDialog/numberUp", static_cast< uint >(numberUp));
}

PrintOptions::NumberUpLayout Settings::PrintDialog::numberUpLayout()
{
    return static_cast< PrintOptions::NumberUpLayout >(m_settings->value("printDialog/numberUpLayout", static_cast< uint >(Defaults::PrintDialog::numberUpLayout())).toUInt());
}

void Settings::PrintDialog::setNumberUpLayout(PrintOptions::NumberUpLayout numberUpLayout)
{
    m_settings->setValue("printDialog/numberUpLayout", static_cast< uint >(numberUpLayout));
}

QString Defaults::MainWindow::path()
{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

    return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

#else

    return QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);

#endif // QT_VERSION
}
