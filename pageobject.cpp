#include "pageobject.h"

PageObject::PageObject(Poppler::Page *page, QGraphicsItem *parent) : QGraphicsObject(parent),
    m_page(page),m_resolutionX(72.0),m_resolutionY(72.0),m_rotation(0),m_filePath(),m_currentPage(-1),m_links(),m_highlight(),m_selection()
{
    foreach(Poppler::Link *link, m_page->links())
    {
        if(link->linkType() == Poppler::Link::Goto)
        {
            Poppler::LinkGoto *linkGoto = static_cast<Poppler::LinkGoto*>(link);

            if(!linkGoto->isExternal())
            {
                m_links.append(linkGoto);

                continue;
            }
        }

        delete link;
    }

    m_renderWatcher = new QFutureWatcher<QImage>();

    connect(m_renderWatcher, SIGNAL(finished()), this, SLOT(insertPage()));
}

QMutex PageObject::s_mutex;
QMap<QPair<QString, int>, QImage> PageObject::s_pageCache;
int PageObject::s_maximumPageCacheSize = QSettings("qpdfview","qpdfview").value("pageObject/maximumPageCacheSize", 32).toInt();

PageObject::~PageObject()
{
    if(m_renderWatcher->isRunning())
    {
        m_renderWatcher->waitForFinished();
    }

    delete m_renderWatcher;

    QMutexLocker mutexLocker(&s_mutex);

    s_pageCache.remove(QPair<QString, int>(m_filePath, m_currentPage));

    mutexLocker.unlock();

    while(!m_links.isEmpty()) { delete m_links.takeFirst(); }

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
        this->updatePage();
    }

    return result;
}


void PageObject::clearHighlight()
{
    m_highlight = QRectF();

    this->updatePage();
}

QRectF PageObject::highlightedArea() const
{
    QRectF highlight;

    highlight.setX(m_resolutionX * m_highlight.x() / 72.0);
    highlight.setY(m_resolutionY * m_highlight.y() / 72.0);
    highlight.setWidth(m_resolutionX * m_highlight.width() / 72.0);
    highlight.setHeight(m_resolutionY * m_highlight.height() / 72.0);

    highlight.adjust(-5.0, -5.0, 5.0, 5.0);
    highlight.translate(this->pos());

    return highlight;
}

QString PageObject::highlightedText() const
{
    return m_page->text(m_highlight);
}


void PageObject::prefetch()
{
    QMutexLocker mutexLocker(&s_mutex);

    if(!s_pageCache.contains(QPair<QString, int>(m_filePath, m_currentPage)) && !m_renderWatcher->isRunning())
    {
        m_renderWatcher->setFuture(QtConcurrent::run(this, &PageObject::renderPage, true));
    }

    mutexLocker.unlock();
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
    // draw page

    painter->fillRect(boundingRect(), QBrush(Qt::white));

    QMutexLocker mutexLocker(&s_mutex);

    if(!s_pageCache.contains(QPair<QString, int>(m_filePath, m_currentPage)) && !m_renderWatcher->isRunning())
    {
        m_renderWatcher->setFuture(QtConcurrent::run(this, &PageObject::renderPage, false));
    }
    else
    {
        painter->drawImage(QPointF(0.0, 0.0), s_pageCache.value(QPair<QString, int>(m_filePath, m_currentPage)));
    }

    mutexLocker.unlock();

    painter->setPen(QPen(Qt::black));
    painter->drawRect(boundingRect());

    // draw links

    painter->setPen(QPen(Qt::red));

    foreach(Poppler::LinkGoto *link, m_links)
    {
        QRectF linkArea;

        linkArea.setX(m_resolutionX * link->linkArea().x() * m_page->pageSizeF().width() / 72.0);
        linkArea.setY(m_resolutionY * link->linkArea().y() * m_page->pageSizeF().height() / 72.0);
        linkArea.setWidth(m_resolutionX * link->linkArea().width() * m_page->pageSizeF().width() / 72.0);
        linkArea.setHeight(m_resolutionY * link->linkArea().height() * m_page->pageSizeF().height() / 72.0);

        if(linkArea.width() < 0.0)
        {
            linkArea.translate(linkArea.width(), 0.0);
            linkArea.setWidth(-linkArea.width());
        }

        if(linkArea.height() < 0.0)
        {
            linkArea.translate(0.0, linkArea.height());
            linkArea.setHeight(-linkArea.height());
        }

        painter->drawRect(linkArea);
    }

    // draw highlight

    if(!m_highlight.isNull())
    {
        QRectF highlight;

        highlight.setX(m_resolutionX * m_highlight.x() / 72.0);
        highlight.setY(m_resolutionY * m_highlight.y() / 72.0);
        highlight.setWidth(m_resolutionX * m_highlight.width() / 72.0);
        highlight.setHeight(m_resolutionY * m_highlight.height() / 72.0);

        highlight.adjust(-5.0,-5.0,5.0,5.0);

        painter->fillRect(highlight, QBrush(QColor(0,0,0,31)));
    }

    // draw selection

    if(!m_selection.isNull())
    {
        QPen pen;
        pen.setColor(Qt::black);
        pen.setStyle(Qt::DashLine);
        painter->setPen(pen);

        painter->drawRect(m_selection);
    }
}

