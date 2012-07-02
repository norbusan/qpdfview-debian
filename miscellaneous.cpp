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

#include "miscellaneous.h"

TabBar::TabBar(QWidget* parent) : QTabBar(parent)
{
}

void TabBar::mousePressEvent(QMouseEvent* event)
{
    QTabBar::mousePressEvent(event);

    if(event->button() == Qt::MiddleButton)
    {
        emit tabCloseRequested(tabAt(event->pos()));
    }
}

TabWidget::TabWidget(QWidget* parent) : QTabWidget(parent),
    m_tabBarPolicy(TabBarAsNeeded)
{
    setTabBar(new TabBar(this));
}

TabWidget::TabBarPolicy TabWidget::tabBarPolicy() const
{
    return m_tabBarPolicy;
}

void TabWidget::setTabBarPolicy(TabWidget::TabBarPolicy tabBarPolicy)
{
    m_tabBarPolicy = tabBarPolicy;

    switch(m_tabBarPolicy)
    {
    case TabBarAsNeeded:
        tabBar()->setVisible(count() > 1);
        break;
    case TabBarAlwaysOn:
        tabBar()->setVisible(true);
        break;
    case TabBarAlwaysOff:
        tabBar()->setVisible(false);
        break;
    }
}

void TabWidget::tabInserted(int index)
{
    QTabWidget::tabInserted(index);

    if(m_tabBarPolicy == TabBarAsNeeded)
    {
        tabBar()->setVisible(count() > 1);
    }
}

void TabWidget::tabRemoved(int index)
{
    QTabWidget::tabRemoved(index);

    if(m_tabBarPolicy == TabBarAsNeeded)
    {
        tabBar()->setVisible(count() > 1);
    }
}

LineEdit::LineEdit(QWidget* parent) : QLineEdit(parent)
{
}

void LineEdit::mousePressEvent(QMouseEvent* event)
{
    QLineEdit::mousePressEvent(event);

    selectAll();
}

ComboBox::ComboBox(QWidget* parent) : QComboBox(parent)
{
    setLineEdit(new LineEdit(this));
}

ProgressLineEdit::ProgressLineEdit(QWidget* parent) : QLineEdit(parent),
    m_progress(0)
{
    QPalette p = palette();
    p.setColor(QPalette::Base, Qt::transparent);
    setPalette(p);
}

int ProgressLineEdit::progress() const
{
    return m_progress;
}

void ProgressLineEdit::setProgress(int progress)
{
    if(m_progress != progress && progress >= 0 && progress <= 100)
    {
        m_progress = progress;

        update();
    }
}

