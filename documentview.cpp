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
    m_document(0),m_settings(),m_pageToPageObject(),m_valueToPage(),m_filePath(),m_currentPage(-1),m_numberOfPages(-1),m_pageLayout(OnePage),m_scaling(ScaleTo100),m_rotation(RotateBy0)
{
    m_graphicsScene = new QGraphicsScene(this);
    m_graphicsView = new QGraphicsView(m_graphicsScene, this);

    m_graphicsScene->setBackgroundBrush(QBrush(Qt::darkGray));

    this->setLayout(new QHBoxLayout());
    this->layout()->addWidget(m_graphicsView);

    connect(m_graphicsView->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(scrollToPage(int)));

    m_tabMenuAction = new QAction(this);
    connect(m_tabMenuAction, SIGNAL(triggered()), this, SLOT(tabMenuActionTriggered()));

    m_progressDialog = new QProgressDialog(this);
    m_progressWatcher = new QFutureWatcher<void>();

    connect(this, SIGNAL(printingProgressed(int)), m_progressDialog, SLOT(setValue(int)));
    connect(this, SIGNAL(printingCanceled()), m_progressDialog, SLOT(close()));
    connect(this, SIGNAL(printingFinished()), m_progressDialog, SLOT(close()));

    m_pageLayout = static_cast<PageLayout>(m_settings.value("documentView/pageLayout", 0).toUInt());
    m_scaling = static_cast<Scaling>(m_settings.value("documentView/scaling", 4).toUInt());
    m_rotation = static_cast<Rotation>(m_settings.value("documentView/rotation", 0).toUInt());
}

DocumentView::~DocumentView()
{
    m_graphicsScene->clear();

    delete m_graphicsView;
    delete m_graphicsScene;

    delete m_tabMenuAction;

    if(m_progressWatcher->isRunning())
    {
        m_progressWatcher->waitForFinished();
    }

    delete m_progressWatcher;
    delete m_progressDialog;

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
            clearHighlights();

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

        m_settings.setValue("documentView/rotation", static_cast<uint>(m_rotation));
    }
}

qreal DocumentView::resolutionX() const
{
    return m_resolutionX;
}

qreal DocumentView::resolutionY() const
{
    return m_resolutionY;
}


QAction *DocumentView::tabMenuAction() const
{
    return m_tabMenuAction;
}

void DocumentView::tabMenuActionTriggered()
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

        m_tabMenuAction->setText(QFileInfo(m_filePath).baseName());
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

void DocumentView::print()
{
    if(m_document)
    {
        QPrinter *printer = new QPrinter();
        printer->setFullPage(true);

        QPrintDialog printDialog(printer, this);
        printDialog.setMinMax(1, m_numberOfPages);

        if(printDialog.exec() == QDialog::Accepted)
        {
            int fromPage = printDialog.fromPage() != 0 ? printDialog.fromPage() : 1;
            int toPage = printDialog.toPage() != 0 ? printDialog.toPage() : m_numberOfPages;

            if(m_progressWatcher->isRunning())
            {
                m_progressWatcher->waitForFinished();
            }

            m_progressDialog->reset();

            m_progressDialog->setLabelText(tr("Printing pages %1 to %2 of file \"%3\"...").arg(fromPage).arg(toPage).arg(QFileInfo(m_filePath).fileName()));
            m_progressDialog->setRange(fromPage-1, toPage);
            m_progressDialog->setValue(fromPage-1);

            m_progressWatcher->setFuture(QtConcurrent::run(this, &DocumentView::printDocument, printer, fromPage, toPage));

            m_progressDialog->exec();
        }
    }
}

