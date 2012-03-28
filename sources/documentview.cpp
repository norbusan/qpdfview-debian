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
#include "miscellaneous.h"

DocumentView::DocumentView(DocumentModel *model, QWidget *parent) : QWidget(parent),
    m_resolutionX(72.0),
    m_resolutionY(72.0),
    m_pageToPageObject(),
    m_heightToPage(),
    m_results(),
    m_currentResult(m_results.end()),
    m_settings(),
    m_currentPage(1),
    m_pageLayout(OnePage),
    m_scaling(ScaleTo100),
    m_rotation(RotateBy0),
    m_highlightAll(false)
{
    m_model = model;

    connect(m_model, SIGNAL(filePathChanged(QString, bool)), this, SLOT(updateFilePath(QString, bool)));
    connect(m_model, SIGNAL(resultsChanged()), this, SLOT(updateResults()));

    // settings

    m_pageLayout = static_cast<PageLayout>(m_settings.value("documentView/pageLayout", 0).toUInt());
    m_scaling = static_cast<Scaling>(m_settings.value("documentView/scaling", 4).toUInt());
    m_rotation = static_cast<Rotation>(m_settings.value("documentView/rotation", 0).toUInt());
    m_highlightAll = m_settings.value("documentView/highlightAll", false).toBool();

    // graphics

    m_scene = new QGraphicsScene(this);
    m_view = new QGraphicsView(m_scene, this);

    this->setLayout(new QHBoxLayout());
    this->layout()->addWidget(m_view);

    m_scene->setBackgroundBrush(QBrush(Qt::darkGray));
    m_view->show();

    // makeCurrentTab

    m_makeCurrentTabAction = new QAction(QFileInfo(m_model->filePath()).completeBaseName(), this);

    connect(m_makeCurrentTabAction, SIGNAL(triggered()), this, SLOT(makeCurrentTab()));

    // bookmarks

    m_bookmarksMenu = new BookmarksMenu(this);

    connect(m_bookmarksMenu, SIGNAL(entrySelected(int,qreal)), this, SLOT(setCurrentPage(int,qreal)));

    // prefetchTimer

    m_prefetchTimer = new QTimer(this);
    m_prefetchTimer->setSingleShot(true);
    m_prefetchTimer->setInterval(500);

    connect(this, SIGNAL(currentPageChanged(int)), m_prefetchTimer, SLOT(start()));
    connect(this, SIGNAL(pageLayoutChanged(DocumentView::PageLayout)), m_prefetchTimer, SLOT(start()));
    connect(this, SIGNAL(scalingChanged(DocumentView::Scaling)), m_prefetchTimer, SLOT(start()));
    connect(this, SIGNAL(rotationChanged(DocumentView::Rotation)), m_prefetchTimer, SLOT(start()));

    connect(m_prefetchTimer, SIGNAL(timeout()), this, SLOT(prefetchTimeout()));

    // verticalScrollBar

    m_view->verticalScrollBar()->installEventFilter(this);
    connect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(changeCurrentPage(int)));

    this->preparePages();
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

DocumentModel *DocumentView::model() const
{
    return m_model;
}

qreal DocumentView::resolutionX() const
{
    return m_resolutionX;
}

qreal DocumentView::resolutionY() const
{
    return m_resolutionY;
}

QAction *DocumentView::makeCurrentTabAction() const
{
    return m_makeCurrentTabAction;
}

void DocumentView::setCurrentPage(int currentPage, qreal top)
{
    if(currentPage >= 1 && currentPage <= m_model->pageCount())
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

        this->prepareView(top);
    }
}

void DocumentView::previousPage(qreal top)
{
    switch(m_pageLayout)
    {
    case OnePage:
    case OneColumn:
        if(m_currentPage > 1)
        {
            m_currentPage -= 1;

            emit currentPageChanged(m_currentPage);

            this->prepareView(top);
        }
        break;
    case TwoPages:
    case TwoColumns:
        if(m_currentPage > 2)
        {
            m_currentPage -= 2;

            emit currentPageChanged(m_currentPage);

            this->prepareView(top);
        }
        break;
    }
}

void DocumentView::nextPage(qreal top)
{
    switch(m_pageLayout)
    {
    case OnePage:
    case OneColumn:
        if(m_currentPage <= m_model->pageCount()-1)
        {
            m_currentPage += 1;

            emit currentPageChanged(m_currentPage);

            this->prepareView(top);
        }
        break;
    case TwoPages:
    case TwoColumns:
        if(m_currentPage <= m_model->pageCount()-2)
        {
            m_currentPage += 2;

            emit currentPageChanged(m_currentPage);

            this->prepareView(top);
        }
        break;
    }
}

