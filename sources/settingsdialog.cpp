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
#include <QLineEdit>
#include <QPushButton>
#include <QShortcut>
#include <QSpinBox>
#include <QTableView>

#include "settings.h"
#include "model.h"
#include "pluginhandler.h"
#include "shortcuthandler.h"
#include "documentview.h"
#include "miscellaneous.h"

Settings* SettingsDialog::s_settings = 0;

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent)
{
    if(s_settings == 0)
    {
        s_settings = Settings::instance();
    }

    m_graphicsTabWidget = new QTabWidget(this);
    m_graphicsTabWidget->addTab(new QWidget(this), tr("General"));

#ifdef WITH_PDF

    m_pdfSettingsWidget = PluginHandler::createPdfSettingsWidget(this);

    if(m_pdfSettingsWidget != 0)
    {
        m_graphicsTabWidget->addTab(m_pdfSettingsWidget, "PDF");
    }

#endif // WITH_PDF

#ifdef WITH_PS

    m_psSettingsWidget = PluginHandler::createPsSettingsWidget(this);

    if(m_psSettingsWidget != 0)
    {
        m_graphicsTabWidget->addTab(m_psSettingsWidget, "PS");
    }

#endif // WITH_PS

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

    m_defaultsOnTabButton = m_dialogButtonBox->addButton(tr("Defaults on tab"), QDialogButtonBox::ResetRole);
    connect(m_defaultsOnTabButton, SIGNAL(clicked()), SLOT(resetCurrentTab()));

    m_behaviorLayout = new QFormLayout(m_tabWidget->widget(0));
    m_interfaceLayout = new QFormLayout(m_tabWidget->widget(2));
    m_modifiersLayout = new QFormLayout(m_tabWidget->widget(4));

    setLayout(new QVBoxLayout(this));
    layout()->addWidget(m_tabWidget);
    layout()->addWidget(m_dialogButtonBox);

    createBehaviorTab();
    createGraphicsTab();
    createInterfaceTab();
    createModifiersTab();
}

