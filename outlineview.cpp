#include "outlineview.h"

OutlineView::OutlineView(QWidget *parent) : QDockWidget(parent),
    m_documentView(0), m_outline(0)
{
    this->setWindowTitle(tr("&Outline"));
    this->setObjectName("outlineView");

    this->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    this->setFeatures(QDockWidget::AllDockWidgetFeatures);

    m_treeWidget = new QTreeWidget();
    this->setWidget(m_treeWidget);

    connect(m_treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(followLink(QTreeWidgetItem*,int)));

    m_treeWidget->header()->setVisible(false);
}

OutlineView::~OutlineView()
{
    delete m_treeWidget;

    if(m_outline)
    {
        delete m_outline;
    }
}

void OutlineView::setDocumentView(DocumentView *documentView)
{
    if(m_documentView != documentView)
    {
        m_documentView = documentView;

        if(m_documentView)
        {
            connect(m_documentView, SIGNAL(filePathChanged(QString)), this, SLOT(updateOutline()));
        }

        this->updateOutline();
    }
}


void OutlineView::updateOutline(const QDomNode &parentNode, QTreeWidgetItem *parentItem)
{
    QTreeWidgetItem *item = 0;

    for(QDomNode node = parentNode.firstChildElement(); !node.isNull(); node = node.nextSiblingElement())
    {
        if(parentItem)
        {
            item = new QTreeWidgetItem(parentItem);
        }
        else
        {
            item = new QTreeWidgetItem(m_treeWidget, item);
        }

        item->setText(0, node.toElement().tagName());

        if(!node.toElement().hasAttribute("ExternalFileName") && node.toElement().hasAttribute("DestinationName"))
        {
            item->setData(0, Qt::UserRole, QVariant(node.toElement().attribute("DestinationName")));
        }

        if(node.toElement().hasAttribute("Open"))
        {
            if(QVariant(node.toElement().attribute("Open")).toBool())
            {
                m_treeWidget->expandItem(item);
            }
        }

        if(node.hasChildNodes())
        {
            updateOutline(node, item);
        }
    }
}

void OutlineView::updateOutline()
{
    m_treeWidget->clear();

    if(m_documentView)
    {
        if(m_outline)
        {
            delete m_outline;
        }

        m_outline = m_documentView->m_document->toc();

        if(m_outline)
        {
            updateOutline(*m_outline, 0);
        }
    }
}

void OutlineView::followLink(QTreeWidgetItem *item, const int &column)
{
    Poppler::LinkDestination *linkDestination = m_documentView->m_document->linkDestination(item->data(column, Qt::UserRole).toString());

    if(linkDestination)
    {
        m_documentView->followLink(linkDestination->pageNumber());
    }
}
