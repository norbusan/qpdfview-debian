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
    m_settings(),m_document(0),m_pageToPageObject(),m_valueToPage(),m_filePath(),m_currentPage(-1),m_numberOfPages(-1),m_pageLayout(OnePage),m_scaling(ScaleTo100),m_rotation(RotateBy0)
{
    m_graphicsScene = new QGraphicsScene(this);
    m_graphicsScene->setBackgroundBrush(QBrush(Qt::darkGray));

    m_graphicsView = new QGraphicsView(m_graphicsScene, this);
    m_graphicsView->setInteractive(false);

    this->setLayout(new QHBoxLayout());
    this->layout()->addWidget(m_graphicsView);

    m_graphicsView->show();

    connect(m_graphicsView->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(changeCurrentPage(int)));

    m_pageLayout = static_cast<PageLayout>(m_settings.value("documentView/pageLayout", 0).toUInt());
    m_scaling = static_cast<Scaling>(m_settings.value("documentView/scaling", 4).toUInt());
    m_rotation = static_cast<Rotation>(m_settings.value("documentView/rotation", 0).toUInt());
}

DocumentView::~DocumentView()
{
    disconnect(m_graphicsView->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(changeCurrentPage(int)));

    delete m_graphicsView;
    delete m_graphicsScene;

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
            prefetch();
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
        prefetch();

        m_settings.setValue("documentView/pageLayout", static_cast<uint>(m_pageLayout));
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
        prefetch();

        m_settings.setValue("documentView/scaling", static_cast<uint>(m_scaling));
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
        prefetch();

        m_settings.setValue("documentView/rotation", static_cast<uint>(m_rotation));
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
        prefetch();
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
            prefetch();
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
                prefetch();
            }
            break;
        case TwoPages:
        case TwoColumns:
            if(m_currentPage > 2)
            {
                m_currentPage -= 2;

                emit currentPageChanged(m_currentPage);

                prepareView();
                prefetch();
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
                prefetch();
            }
            break;
        case TwoPages:
        case TwoColumns:
            if(m_currentPage <= m_numberOfPages-2)
            {
                m_currentPage += 2;

                emit currentPageChanged(m_currentPage);

                prepareView();
                prefetch();
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
            prefetch();
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
                prefetch();
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
                    prefetch();
                }
            }
            else
            {
                if(m_currentPage != m_numberOfPages)
                {
                    m_currentPage = m_numberOfPages;

                    emit currentPageChanged(m_currentPage);

                    prepareView();
                    prefetch();
                }
            }
            break;
        }
    }
}

bool DocumentView::findNext(const QString &text)
{
    if(m_document)
    {
        bool result = false;

        for(int page = m_currentPage; page <= m_numberOfPages; page++)
        {
            qDebug() << "search from the current page:" << page;

            PageObject *pageObject = m_pageToPageObject.value(page);
            result = pageObject->findNext(text);

            if(result)
            {
                if(page != m_currentPage) { m_pageToPageObject.value(m_currentPage)->clearHighlight(); }

                this->setCurrentPage(page);

                disconnect(m_graphicsView->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(changeCurrentPage(int)));

                m_graphicsView->centerOn(pageObject->highlight().center());

                connect(m_graphicsView->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(changeCurrentPage(int)));

                break;
            }
        }

        if(!result)
        {
            for(int page = 1; page < m_currentPage; page++)
            {
                qDebug() << "search up to the current page:" << page;

                PageObject *pageObject = m_pageToPageObject.value(page);
                result = pageObject->findNext(text);

                if(result)
                {
                    if(page != m_currentPage) { m_pageToPageObject.value(m_currentPage)->clearHighlight(); }

                    this->setCurrentPage(page);

                    disconnect(m_graphicsView->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(changeCurrentPage(int)));

                    m_graphicsView->centerOn(pageObject->highlight().center());

                    connect(m_graphicsView->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(changeCurrentPage(int)));

                    break;
                }
            }
        }

        return result;
    }
    else
    {
        return false;
    }
}

