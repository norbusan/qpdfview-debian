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

#include "documentmodel.h"

QMap<DocumentModel::PageCacheKey, QImage> DocumentModel::s_pageCache;
QMutex DocumentModel::s_pageCacheMutex;

uint DocumentModel::s_pageCacheSize = 0;
uint DocumentModel::s_maximumPageCacheSize = 134217728;

DocumentModel::DocumentModel(QObject *parent) : QObject(parent),
    m_filePath(""),m_pageCount(-1),m_results(),m_resultsMutex(),m_search(),m_print()
{
    m_document = 0;
}

DocumentModel::~DocumentModel()
{
    this->cancelSearch();
    this->cancelPrint();

    if(m_document)
    {
        delete m_document;
    }
}

const QString &DocumentModel::filePath() const
{
    return m_filePath;
}

int DocumentModel::pageCount() const
{
    return m_pageCount;
}

QSizeF DocumentModel::pageSize(int index)
{
    QSizeF result;

    if(m_document != 0 && index >= 0 && index < m_pageCount)
    {
        Poppler::Page *page = m_document->page(index);

        result = page->pageSizeF();

        delete page;
    }

    return result;
}

// links

QList<DocumentModel::Link> DocumentModel::links(int index)
{
    QList<Link> results;

    if(m_document != 0  && index >= 0 && index < m_pageCount)
    {
        Poppler::Page *page = m_document->page(index);

        foreach(Poppler::Link *link, page->links())
        {
            if(link->linkType() == Poppler::Link::Goto)
            {
                QRectF linkArea = link->linkArea().normalized();
                int linkIndex = static_cast<Poppler::LinkGoto*>(link)->destination().pageNumber()-1;

                Link result;
                result.area = linkArea;
                result.index = linkIndex;

                results.append(result);
            }

            delete link;
        }

        delete page;
    }

    return results;
}

// results

QList<QRectF> DocumentModel::results(int index)
{
    QList<QRectF> results;

    if(m_document != 0 && index >= 0 &&  index < m_pageCount)
    {
        m_resultsMutex.lock();

        if(m_results.contains(index))
        {
            results = m_results.values(index);
        }

        m_resultsMutex.unlock();
    }

    return results;
}

QMap<int, QRectF> DocumentModel::results()
{
    QMap<int, QRectF> results;

    if(m_document)
    {
        m_resultsMutex.lock();

        results = m_results;

        m_resultsMutex.unlock();
    }

    return results;
}

// text

QString DocumentModel::text(int index, QRectF area)
{
    QString result;

    if(m_document != 0 && index >= 0 && index < m_pageCount)
    {
        Poppler::Page *page = m_document->page(index);

        result = page->text(area);

        delete page;
    }

    return result;
}

// outline

static DocumentModel::Outline *domNodeToOutline(const QDomNode &domNode)
{
    DocumentModel::Outline *result = new DocumentModel::Outline;

    result->text = domNode.toElement().tagName();
    result->isOpen = QVariant(domNode.toElement().attribute("Open")).toBool();
    result->destinationName = domNode.toElement().attribute("DestinationName");

    QDomNode siblingNode = domNode.nextSibling();
    if(!siblingNode.isNull())
    {
        result->sibling = domNodeToOutline(siblingNode);
    }

    QDomNode childNode = domNode.firstChild();
    if(!childNode.isNull())
    {
        result->child = domNodeToOutline(childNode);
    }

    return result;
}

DocumentModel::Outline *DocumentModel::outline()
{
    QDomDocument *document = m_document->toc();

    if(document)
    {
        Outline *result = domNodeToOutline(document->firstChild());

        delete document;

        return result;
    }
    else
    {
        return 0;
    }
}

int DocumentModel::destination(const QString &destinationName)
{
    int pageNumber = -1;

    Poppler::LinkDestination *linkDestination = m_document->linkDestination(destinationName);

    if(linkDestination)
    {
        pageNumber = linkDestination->pageNumber();
    }

    return pageNumber;
}

// thumbnail

QImage DocumentModel::thumbnail(int index)
{
    QImage result;

    if(m_document != 0 && index >= 0 && index < m_pageCount)
    {
        Poppler::Page *page = m_document->page(index);

        result = page->thumbnail();

        delete page;
    }

    return result;
}

// page cache

