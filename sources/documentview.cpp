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

#include "documentmodel.h"
#include "pageobject.h"

DocumentView::DocumentView(DocumentModel *model, QWidget *parent) : QWidget(parent),
    m_resolutionX(72.0),m_resolutionY(72.0),m_pageToPageObject(),m_heightToPage(),m_results(),m_currentResult(m_results.end()),m_settings(),m_currentPage(1),m_pageLayout(OnePage),m_scaling(ScaleTo100),m_rotation(RotateBy0),m_highlightAll(false)
{
    m_model = model;

    connect(m_model, SIGNAL(filePathChanged(QString)), this, SLOT(updateFilePath(QString)));
    connect(m_model, SIGNAL(resultsChanged()), this, SLOT(updateResults()));

    // makeCurrentTab

    m_makeCurrentTabAction = new QAction(QFileInfo(m_model->filePath()).completeBaseName(), this);

    connect(m_makeCurrentTabAction, SIGNAL(triggered()), this, SLOT(makeCurrentTab()));

    // settings

    m_pageLayout = static_cast<PageLayout>(m_settings.value("documentView/pageLayout", 0).toUInt());
    m_scaling = static_cast<Scaling>(m_settings.value("documentView/scaling", 4).toUInt());
    m_rotation = static_cast<Rotation>(m_settings.value("documentView/rotation", 0).toUInt());
    m_highlightAll = m_settings.value("documentView/highlightAll", false).toBool();

    // graphics

    m_scene = new QGraphicsScene(this);
    m_view = new QGraphicsView(m_scene, this);

    m_scene->setBackgroundBrush(QBrush(Qt::darkGray));
    m_view->show();

    this->setLayout(new QHBoxLayout());
    this->layout()->addWidget(m_view);

    this->prepareScene();
    this->prepareView();

    connect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(scrollToPage(int)));
}

int DocumentView::currentPage() const
{
    return m_currentPage;
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

        if( ( m_pageLayout == TwoPages || m_pageLayout == TwoColumns ) && m_currentPage % 2 == 0 )
        {
            m_currentPage -= 1;

            emit currentPageChanged(m_currentPage);
        }

        emit pageLayoutChanged(m_pageLayout);

        this->prepareScene();
        this->prepareView();

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

        this->prepareScene();
        this->prepareView();

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

        this->prepareScene();
        this->prepareView();

        m_settings.setValue("documentView/rotation", static_cast<uint>(m_rotation));
    }
}

bool DocumentView::highlightAll() const
{
    return m_highlightAll;
}

void DocumentView::setHighlightAll(bool highlightAll)
{
    if(m_highlightAll != highlightAll)
    {
        m_highlightAll = highlightAll;

        emit highlightAllChanged(m_highlightAll);

        m_settings.setValue("documentView/highlightAll", m_highlightAll);
    }
}

void DocumentView::setCurrentPage(int currentPage)
{
    if(m_currentPage != currentPage && currentPage >= 1 &&  currentPage <= m_model->pageCount())
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

        this->prepareView();
    }
}

void DocumentView::previousPage()
{
    switch(m_pageLayout)
    {
    case OnePage:
    case OneColumn:
        if(m_currentPage > 1)
        {
            m_currentPage -= 1;

            emit currentPageChanged(m_currentPage);

            this->prepareView();
        }
        break;
    case TwoPages:
    case TwoColumns:
        if(m_currentPage > 2)
        {
            m_currentPage -= 2;

            emit currentPageChanged(m_currentPage);

            this->prepareView();
        }
        break;
    }
}

void DocumentView::nextPage()
{
    switch(m_pageLayout)
    {
    case OnePage:
    case OneColumn:
        if(m_currentPage <= m_model->pageCount()-1)
        {
            m_currentPage += 1;

            emit currentPageChanged(m_currentPage);

            this->prepareView();
        }
        break;
    case TwoPages:
    case TwoColumns:
        if(m_currentPage <= m_model->pageCount()-2)
        {
            m_currentPage += 2;

            emit currentPageChanged(m_currentPage);

            this->prepareView();
        }
        break;
    }
}

void DocumentView::firstPage()
{
    if(m_currentPage != 1)
    {
        m_currentPage = 1;

        emit currentPageChanged(m_currentPage);

        this->prepareView();
    }
}

