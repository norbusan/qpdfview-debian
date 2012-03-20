#include "pageobject.h"

PageObject::PageObject(Poppler::Page *page, int index, DocumentView *view, QGraphicsItem *parent) : QGraphicsObject(parent),
    m_page(page),m_index(index),m_view(view),m_linkTransform(),m_highlightTransform(),m_links(),m_externalLinks(),m_highlight(),m_lastResult(),m_rubberBand()
{
    switch(rotation())
    {
    case Poppler::Page::Rotate0:
        m_highlightTransform.setMatrix(resolutionX() / 72.0, 0.0,
                                       0.0, resolutionY() / 72.0,
                                       0.0, 0.0);

        m_linkTransform.setMatrix(resolutionX() / 72.0 * m_page->pageSizeF().width(), 0.0,
                                  0.0, resolutionY() / 72.0 * m_page->pageSizeF().height(),
                                  0.0, 0.0);

        break;
    case Poppler::Page::Rotate90:
        m_highlightTransform.setMatrix(0.0, resolutionX() / 72.0,
                                       -1.0 * resolutionY() / 72.0, 0.0,
                                       resolutionX() / 72.0 * m_page->pageSizeF().height(), 0.0);

        m_linkTransform.setMatrix(0.0, resolutionY() / 72.0 * m_page->pageSizeF().width(),
                                  -1.0 * resolutionX() / 72.0 * m_page->pageSizeF().height(), 0.0,
                                  resolutionX() / 72.0 * m_page->pageSizeF().height(), 0.0);

        break;
    case Poppler::Page::Rotate180:
        m_highlightTransform.setMatrix(-1.0 * resolutionX() / 72.0, 0.0,
                                       0.0, -1.0 * resolutionY() / 72.0,
                                       resolutionX() / 72.0 * m_page->pageSizeF().width(), resolutionY() / 72.0 * m_page->pageSizeF().height());

        m_linkTransform.setMatrix(-1.0 * resolutionX() / 72.0 * m_page->pageSizeF().width(), 0.0,
                            0.0, -1.0 * resolutionY() / 72.0 * m_page->pageSizeF().height(),
                            resolutionX() / 72.0 * m_page->pageSizeF().width(), resolutionY() / 72.0 * m_page->pageSizeF().height());

        break;
    case Poppler::Page::Rotate270:
        m_highlightTransform.setMatrix(0.0, -1.0 * resolutionX() / 72.0,
                            resolutionY() / 72.0, 0.0,
                            0.0, resolutionY() / 72.0 * m_page->pageSizeF().width());

        m_linkTransform.setMatrix(0.0, -1.0 * resolutionY() / 72.0 * m_page->pageSizeF().width(),
                            resolutionX() / 72.0 * m_page->pageSizeF().height(), 0.0,
                            0.0, resolutionY() / 72.0 * m_page->pageSizeF().width());

        break;
    }

    foreach(Poppler::Link *link, m_page->links())
    {
        if(link->linkType() == Poppler::Link::Goto)
        {
            Poppler::LinkGoto *linkGoto = static_cast<Poppler::LinkGoto*>(link);

            QRectF linkArea = link->linkArea();

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

            linkArea = m_linkTransform.mapRect(linkArea);

            if(linkGoto->isExternal())
            {
                m_externalLinks.append(ExternalLink(linkArea, linkGoto->fileName(), linkGoto->destination().pageNumber()));
            }
            else
            {
                m_links.append(Link(linkArea, linkGoto->destination().pageNumber()));
            }
        }

        delete link;
    }

    m_renderWatcher = new QFutureWatcher<void>();
}

PageObject::~PageObject()
{
    if(m_renderWatcher->isRunning())
    {
        m_renderWatcher->waitForFinished();
    }

    delete m_renderWatcher;

    QMutexLocker mutexLocker(&s_pageCacheMutex);

    QPair<QString, int> key(filePath(), index());

    if(s_pageCache.contains(key))
    {
        s_pageCacheByteCount -= s_pageCache.value(key).byteCount();
        s_pageCache.remove(key);
    }

    mutexLocker.unlock();

    delete m_page;
}

bool PageObject::s_concurrentPageCache = true;

QMutex PageObject::s_pageCacheMutex;
QMap<QPair<QString, int>, QImage> PageObject::s_pageCache;

uint PageObject::s_pageCacheByteCount = 0;
uint PageObject::s_maximumPageCacheByteCount = 134217728;


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


QRectF PageObject::highlightedArea() const
{
    return m_highlight.translated(pos());
}

QString PageObject::highlightedText() const
{
    return m_page->text(m_highlightTransform.inverted().mapRect(m_highlight));
}


QRectF PageObject::lastResult() const
{
    return QMatrix().scale(resolutionX() / 72.0, resolutionY() / 72.0).mapRect(m_lastResult).translated(pos()).adjusted(-5.0, -5.0, 5.0, 5.0);
}

bool PageObject::findPrevious(const QString &text, bool matchCase)
{
    bool result = m_page->search(text, m_lastResult, Poppler::Page::PreviousResult, matchCase ? Poppler::Page::CaseSensitive : Poppler::Page::CaseInsensitive, rotation());

    if(result)
    {
        this->updateScene();
    }

    return result;
}

