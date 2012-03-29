#include "document.h"
#include "pageview.h"
#include "documentview.h"
#include "miscellaneous.h"
#include "mainwindow.h"

DocumentView::DocumentView(QWidget *parent) : QWidget(parent),
    m_currentPage(1),
    m_pageLayout(OnePage),
    m_scaling(ScaleTo100),
    m_rotation(RotateBy0),
    m_highlightAll(false),
    m_searchResults(),
    m_searchPosition(m_searchResults.end()),
    m_settings(),
    m_highlight(0),
    m_pageToPageView(),
    m_heightToPage()
{
    m_document = new Document(this);

    connect(m_document, SIGNAL(filePathChanged(QString)), this, SLOT(slotFilePathChanged(QString)));
    connect(m_document, SIGNAL(numberOfPagesChanged(int)), this, SLOT(slotNumberOfPagesChanged(int)));

    connect(m_document, SIGNAL(documentChanged()), this, SLOT(slotDocumentChanged()));

    connect(m_document, SIGNAL(pageSearched(int)), this, SLOT(slotPageSearched(int)));

    connect(m_document, SIGNAL(searchProgressed(int)), this, SLOT(slotSearchProgressed(int)));
    connect(m_document, SIGNAL(searchCanceled()), this, SLOT(slotSearchCanceled()));
    connect(m_document, SIGNAL(searchFinished()), this, SLOT(slotSearchFinished()));

    connect(m_document, SIGNAL(printProgressed(int)), this, SLOT(slotPrintProgressed(int)));
    connect(m_document, SIGNAL(printCanceled()), this, SLOT(slotPrintCanceled()));
    connect(m_document, SIGNAL(printFinished()), this, SLOT(slotPrintFinished()));

    // settings

    m_pageLayout = static_cast<PageLayout>(m_settings.value("documentView/pageLayout", 0).toUInt());
    m_scaling = static_cast<Scaling>(m_settings.value("documentView/scaling", 4).toUInt());
    m_rotation = static_cast<Rotation>(m_settings.value("documentView/rotation", 0).toUInt());
    m_highlightAll = m_settings.value("documentView/highlightAll", false).toBool();

    // graphics

    m_scene = new QGraphicsScene(this);
    m_view = new QGraphicsView(m_scene, this);

    m_scene->setBackgroundBrush(QBrush(Qt::darkGray));

    this->setLayout(new QVBoxLayout());
    this->layout()->setContentsMargins(0, 0, 0, 0);
    this->layout()->addWidget(m_view);

    // verticalScrollBar

    connect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(slotVerticalScrollBarValueChanged(int)));

    // prefetchTimer

    m_prefetchTimer = new QTimer(this);
    m_prefetchTimer->setSingleShot(true);
    m_prefetchTimer->setInterval(500);

    connect(this, SIGNAL(currentPageChanged(int)), m_prefetchTimer, SLOT(start()));
    connect(this, SIGNAL(pageLayoutChanged(PageLayout)), m_prefetchTimer, SLOT(start()));
    connect(this, SIGNAL(scalingChanged(Scaling)), m_prefetchTimer, SLOT(start()));
    connect(this, SIGNAL(rotationChanged(Rotation)), m_prefetchTimer, SLOT(start()));

    connect(m_prefetchTimer, SIGNAL(timeout()), this, SLOT(prefetch()));

    // makeCurrentTabAction

    m_makeCurrentTabAction = new QAction(this);

    connect(m_makeCurrentTabAction, SIGNAL(triggered()), this, SLOT(slotMakeCurrentTab()));

    // bookmarksMenu

    m_bookmarksMenu = new BookmarksMenu(this);

    connect(m_bookmarksMenu, SIGNAL(entrySelected(int,qreal)), this, SLOT(slotBookmarksEntrySelected(int,qreal)));
}

const QString &DocumentView::filePath() const
{
    return m_document->filePath();
}

int DocumentView::numberOfPages() const
{
    return m_document->numberOfPages();
}

int DocumentView::currentPage() const
{
    return m_currentPage;
}

PageLayout DocumentView::pageLayout() const
{
    return m_pageLayout;
}

