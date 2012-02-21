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
    m_document(0),m_numberToObject(),m_heightToNumber(),m_filePath(),m_currentPage(-1),m_numberOfPages(-1),m_pageLayout(OnePage),m_scaling(ScaleTo100),m_rotation(RotateBy0)
{
    m_graphicsScene = new QGraphicsScene(this);
    m_graphicsScene->setBackgroundBrush(QBrush(Qt::darkGray));

    m_graphicsView = new QGraphicsView(m_graphicsScene, this);
    m_graphicsView->setInteractive(false);

    connect(m_graphicsView->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(changeCurrentPage(int)));

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
            switch(m_pageLayout)
            {
            case OnePage:
            case OneColumn:
                m_currentPage = currentPage;
                break;
            case TwoPages:
            case TwoColumns:
                if(currentPage % 2 == 0)
                {
                    m_currentPage = currentPage-1;
                }
                else
                {
                    m_currentPage = currentPage;
                }
                break;
            }

            emit currentPageChanged(m_currentPage);

            prepareView();
        }
    }
}

int DocumentView::numberOfPages() const
{
    return m_numberOfPages;
}

DocumentView::PageLayout DocumentView::pageLayout() const
{
    return m_pageLayout;
}

void DocumentView::setPageLayout(const DocumentView::PageLayout &pageLayout)
{
    if(m_pageLayout != pageLayout)
    {
        m_pageLayout = pageLayout;

        if((m_pageLayout == TwoPages || m_pageLayout == TwoColumns) && m_currentPage % 2 == 0)
        {
            m_currentPage -= 1;

            emit currentPageChanged(m_currentPage);
        }

        emit pageLayoutChanged(m_pageLayout);

        prepareScene();
        prepareView();
    }
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

        prepareScene();
        prepareView();
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

        prepareScene();
        prepareView();
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

        prepareScene();
        prepareView();
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

            prepareScene();
            prepareView();
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
        switch(m_pageLayout)
        {
        case OnePage:
        case OneColumn:
            if(m_currentPage > 1)
            {
                m_currentPage -= 1;

                emit currentPageChanged(m_currentPage);

                prepareView();
            }
            break;
        case TwoPages:
        case TwoColumns:
            if(m_currentPage > 2)
            {
                m_currentPage -= 2;

                emit currentPageChanged(m_currentPage);

                prepareView();
            }
            break;
        }
    }
}

void DocumentView::nextPage()
{
    if(m_document)
    {
        switch(m_pageLayout)
        {
        case OnePage:
        case OneColumn:
            if(m_currentPage <= m_numberOfPages-1)
            {
                m_currentPage += 1;

                emit currentPageChanged(m_currentPage);

                prepareView();
            }
            break;
        case TwoPages:
        case TwoColumns:
            if(m_currentPage <= m_numberOfPages-2)
            {
                m_currentPage += 2;

                emit currentPageChanged(m_currentPage);

                prepareView();
            }
            break;
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

            prepareView();
        }
    }
}

void DocumentView::lastPage()
{
    if(m_document)
    {
        switch(m_pageLayout)
        {
        case OnePage:
        case OneColumn:
            if(m_currentPage != m_numberOfPages)
            {
                m_currentPage = m_numberOfPages;

                emit currentPageChanged(m_currentPage);

                prepareView();
            }
            break;
        case TwoPages:
        case TwoColumns:
            if(m_numberOfPages % 2 == 0)
            {
                if(m_currentPage != m_numberOfPages-1)
                {
                    m_currentPage = m_numberOfPages-1;

                    emit currentPageChanged(m_currentPage);

                    prepareView();
                }
            }
            else
            {
                if(m_currentPage != m_numberOfPages)
                {
                    m_currentPage = m_numberOfPages;

                    emit currentPageChanged(m_currentPage);

                    prepareView();
                }
            }
            break;
        }
    }
}