void DocumentView::clearHighlights()
{
    for(int page = 1; page <= m_numberOfPages; page++)
    {
        PageObject *pageObject = m_pageToPageObject.value(page);

        pageObject->clearHighlight();
    }
}


void DocumentView::prepareScene()
{
    m_graphicsScene->clear();

    m_pageToPageObject.clear();
    m_valueToPage.clear();

    if(m_document)
    {
        qreal pageWidth = 0.0, pageHeight = 0.0;
        qreal resolutionX = this->physicalDpiX(), resolutionY = this->physicalDpiX(), scaleFactor = 4.0;
        qreal sceneWidth = 0.0, sceneHeight = 0.0;

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
                    Poppler::Page *page = m_document->page(i);

                    if(m_rotation == RotateBy90 || m_rotation == RotateBy270)
                    {
                        pageWidth = resolutionX * page->pageSizeF().height() / 72.0;
                        pageHeight = resolutionY * page->pageSizeF().width() / 72.0;
                    }
                    else
                    {
                        pageWidth = resolutionX * page->pageSizeF().width() / 72.0;
                        pageHeight = resolutionY * page->pageSizeF().height() / 72.0;
                    }

                    delete page;

                    scaleFactor = qMin(scaleFactor, 0.95 * static_cast<qreal>(m_graphicsView->width()) / (pageWidth + 20.0));
                    if(m_scaling == FitToPage)
                    {
                        scaleFactor = qMin(scaleFactor, 0.95 * static_cast<qreal>(m_graphicsView->height()) / (pageHeight + 20.0));
                    }
                }
                break;
            case TwoPages:
            case TwoColumns:
                for(int i = 0; i < (m_numberOfPages % 2 != 0 ? m_numberOfPages-1 : m_numberOfPages); i += 2)
                {
                    Poppler::Page *page = m_document->page(i);

                    if(m_rotation == RotateBy90 || m_rotation == RotateBy270)
                    {
                        pageWidth = resolutionX * page->pageSizeF().height() / 72.0;
                        pageHeight = resolutionY * page->pageSizeF().width() / 72.0;
                    }
                    else
                    {
                        pageWidth = resolutionX * page->pageSizeF().width() / 72.0;
                        pageHeight = resolutionY * page->pageSizeF().height() / 72.0;
                    }

                    delete page;

                    page = m_document->page(i+1);

                    if(m_rotation == RotateBy90 || m_rotation == RotateBy270)
                    {
                        pageWidth += resolutionX * page->pageSizeF().height() / 72.0;
                        pageHeight = qMax(pageHeight, resolutionY * page->pageSizeF().width() / 72.0);
                    }
                    else
                    {
                        pageWidth += resolutionX * page->pageSizeF().width() / 72.0;
                        pageHeight += qMax(pageHeight, resolutionY * page->pageSizeF().height() / 72.0);
                    }

                    delete page;

                    scaleFactor = qMin(scaleFactor, 0.95 * static_cast<qreal>(m_graphicsView->width()) / (pageWidth + 30.0));
                    if(m_scaling == FitToPage)
                    {
                        scaleFactor = qMin(scaleFactor, 0.95 * static_cast<qreal>(m_graphicsView->height()) / (pageHeight + 20.0));
                    }
                }

                if(m_numberOfPages % 2 != 0)
                {
                    Poppler::Page *page = m_document->page(m_numberOfPages-1);

                    if(m_rotation == RotateBy90 || m_rotation == RotateBy270)
                    {
                        pageWidth = resolutionX * page->pageSizeF().height() / 72.0;
                        pageHeight = resolutionY * page->pageSizeF().width() / 72.0;
                    }
                    else
                    {
                        pageWidth = resolutionX * page->pageSizeF().width() / 72.0;
                        pageHeight = resolutionY * page->pageSizeF().height() / 72.0;
                    }

                    delete page;

                    scaleFactor = qMin(scaleFactor, 0.95 * static_cast<qreal>(m_graphicsView->width()) / (pageWidth + 20.0));
                    if(m_scaling == FitToPage)
                    {
                        scaleFactor = qMin(scaleFactor, 0.95 * static_cast<qreal>(m_graphicsView->height()) / (pageHeight + 20.0));
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

        switch(m_pageLayout)
        {
        case OnePage:
        case OneColumn:
            for(int i = 0; i < m_numberOfPages; i++)
            {
                PageObject *page = new PageObject(m_document->page(i));

                page->setResolutionX(resolutionX);
                page->setResolutionY(resolutionY);
                page->setRotation(static_cast<uint>(m_rotation));
                page->setFilePath(m_filePath);
                page->setCurrentPage(i+1);

                page->setX(10.0);
                page->setY(sceneHeight+10.0);

                m_graphicsScene->addItem(page);
                m_pageToPageObject.insert(i+1, page);
                m_valueToPage.insert(-static_cast<int>(page->y()), i+1);

                sceneWidth = qMax(sceneWidth, page->boundingRect().width() + 20.0);
                sceneHeight += page->boundingRect().height() + 10.0;
            }
            break;
        case TwoPages:
        case TwoColumns:
            for(int i = 0; i < (m_numberOfPages % 2 != 0 ? m_numberOfPages-1 : m_numberOfPages); i += 2)
            {
                PageObject *leftPage = new PageObject(m_document->page(i));

                leftPage->setResolutionX(resolutionX);
                leftPage->setResolutionY(resolutionY);
                leftPage->setRotation(static_cast<uint>(m_rotation));
                leftPage->setFilePath(m_filePath);
                leftPage->setCurrentPage(i+1);

                leftPage->setX(10.0);
                leftPage->setY(sceneHeight+10.0);

                m_graphicsScene->addItem(leftPage);
                m_pageToPageObject.insert(i+1, leftPage);
                m_valueToPage.insert(-static_cast<int>(leftPage->y()), i+1);

                PageObject *rightPage = new PageObject(m_document->page(i+1));

                rightPage->setResolutionX(resolutionX);
                rightPage->setResolutionY(resolutionY);
                rightPage->setRotation(static_cast<uint>(m_rotation));
                rightPage->setFilePath(m_filePath);
                rightPage->setCurrentPage(i+2);

                rightPage->setX(leftPage->boundingRect().width() + 20.0);
                rightPage->setY(sceneHeight+10.0);

                m_graphicsScene->addItem(rightPage);
                m_pageToPageObject.insert(i+2, rightPage);

                sceneWidth = qMax(sceneWidth, leftPage->boundingRect().width() + rightPage->boundingRect().width() + 30.0);
                sceneHeight += qMax(leftPage->boundingRect().height(), rightPage->boundingRect().height()) + 10.0;
            }

            if(m_numberOfPages % 2 != 0)
            {
                PageObject *leftPage = new PageObject(m_document->page(m_numberOfPages-1));

                leftPage->setResolutionX(resolutionX);
                leftPage->setResolutionY(resolutionY);
                leftPage->setRotation(static_cast<uint>(m_rotation));
                leftPage->setFilePath(m_filePath);
                leftPage->setCurrentPage(m_numberOfPages);

                leftPage->setX(10.0);
                leftPage->setY(sceneHeight+10.0);

                m_graphicsScene->addItem(leftPage);
                m_pageToPageObject.insert(m_numberOfPages, leftPage);
                m_valueToPage.insert(-leftPage->y(), m_numberOfPages);

                sceneWidth = qMax(sceneWidth, leftPage->boundingRect().width() + 20.0);
                sceneHeight += leftPage->boundingRect().height() + 10.0;
            }
            break;
        }

        m_graphicsScene->setSceneRect(0.0, 0.0, sceneWidth, sceneHeight);
    }
}

void DocumentView::prepareView()
{
    PageObject *page = m_pageToPageObject.value(m_currentPage);


    switch(m_pageLayout)
    {
    case OnePage:
        foreach(QGraphicsItem *item, m_graphicsScene->items())
        {
            item->setVisible(false);
        }

        m_graphicsView->setSceneRect(page->x()-10.0, page->y()-10.0,
                                     page->boundingRect().width()+20.0,
                                     page->boundingRect().height()+20.0);

        page->setVisible(true);
        break;
    case TwoPages:
        foreach(QGraphicsItem *item, m_graphicsScene->items())
        {
            item->setVisible(false);
        }

        if(m_pageToPageObject.contains(m_currentPage+1))
        {
            PageObject *nextPage = m_pageToPageObject.value(m_currentPage+1);

            m_graphicsView->setSceneRect(page->x()-10.0, page->y()-10.0,
                                         page->boundingRect().width() + nextPage->boundingRect().width() + 30.0,
                                         qMax(page->boundingRect().height(), nextPage->boundingRect().height()) + 20.0);

            page->setVisible(true);
            nextPage->setVisible(true);
        }
        else
        {
            m_graphicsView->setSceneRect(page->x()-10.0, page->y()-10.0,
                                         page->boundingRect().width()+20.0,
                                         page->boundingRect().height()+20.0);

            page->setVisible(true);
        }

        break;
    case OneColumn:
    case TwoColumns:
        m_graphicsView->setSceneRect(QRectF());
        break;
    }

    disconnect(m_graphicsView->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(changeCurrentPage(int)));

    m_graphicsView->centerOn(page);

    connect(m_graphicsView->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(changeCurrentPage(int)));
}

void DocumentView::prefetch()
{
    for(int i = -2; i < 4; i++)
    {
        if(m_pageToPageObject.contains(m_currentPage + i))
        {
            m_pageToPageObject.value(m_currentPage + i)->prefetch();
        }
    }
}


void DocumentView::changeCurrentPage(const int &value)
{
    if(m_document)
    {
        int visiblePage = 1;

        switch(m_pageLayout)
        {
        case OnePage:
        case TwoPages:
            break;
        case OneColumn:
        case TwoColumns:
            if(m_valueToPage.lowerBound(-value) != m_valueToPage.end())
            {
                visiblePage = m_valueToPage.lowerBound(-value).value();
            }

            if(m_currentPage != visiblePage) {
                m_currentPage = visiblePage;

                emit currentPageChanged(m_currentPage);

                prefetch();
            }
        }
    }
}


void DocumentView::resizeEvent(QResizeEvent*)
{
    if(m_document)
    {
        if(m_scaling == FitToPage || m_scaling == FitToPageWidth)
        {
            prepareScene();
            prepareView();
            prefetch();
        }
    }
}

void DocumentView::wheelEvent(QWheelEvent *wheelEvent)
{
    if(m_document)
    {
        switch(m_pageLayout)
        {
        case OnePage:
            if(wheelEvent->delta() > 0 && m_graphicsView->verticalScrollBar()->value() == m_graphicsView->verticalScrollBar()->minimum() && m_currentPage != 1)
            {
                this->previousPage();

                m_graphicsView->verticalScrollBar()->setValue(m_graphicsView->verticalScrollBar()->maximum());
            }
            else if(wheelEvent->delta() < 0 && m_graphicsView->verticalScrollBar()->value() == m_graphicsView->verticalScrollBar()->maximum() && m_currentPage != m_numberOfPages)
            {
                this->nextPage();

                m_graphicsView->verticalScrollBar()->setValue(m_graphicsView->verticalScrollBar()->minimum());
            }
            break;
        case TwoPages:
            if(wheelEvent->delta() > 0 && m_graphicsView->verticalScrollBar()->value() == m_graphicsView->verticalScrollBar()->minimum() && m_currentPage != 1)
            {
                this->previousPage();

                m_graphicsView->verticalScrollBar()->setValue(m_graphicsView->verticalScrollBar()->maximum());
            }
            else if(wheelEvent->delta() < 0 && m_graphicsView->verticalScrollBar()->value() == m_graphicsView->verticalScrollBar()->maximum() && m_currentPage != (m_numberOfPages % 2 == 0 ? m_numberOfPages-1 : m_numberOfPages))
            {
                this->nextPage();

                m_graphicsView->verticalScrollBar()->setValue(m_graphicsView->verticalScrollBar()->minimum());
            }
            break;
        case OneColumn:
        case TwoColumns:
            break;
        }
    }
}
