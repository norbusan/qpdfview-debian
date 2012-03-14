#include "pageobject.h"

PageObject::PageObject(Poppler::Page *page, int index, DocumentView *view, QGraphicsItem *parent) : QGraphicsObject(parent),
    m_page(page),m_index(index),m_view(view),m_matrix1(),m_matrix2(),m_links(),m_destinations(),m_highlight(),m_selection()
{
    m_matrix1.scale(resolutionX() / 72.0, resolutionY() / 72.0);

    switch(rotation())
    {
    case Poppler::Page::Rotate0:
        m_matrix2.setMatrix(resolutionX() / 72.0 * m_page->pageSizeF().width(), 0.0,
                            0.0, resolutionY() / 72.0 * m_page->pageSizeF().height(),
                            0.0, 0.0);
        break;
    case Poppler::Page::Rotate90:
        m_matrix2.setMatrix(0.0, resolutionY() / 72.0 * m_page->pageSizeF().width(),
                            -1.0 * resolutionX() / 72.0 * m_page->pageSizeF().height(), 0.0,
                            resolutionX() / 72.0 * m_page->pageSizeF().height(), 0.0);
        break;
    case Poppler::Page::Rotate180:
        m_matrix2.setMatrix(-1.0 * resolutionX() / 72.0 * m_page->pageSizeF().width(), 0.0,
                            0.0, -1.0 * resolutionY() / 72.0 * m_page->pageSizeF().height(),
                            resolutionX() / 72.0 * m_page->pageSizeF().width(), resolutionY() / 72.0 * m_page->pageSizeF().height());
        break;
    case Poppler::Page::Rotate270:
        m_matrix2.setMatrix(0.0, -1.0 * resolutionY() / 72.0 * m_page->pageSizeF().width(),
                            resolutionX() / 72.0 * m_page->pageSizeF().height(), 0.0,
                            0.0, resolutionY() / 72.0 * m_page->pageSizeF().width());
        break;
    }

    foreach(Poppler::Link *link, m_page->links())
    {
        if(link->linkType() == Poppler::Link::Goto)
        {
            Poppler::LinkGoto *linkGoto = static_cast<Poppler::LinkGoto*>(link);

            if(!linkGoto->isExternal())
            {
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

                linkArea = m_matrix2.mapRect(linkArea);

                m_links.append(linkArea);
                m_destinations.append(linkGoto->destination().pageNumber());
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

    QMutexLocker mutexLocker(&s_mutex);

    QPair<QString, int> key(filePath(), index());

    if(s_pageCache.contains(key))
    {
        s_pageCacheByteCount -= s_pageCache.value(key).byteCount();
        s_pageCache.remove(key);
    }

    mutexLocker.unlock();

    delete m_page;
}


QMutex PageObject::s_mutex;
QMap<QPair<QString, int>, QImage> PageObject::s_pageCache;

uint PageObject::s_pageCacheByteCount = 0;
uint PageObject::s_maximumPageCacheByteCount = QSettings("qpdfview","qpdfview").value("pageObject/maximumPageCacheByteCount", 134217728).toUInt();


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
    QRectF highlight = m_matrix1.mapRect(m_highlight);

    highlight.adjust(-5.0, -5.0, 5.0, 5.0);
    highlight.translate(pos());

    return highlight;
}

QString PageObject::highlightedText() const
{
    return m_page->text(m_highlight);
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
        m_renderWatcher->setFuture(QtConcurrent::run(this, &PageObject::renderPage));
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

    foreach(QRectF link, m_links)
    {
        painter->drawRect(link);
    }

    // draw highlight

    if(!m_highlight.isNull())
    {
        QRectF highlight = m_matrix1.mapRect(m_highlight);

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


void PageObject::updateScene()
{
    QRectF pageRect = boundingRect(); pageRect.translate(pos());

    this->scene()->update(pageRect);
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

    QMutexLocker mutexLocker(&s_mutex);

    QPair<QString, int> key(filePath(), index());

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

    mutexLocker.unlock();

    this->updateScene();

    delete page;
    delete document;
}


void PageObject::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    foreach(QRectF link, m_links)
    {
        if(link.contains(event->scenePos() - pos()))
        {
            return;
        }
    }

    m_selection = QRectF(event->scenePos() - pos(), QSizeF());
}

void PageObject::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if(!m_selection.isNull())
    {
        m_highlight = m_matrix1.inverted().mapRect(m_selection);
        m_selection = QRectF();

        this->updateScene();
    }

    for(int i = 0; i < m_links.size(); i++)
    {
        if(m_links[i].contains(event->scenePos() - pos()))
        {
            emit linkClicked(m_destinations[i]);

            return;
        }
    }
}

void PageObject::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if(!m_selection.isNull())
    {
        m_selection.setBottomRight(event->scenePos() - pos());

        this->updateScene();
    }
}
