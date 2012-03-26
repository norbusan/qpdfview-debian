/*

Copyright 2012 Adam Reichold

This file is part of qpdfview.

qpdfview is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

qpdfview is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with qpdfview.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "miscellaneous.h"

// auxiliary view

AuxiliaryView::AuxiliaryView(QWidget *parent) : QWidget(parent)
{
    m_view = 0;
}

DocumentView *AuxiliaryView::view() const
{
    return m_view;
}

void AuxiliaryView::setView(DocumentView *view)
{
    m_view = view;

    if(m_view)
    {
        connect(m_view->model(), SIGNAL(filePathChanged(QString, bool)), this, SLOT(updateModel()));
    }

    if(this->isVisible())
    {
        this->updateModel();
    }
}

void AuxiliaryView::updateVisibility(bool visible)
{
    if(visible)
    {
        this->updateModel();
    }
}

// outline view

static void outlineToTree(DocumentModel::Outline *node, QTreeWidget *tree, QTreeWidgetItem *parentItem)
{
    QTreeWidgetItem *item = 0;

    for(DocumentModel::Outline *nextNode = node; nextNode; nextNode = nextNode->sibling)
    {
        if(parentItem)
        {
            item = new QTreeWidgetItem(parentItem);
        }
        else
        {
            item = new QTreeWidgetItem(tree, item);
        }

        item->setText(0, nextNode->text);
        item->setData(0, Qt::UserRole, nextNode->pageNumber);

        if(nextNode->isOpen)
        {
            tree->expandItem(item);
        }

        if(nextNode->child)
        {
            outlineToTree(nextNode->child, tree, item);
        }
    }
}

OutlineView::OutlineView(QWidget *parent) : AuxiliaryView(parent)
{
    m_treeWidget = new QTreeWidget(this);

    m_treeWidget->setAlternatingRowColors(true);
    m_treeWidget->header()->setVisible(false);

    this->setLayout(new QHBoxLayout());
    this->layout()->addWidget(m_treeWidget);

    connect(m_treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(followLink(QTreeWidgetItem*,int)));
}

void OutlineView::updateModel()
{
    m_treeWidget->clear();

    if(this->view())
    {
        DocumentModel::Outline *outline = this->view()->model()->outline();

        if(outline)
        {
            outlineToTree(outline, m_treeWidget, 0);

            delete outline;
        }
    }
}

void OutlineView::followLink(QTreeWidgetItem *item, int column)
{
    int pageNumber = item->data(column, Qt::UserRole).toInt();

    if(pageNumber != -1)
    {
        this->view()->setCurrentPage(pageNumber);
    }
}

// thumbnails view

ThumbnailsView::ThumbnailsView(QWidget *parent) : AuxiliaryView(parent)
{
    m_listWidget = new QListWidget();

    m_listWidget->setViewMode(QListView::IconMode);
    m_listWidget->setResizeMode(QListView::Adjust);
    m_listWidget->setMovement(QListView::Static);

    this->setLayout(new QHBoxLayout());
    this->layout()->addWidget(m_listWidget);

    connect(m_listWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(followLink(QListWidgetItem*)));
}

void ThumbnailsView::updateModel()
{
    m_listWidget->clear();

    if(this->view())
    {
        QListWidgetItem *item = 0;
        int itemWidth = 0.0, itemHeight = 0.0;

        for(int index = 0; index < this->view()->model()->pageCount(); index++)
        {
            QImage thumbnail = this->view()->model()->thumbnail(index);

            if(!thumbnail.isNull())
            {
                item = new QListWidgetItem(m_listWidget);
                item->setText(tr("%1").arg(index+1));
                item->setIcon(QIcon(QPixmap::fromImage(thumbnail)));
                item->setData(Qt::UserRole, index+1);

                itemWidth = qMax(itemWidth, thumbnail.width());
                itemHeight = qMax(itemHeight, thumbnail.height());
            }
        }

        m_listWidget->setIconSize(QSize(itemWidth, itemHeight));
    }
}

void ThumbnailsView::followLink(QListWidgetItem *item)
{
    int pageNumber = item->data(Qt::UserRole).toInt();

    if(pageNumber != -1)
    {
        this->view()->setCurrentPage(pageNumber);
    }
}

// recently used action

RecentlyUsedAction::RecentlyUsedAction(QObject *parent) : QAction(tr("Recently &used"), parent),
    m_settings()
{
    m_menu = new QMenu();
    this->setMenu(m_menu);

    m_actionGroup = new QActionGroup(this);
    connect(m_actionGroup, SIGNAL(triggered(QAction*)), this, SLOT(selectFilePath(QAction*)));

    m_clearListAction = new QAction(tr("Clear list"), this);
    connect(m_clearListAction, SIGNAL(triggered()), this, SLOT(clearList()));

    m_separator = m_menu->addSeparator();
    m_menu->addAction(m_clearListAction);

    QStringList filePaths = m_settings.value("mainWindow/recentlyUsed").toStringList();

    foreach(QString filePath, filePaths)
    {
        QAction *action = new QAction(this);
        action->setText(filePath);
        action->setData(filePath);

        m_actionGroup->addAction(action);
        m_menu->insertAction(m_separator, action);
    }
}

RecentlyUsedAction::~RecentlyUsedAction()
{
    QStringList filePaths;

    foreach(QAction *action, m_actionGroup->actions())
    {
        filePaths.append(action->data().toString());
    }

    m_settings.setValue("mainWindow/recentlyUsed", filePaths);

    delete m_menu;
}

void RecentlyUsedAction::selectFilePath(QAction *action)
{
    emit filePathSelected(action->data().toString());
}

void RecentlyUsedAction::clearList()
{
    foreach(QAction *action, m_actionGroup->actions())
    {
        m_actionGroup->removeAction(action);
        m_menu->removeAction(action);
    }
}

void RecentlyUsedAction::addFilePath(const QString &filePath)
{
    bool addItem = true;

    foreach(QAction *action, m_actionGroup->actions())
    {
        addItem = addItem && action->data().toString() != filePath;
    }

    if(addItem)
    {
        QAction *action = new QAction(this);
        action->setText(filePath);
        action->setData(filePath);

        m_actionGroup->addAction(action);
        m_menu->insertAction(m_separator, action);
    }

    if(m_actionGroup->actions().size() > 5)
    {
        QAction *first = m_actionGroup->actions().first();

        m_actionGroup->removeAction(first);
        m_menu->removeAction(first);
    }
}

// settings dialog

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent),
    m_settings()
{
    m_layout = new QFormLayout(this);
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);

    m_refreshAutomaticallyCheckBox = new QCheckBox(this);

    m_antialiasingCheckBox = new QCheckBox(this);
    m_textAntialiasingCheckBox = new QCheckBox(this);

    m_pageCacheSizeComboBox = new QComboBox(this);
    m_pageCacheSizeComboBox->addItem(tr("32 MB"), QVariant(33554432u));
    m_pageCacheSizeComboBox->addItem(tr("64 MB"), QVariant(67108864u));
    m_pageCacheSizeComboBox->addItem(tr("128 MB"), QVariant(134217728u));
    m_pageCacheSizeComboBox->addItem(tr("256 MB"), QVariant(268435456u));
    m_pageCacheSizeComboBox->addItem(tr("512 MB"), QVariant(536870912u));
    m_pageCacheSizeComboBox->addItem(tr("1024 MB"), QVariant(1073741824u));
    m_pageCacheSizeComboBox->addItem(tr("2048 MB"), QVariant(2147483648u));

    connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    m_layout->addRow(tr("&Refresh automatically:"), m_refreshAutomaticallyCheckBox);
    m_layout->addRow(tr("&Antialiasing:"), m_antialiasingCheckBox);
    m_layout->addRow(tr("&Text antialiasing:"), m_textAntialiasingCheckBox);
    m_layout->addRow(tr("Page cache &size:"), m_pageCacheSizeComboBox);
    m_layout->addRow(m_buttonBox);

    this->setLayout(m_layout);

    m_refreshAutomaticallyCheckBox->setChecked(DocumentModel::watchFilePath());

    m_antialiasingCheckBox->setChecked(DocumentModel::antialiasing());
    m_textAntialiasingCheckBox->setChecked(DocumentModel::textAntialiasing());

    // maximumPageCacheSize

    bool addItem = true;

    for(int index = 0; index < m_pageCacheSizeComboBox->count(); index++)
    {
        uint itemData = m_pageCacheSizeComboBox->itemData(index).toUInt();

        if(itemData == DocumentModel::maximumPageCacheSize())
        {
            addItem = false;

            m_pageCacheSizeComboBox->setCurrentIndex(index);
        }
    }

    if(addItem)
    {
        m_pageCacheSizeComboBox->addItem(tr("%1 MB").arg(DocumentModel::maximumPageCacheSize() / 1024 / 1024), DocumentModel::maximumPageCacheSize());

        m_pageCacheSizeComboBox->setCurrentIndex(m_pageCacheSizeComboBox->count()-1);
    }
}

void SettingsDialog::accept()
{
    DocumentModel::setWatchFilePath(m_refreshAutomaticallyCheckBox->isChecked());

    DocumentModel::setAntialiasing(m_antialiasingCheckBox->isChecked());
    DocumentModel::setTextAntialiasing(m_textAntialiasingCheckBox->isChecked());

    DocumentModel::setMaximumPageCacheSize(m_pageCacheSizeComboBox->itemData(m_pageCacheSizeComboBox->currentIndex()).toUInt());

    QDialog::accept();
}

