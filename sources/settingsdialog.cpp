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

#include "settingsdialog.h"

#include <QApplication>
#include <QCheckBox>
#include <QDesktopWidget>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QShortcut>
#include <QTableView>

#include "settings.h"
#include "model.h"
#include "pluginhandler.h"
#include "shortcuthandler.h"
#include "documentview.h"
#include "miscellaneous.h"

namespace
{

using namespace qpdfview;

void addSettingsWidget(QTabWidget* tabWidget, SettingsWidget*& settingsWidget, PluginHandler::FileType fileType)
{
    settingsWidget = PluginHandler::instance()->createSettingsWidget(fileType, tabWidget);

    if(settingsWidget != 0)
    {
        tabWidget->addTab(settingsWidget, PluginHandler::fileTypeName(fileType));
    }
}

void setCurrentTextToColorName(QComboBox* comboBox, const QColor& color)
{
    comboBox->lineEdit()->setText(color.isValid() ? color.name() : QString());
}

QColor getValidColorFromCurrentText(const QComboBox* comboBox, const QColor& defaultColor)
{
    const QColor color(comboBox->currentText());

    return color.isValid() ? color : defaultColor;
}

void setCurrentIndexFromKeyboardModifiers(QComboBox* comboBox, const Qt::KeyboardModifiers& modifiers)
{
    comboBox->setCurrentIndex(comboBox->findData(static_cast< int >(modifiers)));
}

Qt::KeyboardModifier getKeyboardModifierFromItemData(const QComboBox* comboBox)
{
    return static_cast< Qt::KeyboardModifier >(comboBox->itemData(comboBox->currentIndex()).toInt());
}

} // anonymous