void DocumentView::printDocument(QPrinter *printer, int fromPage, int toPage)
{
    Poppler::Document *document = Poppler::Document::load(m_filePath);

    if(document == 0)
    {
        qDebug() << "document == 0:" << m_filePath;

        emit printingCanceled();

        delete printer;

        return;
    }

    QPainter *painter = new QPainter();

    painter->begin(printer);

    for(int currentPage = fromPage; currentPage <= toPage; currentPage++)
    {
        Poppler::Page *page = document->page(currentPage-1);

        if(page == 0)
        {
            qDebug() << "page == 0:" << m_filePath << currentPage;

            emit printingCanceled();

            delete document;
            delete painter;
            delete printer;

            return;
        }

        qreal fitToWidth = static_cast<qreal>(printer->width()) / (printer->physicalDpiX() * page->pageSizeF().width() / 72.0);
        qreal fitToHeight = static_cast<qreal>(printer->height()) / (printer->physicalDpiY() * page->pageSizeF().height() / 72.0);
        qreal fit = qMin(fitToWidth, fitToHeight);

        painter->resetTransform();
        painter->scale(fit, fit);
        painter->drawImage(QPointF(0.0, 0.0), page->renderToImage(printer->physicalDpiX(), printer->physicalDpiY()));

        delete page;

        if(currentPage != toPage)
        {
            printer->newPage();
        }

        if(m_progressDialog->wasCanceled())
        {
            emit printingCanceled();

            delete document;
            delete painter;
            delete printer;

            return;
        }
        else
        {
            emit printingProgressed(currentPage);
        }
    }

    painter->end();

    emit printingFinished();

    delete document;
    delete painter;
    delete printer;
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
                clearHighlights();

                m_currentPage -= 1;

                emit currentPageChanged(m_currentPage);

                prepareView();
            }
            break;
        case TwoPages:
        case TwoColumns:
            if(m_currentPage > 2)
            {
                clearHighlights();

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
                clearHighlights();

                m_currentPage += 1;

                emit currentPageChanged(m_currentPage);

                prepareView();
            }
            break;
        case TwoPages:
        case TwoColumns:
            if(m_currentPage <= m_numberOfPages-2)
            {
                clearHighlights();

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
            clearHighlights();

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
                clearHighlights();

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
                    clearHighlights();

                    m_currentPage = m_numberOfPages-1;

                    emit currentPageChanged(m_currentPage);

                    prepareView();
                }
            }
            else
            {
                if(m_currentPage != m_numberOfPages)
                {
                    clearHighlights();

                    m_currentPage = m_numberOfPages;

                    emit currentPageChanged(m_currentPage);

                    prepareView();
                }
            }
            break;
        }
    }
}


