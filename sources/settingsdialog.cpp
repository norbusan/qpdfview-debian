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

#include "model.h"
#include "documentview.h"
#include "settings.h"

SettingsDialog::SettingsDialog(Settings* settings, QWidget* parent) : QDialog(parent),
    m_settings(settings)
{
    m_graphicsTabWidget = new QTabWidget(this);
    m_graphicsTabWidget->addTab(new QWidget(this), tr("General"));

#ifdef WITH_PDF

    m_pdfSettingsWidget = DocumentView::createPDFSettingsWidget(this);

    if(m_pdfSettingsWidget != 0)
    {
        m_graphicsTabWidget->addTab(m_pdfSettingsWidget, "PDF");
    }

#endif // WITH_PDF

#ifdef WITH_PS

    m_psSettingsWidget = DocumentView::createPSSettingsWidget(this);

    if(m_psSettingsWidget != 0)
    {
        m_graphicsTabWidget->addTab(m_psSettingsWidget, "PS");
    }

#endif // WITH_PS

    m_graphicsLayout = new QFormLayout(m_graphicsTabWidget->widget(0));

    m_shortcutsTableView = new QTableView(this);
    m_shortcutsTableView->setModel(m_settings->shortcuts()->createTableModel(this));

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
    connect(m_defaultsButton, SIGNAL(clicked()), SLOT(on_defaults_clicked()));

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

    qobject_cast< ShortcutsTableModel* >(m_shortcutsTableView->model())->accept();

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

    m_settings->pageItem()->setBackgroundColor(m_backgroundColorComboBox->currentText());
    m_settings->pageItem()->setPaperColor(m_paperColorComboBox->currentText());

    m_settings->documentView()->setPagesPerRow(m_pagesPerRowSpinBox->value());

    m_settings->documentView()->setPageSpacing(m_pageSpacingSpinBox->value());
    m_settings->documentView()->setThumbnailSpacing(m_thumbnailSpacingSpinBox->value());

    m_settings->documentView()->setThumbnailSize(m_thumbnailSizeSpinBox->value());

    m_settings->pageItem()->setCacheSize(m_cacheSizeComboBox->itemData(m_cacheSizeComboBox->currentIndex()).toInt());
    m_settings->documentView()->setPrefetch(m_prefetchCheckBox->isChecked());
    m_settings->documentView()->setPrefetchDistance(m_prefetchDistanceSpinBox->value());

    // interface

    m_settings->mainWindow()->setTabPosition(static_cast< QTabWidget::TabPosition >(m_tabPositionComboBox->itemData(m_tabPositionComboBox->currentIndex()).toUInt()));
    m_settings->mainWindow()->setTabVisibility(static_cast< TabWidget::TabBarPolicy >(m_tabVisibilityComboBox->itemData(m_tabVisibilityComboBox->currentIndex()).toUInt()));

    m_settings->mainWindow()->setNewTabNextToCurrentTab(m_newTabNextToCurrentTabCheckBox->isChecked());

    m_settings->mainWindow()->setFileToolBar(m_fileToolBarLineEdit->text().split(",", QString::SkipEmptyParts));
    m_settings->mainWindow()->setEditToolBar(m_editToolBarLineEdit->text().split(",", QString::SkipEmptyParts));
    m_settings->mainWindow()->setViewToolBar(m_viewToolBarLineEdit->text().split(",", QString::SkipEmptyParts));

    m_settings->documentView()->setLimitThumbnailsToResults(m_limitThumbnailsToResultsCheckBox->isChecked());

    // modifiers

    m_settings->documentView()->setZoomModifiers(static_cast<Qt::KeyboardModifier>(m_zoomModifiersComboBox->itemData(m_zoomModifiersComboBox->currentIndex()).toInt()));
    m_settings->documentView()->setRotateModifiers(static_cast<Qt::KeyboardModifier>(m_rotateModifiersComboBox->itemData(m_rotateModifiersComboBox->currentIndex()).toInt()));
    m_settings->documentView()->setScrollModifiers(static_cast<Qt::KeyboardModifier>(m_scrollModifiersComboBox->itemData(m_scrollModifiersComboBox->currentIndex()).toInt()));

    m_settings->pageItem()->setCopyToClipboardModifiers(static_cast<Qt::KeyboardModifier>(m_copyToClipboardModifiersComboBox->itemData(m_copyToClipboardModifiersComboBox->currentIndex()).toInt()));
    m_settings->pageItem()->setAddAnnotationModifiers(static_cast<Qt::KeyboardModifier>(m_addAnnotationModifiersComboBox->itemData(m_addAnnotationModifiersComboBox->currentIndex()).toInt()));
}

