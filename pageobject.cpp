#include "pageobject.h"

PageCache::PageCache() : m_map(),m_mutex()
{
}

PageCache::~PageCache()
{
}

void PageCache::clear()
{
    QMutexLocker mutexLocker(&m_mutex);

    m_map.clear();
}

bool PageCache::contains(int page)
{
    QMutexLocker mutexLocker(&m_mutex);

    return m_map.contains(page);
}

QImage PageCache::retrieve(int page)
{
    QMutexLocker mutexLocker(&m_mutex);

    return m_map.value(page);
}

void PageCache::insert(int page, QImage image)
{
    QMutexLocker mutexLocker(&m_mutex);

    if(m_map.size() < PageCacheLimit)
    {
        m_map.insert(page, image);
    }
    else
    {
        if(m_map.lowerBound(page) != m_map.end())
        {
            m_map.remove((--m_map.end()).key());
        }
        else
        {
            m_map.remove(m_map.begin().key());
        }

        m_map.insert(page, image);
    }
}

PageObject::PageObject(Poppler::Page *page, PageCache *pageCache, QGraphicsItem *parent) : QGraphicsObject(parent),
    m_page(page),m_pageCache(pageCache),m_rotation(Poppler::Page::Rotate0),m_resolutionX(72.0),m_resolutionY(72.0),m_filePath(),m_currentPage(-1),m_futureWatcher()
{
    connect(&m_futureWatcher, SIGNAL(finished()), this, SLOT(updatePageCache()));
}

PageObject::~PageObject()
{
    if(m_futureWatcher.isRunning())
    {
        m_futureWatcher.cancel();
        m_futureWatcher.waitForFinished();
    }

    if(m_page)
    {
        delete m_page;
    }
}

Poppler::Page::Rotation PageObject::rotation() const
{
    return m_rotation;
}

void PageObject::setRotation(const Poppler::Page::Rotation &rotation)
{
    if(m_rotation != rotation)
    {
        m_rotation = rotation;

        emit rotationChanged(m_rotation);
    }
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

QString PageObject::filePath() const
{
    return m_filePath;
}

void PageObject::setFilePath(const QString &filePath)
{
    if(m_filePath != filePath)
    {
        m_filePath = filePath;

        emit filePathChanged(m_filePath);
    }
}

int PageObject::currentPage() const
{
    return m_currentPage;
}

void PageObject::setCurrentPage(const int &currentPage)
{
    if(m_currentPage != currentPage)
    {
        m_currentPage = currentPage;

        emit currentPageChanged(m_currentPage);
    }
}


QRectF PageObject::boundingRect() const
{
    if(m_rotation == Poppler::Page::Rotate90 || m_rotation == Poppler::Page::Rotate270)
    {
        return QRectF(0.0, 0.0, m_resolutionX * m_page->pageSizeF().height() / 72.0, m_resolutionY * m_page->pageSizeF().width() / 72.0);
    }
    else
    {
        return QRectF(0.0, 0.0, m_resolutionX * m_page->pageSizeF().width() / 72.0, m_resolutionY * m_page->pageSizeF().height() / 72.0);
    }
}

void PageObject::paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    if(m_pageCache->contains(m_currentPage))
    {
        painter->drawImage(QPointF(0.0, 0.0), m_pageCache->retrieve(m_currentPage));
    }
    else
    {
        painter->fillRect(boundingRect(), QBrush(Qt::white));

        if(!m_futureWatcher.isRunning())
        {
            m_futureWatcher.setFuture(QtConcurrent::run(this, &PageObject::renderPage, false));
        }
    }

    painter->setPen(QPen(Qt::black));
    painter->drawRect(boundingRect());
}

void PageObject::prefetch()
{
    if(!m_pageCache->contains(m_currentPage) && !m_futureWatcher.isRunning())
    {
        m_futureWatcher.setFuture(QtConcurrent::run(this, &PageObject::renderPage, true));
    }
}

QImage PageObject::renderPage(bool prefetch)
{
    QRectF viewRect = this->scene()->views().first()->mapToScene(this->scene()->views().first()->rect()).boundingRect();
    QRectF pageRect = boundingRect(); pageRect.translate(x(),y());

    if(!viewRect.intersects(pageRect) && !prefetch)
    {
        return QImage();
    }

    Poppler::Document *document = Poppler::Document::load(m_filePath);

    if(document == 0)
    {
        qDebug() << "document == 0:" << m_filePath << m_currentPage;

        return QImage();
    }

    Poppler::Page *page = document->page(m_currentPage-1);

    if(page == 0) {
        qDebug() << "page == 0:" << m_filePath << m_currentPage;

        return QImage();
    }

    document->setRenderHint(Poppler::Document::Antialiasing);
    document->setRenderHint(Poppler::Document::TextAntialiasing);

    return page->renderToImage(m_resolutionX, m_resolutionY, -1, -1, -1, -1, m_rotation);
}

void PageObject::updatePageCache()
{
    if(!m_futureWatcher.result().isNull())
    {
        m_pageCache->insert(m_currentPage, m_futureWatcher.result());

        QRectF pageRect = boundingRect(); pageRect.translate(x(),y());

        this->scene()->update(pageRect);
        this->scene()->views().first()->update();
    }
}
