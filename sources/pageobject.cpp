#include "pageobject.h"

PageObject::PageObject(DocumentModel *model, DocumentView *view, int index, QGraphicsItem *parent) : QGraphicsObject(parent),
    m_index(index),m_size(),m_links(),m_selection(),m_rubberBand(),m_render()
{
    m_model = model;
    m_view = view;

    m_size = m_model->pageSize(m_index);
    m_links = m_model->links(m_index);
    m_results = m_model->results(index);

    qreal scaleX = m_view->resolutionX() / 72.0;
    qreal scaleY = m_view->resolutionY() / 72.0;
    qreal width = m_size.width();
    qreal height = m_size.height();

    switch(m_view->rotation())
    {
    case DocumentView::RotateBy0:
        m_pageTransform = QTransform(1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
        m_linkTransform = QTransform(scaleX * width, 0.0, 0.0, scaleY * height, 0.0, 0.0);
        m_resultsTransform = QTransform(scaleX, 0.0, 0.0, scaleY, 0.0, 0.0);

        break;
    case DocumentView::RotateBy90:
        m_pageTransform = QTransform(0.0, 1.0, -1.0, 0.0, scaleX * height, 0.0);
        m_linkTransform = QTransform(0.0, scaleY * width, -scaleX * height, 0.0, scaleX * height, 0.0);
        m_resultsTransform = QTransform(0.0, scaleY, -scaleX, 0.0, scaleX * height, 0.0);

        break;
    case DocumentView::RotateBy180:
        m_pageTransform = QTransform(-1.0, 0.0, 0.0, -1.0, scaleX * width, scaleY * height);
        m_linkTransform = QTransform(-scaleX * width, 0.0, 0.0, -scaleY * height, scaleX * width, scaleY * height);
        m_resultsTransform = QTransform(-scaleX, 0.0, 0.0, -scaleY, scaleX * width, scaleY * height);

        break;
    case DocumentView::RotateBy270:
        m_pageTransform = QTransform(0.0, -1.0, 1.0, 0.0, 0.0, scaleX * width);
        m_linkTransform = QTransform(0.0, -scaleY * width, scaleX * height, 0.0, 0.0, scaleY * width);
        m_resultsTransform = QTransform(0.0, -scaleY, scaleX, 0.0, 0.0, scaleY * width);

        break;
    }

    connect(&m_render, SIGNAL(finished()), this, SLOT(pageRendered()));
    connect(m_model, SIGNAL(pageSearched(int)), this, SLOT(pageSearched(int)));
    connect(m_view, SIGNAL(highlightAllChanged(bool)), this, SLOT(highlightAllChanged()));
}

PageObject::~PageObject()
{
    if(m_render.isRunning())
    {
        m_render.waitForFinished();
    }

    m_model->dropPage(m_index, m_view->resolutionX(), m_view->resolutionY());
}

int PageObject::index() const
{
    return m_index;
}

void PageObject::setIndex(const int &index)
{
    if(m_index != index)
    {
        m_index = index;

        emit indexChanged(m_index);
    }
}

qreal PageObject::pageWidth() const
{
    return m_size.width();
}

qreal PageObject::pageHeight() const
{
    return m_size.height();
}

QRectF PageObject::selectedArea() const
{
    return m_resultsTransform.inverted().mapRect(m_selection);
}

QString PageObject::selectedText() const
{
    QString text;

    if(!m_selection.isNull())
    {
        text = m_model->text(m_index, this->selectedArea());
    }

    return text;
}


QRectF PageObject::boundingRect() const
{
    QRectF result;

    qreal scaleX = m_view->resolutionX() / 72.0;
    qreal scaleY = m_view->resolutionY() / 72.0;
    qreal width = m_size.width();
    qreal height = m_size.height();

    switch(m_view->rotation())
    {
    case DocumentView::RotateBy0:
    case DocumentView::RotateBy180:
        result = QRectF(0.0, 0.0, qCeil(scaleX * width), qCeil(scaleY * height));

        break;
    case DocumentView::RotateBy90:
    case DocumentView::RotateBy270:
        result = QRectF(0.0, 0.0, qCeil(scaleX * height), qCeil(scaleY * width));

        break;
    }

    return result;
}

void PageObject::paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    // draw page

    painter->fillRect(boundingRect(), QBrush(Qt::white));

    QImage image = m_model->pullPage(m_index, m_view->resolutionX(), m_view->resolutionY());

    if(!image.isNull())
    {
        painter->setTransform(m_pageTransform, true);
        painter->drawImage(QPointF(0.0, 0.0), image);
        painter->setTransform(m_pageTransform.inverted(), true);
    }
    else
    {
        if(!m_render.isRunning())
        {
            m_render.setFuture(QtConcurrent::run(this, &PageObject::render));
        }
    }

    painter->setPen(QPen(Qt::black));
    painter->drawRect(boundingRect());

    // draw links

    painter->setPen(QPen(QColor(255,0,0,127)));
    painter->setTransform(m_linkTransform, true);

    foreach(DocumentModel::Link link, m_links)
    {
        painter->drawRect(link.area);
    }

    painter->setTransform(m_linkTransform.inverted(), true);

    // draw results

    if(m_view->highlightAll())
    {
        painter->setTransform(m_resultsTransform, true);

        foreach(QRectF result, m_results)
        {
            painter->fillRect(result.adjusted(-1.0, -1.0, 1.0, 1.0), QBrush(QColor(0,255,0,127)));
        }

        painter->setTransform(m_resultsTransform.inverted(), true);
    }

    // draw selection

    if(!m_selection.isNull())
    {
        painter->fillRect(m_selection, QBrush(QColor(0,0,255,127)));
    }

    // draw rubber band

    if(!m_rubberBand.isNull())
    {
        QPen pen;
        pen.setColor(Qt::black);
        pen.setStyle(Qt::DashLine);
        painter->setPen(pen);

        painter->drawRect(m_rubberBand);
    }
}

