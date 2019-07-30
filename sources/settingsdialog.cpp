/*

Copyright 2015 S. Razi Alavizadeh
Copyright 2012-2015 Adam Reichold
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
#include <QGroupBox>
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

void setCurrentIndexFromData(QComboBox* comboBox, int data)
{
    comboBox->setCurrentIndex(comboBox->findData(data));
}

int dataFromCurrentIndex(const QComboBox* comboBox)
{
    return comboBox->itemData(comboBox->currentIndex()).toInt();
}

void setCurrentTextToColorName(QComboBox* comboBox, const QColor& color)
{
    comboBox->lineEdit()->setText(color.isValid() ? color.name() : QString());
}

QColor validColorFromCurrentText(const QComboBox* comboBox, const QColor& defaultColor)
{
    const QColor color(comboBox->currentText());

    return color.isValid() ? color : defaultColor;
}

void setCurrentIndexFromKeyboardModifiers(QComboBox* comboBox, Qt::KeyboardModifiers modifiers)
{
    comboBox->setCurrentIndex(comboBox->findData(static_cast< int >(modifiers)));
}

Qt::KeyboardModifier keyboardModifierFromCurrentIndex(const QComboBox* comboBox)
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

    m_wheelModifiersGroupBox = new QGroupBox(tr("Mouse wheel modifiers"));
    m_wheelModifiersLayout = new QFormLayout(m_wheelModifiersGroupBox);

    m_buttonModifiersGroupBox = new QGroupBox(tr("Mouse button modifiers"));
    m_buttonModifiersLayout = new QFormLayout(m_buttonModifiersGroupBox);

    QWidget* modifiersTab = m_tabWidget->widget(4);
    QVBoxLayout* modifiersLayout = new QVBoxLayout(modifiersTab);
    modifiersTab->setLayout(modifiersLayout);
    modifiersLayout->addWidget(m_wheelModifiersGroupBox);
    modifiersLayout->addWidget(m_buttonModifiersGroupBox);
    modifiersLayout->addStretch();

    QVBoxLayout* layout = new QVBoxLayout(this);
    setLayout(layout);
    layout->addWidget(m_tabWidget);
    layout->addWidget(m_dialogButtonBox);

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
    m_openUrlCheckBox = addCheckBox(m_behaviorLayout, tr("Open URL:"), QString(),
                                    s_settings->documentView().openUrl());


    m_autoRefreshCheckBox = addCheckBox(m_behaviorLayout, tr("Auto-refresh:"), QString(),
                                        s_settings->documentView().autoRefresh());


    m_trackRecentlyUsedCheckBox = addCheckBox(m_behaviorLayout, tr("Track recently used:"), tr("Effective after restart."),
                                              s_settings->mainWindow().trackRecentlyUsed());

    m_keepRecentlyClosedCheckBox = addCheckBox(m_behaviorLayout, tr("Keep recently closed:"), tr("Effective after restart."),
                                               s_settings->mainWindow().keepRecentlyClosed());


    m_restoreTabsCheckBox = addCheckBox(m_behaviorLayout, tr("Restore tabs:"), QString(),
                                        s_settings->mainWindow().restoreTabs());

    m_restoreBookmarksCheckBox = addCheckBox(m_behaviorLayout, tr("Restore bookmarks:"), QString(),
                                             s_settings->mainWindow().restoreBookmarks());

    m_restorePerFileSettingsCheckBox = addCheckBox(m_behaviorLayout, tr("Restore per-file settings:"), QString(),
                                                   s_settings->mainWindow().restorePerFileSettings());

    m_saveDatabaseInterval = addSpinBox(m_behaviorLayout, tr("Save database interval:"), QString(), tr(" min"), tr("Never"),
                                        -1, 60, 1, s_settings->mainWindow().saveDatabaseInterval() / 1000 / 60);

#ifndef WITH_SQL

    m_restoreTabsCheckBox->setEnabled(false);
    m_restoreBookmarksCheckBox->setEnabled(false);
    m_restorePerFileSettingsCheckBox->setEnabled(false);
    m_saveDatabaseInterval->setEnabled(false);

#endif // WITH_SQL


    m_synchronizePresentationCheckBox = addCheckBox(m_behaviorLayout, tr("Synchronize presentation:"), QString(),
                                                    s_settings->presentationView().synchronize());

    m_presentationScreenSpinBox = addSpinBox(m_behaviorLayout, tr("Presentation screen:"), QString(), QString(), tr("Default"),
                                             -1, QApplication::desktop()->screenCount() - 1, 1, s_settings->presentationView().screen());


    m_synchronizeOutlineViewCheckBox = addCheckBox(m_behaviorLayout, tr("Synchronize outline view:"), QString(),
                                                   s_settings->mainWindow().synchronizeOutlineView());

    m_synchronizeSplitViewsCheckBox = addCheckBox(m_behaviorLayout, tr("Synchronize split views:"), QString(),
                                                  s_settings->mainWindow().synchronizeSplitViews());


    m_minimalScrollingCheckBox = addCheckBox(m_behaviorLayout, tr("Minimal scrolling:"), QString(),
                                             s_settings->documentView().minimalScrolling());

    m_zoomFactorSpinBox = addDoubleSpinBox(m_behaviorLayout, tr("Zoom factor:"), QString(), QString(), QString(),
                                           1.0, 2.0, 0.05, s_settings->documentView().zoomFactor());

    m_parallelSearchExecutionCheckBox = addCheckBox(m_behaviorLayout, tr("Parallel search execution:"), QString(),
                                                    s_settings->documentView().parallelSearchExecution());


    m_highlightDurationSpinBox = addSpinBox(m_behaviorLayout, tr("Highlight duration:"), QString(), tr(" ms"), tr("None"),
                                            0, 60000, 500, s_settings->documentView().highlightDuration());

    m_highlightColorComboBox = addColorComboBox(m_behaviorLayout, tr("Highlight color:"), QString(),
                                                s_settings->pageItem().highlightColor());

    m_annotationColorComboBox = addColorComboBox(m_behaviorLayout, tr("Annotation color:"), QString(),
                                                 s_settings->pageItem().annotationColor());


    m_sourceEditorLineEdit = addLineEdit(m_behaviorLayout, tr("Source editor:"), tr("'%1' is replaced by the absolute file path. '%2' resp. '%3' is replaced by line resp. column number."),
                                         s_settings->documentView().sourceEditor());
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
    s_settings->mainWindow().setSynchronizeSplitViews(m_synchronizeSplitViewsCheckBox->isChecked());

    s_settings->documentView().setMinimalScrolling(m_minimalScrollingCheckBox->isChecked());
    s_settings->documentView().setZoomFactor(m_zoomFactorSpinBox->value());
    s_settings->documentView().setParallelSearchExecution(m_parallelSearchExecutionCheckBox->isChecked());

    s_settings->documentView().setHighlightDuration(m_highlightDurationSpinBox->value());
    s_settings->pageItem().setHighlightColor(validColorFromCurrentText(m_highlightColorComboBox, Defaults::PageItem::highlightColor()));
    s_settings->pageItem().setAnnotationColor(validColorFromCurrentText(m_annotationColorComboBox, Defaults::PageItem::annotationColor()));

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
    m_synchronizeSplitViewsCheckBox->setChecked(Defaults::MainWindow::synchronizeSplitViews());

    m_minimalScrollingCheckBox->setChecked(Defaults::DocumentView::minimalScrolling());
    m_zoomFactorSpinBox->setValue(Defaults::DocumentView::zoomFactor());
    m_parallelSearchExecutionCheckBox->setChecked(Defaults::DocumentView::parallelSearchExecution());

    m_highlightDurationSpinBox->setValue(Defaults::DocumentView::highlightDuration());
    setCurrentTextToColorName(m_highlightColorComboBox, Defaults::PageItem::highlightColor());
    setCurrentTextToColorName(m_annotationColorComboBox, Defaults::PageItem::annotationColor());

    m_sourceEditorLineEdit->clear();
}

void SettingsDialog::createGraphicsTab()
{
    m_useTilingCheckBox = addCheckBox(m_graphicsLayout, tr("Use tiling:"), QString(),
                                      s_settings->pageItem().useTiling());

    m_keepObsoletePixmapsCheckBox = addCheckBox(m_graphicsLayout, tr("Keep obsolete pixmaps:"), QString(),
                                                s_settings->pageItem().keepObsoletePixmaps());

    m_useDevicePixelRatioCheckBox = addCheckBox(m_graphicsLayout, tr("Use device pixel ratio:"), QString(),
                                                s_settings->pageItem().useDevicePixelRatio());

#if QT_VERSION < QT_VERSION_CHECK(5,1,0)

    m_useDevicePixelRatioCheckBox->setEnabled(false);

#endif // QT_VERSION


    m_decoratePagesCheckBox = addCheckBox(m_graphicsLayout, tr("Decorate pages:"), QString(),
                                          s_settings->pageItem().decoratePages());

    m_decorateLinksCheckBox = addCheckBox(m_graphicsLayout, tr("Decorate links:"), QString(),
                                          s_settings->pageItem().decorateLinks());

    m_decorateFormFieldsCheckBox = addCheckBox(m_graphicsLayout, tr("Decorate form fields:"), QString(),
                                               s_settings->pageItem().decorateFormFields());


    m_backgroundColorComboBox = addColorComboBox(m_graphicsLayout, tr("Background color:"), QString(),
                                                 s_settings->pageItem().backgroundColor());

    m_paperColorComboBox = addColorComboBox(m_graphicsLayout, tr("Paper color:"), QString(),
                                            s_settings->pageItem().paperColor());

    m_presentationBackgroundColorComboBox = addColorComboBox(m_graphicsLayout, tr("Presentation background color:"), QString(),
                                                             s_settings->presentationView().backgroundColor());


    m_pagesPerRowSpinBox = addSpinBox(m_graphicsLayout, tr("Pages per row:"), QString(), QString(), QString(),
                                      1, 10, 1, s_settings->documentView().pagesPerRow());


    m_pageSpacingSpinBox = addDoubleSpinBox(m_graphicsLayout, tr("Page spacing:"), QString(), tr(" px"), QString(),
                                            0.0, 25.0, 0.25, s_settings->documentView().pageSpacing());

    m_thumbnailSpacingSpinBox = addDoubleSpinBox(m_graphicsLayout, tr("Thumbnail spacing:"), QString(), tr(" px"), QString(),
                                                 0.0, 25.0, 0.25, s_settings->documentView().thumbnailSpacing());


    m_thumbnailSizeSpinBox = addDoubleSpinBox(m_graphicsLayout, tr("Thumbnail size:"), QString(), tr(" px"), tr("Fit to viewport"),
                                              0.0, 1800.0, 25.0, s_settings->documentView().thumbnailSize());


    m_cacheSizeComboBox = addDataSizeComboBox(m_graphicsLayout, tr("Cache size:"), QString(),
                                              s_settings->pageItem().cacheSize());

    m_prefetchCheckBox = addCheckBox(m_graphicsLayout, tr("Prefetch:"), QString(),
                                     s_settings->documentView().prefetch());

    m_prefetchDistanceSpinBox = addSpinBox(m_graphicsLayout, tr("Prefetch distance:"), QString(), QString(), QString(),
                                           1, 10, 1, s_settings->documentView().prefetchDistance());
}

void SettingsDialog::acceptGraphicsTab()
{
    s_settings->pageItem().setUseTiling(m_useTilingCheckBox->isChecked());
    s_settings->pageItem().setKeepObsoletePixmaps(m_keepObsoletePixmapsCheckBox->isChecked());
    s_settings->pageItem().setUseDevicePixelRatio(m_useDevicePixelRatioCheckBox->isChecked());

    s_settings->pageItem().setDecoratePages(m_decoratePagesCheckBox->isChecked());
    s_settings->pageItem().setDecorateLinks(m_decorateLinksCheckBox->isChecked());
    s_settings->pageItem().setDecorateFormFields(m_decorateFormFieldsCheckBox->isChecked());

    s_settings->pageItem().setBackgroundColor(validColorFromCurrentText(m_backgroundColorComboBox, Defaults::PageItem::backgroundColor()));
    s_settings->pageItem().setPaperColor(validColorFromCurrentText(m_paperColorComboBox, Defaults::PageItem::paperColor()));
    s_settings->presentationView().setBackgroundColor(validColorFromCurrentText(m_presentationBackgroundColorComboBox, Defaults::PresentationView::backgroundColor()));

    s_settings->documentView().setPagesPerRow(m_pagesPerRowSpinBox->value());

    s_settings->documentView().setPageSpacing(m_pageSpacingSpinBox->value());
    s_settings->documentView().setThumbnailSpacing(m_thumbnailSpacingSpinBox->value());

    s_settings->documentView().setThumbnailSize(m_thumbnailSizeSpinBox->value());

    s_settings->pageItem().setCacheSize(dataFromCurrentIndex(m_cacheSizeComboBox));
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
    m_useDevicePixelRatioCheckBox->setChecked(Defaults::PageItem::useDevicePixelRatio());

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

    setCurrentIndexFromData(m_cacheSizeComboBox, Defaults::PageItem::cacheSize());
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
    m_extendedSearchDock = addCheckBox(m_interfaceLayout, tr("Extended search dock:"), tr("Effective after restart."),
                                       s_settings->mainWindow().extendedSearchDock());

    m_annotationOverlayCheckBox = addCheckBox(m_interfaceLayout, tr("Annotation overlay:"), QString(),
                                              s_settings->pageItem().annotationOverlay());

    m_formFieldOverlayCheckBox = addCheckBox(m_interfaceLayout, tr("Form field overlay:"), QString(),
                                             s_settings->pageItem().formFieldOverlay());


    m_tabPositionComboBox = addComboBox(m_interfaceLayout, tr("Tab position:"), QString(),
                                        QStringList() << tr("Top") << tr("Bottom") << tr("Left") << tr("Right"),
                                        QList< int >() << QTabWidget::North << QTabWidget::South << QTabWidget::West << QTabWidget::East,
                                        s_settings->mainWindow().tabPosition());

    m_tabVisibilityComboBox = addComboBox(m_interfaceLayout, tr("Tab visibility:"), QString(),
                                          QStringList() << tr("As needed") << tr("Always") << tr("Never"),
                                          QList< int >() << TabWidget::TabBarAsNeeded << TabWidget::TabBarAlwaysOn << TabWidget::TabBarAlwaysOff,
                                          s_settings->mainWindow().tabVisibility());

    m_spreadTabsCheckBox = addCheckBox(m_interfaceLayout, tr("Spread tabs:"), QString(),
                                       s_settings->mainWindow().spreadTabs());


    m_newTabNextToCurrentTabCheckBox = addCheckBox(m_interfaceLayout, tr("New tab next to current tab:"), QString(),
                                                   s_settings->mainWindow().newTabNextToCurrentTab());

    m_exitAfterLastTabCheckBox = addCheckBox(m_interfaceLayout, tr("Exit after last tab:"), QString(),
                                             s_settings->mainWindow().exitAfterLastTab());


    m_recentlyUsedCountSpinBox = addSpinBox(m_interfaceLayout, tr("Recently used count:"), tr("Effective after restart."), QString(), QString(),
                                            1, 50, 1, s_settings->mainWindow().recentlyUsedCount());

    m_recentlyClosedCountSpinBox = addSpinBox(m_interfaceLayout, tr("Recently closed count:"), tr("Effective after restart."), QString(), QString(),
                                              1, 25, 1, s_settings->mainWindow().recentlyClosedCount());


    m_fileToolBarLineEdit = addLineEdit(m_interfaceLayout, tr("File tool bar:"), tr("Effective after restart."),
                                        s_settings->mainWindow().fileToolBar().join(","));

    m_editToolBarLineEdit = addLineEdit(m_interfaceLayout, tr("Edit tool bar:"), tr("Effective after restart."),
                                        s_settings->mainWindow().editToolBar().join(","));

    m_viewToolBarLineEdit = addLineEdit(m_interfaceLayout, tr("View tool bar:"), tr("Effective after restart."),
                                        s_settings->mainWindow().viewToolBar().join(","));

    m_documentContextMenuLineEdit = addLineEdit(m_interfaceLayout, tr("Document context menu:"), QString(),
                                                s_settings->mainWindow().documentContextMenu().join(","));

    m_tabContextMenuLineEdit = addLineEdit(m_interfaceLayout, tr("Tab context menu:"), QString(),
                                           s_settings->mainWindow().tabContextMenu().join(","));

    m_scrollableMenusCheckBox = addCheckBox(m_interfaceLayout, tr("Scrollable menus:"), tr("Effective after restart."),
                                            s_settings->mainWindow().scrollableMenus());

    m_searchableMenusCheckBox = addCheckBox(m_interfaceLayout, tr("Searchable menus:"), QString(),
                                            s_settings->mainWindow().searchableMenus());


    m_toggleToolAndMenuBarsWithFullscreenCheckBox = addCheckBox(m_interfaceLayout, tr("Toggle tool and menu bars with fullscreen:"), QString(),
                                                                s_settings->mainWindow().toggleToolAndMenuBarsWithFullscreen());


    m_usePageLabelCheckBox = addCheckBox(m_interfaceLayout, tr("Use page label:"), QString(),
                                         s_settings->mainWindow().usePageLabel());

    m_documentTitleAsTabTitleCheckBox = addCheckBox(m_interfaceLayout, tr("Document title as tab title:"), QString(),
                                                    s_settings->mainWindow().documentTitleAsTabTitle());


    m_currentPageInWindowTitleCheckBox = addCheckBox(m_interfaceLayout, tr("Current page in window title:"), QString(),
                                                     s_settings->mainWindow().currentPageInWindowTitle());

    m_instanceNameInWindowTitleCheckBox = addCheckBox(m_interfaceLayout, tr("Instance name in window title:"), QString(),
                                                      s_settings->mainWindow().instanceNameInWindowTitle());


    m_highlightCurrentThumbnailCheckBox = addCheckBox(m_interfaceLayout, tr("Highlight current thumbnail:"), QString(),
                                                      s_settings->documentView().highlightCurrentThumbnail());

    m_limitThumbnailsToResultsCheckBox = addCheckBox(m_interfaceLayout, tr("Limit thumbnails to results:"), QString(),
                                                     s_settings->documentView().limitThumbnailsToResults());
}

void SettingsDialog::acceptInterfaceTab()
{
    s_settings->mainWindow().setExtendedSearchDock(m_extendedSearchDock->isChecked());

    s_settings->pageItem().setAnnotationOverlay(m_annotationOverlayCheckBox->isChecked());
    s_settings->pageItem().setFormFieldOverlay(m_formFieldOverlayCheckBox);

    s_settings->mainWindow().setTabPosition(dataFromCurrentIndex(m_tabPositionComboBox));
    s_settings->mainWindow().setTabVisibility(dataFromCurrentIndex(m_tabVisibilityComboBox));
    s_settings->mainWindow().setSpreadTabs(m_spreadTabsCheckBox->isChecked());

    s_settings->mainWindow().setNewTabNextToCurrentTab(m_newTabNextToCurrentTabCheckBox->isChecked());
    s_settings->mainWindow().setExitAfterLastTab(m_exitAfterLastTabCheckBox->isChecked());

    s_settings->mainWindow().setRecentlyUsedCount(m_recentlyUsedCountSpinBox->value());
    s_settings->mainWindow().setRecentlyClosedCount(m_recentlyClosedCountSpinBox->value());

    s_settings->mainWindow().setFileToolBar(m_fileToolBarLineEdit->text().split(",", QString::SkipEmptyParts));
    s_settings->mainWindow().setEditToolBar(m_editToolBarLineEdit->text().split(",", QString::SkipEmptyParts));
    s_settings->mainWindow().setViewToolBar(m_viewToolBarLineEdit->text().split(",", QString::SkipEmptyParts));

    s_settings->mainWindow().setDocumentContextMenu(m_documentContextMenuLineEdit->text().split(",", QString::SkipEmptyParts));
    s_settings->mainWindow().setTabContextMenu(m_tabContextMenuLineEdit->text().split(",", QString::SkipEmptyParts));

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

    setCurrentIndexFromData(m_tabPositionComboBox, Defaults::MainWindow::tabPosition());
    setCurrentIndexFromData(m_tabVisibilityComboBox, Defaults::MainWindow::tabVisibility());
    m_spreadTabsCheckBox->setChecked(Defaults::MainWindow::spreadTabs());

    m_newTabNextToCurrentTabCheckBox->setChecked(Defaults::MainWindow::newTabNextToCurrentTab());
    m_exitAfterLastTabCheckBox->setChecked(Defaults::MainWindow::exitAfterLastTab());

    m_recentlyUsedCountSpinBox->setValue(Defaults::MainWindow::recentlyUsedCount());
    m_recentlyClosedCountSpinBox->setValue(Defaults::MainWindow::recentlyClosedCount());

    m_fileToolBarLineEdit->setText(Defaults::MainWindow::fileToolBar().join(","));
    m_editToolBarLineEdit->setText(Defaults::MainWindow::editToolBar().join(","));
    m_viewToolBarLineEdit->setText(Defaults::MainWindow::viewToolBar().join(","));

    m_documentContextMenuLineEdit->setText(Defaults::MainWindow::documentContextMenu().join(","));
    m_tabContextMenuLineEdit->setText(Defaults::MainWindow::tabContexntMenu().join(","));

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
    m_zoomModifiersComboBox = addModifiersComboBox(m_wheelModifiersLayout, tr("Zoom:"), QString(),
                                                   s_settings->documentView().zoomModifiers());

    m_rotateModifiersComboBox = addModifiersComboBox(m_wheelModifiersLayout, tr("Rotate:"), QString(),
                                                     s_settings->documentView().rotateModifiers());

    m_scrollModifiersComboBox = addModifiersComboBox(m_wheelModifiersLayout, tr("Scroll:"), QString(),
                                                     s_settings->documentView().scrollModifiers());

    m_copyToClipboardModifiersComboBox = addModifiersComboBox(m_buttonModifiersLayout, tr("Copy to clipboard:"), QString(),
                                                              s_settings->pageItem().copyToClipboardModifiers());

    m_addAnnotationModifiersComboBox = addModifiersComboBox(m_buttonModifiersLayout, tr("Add annotation:"), QString(),
                                                            s_settings->pageItem().addAnnotationModifiers());

    m_zoomToSelectionModifiersComboBox = addModifiersComboBox(m_buttonModifiersLayout, tr("Zoom to selection:"), QString(),
                                                              s_settings->pageItem().zoomToSelectionModifiers());

    m_openInSourceEditorModifiersComboBox = addModifiersComboBox(m_buttonModifiersLayout, tr("Open in source editor:"), QString(),
                                                                 s_settings->pageItem().openInSourceEditorModifiers());

#ifndef WITH_SYNCTEX

    m_openInSourceEditorModifiersComboBox->setEnabled(false);

#endif // WITH_SYNCTEX
}

void SettingsDialog::acceptModifiersTab()
{
    s_settings->documentView().setZoomModifiers(keyboardModifierFromCurrentIndex(m_zoomModifiersComboBox));
    s_settings->documentView().setRotateModifiers(keyboardModifierFromCurrentIndex(m_rotateModifiersComboBox));
    s_settings->documentView().setScrollModifiers(keyboardModifierFromCurrentIndex(m_scrollModifiersComboBox));

    s_settings->pageItem().setCopyToClipboardModifiers(keyboardModifierFromCurrentIndex(m_copyToClipboardModifiersComboBox));
    s_settings->pageItem().setAddAnnotationModifiers(keyboardModifierFromCurrentIndex(m_addAnnotationModifiersComboBox));
    s_settings->pageItem().setZoomToSelectionModifiers(keyboardModifierFromCurrentIndex(m_zoomToSelectionModifiersComboBox));
    s_settings->pageItem().setOpenInSourceEditorModifiers(keyboardModifierFromCurrentIndex(m_openInSourceEditorModifiersComboBox));
}

void SettingsDialog::resetModifiersTab()
{
    setCurrentIndexFromKeyboardModifiers(m_zoomModifiersComboBox, Defaults::DocumentView::zoomModifiers());
    setCurrentIndexFromKeyboardModifiers(m_rotateModifiersComboBox, Defaults::DocumentView::rotateModifiers());
    setCurrentIndexFromKeyboardModifiers(m_scrollModifiersComboBox, Defaults::DocumentView::scrollModifiers());

    setCurrentIndexFromKeyboardModifiers(m_copyToClipboardModifiersComboBox, Defaults::PageItem::copyToClipboardModifiers());
    setCurrentIndexFromKeyboardModifiers(m_addAnnotationModifiersComboBox, Defaults::PageItem::addAnnotationModifiers());
    setCurrentIndexFromKeyboardModifiers(m_zoomToSelectionModifiersComboBox, Defaults::PageItem::zoomToSelectionModifiers());
    setCurrentIndexFromKeyboardModifiers(m_openInSourceEditorModifiersComboBox, Defaults::PageItem::openInSourceEditorModifiers());
}

QCheckBox* SettingsDialog::addCheckBox(QFormLayout* layout, const QString& label, const QString& toolTip, bool checked)
{
    QCheckBox* checkBox = new QCheckBox(this);
    checkBox->setChecked(checked);

    checkBox->setToolTip(toolTip);
    layout->addRow(label, checkBox);

    return checkBox;
}

QLineEdit* SettingsDialog::addLineEdit(QFormLayout* layout, const QString& label, const QString& toolTip, const QString& text)
{
    QLineEdit* lineEdit = new QLineEdit(this);
    lineEdit->setText(text);

    lineEdit->setToolTip(toolTip);
    layout->addRow(label, lineEdit);

    return lineEdit;
}

QSpinBox* SettingsDialog::addSpinBox(QFormLayout* layout, const QString& label, const QString& toolTip, const QString& suffix, const QString& special, int min, int max, int step, int val)
{
    QSpinBox* spinBox = new QSpinBox(this);
    spinBox->setRange(min, max);
    spinBox->setSingleStep(step);
    spinBox->setValue(val);

    spinBox->setSuffix(suffix);
    spinBox->setSpecialValueText(special);

    spinBox->setToolTip(toolTip);
    layout->addRow(label, spinBox);

    return spinBox;
}

QDoubleSpinBox* SettingsDialog::addDoubleSpinBox(QFormLayout* layout, const QString& label, const QString& toolTip, const QString& suffix, const QString& special, double min, double max, double step, double val)
{
    QDoubleSpinBox* spinBox = new QDoubleSpinBox(this);
    spinBox->setRange(min, max);
    spinBox->setSingleStep(step);
    spinBox->setValue(val);

    spinBox->setSuffix(suffix);
    spinBox->setSpecialValueText(special);

    spinBox->setToolTip(toolTip);
    layout->addRow(label, spinBox);

    return spinBox;
}

QComboBox* SettingsDialog::addComboBox(QFormLayout* layout, const QString& label, const QString& toolTip, const QStringList& text, const QList< int >& data, int value)
{
    QComboBox* comboBox = new QComboBox(this);

    for(int index = 0, count = text.count(); index < count; ++index)
    {
        comboBox->addItem(text.at(index), data.at(index));
    }

    setCurrentIndexFromData(comboBox, value);

    comboBox->setToolTip(toolTip);
    layout->addRow(label, comboBox);

    return comboBox;
}

QComboBox* SettingsDialog::addDataSizeComboBox(QFormLayout* layout, const QString& label, const QString& toolTip, int initialDataSize)
{
    QComboBox* comboBox = new QComboBox(this);

    for(int dataSize = 8; dataSize <= 8192; dataSize *= 2)
    {
        comboBox->addItem(tr("%1 MB").arg(dataSize), dataSize * 1024);
    }

    int currentIndex = comboBox->findData(initialDataSize);

    if(currentIndex == -1)
    {
        currentIndex = comboBox->count();

        comboBox->addItem(tr("%1 MB").arg(initialDataSize / 1024), initialDataSize);
    }

    comboBox->setCurrentIndex(currentIndex);

    comboBox->setToolTip(toolTip);
    layout->addRow(label, comboBox);

    return comboBox;
}

QComboBox* SettingsDialog::addColorComboBox(QFormLayout* layout, const QString& label, const QString& toolTip, const QColor& color)
{
    QComboBox* comboBox = new QComboBox(this);
    comboBox->setEditable(true);
    comboBox->setInsertPolicy(QComboBox::NoInsert);
    comboBox->addItems(QColor::colorNames());

    setCurrentTextToColorName(comboBox, color);

    comboBox->setToolTip(toolTip);
    layout->addRow(label, comboBox);

    return comboBox;
}

QComboBox* SettingsDialog::addModifiersComboBox(QFormLayout* layout, const QString& label, const QString& toolTip, Qt::KeyboardModifiers modifiers)
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
    comboBox->addItem(QShortcut::tr("None"), static_cast< int >(Qt::NoModifier));

    setCurrentIndexFromKeyboardModifiers(comboBox, modifiers);

    comboBox->setToolTip(toolTip);
    layout->addRow(label, comboBox);

    return comboBox;
}

} // qpdfview
