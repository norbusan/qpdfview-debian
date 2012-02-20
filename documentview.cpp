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

DocumentView::DocumentView(QWidget *parent) : QWidget(parent),
    m_document(0),m_filePath(),m_currentPage(-1),m_numberOfPages(-1),m_scaling(ScaleTo100),m_rotation(RotateBy0),m_twoPageSpread(false)
{
    m_graphicsScene = new QGraphicsScene(this);
    m_graphicsScene->setBackgroundBrush(QBrush(Qt::darkGray));

    m_graphicsView = new QGraphicsView(m_graphicsScene, this);
    m_graphicsView->setInteractive(false);

    this->setLayout(new QHBoxLayout());
    this->layout()->addWidget(m_graphicsView);

    m_graphicsView->show();
}

DocumentView::~DocumentView()
{
    delete m_graphicsScene;
    delete m_graphicsView;

    if(m_document)
    {
        delete m_document;
    }
}


QString DocumentView::filePath() const
{
    return m_filePath;
}

int DocumentView::currentPage() const
{
    return m_currentPage;
}

void DocumentView::setCurrentPage(const int &currentPage)
{
    if(m_document)
    {
        if(m_currentPage != currentPage && currentPage >= 1 &&  currentPage <= m_numberOfPages)
        {
            if(m_twoPageSpread)
            {
                if(currentPage % 2 == 0)
                {
                    m_currentPage = currentPage-1;
                }
                else
                {
                    m_currentPage = currentPage;
                }
            }
            else
            {
                m_currentPage = currentPage;
            }
        }

        emit currentPageChanged(m_currentPage);
    }
}

int DocumentView::numberOfPages() const
{
    return m_numberOfPages;
}

DocumentView::Scaling DocumentView::scaling() const
{
    return m_scaling;
}

void DocumentView::setScaling(const Scaling &scaling)
{
    if(m_scaling != scaling)
    {
        m_scaling = scaling;

        emit scalingChanged(m_scaling);

        preparePages();
    }
}

DocumentView::Rotation DocumentView::rotation() const
{
    return m_rotation;
}

void DocumentView::setRotation(const Rotation &rotation)
{
    if(m_rotation != rotation)
    {
        m_rotation = rotation;

        emit rotationChanged(m_rotation);

        preparePages();
    }
}

bool DocumentView::twoPageSpread() const
{
    return m_twoPageSpread;
}

void DocumentView::setTwoPageSpread(const bool &twoPageSpread)
{
    if(m_twoPageSpread != twoPageSpread)
    {
        m_twoPageSpread = twoPageSpread;

        emit twoPageSpreadChanged(m_twoPageSpread);

        if(m_twoPageSpread && m_currentPage % 2 == 0)
        {
            m_currentPage -= 1;

            emit currentPageChanged(m_currentPage);
        }

        preparePages();
    }
}


bool DocumentView::open(const QString &filePath)
{
    Poppler::Document *document = Poppler::Document::load(filePath);

    if(document)
    {
        if(m_document) { delete m_document; }

        m_document = document;

        m_filePath = filePath;
        m_currentPage = 1;
        m_numberOfPages = m_document->numPages();

        emit filePathChanged(m_filePath);
        emit currentPageChanged(m_currentPage);
        emit numberOfPagesChanged(m_numberOfPages);

        document->setRenderHint(Poppler::Document::Antialiasing);
        document->setRenderHint(Poppler::Document::TextAntialiasing);

        preparePages();
    }

    return document != 0;
}

bool DocumentView::refresh()
{
    if(m_document)
    {
        Poppler::Document *document = Poppler::Document::load(m_filePath);

        if(document)
        {
            if(m_document) { delete m_document; }

            m_document = document;

            if(m_currentPage > document->numPages())
            {
                m_currentPage = 1;
            }
            m_numberOfPages = document->numPages();

            emit currentPageChanged(m_currentPage);
            emit numberOfPagesChanged(m_numberOfPages);

            document->setRenderHint(Poppler::Document::Antialiasing);
            document->setRenderHint(Poppler::Document::TextAntialiasing);

            preparePages();
        }

        return document != 0;
    }
    else
    {
        return false;
    }
}

void DocumentView::previousPage()
{
    if(m_document)
    {
        if(m_twoPageSpread)
        {
            if(m_currentPage > 2)
            {
                m_currentPage -= 2;

                emit currentPageChanged(m_currentPage);
            }
        }
        else
        {
            if(m_currentPage > 1)
            {
                m_currentPage -= 1;

                emit currentPageChanged(m_currentPage);
            }
        }
    }
}

void DocumentView::nextPage()
{
    if(m_document)
    {
        if(m_twoPageSpread)
        {
            if(m_currentPage <= m_numberOfPages-2)
            {
                m_currentPage += 2;

                emit currentPageChanged(m_currentPage);
            }
        }
        else
        {
            if(m_currentPage <= m_numberOfPages-1)
            {
                m_currentPage += 1;

                emit currentPageChanged(m_currentPage);
            }
        }
    }
}

void DocumentView::firstPage()
{
    if(m_document)
    {
        if(m_currentPage != 1)
        {
            m_currentPage = 1;

            emit currentPageChanged(m_currentPage);
        }
    }
}