void DocumentView::setPageLayout(PageLayout pageLayout)
{
    if(m_pageLayout != pageLayout)
    {
        m_pageLayout = pageLayout;

        if((m_pageLayout == TwoPages || m_pageLayout == TwoColumns) && m_currentPage % 2 == 0 )
        {
            m_currentPage -= 1;

            emit currentPageChanged(m_currentPage);
        }

        m_settings.setValue("documentView/pageLayout", static_cast<uint>(m_pageLayout));

        this->prepareScene();
        this->prepareView();

        emit pageLayoutChanged(m_pageLayout);
    }
}

Scaling DocumentView::scaling() const
{
    return m_scaling;
}

void DocumentView::setScaling(Scaling scaling)
{
    if(m_scaling != scaling)
    {
        m_scaling = scaling;

        m_settings.setValue("documentView/scaling", static_cast<uint>(m_scaling));

        this->prepareScene();
        this->prepareView();

        emit scalingChanged(m_scaling);
    }
}

Rotation DocumentView::rotation() const
{
    return m_rotation;
}

void DocumentView::setRotation(Rotation rotation)
{
    if(m_rotation != rotation)
    {
        m_rotation = rotation;

        m_settings.setValue("documentView/rotation", static_cast<uint>(m_rotation));

        this->prepareScene();
        this->prepareView();

        emit rotationChanged(m_rotation);
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

        m_settings.setValue("documentView/highlightAll", m_highlightAll);

        foreach(PageView *pageView, m_pageToPageView.values())
        {
            pageView->setHighlightAll(m_highlightAll);
        }

        emit highlightAllChanged(m_highlightAll);
    }
}

QAction *DocumentView::makeCurrentTabAction() const
{
    return m_makeCurrentTabAction;
}

bool DocumentView::open(const QString &filePath)
{
    return m_document->open(filePath);
}

bool DocumentView::refresh()
{
    return m_document->refresh();
}

bool DocumentView::saveCopy(const QString &filePath)
{
    return m_document->saveCopy(filePath);
}

void DocumentView::prefetch()
{
    if(m_document->numberOfPages() == -1)
    {
        return;
    }

    int fromPage = qMax(m_currentPage-2, 1);
    int toPage = qMin(m_currentPage+3, m_document->numberOfPages());

    for(int page = fromPage; page <= toPage; page++)
    {
        m_pageToPageView.value(page)->prefetch();
    }
}

void DocumentView::startSearch(const QString &text, bool matchCase)
{
    m_searchResults.clear();
    m_searchPosition = m_searchResults.end();

    m_highlight->setVisible(false);

    m_document->startSearch(text, matchCase);
}

void DocumentView::cancelSearch()
{
    m_searchResults.clear();
    m_searchPosition = m_searchResults.end();

    m_highlight->setVisible(false);

    m_document->cancelSearch();
}

void DocumentView::startPrint(QPrinter *printer, int fromPage, int toPage)
{
    m_document->startPrint(printer, fromPage, toPage);
}

void DocumentView::cancelPrint()
{
    m_document->cancelPrint();
}

void DocumentView::setCurrentPage(int currentPage, qreal top)
{
    if(currentPage >= 1 && currentPage <= m_document->numberOfPages())
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

        this->prepareView(top);

        emit currentPageChanged(m_currentPage);

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

            this->prepareView();

            emit currentPageChanged(m_currentPage);
        }

        break;
    case TwoPages:
    case TwoColumns:
        if(m_currentPage > 2)
        {
            m_currentPage -= 2;

            this->prepareView();

            emit currentPageChanged(m_currentPage);
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
        if(m_currentPage <= m_document->numberOfPages()-1)
        {
            m_currentPage += 1;

            this->prepareView();

            emit currentPageChanged(m_currentPage);
        }

        break;
    case TwoPages:
    case TwoColumns:
        if(m_currentPage <= m_document->numberOfPages()-2)
        {
            m_currentPage += 2;

            this->prepareView();

            emit currentPageChanged(m_currentPage);
        }

        break;
    }
}

