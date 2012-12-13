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

#include "settingsdialog.h"

#include <poppler-qt4.h>

#include "miscellaneous.h"
#include "settings.h"

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent)
{
    m_settings = new Settings(this);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(new QWidget(this), tr("&Behavior"));
    m_tabWidget->addTab(new QWidget(this), tr("&Graphics"));
    m_tabWidget->addTab(new QWidget(this), tr("&Interface"));
    m_tabWidget->addTab(new QWidget(this), tr("&Modifiers"));

    m_dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    connect(m_dialogButtonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(m_dialogButtonBox, SIGNAL(rejected()), SLOT(reject()));

    m_defaultsButton = m_dialogButtonBox->addButton(tr("Defaults"), QDialogButtonBox::ResetRole);
    connect(m_defaultsButton, SIGNAL(clicked()), SLOT(on_defaults_clicked()));

    m_behaviorLayout = new QFormLayout(m_tabWidget->widget(0));
    m_graphicsLayout = new QFormLayout(m_tabWidget->widget(1));
    m_interfaceLayout = new QFormLayout(m_tabWidget->widget(2));
    m_modifiersLayout = new QFormLayout(m_tabWidget->widget(3));

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
    // behavior

    m_settings->documentView()->setOpenUrl(m_openUrlCheckBox->isChecked());

    m_settings->documentView()->setAutoRefresh(m_autoRefreshCheckBox->isChecked());

    m_settings->mainWindow()->setTrackRecentlyUsed(m_trackRecentlyUsedCheckBox->isChecked());

    m_settings->mainWindow()->setRestoreTabs(m_restoreTabsCheckBox->isChecked());
    m_settings->mainWindow()->setRestoreBookmarks(m_restoreBookmarksCheckBox->isChecked());
    m_settings->mainWindow()->setRestorePerFileSettings(m_restorePerFileSettingsCheckBox->isChecked());

    m_settings->presentationView()->setSync(m_presentationSyncCheckBox->isChecked());
    m_settings->presentationView()->setScreen(m_presentationScreenSpinBox->value());

    m_settings->documentView()->setSourceEditor(m_sourceEditorLineEdit->text());

    // graphics

    m_settings->pageItem()->setDecoratePages(m_decoratePagesCheckBox->isChecked());
    m_settings->pageItem()->setDecorateLinks(m_decorateLinksCheckBox->isChecked());
    m_settings->pageItem()->setDecorateFormFields(m_decorateFormFieldsCheckBox->isChecked());

    m_settings->documentView()->setHighlightDuration(m_highlightDurationSpinBox->value());

    m_settings->pageItem()->setBackgroundColor(m_backgroundColorLineEdit->text());
    m_settings->pageItem()->setPaperColor(m_paperColorLineEdit->text());
    m_settings->pageItem()->setInvertColors(m_invertColorsCheckBox->isChecked());

    m_settings->documentView()->setOverprintPreview(m_overprintPreviewCheckBox->isChecked());

    m_settings->documentView()->setPagesPerRow(m_pagesPerRowSpinBox->value());

    m_settings->documentView()->setPageSpacing(m_pageSpacingSpinBox->value());
    m_settings->documentView()->setThumbnailSpacing(m_thumbnailSpacingSpinBox->value());

    m_settings->documentView()->setThumbnailSize(m_thumbnailSizeSpinBox->value());

    m_settings->documentView()->setAntialiasing(m_antialiasingCheckBox->isChecked());
    m_settings->documentView()->setTextAntialiasing(m_textAntialiasingCheckBox->isChecked());
    m_settings->documentView()->setTextHinting(m_textHintingCheckBox->isChecked());

    m_settings->pageItem()->setCacheSize(m_cacheSizeComboBox->itemData(m_cacheSizeComboBox->currentIndex()).toInt());
    m_settings->documentView()->setPrefetch(m_prefetchCheckBox->isChecked());

    // interface

    m_settings->mainWindow()->setTabPosition(static_cast< QTabWidget::TabPosition >(m_tabPositionComboBox->itemData(m_tabPositionComboBox->currentIndex()).toUInt()));
    m_settings->mainWindow()->setTabVisibility(static_cast< TabWidget::TabBarPolicy >(m_tabVisibilityComboBox->itemData(m_tabVisibilityComboBox->currentIndex()).toUInt()));

    m_settings->mainWindow()->setFileToolBar(m_fileToolBarLineEdit->text().split(",", QString::SkipEmptyParts));
    m_settings->mainWindow()->setEditToolBar(m_editToolBarLineEdit->text().split(",", QString::SkipEmptyParts));
    m_settings->mainWindow()->setViewToolBar(m_viewToolBarLineEdit->text().split(",", QString::SkipEmptyParts));

    // modifiers

    m_settings->documentView()->setZoomModifiers(static_cast<Qt::KeyboardModifier>(m_zoomModifiersComboBox->itemData(m_zoomModifiersComboBox->currentIndex()).toInt()));
    m_settings->documentView()->setRotateModifiers(static_cast<Qt::KeyboardModifier>(m_rotateModifiersComboBox->itemData(m_rotateModifiersComboBox->currentIndex()).toInt()));
    m_settings->documentView()->setHorizontalModifiers(static_cast<Qt::KeyboardModifier>(m_horizontalModifiersComboBox->itemData(m_horizontalModifiersComboBox->currentIndex()).toInt()));

    m_settings->pageItem()->setCopyModifiers(static_cast<Qt::KeyboardModifier>(m_copyModifiersComboBox->itemData(m_copyModifiersComboBox->currentIndex()).toInt()));
    m_settings->pageItem()->setAnnotateModifiers(static_cast<Qt::KeyboardModifier>(m_annotateModifiersComboBox->itemData(m_annotateModifiersComboBox->currentIndex()).toInt()));

    QDialog::accept();
}

void SettingsDialog::on_defaults_clicked()
{
    // behavior

    m_openUrlCheckBox->setChecked(m_settings->documentView()->defaultOpenUrl());

    m_autoRefreshCheckBox->setChecked(Settings::DocumentView::defaultAutoRefresh());

    m_trackRecentlyUsedCheckBox->setChecked(Settings::MainWindow::defaultTrackRecentlyUsed());

    m_restoreTabsCheckBox->setChecked(Settings::MainWindow::defaultRestoreTabs());
    m_restoreBookmarksCheckBox->setChecked(Settings::MainWindow::defaultRestoreBookmarks());
    m_restorePerFileSettingsCheckBox->setChecked(Settings::MainWindow::defaultRestorePerFileSettings());

    m_presentationSyncCheckBox->setChecked(Settings::PresentationView::defaultSync());
    m_presentationScreenSpinBox->setValue(Settings::PresentationView::defaultScreen());

    m_sourceEditorLineEdit->clear();

    // graphics

    m_decoratePagesCheckBox->setChecked(Settings::PageItem::defaultDecoratePages());
    m_decorateLinksCheckBox->setChecked(Settings::PageItem::defaultDecorateLinks());
    m_decorateFormFieldsCheckBox->setChecked(Settings::PageItem::defaultDecorateFormFields());

    m_highlightDurationSpinBox->setValue(Settings::DocumentView::defaultHighlightDuration());

    m_backgroundColorLineEdit->setText(Settings::PageItem::defaultBackgroundColor());
    m_paperColorLineEdit->setText(Settings::PageItem::defaultPaperColor());
    m_invertColorsCheckBox->setChecked(Settings::PageItem::defaultInvertColors());

    m_overprintPreviewCheckBox->setChecked(Settings::DocumentView::defaultOverprintPreview());

    m_pagesPerRowSpinBox->setValue(Settings::DocumentView::defaultPagesPerRow());

    m_pageSpacingSpinBox->setValue(Settings::DocumentView::defaultPageSpacing());
    m_thumbnailSpacingSpinBox->setValue(Settings::DocumentView::defaultThumbnailSpacing());

    m_thumbnailSizeSpinBox->setValue(Settings::DocumentView::defaultThumbnailSize());

    m_antialiasingCheckBox->setChecked(Settings::DocumentView::defaultAntialiasing());
    m_textAntialiasingCheckBox->setChecked(Settings::DocumentView::defaultTextAntialiasing());
    m_textHintingCheckBox->setChecked(Settings::DocumentView::defaultTextHinting());

    m_cacheSizeComboBox->setCurrentIndex(m_cacheSizeComboBox->findData(Settings::PageItem::defaultCacheSize()));
    m_prefetchCheckBox->setChecked(Settings::DocumentView::defaultPrefetch());

    // interface

    m_tabPositionComboBox->setCurrentIndex(m_tabPositionComboBox->findData(static_cast< uint >(Settings::MainWindow::defaultTabPosition())));
    m_tabVisibilityComboBox->setCurrentIndex(m_tabVisibilityComboBox->findData(static_cast< uint >(Settings::MainWindow::defaultTabVisibility())));

    m_fileToolBarLineEdit->setText(Settings::MainWindow::defaultFileToolBar().join(","));
    m_editToolBarLineEdit->setText(Settings::MainWindow::defaultEditToolBar().join(","));
    m_viewToolBarLineEdit->setText(Settings::MainWindow::defaultViewToolBar().join(","));

    // modifiers

    m_zoomModifiersComboBox->setCurrentIndex(m_zoomModifiersComboBox->findData(static_cast< int >(Settings::DocumentView::defaultZoomModifiers())));
    m_rotateModifiersComboBox->setCurrentIndex(m_rotateModifiersComboBox->findData(static_cast< int >(Settings::DocumentView::defaultRotateModifiers())));
    m_horizontalModifiersComboBox->setCurrentIndex(m_horizontalModifiersComboBox->findData(static_cast< int >(Settings::DocumentView::defaultHorizontalModifiers())));

    m_copyModifiersComboBox->setCurrentIndex(m_copyModifiersComboBox->findData(static_cast< int >(Settings::PageItem::defaultCopyModifiers())));
    m_annotateModifiersComboBox->setCurrentIndex(m_annotateModifiersComboBox->findData(static_cast< int >(Settings::PageItem::defaultAnnotateModifiers())));
}

void SettingsDialog::createBehaviorTab()
{
    // open URL

    m_openUrlCheckBox = new QCheckBox(this);
    m_openUrlCheckBox->setChecked(m_settings->documentView()->openUrl());

    m_behaviorLayout->addRow(tr("Open URL:"), m_openUrlCheckBox);

    // auto-refresh

    m_autoRefreshCheckBox = new QCheckBox(this);
    m_autoRefreshCheckBox->setChecked(m_settings->documentView()->autoRefresh());

    m_behaviorLayout->addRow(tr("Auto-refresh:"), m_autoRefreshCheckBox);

    // track recently used

    m_trackRecentlyUsedCheckBox = new QCheckBox(this);
    m_trackRecentlyUsedCheckBox->setChecked(m_settings->mainWindow()->trackRecentlyUsed());
    m_trackRecentlyUsedCheckBox->setToolTip(tr("Effective after restart."));

    m_behaviorLayout->addRow(tr("Track recently used:"), m_trackRecentlyUsedCheckBox);

    // restore tabs

    m_restoreTabsCheckBox = new QCheckBox(this);
    m_restoreTabsCheckBox->setChecked(m_settings->mainWindow()->restoreTabs());

    m_behaviorLayout->addRow(tr("Restore tabs:"), m_restoreTabsCheckBox);

    // restore bookmarks

    m_restoreBookmarksCheckBox = new QCheckBox(this);
    m_restoreBookmarksCheckBox->setChecked(m_settings->mainWindow()->restoreBookmarks());

    m_behaviorLayout->addRow(tr("Restore bookmarks:"), m_restoreBookmarksCheckBox);

    // restore per-file settings

    m_restorePerFileSettingsCheckBox = new QCheckBox(this);
    m_restorePerFileSettingsCheckBox->setChecked(m_settings->mainWindow()->restorePerFileSettings());

    m_behaviorLayout->addRow(tr("Restore per-file settings:"), m_restorePerFileSettingsCheckBox);

#ifndef WITH_SQL

    m_restorePerFileSettingsCheckBox->setEnabled(false);

#endif // WITH_SQL

    // presentation sync

    m_presentationSyncCheckBox = new QCheckBox(this);
    m_presentationSyncCheckBox->setChecked(m_settings->presentationView()->sync());

    m_behaviorLayout->addRow(tr("Synchronize presentation:"), m_presentationSyncCheckBox);

    // presentation screen

    m_presentationScreenSpinBox = new QSpinBox(this);
    m_presentationScreenSpinBox->setRange(-1, QApplication::desktop()->screenCount() - 1);
    m_presentationScreenSpinBox->setSpecialValueText(tr("Default"));
    m_presentationScreenSpinBox->setValue(m_settings->presentationView()->screen());

    m_behaviorLayout->addRow(tr("Presentation screen:"), m_presentationScreenSpinBox);

    // source editor

    m_sourceEditorLineEdit = new QLineEdit(this);
    m_sourceEditorLineEdit->setText(m_settings->documentView()->sourceEditor());
    m_sourceEditorLineEdit->setToolTip(tr("'%1' is replaced by the absolute file path. '%2' resp. '%3' is replaced by line resp. column number."));

    m_behaviorLayout->addRow(tr("Source editor:"), m_sourceEditorLineEdit);
}

void SettingsDialog::createGraphicsTab()
{
    // decorate pages

    m_decoratePagesCheckBox = new QCheckBox(this);
    m_decoratePagesCheckBox->setChecked(m_settings->pageItem()->decoratePages());

    m_graphicsLayout->addRow(tr("Decorate pages:"), m_decoratePagesCheckBox);

    // decorate links

    m_decorateLinksCheckBox = new QCheckBox(this);
    m_decorateLinksCheckBox->setChecked(m_settings->pageItem()->decorateLinks());

    m_graphicsLayout->addRow(tr("Decorate links:"), m_decorateLinksCheckBox);

    // decorate form fields

    m_decorateFormFieldsCheckBox = new QCheckBox(this);
    m_decorateFormFieldsCheckBox->setChecked(m_settings->pageItem()->decorateFormFields());

    m_graphicsLayout->addRow(tr("Decorate form fields:"), m_decorateFormFieldsCheckBox);

    // highlight duration

    m_highlightDurationSpinBox = new QSpinBox(this);
    m_highlightDurationSpinBox->setSuffix(" ms");
    m_highlightDurationSpinBox->setRange(0, 60000);
    m_highlightDurationSpinBox->setSingleStep(500);
    m_highlightDurationSpinBox->setSpecialValueText(tr("None"));
    m_highlightDurationSpinBox->setValue(m_settings->documentView()->highlightDuration());

    m_graphicsLayout->addRow(tr("Highlight duration:"), m_highlightDurationSpinBox);

    // background color

    m_backgroundColorLineEdit = new QLineEdit(this);
    m_backgroundColorLineEdit->setText(m_settings->pageItem()->backgroundColor());

    m_graphicsLayout->addRow(tr("Background color:"), m_backgroundColorLineEdit);

    // paper color

    m_paperColorLineEdit = new QLineEdit(this);
    m_paperColorLineEdit->setText(m_settings->pageItem()->paperColor());

    m_graphicsLayout->addRow(tr("Paper color:"), m_paperColorLineEdit);

    // invert colors

    m_invertColorsCheckBox = new QCheckBox(this);
    m_invertColorsCheckBox->setChecked(m_settings->pageItem()->invertColors());

    m_graphicsLayout->addRow(tr("Invert colors:"), m_invertColorsCheckBox);

    // overprint preview

    m_overprintPreviewCheckBox = new QCheckBox(this);
    m_overprintPreviewCheckBox->setChecked(m_settings->documentView()->overprintPreview());

    m_graphicsLayout->addRow(tr("Overprint preview:"), m_overprintPreviewCheckBox);

#ifdef HAS_POPPLER_22

    m_overprintPreviewCheckBox->setEnabled(Poppler::isOverprintPreviewAvailable());

#else

    m_overprintPreviewCheckBox->setEnabled(false);

#endif // HAS_POPPLER_22

    // pages per row

    m_pagesPerRowSpinBox = new QSpinBox(this);
    m_pagesPerRowSpinBox->setRange(1, 10);
    m_pagesPerRowSpinBox->setValue(m_settings->documentView()->pagesPerRow());

    m_graphicsLayout->addRow(tr("Pages per row:"), m_pagesPerRowSpinBox);

    // page spacing

    m_pageSpacingSpinBox = new QDoubleSpinBox(this);
    m_pageSpacingSpinBox->setSuffix(" px");
    m_pageSpacingSpinBox->setRange(0.0, 25.0);
    m_pageSpacingSpinBox->setSingleStep(0.25);
    m_pageSpacingSpinBox->setValue(m_settings->documentView()->pageSpacing());

    m_graphicsLayout->addRow(tr("Page spacing:"), m_pageSpacingSpinBox);

    // thumbnail spacing

    m_thumbnailSpacingSpinBox = new QDoubleSpinBox(this);
    m_thumbnailSpacingSpinBox->setSuffix(" px");
    m_thumbnailSpacingSpinBox->setRange(0.0, 25.0);
    m_thumbnailSpacingSpinBox->setSingleStep(0.25);
    m_thumbnailSpacingSpinBox->setValue(m_settings->documentView()->thumbnailSpacing());

    m_graphicsLayout->addRow(tr("Thumbnail spacing:"), m_thumbnailSpacingSpinBox);

    // thumbnail size

    m_thumbnailSizeSpinBox = new QDoubleSpinBox(this);
    m_thumbnailSizeSpinBox->setSuffix(" px");
    m_thumbnailSizeSpinBox->setRange(30.0, 300.0);
    m_thumbnailSizeSpinBox->setSingleStep(10.0);
    m_thumbnailSizeSpinBox->setValue(m_settings->documentView()->thumbnailSize());

    m_graphicsLayout->addRow(tr("Thumbnail size:"), m_thumbnailSizeSpinBox);

    // antialiasing

    m_antialiasingCheckBox = new QCheckBox(this);
    m_antialiasingCheckBox->setChecked(m_settings->documentView()->antialiasing());

    m_graphicsLayout->addRow(tr("Antialiasing:"), m_antialiasingCheckBox);

    // text antialising

    m_textAntialiasingCheckBox = new QCheckBox(this);
    m_textAntialiasingCheckBox->setChecked(m_settings->documentView()->textAntialiasing());

    m_graphicsLayout->addRow(tr("Text antialiasing:"), m_textAntialiasingCheckBox);

    // text hinting

    m_textHintingCheckBox = new QCheckBox(this);
    m_textHintingCheckBox->setChecked(m_settings->documentView()->textHinting());

    m_graphicsLayout->addRow(tr("Text hinting:"), m_textHintingCheckBox);

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
    m_cacheSizeComboBox->setCurrentIndex(m_cacheSizeComboBox->findData(m_settings->pageItem()->cacheSize()));

    m_graphicsLayout->addRow(tr("Cache size:"), m_cacheSizeComboBox);

    // prefetch

    m_prefetchCheckBox = new QCheckBox(this);
    m_prefetchCheckBox->setChecked(m_settings->documentView()->prefetch());

    m_graphicsLayout->addRow(tr("Prefetch:"), m_prefetchCheckBox);
}

void SettingsDialog::createInterfaceTab()
{
    // tab position

    m_tabPositionComboBox = new QComboBox(this);
    m_tabPositionComboBox->addItem(tr("Top"), static_cast< uint >(QTabWidget::North));
    m_tabPositionComboBox->addItem(tr("Bottom"), static_cast< uint >(QTabWidget::South));
    m_tabPositionComboBox->addItem(tr("Left"), static_cast< uint >(QTabWidget::West));
    m_tabPositionComboBox->addItem(tr("Right"), static_cast< uint >(QTabWidget::East));
    m_tabPositionComboBox->setCurrentIndex(m_tabPositionComboBox->findData(static_cast< uint >(m_settings->mainWindow()->tabPosition())));

    m_interfaceLayout->addRow(tr("Tab position:"), m_tabPositionComboBox);

    // tab visibility

    m_tabVisibilityComboBox = new QComboBox(this);
    m_tabVisibilityComboBox->addItem(tr("As needed"), static_cast< uint >(TabWidget::TabBarAsNeeded));
    m_tabVisibilityComboBox->addItem(tr("Always"), static_cast< uint >(TabWidget::TabBarAlwaysOn));
    m_tabVisibilityComboBox->addItem(tr("Never"), static_cast< uint >(TabWidget::TabBarAlwaysOff));
    m_tabVisibilityComboBox->setCurrentIndex(m_tabVisibilityComboBox->findData(static_cast< uint >(m_settings->mainWindow()->tabVisibility())));

    m_interfaceLayout->addRow(tr("Tab visibility:"), m_tabVisibilityComboBox);

    // file tool bar

    m_fileToolBarLineEdit = new QLineEdit(this);
    m_fileToolBarLineEdit->setText(m_settings->mainWindow()->fileToolBar().join(","));
    m_fileToolBarLineEdit->setToolTip(tr("Effective after restart."));

    m_interfaceLayout->addRow(tr("File tool bar:"), m_fileToolBarLineEdit);

    // edit tool bar

    m_editToolBarLineEdit = new QLineEdit(this);
    m_editToolBarLineEdit->setText(m_settings->mainWindow()->editToolBar().join(","));
    m_editToolBarLineEdit->setToolTip(tr("Effective after restart."));

    m_interfaceLayout->addRow(tr("Edit tool bar:"), m_editToolBarLineEdit);

    // view tool bar

    m_viewToolBarLineEdit = new QLineEdit(this);
    m_viewToolBarLineEdit->setText(m_settings->mainWindow()->viewToolBar().join(","));
    m_viewToolBarLineEdit->setToolTip(tr("Effective after restart."));

    m_interfaceLayout->addRow(tr("View tool bar:"), m_viewToolBarLineEdit);
}

void SettingsDialog::createModifiersTab()
{
    // zoom modifiers

    createModifiersComboBox(m_zoomModifiersComboBox, m_settings->documentView()->zoomModifiers());

    m_modifiersLayout->addRow(tr("Zoom modifiers:"), m_zoomModifiersComboBox);

    // rototate modifiers

    createModifiersComboBox(m_rotateModifiersComboBox, m_settings->documentView()->rotateModifiers());

    m_modifiersLayout->addRow(tr("Rotate modifiers:"), m_rotateModifiersComboBox);

    // horizontal modifiers

    createModifiersComboBox(m_horizontalModifiersComboBox, m_settings->documentView()->horizontalModifiers());

    m_modifiersLayout->addRow(tr("Horizontal modifiers:"), m_horizontalModifiersComboBox);

    // copy modifiers

    createModifiersComboBox(m_copyModifiersComboBox, m_settings->pageItem()->copyModifiers());

    m_modifiersLayout->addRow(tr("Copy modifiers:"), m_copyModifiersComboBox);

    // annotate modifiers

    createModifiersComboBox(m_annotateModifiersComboBox, m_settings->pageItem()->annotateModifiers());

    m_modifiersLayout->addRow(tr("Annotate modifiers:"), m_annotateModifiersComboBox);
}

void SettingsDialog::createModifiersComboBox(QComboBox*& comboBox, const Qt::KeyboardModifiers& modifiers)
{
    comboBox = new QComboBox(this);
    comboBox->addItem(tr("Shift"), static_cast< int >(Qt::ShiftModifier));
    comboBox->addItem(tr("Control"), static_cast< int >(Qt::ControlModifier));
    comboBox->addItem(tr("Alt"), static_cast< int >(Qt::AltModifier));
    comboBox->addItem(tr("Shift and Control"), static_cast< int >(Qt::ShiftModifier | Qt::ControlModifier));
    comboBox->addItem(tr("Shift and Alt"), static_cast< int >(Qt::ShiftModifier | Qt::AltModifier));
    comboBox->addItem(tr("Control and Alt"), static_cast< int >(Qt::ControlModifier | Qt::AltModifier));
    comboBox->setCurrentIndex(comboBox->findData(static_cast< int >(modifiers)));
}
