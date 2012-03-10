#include "pageobject.h"

PageObject::PageObject(Poppler::Page *page, int index, DocumentView *view, QGraphicsItem *parent) : QGraphicsObject(parent),
    m_page(page),m_index(index),m_view(view),m_links(),m_highlight(),m_selection()
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

    m_renderWatcher = new QFutureWatcher<void>();
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

    s_pageCache.remove(QPair<QString, int>(filePath(), index()));

    mutexLocker.unlock();

    while(!m_links.isEmpty()) { delete m_links.takeFirst(); }

    delete m_page;
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

QString PageObject::filePath() const
{
    return m_view->filePath();
}

qreal PageObject::resolutionX() const
{
    return m_view->resolutionX();
}

qreal PageObject::resolutionY() const
{
    return m_view->resolutionY();
}

Poppler::Page::Rotation PageObject::rotation() const
{
    return static_cast<Poppler::Page::Rotation>(m_view->rotation());
}


bool PageObject::findNext(const QString &text)
{
    bool result = m_page->search(text, m_highlight, Poppler::Page::NextResult, Poppler::Page::CaseInsensitive, rotation());

    if(result)
    {
        this->updateScene();
    }

    return result;
}


void PageObject::clearHighlight()
{
    m_highlight = QRectF();

    this->updateScene();
}

QRectF PageObject::highlightedArea() const
{
    QRectF highlight;

    highlight.setX(resolutionX() * m_highlight.x() / 72.0);
    highlight.setY(resolutionY() * m_highlight.y() / 72.0);
    highlight.setWidth(resolutionX() * m_highlight.width() / 72.0);
    highlight.setHeight(resolutionY() * m_highlight.height() / 72.0);

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

    if(!s_pageCache.contains(QPair<QString, int>(filePath(), index())) && !m_renderWatcher->isRunning())
    {
        m_renderWatcher->setFuture(QtConcurrent::run(this, &PageObject::renderPage, true));
    }

    mutexLocker.unlock();
}

void PageObject::updateScene()
{
    QRectF pageRect = boundingRect(); pageRect.translate(pos());

    this->scene()->update(pageRect);
}


QRectF PageObject::boundingRect() const
{
    if(rotation() == Poppler::Page::Rotate90 || rotation() == Poppler::Page::Rotate270)
    {
        return QRectF(0.0, 0.0, qCeil(resolutionX() * m_page->pageSizeF().height() / 72.0), qCeil(resolutionY() * m_page->pageSizeF().width() / 72.0));
    }
    else
    {
        return QRectF(0.0, 0.0, qCeil(resolutionX() * m_page->pageSizeF().width() / 72.0), qCeil(resolutionY() * m_page->pageSizeF().height() / 72.0));
    }
}

void PageObject::paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    // draw page

    painter->fillRect(boundingRect(), QBrush(Qt::white));

    QMutexLocker mutexLocker(&s_mutex);

    if(!s_pageCache.contains(QPair<QString, int>(filePath(), index())) && !m_renderWatcher->isRunning())
    {
        m_renderWatcher->setFuture(QtConcurrent::run(this, &PageObject::renderPage, false));
    }
    else
    {
        painter->drawImage(QPointF(0.0, 0.0), s_pageCache.value(QPair<QString, int>(filePath(), index())));
    }

    mutexLocker.unlock();

    painter->setPen(QPen(Qt::black));
    painter->drawRect(boundingRect());

    // draw links

    painter->setPen(QPen(Qt::red));

    foreach(Poppler::LinkGoto *link, m_links)
    {
        QRectF linkArea;

        linkArea.setX(resolutionX() * link->linkArea().x() * m_page->pageSizeF().width() / 72.0);
        linkArea.setY(resolutionY() * link->linkArea().y() * m_page->pageSizeF().height() / 72.0);
        linkArea.setWidth(resolutionX() * link->linkArea().width() * m_page->pageSizeF().width() / 72.0);
        linkArea.setHeight(resolutionY() * link->linkArea().height() * m_page->pageSizeF().height() / 72.0);

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

        highlight.setX(resolutionX() * m_highlight.x() / 72.0);
        highlight.setY(resolutionY() * m_highlight.y() / 72.0);
        highlight.setWidth(resolutionX() * m_highlight.width() / 72.0);
        highlight.setHeight(resolutionY() * m_highlight.height() / 72.0);

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


void PageObject::renderPage(bool prefetch)
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
        return;
    }

    Poppler::Document *document = Poppler::Document::load(filePath());

    if(document == 0)
    {
        qDebug() << "document == 0:" << filePath();

        return;
    }

    Poppler::Page *page = document->page(index());

    if(page == 0) {
        qDebug() << "page == 0:" << filePath() << index();

        return;
    }

    document->setRenderHint(Poppler::Document::Antialiasing);
    document->setRenderHint(Poppler::Document::TextAntialiasing);

    QImage image = page->renderToImage(resolutionX(), resolutionY(), -1, -1, -1, -1, rotation());

    QMutexLocker mutexLocker(&s_mutex);

    if(s_pageCache.size() < s_maximumPageCacheSize)
    {
        s_pageCache.insert(QPair<QString, int>(filePath(), index()), image);
    }
    else
    {
        if(s_pageCache.lowerBound(QPair<QString, int>(filePath(), index())) != s_pageCache.end())
        {
            s_pageCache.remove((--s_pageCache.end()).key());
        }
        else
        {
            s_pageCache.remove(s_pageCache.begin().key());
        }

        s_pageCache.insert(QPair<QString, int>(filePath(), index()), image);
    }

    mutexLocker.unlock();

    this->updateScene();

    delete page;
    delete document;
}


void PageObject::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    foreach(Poppler::LinkGoto *link, m_links)
    {
        QRectF linkArea;

        linkArea.setX(resolutionX() * link->linkArea().x() * m_page->pageSizeF().width() / 72.0);
        linkArea.setY(resolutionY() * link->linkArea().y() * m_page->pageSizeF().height() / 72.0);
        linkArea.setWidth(resolutionX() * link->linkArea().width() * m_page->pageSizeF().width() / 72.0);
        linkArea.setHeight(resolutionY() * link->linkArea().height() * m_page->pageSizeF().height() / 72.0);

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
        m_highlight.setX(72.0 * m_selection.x() / resolutionX());
        m_highlight.setY(72.0 * m_selection.y() / resolutionY());
        m_highlight.setWidth(72.0 * m_selection.width() / resolutionX());
        m_highlight.setHeight(72.0 * m_selection.height() / resolutionY());

        m_selection = QRectF();

        this->updateScene();
    }

    foreach(Poppler::LinkGoto *link, m_links)
    {
        QRectF linkArea;

        linkArea.setX(resolutionX() * link->linkArea().x() * m_page->pageSizeF().width() / 72.0);
        linkArea.setY(resolutionY() * link->linkArea().y() * m_page->pageSizeF().height() / 72.0);
        linkArea.setWidth(resolutionX() * link->linkArea().width() * m_page->pageSizeF().width() / 72.0);
        linkArea.setHeight(resolutionY() * link->linkArea().height() * m_page->pageSizeF().height() / 72.0);

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

        this->updateScene();
    }
}
