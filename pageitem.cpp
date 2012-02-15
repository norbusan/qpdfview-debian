#include "pageitem.h"

PageItem::PageItem(QGraphicsItem *parent) :
    QGraphicsItem(parent),
    m_resolutionX(-1.0),
    m_resolutionY(-1.0),
    m_page(0),
    m_filePath(),
    m_index(-1),
    m_state(Dummy),
    m_stateMutex()
{
    this->setAutoDelete(false);
}

const int PageItem::m_pageCacheCapacity = 2;
QMap<int, QImage> PageItem::m_pageCache;
QMutex PageItem::m_pageCacheMutex;

QRectF PageItem::boundingRect() const
{
    return QRectF(0.0, 0.0, m_resolutionX / 72.0 * m_page->pageSizeF().width(), m_resolutionY / 72.0 * m_page->pageSizeF().height());
}

void PageItem::paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    m_stateMutex.lock();

    switch(m_state)
    {
    case Dummy:
        m_state = Running;

        QThreadPool::globalInstance()->start(this);

        painter->fillRect(boundingRect(), QBrush(Qt::white));

        break;
    case Running:
        painter->fillRect(boundingRect(), QBrush(Qt::white));

        break;
    case Finished:
        m_pageCacheMutex.lock();

        if(m_pageCache.contains(m_index))
        {
            painter->drawImage(0, 0, m_pageCache.value(m_index));
        }
        else
        {
            m_state = Running;

            QThreadPool::globalInstance()->start(this);

            painter->fillRect(boundingRect(), QBrush(Qt::white));
        }

        m_pageCacheMutex.unlock();

        break;
    }

    m_stateMutex.unlock();

    painter->setPen(QPen(Qt::black));
    painter->drawRect(boundingRect());
}

void PageItem::run()
{
    QGraphicsScene *scene = this->scene();
    QGraphicsView *view = this->scene()->views().first();

    QRectF sceneRect = boundingRect(); sceneRect.translate(x(),y());
    QRectF viewRect = view->mapToScene(view->rect()).boundingRect();

    if(!viewRect.intersects(sceneRect))
    {
        m_stateMutex.lock();
        m_state = Dummy;
        m_stateMutex.unlock();

        return;
    }

    Poppler::Document *document = Poppler::Document::load(m_filePath);

    if(document == 0)
    {
        qDebug() << "document == 0:" << m_filePath << m_index;

        m_stateMutex.lock();
        m_state = Dummy;
        m_stateMutex.unlock();

        return;
    }

    Poppler::Page *page = document->page(m_index-1);

    if(page == 0) {
        qDebug() << "page == 0:" << m_filePath << m_index;

        m_stateMutex.lock();
        m_state = Dummy;
        m_stateMutex.unlock();

        return;
    }

    document->setRenderHint(Poppler::Document::Antialiasing);
    document->setRenderHint(Poppler::Document::TextAntialiasing);

    QImage image = page->renderToImage(m_resolutionX, m_resolutionY);

    m_pageCacheMutex.lock();

    if(m_pageCache.size() < m_pageCacheCapacity)
    {
        m_pageCache.insert(m_index, image);
    }
    else
    {
        QMap<int, QImage>::iterator lowerBound = m_pageCache.lowerBound(m_index);

        if(lowerBound != m_pageCache.end())
        {
            m_pageCache.remove(lowerBound.key());
        }
        else
        {
            m_pageCache.remove(m_pageCache.begin().key());
        }

        m_pageCache.insert(m_index, image);
    }

    m_pageCacheMutex.unlock();

    m_stateMutex.lock();
    m_state  = Finished;
    m_stateMutex.unlock();

    scene->update(sceneRect);
    view->update();
}
