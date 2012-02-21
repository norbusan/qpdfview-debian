#include "pageobject.h"

PageObject::PageObject(Poppler::Page *page, QGraphicsItem *parent) : QGraphicsObject(parent),
    m_page(page),m_resolutionX(-1.0),m_resolutionY(-1.0)
{
}

PageObject::~PageObject()
{
}


qreal PageObject::resolutionX() const
{
    return m_resolutionX;
}

void PageObject::setResolutionX(const qreal &resolutionX)
{
    if(m_resolutionX != resolutionX)
    {
        m_resolutionX = resolutionX;

        emit resolutionXChanged(m_resolutionX);
    }
}

qreal PageObject::resolutionY() const
{
    return m_resolutionY;
}

void PageObject::setResolutionY(const qreal &resolutionY)
{
    if(m_resolutionY != resolutionY)
    {
        m_resolutionY = resolutionY;

        emit resolutionYChanged(m_resolutionY);
    }
}


QRectF PageObject::boundingRect() const
{
    return QRectF(0.0, 0.0, qCeil(m_resolutionX * m_page->pageSizeF().width() / 72.0), qCeil(m_resolutionY * m_page->pageSizeF().height() / 72.0));
}

void PageObject::paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    QImage image = m_page->renderToImage(m_resolutionX, m_resolutionY);
    painter->drawImage(QPointF(0.0, 0.0), image);

    painter->setPen(QPen(Qt::black));
    painter->drawRect(boundingRect());
}

/*PageItem::PageItem(QGraphicsItem *parent) :
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

QMap<int, QImage> PageItem::s_pageCache;
QMutex PageItem::s_pageCacheMutex;

const int PageItem::maximumPageCacheSize = 16;

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
        s_pageCacheMutex.lock();

        if(s_pageCache.contains(m_index))
        {
            painter->drawImage(0, 0, s_pageCache.value(m_index));
        }
        else
        {
            m_state = Running;

            QThreadPool::globalInstance()->start(this);

            painter->fillRect(boundingRect(), QBrush(Qt::white));
        }

        s_pageCacheMutex.unlock();

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

    s_pageCacheMutex.lock();

    if(s_pageCache.size() < maximumPageCacheSize)
    {
        s_pageCache.insert(m_index, image);
    }
    else
    {
        if(s_pageCache.lowerBound(m_index) != s_pageCache.end())
        {
            s_pageCache.remove((--s_pageCache.end()).key());
        }
        else
        {
            s_pageCache.remove(s_pageCache.begin().key());
        }

        s_pageCache.insert(m_index, image);
    }

    s_pageCacheMutex.unlock();

    m_stateMutex.lock();
    m_state  = Finished;
    m_stateMutex.unlock();

    scene->update(sceneRect);
    view->update();
}*/
