#include "thumbnailsview.h"

ThumbnailsView::ThumbnailsView(QWidget *parent) :
    QDockWidget(parent)
{
    this->setWindowTitle(tr("Thumbnails"));
    this->setObjectName("thumbnailsView");

    this->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    this->setFeatures(QDockWidget::AllDockWidgetFeatures);
}

ThumbnailsView::~ThumbnailsView()
{
}