void DocumentView::firstPage()
{
    if(m_currentPage != 1)
    {
        m_currentPage = 1;

        this->prepareView();

        emit currentPageChanged(m_currentPage);
    }
}

void DocumentView::lastPage()
{
    switch(m_pageLayout)
    {
    case OnePage:
    case OneColumn:
        if(m_currentPage != m_document->numberOfPages())
        {
            m_currentPage = m_document->numberOfPages();

            this->prepareView();

            emit currentPageChanged(m_currentPage);
        }

        break;
    case TwoPages:
    case TwoColumns:
        if(m_document->numberOfPages() % 2 == 0)
        {
            if(m_currentPage != m_document->numberOfPages()-1)
            {
                m_currentPage = m_document->numberOfPages()-1;

                this->prepareView();

                emit currentPageChanged(m_currentPage);
            }
        }
        else
        {
            if(m_currentPage != m_document->numberOfPages())
            {
                m_currentPage = m_document->numberOfPages();

                this->prepareView();

                emit currentPageChanged(m_currentPage);
            }
        }

        break;
    }
}

void DocumentView::findPrevious()
{
    if(m_searchPosition != m_searchResults.end())
    {
        switch(m_pageLayout)
        {
        case OnePage:
        case OneColumn:
            if(m_searchPosition.key() != m_currentPage)
            {
                m_searchPosition = --m_searchResults.upperBound(m_currentPage);
            }
            else
            {
                --m_searchPosition;
            }

            if(m_searchPosition == m_searchResults.end())
            {
                m_searchPosition = --m_searchResults.upperBound(m_document->numberOfPages());
            }

            break;
        case TwoPages:
        case TwoColumns:
            if(m_searchPosition.key() != m_currentPage && m_searchPosition.key() != m_currentPage+1)
            {
                m_searchPosition = --m_searchResults.upperBound(m_currentPage);
            }
            else
            {
                --m_searchPosition;
            }

            if(m_searchPosition == m_searchResults.end())
            {
                m_searchPosition = --m_searchResults.upperBound(m_document->numberOfPages());
            }

            break;
        }
    }
    else
    {
        m_searchPosition = --m_searchResults.upperBound(m_currentPage);

        if(m_searchPosition == m_searchResults.end())
        {
            m_searchPosition = --m_searchResults.upperBound(m_document->numberOfPages());
        }
    }

    if(m_searchPosition != m_searchResults.end())
    {
        this->setCurrentPage(m_searchPosition.key());

        // verticalScrollBar

        disconnect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(slotVerticalScrollBarValueChanged(int)));

        m_view->centerOn(m_highlight);

        connect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(slotVerticalScrollBarValueChanged(int)));

        // bookmarksMenu

        PageView *pageView = m_pageToPageView.value(m_currentPage);
        qreal top = qMax(0.0, static_cast<qreal>(m_view->verticalScrollBar()->value()) - pageView->y()) / pageView->boundingRect().height();

        m_bookmarksMenu->setPosition(m_currentPage, top);
    }
}

void DocumentView::findNext()
{
    if(m_searchPosition != m_searchResults.end())
    {
        switch(m_pageLayout)
        {
        case OnePage:
        case OneColumn:
            if(m_searchPosition.key() != m_currentPage)
            {
                m_searchPosition = --m_searchResults.upperBound(m_currentPage);
            }
            else
            {
                ++m_searchPosition;
            }

            if(m_searchPosition == m_searchResults.end())
            {
                m_searchPosition = m_searchResults.lowerBound(1);
            }

            break;
        case TwoPages:
        case TwoColumns:
            if(m_searchPosition.key() != m_currentPage && m_searchPosition.key() != m_currentPage+1)
            {
                m_searchPosition = --m_searchResults.upperBound(m_currentPage);
            }
            else
            {
                ++m_searchPosition;
            }

            if(m_searchPosition == m_searchResults.end())
            {
                m_searchPosition = m_searchResults.lowerBound(1);
            }

            break;
        }
    }
    else
    {
        m_searchPosition = m_searchResults.lowerBound(m_currentPage);

        if(m_searchPosition == m_searchResults.end())
        {
            m_searchPosition = m_searchResults.lowerBound(1);
        }
    }

    if(m_searchPosition != m_searchResults.end())
    {
        this->setCurrentPage(m_searchPosition.key());

        // verticalScrollBar

        disconnect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(slotVerticalScrollBarValueChanged(int)));

        m_view->centerOn(m_highlight);

        connect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(slotVerticalScrollBarValueChanged(int)));

        // bookmarksMenu

        PageView *pageView = m_pageToPageView.value(m_currentPage);
        qreal top = qMax(0.0, static_cast<qreal>(m_view->verticalScrollBar()->value()) - pageView->y()) / pageView->boundingRect().height();

        m_bookmarksMenu->setPosition(m_currentPage, top);
    }
}