void DocumentView::firstPage(qreal top)
{
    if(m_currentPage != 1)
    {
        m_currentPage = 1;

        emit currentPageChanged(m_currentPage);

        this->prepareView(top);
    }
}

void DocumentView::lastPage(qreal top)
{
    switch(m_pageLayout)
    {
    case OnePage:
    case OneColumn:
        if(m_currentPage != m_model->pageCount())
        {
            m_currentPage = m_model->pageCount();

            emit currentPageChanged(m_currentPage);

            this->prepareView(top);
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

                this->prepareView(top);
            }
        }
        else
        {
            if(m_currentPage != m_model->pageCount())
            {
                m_currentPage = m_model->pageCount();

                emit currentPageChanged(m_currentPage);

                this->prepareView(top);
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
            }

            if(m_currentResult == m_results.end())
            {
                m_currentResult = --m_results.upperBound(m_model->pageCount()-1);
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
            }

            if(m_currentResult == m_results.end())
            {
                m_currentResult = --m_results.upperBound(m_model->pageCount()-1);
            }

            break;
        }
    }
    else
    {
        m_currentResult = --m_results.upperBound(m_currentPage-1);

        if(m_currentResult == m_results.end())
        {
            m_currentResult = --m_results.upperBound(m_model->pageCount()-1);
        }
    }

    if(m_currentResult != m_results.end())
    {
        this->setCurrentPage(m_currentResult.key()+1);

        disconnect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(changeCurrentPage(int)));

        m_view->centerOn(m_highlight);

        connect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(changeCurrentPage(int)));

        // bookmarks

        PageObject *page = m_pageToPageObject.value(m_currentPage);
        qreal top = qMax(0.0, static_cast<qreal>(m_view->verticalScrollBar()->value()) - page->y()) / page->boundingRect().height();

        m_bookmarksMenu->updateCurrrentPage(m_currentPage, top);
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
            }

            if(m_currentResult == m_results.end())
            {
                m_currentResult = m_results.lowerBound(0);
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
            }

            if(m_currentResult == m_results.end())
            {
                m_currentResult = m_results.lowerBound(0);
            }

            break;
        }
    }
    else
    {
        m_currentResult = m_results.lowerBound(m_currentPage-1);

        if(m_currentResult == m_results.end())
        {
            m_currentResult = m_results.lowerBound(0);
        }
    }

    if(m_currentResult != m_results.end())
    {
        this->setCurrentPage(m_currentResult.key()+1);

        disconnect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(changeCurrentPage(int)));

        m_view->centerOn(m_highlight);

        connect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(changeCurrentPage(int)));

        // bookmarks

        PageObject *page = m_pageToPageObject.value(m_currentPage);
        qreal top = qMax(0.0, static_cast<qreal>(m_view->verticalScrollBar()->value()) - page->y()) / page->boundingRect().height();

        m_bookmarksMenu->updateCurrrentPage(m_currentPage, top);
    }
}

void DocumentView::preparePages()
{
    // load pages

    m_scene->clear();
    m_pageToPageObject.clear();

    for(int index = 0; index < m_model->pageCount(); index++)
    {
        PageObject *page = new PageObject(m_model, this, index);

        m_scene->addItem(page);
        m_pageToPageObject.insert(index+1, page);
    }

    // highlight

    m_highlight = new QGraphicsRectItem();
    m_highlight->setPen(QPen(QColor(0,255,0,255)));
    m_highlight->setBrush(QBrush(QColor(0,255,0,127)));

    m_scene->addItem(m_highlight);

    // prepare

    this->prepareScene();
    this->prepareView();

    m_prefetchTimer->start();
}