void DocumentView::lastPage()
{
    switch(m_pageLayout)
    {
    case OnePage:
    case OneColumn:
        if(m_currentPage != m_model->pageCount())
        {
            m_currentPage = m_model->pageCount();

            emit currentPageChanged(m_currentPage);

            this->prepareView();
        }

        break;
    case TwoPages:
    case TwoColumns:
        if(m_model->pageCount() % 2 == 0)
        {
            if(m_currentPage != m_model->pageCount()-1)
            {
                m_currentPage = m_model->pageCount()-1;

                emit currentPageChanged(m_currentPage);

                this->prepareView();
            }
        }
        else
        {
            if(m_currentPage != m_model->pageCount())
            {
                m_currentPage = m_model->pageCount();

                emit currentPageChanged(m_currentPage);

                this->prepareView();
            }
        }
        break;
    }
}

void DocumentView::findPrevious()
{
    if(m_currentResult != m_results.end())
    {
        switch(m_pageLayout)
        {
        case OnePage:
        case OneColumn:
            if(m_currentResult.key() != m_currentPage-1)
            {
                m_currentResult = --m_results.upperBound(m_currentPage-1);
            }
            else
            {
                --m_currentResult;

                if(m_currentResult == m_results.end())
                {
                    m_currentResult = --m_results.upperBound(m_model->pageCount()-1);
                }
            }

            break;
        case TwoPages:
        case TwoColumns:
            if(m_currentResult.key() != m_currentPage-1 && m_currentResult.key() != m_currentPage)
            {
                m_currentResult = --m_results.upperBound(m_currentPage-1);
            }
            else
            {
                --m_currentResult;

                if(m_currentResult == m_results.end())
                {
                    m_currentResult = --m_results.upperBound(m_model->pageCount()-1);
                }
            }

            break;
        }
    }
    else
    {
        m_currentResult = --m_results.upperBound(m_currentPage-1);
    }

    if(m_currentResult != m_results.end())
    {
        this->prepareHighlight();

        this->setCurrentPage(m_currentResult.key()+1);

        disconnect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(scrollToPage(int)));

        m_view->centerOn(m_highlight);

        connect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(scrollToPage(int)));
    }
}

void DocumentView::findNext()
{
    if(m_currentResult != m_results.end())
    {
        switch(m_pageLayout)
        {
        case OnePage:
        case OneColumn:
            if(m_currentResult.key() != m_currentPage-1)
            {
                m_currentResult = m_results.lowerBound(m_currentPage-1);
            }
            else
            {
                ++m_currentResult;

                if(m_currentResult == m_results.end())
                {
                    m_currentResult = m_results.lowerBound(0);
                }
            }

            break;
        case TwoPages:
        case TwoColumns:
            if(m_currentResult.key() != m_currentPage-1 && m_currentResult.key() != m_currentPage)
            {
                m_currentResult = m_results.lowerBound(m_currentPage-1);
            }
            else
            {
                ++m_currentResult;

                if(m_currentResult == m_results.end())
                {
                    m_currentResult = m_results.lowerBound(0);
                }
            }

            break;
        }
    }
    else
    {
        m_currentResult = m_results.lowerBound(m_currentPage-1);
    }

    if(m_currentResult != m_results.end())
    {
        this->prepareHighlight();

        this->setCurrentPage(m_currentResult.key()+1);

        disconnect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(scrollToPage(int)));

        m_view->centerOn(m_highlight);

        connect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(scrollToPage(int)));
    }
}