void SettingsDialog::accept()
{
    QDialog::accept();

#ifdef WITH_PDF

    if(m_pdfSettingsWidget != 0)
    {
        m_pdfSettingsWidget->accept();
    }

#endif // WITH_PDF

#ifdef WITH_PS

    if(m_psSettingsWidget != 0)
    {
        m_psSettingsWidget->accept();
    }

#endif // WITH_PS

    // behavior

    s_settings->documentView().setOpenUrl(m_openUrlCheckBox->isChecked());

    s_settings->documentView().setAutoRefresh(m_autoRefreshCheckBox->isChecked());

    s_settings->mainWindow().setTrackRecentlyUsed(m_trackRecentlyUsedCheckBox->isChecked());

    s_settings->mainWindow().setRestoreTabs(m_restoreTabsCheckBox->isChecked());
    s_settings->mainWindow().setRestoreBookmarks(m_restoreBookmarksCheckBox->isChecked());
    s_settings->mainWindow().setRestorePerFileSettings(m_restorePerFileSettingsCheckBox->isChecked());

    s_settings->presentationView().setSync(m_presentationSyncCheckBox->isChecked());
    s_settings->presentationView().setScreen(m_presentationScreenSpinBox->value());

    const QColor highlightColor(m_highlightColorComboBox->currentText());
    s_settings->pageItem().setHighlightColor(highlightColor.isValid() ? highlightColor : Defaults::PageItem::highlightColor());

    s_settings->documentView().setHighlightDuration(m_highlightDurationSpinBox->value());

    const QColor annotationColor(m_annotationColorComboBox->currentText());
    s_settings->pageItem().setAnnotationColor(annotationColor.isValid() ? annotationColor : Defaults::PageItem::annotationColor());

    s_settings->documentView().setSourceEditor(m_sourceEditorLineEdit->text());

    // graphics

    s_settings->pageItem().setDecoratePages(m_decoratePagesCheckBox->isChecked());
    s_settings->pageItem().setDecorateLinks(m_decorateLinksCheckBox->isChecked());
    s_settings->pageItem().setDecorateFormFields(m_decorateFormFieldsCheckBox->isChecked());

    const QColor backgroundColor(m_backgroundColorComboBox->currentText());
    s_settings->pageItem().setBackgroundColor(backgroundColor.isValid() ? backgroundColor : Defaults::PageItem::backgroundColor());

    const QColor paperColor(m_paperColorComboBox->currentText());
    s_settings->pageItem().setPaperColor(paperColor.isValid() ? paperColor : Defaults::PageItem::paperColor());

    s_settings->documentView().setPagesPerRow(m_pagesPerRowSpinBox->value());

    s_settings->documentView().setPageSpacing(m_pageSpacingSpinBox->value());
    s_settings->documentView().setThumbnailSpacing(m_thumbnailSpacingSpinBox->value());

    s_settings->documentView().setThumbnailSize(m_thumbnailSizeSpinBox->value());

    s_settings->pageItem().setCacheSize(m_cacheSizeComboBox->itemData(m_cacheSizeComboBox->currentIndex()).toInt());
    s_settings->documentView().setPrefetch(m_prefetchCheckBox->isChecked());
    s_settings->documentView().setPrefetchDistance(m_prefetchDistanceSpinBox->value());

    // interface

    s_settings->mainWindow().setTabPosition(m_tabPositionComboBox->itemData(m_tabPositionComboBox->currentIndex()).toInt());
    s_settings->mainWindow().setTabVisibility(m_tabVisibilityComboBox->itemData(m_tabVisibilityComboBox->currentIndex()).toInt());

    s_settings->mainWindow().setNewTabNextToCurrentTab(m_newTabNextToCurrentTabCheckBox->isChecked());

    s_settings->mainWindow().setRecentlyUsedCount(m_recentlyUsedCountSpinBox->value());

    s_settings->mainWindow().setFileToolBar(m_fileToolBarLineEdit->text().split(",", QString::SkipEmptyParts));
    s_settings->mainWindow().setEditToolBar(m_editToolBarLineEdit->text().split(",", QString::SkipEmptyParts));
    s_settings->mainWindow().setViewToolBar(m_viewToolBarLineEdit->text().split(",", QString::SkipEmptyParts));

    s_settings->mainWindow().setCurrentPageInWindowTitle(m_currentPageInWindowTitleCheckBox->isChecked());

    s_settings->documentView().setHighlightCurrentThumbnail(m_highlightCurrentThumbnailCheckBox->isChecked());
    s_settings->documentView().setLimitThumbnailsToResults(m_limitThumbnailsToResultsCheckBox->isChecked());

    // modifiers

    s_settings->documentView().setZoomModifiers(static_cast<Qt::KeyboardModifier>(m_zoomModifiersComboBox->itemData(m_zoomModifiersComboBox->currentIndex()).toInt()));
    s_settings->documentView().setRotateModifiers(static_cast<Qt::KeyboardModifier>(m_rotateModifiersComboBox->itemData(m_rotateModifiersComboBox->currentIndex()).toInt()));
    s_settings->documentView().setScrollModifiers(static_cast<Qt::KeyboardModifier>(m_scrollModifiersComboBox->itemData(m_scrollModifiersComboBox->currentIndex()).toInt()));

    s_settings->pageItem().setCopyToClipboardModifiers(static_cast<Qt::KeyboardModifier>(m_copyToClipboardModifiersComboBox->itemData(m_copyToClipboardModifiersComboBox->currentIndex()).toInt()));
    s_settings->pageItem().setAddAnnotationModifiers(static_cast<Qt::KeyboardModifier>(m_addAnnotationModifiersComboBox->itemData(m_addAnnotationModifiersComboBox->currentIndex()).toInt()));
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

    m_openUrlCheckBox = new QCheckBox(this);
    m_openUrlCheckBox->setChecked(s_settings->documentView().openUrl());

    m_behaviorLayout->addRow(tr("Open URL:"), m_openUrlCheckBox);

    // auto-refresh

    m_autoRefreshCheckBox = new QCheckBox(this);
    m_autoRefreshCheckBox->setChecked(s_settings->documentView().autoRefresh());

    m_behaviorLayout->addRow(tr("Auto-refresh:"), m_autoRefreshCheckBox);

    // track recently used

    m_trackRecentlyUsedCheckBox = new QCheckBox(this);
    m_trackRecentlyUsedCheckBox->setChecked(s_settings->mainWindow().trackRecentlyUsed());
    m_trackRecentlyUsedCheckBox->setToolTip(tr("Effective after restart."));

    m_behaviorLayout->addRow(tr("Track recently used:"), m_trackRecentlyUsedCheckBox);

    // restore tabs

    m_restoreTabsCheckBox = new QCheckBox(this);
    m_restoreTabsCheckBox->setChecked(s_settings->mainWindow().restoreTabs());

    m_behaviorLayout->addRow(tr("Restore tabs:"), m_restoreTabsCheckBox);

    // restore bookmarks

    m_restoreBookmarksCheckBox = new QCheckBox(this);
    m_restoreBookmarksCheckBox->setChecked(s_settings->mainWindow().restoreBookmarks());

    m_behaviorLayout->addRow(tr("Restore bookmarks:"), m_restoreBookmarksCheckBox);

    // restore per-file settings

    m_restorePerFileSettingsCheckBox = new QCheckBox(this);
    m_restorePerFileSettingsCheckBox->setChecked(s_settings->mainWindow().restorePerFileSettings());

    m_behaviorLayout->addRow(tr("Restore per-file settings:"), m_restorePerFileSettingsCheckBox);

#ifndef WITH_SQL

    m_restoreTabsCheckBox->setEnabled(false);
    m_restoreBookmarksCheckBox->setEnabled(false);
    m_restorePerFileSettingsCheckBox->setEnabled(false);

#endif // WITH_SQL

    // presentation sync

    m_presentationSyncCheckBox = new QCheckBox(this);
    m_presentationSyncCheckBox->setChecked(s_settings->presentationView().sync());

    m_behaviorLayout->addRow(tr("Synchronize presentation:"), m_presentationSyncCheckBox);

    // presentation screen

    m_presentationScreenSpinBox = new QSpinBox(this);
    m_presentationScreenSpinBox->setRange(-1, QApplication::desktop()->screenCount() - 1);
    m_presentationScreenSpinBox->setSpecialValueText(tr("Default"));
    m_presentationScreenSpinBox->setValue(s_settings->presentationView().screen());

    m_behaviorLayout->addRow(tr("Presentation screen:"), m_presentationScreenSpinBox);

    // highlight color

    m_highlightColorComboBox = new QComboBox(this);
    m_highlightColorComboBox->setEditable(true);
    m_highlightColorComboBox->setInsertPolicy(QComboBox::NoInsert);
    m_highlightColorComboBox->addItems(QColor::colorNames());
    m_highlightColorComboBox->lineEdit()->setText(s_settings->pageItem().highlightColor().name());

    m_behaviorLayout->addRow(tr("Highlight color:"), m_highlightColorComboBox);

    // highlight duration

    m_highlightDurationSpinBox = new QSpinBox(this);
    m_highlightDurationSpinBox->setSuffix(" ms");
    m_highlightDurationSpinBox->setRange(0, 60000);
    m_highlightDurationSpinBox->setSingleStep(500);
    m_highlightDurationSpinBox->setSpecialValueText(tr("None"));
    m_highlightDurationSpinBox->setValue(s_settings->documentView().highlightDuration());

    m_behaviorLayout->addRow(tr("Highlight duration:"), m_highlightDurationSpinBox);

    // annotation color

    m_annotationColorComboBox = new QComboBox(this);
    m_annotationColorComboBox->setEditable(true);
    m_annotationColorComboBox->setInsertPolicy(QComboBox::NoInsert);
    m_annotationColorComboBox->addItems(QColor::colorNames());
    m_annotationColorComboBox->lineEdit()->setText(s_settings->pageItem().annotationColor().name());

    m_behaviorLayout->addRow(tr("Annotation color:"), m_annotationColorComboBox);

    // source editor

    m_sourceEditorLineEdit = new QLineEdit(this);
    m_sourceEditorLineEdit->setText(s_settings->documentView().sourceEditor());
    m_sourceEditorLineEdit->setToolTip(tr("'%1' is replaced by the absolute file path. '%2' resp. '%3' is replaced by line resp. column number."));

    m_behaviorLayout->addRow(tr("Source editor:"), m_sourceEditorLineEdit);
}