void DocumentView::prepareScene()
{
    m_graphicsScene->clear();
    m_numberToObject.clear();
    m_heightToNumber.clear();

    PageObject::pageCache.clear();

    if(m_document)
    {
        Poppler::Page::Rotation pageRotation = Poppler::Page::Rotate0;

        switch(m_rotation)
        {
        case RotateBy0:
            pageRotation = Poppler::Page::Rotate0;
            break;
        case RotateBy90:
            pageRotation = Poppler::Page::Rotate90;
            break;
        case RotateBy180:
            pageRotation = Poppler::Page::Rotate180;
            break;
        case RotateBy270:
            pageRotation = Poppler::Page::Rotate270;
            break;
        }

        qreal resolutionX = this->physicalDpiX(), resolutionY = this->physicalDpiX(), scaleFactor = 4.0;

        switch(m_scaling)
        {
        case FitToPage:
        case FitToPageWidth:
            switch(m_pageLayout)
            {
            case OnePage:
            case OneColumn:
                for(int i = 0; i < m_numberOfPages; i++)
                {
                    qreal currentPageWidth = 0.0, currentPageHeight = 0.0;
                    Poppler::Page *currentPage = m_document->page(i);

                    if(pageRotation == Poppler::Page::Rotate90 || pageRotation == Poppler::Page::Rotate270)
                    {
                        currentPageWidth = qCeil(resolutionX * currentPage->pageSizeF().height() / 72.0);
                        currentPageHeight = qCeil(resolutionY * currentPage->pageSizeF().width() / 72.0);
                    }
                    else
                    {
                        currentPageWidth = qCeil(resolutionX * currentPage->pageSizeF().width() / 72.0);
                        currentPageHeight = qCeil(resolutionY * currentPage->pageSizeF().height() / 72.0);
                    }

                    delete currentPage;

                    scaleFactor = qMin(scaleFactor, 0.95 * static_cast<qreal>(m_graphicsView->width()) / (currentPageWidth + 20.0));
                    if(m_scaling == FitToPage)
                    {
                        scaleFactor = qMin(scaleFactor, 0.95 * static_cast<qreal>(m_graphicsView->height()) / (currentPageHeight + 20.0));
                    }
                }
                break;
            case TwoPages:
            case TwoColumns:
                if(m_numberOfPages % 2 == 0)
                {
                    for(int i = 0; i < m_numberOfPages; i += 2)
                    {
                        qreal currentPageWidth = 0.0, currentPageHeight = 0.0;
                        Poppler::Page *currentPage = m_document->page(i);

                        qreal nextPageWidth = 0.0, nextPageHeight = 0.0;
                        Poppler::Page *nextPage = m_document->page(i+1);

                        if(pageRotation == Poppler::Page::Rotate90 || pageRotation == Poppler::Page::Rotate270)
                        {
                            currentPageWidth = qCeil(resolutionX * currentPage->pageSizeF().height() / 72.0);
                            currentPageHeight = qCeil(resolutionY * currentPage->pageSizeF().width() / 72.0);
                            nextPageWidth = qCeil(resolutionX * nextPage->pageSizeF().height() / 72.0);
                            nextPageHeight = qCeil(resolutionY * nextPage->pageSizeF().width() / 72.0);
                        }
                        else
                        {
                            currentPageWidth = qCeil(resolutionX * currentPage->pageSizeF().width() / 72.0);
                            currentPageHeight = qCeil(resolutionY * currentPage->pageSizeF().height() / 72.0);
                            nextPageWidth = qCeil(resolutionX * nextPage->pageSizeF().width() / 72.0);
                            nextPageHeight = qCeil(resolutionY * nextPage->pageSizeF().height() / 72.0);
                        }

                        delete currentPage;
                        delete nextPage;

                        scaleFactor = qMin(scaleFactor, 0.95 * static_cast<qreal>(m_graphicsView->width()) / (currentPageWidth + nextPageWidth + 30.0));
                        if(m_scaling == FitToPage)
                        {
                            scaleFactor = qMin(scaleFactor, 0.95 * static_cast<qreal>(m_graphicsView->height()) / (qMax(currentPageHeight, nextPageHeight) + 20.0));
                        }
                    }
                }
                else
                {
                    for(int i=0;i<m_numberOfPages-1;i+=2)
                    {
                        qreal currentPageWidth = 0.0, currentPageHeight = 0.0;
                        Poppler::Page *currentPage = m_document->page(i);

                        qreal nextPageWidth = 0.0, nextPageHeight = 0.0;
                        Poppler::Page *nextPage = m_document->page(i+1);

                        if(pageRotation == Poppler::Page::Rotate90 || pageRotation == Poppler::Page::Rotate270)
                        {
                            currentPageWidth = qCeil(resolutionX * currentPage->pageSizeF().height() / 72.0);
                            currentPageHeight = qCeil(resolutionY * currentPage->pageSizeF().width() / 72.0);
                            nextPageWidth = qCeil(resolutionX * nextPage->pageSizeF().height() / 72.0);
                            nextPageHeight = qCeil(resolutionY * nextPage->pageSizeF().width() / 72.0);
                        }
                        else
                        {
                            currentPageWidth = qCeil(resolutionX * currentPage->pageSizeF().width() / 72.0);
                            currentPageHeight = qCeil(resolutionY * currentPage->pageSizeF().height() / 72.0);
                            nextPageWidth = qCeil(resolutionX * nextPage->pageSizeF().width() / 72.0);
                            nextPageHeight = qCeil(resolutionY * nextPage->pageSizeF().height() / 72.0);
                        }

                        delete currentPage;
                        delete nextPage;

                        scaleFactor = qMin(scaleFactor, 0.95 * static_cast<qreal>(m_graphicsView->width()) / (currentPageWidth + nextPageWidth + 30.0));
                        if(m_scaling == FitToPage)
                        {
                            scaleFactor = qMin(scaleFactor, 0.95 * static_cast<qreal>(m_graphicsView->height()) / (qMax(currentPageHeight, nextPageHeight) + 20.0));
                        }
                    }

                    qreal currentPageWidth = 0.0, currentPageHeight = 0.0;
                    Poppler::Page *currentPage = m_document->page(m_numberOfPages-1);

                    if(pageRotation == Poppler::Page::Rotate90 || pageRotation == Poppler::Page::Rotate270)
                    {
                        currentPageWidth = qCeil(resolutionX * currentPage->pageSizeF().height() / 72.0);
                        currentPageHeight = qCeil(resolutionY * currentPage->pageSizeF().width() / 72.0);
                    }
                    else
                    {
                        currentPageWidth = qCeil(resolutionX * currentPage->pageSizeF().width() / 72.0);
                        currentPageHeight = qCeil(resolutionY * currentPage->pageSizeF().height() / 72.0);
                    }

                    delete currentPage;

                    scaleFactor = qMin(scaleFactor, 0.95 * static_cast<qreal>(m_graphicsView->width()) / (currentPageWidth + 20.0));
                    if(m_scaling == FitToPage)
                    {
                        scaleFactor = qMin(scaleFactor, 0.95 * static_cast<qreal>(m_graphicsView->height()) / (currentPageHeight + 20.0));
                    }
                }
                break;
            }
            break;
        case ScaleTo25:
            scaleFactor = 0.25;
            break;
        case ScaleTo50:
            scaleFactor = 0.50;
            break;
        case ScaleTo100:
            scaleFactor = 1.00;
            break;
        case ScaleTo200:
            scaleFactor = 2.00;
            break;
        case ScaleTo400:
            scaleFactor = 4.00;
            break;
        }

        resolutionX *= scaleFactor;
        resolutionY *= scaleFactor;

        qreal sceneWidth = 20.0, sceneHeight = 10.0;

        switch(m_pageLayout)
        {
        case OnePage:
        case OneColumn:
            for(int i = 0; i < m_document->numPages(); i++)
            {
                Poppler::Page *currentPage = m_document->page(i);
                PageObject *currentPageObject = new PageObject(currentPage, pageRotation);

                currentPageObject->setResolutionX(resolutionX);
                currentPageObject->setResolutionY(resolutionY);
                currentPageObject->setFilePath(m_filePath);
                currentPageObject->setCurrentPage(i+1);

                currentPageObject->setX(10.0);
                currentPageObject->setY(sceneHeight+10.0);

                m_graphicsScene->addItem(currentPageObject);
                m_numberToObject.insert(i+1, currentPageObject);
                m_heightToNumber.insert(-currentPageObject->y(), i+1);

                sceneWidth = qMax(sceneWidth, currentPageObject->boundingRect().width() + 20.0);
                sceneHeight += currentPageObject->boundingRect().height() + 10.0;
            }
            break;
        case TwoPages:
        case TwoColumns:
            if(m_numberOfPages % 2 == 0)
            {
                for(int i = 0; i < m_numberOfPages; i += 2)
                {
                    Poppler::Page *currentPage = m_document->page(i);
                    PageObject *currentPageObject = new PageObject(currentPage, pageRotation);

                    currentPageObject->setResolutionX(resolutionX);
                    currentPageObject->setResolutionY(resolutionY);
                    currentPageObject->setFilePath(m_filePath);
                    currentPageObject->setCurrentPage(i+1);

                    currentPageObject->setX(10.0);
                    currentPageObject->setY(sceneHeight+10.0);

                    m_graphicsScene->addItem(currentPageObject);
                    m_numberToObject.insert(i+1, currentPageObject);
                    m_heightToNumber.insert(-currentPageObject->y(), i+1);

                    Poppler::Page *nextPage = m_document->page(i+1);
                    PageObject *nextPageObject = new PageObject(nextPage, pageRotation);

                    nextPageObject->setResolutionX(resolutionX);
                    nextPageObject->setResolutionY(resolutionY);
                    nextPageObject->setFilePath(m_filePath);
                    nextPageObject->setCurrentPage(i+2);

                    nextPageObject->setX(currentPageObject->boundingRect().width() + 20.0);
                    nextPageObject->setY(sceneHeight+10.0);

                    m_graphicsScene->addItem(nextPageObject);
                    m_numberToObject.insert(i+2, nextPageObject);

                    sceneWidth = qMax(sceneWidth, currentPageObject->boundingRect().width() + nextPageObject->boundingRect().width() + 30.0);
                    sceneHeight += qMax(currentPageObject->boundingRect().height(), nextPageObject->boundingRect().height()) + 10.0;
                }
            }
            else
            {
                for(int i=0;i<m_numberOfPages-1;i+=2)
                {
                    Poppler::Page *currentPage = m_document->page(i);
                    PageObject *currentPageObject = new PageObject(currentPage, pageRotation);

                    currentPageObject->setResolutionX(resolutionX);
                    currentPageObject->setResolutionY(resolutionY);
                    currentPageObject->setFilePath(m_filePath);
                    currentPageObject->setCurrentPage(i+1);

                    currentPageObject->setX(10.0);
                    currentPageObject->setY(sceneHeight+10.0);

                    m_graphicsScene->addItem(currentPageObject);
                    m_numberToObject.insert(i+1, currentPageObject);
                    m_heightToNumber.insert(-currentPageObject->y(), i+1);

                    Poppler::Page *nextPage = m_document->page(i+1);
                    PageObject *nextPageObject = new PageObject(nextPage, pageRotation);

                    nextPageObject->setResolutionX(resolutionX);
                    nextPageObject->setResolutionY(resolutionY);
                    nextPageObject->setFilePath(m_filePath);
                    nextPageObject->setCurrentPage(i+1);

                    nextPageObject->setX(currentPageObject->boundingRect().width() + 20.0);
                    nextPageObject->setY(sceneHeight+10.0);

                    m_graphicsScene->addItem(nextPageObject);
                    m_numberToObject.insert(i+2, nextPageObject);

                    sceneWidth = qMax(sceneWidth, currentPageObject->boundingRect().width() + nextPageObject->boundingRect().width() + 30.0);
                    sceneHeight += qMax(currentPageObject->boundingRect().height(), nextPageObject->boundingRect().height()) + 10.0;
                }

                Poppler::Page *currentPage = m_document->page(m_numberOfPages-1);
                PageObject *currentPageObject = new PageObject(currentPage, pageRotation);

                currentPageObject->setResolutionX(resolutionX);
                currentPageObject->setResolutionY(resolutionY);
                currentPageObject->setFilePath(m_filePath);
                currentPageObject->setCurrentPage(m_numberOfPages);

                currentPageObject->setX(10.0);
                currentPageObject->setY(sceneHeight+10.0);

                m_graphicsScene->addItem(currentPageObject);
                m_numberToObject.insert(m_numberOfPages, currentPageObject);
                m_heightToNumber.insert(-currentPageObject->y(), m_numberOfPages);

                sceneWidth = qMax(sceneWidth, currentPageObject->boundingRect().width() + 20.0);
                sceneHeight += currentPageObject->boundingRect().height() + 10.0;
            }
            break;
        }

        m_graphicsScene->setSceneRect(0.0, 0.0, sceneWidth, sceneHeight);
    }
}

