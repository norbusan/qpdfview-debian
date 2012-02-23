#include "pageobject.h"

PageObject::PageObject(Poppler::Page *page, QGraphicsItem *parent) : QGraphicsObject(parent),
    m_page(page),m_resolutionX(72.0),m_resolutionY(72.0),m_rotation(0),m_highlight(),m_filePath(),m_currentPage(-1),m_futureWatcher()
{
    connect(&m_futureWatcher, SIGNAL(finished()), this, SLOT(insertPage()));
}

QMap<QPair<QString, int>, QImage> PageObject::s_pageCache;
QMutex PageObject::s_mutex;

PageObject::~PageObject()
{
    if(m_futureWatcher.isRunning())
    {
        m_futureWatcher.waitForFinished();
    }

    QMutexLocker mutexLocker(&s_mutex);

    s_pageCache.remove(QPair<QString, int>(m_filePath, m_currentPage));

    mutexLocker.unlock();

    delete m_page;
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

uint PageObject::rotation() const
{
    return m_rotation;
}

void PageObject::setRotation(const uint &rotation)
{
    if(m_rotation != rotation)
    {
        m_rotation = rotation;

        emit rotationChanged(m_rotation);
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


bool PageObject::findNext(const QString &text)
{
    bool result = m_page->search(text, m_highlight, Poppler::Page::NextResult, Poppler::Page::CaseInsensitive, static_cast<Poppler::Page::Rotation>(m_rotation));

    if(result)
    {
        QRectF pageRect = boundingRect(); pageRect.translate(pos());

        this->scene()->update(pageRect);
        this->scene()->views().first()->update();
    }

    return result;
}

void PageObject::clearHighlight()
{
    m_highlight = QRectF();
}

QRectF PageObject::highlight() const
{
    QRectF highlight;

    highlight.setX(m_resolutionX * m_highlight.x() / 72.0 - 5.0);
    highlight.setY(m_resolutionY * m_highlight.y() / 72.0 - 5.0);
    highlight.setWidth(m_resolutionX * m_highlight.width() / 72.0 + 10.0);
    highlight.setHeight(m_resolutionY * m_highlight.height() / 72.0 + 10.0);

    highlight.translate(pos());

    return highlight;
}


QRectF PageObject::boundingRect() const
{
    if(m_rotation == 1 || m_rotation == 3)
    {
        return QRectF(0.0, 0.0, qCeil(m_resolutionX * m_page->pageSizeF().height() / 72.0), qCeil(m_resolutionY * m_page->pageSizeF().width() / 72.0));
    }
    else
    {
        return QRectF(0.0, 0.0, qCeil(m_resolutionX * m_page->pageSizeF().width() / 72.0), qCeil(m_resolutionY * m_page->pageSizeF().height() / 72.0));
    }
}

void PageObject::paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->fillRect(boundingRect(), QBrush(Qt::white));

    QMutexLocker mutexLocker(&s_mutex);

    if(!s_pageCache.contains(QPair<QString, int>(m_filePath, m_currentPage)) && !m_futureWatcher.isRunning())
    {
        m_futureWatcher.setFuture(QtConcurrent::run(this, &PageObject::renderPage, false));

    }
    else
    {
        painter->drawImage(QPointF(0.0, 0.0), s_pageCache.value(QPair<QString, int>(m_filePath, m_currentPage)));
    }

    mutexLocker.unlock();

    if(!m_highlight.isNull())
    {
        QRect highlight;

        highlight.setX(m_resolutionX * m_highlight.x() / 72.0 - 5.0);
        highlight.setY(m_resolutionY * m_highlight.y() / 72.0 - 5.0);
        highlight.setWidth(m_resolutionX * m_highlight.width() / 72.0 + 10.0);
        highlight.setHeight(m_resolutionY * m_highlight.height() / 72.0 + 10.0);

        painter->fillRect(highlight, QBrush(QColor(0,0,0,31)));
    }

    painter->setPen(QPen(Qt::black));
    painter->drawRect(boundingRect());
}

void PageObject::prefetch()
{
    QMutexLocker mutexLocker(&s_mutex);

    if(!s_pageCache.contains(QPair<QString, int>(m_filePath, m_currentPage)) && !m_futureWatcher.isRunning())
    {
        m_futureWatcher.setFuture(QtConcurrent::run(this, &PageObject::renderPage, true));
    }

    mutexLocker.unlock();
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
        qDebug() << "document == 0:" << m_filePath;

        return QImage();
    }

    Poppler::Page *page = document->page(m_currentPage-1);

    if(page == 0) {
        qDebug() << "page == 0:" << m_filePath << m_currentPage;

        return QImage();
    }

    document->setRenderHint(Poppler::Document::Antialiasing);
    document->setRenderHint(Poppler::Document::TextAntialiasing);

    QImage image = page->renderToImage(m_resolutionX, m_resolutionY, -1, -1, -1, -1, static_cast<Poppler::Page::Rotation>(m_rotation));

    delete page;
    delete document;

    return image;
}

void PageObject::insertPage()
{
    if(!m_futureWatcher.result().isNull())
    {
        QMutexLocker mutexLocker(&s_mutex);

        if(s_pageCache.size() < 32)
        {
            s_pageCache.insert(QPair<QString, int>(m_filePath, m_currentPage), m_futureWatcher.result());
        }
        else
        {
            if(s_pageCache.lowerBound(QPair<QString, int>(m_filePath, m_currentPage)) != s_pageCache.end())
            {
                s_pageCache.remove((--s_pageCache.end()).key());
            }
            else
            {
                s_pageCache.remove(s_pageCache.begin().key());
            }

            s_pageCache.insert(QPair<QString, int>(m_filePath, m_currentPage), m_futureWatcher.result());
        }

        mutexLocker.unlock();

        QRectF pageRect = boundingRect(); pageRect.translate(pos());

        this->scene()->update(pageRect);
        this->scene()->views().first()->update();
    }
}
