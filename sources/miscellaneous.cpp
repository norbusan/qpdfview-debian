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

void AuxiliaryView::attachTo(DocumentView *view)
{
    m_view = view;

    if(view)
    {
        connect(m_view->model(), SIGNAL(filePathChanged(QString)), this, SLOT(updateContent()));
    }

    this->updateContent();
}

void AuxiliaryView::updateContent()
{
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
        if(nextNode->isOpen)
        {
            tree->expandItem(item);
        }
        item->setData(0, Qt::UserRole, nextNode->destinationName);

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

    m_outline = 0;
}

OutlineView::~OutlineView()
{
    if(m_outline)
    {
        delete m_outline;
    }
}

void OutlineView::updateContent()
{
    m_treeWidget->clear();

    if(m_view)
    {
        if(m_outline)
        {
            delete m_outline;
        }

        m_outline = m_view->model()->outline();

        if(m_outline)
        {
            outlineToTree(m_outline, m_treeWidget, 0);
        }
    }
}

void OutlineView::followLink(QTreeWidgetItem *item, int column)
{
    QString destinationName = item->data(column, Qt::UserRole).toString();

    int pageNumber = m_view->model()->destination(destinationName);

    if(pageNumber != -1)
    {
        m_view->setCurrentPage(pageNumber);
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

void ThumbnailsView::updateContent()
{
    m_listWidget->clear();

    if(m_view)
    {
        QListWidgetItem *item = 0;
        int itemWidth = 0.0, itemHeight = 0.0;

        for(int index = 0; index < m_view->model()->pageCount(); index++)
        {
            QImage thumbnail = m_view->model()->thumbnail(index);

            if(!thumbnail.isNull())
            {
                item = new QListWidgetItem(m_listWidget);
                item->setIcon(QIcon(QPixmap::fromImage(thumbnail)));
                item->setText(tr("%1").arg(index+1));
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
        m_view->setCurrentPage(pageNumber);
    }
}

// settings dialog

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent),
    m_settings()
{
    m_layout = new QFormLayout(this);
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);

    m_pageCacheSizeLineEdit = new QLineEdit(this);
    m_pageCacheSizeValidator = new QIntValidator(this);

    m_pageCacheSizeLineEdit->setValidator(m_pageCacheSizeValidator);

    connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    m_layout->addRow(tr("Page cache &size:"), m_pageCacheSizeLineEdit);
    m_layout->addRow(m_buttonBox);

    this->setLayout(m_layout);

    m_pageCacheSizeLineEdit->setText(m_settings.value("pageObject/pageCacheSize", 134217728).toString());
}

void SettingsDialog::accept()
{
    m_settings.setValue("pageObject/pageCacheSize", m_pageCacheSizeLineEdit->text().toUInt());

    DocumentModel::setPageCacheSize(m_pageCacheSizeLineEdit->text().toUInt());

    QDialog::accept();
}