void SettingsDialog::resetBehaviorTab()
{
    m_openUrlCheckBox->setChecked(Defaults::DocumentView::openUrl());

    m_autoRefreshCheckBox->setChecked(Defaults::DocumentView::autoRefresh());

    m_trackRecentlyUsedCheckBox->setChecked(Defaults::MainWindow::trackRecentlyUsed());

    m_restoreTabsCheckBox->setChecked(Defaults::MainWindow::restoreTabs());
    m_restoreBookmarksCheckBox->setChecked(Defaults::MainWindow::restoreBookmarks());
    m_restorePerFileSettingsCheckBox->setChecked(Defaults::MainWindow::restorePerFileSettings());

    m_presentationSyncCheckBox->setChecked(Defaults::PresentationView::sync());
    m_presentationScreenSpinBox->setValue(Defaults::PresentationView::screen());

    m_highlightColorComboBox->lineEdit()->setText(Defaults::PageItem::highlightColor().name());
    m_highlightDurationSpinBox->setValue(Defaults::DocumentView::highlightDuration());
    m_annotationColorComboBox->lineEdit()->setText(Defaults::PageItem::annotationColor().name());

    m_sourceEditorLineEdit->clear();
}

void SettingsDialog::createGraphicsTab()
{
    // decorate pages

    m_decoratePagesCheckBox = new QCheckBox(this);
    m_decoratePagesCheckBox->setChecked(s_settings->pageItem().decoratePages());

    m_graphicsLayout->addRow(tr("Decorate pages:"), m_decoratePagesCheckBox);

    // decorate links

    m_decorateLinksCheckBox = new QCheckBox(this);
    m_decorateLinksCheckBox->setChecked(s_settings->pageItem().decorateLinks());

    m_graphicsLayout->addRow(tr("Decorate links:"), m_decorateLinksCheckBox);

    // decorate form fields

    m_decorateFormFieldsCheckBox = new QCheckBox(this);
    m_decorateFormFieldsCheckBox->setChecked(s_settings->pageItem().decorateFormFields());

    m_graphicsLayout->addRow(tr("Decorate form fields:"), m_decorateFormFieldsCheckBox);

    // background color

    m_backgroundColorComboBox = new QComboBox(this);
    m_backgroundColorComboBox->setEditable(true);
    m_backgroundColorComboBox->setInsertPolicy(QComboBox::NoInsert);
    m_backgroundColorComboBox->addItems(QColor::colorNames());
    m_backgroundColorComboBox->lineEdit()->setText(s_settings->pageItem().backgroundColor().name());

    m_graphicsLayout->addRow(tr("Background color:"), m_backgroundColorComboBox);

    // paper color

    m_paperColorComboBox = new QComboBox(this);
    m_paperColorComboBox->setEditable(true);
    m_paperColorComboBox->setInsertPolicy(QComboBox::NoInsert);
    m_paperColorComboBox->addItems(QColor::colorNames());
    m_paperColorComboBox->lineEdit()->setText(s_settings->pageItem().paperColor().name());

    m_graphicsLayout->addRow(tr("Paper color:"), m_paperColorComboBox);

    // pages per row

    m_pagesPerRowSpinBox = new QSpinBox(this);
    m_pagesPerRowSpinBox->setRange(1, 10);
    m_pagesPerRowSpinBox->setValue(s_settings->documentView().pagesPerRow());

    m_graphicsLayout->addRow(tr("Pages per row:"), m_pagesPerRowSpinBox);

    // page spacing

    m_pageSpacingSpinBox = new QDoubleSpinBox(this);
    m_pageSpacingSpinBox->setSuffix(" px");
    m_pageSpacingSpinBox->setRange(0.0, 25.0);
    m_pageSpacingSpinBox->setSingleStep(0.25);
    m_pageSpacingSpinBox->setValue(s_settings->documentView().pageSpacing());

    m_graphicsLayout->addRow(tr("Page spacing:"), m_pageSpacingSpinBox);

    // thumbnail spacing

    m_thumbnailSpacingSpinBox = new QDoubleSpinBox(this);
    m_thumbnailSpacingSpinBox->setSuffix(" px");
    m_thumbnailSpacingSpinBox->setRange(0.0, 25.0);
    m_thumbnailSpacingSpinBox->setSingleStep(0.25);
    m_thumbnailSpacingSpinBox->setValue(s_settings->documentView().thumbnailSpacing());

    m_graphicsLayout->addRow(tr("Thumbnail spacing:"), m_thumbnailSpacingSpinBox);

    // thumbnail size

    m_thumbnailSizeSpinBox = new QDoubleSpinBox(this);
    m_thumbnailSizeSpinBox->setSuffix(" px");
    m_thumbnailSizeSpinBox->setRange(30.0, 300.0);
    m_thumbnailSizeSpinBox->setSingleStep(10.0);
    m_thumbnailSizeSpinBox->setValue(s_settings->documentView().thumbnailSize());

    m_graphicsLayout->addRow(tr("Thumbnail size:"), m_thumbnailSizeSpinBox);

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
    m_cacheSizeComboBox->setCurrentIndex(m_cacheSizeComboBox->findData(s_settings->pageItem().cacheSize()));

    m_graphicsLayout->addRow(tr("Cache size:"), m_cacheSizeComboBox);

    // prefetch

    m_prefetchCheckBox = new QCheckBox(this);
    m_prefetchCheckBox->setChecked(s_settings->documentView().prefetch());

    m_graphicsLayout->addRow(tr("Prefetch:"), m_prefetchCheckBox);

    // prefetch distance

    m_prefetchDistanceSpinBox = new QSpinBox(this);
    m_prefetchDistanceSpinBox->setRange(1, 10);
    m_prefetchDistanceSpinBox->setValue(s_settings->documentView().prefetchDistance());

    m_graphicsLayout->addRow(tr("Prefetch distance:"), m_prefetchDistanceSpinBox);
}

