#include "outlineview.h"

OutlineView::OutlineView(QWidget *parent) :
    QDockWidget(parent)
{
    this->setWindowTitle(tr("Outline"));
    this->setObjectName("outlineView");

    this->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    this->setFeatures(QDockWidget::AllDockWidgetFeatures);

    m_treeWidget = new QTreeWidget();
    this->setWidget(m_treeWidget);
}

OutlineView::~OutlineView()
{
    delete m_treeWidget;
}