void DocumentView::prepareScene()
{
    m_scene->clear();

    m_pageToPageObject.clear();
    m_heightToPage.clear();

    int pageCount = m_model->pageCount();

    // calculate scale factor

    QSizeF pageSize;
    qreal pageWidth = 0.0, pageHeight = 0.0;
    qreal viewWidth = static_cast<qreal>(m_view->width());
    qreal viewHeight = static_cast<qreal>(m_view->height());
    qreal scaleFactor = 4.0;

    m_resolutionX = this->physicalDpiX();
    m_resolutionY = this->physicalDpiX();

    switch(m_scaling)
    {
    case FitToPage:
    case FitToPageWidth:
        switch(m_pageLayout)
        {
        case OnePage:
        case OneColumn:
            for(int index = 0; index < pageCount; index++)
            {
                pageSize = m_model->pageSize(index);

                switch(m_rotation)
                {
                case RotateBy0:
                case RotateBy180:
                    pageWidth = m_resolutionX / 72.0 * pageSize.width();
                    pageHeight = m_resolutionY / 72.0 * pageSize.height();

                    break;
                case RotateBy90:
                case RotateBy270:
                    pageWidth = m_resolutionX / 72.0 * pageSize.height();
                    pageHeight = m_resolutionY / 72.0 * pageSize.width();

                    break;
                }

                scaleFactor = qMin(scaleFactor, 0.95 * viewWidth / (pageWidth + 20.0));
                if(m_scaling == FitToPage)
                {
                    scaleFactor = qMin(scaleFactor, 0.95 * viewHeight / (pageHeight + 20.0));
                }
            }
            break;
        case TwoPages:
        case TwoColumns:
            for(int index = 0; index < (pageCount % 2 != 0 ? pageCount-1 : pageCount); index += 2)
            {
                pageSize = m_model->pageSize(index);

                switch(m_rotation)
                {
                case RotateBy0:
                case RotateBy180:
                    pageWidth = m_resolutionX / 72.0 * pageSize.width();
                    pageHeight = m_resolutionY / 72.0 * pageSize.height();

                    break;
                case RotateBy90:
                case RotateBy270:
                    pageWidth = m_resolutionX / 72.0 * pageSize.height();
                    pageHeight = m_resolutionY / 72.0 * pageSize.width();

                    break;
                }

                pageSize = m_model->pageSize(index);

                switch(m_rotation)
                {
                case RotateBy0:
                case RotateBy180:
                    pageWidth += m_resolutionX / 72.0 * pageSize.width();
                    pageHeight = qMax(pageHeight, m_resolutionY / 72.0 * pageSize.height());

                    break;
                case RotateBy90:
                case RotateBy270:
                    pageWidth += m_resolutionX / 72.0 * pageSize.height();
                    pageHeight = qMax(pageHeight, m_resolutionY / 72.0 * pageSize.width());

                    break;
                }

                scaleFactor = qMin(scaleFactor, 0.95 * viewWidth / (pageWidth + 30.0));
                if(m_scaling == FitToPage)
                {
                    scaleFactor = qMin(scaleFactor, 0.95 * viewHeight / (pageHeight + 20.0));
                }
            }

            if(pageCount % 2 != 0)
            {
                pageSize = m_model->pageSize(pageCount-1);

                switch(m_rotation)
                {
                case RotateBy0:
                case RotateBy180:
                    pageWidth = m_resolutionX / 72.0 * pageSize.width();
                    pageHeight = m_resolutionY / 72.0 * pageSize.height();

                    break;
                case RotateBy90:
                case RotateBy270:
                    pageWidth = m_resolutionX / 72.0 * pageSize.height();
                    pageHeight = m_resolutionY / 72.0 * pageSize.width();

                    break;
                }

                scaleFactor = qMin(scaleFactor, 0.95 * viewWidth / (pageWidth + 20.0));
                if(m_scaling == FitToPage)
                {
                    scaleFactor = qMin(scaleFactor, 0.95 * viewHeight / (pageHeight + 20.0));
                }
            }
            break;
        }
        break;
    case ScaleTo50:
        scaleFactor = 0.50;
        break;
    case ScaleTo75:
        scaleFactor = 0.75;
        break;
    case ScaleTo100:
        scaleFactor = 1.00;
        break;
    case ScaleTo125:
        scaleFactor = 1.25;
        break;
    case ScaleTo150:
        scaleFactor = 1.50;
        break;
    case ScaleTo200:
        scaleFactor = 2.00;
        break;
    case ScaleTo400:
        scaleFactor = 4.00;
        break;
    }

    m_resolutionX *= scaleFactor;
    m_resolutionY *= scaleFactor;

    // populate scene

    qreal sceneWidth = 0.0, sceneHeight = 10.0;

    switch(m_pageLayout)
    {
    case OnePage:
    case OneColumn:
        for(int index = 0; index < pageCount; index++)
        {
            PageObject *page = new PageObject(m_model, this, index);
            page->setPos(10.0, sceneHeight);

            m_scene->addItem(page);
            m_pageToPageObject.insert(index+1, page);
            m_heightToPage.insert(-qFloor(page->y()), index+1);

            sceneWidth = qMax(sceneWidth, page->boundingRect().width() + 20.0);
            sceneHeight += page->boundingRect().height() + 10.0;
        }

        break;
    case TwoPages:
    case TwoColumns:
        for(int index = 0; index < (pageCount % 2 != 0 ? pageCount-1 : pageCount); index += 2)
        {
            PageObject *leftPage = new PageObject(m_model, this, index);
            leftPage->setPos(10.0, sceneHeight);

            m_scene->addItem(leftPage);
            m_pageToPageObject.insert(index+1, leftPage);
            m_heightToPage.insert(-qFloor(leftPage->y()), index+1);

            PageObject *rightPage = new PageObject(m_model, this, index+1);
            rightPage->setPos(leftPage->boundingRect().width() + 20.0, sceneHeight);

            m_scene->addItem(rightPage);
            m_pageToPageObject.insert(index+2, rightPage);

            sceneWidth = qMax(sceneWidth, leftPage->boundingRect().width() + rightPage->boundingRect().width() + 30.0);
            sceneHeight += qMax(leftPage->boundingRect().height(), rightPage->boundingRect().height()) + 10.0;
        }

        if(pageCount % 2 != 0)
        {
            PageObject *page = new PageObject(m_model, this, pageCount-1);
            page->setPos(10.0, sceneHeight);

            m_scene->addItem(page);
            m_pageToPageObject.insert(pageCount, page);
            m_heightToPage.insert(-qFloor(page->y()), pageCount);

            sceneWidth = qMax(sceneWidth, page->boundingRect().width() + 20.0);
            sceneHeight += page->boundingRect().height() + 10.0;
        }

        break;
    }

    m_scene->setSceneRect(0.0, 0.0, sceneWidth, sceneHeight);

    // highlight

    m_highlight = new QGraphicsRectItem();
    m_highlight->setPen(QPen(QColor(0,255,0,255)));
    m_highlight->setBrush(QBrush(QColor(0,255,0,127)));

    m_scene->addItem(m_highlight);

    this->prepareHighlight();
}

