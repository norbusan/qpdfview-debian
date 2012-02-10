#include "documentview.h"

DocumentView::DocumentView(QWidget *parent) :
    QGraphicsView(parent),
    m_scene(),
    m_itemList()
{
}

void DocumentView::changeDocument(const QString &filePath)
{
}


void DocumentView::changeIndex(const int &index)
{
}


void DocumentView::changeDisplayMode(const DocumentModel::DisplayModes &displayMode)
{
}


void DocumentView::changeScaleMode(const DocumentModel::ScaleModes &scaleMode)
{
}

void DocumentView::changeScaleFactor(const qreal &scaleFactor)
{
}
