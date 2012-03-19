#include "thumbnailsview.h"

ThumbnailsView::ThumbnailsView(QWidget *parent) : QDockWidget(parent),
    m_documentView(0)
{
    this->setWindowTitle(tr("&Thumbnails"));
    this->setObjectName("thumbnailsView");

    this->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    this->setFeatures(QDockWidget::AllDockWidgetFeatures);

    m_listWidget = new QListWidget();
    this->setWidget(m_listWidget);

    connect(m_listWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(followLink(QListWidgetItem*)));

    m_listWidget->setViewMode(QListView::IconMode);
    m_listWidget->setResizeMode(QListView::Adjust);
    m_listWidget->setMovement(QListView::Static);
}

ThumbnailsView::~ThumbnailsView()
{
    delete m_listWidget;
}

void ThumbnailsView::setDocumentView(DocumentView *documentView)
{
    if(m_documentView != documentView)
    {
        m_documentView = documentView;

        if(m_documentView)
        {
            connect(m_documentView, SIGNAL(filePathChanged(QString)), this, SLOT(updateThumbnails()));
        }

        this->updateThumbnails();
    }
}


void ThumbnailsView::updateThumbnails()
{
    m_listWidget->clear();

    if(m_documentView)
    {
        QListWidgetItem *item = 0;
        int itemWidth = 0.0, itemHeight = 0.0;

        for(int index = 0; index < m_documentView->m_document->numPages(); index++)
        {
            Poppler::Page *page = m_documentView->m_document->page(index);

            QImage thumbnail = page->thumbnail();
            if(!thumbnail.isNull())
            {
                item = new QListWidgetItem(m_listWidget);

                item->setIcon(QIcon(QPixmap::fromImage(thumbnail)));
                item->setData(Qt::UserRole, index+1);

                itemWidth = qMax(itemWidth, thumbnail.width());
                itemHeight = qMax(itemHeight, thumbnail.height());
            }

            delete page;
        }

        m_listWidget->setIconSize(QSize(itemWidth, itemHeight));
    }
}

void ThumbnailsView::followLink(QListWidgetItem *item)
{
    m_documentView->followLink(item->data(Qt::UserRole).toInt());
}