void ProgressLineEdit::paintEvent(QPaintEvent* event)
{
    QPainter painter;
    painter.begin(this);

    QRect r1 = rect();
    r1.setWidth(m_progress * width() / 100);

    painter.fillRect(r1, QApplication::palette().highlight());

    QRect r2 = rect();
    r2.setLeft(m_progress * width() / 100);
    r2.setWidth((100 - m_progress) * width() / 100);

    painter.fillRect(r2, QApplication::palette().base());

    painter.end();

    QLineEdit::paintEvent(event);
}

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent)
{
    m_settings = new QSettings(this);

    m_dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    connect(m_dialogButtonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(m_dialogButtonBox, SIGNAL(rejected()), SLOT(reject()));

    m_formLayout = new QFormLayout(this);
    setLayout(m_formLayout);

    // tab position

    m_tabPositionComboBox = new QComboBox(this);
    m_tabPositionComboBox->addItem(tr("Top"), static_cast< uint >(QTabWidget::North));
    m_tabPositionComboBox->addItem(tr("Bottom"), static_cast< uint >(QTabWidget::South));
    m_tabPositionComboBox->addItem(tr("Left"), static_cast< uint >(QTabWidget::West));
    m_tabPositionComboBox->addItem(tr("Right"), static_cast< uint >(QTabWidget::East));

    uint tabPosition = static_cast< uint >(m_settings->value("mainWindow/tabPosition", 0).toUInt());

    for(int index = 0; index < m_tabPositionComboBox->count(); index++)
    {
        if(m_tabPositionComboBox->itemData(index).toUInt() == tabPosition)
        {
            m_tabPositionComboBox->setCurrentIndex(index);
        }
    }

    // tab visibility

    m_tabVisibilityComboBox = new QComboBox(this);
    m_tabVisibilityComboBox->addItem(tr("As needed"), static_cast< uint >(TabWidget::TabBarAsNeeded));
    m_tabVisibilityComboBox->addItem(tr("Always"), static_cast< uint >(TabWidget::TabBarAlwaysOn));
    m_tabVisibilityComboBox->addItem(tr("Never"), static_cast< uint >(TabWidget::TabBarAlwaysOff));

    uint tabBarPolicy = static_cast< uint >(m_settings->value("mainWindow/tabVisibility", 0).toUInt());

    for(int index = 0; index < m_tabVisibilityComboBox->count(); index++)
    {
        if(m_tabVisibilityComboBox->itemData(index).toUInt() == tabBarPolicy)
        {
            m_tabVisibilityComboBox->setCurrentIndex(index);
        }
    }

    // open URL

    m_openUrlCheckBox = new QCheckBox(this);
    m_openUrlCheckBox->setChecked(m_settings->value("documentView/openUrl", false).toBool());

    // auto-refresh

    m_autoRefreshCheckBox = new QCheckBox(this);
    m_autoRefreshCheckBox->setChecked(m_settings->value("documentView/autoRefresh", false).toBool());

    // restore tabs

    m_restoreTabsCheckBox = new QCheckBox(this);
    m_restoreTabsCheckBox->setChecked(m_settings->value("mainWindow/restoreTabs", false).toBool());

    // restore bookmarks

    m_restoreBookmarksCheckBox = new QCheckBox(this);
    m_restoreBookmarksCheckBox->setChecked(m_settings->value("mainWindow/restoreBookmarks", false).toBool());

    // decorate pages

    m_decoratePagesCheckBox = new QCheckBox(this);
    m_decoratePagesCheckBox->setChecked(m_settings->value("pageItem/decoratePages", true).toBool());

    // decorate links

    m_decorateLinksCheckBox = new QCheckBox(this);
    m_decorateLinksCheckBox->setChecked(m_settings->value("pageItem/decorateLinks", true).toBool());

    // page spacing

    m_pageSpacingSpinBox = new QDoubleSpinBox(this);
    m_pageSpacingSpinBox->setRange(0.0, 25.0);
    m_pageSpacingSpinBox->setSingleStep(0.25);
    m_pageSpacingSpinBox->setValue(m_settings->value("documentView/pageSpacing", 5.0).toDouble());

    // thumbnail spacing

    m_thumbnailSpacingSpinBox = new QDoubleSpinBox(this);
    m_thumbnailSpacingSpinBox->setRange(0.0, 25.0);
    m_thumbnailSpacingSpinBox->setSingleStep(0.25);
    m_thumbnailSpacingSpinBox->setValue(m_settings->value("documentView/thumbnailSpacing", 3.0).toDouble());

    // thumbnail size

    m_thumbnailSizeSpinBox = new QDoubleSpinBox(this);
    m_thumbnailSizeSpinBox->setRange(30.0, 300.0);
    m_thumbnailSizeSpinBox->setSingleStep(10.0);
    m_thumbnailSizeSpinBox->setValue(m_settings->value("documentView/thumbnailSize", 150.0).toDouble());

    // antialiasing

    m_antialiasingCheckBox = new QCheckBox(this);
    m_antialiasingCheckBox->setChecked(m_settings->value("documentView/antialiasing", true).toBool());

    // text antialising

    m_textAntialiasingCheckBox = new QCheckBox(this);
    m_textAntialiasingCheckBox->setChecked(m_settings->value("documentView/textAntialiasing", true).toBool());

    // text hinting

    m_textHintingCheckBox = new QCheckBox(this);
    m_textHintingCheckBox->setChecked(m_settings->value("documentView/textHinting", false).toBool());

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

    for(int index = 0; index < m_cacheSizeComboBox->count(); index++)
    {
        if(m_cacheSizeComboBox->itemData(index).toInt() == cacheSize)
        {
            m_cacheSizeComboBox->setCurrentIndex(index);
        }
    }

    // prefetch

    m_prefetchCheckBox = new QCheckBox(this);
    m_prefetchCheckBox->setChecked(m_settings->value("documentView/prefetch", false).toBool());

    m_formLayout->addRow(tr("Tab position:"), m_tabPositionComboBox);
    m_formLayout->addRow(tr("Tab visibility:"), m_tabVisibilityComboBox);

    m_formLayout->addRow(tr("Open URL:"), m_openUrlCheckBox);

    m_formLayout->addRow(tr("Auto-refresh:"), m_autoRefreshCheckBox);

    m_formLayout->addRow(tr("Restore tabs:"), m_restoreTabsCheckBox);
    m_formLayout->addRow(tr("Restore bookmarks:"), m_restoreBookmarksCheckBox);

    m_formLayout->addRow(tr("Decorate pages:"), m_decoratePagesCheckBox);
    m_formLayout->addRow(tr("Decorate links:"), m_decorateLinksCheckBox);

    m_formLayout->addRow(tr("Page spacing:"), m_pageSpacingSpinBox);
    m_formLayout->addRow(tr("Thumbnail spacing:"), m_thumbnailSpacingSpinBox);

    m_formLayout->addRow(tr("Thumbnail size:"), m_thumbnailSizeSpinBox);

    m_formLayout->addRow(tr("Antialiasing:"), m_antialiasingCheckBox);
    m_formLayout->addRow(tr("Text antialiasing:"), m_textAntialiasingCheckBox);
    m_formLayout->addRow(tr("Text hinting:"), m_textHintingCheckBox);

    m_formLayout->addRow(tr("Cache size:"), m_cacheSizeComboBox);

    m_formLayout->addRow(tr("Prefetch:"), m_prefetchCheckBox);

    m_formLayout->addRow(m_dialogButtonBox);
}

void SettingsDialog::accept()
{
    m_settings->setValue("mainWindow/tabPosition", m_tabPositionComboBox->itemData(m_tabPositionComboBox->currentIndex()));
    m_settings->setValue("mainWindow/tabVisibility", m_tabVisibilityComboBox->itemData(m_tabVisibilityComboBox->currentIndex()));

    m_settings->setValue("documentView/openUrl", m_openUrlCheckBox->isChecked());

    m_settings->setValue("documentView/autoRefresh", m_autoRefreshCheckBox->isChecked());

    m_settings->setValue("mainWindow/restoreTabs", m_restoreTabsCheckBox->isChecked());
    m_settings->setValue("mainWindow/restoreBookmarks", m_restoreBookmarksCheckBox->isChecked());

    m_settings->setValue("pageItem/decoratePages", m_decoratePagesCheckBox->isChecked());
    m_settings->setValue("pageItem/decorateLinks", m_decorateLinksCheckBox->isChecked());

    m_settings->setValue("documentView/pageSpacing", m_pageSpacingSpinBox->value());
    m_settings->setValue("documentView/thumbnailSpacing", m_thumbnailSpacingSpinBox->value());

    m_settings->setValue("documentView/thumbnailSize", m_thumbnailSizeSpinBox->value());

    m_settings->setValue("documentView/antialiasing", m_antialiasingCheckBox->isChecked());
    m_settings->setValue("documentView/textAntialiasing", m_textAntialiasingCheckBox->isChecked());
    m_settings->setValue("documentView/textHinting", m_textHintingCheckBox->isChecked());

    m_settings->setValue("pageItem/cacheSize", m_cacheSizeComboBox->itemData(m_cacheSizeComboBox->currentIndex()));

    m_settings->setValue("documentView/prefetch", m_prefetchCheckBox->isChecked());

    QDialog::accept();
}


Bookmark::Bookmark(const QString& filePath, QWidget* parent) : QMenu(parent)
{
    QFileInfo fileInfo(filePath);

    menuAction()->setText(fileInfo.completeBaseName());
    menuAction()->setToolTip(fileInfo.absoluteFilePath());

    menuAction()->setData(filePath);

    m_removeBookmarkAction = new QAction(tr("&Remove bookmark"), this);
    connect(m_removeBookmarkAction, SIGNAL(triggered()), SLOT(on_removeBookmark_triggered()));

    m_openAction = new QAction(tr("&Open"), this);
    connect(m_openAction, SIGNAL(triggered()), SLOT(on_open_triggered()));

    m_openInNewTabAction = new QAction(tr("Open in new &tab"), this);
    connect(m_openInNewTabAction, SIGNAL(triggered()), SLOT(on_openInNewTab_triggered()));

    addAction(m_removeBookmarkAction);
    addSeparator();
    addAction(m_openAction);
    addAction(m_openInNewTabAction);

    m_jumpToPageGroup = new QActionGroup(this);
    connect(m_jumpToPageGroup, SIGNAL(triggered(QAction*)), SLOT(on_jumpToPage_triggered(QAction*)));
}

void Bookmark::addJumpToPage(int page)
{
    QAction* before = 0;

    foreach(QAction* action, m_jumpToPageGroup->actions())
    {
        if(action->data().toInt() == page)
        {
            return;
        }
        else if(action->data().toInt() > page)
        {
            before = action;

            break;
        }
    }

    QAction* action = new QAction(tr("Jump to page %1").arg(page), this);
    action->setData(page);

    insertAction(before, action);
    m_jumpToPageGroup->addAction(action);
}

QString Bookmark::filePath() const
{
    return menuAction()->data().toString();
}

QList< int > Bookmark::pages() const
{
    QList< int > pages;

    foreach(QAction* action, m_jumpToPageGroup->actions())
    {
        pages.append(action->data().toInt());
    }

    return pages;
}

void Bookmark::on_removeBookmark_triggered()
{
    deleteLater();
}

void Bookmark::on_open_triggered()
{
    emit openTriggered(filePath());
}

void Bookmark::on_openInNewTab_triggered()
{
    emit openInNewTabTriggered(filePath());
}

void Bookmark::on_jumpToPage_triggered(QAction* action)
{
    emit jumpToPageTriggered(filePath(), action->data().toInt());
}