void SettingsDialog::resetGraphicsTab()
{
#ifdef WITH_PDF

    if(m_pdfSettingsWidget != 0)
    {
        m_pdfSettingsWidget->reset();
    }

#endif // WITH_PDF

#ifdef WITH_PS

    if(m_psSettingsWidget != 0)
    {
        m_psSettingsWidget->reset();
    }

#endif // WITH_PS

    m_decoratePagesCheckBox->setChecked(Defaults::PageItem::decoratePages());
    m_decorateLinksCheckBox->setChecked(Defaults::PageItem::decorateLinks());
    m_decorateFormFieldsCheckBox->setChecked(Defaults::PageItem::decorateFormFields());

    m_backgroundColorComboBox->lineEdit()->setText(Defaults::PageItem::backgroundColor().name());
    m_paperColorComboBox->lineEdit()->setText(Defaults::PageItem::paperColor().name());

    m_pagesPerRowSpinBox->setValue(Defaults::DocumentView::pagesPerRow());

    m_pageSpacingSpinBox->setValue(Defaults::DocumentView::pageSpacing());
    m_thumbnailSpacingSpinBox->setValue(Defaults::DocumentView::thumbnailSpacing());

    m_thumbnailSizeSpinBox->setValue(Defaults::DocumentView::thumbnailSize());

    m_cacheSizeComboBox->setCurrentIndex(m_cacheSizeComboBox->findData(Defaults::PageItem::cacheSize()));
    m_prefetchCheckBox->setChecked(Defaults::DocumentView::prefetch());
    m_prefetchDistanceSpinBox->setValue(Defaults::DocumentView::prefetchDistance());
}