void DocumentView::prepareView()
{
    PageObject *page = m_pageToPageObject.value(m_currentPage), *nextPage = 0;

    switch(m_pageLayout)
    {
    case OnePage:
        foreach(PageObject *page, m_pageToPageObject.values())
        {
            page->setVisible(false);
        }

        m_view->setSceneRect(page->x()-10.0, page->y()-10.0,
                                     page->boundingRect().width()+20.0,
                                     page->boundingRect().height()+20.0);

        page->setVisible(true);

        break;
    case TwoPages:
        foreach(PageObject *page, m_pageToPageObject.values())
        {
            page->setVisible(false);
        }

        if(m_pageToPageObject.contains(m_currentPage+1))
        {
            nextPage = m_pageToPageObject.value(m_currentPage+1);


            m_view->setSceneRect(page->x()-10.0, page->y()-10.0,
                                         page->boundingRect().width() + nextPage->boundingRect().width() + 30.0,
                                         qMax(page->boundingRect().height(), nextPage->boundingRect().height()) + 20.0);

            page->setVisible(true);
            nextPage->setVisible(true);
        }
        else
        {
            m_view->setSceneRect(page->x()-10.0, page->y()-10.0,
                                         page->boundingRect().width()+20.0,
                                         page->boundingRect().height()+20.0);

            page->setVisible(true);
        }

        break;
    case OneColumn:
    case TwoColumns:
        m_view->setSceneRect(QRectF());

        break;
    }

    disconnect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(scrollToPage(int)));

    QRectF pageRect = page->boundingRect().translated(page->pos());
    QRectF viewRect = m_view->mapToScene(m_view->rect()).boundingRect();

    viewRect.translate(page->pos() - viewRect.topLeft());

    m_view->ensureVisible(viewRect.intersected(pageRect), 0, 0);

    connect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(scrollToPage(int)));
}