void DocumentView::prepareScene()
{
    // calculate scale factor

    QSizeF size;
    qreal pageWidth = 0.0, pageHeight = 0.0;
    qreal viewWidth = static_cast<qreal>(m_view->width()), viewHeight = static_cast<qreal>(m_view->height());
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
            for(int page = 1; page <= m_model->pageCount(); page++)
            {
                size = m_pageToPageObject.value(page)->size();

                switch(m_rotation)
                {
                case RotateBy0:
                case RotateBy180:
                    pageWidth = m_resolutionX / 72.0 * size.width();
                    pageHeight = m_resolutionY / 72.0 * size.height();

                    break;
                case RotateBy90:
                case RotateBy270:
                    pageWidth = m_resolutionX / 72.0 * size.height();
                    pageHeight = m_resolutionY / 72.0 * size.width();

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
            for(int page = 1; page <= (m_model->pageCount() % 2 != 0 ? m_model->pageCount()-1 : m_model->pageCount()); page += 2)
            {
                size = m_pageToPageObject.value(page)->size();

                switch(m_rotation)
                {
                case RotateBy0:
                case RotateBy180:
                    pageWidth = m_resolutionX / 72.0 * size.width();
                    pageHeight = m_resolutionY / 72.0 * size.height();

                    break;
                case RotateBy90:
                case RotateBy270:
                    pageWidth = m_resolutionX / 72.0 * size.height();
                    pageHeight = m_resolutionY / 72.0 * size.width();

                    break;
                }

                size = m_pageToPageObject.value(page+1)->size();

                switch(m_rotation)
                {
                case RotateBy0:
                case RotateBy180:
                    pageWidth += m_resolutionX / 72.0 * size.width();
                    pageHeight = qMax(pageHeight, m_resolutionY / 72.0 * size.height());

                    break;
                case RotateBy90:
                case RotateBy270:
                    pageWidth += m_resolutionX / 72.0 * size.height();
                    pageHeight = qMax(pageHeight, m_resolutionY / 72.0 * size.width());

                    break;
                }

                scaleFactor = qMin(scaleFactor, 0.95 * viewWidth / (pageWidth + 30.0));
                if(m_scaling == FitToPage)
                {
                    scaleFactor = qMin(scaleFactor, 0.95 * viewHeight / (pageHeight + 20.0));
                }
            }

            if(m_model->pageCount() % 2 != 0)
            {
                size = m_pageToPageObject.value(m_model->pageCount())->size();

                switch(m_rotation)
                {
                case RotateBy0:
                case RotateBy180:
                    pageWidth = m_resolutionX / 72.0 * size.width();
                    pageHeight = m_resolutionY / 72.0 * size.height();

                    break;
                case RotateBy90:
                case RotateBy270:
                    pageWidth = m_resolutionX / 72.0 * size.height();
                    pageHeight = m_resolutionY / 72.0 * size.width();

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

    // arrange pages

    qreal sceneWidth = 0.0, sceneHeight = 10.0;

    m_heightToPage.clear();

    switch(m_pageLayout)
    {
    case OnePage:
    case OneColumn:
        for(int page = 1; page <= m_model->pageCount(); page++)
        {
            PageObject *pageObject = m_pageToPageObject.value(page);

            pageObject->prepareTransforms();
            pageObject->setPos(10.0, sceneHeight);

            m_heightToPage.insert(-qFloor(pageObject->y() - 0.4 * pageObject->boundingRect().height()), page);

            sceneWidth = qMax(sceneWidth, pageObject->boundingRect().width() + 20.0);
            sceneHeight += pageObject->boundingRect().height() + 10.0;
        }

        break;
    case TwoPages:
    case TwoColumns:
        for(int page = 1; page <= (m_model->pageCount() % 2 != 0 ? m_model->pageCount()-1 : m_model->pageCount()); page += 2)
        {
            PageObject *pageObject = m_pageToPageObject.value(page);

            pageObject->prepareTransforms();
            pageObject->setPos(10.0, sceneHeight);

            m_heightToPage.insert(-qFloor(pageObject->y() - 0.4 * pageObject->boundingRect().height()), page);

            PageObject *nextPageObject = m_pageToPageObject.value(page+1);

            nextPageObject->prepareTransforms();
            nextPageObject->setPos(pageObject->boundingRect().width() + 20.0, sceneHeight);

            sceneWidth = qMax(sceneWidth, pageObject->boundingRect().width() + nextPageObject->boundingRect().width() + 30.0);
            sceneHeight += qMax(pageObject->boundingRect().height(), nextPageObject->boundingRect().height()) + 10.0;
        }

        if(m_model->pageCount() % 2 != 0)
        {
            PageObject *pageObject = m_pageToPageObject.value(m_model->pageCount());

            pageObject->prepareTransforms();
            pageObject->setPos(10.0, sceneHeight);

            m_heightToPage.insert(-qFloor(pageObject->y() - 0.4 * pageObject->boundingRect().height()), m_model->pageCount());

            sceneWidth = qMax(sceneWidth, pageObject->boundingRect().width() + 20.0);
            sceneHeight += pageObject->boundingRect().height() + 10.0;
        }

        break;
    }

    m_scene->setSceneRect(0.0, 0.0, sceneWidth, sceneHeight);

    // bookmarks

    m_bookmarksMenu->clearList();
}

void DocumentView::prepareView(qreal top)
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
        foreach(PageObject *page, m_pageToPageObject.values())
        {
            page->setVisible(true);
        }

        m_view->setSceneRect(QRectF());

        break;
    }

    // highlight

    if(m_currentResult != m_results.end())
    {
        PageObject *page = m_pageToPageObject.value(m_currentResult.key()+1);

        m_highlight->setPos(page->pos());
        m_highlight->setTransform(page->resultTransform());

        m_highlight->setRect(m_currentResult.value().adjusted(-1.0, -1.0, 1.0, 1.0));

        m_highlight->setVisible(true);
    }
    else
    {
        m_highlight->setVisible(false);
    }

    // verticalScrollBar

    disconnect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(changeCurrentPage(int)));

    m_view->verticalScrollBar()->setValue(qCeil(page->y() + page->boundingRect().height() * top));

    connect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(changeCurrentPage(int)));

    // bookmarks

    m_bookmarksMenu->updateCurrrentPage(m_currentPage, top);
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
    else
    {
        qCritical("!tabWidget");
    }
}

