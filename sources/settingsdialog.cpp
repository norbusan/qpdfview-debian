/*

Copyright 2012 Adam Reichold

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

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent)
{
    m_settings = new QSettings(this);

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

    m_settings->setValue("documentView/openUrl", m_openUrlCheckBox->isChecked());

    m_settings->setValue("documentView/autoRefresh", m_autoRefreshCheckBox->isChecked());

    m_settings->setValue("mainWindow/trackRecentlyUsed", m_trackRecentlyUsedCheckBox->isChecked());

    m_settings->setValue("mainWindow/restoreTabs", m_restoreTabsCheckBox->isChecked());
    m_settings->setValue("mainWindow/restoreBookmarks", m_restoreBookmarksCheckBox->isChecked());
    m_settings->setValue("mainWindow/restorePerFileSettings", m_restorePerFileSettingsCheckBox->isChecked());

    m_settings->setValue("presentationView/sync", m_presentationSyncCheckBox->isChecked());
    m_settings->setValue("presentationView/screen", m_presentationScreenSpinBox->value());

    m_settings->setValue("documentView/sourceEditor", m_sourceEditorLineEdit->text());

    // graphics

    m_settings->setValue("pageItem/decoratePages", m_decoratePagesCheckBox->isChecked());
    m_settings->setValue("pageItem/decorateLinks", m_decorateLinksCheckBox->isChecked());
    m_settings->setValue("pageItem/decorateFormFields", m_decorateFormFieldsCheckBox->isChecked());

    m_settings->setValue("documentView/highlightDuration", m_highlightDurationSpinBox->value());

    m_settings->setValue("documentView/backgroundColor", m_backgroundColorLineEdit->text());
    m_settings->setValue("documentView/paperColor", m_paperColorLineEdit->text());
    m_settings->setValue("pageItem/invertColors", m_invertColorsCheckBox->isChecked());

    m_settings->setValue("documentView/overprintPreview", m_overprintPreviewCheckBox->isChecked());

    m_settings->setValue("documentView/pagesPerRow", m_pagesPerRowSpinBox->value());

    m_settings->setValue("documentView/pageSpacing", m_pageSpacingSpinBox->value());
    m_settings->setValue("documentView/thumbnailSpacing", m_thumbnailSpacingSpinBox->value());

    m_settings->setValue("documentView/thumbnailSize", m_thumbnailSizeSpinBox->value());

    m_settings->setValue("documentView/antialiasing", m_antialiasingCheckBox->isChecked());
    m_settings->setValue("documentView/textAntialiasing", m_textAntialiasingCheckBox->isChecked());
    m_settings->setValue("documentView/textHinting", m_textHintingCheckBox->isChecked());

    m_settings->setValue("pageItem/cacheSize", m_cacheSizeComboBox->itemData(m_cacheSizeComboBox->currentIndex()));
    m_settings->setValue("documentView/prefetch", m_prefetchCheckBox->isChecked());

    // interface

    m_settings->setValue("mainWindow/tabPosition", m_tabPositionComboBox->itemData(m_tabPositionComboBox->currentIndex()));
    m_settings->setValue("mainWindow/tabVisibility", m_tabVisibilityComboBox->itemData(m_tabVisibilityComboBox->currentIndex()));

    m_settings->setValue("mainWindow/fileToolBar", m_fileToolBarLineEdit->text().split(",", QString::SkipEmptyParts));
    m_settings->setValue("mainWindow/editToolBar", m_editToolBarLineEdit->text().split(",", QString::SkipEmptyParts));
    m_settings->setValue("mainWindow/viewToolBar", m_viewToolBarLineEdit->text().split(",", QString::SkipEmptyParts));

    // modifiers

    m_settings->setValue("documentView/zoomModifiers", m_zoomModifiersComboBox->itemData(m_zoomModifiersComboBox->currentIndex()));
    m_settings->setValue("documentView/rotateModifiers", m_rotateModifiersComboBox->itemData(m_rotateModifiersComboBox->currentIndex()));
    m_settings->setValue("documentView/horizontalModifiers", m_horizontalModifiersComboBox->itemData(m_horizontalModifiersComboBox->currentIndex()));

    m_settings->setValue("pageItem/copyModifiers", m_copyModifiersComboBox->itemData(m_copyModifiersComboBox->currentIndex()));
    m_settings->setValue("pageItem/annotateModifiers", m_annotateModifiersComboBox->itemData(m_annotateModifiersComboBox->currentIndex()));

    m_settings->sync();

    QDialog::accept();
}

void SettingsDialog::on_defaults_clicked()
{
    // behavior

    m_openUrlCheckBox->setChecked(false);

    m_autoRefreshCheckBox->setChecked(false);

    m_trackRecentlyUsedCheckBox->setChecked(false);

    m_restoreTabsCheckBox->setChecked(false);
    m_restoreBookmarksCheckBox->setChecked(false);
    m_restorePerFileSettingsCheckBox->setChecked(false);

    m_presentationSyncCheckBox->setChecked(false);
    m_presentationScreenSpinBox->setValue(-1);

    m_sourceEditorLineEdit->clear();

    // graphics

    m_decoratePagesCheckBox->setChecked(true);
    m_decorateLinksCheckBox->setChecked(true);
    m_decorateFormFieldsCheckBox->setChecked(true);

    m_highlightDurationSpinBox->setValue(5000);

    m_backgroundColorLineEdit->setText("gray");
    m_paperColorLineEdit->setText("white");
    m_invertColorsCheckBox->setChecked(false);

    m_overprintPreviewCheckBox->setChecked(false);

    m_pagesPerRowSpinBox->setValue(3);

    m_pageSpacingSpinBox->setValue(5.0);
    m_thumbnailSpacingSpinBox->setValue(3.0);

    m_thumbnailSizeSpinBox->setValue(150.0);

    m_antialiasingCheckBox->setChecked(true);
    m_textAntialiasingCheckBox->setChecked(true);
    m_textHintingCheckBox->setChecked(false);

    m_cacheSizeComboBox->setCurrentIndex(3);
    m_prefetchCheckBox->setChecked(false);

    // interface

    m_tabPositionComboBox->setCurrentIndex(0);
    m_tabVisibilityComboBox->setCurrentIndex(0);

    m_fileToolBarLineEdit->setText("openInNewTab,refresh");
    m_editToolBarLineEdit->setText("currentPage,previousPage,nextPage");
    m_viewToolBarLineEdit->setText("scaleFactor,zoomIn,zoomOut");

    // modifiers

    m_zoomModifiersComboBox->setCurrentIndex(1);
    m_rotateModifiersComboBox->setCurrentIndex(0);
    m_horizontalModifiersComboBox->setCurrentIndex(2);

    m_copyModifiersComboBox->setCurrentIndex(0);
    m_annotateModifiersComboBox->setCurrentIndex(1);
}

void SettingsDialog::createBehaviorTab()
{
    // open URL

    m_openUrlCheckBox = new QCheckBox(this);
    m_openUrlCheckBox->setChecked(m_settings->value("documentView/openUrl", false).toBool());

    m_behaviorLayout->addRow(tr("Open URL:"), m_openUrlCheckBox);

    // auto-refresh

    m_autoRefreshCheckBox = new QCheckBox(this);
    m_autoRefreshCheckBox->setChecked(m_settings->value("documentView/autoRefresh", false).toBool());

    m_behaviorLayout->addRow(tr("Auto-refresh:"), m_autoRefreshCheckBox);

    // track recently used

    m_trackRecentlyUsedCheckBox = new QCheckBox(this);
    m_trackRecentlyUsedCheckBox->setChecked(m_settings->value("mainWindow/trackRecentlyUsed", false).toBool());
    m_trackRecentlyUsedCheckBox->setToolTip(tr("Effective after restart."));

    m_behaviorLayout->addRow(tr("Track recently used:"), m_trackRecentlyUsedCheckBox);

    // restore tabs

    m_restoreTabsCheckBox = new QCheckBox(this);
    m_restoreTabsCheckBox->setChecked(m_settings->value("mainWindow/restoreTabs", false).toBool());

    m_behaviorLayout->addRow(tr("Restore tabs:"), m_restoreTabsCheckBox);

    // restore bookmarks

    m_restoreBookmarksCheckBox = new QCheckBox(this);
    m_restoreBookmarksCheckBox->setChecked(m_settings->value("mainWindow/restoreBookmarks", false).toBool());

    m_behaviorLayout->addRow(tr("Restore bookmarks:"), m_restoreBookmarksCheckBox);

    // restore per-file settings

    m_restorePerFileSettingsCheckBox = new QCheckBox(this);
    m_restorePerFileSettingsCheckBox->setChecked(m_settings->value("mainWindow/restorePerFileSettings", false).toBool());

    m_behaviorLayout->addRow(tr("Restore per-file settings:"), m_restorePerFileSettingsCheckBox);

#ifndef WITH_SQL

    m_restorePerFileSettingsCheckBox->setEnabled(false);

#endif // WITH_SQL

    // presentation sync

    m_presentationSyncCheckBox = new QCheckBox(this);
    m_presentationSyncCheckBox->setChecked(m_settings->value("presentationView/sync", false).toBool());

    m_behaviorLayout->addRow(tr("Synchronize presentation:"), m_presentationSyncCheckBox);

    // presentation screen

    m_presentationScreenSpinBox = new QSpinBox(this);
    m_presentationScreenSpinBox->setRange(-1, QApplication::desktop()->screenCount() - 1);
    m_presentationScreenSpinBox->setSpecialValueText(tr("Default"));
    m_presentationScreenSpinBox->setValue(m_settings->value("presentationView/screen", -1).toInt());

    m_behaviorLayout->addRow(tr("Presentation screen:"), m_presentationScreenSpinBox);

    // source editor

    m_sourceEditorLineEdit = new QLineEdit(this);
    m_sourceEditorLineEdit->setText(m_settings->value("documentView/sourceEditor").toString());
    m_sourceEditorLineEdit->setToolTip(tr("'%1' is replaced by the absolute file path. '%2' resp. '%3' is replaced by line resp. column number."));

    m_behaviorLayout->addRow(tr("Source editor:"), m_sourceEditorLineEdit);
}

void SettingsDialog::createGraphicsTab()
{
    // decorate pages

    m_decoratePagesCheckBox = new QCheckBox(this);
    m_decoratePagesCheckBox->setChecked(m_settings->value("pageItem/decoratePages", true).toBool());

    m_graphicsLayout->addRow(tr("Decorate pages:"), m_decoratePagesCheckBox);

    // decorate links

    m_decorateLinksCheckBox = new QCheckBox(this);
    m_decorateLinksCheckBox->setChecked(m_settings->value("pageItem/decorateLinks", true).toBool());

    m_graphicsLayout->addRow(tr("Decorate links:"), m_decorateLinksCheckBox);

    // decorate form fields

    m_decorateFormFieldsCheckBox = new QCheckBox(this);
    m_decorateFormFieldsCheckBox->setChecked(m_settings->value("pageItem/decorateFormFields", true).toBool());

    m_graphicsLayout->addRow(tr("Decorate form fields:"), m_decorateFormFieldsCheckBox);

    // highlight duration

    m_highlightDurationSpinBox = new QSpinBox(this);
    m_highlightDurationSpinBox->setSuffix(" ms");
    m_highlightDurationSpinBox->setRange(0, 60000);
    m_highlightDurationSpinBox->setSingleStep(500);
    m_highlightDurationSpinBox->setSpecialValueText(tr("None"));
    m_highlightDurationSpinBox->setValue(m_settings->value("documentView/highlightDuration", 5000).toInt());

    m_graphicsLayout->addRow(tr("Highlight duration:"), m_highlightDurationSpinBox);

    // background color

    m_backgroundColorLineEdit = new QLineEdit(this);
    m_backgroundColorLineEdit->setText(m_settings->value("documentView/backgroundColor", "gray").toString());

    m_graphicsLayout->addRow(tr("Background color:"), m_backgroundColorLineEdit);

    // paper color

    m_paperColorLineEdit = new QLineEdit(this);
    m_paperColorLineEdit->setText(m_settings->value("documentView/paperColor", "white").toString());

    m_graphicsLayout->addRow(tr("Paper color:"), m_paperColorLineEdit);

    // invert colors

    m_invertColorsCheckBox = new QCheckBox(this);
    m_invertColorsCheckBox->setChecked(m_settings->value("pageItem/invertColors", false).toBool());

    m_graphicsLayout->addRow(tr("Invert colors:"), m_invertColorsCheckBox);

    // overprint preview

    m_overprintPreviewCheckBox = new QCheckBox(this);
    m_overprintPreviewCheckBox->setChecked(m_settings->value("documentView/overprintPreview", false).toBool());

    m_graphicsLayout->addRow(tr("Overprint preview:"), m_overprintPreviewCheckBox);

#ifdef HAS_POPPLER_22

    m_overprintPreviewCheckBox->setEnabled(Poppler::isOverprintPreviewAvailable());

#else

    m_overprintPreviewCheckBox->setEnabled(false);

#endif // HAS_POPPLER_22

    // pages per row

    m_pagesPerRowSpinBox = new QSpinBox(this);
    m_pagesPerRowSpinBox->setRange(1, 10);
    m_pagesPerRowSpinBox->setValue(m_settings->value("documentView/pagesPerRow", 3).toInt());

    m_graphicsLayout->addRow(tr("Pages per row:"), m_pagesPerRowSpinBox);

    // page spacing

    m_pageSpacingSpinBox = new QDoubleSpinBox(this);
    m_pageSpacingSpinBox->setSuffix(" px");
    m_pageSpacingSpinBox->setRange(0.0, 25.0);
    m_pageSpacingSpinBox->setSingleStep(0.25);
    m_pageSpacingSpinBox->setValue(m_settings->value("documentView/pageSpacing", 5.0).toDouble());

    m_graphicsLayout->addRow(tr("Page spacing:"), m_pageSpacingSpinBox);

    // thumbnail spacing

    m_thumbnailSpacingSpinBox = new QDoubleSpinBox(this);
    m_thumbnailSpacingSpinBox->setSuffix(" px");
    m_thumbnailSpacingSpinBox->setRange(0.0, 25.0);
    m_thumbnailSpacingSpinBox->setSingleStep(0.25);
    m_thumbnailSpacingSpinBox->setValue(m_settings->value("documentView/thumbnailSpacing", 3.0).toDouble());

    m_graphicsLayout->addRow(tr("Thumbnail spacing:"), m_thumbnailSpacingSpinBox);

    // thumbnail size

    m_thumbnailSizeSpinBox = new QDoubleSpinBox(this);
    m_thumbnailSizeSpinBox->setSuffix(" px");
    m_thumbnailSizeSpinBox->setRange(30.0, 300.0);
    m_thumbnailSizeSpinBox->setSingleStep(10.0);
    m_thumbnailSizeSpinBox->setValue(m_settings->value("documentView/thumbnailSize", 150.0).toDouble());

    m_graphicsLayout->addRow(tr("Thumbnail size:"), m_thumbnailSizeSpinBox);

    // antialiasing

    m_antialiasingCheckBox = new QCheckBox(this);
    m_antialiasingCheckBox->setChecked(m_settings->value("documentView/antialiasing", true).toBool());

    m_graphicsLayout->addRow(tr("Antialiasing:"), m_antialiasingCheckBox);

    // text antialising

    m_textAntialiasingCheckBox = new QCheckBox(this);
    m_textAntialiasingCheckBox->setChecked(m_settings->value("documentView/textAntialiasing", true).toBool());

    m_graphicsLayout->addRow(tr("Text antialiasing:"), m_textAntialiasingCheckBox);

    // text hinting

    m_textHintingCheckBox = new QCheckBox(this);
    m_textHintingCheckBox->setChecked(m_settings->value("documentView/textHinting", false).toBool());

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

    int cacheSize = m_settings->value("pageItem/cacheSize", 32 * 1024 * 1024).toInt();

    for(int index = 0; index < m_cacheSizeComboBox->count(); ++index)
    {
        if(m_cacheSizeComboBox->itemData(index).toInt() == cacheSize)
        {
            m_cacheSizeComboBox->setCurrentIndex(index);
        }
    }

    m_graphicsLayout->addRow(tr("Cache size:"), m_cacheSizeComboBox);

    // prefetch

    m_prefetchCheckBox = new QCheckBox(this);
    m_prefetchCheckBox->setChecked(m_settings->value("documentView/prefetch", false).toBool());

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

    uint tabPosition = static_cast< uint >(m_settings->value("mainWindow/tabPosition", 0).toUInt());

    for(int index = 0; index < m_tabPositionComboBox->count(); ++index)
    {
        if(m_tabPositionComboBox->itemData(index).toUInt() == tabPosition)
        {
            m_tabPositionComboBox->setCurrentIndex(index);
        }
    }

    m_interfaceLayout->addRow(tr("Tab position:"), m_tabPositionComboBox);

    // tab visibility

    m_tabVisibilityComboBox = new QComboBox(this);
    m_tabVisibilityComboBox->addItem(tr("As needed"), static_cast< uint >(TabWidget::TabBarAsNeeded));
    m_tabVisibilityComboBox->addItem(tr("Always"), static_cast< uint >(TabWidget::TabBarAlwaysOn));
    m_tabVisibilityComboBox->addItem(tr("Never"), static_cast< uint >(TabWidget::TabBarAlwaysOff));

    uint tabBarPolicy = static_cast< uint >(m_settings->value("mainWindow/tabVisibility", 0).toUInt());

    for(int index = 0; index < m_tabVisibilityComboBox->count(); ++index)
    {
        if(m_tabVisibilityComboBox->itemData(index).toUInt() == tabBarPolicy)
        {
            m_tabVisibilityComboBox->setCurrentIndex(index);
        }
    }

    m_interfaceLayout->addRow(tr("Tab visibility:"), m_tabVisibilityComboBox);

    // file tool bar

    m_fileToolBarLineEdit = new QLineEdit(this);
    m_fileToolBarLineEdit->setText(m_settings->value("mainWindow/fileToolBar", QStringList() << "openInNewTab" << "refresh").toStringList().join(","));
    m_fileToolBarLineEdit->setToolTip(tr("Effective after restart."));

    m_interfaceLayout->addRow(tr("File tool bar:"), m_fileToolBarLineEdit);

    // edit tool bar

    m_editToolBarLineEdit = new QLineEdit(this);
    m_editToolBarLineEdit->setText(m_settings->value("mainWindow/editToolBar", QStringList() << "currentPage" << "previousPage" << "nextPage").toStringList().join(","));
    m_editToolBarLineEdit->setToolTip(tr("Effective after restart."));

    m_interfaceLayout->addRow(tr("Edit tool bar:"), m_editToolBarLineEdit);

    // view tool bar

    m_viewToolBarLineEdit = new QLineEdit(this);
    m_viewToolBarLineEdit->setText(m_settings->value("mainWindow/viewToolBar", QStringList() << "scaleFactor" << "zoomIn" << "zoomOut").toStringList().join(","));
    m_viewToolBarLineEdit->setToolTip(tr("Effective after restart."));

    m_interfaceLayout->addRow(tr("View tool bar:"), m_viewToolBarLineEdit);
}

void SettingsDialog::createModifiersTab()
{
    // zoom modifiers

    createModifiersComboBox(m_zoomModifiersComboBox, m_settings->value("documentView/zoomModifiers", 0x04000000).toInt());

    m_modifiersLayout->addRow(tr("Zoom modifiers:"), m_zoomModifiersComboBox);

    // rototate modifiers

    createModifiersComboBox(m_rotateModifiersComboBox, m_settings->value("documentView/rotateModifiers", 0x02000000).toInt());

    m_modifiersLayout->addRow(tr("Rotate modifiers:"), m_rotateModifiersComboBox);

    // horizontal modifiers

    createModifiersComboBox(m_horizontalModifiersComboBox, m_settings->value("documentView/horizontalModifiers", 0x08000000).toInt());

    m_modifiersLayout->addRow(tr("Horizontal modifiers:"), m_horizontalModifiersComboBox);

    // copy modifiers

    createModifiersComboBox(m_copyModifiersComboBox, m_settings->value("pageItem/copyModifiers", 0x02000000).toInt());

    m_modifiersLayout->addRow(tr("Copy modifiers:"), m_copyModifiersComboBox);

    // annotate modifiers

    createModifiersComboBox(m_annotateModifiersComboBox, m_settings->value("pageItem/annotateModifiers", 0x04000000).toInt());

    m_modifiersLayout->addRow(tr("Annotate modifiers:"), m_annotateModifiersComboBox);
}

void SettingsDialog::createModifiersComboBox(QComboBox*& comboBox, int modifiers)
{
    comboBox = new QComboBox(this);
    comboBox->addItem(tr("Shift"), static_cast< int >(Qt::ShiftModifier));
    comboBox->addItem(tr("Control"), static_cast< int >(Qt::ControlModifier));
    comboBox->addItem(tr("Alt"), static_cast< int >(Qt::AltModifier));
    comboBox->addItem(tr("Shift and Control"), static_cast< int >(Qt::ShiftModifier | Qt::ControlModifier));
    comboBox->addItem(tr("Shift and Alt"), static_cast< int >(Qt::ShiftModifier | Qt::AltModifier));
    comboBox->addItem(tr("Control and Alt"), static_cast< int >(Qt::ControlModifier | Qt::AltModifier));

    for(int index = 0; index < comboBox->count(); ++index)
    {
        if(comboBox->itemData(index).toInt() == modifiers)
        {
            comboBox->setCurrentIndex(index);
        }
    }
}
