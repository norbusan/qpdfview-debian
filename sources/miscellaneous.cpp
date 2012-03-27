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
        item->setData(0, Qt::UserRole+1, nextNode->top);

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

    connect(m_treeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(followLink(QTreeWidgetItem*,int)));
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
    qreal top = item->data(column, Qt::UserRole+1).toReal();

    if(pageNumber != -1)
    {
        this->view()->setCurrentPage(pageNumber, top);
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

    connect(m_listWidget, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(followLink(QListWidgetItem*)));
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
    connect(m_actionGroup, SIGNAL(triggered(QAction*)), this, SLOT(selectEntry(QAction*)));

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

void RecentlyUsedAction::selectEntry(QAction *action)
{
    emit entrySelected(action->data().toString());
}

void RecentlyUsedAction::clearList()
{
    foreach(QAction *action, m_actionGroup->actions())
    {
        m_actionGroup->removeAction(action);
        m_menu->removeAction(action);
    }
}

void RecentlyUsedAction::addEntry(const QString &filePath)
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

// bookmarks menu

BookmarksMenu::BookmarksMenu(QWidget *parent) : QMenu(tr("Bookmarks"), parent),
    m_currentPage(1),
    m_top(0.0)
{
    m_actionGroup = new QActionGroup(this);
    connect(m_actionGroup, SIGNAL(triggered(QAction*)), this, SLOT(selectEntry(QAction*)));

    m_addEntryAction = new QAction(tr("Add entry"), this);
    m_addEntryAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_B));
    connect(m_addEntryAction, SIGNAL(triggered()), this, SLOT(addEntry()));

    m_selectPreviousEntryAction = new QAction(tr("Go to previous"), this);
    m_selectPreviousEntryAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_PageUp));
    connect(m_selectPreviousEntryAction, SIGNAL(triggered()), this, SLOT(selectPreviousEntry()));

    m_selectNextEntryAction = new QAction(tr("Go to next"), this);
    m_selectNextEntryAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_PageDown));
    connect(m_selectNextEntryAction, SIGNAL(triggered()), this, SLOT(selectNextEntry()));

    m_clearListAction = new QAction(tr("Clear list"), this);
    connect(m_clearListAction, SIGNAL(triggered()), this, SLOT(clearList()));

    this->addAction(m_addEntryAction);
    this->addAction(m_selectPreviousEntryAction);
    this->addAction(m_selectNextEntryAction);
    this->addAction(m_clearListAction);
    this->addSeparator();
}

void BookmarksMenu::selectEntry(QAction *action)
{
    qreal pos = action->data().toReal();

    int currentPage = qFloor(pos);
    double top = pos - qFloor(pos);

    emit entrySelected(currentPage, top);
}

void BookmarksMenu::addEntry()
{
    qreal pos = m_currentPage + m_top;
    bool addItem = true;
    QAction *before = 0;

    foreach(QAction *action, m_actionGroup->actions())
    {
        if(action->data().toReal() == pos)
        {
            addItem = false;

            break;
        }
        else if(action->data().toReal() > pos)
        {
            if(before)
            {
                before = before->data().toReal() < action->data().toReal() ? before : action;
            }
            else
            {
                before = action;
            }
        }
    }

    if(addItem)
    {
        QAction *action = new QAction(this);
        action->setText(tr("Page %1 at %2%").arg(m_currentPage).arg(qFloor(100.0 * m_top)));
        action->setData(pos);

        m_actionGroup->addAction(action);
        this->insertAction(before, action);
    }

    if(m_actionGroup->actions().size() > 10)
    {
        QAction *first = m_actionGroup->actions().first();

        m_actionGroup->removeAction(first);
        this->removeAction(first);
    }
}

void BookmarksMenu::selectPreviousEntry()
{
    qreal pos = m_currentPage + m_top;
    QAction *previous = 0;

    foreach(QAction *action, m_actionGroup->actions())
    {
        if(action->data().toReal() < pos)
        {
            if(previous)
            {
                previous = previous->data().toReal() > action->data().toReal() ? previous : action;
            }
            else
            {
                previous = action;
            }
        }
    }

    if(previous)
    {
        previous->trigger();
    }
}