void DocumentView::prepareView()
{
    PageObject *currentPageObject = m_numberToObject.value(m_currentPage);
    PageObject *nextPageObject = m_numberToObject.value(m_currentPage+1);

    switch(m_pageLayout)
    {
    case OnePage:
        m_graphicsView->setSceneRect(currentPageObject->x()-10.0, currentPageObject->y()-10.0,
                                     currentPageObject->boundingRect().width()+20.0,
                                     currentPageObject->boundingRect().height()+20.0);

        foreach(QGraphicsItem *item, m_graphicsScene->items())
        {
            item->setVisible(false);
        }
        currentPageObject->setVisible(true);
        break;
    case TwoPages:
        if(m_numberOfPages % 2 == 0)
        {
            m_graphicsView->setSceneRect(currentPageObject->x()-10.0, currentPageObject->y()-10.0,
                                         currentPageObject->boundingRect().width() + nextPageObject->boundingRect().width() + 30.0,
                                         qMax(currentPageObject->boundingRect().height(), nextPageObject->boundingRect().height()) + 20.0);
        }
        else
        {
            if(m_currentPage < m_numberOfPages)
            {
                m_graphicsView->setSceneRect(currentPageObject->x()-10.0, currentPageObject->y()-10.0,
                                             currentPageObject->boundingRect().width() + nextPageObject->boundingRect().width() + 30.0,
                                             qMax(currentPageObject->boundingRect().height(), nextPageObject->boundingRect().height()) + 20.0);
            }
            else
            {
                m_graphicsView->setSceneRect(currentPageObject->x()-10.0, currentPageObject->y()-10.0,
                                             currentPageObject->boundingRect().width()+20.0,
                                             currentPageObject->boundingRect().height()+20.0);
            }
        }

        foreach(QGraphicsItem *item, m_graphicsScene->items())
        {
            item->setVisible(false);
        }
        currentPageObject->setVisible(true);
        nextPageObject->setVisible(true);
        break;
    case OneColumn:
    case TwoColumns:
        m_graphicsView->setSceneRect(QRectF());
        break;
    }

    disconnect(m_graphicsView->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(changeCurrentPage(int)));
    m_graphicsView->centerOn(currentPageObject);
    connect(m_graphicsView->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(changeCurrentPage(int)));
}


void DocumentView::changeCurrentPage(const int &value)
{
    if(m_document)
    {
        int visiblePage = -1;

        switch(m_pageLayout)
        {
        case OnePage:
        case TwoPages:
            break;
        case OneColumn:
        case TwoColumns:
            visiblePage = m_heightToNumber.lowerBound(static_cast<qreal>(-value)).value();

            if(m_currentPage != visiblePage) {
                m_currentPage = visiblePage;

                emit currentPageChanged(m_currentPage);
            }
        }
    }
}


void DocumentView::wheelEvent(QWheelEvent *wheelEvent)
{
}