void SettingsDialog::createInterfaceTab()
{
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

    // new tab next to current tab

    m_newTabNextToCurrentTabCheckBox = new QCheckBox(this);
    m_newTabNextToCurrentTabCheckBox->setChecked(s_settings->mainWindow().newTabNextToCurrentTab());

    m_interfaceLayout->addRow(tr("New tab next to current tab:"), m_newTabNextToCurrentTabCheckBox);

    // recently used count

    m_recentlyUsedCountSpinBox = new QSpinBox(this);
    m_recentlyUsedCountSpinBox->setRange(1, 50);
    m_recentlyUsedCountSpinBox->setValue(s_settings->mainWindow().recentlyUsedCount());
    m_recentlyUsedCountSpinBox->setToolTip(tr("Effective after restart."));

    m_interfaceLayout->addRow(tr("Recently used count:"), m_recentlyUsedCountSpinBox);

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

    // current page in window title

    m_currentPageInWindowTitleCheckBox = new QCheckBox(this);
    m_currentPageInWindowTitleCheckBox->setChecked(s_settings->mainWindow().currentPageInWindowTitle());

    m_interfaceLayout->addRow(tr("Current page in window title:"), m_currentPageInWindowTitleCheckBox);

    // highlight current thumbnail

    m_highlightCurrentThumbnailCheckBox = new QCheckBox(this);
    m_highlightCurrentThumbnailCheckBox->setChecked(s_settings->documentView().highlightCurrentThumbnail());

    m_interfaceLayout->addRow(tr("Highlight current thumbnail:"), m_highlightCurrentThumbnailCheckBox);

    // limit thumbnails to results

    m_limitThumbnailsToResultsCheckBox = new QCheckBox(this);
    m_limitThumbnailsToResultsCheckBox->setChecked(s_settings->documentView().limitThumbnailsToResults());

    m_interfaceLayout->addRow(tr("Limit thumbnails to results:"), m_limitThumbnailsToResultsCheckBox);
}