void SettingsDialog::on_defaults_clicked()
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

    qobject_cast< ShortcutsTableModel* >(m_shortcutsTableView->model())->reset();

    // behavior

    m_openUrlCheckBox->setChecked(Defaults::DocumentView::openUrl());

    m_autoRefreshCheckBox->setChecked(Defaults::DocumentView::autoRefresh());

    m_trackRecentlyUsedCheckBox->setChecked(Defaults::MainWindow::trackRecentlyUsed());

    m_restoreTabsCheckBox->setChecked(Defaults::MainWindow::restoreTabs());
    m_restoreBookmarksCheckBox->setChecked(Defaults::MainWindow::restoreBookmarks());
    m_restorePerFileSettingsCheckBox->setChecked(Defaults::MainWindow::restorePerFileSettings());

    m_presentationSyncCheckBox->setChecked(Defaults::PresentationView::sync());
    m_presentationScreenSpinBox->setValue(Defaults::PresentationView::screen());

    m_sourceEditorLineEdit->clear();

    // graphics

    m_decoratePagesCheckBox->setChecked(Defaults::PageItem::decoratePages());
    m_decorateLinksCheckBox->setChecked(Defaults::PageItem::decorateLinks());
    m_decorateFormFieldsCheckBox->setChecked(Defaults::PageItem::decorateFormFields());

    m_highlightDurationSpinBox->setValue(Defaults::DocumentView::highlightDuration());

    m_backgroundColorComboBox->lineEdit()->setText(Defaults::PageItem::backgroundColor());
    m_paperColorComboBox->lineEdit()->setText(Defaults::PageItem::paperColor());

    m_pagesPerRowSpinBox->setValue(Defaults::DocumentView::pagesPerRow());

    m_pageSpacingSpinBox->setValue(Defaults::DocumentView::pageSpacing());
    m_thumbnailSpacingSpinBox->setValue(Defaults::DocumentView::thumbnailSpacing());

    m_thumbnailSizeSpinBox->setValue(Defaults::DocumentView::thumbnailSize());

    m_cacheSizeComboBox->setCurrentIndex(m_cacheSizeComboBox->findData(Defaults::PageItem::cacheSize()));
    m_prefetchCheckBox->setChecked(Defaults::DocumentView::prefetch());
    m_prefetchDistanceSpinBox->setValue(Defaults::DocumentView::prefetchDistance());

    // interface

    m_tabPositionComboBox->setCurrentIndex(m_tabPositionComboBox->findData(static_cast< uint >(Defaults::MainWindow::tabPosition())));
    m_tabVisibilityComboBox->setCurrentIndex(m_tabVisibilityComboBox->findData(static_cast< uint >(Defaults::MainWindow::tabVisibility())));

    m_newTabNextToCurrentTabCheckBox->setChecked(Defaults::MainWindow::newTabNextToCurrentTab());

    m_fileToolBarLineEdit->setText(Defaults::MainWindow::fileToolBar().join(","));
    m_editToolBarLineEdit->setText(Defaults::MainWindow::editToolBar().join(","));
    m_viewToolBarLineEdit->setText(Defaults::MainWindow::viewToolBar().join(","));

    m_limitThumbnailsToResultsCheckBox->setChecked(Defaults::DocumentView::limitThumbnailsToResults());

    // modifiers

    m_zoomModifiersComboBox->setCurrentIndex(m_zoomModifiersComboBox->findData(static_cast< int >(Defaults::DocumentView::zoomModifiers())));
    m_rotateModifiersComboBox->setCurrentIndex(m_rotateModifiersComboBox->findData(static_cast< int >(Defaults::DocumentView::rotateModifiers())));
    m_scrollModifiersComboBox->setCurrentIndex(m_scrollModifiersComboBox->findData(static_cast< int >(Defaults::DocumentView::scrollModifiers())));

    m_copyToClipboardModifiersComboBox->setCurrentIndex(m_copyToClipboardModifiersComboBox->findData(static_cast< int >(Defaults::PageItem::copyToClipboardModifiers())));
    m_addAnnotationModifiersComboBox->setCurrentIndex(m_addAnnotationModifiersComboBox->findData(static_cast< int >(Defaults::PageItem::addAnnotationModifiers())));
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

    m_restoreTabsCheckBox->setEnabled(false);
    m_restoreBookmarksCheckBox->setEnabled(false);
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

    m_backgroundColorComboBox = new QComboBox(this);
    m_backgroundColorComboBox->setEditable(true);
    m_backgroundColorComboBox->setInsertPolicy(QComboBox::NoInsert);
    m_backgroundColorComboBox->addItems(QColor::colorNames());
    m_backgroundColorComboBox->lineEdit()->setText(m_settings->pageItem()->backgroundColor());

    m_graphicsLayout->addRow(tr("Background color:"), m_backgroundColorComboBox);

    // paper color

    m_paperColorComboBox = new QComboBox(this);
    m_paperColorComboBox->setEditable(true);
    m_paperColorComboBox->setInsertPolicy(QComboBox::NoInsert);
    m_paperColorComboBox->addItems(QColor::colorNames());
    m_paperColorComboBox->lineEdit()->setText(m_settings->pageItem()->paperColor());

    m_graphicsLayout->addRow(tr("Paper color:"), m_paperColorComboBox);

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

    // prefetch distance

    m_prefetchDistanceSpinBox = new QSpinBox(this);
    m_prefetchDistanceSpinBox->setRange(1, 10);
    m_prefetchDistanceSpinBox->setValue(m_settings->documentView()->prefetchDistance());

    m_graphicsLayout->addRow(tr("Prefetch distance:"), m_prefetchDistanceSpinBox);
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

    // new tab next to current tab

    m_newTabNextToCurrentTabCheckBox = new QCheckBox(this);
    m_newTabNextToCurrentTabCheckBox->setChecked(m_settings->mainWindow()->newTabNextToCurrentTab());

    m_interfaceLayout->addRow(tr("New tab next to current tab:"), m_newTabNextToCurrentTabCheckBox);

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

    // limit thumbnails to results

    m_limitThumbnailsToResultsCheckBox = new QCheckBox(this);
    m_limitThumbnailsToResultsCheckBox->setChecked(m_settings->documentView()->limitThumbnailsToResults());

    m_interfaceLayout->addRow(tr("Limit thumnails to results:"), m_limitThumbnailsToResultsCheckBox);
}

