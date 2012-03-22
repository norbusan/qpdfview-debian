#include "miscellaneous.h"

// outline

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

OutlineView::OutlineView(QWidget *parent) : QWidget(parent)
{
    m_treeWidget = new QTreeWidget(this);

    m_treeWidget->setAlternatingRowColors(true);
    m_treeWidget->header()->setVisible(false);

    this->setLayout(new QHBoxLayout());
    this->layout()->addWidget(m_treeWidget);

    connect(m_treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(followLink(QTreeWidgetItem*,int)));

    m_view = 0;
    m_outline = 0;
}

OutlineView::~OutlineView()
{
    if(m_outline)
    {
        delete m_outline;
    }
}

void OutlineView::attachView(DocumentView *view)
{
    m_view = view;

    if(view)
    {
        connect(m_view->model(), SIGNAL(filePathChanged(QString)), this, SLOT(updateView()));
    }

    this->updateView();
}

void OutlineView::updateView()
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

// thumbnails

ThumbnailsView::ThumbnailsView(QWidget *parent) : QWidget(parent)
{
    m_listWidget = new QListWidget();

    m_listWidget->setViewMode(QListView::IconMode);
    m_listWidget->setResizeMode(QListView::Adjust);
    m_listWidget->setMovement(QListView::Static);

    this->setLayout(new QHBoxLayout());
    this->layout()->addWidget(m_listWidget);

    connect(m_listWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(followLink(QListWidgetItem*)));

    m_view = 0;
}

void ThumbnailsView::attachView(DocumentView *view)
{
    m_view = view;

    if(view)
    {
        connect(m_view->model(), SIGNAL(filePathChanged(QString)), this, SLOT(updateView()));
    }

    this->updateView();
}

void ThumbnailsView::updateView()
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

// settings

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