void PageObject::render()
{
    bool visible = false;

    QRectF pageRect = boundingRect().translated(pos());

    foreach(QGraphicsView *view, this->scene()->views())
    {
       QRectF viewRect = view->mapToScene(view->rect()).boundingRect();

       visible = visible || viewRect.intersects(pageRect);
    }

    if(visible)
    {
       switch(m_view->rotation())
       {
       case DocumentView::RotateBy0:
       case DocumentView::RotateBy180:
           m_model->pushPage(m_index, m_view->resolutionX(), m_view->resolutionY());

           break;
       case DocumentView::RotateBy90:
       case DocumentView::RotateBy270:
           m_model->pushPage(m_index, m_view->resolutionY(), m_view->resolutionX());

           break;
       }
    }
}

void PageObject::pageRendered()
{
    this->scene()->update(boundingRect().translated(pos()));
}

void PageObject::pageSearched(int index)
{
    if(m_index == index)
    {
        m_results = m_model->results(index);

        this->scene()->update(boundingRect().translated(pos()));
    }
}

void PageObject::highlightAllChanged()
{
    if(m_results.count() > 0)
    {
        this->scene()->update(boundingRect().translated(pos()));
    }
}

void PageObject::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
   if(event->button() == Qt::LeftButton)
    {
        foreach(DocumentModel::Link link, m_links)
        {
            if(m_linkTransform.mapRect(link.area).contains(event->scenePos() - pos()))
            {
                return;
            }
        }

        m_rubberBand = QRectF(event->scenePos() - pos(), QSizeF());
    }
}

void PageObject::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        if(!m_rubberBand.isNull())
        {
            m_selection = m_rubberBand.adjusted(-5.0, -5.0, 5.0, 5.0);
            m_rubberBand = QRectF();

            this->scene()->update(boundingRect().translated(pos()));

            return;
        }

        foreach(DocumentModel::Link link, m_links)
        {
            if(m_linkTransform.mapRect(link.area).contains(event->scenePos() - pos()))
            {
                emit linkClicked(link.index+1);

                return;
            }
        }
    }
    else if(event->button() == Qt::RightButton)
    {
        if(!m_selection.isNull())
        {
            m_selection = QRectF();

            this->scene()->update(boundingRect().translated(pos()));
        }
    }
}

void PageObject::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if(!m_rubberBand.isNull())
    {
        m_rubberBand.setBottomRight(event->scenePos() - pos());

        this->scene()->update(boundingRect().translated(pos()));
    }
}
