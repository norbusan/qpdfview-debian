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

#include "documentview.h"

DocumentView::DocumentView(QWidget *parent) :
    QGraphicsView(parent),
    m_document(0),
    m_pageList(),
    m_filePath(),
    m_index(-1),
    m_displayMode(PagingMode),
    m_scaleMode(ScaleFactorMode),
    m_scaleFactor(1.0)
{
    m_graphicsScene = new QGraphicsScene();
    m_graphicsScene->setBackgroundBrush(QBrush(QColor("darkgrey")));

    this->setScene(m_graphicsScene);
}

DocumentView::~DocumentView()
{
    if(m_document) { delete m_document; }
    while(!m_pageList.isEmpty()) { delete m_pageList.takeFirst(); }

    delete m_graphicsScene;
}


bool DocumentView::load(const QString &filePath)
{
    Poppler::Document *document = Poppler::Document::load(filePath);

    if(document)
    {
        if(m_document) { delete m_document; }
        while(!m_pageList.isEmpty()) { delete m_pageList.takeFirst(); }

        m_document = document;
        for(int index=0;index<document->numPages();index++) { m_pageList.append(document->page(index)); }

        document->setRenderBackend(Poppler::Document::ArthurBackend);
        document->setRenderHint(Poppler::Document::Antialiasing);
        document->setRenderHint(Poppler::Document::TextAntialiasing);

        m_filePath = filePath;
        m_index = 1;

        emit documentChanged(m_filePath);
        emit indexChanged(m_index);

        layout();
    }

    return document != 0;
}

bool DocumentView::reload() {
    if(m_document)
    {
        Poppler::Document *document = Poppler::Document::load(m_filePath);

        if(document)
        {
            if(m_document) { delete m_document; }
            while(!m_pageList.isEmpty()) { delete m_pageList.takeFirst(); }

            m_document = document;
            for(int index=0;index<document->numPages();index++) { m_pageList.append(document->page(index)); }

            document->setRenderBackend(Poppler::Document::ArthurBackend);
            document->setRenderHint(Poppler::Document::Antialiasing);
            document->setRenderHint(Poppler::Document::TextAntialiasing);

            if(m_index > m_pageList.size())
            {
                m_index = 1;
            }

            emit documentChanged(m_filePath);
            emit indexChanged(m_index);

            layout();
        }

        return document != 0;
    }
    else
    {
        return false;
    }
}

bool DocumentView::save(const QString &filePath) const
{
    if(m_document)
    {
        Poppler::PDFConverter *pdfConverter = m_document->pdfConverter();
        pdfConverter->setOutputFileName(filePath);

        bool result = pdfConverter->convert();

        delete pdfConverter;

        return result;
    }
    else
    {
        return false;
    }
}


void DocumentView::setIndex(const int &index)
{
    if(m_document && m_index != index && index >= 1 &&  index <= m_pageList.size())
    {
        switch(m_displayMode)
        {
        case PagingMode:
        case ScrollingMode:
            m_index = index;
            break;
        case DoublePagingMode:
        case DoubleScrollingMode:
            if(index%2==0)
            {
                m_index = index-1;
            }
            else
            {
                m_index = index;
            }
            break;
        }

        emit indexChanged(m_index);
    }
}

void DocumentView::previousPage()
{
    if(m_document)
    {
        switch(m_displayMode)
        {
        case PagingMode:
        case ScrollingMode:
            if(m_index > 1)
            {
                m_index -= 1;

                emit indexChanged(m_index);
            }
            break;
        case DoublePagingMode:
        case DoubleScrollingMode:
            if(m_index > 2)
            {
                m_index -= 2;

                emit indexChanged(m_index);
            }
            break;
        }
    }
}

void DocumentView::nextPage()
{
    if(m_document)
    {
        switch(m_displayMode)
        {
        case PagingMode:
        case ScrollingMode:
            if(m_index <= m_pageList.size()-1)
            {
                m_index += 1;

                emit indexChanged(m_index);
            }
            break;
        case DoublePagingMode:
        case DoubleScrollingMode:
            if(m_index <= m_pageList.size()-2)
            {
                m_index += 2;

                emit indexChanged(m_index);
            }
            break;
        }
    }
}

void DocumentView::firstPage()
{
    if(m_document && m_index != 1)
    {
        m_index = 1;

        emit indexChanged(m_index);
    }
}

void DocumentView::lastPage()
{
    if(m_document)
    {
        switch(m_displayMode)
        {
        case PagingMode:
        case ScrollingMode:
            if(m_index != m_pageList.size())
            {
                m_index = m_pageList.size();

                emit indexChanged(m_index);
            }
            break;
        case DoublePagingMode:
        case DoubleScrollingMode:
            if(m_pageList.size()%2==0)
            {
                if(m_index != m_pageList.size()-1)
                {
                    m_index = m_pageList.size()-1;

                    emit indexChanged(m_index);
                }
            }
            else
            {
                if(m_index != m_pageList.size())
                {
                    m_index = m_pageList.size();

                    emit indexChanged(m_index);
                }
            }
            break;
        }
    }
}


void DocumentView::setDisplayMode(const DocumentView::DisplayModes &displayMode)
{
    if(m_displayMode != displayMode)
    {
        m_displayMode = displayMode;

        emit displayModeChanged(m_displayMode);

        if(m_displayMode == DoublePagingMode || m_displayMode == DoubleScrollingMode)
        {
            if(m_index%2==0)
            {
                m_index = m_index-1;

                emit indexChanged(m_index);
            }
        }

        layout();
    }
}


void DocumentView::setScaleMode(const DocumentView::ScaleModes &scaleMode)
{
    if(m_scaleMode != scaleMode)
    {
        m_scaleMode = scaleMode;

        emit scaleModeChanged(m_scaleMode);
    }
}

void DocumentView::setScaleFactor(const qreal &scaleFactor)
{
    if(m_scaleFactor != scaleFactor && scaleFactor >= 0.25 && scaleFactor <= 4.0)
    {
        m_scaleFactor = scaleFactor;

        emit scaleFactorChanged(m_scaleFactor);
    }
}


void DocumentView::setRotationMode(const DocumentView::RotationModes &rotationMode)
{
    if(m_rotationMode != rotationMode)
    {
        m_rotationMode = rotationMode;

        emit rotationModeChanged(m_rotationMode);
    }
}


void DocumentView::layout()
{
    m_graphicsScene->clear();

    qreal x = 0.0;
    qreal y = 0.0;

    for(int index=0;index<m_pageList.size();index++)
    {
        Poppler::Page *page = m_pageList[index];

        PageItem *pageItem = new PageItem(page);
        m_graphicsScene->addItem(pageItem);

        pageItem->setPos(x, y);

        y += 1.05 * page->pageSizeF().height();
    }
}