void DocumentView::clearHighlights()
{
    if(m_document)
    {
        switch(m_pageLayout)
        {
        case OnePage:
        case OneColumn:
            m_pageToPageObject.value(m_currentPage)->clearHighlight();

            break;
        case TwoPages:
        case TwoColumns:
            m_pageToPageObject.value(m_currentPage)->clearHighlight();

            if(m_pageToPageObject.contains(m_currentPage+1))
            {
                m_pageToPageObject.value(m_currentPage+1)->clearHighlight();
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
            PageObject *pageObject = m_pageToPageObject.value(page);
            result = pageObject->findNext(text);

            if(result)
            {
                this->setCurrentPage(page);

                disconnect(m_graphicsView->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(scrollToPage(int)));

                m_graphicsView->centerOn(pageObject->highlightedArea().center());

                connect(m_graphicsView->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(scrollToPage(int)));

                break;
            }
        }

        if(!result)
        {
            for(int page = 1; page < m_currentPage; page++)
            {
                PageObject *pageObject = m_pageToPageObject.value(page);
                result = pageObject->findNext(text);

                if(result)
                {
                    this->setCurrentPage(page);

                    disconnect(m_graphicsView->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(scrollToPage(int)));

                    m_graphicsView->centerOn(pageObject->highlightedArea().center());

                    connect(m_graphicsView->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(scrollToPage(int)));

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

void DocumentView::copyText()
{
    if(m_document)
    {
        PageObject *pageObject = m_pageToPageObject.value(m_currentPage);

        if(!pageObject->highlightedText().isEmpty())
        {
            QApplication::clipboard()->setText(pageObject->highlightedText());
        }
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
        qreal sceneWidth = 0.0, sceneHeight = 10.0;

        // calculate scale factor

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
                for(int i = 0; i < m_numberOfPages; i++)
                {
                    Poppler::Page *page = m_document->page(i);

                    if(m_rotation == RotateBy90 || m_rotation == RotateBy270)
                    {
                        pageWidth = m_resolutionX * page->pageSizeF().height() / 72.0;
                        pageHeight = m_resolutionY * page->pageSizeF().width() / 72.0;
                    }
                    else
                    {
                        pageWidth = m_resolutionX * page->pageSizeF().width() / 72.0;
                        pageHeight = m_resolutionY * page->pageSizeF().height() / 72.0;
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
                        pageWidth = m_resolutionX * page->pageSizeF().height() / 72.0;
                        pageHeight = m_resolutionY * page->pageSizeF().width() / 72.0;
                    }
                    else
                    {
                        pageWidth = m_resolutionX * page->pageSizeF().width() / 72.0;
                        pageHeight = m_resolutionY * page->pageSizeF().height() / 72.0;
                    }

                    delete page;

                    page = m_document->page(i+1);

                    if(m_rotation == RotateBy90 || m_rotation == RotateBy270)
                    {
                        pageWidth += m_resolutionX * page->pageSizeF().height() / 72.0;
                        pageHeight = qMax(pageHeight, m_resolutionY * page->pageSizeF().width() / 72.0);
                    }
                    else
                    {
                        pageWidth += m_resolutionX * page->pageSizeF().width() / 72.0;
                        pageHeight += qMax(pageHeight, m_resolutionY * page->pageSizeF().height() / 72.0);
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
                        pageWidth = m_resolutionX * page->pageSizeF().height() / 72.0;
                        pageHeight = m_resolutionY * page->pageSizeF().width() / 72.0;
                    }
                    else
                    {
                        pageWidth = m_resolutionX * page->pageSizeF().width() / 72.0;
                        pageHeight = m_resolutionY * page->pageSizeF().height() / 72.0;
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

        // prepare scene

        switch(m_pageLayout)
        {
        case OnePage:
        case OneColumn:
            for(int i = 0; i < m_numberOfPages; i++)
            {
                PageObject *page = new PageObject(m_document->page(i),i,this);
                page->setPos(10.0, sceneHeight);

                connect(page, SIGNAL(linkClicked(int)), this, SLOT(followLink(int)));

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
                PageObject *leftPage = new PageObject(m_document->page(i),i,this);
                leftPage->setPos(10.0, sceneHeight);

                connect(leftPage, SIGNAL(linkClicked(int)), this, SLOT(followLink(int)));

                m_graphicsScene->addItem(leftPage);
                m_pageToPageObject.insert(i+1, leftPage);
                m_valueToPage.insert(-static_cast<int>(leftPage->y()), i+1);

                PageObject *rightPage = new PageObject(m_document->page(i+1),i+1,this);
                rightPage->setPos(leftPage->boundingRect().width() + 20.0, sceneHeight);

                connect(rightPage, SIGNAL(linkClicked(int)), this, SLOT(followLink(int)));

                m_graphicsScene->addItem(rightPage);
                m_pageToPageObject.insert(i+2, rightPage);

                sceneWidth = qMax(sceneWidth, leftPage->boundingRect().width() + rightPage->boundingRect().width() + 30.0);
                sceneHeight += qMax(leftPage->boundingRect().height(), rightPage->boundingRect().height()) + 10.0;
            }

            if(m_numberOfPages % 2 != 0)
            {
                PageObject *leftPage = new PageObject(m_document->page(m_numberOfPages-1),m_numberOfPages-1,this);
                leftPage->setPos(10.0, sceneHeight);

                connect(leftPage, SIGNAL(linkClicked(int)), this, SLOT(followLink(int)));

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
    if(m_document)
    {
        m_graphicsView->show();

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

        disconnect(m_graphicsView->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(scrollToPage(int)));

        m_graphicsView->centerOn(page);

        connect(m_graphicsView->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(scrollToPage(int)));
    }
    else
    {
        m_graphicsView->hide();
    }
}

void DocumentView::scrollToPage(const int &value)
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
                clearHighlights();

                m_currentPage = visiblePage;

                emit currentPageChanged(m_currentPage);
            }
        }
    }
}

void DocumentView::followLink(int gotoPage)
{
    if(m_document)
    {
        this->setCurrentPage(gotoPage);
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