void SettingsDialog::createModifiersTab()
{
    // zoom modifiers

    createModifiersComboBox(m_zoomModifiersComboBox, m_settings->documentView()->zoomModifiers());

    m_modifiersLayout->addRow(tr("Zoom:"), m_zoomModifiersComboBox);

    // rototate modifiers

    createModifiersComboBox(m_rotateModifiersComboBox, m_settings->documentView()->rotateModifiers());

    m_modifiersLayout->addRow(tr("Rotate:"), m_rotateModifiersComboBox);

    // scroll modifiers

    createModifiersComboBox(m_scrollModifiersComboBox, m_settings->documentView()->scrollModifiers());

    m_modifiersLayout->addRow(tr("Scroll:"), m_scrollModifiersComboBox);

    // copy to clipboard modifiers

    createModifiersComboBox(m_copyToClipboardModifiersComboBox, m_settings->pageItem()->copyToClipboardModifiers());

    m_modifiersLayout->addRow(tr("Copy to clipboard:"), m_copyToClipboardModifiersComboBox);

    // add annotation modifiers

    createModifiersComboBox(m_addAnnotationModifiersComboBox, m_settings->pageItem()->addAnnotationModifiers());

    m_modifiersLayout->addRow(tr("Add annotation:"), m_addAnnotationModifiersComboBox);
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
