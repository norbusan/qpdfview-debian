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

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QDoubleSpinBox;
class QFormLayout;
class QLineEdit;
class QSpinBox;
class QTableView;
class QTabWidget;

class Settings;
class SettingsWidget;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    SettingsDialog(QWidget* parent = 0);

public slots:
    void accept();
    void reset();
    void resetCurrentTab();

private:
    Q_DISABLE_COPY(SettingsDialog)

    static Settings* s_settings;

    QTabWidget* m_graphicsTabWidget;
    QFormLayout* m_graphicsLayout;

#ifdef WITH_PDF

    SettingsWidget* m_pdfSettingsWidget;

#endif // WITH_PDF

#ifdef WITH_PS

    SettingsWidget* m_psSettingsWidget;

#endif // WITH_PS

    QTableView* m_shortcutsTableView;

    QTabWidget* m_tabWidget;
    QDialogButtonBox* m_dialogButtonBox;
    QPushButton* m_defaultsButton;
    QPushButton* m_defaultsOnCurrentTabButton;

    QFormLayout* m_behaviorLayout;
    QFormLayout* m_interfaceLayout;
    QFormLayout* m_modifiersLayout;

    // behavior

    QCheckBox* m_openUrlCheckBox;

    QCheckBox* m_autoRefreshCheckBox;

    QCheckBox* m_trackRecentlyUsedCheckBox;

    QCheckBox* m_restoreTabsCheckBox;
    QCheckBox* m_restoreBookmarksCheckBox;
    QCheckBox* m_restorePerFileSettingsCheckBox;

    QCheckBox* m_presentationSyncCheckBox;
    QSpinBox* m_presentationScreenSpinBox;

    QSpinBox* m_highlightDurationSpinBox;
    QComboBox* m_highlightColorComboBox;
    QComboBox* m_annotationColorComboBox;

    QLineEdit* m_sourceEditorLineEdit;

    void createBehaviorTab();
    void resetBehaviorTab();

    // graphics

    QCheckBox* m_keepObsoletePixmapsCheckBox;

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

    QCheckBox* m_useDevicePixelRatioCheckBox;

#endif // QT_VERSION

    QCheckBox* m_decoratePagesCheckBox;
    QCheckBox* m_decorateLinksCheckBox;
    QCheckBox* m_decorateFormFieldsCheckBox;

    QComboBox* m_backgroundColorComboBox;
    QComboBox* m_paperColorComboBox;
    QComboBox* m_presentationBackgroundColorComboBox;

    QSpinBox* m_pagesPerRowSpinBox;

    QDoubleSpinBox* m_pageSpacingSpinBox;
    QDoubleSpinBox* m_thumbnailSpacingSpinBox;

    QDoubleSpinBox* m_thumbnailSizeSpinBox;

    QComboBox* m_cacheSizeComboBox;
    QCheckBox* m_prefetchCheckBox;
    QSpinBox* m_prefetchDistanceSpinBox;

    void createGraphicsTab();
    void resetGraphicsTab();

    // interface

    QComboBox* m_tabPositionComboBox;
    QComboBox* m_tabVisibilityComboBox;

    QCheckBox* m_newTabNextToCurrentTabCheckBox;

    QSpinBox* m_recentlyUsedCountSpinBox;

    QLineEdit* m_fileToolBarLineEdit;
    QLineEdit* m_editToolBarLineEdit;
    QLineEdit* m_viewToolBarLineEdit;

    QCheckBox* m_currentPageInWindowTitleCheckBox;
    QCheckBox* m_instanceNameInWindowTitleCheckBox;

    QCheckBox* m_synchronizeOutlineViewCheckBox;

    QCheckBox* m_highlightCurrentThumbnailCheckBox;
    QCheckBox* m_limitThumbnailsToResultsCheckBox;

    QCheckBox* m_annotationOverlayCheckBox;
    QCheckBox* m_formFieldOverlayCheckBox;

    void createInterfaceTab();
    void resetInterfaceTab();

    // modifiers

    QComboBox* m_zoomModifiersComboBox;
    QComboBox* m_rotateModifiersComboBox;
    QComboBox* m_scrollModifiersComboBox;

    QComboBox* m_copyToClipboardModifiersComboBox;
    QComboBox* m_addAnnotationModifiersComboBox;

    void createModifiersTab();
    void resetModifiersTab();

    // helper methods

    void createColorComboBox(QComboBox*& comboBox, const QColor& color);
    void createModifiersComboBox(QComboBox*& comboBox, const Qt::KeyboardModifiers& modifiers);

};

#endif // SETTINGSDIALOG_H
