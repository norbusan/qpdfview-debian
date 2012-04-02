/*

Copyright 2012 Adam Reichold

This file is part of qpdfview.

qpdfview is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

qpdfview is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with qpdfview.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <poppler-qt4.h>

#include "document.h"

QSettings Document::s_settings("qpdfview", "qpdfview");

bool Document::s_automaticRefresh = Document::s_settings.value("document/automaticRefresh", false).toBool();
bool Document::s_openUrl = Document::s_settings.value("document/openUrl", false).toBool();

bool Document::s_antialiasing = Document::s_settings.value("document/antialiasing", true).toBool();
bool Document::s_textAntialiasing = Document::s_settings.value("document/textAntialiasing", true).toBool();

QMap<Document::PageCacheKey, QImage> Document::s_pageCache;
QMutex Document::s_pageCacheMutex;

uint Document::s_pageCacheSize = 0;
uint Document::s_maximumPageCacheSize = Document::s_settings.value("document/maximumPageCacheSize", 134217728u).toUInt();

int Document::s_prefetchDistance = Document::s_settings.value("document/prefetchDistance", 2).toInt();

Document::Document(QObject *parent) : QObject(parent),
    m_document(0),
    m_filePath(),
    m_numberOfPages(-1),
    m_fileSystemWatcher(0),
    m_searchResults(),
    m_searchResultsMutex(),
    m_render(),
    m_search(),
    m_print()
{
}

Document::~Document()
{
    if(m_render.isRunning())
    {
        m_render.waitForFinished();
    }

    if(m_search.isRunning())
    {
        m_search.cancel();
        m_search.waitForFinished();
    }

    if(m_print.isRunning())
    {
        m_print.cancel();
        m_print.waitForFinished();
    }

    if(m_document)
    {
        delete m_document;
    }

    if(m_fileSystemWatcher)
    {
        delete m_fileSystemWatcher;
    }
}

// properties

const QString &Document::filePath() const
{
    return m_filePath;
}

int Document::numberOfPages() const
{
    return m_numberOfPages;
}

// settings

bool Document::automaticRefresh()
{
    return s_automaticRefresh;
}

void Document::setAutomaticRefresh(bool automaticRefresh)
{
    if(s_automaticRefresh != automaticRefresh)
    {
        s_automaticRefresh = automaticRefresh;

        s_settings.setValue("document/automaticRefresh", s_automaticRefresh);
    }
}

bool Document::openUrl()
{
    return s_openUrl;
}

void Document::setOpenUrl(bool openUrl)
{
    if(s_openUrl != openUrl)
    {
        s_openUrl = openUrl;

        s_settings.setValue("document/openUrl", s_openUrl);
    }
}

bool Document::antialiasing()
{
    return s_antialiasing;
}

void Document::setAntialiasing(bool antialiasing)
{
    if(s_antialiasing != antialiasing)
    {
        s_antialiasing = antialiasing;

        s_settings.setValue("document/antialiasing", s_antialiasing);

        s_pageCacheMutex.lock();

        s_pageCacheSize = 0;
        s_pageCache.clear();

        s_pageCacheMutex.unlock();
    }
}

bool Document::textAntialiasing()
{
    return s_textAntialiasing;
}

void Document::setTextAntialiasing(bool textAntialiasing)
{
    if(s_textAntialiasing != textAntialiasing)
    {
        s_textAntialiasing = textAntialiasing;

        s_settings.setValue("document/textAntialiasing", s_textAntialiasing);

        s_pageCacheMutex.lock();

        s_pageCacheSize = 0;
        s_pageCache.clear();

        s_pageCacheMutex.unlock();
    }
}

// pages

QSizeF Document::size(int index)
{
    QSizeF result;

    if(m_document != 0 && index >= 0 && index < m_numberOfPages)
    {
        Poppler::Page *page = m_document->page(index);

        result = page->pageSizeF();

        delete page;
    }

    return result;
}

QImage Document::thumbnail(int index)
{
    QImage result;

    if(m_document != 0 && index >= 0 && index < m_numberOfPages)
    {
        Poppler::Page *page = m_document->page(index);

        result = page->thumbnail();

        delete page;
    }

    return result;
}

QString Document::text(int index, QRectF area)
{
    QString result;

    if(m_document != 0 && index >= 0 && index < m_numberOfPages)
    {
        Poppler::Page *page = m_document->page(index);

        result = page->text(area);

        delete page;
    }

    return result;
}

// links

QList<Link> Document::links(int index)
{
    QList<Link> results;

    if(m_document != 0  && index >= 0 && index < m_numberOfPages)
    {
        Poppler::Page *page = m_document->page(index);

        foreach(Poppler::Link *link, page->links())
        {
            if(link->linkType() == Poppler::Link::Goto)
            {
                if(!static_cast<Poppler::LinkGoto*>(link)->isExternal())
                {
                    Link result;

                    result.area = link->linkArea().normalized();

                    result.page = static_cast<Poppler::LinkGoto*>(link)->destination().pageNumber();
                    result.top = static_cast<Poppler::LinkGoto*>(link)->destination().top();

                    results.append(result);
                }
            }
            else if(link->linkType() == Poppler::Link::Browse)
            {
                Link result;

                result.area = link->linkArea().normalized();

                result.url = static_cast<Poppler::LinkBrowse*>(link)->url();

                results.append(result);
            }

            delete link;
        }

        delete page;
    }

    return results;
}

// toc

TocNode *Document::toc()
{
    TocNode *result = 0;

    if(m_document)
    {
        QDomDocument *toc = m_document->toc();

        if(toc)
        {
            result = domNodeToTocNode(toc->firstChild());

            delete toc;
        }
    }

    return result;
}

// page cache

QImage Document::pullPage(int index, qreal scaleFactor)
{
    PageCacheKey key(m_filePath, index, scaleFactor);
    QImage value;

    if(m_document != 0 && index >= 0 && index < m_numberOfPages)
    {
        s_pageCacheMutex.lock();

        if(s_pageCache.contains(key))
        {
            value = s_pageCache.value(key);
        }

        s_pageCacheMutex.unlock();
    }

    return value;
}

void Document::pushPage(int index, qreal scaleFactor)
{
    PageCacheKey key(m_filePath, index, scaleFactor);

    if(m_document != 0 && index >= 0 && index < m_numberOfPages)
    {
        if(!s_pageCache.contains(key))
        {
            render(index, scaleFactor);
        }
    }
}

void Document::dropPage(int index, qreal scaleFactor)
{
    PageCacheKey key(m_filePath, index, scaleFactor);

    if(m_document != 0 && index >= 0 && index < m_numberOfPages)
    {
        s_pageCacheMutex.lock();

        if(s_pageCache.contains(key))
        {
            s_pageCacheSize -= s_pageCache.value(key).byteCount();
            s_pageCache.remove(key);
        }

        s_pageCacheMutex.unlock();
    }
}

uint Document::maximumPageCacheSize()
{
    return s_maximumPageCacheSize;
}

void Document::setMaximumPageCacheSize(uint maximumPageCacheSize)
{
    if(s_maximumPageCacheSize != maximumPageCacheSize)
    {
        s_maximumPageCacheSize = maximumPageCacheSize;

        s_settings.setValue("document/maximumPageCacheSize", s_maximumPageCacheSize);

        s_pageCacheMutex.lock();

        while(s_pageCacheSize > s_maximumPageCacheSize)
        {
            QMap<PageCacheKey, QImage>::iterator first = s_pageCache.begin();

            s_pageCacheSize -= first.value().byteCount();
            s_pageCache.remove(first.key());
        }

        s_pageCacheMutex.unlock();
    }
}

int Document::prefetchDistance()
{
    return s_prefetchDistance;
}

void Document::setPrefetchDistance(int prefetchDistance)
{
    if(s_prefetchDistance != prefetchDistance)
    {
        s_prefetchDistance = prefetchDistance;

        s_settings.setValue("document/prefetchDistance", s_prefetchDistance);
    }
}

// search results

QList<QRectF> Document::searchResults(int index)
{
    QList<QRectF> result;

    if(m_document != 0 && index >= 0 &&  index < m_numberOfPages)
    {
        m_searchResultsMutex.lock();

        if(m_searchResults.contains(index))
        {
            result = m_searchResults.values(index);
        }

        m_searchResultsMutex.unlock();
    }

    return result;
}

// slots

bool Document::open(const QString &filePath)
{
    Poppler::Document *document = Poppler::Document::load(filePath);

    if(document)
    {
        if(m_document)
        {
            delete m_document;
        }

        if(m_fileSystemWatcher)
        {
            delete m_fileSystemWatcher;
        }

        m_document = document;

        m_filePath = filePath;
        m_numberOfPages = m_document->numPages();

        emit filePathChanged(m_filePath);
        emit numberOfPagesChanged(m_numberOfPages);

        emit documentChanged();

        if(s_automaticRefresh)
        {
            m_fileSystemWatcher = new QFileSystemWatcher(this);
            m_fileSystemWatcher->addPath(m_filePath);
            connect(m_fileSystemWatcher, SIGNAL(fileChanged(QString)), this, SLOT(refresh()));
        }
    }

    return document != 0;
}

bool Document::refresh()
{
    if(m_document)
    {
        Poppler::Document *document = Poppler::Document::load(m_filePath);

        if(document)
        {
            if(m_document)
            {
                delete m_document;
            }

            m_document = document;

            if(m_numberOfPages != document->numPages())
            {
                m_numberOfPages = document->numPages();

                emit numberOfPagesChanged(m_numberOfPages);
            }

            s_pageCacheMutex.lock();

            for(QMap<PageCacheKey, QImage>::iterator i =s_pageCache.begin(); i != s_pageCache.end(); i++)
            {
                if(i.key().filePath == m_filePath)
                {
                    s_pageCacheSize -= i.value().byteCount();
                    s_pageCache.remove(i.key());
                }
            }

            s_pageCacheMutex.unlock();

            emit documentChanged();
        }

        return document != 0;
    }

    return false;
}

bool Document::saveCopy(const QString &filePath)
{
    if(m_document)
    {
            Poppler::PDFConverter *converter = m_document->pdfConverter();

            converter->setOutputFileName(filePath);

            bool result = converter->convert();

            delete converter;

            return result;
    }

    return false;
}

void Document::startRender(int index, qreal scaleFactor)
{
    if(m_document != 0 && index >= 0 && index < m_numberOfPages)
    {
        m_render = QtConcurrent::run(this, &Document::render, index, scaleFactor);
    }
}

void Document::startSearch(const QString &text, bool matchCase, int beginWithPage)
{
    this->cancelSearch();

    if(m_document != 0 && !text.isEmpty())
    {
        m_search = QtConcurrent::run(this, &Document::search, text, matchCase, beginWithPage);
    }
}

void Document::cancelSearch()
{
    if(m_search.isRunning())
    {
        m_search.cancel();
        m_search.waitForFinished();
    }

    m_searchResultsMutex.lock();

    m_searchResults.clear();

    m_searchResultsMutex.unlock();

    for(int index = 0; index < m_numberOfPages; index++)
    {
        emit pageSearched(index);
    }
}

void Document::startPrint(QPrinter *printer, int fromPage, int toPage)
{
    this->cancelPrint();

    if(m_document != 0 && fromPage >= 1 && fromPage <= m_numberOfPages && toPage >= 1 && toPage <= m_numberOfPages && fromPage <= toPage)
    {
        m_print = QtConcurrent::run(this, &Document::print, printer, fromPage, toPage);
    }
}

void Document::cancelPrint()
{
    if(m_print.isRunning())
    {
        m_print.cancel();
        m_print.waitForFinished();
    }
}

// internal methods

TocNode *Document::domNodeToTocNode(const QDomNode &domNode)
{
    TocNode *result = new TocNode();

    result->text = domNode.toElement().tagName();

    if(domNode.toElement().hasAttribute("Destination"))
    {
        Poppler::LinkDestination linkDestination(domNode.toElement().attribute("Destination"));

        result->page = linkDestination.pageNumber();
        result->top = linkDestination.top();
    }
    else if(domNode.toElement().hasAttribute("DestinationName"))
    {
        Poppler::LinkDestination *linkDestination = m_document->linkDestination(domNode.toElement().attribute("DestinationName"));

        result->page = linkDestination ? linkDestination->pageNumber() : 1;
        result->top = linkDestination ? linkDestination->top() : 0.0;

        delete linkDestination;
    }

    QDomNode siblingNode = domNode.nextSibling();
    if(!siblingNode.isNull())
    {
        result->sibling = domNodeToTocNode(siblingNode);
    }

    QDomNode childNode = domNode.firstChild();
    if(!childNode.isNull())
    {
        result->child = domNodeToTocNode(childNode);
    }

    return result;
}

void Document::render(int index, qreal scaleFactor)
{
    PageCacheKey key(m_filePath, index, scaleFactor);
    QImage value;

    Poppler::Document *document = Poppler::Document::load(m_filePath);

    if(!document)
    {
        qDebug() << m_filePath;
        qFatal("!document");

        return;
    }

    document->setRenderHint(Poppler::Document::Antialiasing, s_antialiasing);
    document->setRenderHint(Poppler::Document::TextAntialiasing, s_textAntialiasing);

    Poppler::Page *page = document->page(index);

    if(!page)
    {
        qDebug() << m_filePath << index;
        qFatal("!page");

        delete document;
        return;
    }

    value = page->renderToImage(scaleFactor * 72.0, scaleFactor * 72.0);

    delete page;
    delete document;

    if(!value.isNull())
    {
        s_pageCacheMutex.lock();

        if(s_pageCacheSize < s_maximumPageCacheSize)
        {
            s_pageCache.insert(key, value);
            s_pageCacheSize += value.byteCount();
        }
        else
        {
            if(s_pageCache.lowerBound(key) != s_pageCache.end())
            {
                QMap<PageCacheKey, QImage>::iterator last = --s_pageCache.end();

                s_pageCacheSize -= last.value().byteCount();
                s_pageCache.remove(last.key());
            }
            else
            {
                QMap<PageCacheKey, QImage>::iterator first = s_pageCache.begin();

                s_pageCacheSize -= first.value().byteCount();
                s_pageCache.remove(first.key());
            }

            s_pageCache.insert(key, value);
            s_pageCacheSize += value.byteCount();
        }

        s_pageCacheMutex.unlock();
    }

    emit pageRendered(index);
}

void Document::search(const QString &text, bool matchCase, int beginWithPage)
{
    Poppler::Document *document = Poppler::Document::load(m_filePath);

    if(!document)
    {
        qDebug() << m_filePath;
        qFatal("!document");

        return;
    }

    QList<int> indices;

    for(int index = beginWithPage-1; index < m_numberOfPages; index++)
    {
        indices.append(index);
    }

    for(int index = 0; index < beginWithPage-1; index++)
    {
        indices.append(index);
    }

    foreach(int index, indices)
    {
        if(m_search.isCanceled())
        {
            emit searchCanceled();

            delete document;
            return;
        }

        Poppler::Page *page = document->page(index);

        if(!page)
        {
            qDebug() << m_filePath << index;
            qFatal("!page");

            delete document;
            return;
        }

        QList<QRectF> results;

        double rectLeft = 0.0, rectTop = 0.0, rectRight = 0.0, rectBottom = 0.0;

        while(page->search(text, rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::NextResult, matchCase ? Poppler::Page::CaseSensitive : Poppler::Page::CaseInsensitive))
        {
            QRectF rect;
            rect.setLeft(rectLeft);
            rect.setTop(rectTop);
            rect.setRight(rectRight);
            rect.setBottom(rectBottom);

            results.append(rect.normalized());
        }

        m_searchResultsMutex.lock();

        while(!results.isEmpty())
        {
            m_searchResults.insertMulti(index, results.takeLast());
        }

        m_searchResultsMutex.unlock();

        delete page;

        emit pageSearched(index);

        emit searchProgressed((100 * (index+1)) / m_numberOfPages);
    }

    delete document;

    emit searchFinished();
}

void Document::print(QPrinter *printer, int fromPage, int toPage)
{
    Poppler::Document *document = Poppler::Document::load(m_filePath);

    if(!document)
    {
        qDebug() << m_filePath;
        qFatal("!document");

        delete printer;
        return;
    }

    document->setRenderHint(Poppler::Document::Antialiasing, s_antialiasing);
    document->setRenderHint(Poppler::Document::TextAntialiasing, s_textAntialiasing);

    QPainter *painter = new QPainter(printer);

    for(int index = fromPage-1; index <= toPage-1; index++)
    {
        if(m_print.isCanceled())
        {
            emit printCanceled();

            delete document;
            delete painter;
            delete printer;
            return;
        }

        Poppler::Page *page = document->page(index);

        if(!page)
        {
            qDebug() << m_filePath << index;
            qFatal("!page");

            delete document;
            delete painter;
            delete printer;
            return;
        }

        qreal fitToWidth = static_cast<qreal>(printer->width()) / (printer->physicalDpiX() * page->pageSizeF().width() / 72.0);
        qreal fitToHeight = static_cast<qreal>(printer->height()) / (printer->physicalDpiY() * page->pageSizeF().height() / 72.0);
        qreal fit = qMin(fitToWidth, fitToHeight);

        painter->setTransform(QTransform::fromScale(fit, fit));
        painter->drawImage(QPointF(0.0, 0.0), page->renderToImage(printer->physicalDpiX(), printer->physicalDpiY()));

        if(index < toPage-1)
        {
            printer->newPage();
        }

        delete page;

        emit printProgressed((100 * (index+1 - fromPage + 1)) / (toPage - fromPage + 1));
    }

    delete document;
    delete painter;
    delete printer;

    emit printFinished();
}