bool DocumentView::eventFilter(QObject*, QEvent *event)
{
    if(event->type() == QEvent::Wheel)
    {
        QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);

        return wheelEvent->modifiers() == Qt::ControlModifier;
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

void DocumentView::wheelEvent(QWheelEvent *event)
{
    if(event->modifiers() == Qt::NoModifier)
    {
        switch(m_pageLayout)
        {
        case OnePage:
            if(event->delta() > 0 && m_view->verticalScrollBar()->value() == m_view->verticalScrollBar()->minimum() && m_currentPage != 1)
            {
                this->previousPage();

                m_view->verticalScrollBar()->setValue(m_view->verticalScrollBar()->maximum());
            }
            else if(event->delta() < 0 && m_view->verticalScrollBar()->value() == m_view->verticalScrollBar()->maximum() && m_currentPage != m_document->numberOfPages())
            {
                this->nextPage();

                m_view->verticalScrollBar()->setValue(m_view->verticalScrollBar()->minimum());
            }

            break;
        case TwoPages:
            if(event->delta() > 0 && m_view->verticalScrollBar()->value() == m_view->verticalScrollBar()->minimum() && m_currentPage != 1)
            {
                this->previousPage();

                m_view->verticalScrollBar()->setValue(m_view->verticalScrollBar()->maximum());
            }
            else if(event->delta() < 0 && m_view->verticalScrollBar()->value() == m_view->verticalScrollBar()->maximum() && m_currentPage != (m_document->numberOfPages() % 2 == 0 ? m_document->numberOfPages()-1 : m_document->numberOfPages()))
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
    else if(event->modifiers() == Qt::ControlModifier)
    {
        if(event->delta() > 0)
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
        else if(event->delta() < 0)
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
}

void DocumentView::keyPressEvent(QKeyEvent *event)
{
    QKeySequence shortcut(event->modifiers() + event->key());

    foreach(QAction *action, m_bookmarksMenu->actions())
    {
        if(action->shortcut() == shortcut)
        {
            action->trigger();
        }
    }
}

void DocumentView::contextMenuEvent(QContextMenuEvent *event)
{
    m_bookmarksMenu->exec(event->globalPos());
}

void DocumentView::slotFilePathChanged(const QString &filePath)
{
    m_makeCurrentTabAction->setText(QFileInfo(filePath).completeBaseName());

    if(!this->parent())
    {
        return;
    }

    if(!this->parent()->parent())
    {
        return;
    }

    QTabWidget *tabWidget = qobject_cast<QTabWidget*>(this->parent()->parent());

    if(tabWidget)
    {
        int index = tabWidget->indexOf(this);

        tabWidget->setTabText(index, QFileInfo(filePath).completeBaseName());
        tabWidget->setTabToolTip(index, QFileInfo(filePath).completeBaseName());
    }

    emit filePathChanged(filePath);
}

void DocumentView::slotNumberOfPagesChanged(int numberOfPages)
{
    if(m_currentPage > m_document->numberOfPages())
    {
        m_currentPage = 1;

        emit currentPageChanged(m_currentPage);
    }

    emit numberOfPagesChanged(numberOfPages);
}

void DocumentView::slotDocumentChanged()
{
    m_scene->clear();
    m_pageToPageView.clear();

    for(int index = 0; index < m_document->numberOfPages(); index++)
    {
        PageView *pageView = new PageView(m_document, index);

        connect(pageView, SIGNAL(linkLeftClicked(int,qreal)), this, SLOT(slotLinkLeftClicked(int,qreal)));
        connect(pageView, SIGNAL(linkLeftClicked(QString)), this, SLOT(slotLinkLeftClicked(QString)));
        connect(pageView, SIGNAL(linkMiddleClicked(int,qreal)), this, SLOT(slotLinkMiddleClicked(int,qreal)));
        connect(pageView, SIGNAL(linkMiddleClicked(QString)), this, SLOT(slotLinkMiddleClicked(QString)));

        m_scene->addItem(pageView);
        m_pageToPageView.insert(index+1, pageView);
    }

    m_highlight = new QGraphicsRectItem();
    m_highlight->setPen(QPen(QColor(0,255,0,255)));
    m_highlight->setBrush(QBrush(QColor(0,255,0,127)));

    m_scene->addItem(m_highlight);

    this->prepareScene();
    this->prepareView();

    m_prefetchTimer->start();
}

void DocumentView::slotPageSearched(int index)
{
    QList<QRectF> results = m_document->searchResults(index);

    m_searchResults.remove(index+1);

    while(!results.isEmpty())
    {
        m_searchResults.insertMulti(index+1, results.takeLast());
    }

    if(m_searchResults.contains(index+1) && m_searchPosition == m_searchResults.end() && m_currentPage <= index+1)
    {
        this->findNext();
    }
}

void DocumentView::slotSearchProgressed(int value)
{
    emit searchProgressed(value);
}

void DocumentView::slotSearchCanceled()
{
    emit searchCanceled();
}

void DocumentView::slotSearchFinished()
{
    emit searchFinished();
}

void DocumentView::slotPrintProgressed(int value)
{
    emit printProgressed(value);
}

void DocumentView::slotPrintCanceled()
{
    emit printCanceled();
}

void DocumentView::slotPrintFinished()
{
    emit printFinished();
}

void DocumentView::slotVerticalScrollBarValueChanged(int value)
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

    // bookmarksMenu

    PageView *pageView = m_pageToPageView.value(m_currentPage);
    qreal top = qMax(0.0, static_cast<qreal>(value) - pageView->y()) / pageView->boundingRect().height();

    m_bookmarksMenu->setPosition(m_currentPage, top);
}

void DocumentView::slotMakeCurrentTab()
{
    if(!this->parent())
    {
        return;
    }

    if(!this->parent()->parent())
    {
        return;
    }

    QTabWidget *tabWidget = qobject_cast<QTabWidget*>(this->parent()->parent());

    if(tabWidget)
    {
        tabWidget->setCurrentIndex(tabWidget->indexOf(this));
    }
    else
    {
        qCritical("!tabWidget");
    }
}

void DocumentView::slotBookmarksEntrySelected(int page, qreal top)
{
    this->setCurrentPage(page, top);
}

void DocumentView::slotLinkLeftClicked(int page, qreal top)
{
    this->setCurrentPage(page, top);
}

void DocumentView::slotLinkLeftClicked(const QString &url)
{
    if(Document::openUrl())
    {
        QDesktopServices::openUrl(QUrl(url));
    }
    else
    {
        QMessageBox::information(0, tr("Information"), tr("Opening URL is disabled in the settings."));
    }
}

void DocumentView::slotLinkMiddleClicked(int page, qreal top)
{
    MainWindow *mainWindow = qobject_cast<MainWindow*>(this->parent()->parent()->parent());

    if(mainWindow)
    {
        mainWindow->addTab(m_document->filePath(), page, top);
    }
}

void DocumentView::slotLinkMiddleClicked(const QString &url)
{
    Q_UNUSED(url);
}

void DocumentView::prepareScene()
{
    if(m_document->numberOfPages() == -1)
    {
        return;
    }

    // calculate scale factor

    QSizeF size;
    qreal pageWidth = 0.0, pageHeight = 0.0;
    qreal viewWidth = static_cast<qreal>(m_view->width()), viewHeight = static_cast<qreal>(m_view->height());
    qreal scaleFactor = 4.0 * physicalDpiX() / 72.0;

    switch(m_scaling)
    {
    case FitToPage:
    case FitToPageWidth:
        switch(m_pageLayout)
        {
        case OnePage:
        case OneColumn:
            for(int page = 1; page <= m_document->numberOfPages(); page++)
            {
                size = m_pageToPageView.value(page)->size();

                switch(m_rotation)
                {
                case RotateBy0:
                case RotateBy180:
                    pageWidth = size.width();
                    pageHeight = size.height();

                    break;
                case RotateBy90:
                case RotateBy270:
                    pageWidth = size.height();
                    pageHeight = size.width();

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
            for(int page = 1; page <= (m_document->numberOfPages() % 2 != 0 ? m_document->numberOfPages()-1 : m_document->numberOfPages()); page += 2)
            {
                size = m_pageToPageView.value(page)->size();

                switch(m_rotation)
                {
                case RotateBy0:
                case RotateBy180:
                    pageWidth = size.width();
                    pageHeight = size.height();

                    break;
                case RotateBy90:
                case RotateBy270:
                    pageWidth = size.height();
                    pageHeight = size.width();

                    break;
                }

                size = m_pageToPageView.value(page+1)->size();

                switch(m_rotation)
                {
                case RotateBy0:
                case RotateBy180:
                    pageWidth += size.width();
                    pageHeight = qMax(pageHeight, size.height());

                    break;
                case RotateBy90:
                case RotateBy270:
                    pageWidth += size.height();
                    pageHeight = qMax(pageHeight, size.width());

                    break;
                }

                scaleFactor = qMin(scaleFactor, 0.95 * viewWidth / (pageWidth + 30.0));
                if(m_scaling == FitToPage)
                {
                    scaleFactor = qMin(scaleFactor, 0.95 * viewHeight / (pageHeight + 20.0));
                }
            }

            if(m_document->numberOfPages() % 2 != 0)
            {
                size = m_pageToPageView.value(m_document->numberOfPages())->size();

                switch(m_rotation)
                {
                case RotateBy0:
                case RotateBy180:
                    pageWidth = size.width();
                    pageHeight = size.height();

                    break;
                case RotateBy90:
                case RotateBy270:
                    pageWidth = size.height();
                    pageHeight = size.width();

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
        scaleFactor = 0.50 * physicalDpiX() / 72.0;
        break;
    case ScaleTo75:
        scaleFactor = 0.75 * physicalDpiX() / 72.0;
        break;
    case ScaleTo100:
        scaleFactor = 1.00 * physicalDpiX() / 72.0;
        break;
    case ScaleTo125:
        scaleFactor = 1.25 * physicalDpiX() / 72.0;
        break;
    case ScaleTo150:
        scaleFactor = 1.50 * physicalDpiX() / 72.0;
        break;
    case ScaleTo200:
        scaleFactor = 2.00 * physicalDpiX() / 72.0;
        break;
    case ScaleTo400:
        scaleFactor = 4.00 * physicalDpiX() / 72.0;
        break;
    }

    foreach(PageView *pageView, m_pageToPageView.values())
    {
        pageView->setScaleFactor(scaleFactor);
        pageView->setRotation(m_rotation);
    }

    // calculate layout

    qreal sceneWidth = 0.0, sceneHeight = 10.0;

    m_heightToPage.clear();

    m_bookmarksMenu->clearDocument();

    switch(m_pageLayout)
    {
    case OnePage:
    case OneColumn:
        for(int page = 1; page <= m_document->numberOfPages(); page++)
        {
            PageView *pageView = m_pageToPageView.value(page);

            pageView->setPos(10.0, sceneHeight);
            m_heightToPage.insert(-qFloor(pageView->y() - 0.4 * pageView->boundingRect().height()), page);

            sceneWidth = qMax(sceneWidth, pageView->boundingRect().width() + 20.0);
            sceneHeight += pageView->boundingRect().height() + 10.0;
        }

        break;
    case TwoPages:
    case TwoColumns:
        for(int page = 1; page <= (m_document->numberOfPages() % 2 != 0 ? m_document->numberOfPages()-1 : m_document->numberOfPages()); page += 2)
        {
            PageView *pageView = m_pageToPageView.value(page);

            pageView->setPos(10.0, sceneHeight);
            m_heightToPage.insert(-qFloor(pageView->y() - 0.4 * pageView->boundingRect().height()), page);

            PageView *nextPageView = m_pageToPageView.value(page+1);

            nextPageView->setPos(pageView->boundingRect().width() + 20.0, sceneHeight);

            sceneWidth = qMax(sceneWidth, pageView->boundingRect().width() + nextPageView->boundingRect().width() + 30.0);
            sceneHeight += qMax(pageView->boundingRect().height(), nextPageView->boundingRect().height()) + 10.0;
        }

        if(m_document->numberOfPages() % 2 != 0)
        {
            PageView *pageView = m_pageToPageView.value(m_document->numberOfPages());

            pageView->setPos(10.0, sceneHeight);
            m_heightToPage.insert(-qFloor(pageView->y() - 0.4 * pageView->boundingRect().height()), m_document->numberOfPages());

            sceneWidth = qMax(sceneWidth, pageView->boundingRect().width() + 20.0);
            sceneHeight += pageView->boundingRect().height() + 10.0;
        }

        break;
    }

    m_scene->setSceneRect(0.0, 0.0, sceneWidth, sceneHeight);
}

void DocumentView::prepareView(qreal top)
{
    if(m_document->numberOfPages() == -1)
    {
        return;
    }

    // calculate layout

    PageView *pageView = m_pageToPageView.value(m_currentPage), *nextPageView = 0;

    switch(m_pageLayout)
    {
    case OnePage:
        foreach(PageView *pageView, m_pageToPageView.values())
        {
            pageView->setVisible(false);
        }

        m_view->setSceneRect(pageView->x()-10.0, pageView->y()-10.0,
                           pageView->boundingRect().width()+20.0,
                           pageView->boundingRect().height()+20.0);

        pageView->setVisible(true);

        break;
    case TwoPages:
        foreach(PageView *pageView, m_pageToPageView.values())
        {
            pageView->setVisible(false);
        }

        if(m_pageToPageView.contains(m_currentPage+1))
        {
            nextPageView = m_pageToPageView.value(m_currentPage+1);


            m_view->setSceneRect(pageView->x()-10.0, pageView->y()-10.0,
                                 pageView->boundingRect().width() + nextPageView->boundingRect().width() + 30.0,
                                 qMax(pageView->boundingRect().height(), nextPageView->boundingRect().height()) + 20.0);

            pageView->setVisible(true);
            nextPageView->setVisible(true);
        }
        else
        {
            m_view->setSceneRect(pageView->x()-10.0, pageView->y()-10.0,
                               pageView->boundingRect().width()+20.0,
                               pageView->boundingRect().height()+20.0);

            pageView->setVisible(true);
        }

        break;
    case OneColumn:
    case TwoColumns:
        foreach(PageView *pageView, m_pageToPageView.values())
        {
            pageView->setVisible(true);
        }

        m_view->setSceneRect(QRectF());

        break;
    }

    // highlight

    if(m_searchPosition != m_searchResults.end())
    {
        PageView *pageView = m_pageToPageView.value(m_searchPosition.key());

        m_highlight->setPos(pageView->pos());
        m_highlight->setTransform(pageView->highlightTransform());

        m_highlight->setRect(m_searchPosition.value().adjusted(-1.0, -1.0, 1.0, 1.0));

        m_highlight->setVisible(true);
    }
    else
    {
        m_highlight->setVisible(false);
    }

    m_view->show();

    // verticalScrollBar

    disconnect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(slotVerticalScrollBarValueChanged(int)));

    m_view->verticalScrollBar()->setValue(qCeil(pageView->y() + pageView->boundingRect().height() * top));

    connect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(slotVerticalScrollBarValueChanged(int)));

    // bookmarksMenu

    m_bookmarksMenu->setPosition(m_currentPage, top);
}