void BookmarksMenu::selectNextEntry()
{
    qreal pos = m_currentPage + m_top;
    QAction *next = 0;

    foreach(QAction *action, m_actionGroup->actions())
    {
        if(action->data().toReal() > pos)
        {
            if(next)
            {
                next = next->data().toReal() < action->data().toReal() ? next : action;
            }
            else
            {
                next = action;
            }
        }
    }

    if(next)
    {
        next->trigger();
    }
}

void BookmarksMenu::clearList()
{
    foreach(QAction *action, m_actionGroup->actions())
    {
        m_actionGroup->removeAction(action);
        this->removeAction(action);
    }
}

void BookmarksMenu::updateCurrrentPage(int currentPage, qreal top)
{
    m_currentPage = currentPage;
    m_top = top;
}

// settings dialog

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
    m_layout = new QFormLayout(this);
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);

    m_watchFilePathCheckBox = new QCheckBox(this);
    m_openUrlLinksCheckBox = new QCheckBox(this);
    m_openExternalLinksCheckBox = new QCheckBox(this);

    m_antialiasingCheckBox = new QCheckBox(this);
    m_textAntialiasingCheckBox = new QCheckBox(this);
    m_textHintingCheckBox = new QCheckBox(this);

    m_pageCacheSizeComboBox = new QComboBox(this);
    m_pageCacheSizeComboBox->addItem(tr("%1 MB").arg(32), QVariant(33554432u));
    m_pageCacheSizeComboBox->addItem(tr("%1 MB").arg(64), QVariant(67108864u));
    m_pageCacheSizeComboBox->addItem(tr("%1 MB").arg(128), QVariant(134217728u));
    m_pageCacheSizeComboBox->addItem(tr("%1 MB").arg(256), QVariant(268435456u));
    m_pageCacheSizeComboBox->addItem(tr("%1 MB").arg(512), QVariant(536870912u));
    m_pageCacheSizeComboBox->addItem(tr("%1 MB").arg(1024), QVariant(1073741824u));
    m_pageCacheSizeComboBox->addItem(tr("%1 MB").arg(2048), QVariant(2147483648u));

    connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    m_layout->addRow(tr("&Refresh automatically:"), m_watchFilePathCheckBox);
    m_layout->addRow(tr("Open links to &URL:"), m_openUrlLinksCheckBox);
    m_layout->addRow(tr("Open external links"), m_openExternalLinksCheckBox);
    m_layout->addRow(tr("&Antialiasing:"), m_antialiasingCheckBox);
    m_layout->addRow(tr("&Text antialiasing:"), m_textAntialiasingCheckBox);
    m_layout->addRow(tr("Text &hinting:"), m_textHintingCheckBox);
    m_layout->addRow(tr("Page cache &size:"), m_pageCacheSizeComboBox);
    m_layout->addRow(m_buttonBox);

    this->setLayout(m_layout);

    m_watchFilePathCheckBox->setChecked(DocumentModel::watchFilePath());
    m_openUrlLinksCheckBox->setChecked(DocumentModel::openUrlLinks());
    m_openExternalLinksCheckBox->setChecked(DocumentModel::openExternalLinks());

    m_antialiasingCheckBox->setChecked(DocumentModel::antialiasing());
    m_textAntialiasingCheckBox->setChecked(DocumentModel::textAntialiasing());
    m_textHintingCheckBox->setChecked(DocumentModel::textHinting());

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
    DocumentModel::setWatchFilePath(m_watchFilePathCheckBox->isChecked());
    DocumentModel::setOpenUrlLinks(m_openUrlLinksCheckBox->isChecked());
    DocumentModel::setOpenExternalLinks(m_openExternalLinksCheckBox->isChecked());

    DocumentModel::setAntialiasing(m_antialiasingCheckBox->isChecked());
    DocumentModel::setTextAntialiasing(m_textAntialiasingCheckBox->isChecked());
    DocumentModel::setTextHinting(m_textHintingCheckBox->isChecked());

    DocumentModel::setMaximumPageCacheSize(m_pageCacheSizeComboBox->itemData(m_pageCacheSizeComboBox->currentIndex()).toUInt());

    QDialog::accept();
}