void SettingsDialog::resetInterfaceTab()
{
    m_tabPositionComboBox->setCurrentIndex(m_tabPositionComboBox->findData(static_cast< uint >(Defaults::MainWindow::tabPosition())));
    m_tabVisibilityComboBox->setCurrentIndex(m_tabVisibilityComboBox->findData(static_cast< uint >(Defaults::MainWindow::tabVisibility())));

    m_newTabNextToCurrentTabCheckBox->setChecked(Defaults::MainWindow::newTabNextToCurrentTab());

    m_recentlyUsedCountSpinBox->setValue(Defaults::MainWindow::recentlyUsedCount());

    m_fileToolBarLineEdit->setText(Defaults::MainWindow::fileToolBar().join(","));
    m_editToolBarLineEdit->setText(Defaults::MainWindow::editToolBar().join(","));
    m_viewToolBarLineEdit->setText(Defaults::MainWindow::viewToolBar().join(","));

    m_currentPageInWindowTitleCheckBox->setChecked(Defaults::MainWindow::currentPageInWindowTitle());

    m_highlightCurrentThumbnailCheckBox->setChecked(Defaults::DocumentView::highlightCurrentThumbnail());
    m_limitThumbnailsToResultsCheckBox->setChecked(Defaults::DocumentView::limitThumbnailsToResults());
}

void SettingsDialog::createModifiersTab()
{
    // zoom modifiers

    createModifiersComboBox(m_zoomModifiersComboBox, s_settings->documentView().zoomModifiers());

    m_modifiersLayout->addRow(tr("Zoom:"), m_zoomModifiersComboBox);

    // rototate modifiers

    createModifiersComboBox(m_rotateModifiersComboBox, s_settings->documentView().rotateModifiers());

    m_modifiersLayout->addRow(tr("Rotate:"), m_rotateModifiersComboBox);

    // scroll modifiers

    createModifiersComboBox(m_scrollModifiersComboBox, s_settings->documentView().scrollModifiers());

    m_modifiersLayout->addRow(tr("Scroll:"), m_scrollModifiersComboBox);

    // copy to clipboard modifiers

    createModifiersComboBox(m_copyToClipboardModifiersComboBox, s_settings->pageItem().copyToClipboardModifiers());

    m_modifiersLayout->addRow(tr("Copy to clipboard:"), m_copyToClipboardModifiersComboBox);

    // add annotation modifiers

    createModifiersComboBox(m_addAnnotationModifiersComboBox, s_settings->pageItem().addAnnotationModifiers());

    m_modifiersLayout->addRow(tr("Add annotation:"), m_addAnnotationModifiersComboBox);
}

void SettingsDialog::resetModifiersTab()
{
    m_zoomModifiersComboBox->setCurrentIndex(m_zoomModifiersComboBox->findData(static_cast< int >(Defaults::DocumentView::zoomModifiers())));
    m_rotateModifiersComboBox->setCurrentIndex(m_rotateModifiersComboBox->findData(static_cast< int >(Defaults::DocumentView::rotateModifiers())));
    m_scrollModifiersComboBox->setCurrentIndex(m_scrollModifiersComboBox->findData(static_cast< int >(Defaults::DocumentView::scrollModifiers())));

    m_copyToClipboardModifiersComboBox->setCurrentIndex(m_copyToClipboardModifiersComboBox->findData(static_cast< int >(Defaults::PageItem::copyToClipboardModifiers())));
    m_addAnnotationModifiersComboBox->setCurrentIndex(m_addAnnotationModifiersComboBox->findData(static_cast< int >(Defaults::PageItem::addAnnotationModifiers())));
}

void SettingsDialog::createModifiersComboBox(QComboBox*& comboBox, const Qt::KeyboardModifiers& modifiers)
{
    comboBox = new QComboBox(this);
    comboBox->addItem(QShortcut::tr("Shift"), static_cast< int >(Qt::ShiftModifier));
    comboBox->addItem(QShortcut::tr("Ctrl"), static_cast< int >(Qt::ControlModifier));
    comboBox->addItem(QShortcut::tr("Alt"), static_cast< int >(Qt::AltModifier));
    comboBox->addItem(QShortcut::tr("Shift and Ctrl"), static_cast< int >(Qt::ShiftModifier | Qt::ControlModifier));
    comboBox->addItem(QShortcut::tr("Shift and Alt"), static_cast< int >(Qt::ShiftModifier | Qt::AltModifier));
    comboBox->addItem(QShortcut::tr("Ctrl and Alt"), static_cast< int >(Qt::ControlModifier | Qt::AltModifier));
    comboBox->setCurrentIndex(comboBox->findData(static_cast< int >(modifiers)));
}