void DocumentView::lastPage()
{
    if(m_document)
    {
        if(m_twoPageSpread)
        {
            if(m_numberOfPages % 2 == 0)
            {
                if(m_currentPage != m_numberOfPages-1)
                {
                    m_currentPage = m_numberOfPages-1;

                    emit currentPageChanged(m_currentPage);
                }
            }
            else
            {
                if(m_currentPage != m_numberOfPages)
                {
                    m_currentPage = m_numberOfPages;

                    emit currentPageChanged(m_currentPage);
                }
            }
        }
        else
        {
            if(m_currentPage != m_numberOfPages)
            {
                m_currentPage = m_numberOfPages;

                emit currentPageChanged(m_currentPage);
            }
        }
    }
}


void DocumentView::preparePages()
{
    m_graphicsScene->clear();

    if(m_document)
    {
        qreal sceneWidth = 0.0, sceneHeight = 5.0, currentPageX = 0.0, currentPageY = 0.0;

        if(m_twoPageSpread)
        {
            if(m_numberOfPages % 2 == 0)
            {
                for(int i=0;i<m_document->numPages();i+=2)
                {
                    Poppler::Page *leftPage = m_document->page(i);
                    Poppler::Page *rightPage = m_document->page(i+1);

                    qreal leftPageWidth = this->physicalDpiX()/72.0 * leftPage->pageSizeF().width();
                    qreal leftPageHeight = this->physicalDpiY()/72.0 * leftPage->pageSizeF().height();

                    qreal rightPageWidth = this->physicalDpiX()/72.0 * rightPage->pageSizeF().width();
                    qreal rightPageHeight = this->physicalDpiY()/72.0 * rightPage->pageSizeF().height();

                    m_graphicsScene->addRect(10.0, sceneHeight+10.0, leftPageWidth, leftPageHeight);
                    m_graphicsScene->addRect(leftPageWidth+20.0, sceneHeight+10.0, rightPageWidth, rightPageHeight);

                    if(m_currentPage == i+1)
                    {
                        currentPageX = 10.0 + 0.5 * leftPageWidth;
                        currentPageY = sceneHeight + 10.0 + 0.5 * leftPageHeight;
                    }

                    sceneWidth = qMax(sceneWidth, leftPageWidth+rightPageWidth+30.0);
                    sceneHeight += qMax(leftPageHeight, rightPageHeight)+10.0;
                }

                m_graphicsScene->setSceneRect(0.0, 0.0, sceneWidth, sceneHeight);
                m_graphicsView->centerOn(currentPageX, currentPageY);
            }
            else
            {
                for(int i=0;i<m_document->numPages()-1;i+=2)
                {
                    Poppler::Page *leftPage = m_document->page(i);
                    Poppler::Page *rightPage = m_document->page(i+1);

                    qreal leftPageWidth = this->physicalDpiX()/72.0 * leftPage->pageSizeF().width();
                    qreal leftPageHeight = this->physicalDpiY()/72.0 * leftPage->pageSizeF().height();

                    qreal rightPageWidth = this->physicalDpiX()/72.0 * rightPage->pageSizeF().width();
                    qreal rightPageHeight = this->physicalDpiY()/72.0 * rightPage->pageSizeF().height();

                    m_graphicsScene->addRect(10.0, sceneHeight+10.0, leftPageWidth, leftPageHeight);
                    m_graphicsScene->addRect(leftPageWidth+20.0, sceneHeight+10.0, rightPageWidth, rightPageHeight);

                    if(m_currentPage == i+1)
                    {
                        currentPageX = 10.0 + 0.5 * leftPageWidth;
                        currentPageY = sceneHeight + 10.0 + 0.5 * leftPageHeight;
                    }

                    sceneWidth = qMax(sceneWidth, leftPageWidth+rightPageWidth+30.0);
                    sceneHeight += qMax(leftPageHeight, rightPageHeight)+10.0;
                }

                Poppler::Page *lastPage = m_document->page(m_document->numPages()-1);

                qreal lastPageWidth = this->physicalDpiX()/72.0 * lastPage->pageSizeF().width();
                qreal lastPageHeight = this->physicalDpiY()/72.0 * lastPage->pageSizeF().height();

                m_graphicsScene->addRect(10.0, sceneHeight+10.0, lastPageWidth, lastPageHeight);

                if(m_currentPage == m_numberOfPages)
                {
                    currentPageX = 10.0 + 0.5 * lastPageWidth;
                    currentPageY = sceneHeight + 10.0 + 0.5 * lastPageHeight;
                }

                sceneWidth = qMax(sceneWidth, lastPageWidth+20.0);
                sceneHeight += lastPageHeight+10.0;

                m_graphicsScene->setSceneRect(0.0, 0.0, sceneWidth, sceneHeight);
                m_graphicsView->centerOn(currentPageX, currentPageY);
            }
        }
        else
        {
            for(int i=0;i<m_document->numPages();i++)
            {
                Poppler::Page *page = m_document->page(i);

                qreal pageWidth = this->physicalDpiX()/72.0 * page->pageSizeF().width();
                qreal pageHeight = this->physicalDpiY()/72.0 * page->pageSizeF().height();

                m_graphicsScene->addRect(10.0, sceneHeight+10.0, pageWidth, pageHeight);

                if(m_currentPage == i+1)
                {
                    currentPageX = 10.0 + 0.5 * pageWidth;
                    currentPageY = sceneHeight + 10.0 + 0.5 * pageHeight;
                }

                sceneWidth = qMax(sceneWidth, pageWidth+20.0);
                sceneHeight += pageHeight+10.0;
            }

            m_graphicsScene->setSceneRect(0.0, 0.0, sceneWidth, sceneHeight);
            m_graphicsView->centerOn(currentPageX, currentPageY);
        }
    }
}


void DocumentView::wheelEvent(QWheelEvent *wheelEvent)
{
}