QImage DocumentModel::pullPage(int index, qreal resolutionX, qreal resolutionY)
{
    PageCacheKey key(m_filePath, index, resolutionX, resolutionY);
    QImage value;

    if(m_document != 0 && index >= 0 && index < m_pageCount)
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

void DocumentModel::pushPage(int index, qreal resolutionX, qreal resolutionY)
{
    PageCacheKey key(m_filePath, index, resolutionX, resolutionY);
    QImage value;

    Poppler::Document *document = Poppler::Document::load(m_filePath);

    if(!document)
    {
        qDebug() << m_filePath;
        qFatal("!document");

        return;
    }

    document->setRenderHint(Poppler::Document::Antialiasing);
    document->setRenderHint(Poppler::Document::TextAntialiasing);

    Poppler::Page *page = document->page(index);

    if(!page)
    {
        qDebug() << m_filePath << index;
        qFatal("!page");

        delete document;
        return;
    }

    value = page->renderToImage(resolutionX, resolutionY);

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
}

void DocumentModel::dropPage(int index, qreal resolutionX, qreal resolutionY)
{
    PageCacheKey key(m_filePath, index, resolutionX, resolutionY);

    if(m_document != 0 && index >= 0 && index < m_pageCount)
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

bool DocumentModel::open(const QString &filePath)
{
    Poppler::Document *document = Poppler::Document::load(filePath);

    if(document)
    {
        if(m_document)
        {
            delete m_document;
        }

        m_document = document;

        m_filePath = filePath;
        m_pageCount = m_document->numPages();

        emit filePathChanged(m_filePath);
        emit pageCountChanged(m_pageCount);
    }

    return document != 0;
}

bool DocumentModel::refresh()
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

            emit filePathChanged(m_filePath);

            if(m_pageCount != document->numPages())
            {
                m_pageCount = document->numPages();

                emit pageCountChanged(m_pageCount);
            }
        }

        return document != 0;
    }
    else
    {
        return false;
    }
}

bool DocumentModel::saveCopy(const QString &filePath)
{
    if(m_document)
    {
            Poppler::PDFConverter *converter = m_document->pdfConverter();

            converter->setOutputFileName(filePath);
            bool result = converter->convert();

            delete converter;

            return result;
    }
    else
    {
            return false;
    }
}

void DocumentModel::startSearch(const QString &text, bool matchCase)
{
    this->cancelSearch();

    if(m_document)
    {
        m_search = QtConcurrent::run(this, &DocumentModel::search, text, matchCase);
    }
}

void DocumentModel::cancelSearch()
{
    if(m_search.isRunning())
    {
        m_search.cancel();
        m_search.waitForFinished();
    }

    m_resultsMutex.lock();

    m_results.clear();

    m_resultsMutex.unlock();

    for(int index = 0; index < m_pageCount; index++)
    {
        emit resultsChanged(index);
    }

    emit resultsChanged();
}

void DocumentModel::startPrint(QPrinter *printer, int fromPage, int toPage)
{
    this->cancelPrint();

    if(m_document != 0 && fromPage >= 1 && fromPage <= m_pageCount && toPage >= 1 && toPage <= m_pageCount && fromPage <= toPage)
    {
        m_print = QtConcurrent::run(this, &DocumentModel::print, printer, fromPage, toPage);
    }
}

void DocumentModel::cancelPrint()
{
    if(m_print.isRunning())
    {
        m_print.cancel();
        m_print.waitForFinished();
    }
}

void DocumentModel::search(const QString &text, bool matchCase)
{
    Poppler::Document *document = Poppler::Document::load(m_filePath);

    if(!document)
    {
        qDebug() << m_filePath;
        qFatal("!document");

        return;
    }

    for(int index = 0; index < m_pageCount; index++)
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

        m_resultsMutex.lock();

        while(!results.isEmpty())
        {
            m_results.insertMulti(index, results.takeLast());
        }

        m_resultsMutex.unlock();

        delete page;

        emit resultsChanged(index);

        emit searchProgressed((100*(index+1))/m_pageCount);
    }

    delete document;

    emit resultsChanged();

    emit searchFinished();
}

void DocumentModel::print(QPrinter *printer, int fromPage, int toPage)
{
    Poppler::Document *document = Poppler::Document::load(m_filePath);

    if(!document)
    {
        qDebug() << m_filePath;
        qFatal("!document");

        delete printer;
        return;
    }

    document->setRenderHint(Poppler::Document::Antialiasing);
    document->setRenderHint(Poppler::Document::TextAntialiasing);

    QPainter *painter = new QPainter();
    painter->begin(printer);

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

        if(index != toPage-1)
        {
            printer->newPage();
        }

        delete page;

        emit printProgressed((100*(index-fromPage+2))/(toPage-fromPage+1));
    }

    delete document;
    delete painter;
    delete printer;

    emit printFinished();
}