namespace qpdfview
{

Settings* SettingsDialog::s_settings = 0;

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent)
{
    if(s_settings == 0)
    {
        s_settings = Settings::instance();
    }

    setWindowTitle(tr("Settings") + QLatin1String(" - qpdfview"));

    m_graphicsTabWidget = new QTabWidget(this);
    m_graphicsTabWidget->addTab(new QWidget(this), tr("General"));

    addSettingsWidget(m_graphicsTabWidget, m_pdfSettingsWidget, PluginHandler::PDF);
    addSettingsWidget(m_graphicsTabWidget, m_psSettingsWidget, PluginHandler::PS);
    addSettingsWidget(m_graphicsTabWidget, m_djvuSettingsWidget, PluginHandler::DjVu);

    m_graphicsLayout = new QFormLayout(m_graphicsTabWidget->widget(0));

    m_shortcutsTableView = new QTableView(this);

    m_shortcutsTableView->setModel(ShortcutHandler::instance());

    connect(this, SIGNAL(accepted()), ShortcutHandler::instance(), SLOT(submit()));
    connect(this, SIGNAL(rejected()), ShortcutHandler::instance(), SLOT(revert()));

    m_shortcutsTableView->setFrameShape(QFrame::NoFrame);
    m_shortcutsTableView->setAlternatingRowColors(true);

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

    m_shortcutsTableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_shortcutsTableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

#else

    m_shortcutsTableView->horizontalHeader()->setResizeMode(0, QHeaderView::ResizeToContents);
    m_shortcutsTableView->horizontalHeader()->setResizeMode(1, QHeaderView::Stretch);

#endif // QT_VERSION

    m_shortcutsTableView->verticalHeader()->setVisible(false);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(new QWidget(this), tr("&Behavior"));
    m_tabWidget->addTab(m_graphicsTabWidget, tr("&Graphics"));
    m_tabWidget->addTab(new QWidget(this), tr("&Interface"));
    m_tabWidget->addTab(m_shortcutsTableView, tr("&Shortcuts"));
    m_tabWidget->addTab(new QWidget(this), tr("&Modifiers"));

    m_dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    connect(m_dialogButtonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(m_dialogButtonBox, SIGNAL(rejected()), SLOT(reject()));

    m_defaultsButton = m_dialogButtonBox->addButton(tr("Defaults"), QDialogButtonBox::ResetRole);
    connect(m_defaultsButton, SIGNAL(clicked()), SLOT(reset()));

    m_defaultsOnCurrentTabButton = m_dialogButtonBox->addButton(tr("Defaults on current tab"), QDialogButtonBox::ResetRole);
    connect(m_defaultsOnCurrentTabButton, SIGNAL(clicked()), SLOT(resetCurrentTab()));

    m_behaviorLayout = new QFormLayout(m_tabWidget->widget(0));
    m_interfaceLayout = new QFormLayout(m_tabWidget->widget(2));
    m_modifiersLayout = new QFormLayout(m_tabWidget->widget(4));

    setLayout(new QVBoxLayout(this));
    layout()->addWidget(m_tabWidget);
    layout()->addWidget(m_dialogButtonBox);

    resize(s_settings->mainWindow().settingsDialogSize(sizeHint()));

    createBehaviorTab();
    createGraphicsTab();
    createInterfaceTab();
    createModifiersTab();
}

SettingsDialog::~SettingsDialog()
{
    s_settings->mainWindow().setSettingsDialogSize(size());
}

void SettingsDialog::accept()
{
    QDialog::accept();

    acceptBehaivorTab();

    acceptGraphicsTab();

    acceptInterfaceTab();

    acceptModifiersTab();
}

void SettingsDialog::reset()
{
    resetBehaviorTab();

    resetGraphicsTab();

    resetInterfaceTab();

    ShortcutHandler::instance()->reset();

    resetModifiersTab();
}

void SettingsDialog::resetCurrentTab()
{
    switch(m_tabWidget->currentIndex())
    {
    default:
        reset();
        break;
    case 0:
        resetBehaviorTab();
        break;
    case 1:
        resetGraphicsTab();
        break;
    case 2:
        resetInterfaceTab();
        break;
    case 3:
        ShortcutHandler::instance()->reset();
        break;
    case 4:
        resetModifiersTab();
        break;
    }
}

void SettingsDialog::createBehaviorTab()
{
    // open URL

    m_openUrlCheckBox = addCheckBox(m_behaviorLayout, tr("Open URL:"),
                                    s_settings->documentView().openUrl());

    // auto-refresh

    m_autoRefreshCheckBox = addCheckBox(m_behaviorLayout, tr("Auto-refresh:"),
                                        s_settings->documentView().autoRefresh());

    // track recently used

    m_trackRecentlyUsedCheckBox = addCheckBox(m_behaviorLayout, tr("Track recently used:"),
                                              s_settings->mainWindow().trackRecentlyUsed());

    m_trackRecentlyUsedCheckBox->setToolTip(tr("Effective after restart."));

    // keep recently closed

    m_keepRecentlyClosedCheckBox = addCheckBox(m_behaviorLayout, tr("Keep recently closed:"),
                                               s_settings->mainWindow().keepRecentlyClosed());

    m_keepRecentlyClosedCheckBox->setToolTip(tr("Effective after restart."));

    // restore tabs

    m_restoreTabsCheckBox = addCheckBox(m_behaviorLayout, tr("Restore tabs:"),
                                        s_settings->mainWindow().restoreTabs());

    // restore bookmarks

    m_restoreBookmarksCheckBox = addCheckBox(m_behaviorLayout, tr("Restore bookmarks:"),
                                             s_settings->mainWindow().restoreBookmarks());

    // restore per-file settings

    m_restorePerFileSettingsCheckBox = addCheckBox(m_behaviorLayout, tr("Restore per-file settings:"),
                                                   s_settings->mainWindow().restorePerFileSettings());

    // save database interval

    m_saveDatabaseInterval = addSpinBox(m_behaviorLayout, tr("Save database interval:"), tr(" min"), tr("Never"),
                                        0, 60, 1, s_settings->mainWindow().saveDatabaseInterval() / 1000 / 60);

#ifndef WITH_SQL

    m_restoreTabsCheckBox->setEnabled(false);
    m_restoreBookmarksCheckBox->setEnabled(false);
    m_restorePerFileSettingsCheckBox->setEnabled(false);
    m_saveDatabaseInterval->setEnabled(false);

#endif // WITH_SQL

    // synchronize presentation

    m_synchronizePresentationCheckBox = addCheckBox(m_behaviorLayout, tr("Synchronize presentation:"),
                                                    s_settings->presentationView().synchronize());

    // presentation screen

    m_presentationScreenSpinBox = addSpinBox(m_behaviorLayout, tr("Presentation screen:"), QString(), tr("Default"),
                                             -1, QApplication::desktop()->screenCount() - 1, 1, s_settings->presentationView().screen());

    // synchronize outline view

    m_synchronizeOutlineViewCheckBox = addCheckBox(m_behaviorLayout, tr("Synchronize outline view:"),
                                                   s_settings->mainWindow().synchronizeOutlineView());

    // scroll if not visible

    m_scrollIfNotVisibleCheckBox = addCheckBox(m_behaviorLayout, tr("Scroll if not visible:"),
                                               s_settings->documentView().scrollIfNotVisible());

    // zoom factor

    m_zoomFactorSpinBox = addDoubleSpinBox(m_behaviorLayout, tr("Zoom factor:"), QString(),
                                           1.0, 2.0, 0.05, s_settings->documentView().zoomFactor());

    // highlight duration

    m_highlightDurationSpinBox = addSpinBox(m_behaviorLayout, tr("Highlight duration:"), tr(" ms"), tr("None"),
                                            0, 60000, 500, s_settings->documentView().highlightDuration());

    // highlight color

    m_highlightColorComboBox = addColorComboBox(m_behaviorLayout, tr("Highlight color:"),
                                                s_settings->pageItem().highlightColor());

    // annotation color

    m_annotationColorComboBox = addColorComboBox(m_behaviorLayout, tr("Annotation color:"),
                                                 s_settings->pageItem().annotationColor());

    // source editor

    m_sourceEditorLineEdit = new QLineEdit(this);
    m_sourceEditorLineEdit->setText(s_settings->documentView().sourceEditor());
    m_sourceEditorLineEdit->setToolTip(tr("'%1' is replaced by the absolute file path. '%2' resp. '%3' is replaced by line resp. column number."));

    m_behaviorLayout->addRow(tr("Source editor:"), m_sourceEditorLineEdit);
}

void SettingsDialog::acceptBehaivorTab()
{
    s_settings->documentView().setOpenUrl(m_openUrlCheckBox->isChecked());

    s_settings->documentView().setAutoRefresh(m_autoRefreshCheckBox->isChecked());

    s_settings->mainWindow().setTrackRecentlyUsed(m_trackRecentlyUsedCheckBox->isChecked());
    s_settings->mainWindow().setKeepRecentlyClosed(m_keepRecentlyClosedCheckBox->isChecked());

    s_settings->mainWindow().setRestoreTabs(m_restoreTabsCheckBox->isChecked());
    s_settings->mainWindow().setRestoreBookmarks(m_restoreBookmarksCheckBox->isChecked());
    s_settings->mainWindow().setRestorePerFileSettings(m_restorePerFileSettingsCheckBox->isChecked());
    s_settings->mainWindow().setSaveDatabaseInterval(m_saveDatabaseInterval->value() * 60 * 1000);

    s_settings->presentationView().setSynchronize(m_synchronizePresentationCheckBox->isChecked());
    s_settings->presentationView().setScreen(m_presentationScreenSpinBox->value());

    s_settings->mainWindow().setSynchronizeOutlineView(m_synchronizeOutlineViewCheckBox->isChecked());

    s_settings->documentView().setScrollIfNotVisible(m_scrollIfNotVisibleCheckBox->isChecked());
    s_settings->documentView().setZoomFactor(m_zoomFactorSpinBox->value());

    s_settings->documentView().setHighlightDuration(m_highlightDurationSpinBox->value());
    s_settings->pageItem().setHighlightColor(getValidColorFromCurrentText(m_highlightColorComboBox, Defaults::PageItem::highlightColor()));
    s_settings->pageItem().setAnnotationColor(getValidColorFromCurrentText(m_annotationColorComboBox, Defaults::PageItem::annotationColor()));

    s_settings->documentView().setSourceEditor(m_sourceEditorLineEdit->text());
}

void SettingsDialog::resetBehaviorTab()
{
    m_openUrlCheckBox->setChecked(Defaults::DocumentView::openUrl());

    m_autoRefreshCheckBox->setChecked(Defaults::DocumentView::autoRefresh());

    m_trackRecentlyUsedCheckBox->setChecked(Defaults::MainWindow::trackRecentlyUsed());
    m_keepRecentlyClosedCheckBox->setChecked(Defaults::MainWindow::keepRecentlyClosed());

    m_restoreTabsCheckBox->setChecked(Defaults::MainWindow::restoreTabs());
    m_restoreBookmarksCheckBox->setChecked(Defaults::MainWindow::restoreBookmarks());
    m_restorePerFileSettingsCheckBox->setChecked(Defaults::MainWindow::restorePerFileSettings());
    m_saveDatabaseInterval->setValue(Defaults::MainWindow::saveDatabaseInterval());

    m_synchronizePresentationCheckBox->setChecked(Defaults::PresentationView::synchronize());
    m_presentationScreenSpinBox->setValue(Defaults::PresentationView::screen());

    m_synchronizeOutlineViewCheckBox->setChecked(Defaults::MainWindow::synchronizeOutlineView());

    m_scrollIfNotVisibleCheckBox->setChecked(Defaults::DocumentView::scrollIfNotVisible());
    m_zoomFactorSpinBox->setValue(Defaults::DocumentView::zoomFactor());

    m_highlightDurationSpinBox->setValue(Defaults::DocumentView::highlightDuration());
    setCurrentTextToColorName(m_highlightColorComboBox, Defaults::PageItem::highlightColor());
    setCurrentTextToColorName(m_annotationColorComboBox, Defaults::PageItem::annotationColor());

    m_sourceEditorLineEdit->clear();
}

void SettingsDialog::createGraphicsTab()
{
    // use tiling

    m_useTilingCheckBox = addCheckBox(m_graphicsLayout, tr("Use tiling:"),
                                      s_settings->pageItem().useTiling());

    // keep obsolete pixmaps

    m_keepObsoletePixmapsCheckBox = addCheckBox(m_graphicsLayout, tr("Keep obsolete pixmaps:"),
                                                s_settings->pageItem().keepObsoletePixmaps());

#if QT_VERSION >= QT_VERSION_CHECK(5,1,0)

    // use device pixel ratio

    m_useDevicePixelRatioCheckBox = addCheckBox(m_graphicsLayout, tr("Use device pixel ratio:"),
                                                s_settings->pageItem().useDevicePixelRatio());

#endif // QT_VERSION

    // trim margins

    m_trimMarginsCheckBox = addCheckBox(m_graphicsLayout, tr("Trim margins:"),
                                        s_settings->pageItem().trimMargins());

    // decorate pages

    m_decoratePagesCheckBox = addCheckBox(m_graphicsLayout, tr("Decorate pages:"),
                                          s_settings->pageItem().decoratePages());

    // decorate links

    m_decorateLinksCheckBox = addCheckBox(m_graphicsLayout, tr("Decorate links:"),
                                          s_settings->pageItem().decorateLinks());

    // decorate form fields

    m_decorateFormFieldsCheckBox = addCheckBox(m_graphicsLayout, tr("Decorate form fields:"),
                                               s_settings->pageItem().decorateFormFields());

    // background color

    m_backgroundColorComboBox = addColorComboBox(m_graphicsLayout, tr("Background color:"),
                                                 s_settings->pageItem().backgroundColor());

    // paper color

    m_paperColorComboBox = addColorComboBox(m_graphicsLayout, tr("Paper color:"),
                                            s_settings->pageItem().paperColor());

    // presentation background color

    m_presentationBackgroundColorComboBox = addColorComboBox(m_graphicsLayout, tr("Presentation background color:"),
                                                             s_settings->presentationView().backgroundColor());

    // pages per row

    m_pagesPerRowSpinBox = addSpinBox(m_graphicsLayout, tr("Pages per row:"), QString(), QString(),
                                      1, 10, 1, s_settings->documentView().pagesPerRow());

    // page spacing

    m_pageSpacingSpinBox = addDoubleSpinBox(m_graphicsLayout, tr("Page spacing:"), tr(" px"),
                                            0.0, 25.0, 0.25, s_settings->documentView().pageSpacing());

    // thumbnail spacing

    m_thumbnailSpacingSpinBox = addDoubleSpinBox(m_graphicsLayout, tr("Thumbnail spacing:"), tr(" px"),
                                                 0.0, 25.0, 0.25, s_settings->documentView().thumbnailSpacing());

    // thumbnail size

    m_thumbnailSizeSpinBox = addDoubleSpinBox(m_graphicsLayout, tr("Thumbnail size:"), tr(" px"),
                                              30.0, 300.0, 10.0, s_settings->documentView().thumbnailSize());

    // cache size

    m_cacheSizeComboBox = new QComboBox(this);
    m_cacheSizeComboBox->addItem(tr("%1 MB").arg(0), 0);
    m_cacheSizeComboBox->addItem(tr("%1 MB").arg(8), 8 * 1024 * 1024);
    m_cacheSizeComboBox->addItem(tr("%1 MB").arg(16), 16 * 1024 * 1024);
    m_cacheSizeComboBox->addItem(tr("%1 MB").arg(32), 32 * 1024 * 1024);
    m_cacheSizeComboBox->addItem(tr("%1 MB").arg(64), 64 * 1024 * 1024);
    m_cacheSizeComboBox->addItem(tr("%1 MB").arg(128), 128 * 1024 * 1024);
    m_cacheSizeComboBox->addItem(tr("%1 MB").arg(256), 256 * 1024 * 1024);
    m_cacheSizeComboBox->addItem(tr("%1 MB").arg(512), 512 * 1024 * 1024);
    m_cacheSizeComboBox->addItem(tr("%1 MB").arg(1024), 1073741823);
    m_cacheSizeComboBox->addItem(tr("%1 MB").arg(2048), 2147483647);

    const int cacheSize = s_settings->pageItem().cacheSize();
    int cacheSizeIndex = m_cacheSizeComboBox->findData(cacheSize);

    if(cacheSizeIndex == -1)
    {
        m_cacheSizeComboBox->addItem(tr("%1 MB").arg(cacheSize / 1024 / 1024), cacheSize);

        cacheSizeIndex = m_cacheSizeComboBox->count() - 1;
    }

    m_cacheSizeComboBox->setCurrentIndex(cacheSizeIndex);

    m_graphicsLayout->addRow(tr("Cache size:"), m_cacheSizeComboBox);

    // prefetch

    m_prefetchCheckBox = addCheckBox(m_graphicsLayout, tr("Prefetch:"),
                                     s_settings->documentView().prefetch());

    // prefetch distance

    m_prefetchDistanceSpinBox = addSpinBox(m_graphicsLayout, tr("Prefetch distance:"), QString(), QString(),
                                           1, 10, 1, s_settings->documentView().prefetchDistance());
}

void SettingsDialog::acceptGraphicsTab()
{
    s_settings->pageItem().setUseTiling(m_useTilingCheckBox->isChecked());
    s_settings->pageItem().setKeepObsoletePixmaps(m_keepObsoletePixmapsCheckBox->isChecked());

#if QT_VERSION >= QT_VERSION_CHECK(5,1,0)

    s_settings->pageItem().setUseDevicePixelRatio(m_useDevicePixelRatioCheckBox->isChecked());

#endif // QT_VERSION

    s_settings->pageItem().setTrimMargins(m_trimMarginsCheckBox->isChecked());

    s_settings->pageItem().setDecoratePages(m_decoratePagesCheckBox->isChecked());
    s_settings->pageItem().setDecorateLinks(m_decorateLinksCheckBox->isChecked());
    s_settings->pageItem().setDecorateFormFields(m_decorateFormFieldsCheckBox->isChecked());

    s_settings->pageItem().setBackgroundColor(getValidColorFromCurrentText(m_backgroundColorComboBox, Defaults::PageItem::backgroundColor()));
    s_settings->pageItem().setPaperColor(getValidColorFromCurrentText(m_paperColorComboBox, Defaults::PageItem::paperColor()));
    s_settings->presentationView().setBackgroundColor(getValidColorFromCurrentText(m_presentationBackgroundColorComboBox, Defaults::PresentationView::backgroundColor()));

    s_settings->documentView().setPagesPerRow(m_pagesPerRowSpinBox->value());

    s_settings->documentView().setPageSpacing(m_pageSpacingSpinBox->value());
    s_settings->documentView().setThumbnailSpacing(m_thumbnailSpacingSpinBox->value());

    s_settings->documentView().setThumbnailSize(m_thumbnailSizeSpinBox->value());

    s_settings->pageItem().setCacheSize(m_cacheSizeComboBox->itemData(m_cacheSizeComboBox->currentIndex()).toInt());
    s_settings->documentView().setPrefetch(m_prefetchCheckBox->isChecked());
    s_settings->documentView().setPrefetchDistance(m_prefetchDistanceSpinBox->value());

    if(m_pdfSettingsWidget != 0)
    {
        m_pdfSettingsWidget->accept();
    }

    if(m_psSettingsWidget != 0)
    {
        m_psSettingsWidget->accept();
    }

    if(m_djvuSettingsWidget != 0)
    {
        m_djvuSettingsWidget->accept();
    }
}

void SettingsDialog::resetGraphicsTab()
{
    m_useTilingCheckBox->setChecked(Defaults::PageItem::useTiling());
    m_keepObsoletePixmapsCheckBox->setChecked(Defaults::PageItem::keepObsoletePixmaps());

#if QT_VERSION >= QT_VERSION_CHECK(5,1,0)

    m_useDevicePixelRatioCheckBox->setChecked(Defaults::PageItem::useDevicePixelRatio());

#endif // QT_VERSION

    m_trimMarginsCheckBox->setChecked(Defaults::PageItem::trimMargins());

    m_decoratePagesCheckBox->setChecked(Defaults::PageItem::decoratePages());
    m_decorateLinksCheckBox->setChecked(Defaults::PageItem::decorateLinks());
    m_decorateFormFieldsCheckBox->setChecked(Defaults::PageItem::decorateFormFields());

    setCurrentTextToColorName(m_backgroundColorComboBox, Defaults::PageItem::backgroundColor());
    setCurrentTextToColorName(m_paperColorComboBox, Defaults::PageItem::paperColor());
    setCurrentTextToColorName(m_presentationBackgroundColorComboBox, Defaults::PresentationView::backgroundColor());

    m_pagesPerRowSpinBox->setValue(Defaults::DocumentView::pagesPerRow());

    m_pageSpacingSpinBox->setValue(Defaults::DocumentView::pageSpacing());
    m_thumbnailSpacingSpinBox->setValue(Defaults::DocumentView::thumbnailSpacing());

    m_thumbnailSizeSpinBox->setValue(Defaults::DocumentView::thumbnailSize());

    m_cacheSizeComboBox->setCurrentIndex(m_cacheSizeComboBox->findData(Defaults::PageItem::cacheSize()));
    m_prefetchCheckBox->setChecked(Defaults::DocumentView::prefetch());
    m_prefetchDistanceSpinBox->setValue(Defaults::DocumentView::prefetchDistance());

    if(m_pdfSettingsWidget != 0)
    {
        m_pdfSettingsWidget->reset();
    }

    if(m_psSettingsWidget != 0)
    {
        m_psSettingsWidget->reset();
    }

    if(m_djvuSettingsWidget != 0)
    {
        m_djvuSettingsWidget->reset();
    }
}

void SettingsDialog::createInterfaceTab()
{
    // extended search dock

    m_extendedSearchDock = addCheckBox(m_interfaceLayout, tr("Extended search dock:"),
                                       s_settings->mainWindow().extendedSearchDock());

    m_extendedSearchDock->setToolTip(tr("Effective after restart."));

    // annotation overlay

    m_annotationOverlayCheckBox = addCheckBox(m_interfaceLayout, tr("Annotation overlay:"),
                                              s_settings->pageItem().annotationOverlay());

    // form field overlay

    m_formFieldOverlayCheckBox = addCheckBox(m_interfaceLayout, tr("Form field overlay:"),
                                             s_settings->pageItem().formFieldOverlay());

    // tab position

    m_tabPositionComboBox = new QComboBox(this);
    m_tabPositionComboBox->addItem(tr("Top"), static_cast< uint >(QTabWidget::North));
    m_tabPositionComboBox->addItem(tr("Bottom"), static_cast< uint >(QTabWidget::South));
    m_tabPositionComboBox->addItem(tr("Left"), static_cast< uint >(QTabWidget::West));
    m_tabPositionComboBox->addItem(tr("Right"), static_cast< uint >(QTabWidget::East));
    m_tabPositionComboBox->setCurrentIndex(m_tabPositionComboBox->findData(s_settings->mainWindow().tabPosition()));

    m_interfaceLayout->addRow(tr("Tab position:"), m_tabPositionComboBox);

    // tab visibility

    m_tabVisibilityComboBox = new QComboBox(this);
    m_tabVisibilityComboBox->addItem(tr("As needed"), static_cast< uint >(TabWidget::TabBarAsNeeded));
    m_tabVisibilityComboBox->addItem(tr("Always"), static_cast< uint >(TabWidget::TabBarAlwaysOn));
    m_tabVisibilityComboBox->addItem(tr("Never"), static_cast< uint >(TabWidget::TabBarAlwaysOff));
    m_tabVisibilityComboBox->setCurrentIndex(m_tabVisibilityComboBox->findData(s_settings->mainWindow().tabVisibility()));

    m_interfaceLayout->addRow(tr("Tab visibility:"), m_tabVisibilityComboBox);

    // spread tabs

    m_spreadTabsCheckBox = addCheckBox(m_interfaceLayout, tr("Spread tabs:"),
                                       s_settings->mainWindow().spreadTabs());

    // new tab next to current tab

    m_newTabNextToCurrentTabCheckBox = addCheckBox(m_interfaceLayout, tr("New tab next to current tab:"),
                                                   s_settings->mainWindow().newTabNextToCurrentTab());

    // exit after last tab

    m_exitAfterLastTabCheckBox = addCheckBox(m_interfaceLayout, tr("Exit after last tab:"),
                                             s_settings->mainWindow().exitAfterLastTab());

    // recently used count

    m_recentlyUsedCountSpinBox = addSpinBox(m_interfaceLayout, tr("Recently used count:"), QString(), QString(),
                                            1, 50, 1, s_settings->mainWindow().recentlyUsedCount());

    m_recentlyUsedCountSpinBox->setToolTip(tr("Effective after restart."));

    // recently closed count

    m_recentlyClosedCountSpinBox = addSpinBox(m_interfaceLayout, tr("Recently closed count:"), QString(), QString(),
                                              1, 25, 1, s_settings->mainWindow().recentlyClosedCount());

    m_recentlyClosedCountSpinBox->setToolTip(tr("Effective after restart."));

    // file tool bar

    m_fileToolBarLineEdit = new QLineEdit(this);
    m_fileToolBarLineEdit->setText(s_settings->mainWindow().fileToolBar().join(","));
    m_fileToolBarLineEdit->setToolTip(tr("Effective after restart."));

    m_interfaceLayout->addRow(tr("File tool bar:"), m_fileToolBarLineEdit);

    // edit tool bar

    m_editToolBarLineEdit = new QLineEdit(this);
    m_editToolBarLineEdit->setText(s_settings->mainWindow().editToolBar().join(","));
    m_editToolBarLineEdit->setToolTip(tr("Effective after restart."));

    m_interfaceLayout->addRow(tr("Edit tool bar:"), m_editToolBarLineEdit);

    // view tool bar

    m_viewToolBarLineEdit = new QLineEdit(this);
    m_viewToolBarLineEdit->setText(s_settings->mainWindow().viewToolBar().join(","));
    m_viewToolBarLineEdit->setToolTip(tr("Effective after restart."));

    m_interfaceLayout->addRow(tr("View tool bar:"), m_viewToolBarLineEdit);

    // scrollable menus

    m_scrollableMenusCheckBox = addCheckBox(m_interfaceLayout, tr("Scrollable menus:"),
                                            s_settings->mainWindow().scrollableMenus());

    m_scrollableMenusCheckBox->setToolTip(tr("Effective after restart."));

    // searchable menus

    m_searchableMenusCheckBox = addCheckBox(m_interfaceLayout, tr("Searchable menus:"),
                                            s_settings->mainWindow().searchableMenus());

    // toggle tool and menu bars with fullscreen

    m_toggleToolAndMenuBarsWithFullscreenCheckBox = addCheckBox(m_interfaceLayout, tr("Toggle tool and menu bars with fullscreen:"),
                                                                s_settings->mainWindow().toggleToolAndMenuBarsWithFullscreen());

    // use page label

    m_usePageLabelCheckBox = addCheckBox(m_interfaceLayout, tr("Use page label:"),
                                         s_settings->mainWindow().usePageLabel());

    // document title as tab title

    m_documentTitleAsTabTitleCheckBox = addCheckBox(m_interfaceLayout, tr("Document title as tab title:"),
                                                    s_settings->mainWindow().documentTitleAsTabTitle());

    // current page in window title

    m_currentPageInWindowTitleCheckBox = addCheckBox(m_interfaceLayout, tr("Current page in window title:"),
                                                     s_settings->mainWindow().currentPageInWindowTitle());

    // instance name in window title

    m_instanceNameInWindowTitleCheckBox = addCheckBox(m_interfaceLayout, tr("Instance name in window title:"),
                                                      s_settings->mainWindow().instanceNameInWindowTitle());

    // highlight current thumbnail

    m_highlightCurrentThumbnailCheckBox = addCheckBox(m_interfaceLayout, tr("Highlight current thumbnail:"),
                                                      s_settings->documentView().highlightCurrentThumbnail());

    // limit thumbnails to results

    m_limitThumbnailsToResultsCheckBox = addCheckBox(m_interfaceLayout, tr("Limit thumbnails to results:"),
                                                     s_settings->documentView().limitThumbnailsToResults());
}

void SettingsDialog::acceptInterfaceTab()
{
    s_settings->mainWindow().setExtendedSearchDock(m_extendedSearchDock->isChecked());

    s_settings->pageItem().setAnnotationOverlay(m_annotationOverlayCheckBox->isChecked());
    s_settings->pageItem().setFormFieldOverlay(m_formFieldOverlayCheckBox);

    s_settings->mainWindow().setTabPosition(m_tabPositionComboBox->itemData(m_tabPositionComboBox->currentIndex()).toInt());
    s_settings->mainWindow().setTabVisibility(m_tabVisibilityComboBox->itemData(m_tabVisibilityComboBox->currentIndex()).toInt());
    s_settings->mainWindow().setSpreadTabs(m_spreadTabsCheckBox->isChecked());

    s_settings->mainWindow().setNewTabNextToCurrentTab(m_newTabNextToCurrentTabCheckBox->isChecked());
    s_settings->mainWindow().setExitAfterLastTab(m_exitAfterLastTabCheckBox->isChecked());

    s_settings->mainWindow().setRecentlyUsedCount(m_recentlyUsedCountSpinBox->value());
    s_settings->mainWindow().setRecentlyClosedCount(m_recentlyClosedCountSpinBox->value());

    s_settings->mainWindow().setFileToolBar(m_fileToolBarLineEdit->text().split(",", QString::SkipEmptyParts));
    s_settings->mainWindow().setEditToolBar(m_editToolBarLineEdit->text().split(",", QString::SkipEmptyParts));
    s_settings->mainWindow().setViewToolBar(m_viewToolBarLineEdit->text().split(",", QString::SkipEmptyParts));

    s_settings->mainWindow().setScrollableMenus(m_scrollableMenusCheckBox->isChecked());
    s_settings->mainWindow().setSearchableMenus(m_searchableMenusCheckBox->isChecked());

    s_settings->mainWindow().setToggleToolAndMenuBarsWithFullscreen(m_toggleToolAndMenuBarsWithFullscreenCheckBox->isChecked());

    s_settings->mainWindow().setUsePageLabel(m_usePageLabelCheckBox->isChecked());
    s_settings->mainWindow().setDocumentTitleAsTabTitle(m_documentTitleAsTabTitleCheckBox->isChecked());

    s_settings->mainWindow().setCurrentPageInWindowTitle(m_currentPageInWindowTitleCheckBox->isChecked());
    s_settings->mainWindow().setInstanceNameInWindowTitle(m_instanceNameInWindowTitleCheckBox->isChecked());

    s_settings->documentView().setHighlightCurrentThumbnail(m_highlightCurrentThumbnailCheckBox->isChecked());
    s_settings->documentView().setLimitThumbnailsToResults(m_limitThumbnailsToResultsCheckBox->isChecked());
}

void SettingsDialog::resetInterfaceTab()
{
    m_extendedSearchDock->setChecked(Defaults::MainWindow::extendedSearchDock());

    m_annotationOverlayCheckBox->setChecked(Defaults::PageItem::annotationOverlay());
    m_formFieldOverlayCheckBox->setChecked(Defaults::PageItem::formFieldOverlay());

    m_tabPositionComboBox->setCurrentIndex(m_tabPositionComboBox->findData(static_cast< uint >(Defaults::MainWindow::tabPosition())));
    m_tabVisibilityComboBox->setCurrentIndex(m_tabVisibilityComboBox->findData(static_cast< uint >(Defaults::MainWindow::tabVisibility())));
    m_spreadTabsCheckBox->setChecked(Defaults::MainWindow::spreadTabs());

    m_newTabNextToCurrentTabCheckBox->setChecked(Defaults::MainWindow::newTabNextToCurrentTab());
    m_exitAfterLastTabCheckBox->setChecked(Defaults::MainWindow::exitAfterLastTab());

    m_recentlyUsedCountSpinBox->setValue(Defaults::MainWindow::recentlyUsedCount());
    m_recentlyClosedCountSpinBox->setValue(Defaults::MainWindow::recentlyClosedCount());

    m_fileToolBarLineEdit->setText(Defaults::MainWindow::fileToolBar().join(","));
    m_editToolBarLineEdit->setText(Defaults::MainWindow::editToolBar().join(","));
    m_viewToolBarLineEdit->setText(Defaults::MainWindow::viewToolBar().join(","));

    m_scrollableMenusCheckBox->setChecked(Defaults::MainWindow::scrollableMenus());
    m_searchableMenusCheckBox->setChecked(Defaults::MainWindow::searchableMenus());

    m_toggleToolAndMenuBarsWithFullscreenCheckBox->setChecked(Defaults::MainWindow::toggleToolAndMenuBarsWithFullscreen());

    m_usePageLabelCheckBox->setChecked(Defaults::MainWindow::usePageLabel());
    m_documentTitleAsTabTitleCheckBox->setChecked(Defaults::MainWindow::documentTitleAsTabTitle());

    m_currentPageInWindowTitleCheckBox->setChecked(Defaults::MainWindow::currentPageInWindowTitle());
    m_instanceNameInWindowTitleCheckBox->setChecked(Defaults::MainWindow::instanceNameInWindowTitle());

    m_highlightCurrentThumbnailCheckBox->setChecked(Defaults::DocumentView::highlightCurrentThumbnail());
    m_limitThumbnailsToResultsCheckBox->setChecked(Defaults::DocumentView::limitThumbnailsToResults());
}

void SettingsDialog::createModifiersTab()
{
    // zoom modifiers

    m_zoomModifiersComboBox = addModifiersComboBox(m_modifiersLayout, tr("Zoom:"),
                                                   s_settings->documentView().zoomModifiers());

    // rototate modifiers

    m_rotateModifiersComboBox = addModifiersComboBox(m_modifiersLayout, tr("Rotate:"),
                                                     s_settings->documentView().rotateModifiers());

    // scroll modifiers

    m_scrollModifiersComboBox = addModifiersComboBox(m_modifiersLayout, tr("Scroll:"),
                                                     s_settings->documentView().scrollModifiers());

    // copy to clipboard modifiers

    m_copyToClipboardModifiersComboBox = addModifiersComboBox(m_modifiersLayout, tr("Copy to clipboard:"),
                                                              s_settings->pageItem().copyToClipboardModifiers());

    // add annotation modifiers

    m_addAnnotationModifiersComboBox = addModifiersComboBox(m_modifiersLayout, tr("Add annotation:"),
                                                            s_settings->pageItem().addAnnotationModifiers());

    // zoom to selection modifiers

    m_zoomToSelectionModifiersComboBox = addModifiersComboBox(m_modifiersLayout, tr("Zoom to selection:"),
                                                              s_settings->pageItem().zoomToSelectionModifiers());
}

void SettingsDialog::acceptModifiersTab()
{
    s_settings->documentView().setZoomModifiers(getKeyboardModifierFromItemData(m_zoomModifiersComboBox));
    s_settings->documentView().setRotateModifiers(getKeyboardModifierFromItemData(m_rotateModifiersComboBox));
    s_settings->documentView().setScrollModifiers(getKeyboardModifierFromItemData(m_scrollModifiersComboBox));

    s_settings->pageItem().setCopyToClipboardModifiers(getKeyboardModifierFromItemData(m_copyToClipboardModifiersComboBox));
    s_settings->pageItem().setAddAnnotationModifiers(getKeyboardModifierFromItemData(m_addAnnotationModifiersComboBox));
    s_settings->pageItem().setZoomToSelectionModifiers(getKeyboardModifierFromItemData(m_zoomToSelectionModifiersComboBox));
}

void SettingsDialog::resetModifiersTab()
{
    setCurrentIndexFromKeyboardModifiers(m_zoomModifiersComboBox, Defaults::DocumentView::zoomModifiers());
    setCurrentIndexFromKeyboardModifiers(m_rotateModifiersComboBox, Defaults::DocumentView::rotateModifiers());
    setCurrentIndexFromKeyboardModifiers(m_scrollModifiersComboBox, Defaults::DocumentView::scrollModifiers());

    setCurrentIndexFromKeyboardModifiers(m_copyToClipboardModifiersComboBox, Defaults::PageItem::copyToClipboardModifiers());
    setCurrentIndexFromKeyboardModifiers(m_addAnnotationModifiersComboBox, Defaults::PageItem::addAnnotationModifiers());
    setCurrentIndexFromKeyboardModifiers(m_zoomToSelectionModifiersComboBox, Defaults::PageItem::zoomToSelectionModifiers());
}

QCheckBox* SettingsDialog::addCheckBox(QFormLayout* layout, const QString& label, bool checked)
{
    QCheckBox* checkBox = new QCheckBox(this);
    checkBox->setChecked(checked);

    layout->addRow(label, checkBox);

    return checkBox;
}

QSpinBox* SettingsDialog::addSpinBox(QFormLayout* layout, const QString& label, const QString& suffix, const QString& special, int min, int max, int step, int val)
{
    QSpinBox* spinBox = new QSpinBox(this);
    spinBox->setRange(min, max);
    spinBox->setSingleStep(step);
    spinBox->setValue(val);

    spinBox->setSuffix(suffix);
    spinBox->setSpecialValueText(special);

    layout->addRow(label, spinBox);

    return spinBox;
}

QDoubleSpinBox* SettingsDialog::addDoubleSpinBox(QFormLayout* layout, const QString& label, const QString& suffix, double min, double max, double step, double val)
{
    QDoubleSpinBox* spinBox = new QDoubleSpinBox(this);
    spinBox->setRange(min, max);
    spinBox->setSingleStep(step);
    spinBox->setValue(val);

    spinBox->setSuffix(suffix);

    layout->addRow(label, spinBox);

    return spinBox;
}

QComboBox* SettingsDialog::addColorComboBox(QFormLayout* layout, const QString& label, const QColor& color)
{
    QComboBox* comboBox = new QComboBox(this);
    comboBox->setEditable(true);
    comboBox->setInsertPolicy(QComboBox::NoInsert);
    comboBox->addItems(QColor::colorNames());

    setCurrentTextToColorName(comboBox, color);

    layout->addRow(label, comboBox);

    return comboBox;
}

QComboBox* SettingsDialog::addModifiersComboBox(QFormLayout* layout, const QString& label, const Qt::KeyboardModifiers& modifiers)
{
    QComboBox* comboBox = new QComboBox(this);
    comboBox->addItem(QShortcut::tr("Shift"), static_cast< int >(Qt::ShiftModifier));
    comboBox->addItem(QShortcut::tr("Ctrl"), static_cast< int >(Qt::ControlModifier));
    comboBox->addItem(QShortcut::tr("Alt"), static_cast< int >(Qt::AltModifier));
    comboBox->addItem(QShortcut::tr("Shift and Ctrl"), static_cast< int >(Qt::ShiftModifier | Qt::ControlModifier));
    comboBox->addItem(QShortcut::tr("Shift and Alt"), static_cast< int >(Qt::ShiftModifier | Qt::AltModifier));
    comboBox->addItem(QShortcut::tr("Ctrl and Alt"), static_cast< int >(Qt::ControlModifier | Qt::AltModifier));
    comboBox->addItem(QShortcut::tr("Right mouse button"), static_cast< int >(Qt::RightButton));
    comboBox->addItem(QShortcut::tr("Middle mouse button"), static_cast< int >(Qt::MidButton));

    setCurrentIndexFromKeyboardModifiers(comboBox, modifiers);

    layout->addRow(label, comboBox);

    return comboBox;
}

} // qpdfview