void DocumentView::prefetchTimeout()
{
    int fromPage = qMax(m_currentPage-2, 1);
    int toPage = qMin(m_currentPage+3, m_model->pageCount());

    for(int page = fromPage; page <= toPage; page++)
    {
        m_pageToPageObject.value(page)->prefetch();
    }
}

void DocumentView::changeCurrentPage(int value)
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

    // bookmarks

    PageObject *page = m_pageToPageObject.value(m_currentPage);
    qreal top = qMax(0.0, static_cast<qreal>(value) - page->y()) / page->boundingRect().height();

    m_bookmarksMenu->updateCurrrentPage(m_currentPage, top);
}

void DocumentView::updateFilePath(const QString &filePath, bool refresh)
{
    // currentPage

    if(refresh)
    {
        m_currentPage = m_currentPage > m_model->pageCount() ? 1 : m_currentPage;
    }
    else
    {
        m_currentPage = 1;
    }

    emit currentPageChanged(m_currentPage);

    // makeCurrentTab

    m_makeCurrentTabAction->setText(QFileInfo(filePath).completeBaseName());

    QTabWidget *tabWidget = qobject_cast<QTabWidget*>(this->parent()->parent());

    if(tabWidget != 0)
    {
        int index = tabWidget->indexOf(this);

        if(index != -1)
        {
            tabWidget->setTabText(index, QFileInfo(filePath).completeBaseName());
            tabWidget->setTabToolTip(index, QFileInfo(filePath).completeBaseName());
        }
    }

    this->preparePages();
}

void DocumentView::updateResults()
{
    m_results = m_model->results();
    m_currentResult = m_results.end();

    this->prepareView(false);
}

bool DocumentView::eventFilter(QObject*, QEvent *event)
{
    if(event->type() == QEvent::Wheel)
    {
        QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);

        return wheelEvent->modifiers() == Qt::CTRL;
    }
    else
    {
        return false;
    }
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
    if(wheelEvent->modifiers() == Qt::ControlModifier)
    {
        if(wheelEvent->delta() > 0)
        {
            // zoom in
            switch(m_scaling)
            {
            case FitToPage:
            case FitToPageWidth:
                break;
            case ScaleTo50:
                this->setScaling(ScaleTo75);
                break;
            case ScaleTo75:
                this->setScaling(ScaleTo100);
                break;
            case ScaleTo100:
                this->setScaling(ScaleTo125);
                break;
            case ScaleTo125:
                this->setScaling(ScaleTo150);
                break;
            case ScaleTo150:
                this->setScaling(ScaleTo200);
                break;
            case ScaleTo200:
                this->setScaling(ScaleTo400);
                break;
            case ScaleTo400:
                break;
            }
        }
        else if(wheelEvent->delta() < 0)
        {
            // zoom out
            switch(m_scaling)
            {
            case FitToPage:
            case FitToPageWidth:
                break;
            case ScaleTo50:
                break;
            case ScaleTo75:
                this->setScaling(ScaleTo50);
                break;
            case ScaleTo100:
                this->setScaling(ScaleTo75);
                break;
            case ScaleTo125:
                this->setScaling(ScaleTo100);
                break;
            case ScaleTo150:
                this->setScaling(ScaleTo125);
                break;
            case ScaleTo200:
                this->setScaling(ScaleTo150);
                break;
            case ScaleTo400:
                this->setScaling(ScaleTo200);
                break;
            }
        }


    }
    else
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
}

void DocumentView::keyPressEvent(QKeyEvent *keyEvent)
{
    QKeySequence shortcut(keyEvent->modifiers() + keyEvent->key());

    foreach(QAction *action, m_bookmarksMenu->actions())
    {
        if(action->shortcut() == shortcut)
        {
            action->trigger();
        }
    }
}

void DocumentView::contextMenuEvent(QContextMenuEvent *contextMenuEvent)
{
    m_bookmarksMenu->exec(contextMenuEvent->globalPos());
}