void DocumentView::prepareHighlight()
{
    if(m_currentResult != m_results.end())
    {
        PageObject *page = m_pageToPageObject.value(m_currentResult.key()+1);

        switch(m_rotation)
        {
        case RotateBy0:
            m_highlight->setTransform(QTransform(m_resolutionX / 72.0, 0.0, 0.0, m_resolutionY / 72.0, 0.0, 0.0));
            break;
        case RotateBy90:
            m_highlight->setTransform(QTransform(0.0, m_resolutionY / 72.0, -m_resolutionX / 72.0, 0.0, m_resolutionX / 72.0 * page->pageHeight(), 0.0));
            break;
        case RotateBy180:
            m_highlight->setTransform(QTransform(-m_resolutionX / 72., 0.0, 0.0, -m_resolutionY / 72.0, m_resolutionY / 72.0 * page->pageWidth(), m_resolutionY / 72.0 * page->pageHeight()));
            break;
        case RotateBy270:
            m_highlight->setTransform(QTransform(0.0, -m_resolutionY / 72.0, m_resolutionX / 72., 0.0, 0.0, m_resolutionY / 72.0 * page->pageWidth()));
            break;
        }

        m_highlight->setPos(page->pos());
        m_highlight->setRect(m_currentResult.value().adjusted(-1.0, -1.0, 1.0, 1.0));

        m_highlight->setVisible(true);
    }
    else
    {
        m_highlight->setVisible(false);
    }
}

void DocumentView::makeCurrentTab()
{
    QTabWidget *tabWidget = qobject_cast<QTabWidget*>(this->parent()->parent());

    if(tabWidget != 0)
    {
        int index = tabWidget->indexOf(this);

        if(index != -1)
        {
            tabWidget->setCurrentIndex(index);
        }
    }
}

void DocumentView::scrollToPage(int value)
{
    int visiblePage = 1;

    switch(m_pageLayout)
    {
    case OnePage:
    case TwoPages:
        break;
    case OneColumn:
    case TwoColumns:
        if(m_heightToPage.lowerBound(-value) != m_heightToPage.end())
        {
            visiblePage = m_heightToPage.lowerBound(-value).value();
        }

        if(m_currentPage != visiblePage) {
            m_currentPage = visiblePage;

            emit currentPageChanged(m_currentPage);
        }
    }
}

void DocumentView::updateFilePath(const QString &filePath)
{
    m_makeCurrentTabAction->setText(QFileInfo(filePath).completeBaseName());

    if(m_currentPage > m_model->pageCount())
    {
        m_currentPage = 1;
    }

    this->prepareScene();
    this->prepareView();
}

void DocumentView::updateResults()
{
    m_results = m_model->results();
    m_currentResult = m_results.end();

    this->prepareHighlight();
}

void DocumentView::resizeEvent(QResizeEvent*)
{
    if(m_scaling == FitToPage || m_scaling == FitToPageWidth)
    {
        this->prepareScene();
        this->prepareView();
    }
}

void DocumentView::wheelEvent(QWheelEvent *wheelEvent)
{
    switch(m_pageLayout)
    {
    case OnePage:
        if(wheelEvent->delta() > 0 && m_view->verticalScrollBar()->value() == m_view->verticalScrollBar()->minimum() && m_currentPage != 1)
        {
            this->previousPage();

            m_view->verticalScrollBar()->setValue(m_view->verticalScrollBar()->maximum());
        }
        else if(wheelEvent->delta() < 0 && m_view->verticalScrollBar()->value() == m_view->verticalScrollBar()->maximum() && m_currentPage != m_model->pageCount())
        {
            this->nextPage();

            m_view->verticalScrollBar()->setValue(m_view->verticalScrollBar()->minimum());
        }

        break;
    case TwoPages:
        if(wheelEvent->delta() > 0 && m_view->verticalScrollBar()->value() == m_view->verticalScrollBar()->minimum() && m_currentPage != 1)
        {
            this->previousPage();

            m_view->verticalScrollBar()->setValue(m_view->verticalScrollBar()->maximum());
        }
        else if(wheelEvent->delta() < 0 && m_view->verticalScrollBar()->value() == m_view->verticalScrollBar()->maximum() && m_currentPage != (m_model->pageCount() % 2 == 0 ? m_model->pageCount()-1 : m_model->pageCount()))
        {
            this->nextPage();

            m_view->verticalScrollBar()->setValue(m_view->verticalScrollBar()->minimum());
        }

        break;
    case OneColumn:
    case TwoColumns:
        break;
    }
}