QImage PageObject::dummyFuture() const
{
    return QImage();
}

QImage PageObject::renderPage(bool prefetch)
{

    QRectF pageRect = boundingRect(); pageRect.translate(pos());

    bool visible = false;

    foreach(QGraphicsView *view, this->scene()->views())
    {
        QRectF viewRect = view->mapToScene(view->rect()).boundingRect();

        visible = viewRect.intersects(pageRect);
    }

    if(!visible && !prefetch)
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
    if(!m_renderWatcher->result().isNull())
    {
        QMutexLocker mutexLocker(&s_mutex);

        if(s_pageCache.size() < s_maximumPageCacheSize)
        {
            s_pageCache.insert(QPair<QString, int>(m_filePath, m_currentPage), m_renderWatcher->result());
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

            s_pageCache.insert(QPair<QString, int>(m_filePath, m_currentPage), m_renderWatcher->result());
        }

        qDebug() << s_pageCache.size();

        mutexLocker.unlock();

        disconnect(m_renderWatcher, SIGNAL(finished()), this, SLOT(insertPage()));

        m_renderWatcher->setFuture(QtConcurrent::run(this, &PageObject::dummyFuture));
        m_renderWatcher->waitForFinished();

        connect(m_renderWatcher, SIGNAL(finished()), this, SLOT(insertPage()));

        this->updatePage();
    }
}

void PageObject::updatePage()
{
    QRectF pageRect = boundingRect(); pageRect.translate(pos());

    this->scene()->update(pageRect);
}


void PageObject::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    foreach(Poppler::LinkGoto *link, m_links)
    {
        QRectF linkArea;

        linkArea.setX(m_resolutionX * link->linkArea().x() * m_page->pageSizeF().width() / 72.0);
        linkArea.setY(m_resolutionY * link->linkArea().y() * m_page->pageSizeF().height() / 72.0);
        linkArea.setWidth(m_resolutionX * link->linkArea().width() * m_page->pageSizeF().width() / 72.0);
        linkArea.setHeight(m_resolutionY * link->linkArea().height() * m_page->pageSizeF().height() / 72.0);

        if(linkArea.width() < 0.0)
        {
            linkArea.translate(linkArea.width(), 0.0);
            linkArea.setWidth(-linkArea.width());
        }

        if(linkArea.height() < 0.0)
        {
            linkArea.translate(0.0, linkArea.height());
            linkArea.setHeight(-linkArea.height());
        }

        if(linkArea.contains(event->scenePos() - this->pos()))
        {
            return;
        }
    }

    m_selection = QRectF(event->scenePos() - this->pos(), QSizeF());
}

void PageObject::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if(!m_selection.isNull())
    {
        m_highlight.setX(72.0 * m_selection.x() / m_resolutionX);
        m_highlight.setY(72.0 * m_selection.y() / m_resolutionY);
        m_highlight.setWidth(72.0 * m_selection.width() / m_resolutionX);
        m_highlight.setHeight(72.0 * m_selection.height() / m_resolutionY);

        m_selection = QRectF();

        this->updatePage();
    }

    foreach(Poppler::LinkGoto *link, m_links)
    {
        QRectF linkArea;

        linkArea.setX(m_resolutionX * link->linkArea().x() * m_page->pageSizeF().width() / 72.0);
        linkArea.setY(m_resolutionY * link->linkArea().y() * m_page->pageSizeF().height() / 72.0);
        linkArea.setWidth(m_resolutionX * link->linkArea().width() * m_page->pageSizeF().width() / 72.0);
        linkArea.setHeight(m_resolutionY * link->linkArea().height() * m_page->pageSizeF().height() / 72.0);

        if(linkArea.width() < 0.0)
        {
            linkArea.translate(linkArea.width(), 0.0);
            linkArea.setWidth(-linkArea.width());
        }

        if(linkArea.height() < 0.0)
        {
            linkArea.translate(0.0, linkArea.height());
            linkArea.setHeight(-linkArea.height());
        }

        if(linkArea.contains(event->scenePos() - this->pos()))
        {
            emit linkClicked(link->destination().pageNumber());

            return;
        }
    }
}

void PageObject::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if(!m_selection.isNull())
    {
        m_selection.setBottomRight(event->scenePos() - this->pos());

        this->updatePage();
    }
}