bool PageObject::findNext(const QString &text, bool matchCase)
{
    bool result = m_page->search(text, m_lastResult, Poppler::Page::NextResult, matchCase ? Poppler::Page::CaseSensitive : Poppler::Page::CaseInsensitive, rotation());

    if(result)
    {
        this->updateScene();
    }

    return result;
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

    QPair<QString, int> key(filePath(), index());

    if(s_concurrentPageCache)
    {
        QMutexLocker mutexLocker(&s_pageCacheMutex);

        if(!s_pageCache.contains(key) && !m_renderWatcher->isRunning())
        {
            m_renderWatcher->setFuture(QtConcurrent::run(this, &PageObject::renderPage));
        }
        else
        {
            painter->drawImage(QPointF(0.0, 0.0), s_pageCache.value(key));
        }

        mutexLocker.unlock();
    }
    else
    {
        if(!s_pageCache.contains(key))
        {
            QImage image = m_page->renderToImage(resolutionX(), resolutionY(), -1, -1, -1, -1, rotation());

            this->updatePageCache(key, image);
        }

        painter->drawImage(QPointF(0.0, 0.0), s_pageCache.value(key));
    }

    painter->setPen(QPen(Qt::black));
    painter->drawRect(boundingRect());

    // draw links

    painter->setPen(QPen(QColor(255,0,0,127)));

    foreach(Link link, m_links)
    {
        painter->drawRect(link.area);
    }

    // draw external links

    foreach(ExternalLink link, m_externalLinks)
    {
        painter->drawRect(link.area);
    }

    // draw highlight

    if(!m_highlight.isNull())
    {
        painter->fillRect(m_highlight, QBrush(QColor(0,0,255,127)));
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

    // draw last result

    if(!m_lastResult.isNull())
    {
        painter->fillRect(QMatrix().scale(resolutionX() / 72.0, resolutionY() / 72.0).mapRect(m_lastResult).adjusted(-5.0, -5.0, 5.0, 5.0), QBrush(QColor(0,255,0,127)));
    }
}


void PageObject::renderPage()
{
    QRectF pageRect = boundingRect(); pageRect.translate(pos());

    bool visible = false;

    foreach(QGraphicsView *view, this->scene()->views())
    {
        QRectF viewRect = view->mapToScene(view->viewport()->rect()).boundingRect();

        visible = viewRect.intersects(pageRect);
    }

    if(!visible)
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

    QMutexLocker mutexLocker(&s_pageCacheMutex);

    QPair<QString, int> key(filePath(), index());

    this->updatePageCache(key, image);

    mutexLocker.unlock();

    this->updateScene();

    delete page;
    delete document;
}

void PageObject::updateScene()
{
    QRectF pageRect = boundingRect(); pageRect.translate(pos());

    this->scene()->update(pageRect);
}

void PageObject::updatePageCache(QPair<QString, int> key, QImage image)
{
    if(s_pageCacheByteCount < s_maximumPageCacheByteCount)
    {
        s_pageCache.insert(key, image);
        s_pageCacheByteCount += image.byteCount();
    }
    else
    {
        if(s_pageCache.lowerBound(key) != s_pageCache.end())
        {
            s_pageCacheByteCount -= (--s_pageCache.end()).value().byteCount();
            s_pageCache.remove((--s_pageCache.end()).key());
        }
        else
        {
            s_pageCacheByteCount -= s_pageCache.begin().value().byteCount();
            s_pageCache.remove(s_pageCache.begin().key());
        }

        s_pageCache.insert(key, image);
        s_pageCacheByteCount += image.byteCount();
    }
}

bool PageObject::pageCacheThreading()
{
    return s_concurrentPageCache;
}

void PageObject::setPageCacheThreading(bool threading)
{
    s_concurrentPageCache = threading;
}

uint PageObject::pageCacheSize()
{
    return s_maximumPageCacheByteCount;
}

void PageObject::setPageCacheSize(uint size)
{
    s_maximumPageCacheByteCount = size;
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

void PageObject::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        foreach(Link link, m_links)
        {
            if(link.area.contains(event->scenePos() - pos()))
            {
                return;
            }
        }

        foreach(ExternalLink link, m_externalLinks)
        {
            if(link.area.contains(event->scenePos() - pos()))
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
            m_highlight = m_rubberBand.adjusted(-5.0, -5.0, 5.0, 5.0);
            m_rubberBand = QRectF();

            this->updateScene();
        }

        foreach(Link link, m_links)
        {
            if(link.area.contains(event->scenePos() - pos()))
            {
                emit linkClicked(link.pageNumber);

                return;
            }
        }

        foreach(ExternalLink link, m_externalLinks)
        {
            if(link.area.contains(event->scenePos() - pos()))
            {
                emit linkClicked(link.fileName, link.pageNumber);

                return;
            }
        }
    }
    else if(event->button() == Qt::RightButton)
    {
        if(!m_highlight.isNull())
        {
            m_highlight = QRectF();

            this->updateScene();
        }
    }
}

void PageObject::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if(!m_rubberBand.isNull())
    {
        m_rubberBand.setBottomRight(event->scenePos() - pos());

        this->updateScene();
    }
}
