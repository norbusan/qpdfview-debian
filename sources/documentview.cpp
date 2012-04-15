#include "documentview.h"

DocumentView::PageItem::PageItem(QGraphicsItem *parent, QGraphicsScene *scene) : QGraphicsItem(parent, scene),
    m_page(0),
    m_index(-1),
    m_scale(1.0),
    m_rotation(DocumentView::RotateBy0),
    m_highlightAll(false),
    m_links(),
    m_highlight(),
    m_rubberBand(),
    m_pageTransform(),
    m_linkTransform(),
    m_highlightTransform(),
    m_render()
{
    setAcceptHoverEvents(true);
}

DocumentView::PageItem::~PageItem()
{
    m_render.waitForFinished();

    if(m_page)
    {
        delete m_page;
    }
}

QRectF DocumentView::PageItem::boundingRect() const
{
    QSizeF size = m_page->pageSizeF();
    QRectF rect;

    switch(m_rotation)
    {
    case DocumentView::RotateBy0:
    case DocumentView::RotateBy180:
        rect = QRectF(0.0, 0.0, qCeil(m_scale * size.width()), qCeil(m_scale * size.height()));

        break;
    case DocumentView::RotateBy90:
    case DocumentView::RotateBy270:
        rect = QRectF(0.0, 0.0, qCeil(m_scale * size.height()), qCeil(m_scale * size.width()));

        break;
    }

    return rect;
}

void DocumentView::PageItem::paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    DocumentView* parent = qobject_cast<DocumentView*>(scene()->parent());

    if(!parent)
    {
        qFatal("!parent");
        return;
    }

    // page

    painter->fillRect(boundingRect(), QBrush(Qt::white));

    parent->m_pageCacheMutex.lock();

    DocumentView::PageCacheKey key(m_index, m_scale);

    if(parent->m_pageCache.contains(key))
    {
        painter->setTransform(m_pageTransform, true);
        painter->drawImage(0.0, 0.0, parent->m_pageCache.value(key));
        painter->setTransform(m_pageTransform.inverted(), true);
    }
    else
    {
        if(!m_render.isRunning())
        {
            m_render = QtConcurrent::run(this, &DocumentView::PageItem::render);
        }
    }

    parent->m_pageCacheMutex.unlock();

    painter->setPen(QPen(Qt::black));
    painter->drawRect(boundingRect());

    // links

    painter->setPen(QPen(QColor(255,0,0,127)));
    painter->setTransform(m_linkTransform, true);

    foreach(Link link, m_links)
    {
        painter->drawRect(link.area);
    }

    painter->setTransform(m_linkTransform.inverted(), true);

    // highlight

    if(!m_highlight.isNull())
    {
        painter->fillRect(m_highlight, QBrush(QColor(0,0,255,127)));
    }

    painter->setTransform(m_highlightTransform.inverted(), true);

    // rubber band

    if(!m_rubberBand.isNull())
    {
        QPen pen;
        pen.setColor(Qt::black);
        pen.setStyle(Qt::DashLine);
        painter->setPen(pen);

        painter->drawRect(m_rubberBand);
    }
}

void DocumentView::PageItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    QApplication::restoreOverrideCursor();

    foreach(Link link, m_links)
    {
        if(m_linkTransform.mapRect(link.area).contains(event->pos()))
        {
            QApplication::setOverrideCursor(Qt::PointingHandCursor);

            if(link.page != -1)
            {
                QToolTip::showText(event->screenPos(), tr("Go to page %1.").arg(link.page));
            }
            else if(!link.url.isEmpty())
            {
                QToolTip::showText(event->screenPos(), tr("Open URL \"%1\".").arg(link.url));
            }

            return;
        }
    }

    QToolTip::hideText();
}

void DocumentView::PageItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    DocumentView* parent = qobject_cast<DocumentView*>(scene()->parent());

    if(!parent)
    {
        qFatal("!parent");
        return;
    }

    foreach(Link link, m_links)
    {
        if(m_linkTransform.mapRect(link.area).contains(event->pos()))
        {
            if(link.page != -1)
            {
                parent->setCurrentPage(link.page, link.top);
            }
            else if(!link.url.isEmpty())
            {
                QDesktopServices::openUrl(QUrl(link.url));
            }

            return;
        }
    }

    m_rubberBand = QRectF(event->pos(), QSizeF());
}

void DocumentView::PageItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if(!m_rubberBand.isNull())
    {
        m_rubberBand.setBottomRight(event->pos());

        scene()->update(boundingRect().translated(pos()));
    }
}

void DocumentView::PageItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    DocumentView* parent = qobject_cast<DocumentView*>(scene()->parent());

    if(!parent)
    {
        qFatal("!parent");
        return;
    }

    if(!m_rubberBand.isNull())
    {
        m_rubberBand.setBottomRight(event->pos());

        m_highlight = m_highlightTransform.inverted().mapRect(m_rubberBand).adjusted(-1.0, -1.0, 1.0, 1.0);

        m_rubberBand = QRectF();

        parent->m_documentMutex.lock();

        QString text = m_page->text(m_highlight);

        parent->m_documentMutex.unlock();

        if(!text.isEmpty())
        {
            QApplication::clipboard()->setText(text);
        }

        scene()->update(boundingRect().translated(pos()));
    }
}

void DocumentView::PageItem::prepareTransforms()
{
    QSizeF size = m_page->pageSizeF();

    switch(m_rotation)
    {
    case DocumentView::RotateBy0:
        m_pageTransform = QTransform(1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
        m_linkTransform = QTransform(m_scale * size.width(), 0.0, 0.0, m_scale * size.height(), 0.0, 0.0);
        m_highlightTransform = QTransform(m_scale, 0.0, 0.0, m_scale, 0.0, 0.0);

        break;
    case DocumentView::RotateBy90:
        m_pageTransform = QTransform(0.0, 1.0, -1.0, 0.0, m_scale * size.height(), 0.0);
        m_linkTransform = QTransform(0.0, m_scale * size.width(), -m_scale * size.height(), 0.0, m_scale * size.height(), 0.0);
        m_highlightTransform = QTransform(0.0, m_scale, -m_scale, 0.0, m_scale * size.height(), 0.0);

        break;
    case DocumentView::RotateBy180:
        m_pageTransform = QTransform(-1.0, 0.0, 0.0, -1.0, m_scale * size.width(), m_scale * size.height());
        m_linkTransform = QTransform(-m_scale * size.width(), 0.0, 0.0, -m_scale * size.height(), m_scale * size.width(), m_scale * size.height());
        m_highlightTransform = QTransform(-m_scale, 0.0, 0.0, -m_scale, m_scale * size.width(), m_scale * size.height());

        break;
    case DocumentView::RotateBy270:
        m_pageTransform = QTransform(0.0, -1.0, 1.0, 0.0, 0.0, m_scale * size.width());
        m_linkTransform = QTransform(0.0, -m_scale * size.width(), m_scale * size.height(), 0.0, 0.0, m_scale * size.width());
        m_highlightTransform = QTransform(0.0, -m_scale, m_scale, 0.0, 0.0, m_scale * size.width());

        break;
    }
}

void DocumentView::PageItem::render()
{
    DocumentView* parent = qobject_cast<DocumentView*>(scene()->parent());

    if(!parent)
    {
        qFatal("!parent");
        return;
    }

    QRectF rect = boundingRect().translated(pos());
    QRectF visibleRect = parent->m_view->mapToScene(parent->m_view->rect()).boundingRect();

    if(!rect.intersects(visibleRect))
    {
        return;
    }

    parent->m_documentMutex.lock();

    QImage image = m_page->renderToImage(m_scale * 72.0, m_scale * 72.0);

    parent->m_documentMutex.unlock();

    parent->m_pageCacheMutex.lock();

    DocumentView::PageCacheKey key(m_index, m_scale);

    while(parent->m_pageCacheSize > parent->m_maximumPageCacheSize)
    {
        QMap< DocumentView::PageCacheKey, QImage >::iterator iterator = parent->m_pageCache.begin();

        parent->m_pageCacheSize -= iterator.value().byteCount();
        parent->m_pageCache.remove(iterator.key());
    }

    parent->m_pageCache.insert(key, image);
    parent->m_pageCacheSize += image.byteCount();

    parent->m_pageCacheMutex.unlock();

    scene()->update(rect);
}

DocumentView::ThumbnailItem::ThumbnailItem(QGraphicsItem *parent, QGraphicsScene *scene) : QGraphicsItem(parent, scene),
    m_page(0),
    m_index(-1),
    m_render()
{
}

DocumentView::ThumbnailItem::~ThumbnailItem()
{
    m_render.waitForFinished();

    if(m_page)
    {
        delete m_page;
    }
}

QRectF DocumentView::ThumbnailItem::boundingRect() const
{
    QSizeF size = m_page->pageSizeF();

    return QRectF(0.0, 0.0, qCeil(0.1 * size.width()), qCeil(0.1 * size.height()));
}

void DocumentView::ThumbnailItem::paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    DocumentView* parent = qobject_cast<DocumentView*>(scene()->parent());

    if(!parent)
    {
        qFatal("!parent");
        return;
    }

    // page

    painter->fillRect(boundingRect(), QBrush(Qt::white));

    parent->m_pageCacheMutex.lock();

    DocumentView::PageCacheKey key(m_index, 0.1);

    if(parent->m_pageCache.contains(key))
    {
        painter->drawImage(0.0, 0.0, parent->m_pageCache.value(key));
    }
    else
    {
        if(!m_render.isRunning())
        {
            m_render = QtConcurrent::run(this, &DocumentView::ThumbnailItem::render);
        }
    }

    parent->m_pageCacheMutex.unlock();

    painter->setPen(QPen(Qt::black));
    painter->drawRect(boundingRect());
}

void DocumentView::ThumbnailItem::mousePressEvent(QGraphicsSceneMouseEvent*)
{
    DocumentView* parent = qobject_cast<DocumentView*>(scene()->parent());

    if(!parent)
    {
        qFatal("!parent");
        return;
    }

    parent->setCurrentPage(m_index + 1);
}

void DocumentView::ThumbnailItem::render()
{
    DocumentView* parent = qobject_cast<DocumentView*>(scene()->parent());

    if(!parent)
    {
        qFatal("!parent");
        return;
    }

    QRectF rect = boundingRect().translated(pos());
    QRectF visibleRect = parent->m_thumbnailsGraphicsView->mapToScene(parent->m_thumbnailsGraphicsView->rect()).boundingRect();

    if(!rect.intersects(visibleRect))
    {
        return;
    }

    parent->m_documentMutex.lock();

    QImage image = m_page->renderToImage(7.20, 7.20);

    parent->m_documentMutex.unlock();

    parent->m_pageCacheMutex.lock();

    DocumentView::PageCacheKey key(m_index, 0.1);

    while(parent->m_pageCacheSize > parent->m_maximumPageCacheSize)
    {
        QMap< DocumentView::PageCacheKey, QImage >::iterator iterator = parent->m_pageCache.begin();

        parent->m_pageCacheSize -= iterator.value().byteCount();
        parent->m_pageCache.remove(iterator.key());
    }

    parent->m_pageCache.insert(key, image);
    parent->m_pageCacheSize += image.byteCount();

    parent->m_pageCacheMutex.unlock();

    scene()->update(rect);
}

DocumentView::DocumentView(QWidget *parent) : QWidget(parent),
    m_document(0),
    m_documentMutex(),
    m_pageCache(),
    m_pageCacheMutex(),
    m_pageCacheSize(0u),
    m_maximumPageCacheSize(134217728u),
    m_filePath(),
    m_numberOfPages(-1),
    m_currentPage(-1),
    m_pageLayout(OnePage),
    m_scaling(ScaleTo100),
    m_rotation(RotateBy0),
    m_highlightAll(false),
    m_pagesByIndex(),
    m_pagesByHeight(),
    m_search(),
    m_print()
{
    // settings

    m_maximumPageCacheSize = m_settings.value("documentView/maximumPageCacheSize", 134217728u).toUInt();
    m_pageLayout = static_cast<PageLayout>(m_settings.value("documentView/pageLayout", 0).toUInt());
    m_scaling = static_cast<Scaling>(m_settings.value("documentView/scaling", 4).toUInt());
    m_rotation = static_cast<Rotation>(m_settings.value("documentView/rotation", 0).toUInt());
    m_highlightAll = m_settings.value("documentView/highlightAll", false).toBool();

    // graphics

    m_scene = new QGraphicsScene(this);
    m_scene->setBackgroundBrush(QBrush(Qt::darkGray));

    m_view = new QGraphicsView(m_scene, this);
    m_view->show();

    // verticalScrollBar

    m_view->verticalScrollBar()->installEventFilter(this);

    connect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(slotVerticalScrollBarValueChanged(int)));

    // tabAction

    m_tabAction = new QAction(this);

    connect(m_tabAction, SIGNAL(triggered()), this, SLOT(slotTabActionTriggered()));

    // outline

    m_outlineTreeWidget = new QTreeWidget();
    m_outlineTreeWidget->setAlternatingRowColors(true);
    m_outlineTreeWidget->header()->setVisible(false);

    connect(m_outlineTreeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(slotOutlineTreeWidgetItemClicked(QTreeWidgetItem*,int)));

    // thumbnails

    m_thumbnailsGraphicsView = new QGraphicsView(new QGraphicsScene(this));
}

DocumentView::~DocumentView()
{
    m_scene->clear();

    if(m_document)
    {
        delete m_document;
    }

    delete m_outlineTreeWidget;
}

uint DocumentView::maximumPageCacheSize() const
{
    return m_maximumPageCacheSize;
}

void DocumentView::setMaximumPageCacheSize(uint maximumPageCacheSize)
{
    if(m_maximumPageCacheSize != maximumPageCacheSize)
    {
        m_maximumPageCacheSize = maximumPageCacheSize;

        m_settings.setValue("documentView/maximumPageCacheSize", m_maximumPageCacheSize);

        m_pageCacheMutex.lock();

        while(m_pageCacheSize > m_maximumPageCacheSize)
        {
            QMap< DocumentView::PageCacheKey, QImage >::iterator iterator = m_pageCache.begin();

            m_pageCacheSize -= iterator.value().byteCount();
            m_pageCache.remove(iterator.key());
        }

        m_pageCacheMutex.unlock();
    }
}

const QString &DocumentView::filePath() const
{
    return m_filePath;
}

int DocumentView::numberOfPages() const
{
    return m_numberOfPages;
}

int DocumentView::currentPage() const
{
    return m_currentPage;
}

void DocumentView::setCurrentPage(int currentPage, qreal top)
{
    if(currentPage >= 1 && currentPage <= m_numberOfPages)
    {
        switch(m_pageLayout)
        {
        case OnePage:
        case OneColumn:
            if(m_currentPage != currentPage)
            {
                m_currentPage = currentPage;

                prepareView(top);

                emit currentPageChanged(m_currentPage);
            }

            break;
        case TwoPages:
        case TwoColumns:
            if(m_currentPage != (currentPage % 2 != 0 ? currentPage : currentPage - 1))
            {
                m_currentPage = currentPage % 2 != 0 ? currentPage : currentPage - 1;

                prepareView(top);

                emit currentPageChanged(m_currentPage);
            }

            break;
        }
    }
}

DocumentView::PageLayout DocumentView::pageLayout() const
{
    return m_pageLayout;
}

void DocumentView::setPageLayout(DocumentView::PageLayout pageLayout)
{
    if(m_pageLayout != pageLayout)
    {
        m_pageLayout = pageLayout;

        m_settings.setValue("documentView/pageLayout", static_cast<uint>(m_pageLayout));

        if((m_pageLayout == TwoPages || m_pageLayout == TwoColumns) && m_currentPage % 2 == 0)
        {
            m_currentPage -= 1;

            emit currentPageChanged(m_currentPage);
        }

        prepareScene();
        prepareView();

        emit pageLayoutChanged(m_pageLayout);
    }
}

DocumentView::Scaling DocumentView::scaling() const
{
    return m_scaling;
}

void DocumentView::setScaling(DocumentView::Scaling scaling)
{
    if(m_scaling != scaling)
    {
        m_scaling = scaling;

        m_settings.setValue("documentView/scaling", static_cast<uint>(m_scaling));

        prepareScene();
        prepareView();

        emit scalingChanged(m_scaling);
    }
}

DocumentView::Rotation DocumentView::rotation() const
{
    return m_rotation;
}

void DocumentView::setRotation(DocumentView::Rotation rotation)
{
    if(m_rotation != rotation)
    {
        m_rotation = rotation;

        m_settings.setValue("documentView/rotation", static_cast<uint>(m_rotation));

        prepareScene();
        prepareView();

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

        foreach(PageItem *pageItem, m_pagesByIndex.values())
        {
            pageItem->m_highlightAll = m_highlightAll;

            m_scene->update(pageItem->boundingRect().translated(pageItem->pos()));
        }

        emit highlightAllChanged(m_highlightAll);
    }
}

QAction *DocumentView::tabAction() const
{
    return m_tabAction;
}

QTreeWidget *DocumentView::outlineTreeWidget() const
{
    return m_outlineTreeWidget;
}

QGraphicsView *DocumentView::thumbnailsGraphicsView() const
{
    return m_thumbnailsGraphicsView;
}

bool DocumentView::open(const QString &filePath)
{
    m_scene->clear();
    m_thumbnailsGraphicsView->scene()->clear();

    Poppler::Document *document = Poppler::Document::load(filePath);

    if(document)
    {
        if(m_document)
        {
            delete m_document;
        }

        m_document = document;

        m_document->setRenderHint(Poppler::Document::Antialiasing, true);
        m_document->setRenderHint(Poppler::Document::TextAntialiasing, true);
        m_document->setRenderHint(Poppler::Document::TextHinting, true);
        m_document->setRenderHint(Poppler::Document::TextSlightHinting, true);

        m_filePath = filePath;
        m_numberOfPages = m_document->numPages();

        emit filePathChanged(m_filePath);
        emit numberOfPagesChanged(m_numberOfPages);

        m_currentPage = 1;

        emit currentPageChanged(m_currentPage);

        preparePages();

        m_tabAction->setText(QFileInfo(m_filePath).completeBaseName());

        prepareOutline();
        prepareThumbnails();
    }

    m_pageCache.clear();
    m_pageCacheSize = 0u;

    prepareScene();
    prepareView();

    return document != 0;
}

bool DocumentView::refresh()
{
    m_scene->clear();
    m_thumbnailsGraphicsView->scene()->clear();

    Poppler::Document *document = Poppler::Document::load(m_filePath);

    if(document)
    {
        if(m_document)
        {
            delete m_document;
        }

        m_document = document;

        m_document->setRenderHint(Poppler::Document::Antialiasing, true);
        m_document->setRenderHint(Poppler::Document::TextAntialiasing, true);
        m_document->setRenderHint(Poppler::Document::TextHinting, true);
        m_document->setRenderHint(Poppler::Document::TextSlightHinting, true);

        int numberOfPages = m_document->numPages();

        if(m_numberOfPages != numberOfPages)
        {
            m_numberOfPages = numberOfPages;

            emit numberOfPagesChanged(m_numberOfPages);
        }

        if(m_currentPage > m_numberOfPages)
        {
            m_currentPage = 1;

            emit currentPageChanged(m_currentPage);
        }

        m_pageCache.clear();
        m_pageCacheSize = 0u;

        preparePages();

        m_tabAction->setText(QFileInfo(m_filePath).completeBaseName());

        prepareOutline();
        prepareThumbnails();
    }

    prepareScene();
    prepareView();

    return document != 0;
}

bool DocumentView::saveCopy(const QString &filePath)
{
    if(m_document)
    {
            Poppler::PDFConverter *converter = m_document->pdfConverter();

            converter->setOutputFileName(filePath);

            bool success = converter->convert();

            delete converter;

            return success;
    }

    return false;
}

void DocumentView::close()
{
    m_scene->clear();
    m_thumbnailsGraphicsView->scene()->clear();

    if(m_document)
    {
        delete m_document;
    }

    m_document = 0;

    m_filePath = QString();
    m_numberOfPages = -1;
    m_currentPage = -1;

    emit filePathChanged(m_filePath);
    emit numberOfPagesChanged(m_numberOfPages);
    emit currentPageChanged(m_currentPage);

    m_pageCache.clear();
    m_pageCacheSize = 0u;

    preparePages();

    m_tabAction->setText(QString());

    prepareOutline();
    prepareThumbnails();

    prepareScene();
    prepareView();
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
        if(m_currentPage <= m_numberOfPages-1)
        {
            m_currentPage += 1;

            this->prepareView();

            emit currentPageChanged(m_currentPage);
        }

        break;
    case TwoPages:
    case TwoColumns:
        if(m_currentPage <= m_numberOfPages-2)
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
        if(m_currentPage != m_numberOfPages)
        {
            m_currentPage = m_numberOfPages;

            this->prepareView();

            emit currentPageChanged(m_currentPage);
        }

        break;
    case TwoPages:
    case TwoColumns:
        if(m_currentPage != (m_numberOfPages % 2 != 0 ? m_numberOfPages : m_numberOfPages - 1))
        {
            m_currentPage = m_numberOfPages % 2 != 0 ? m_numberOfPages : m_numberOfPages - 1;

            this->prepareView();

            emit currentPageChanged(m_currentPage);
        }

        break;
    }
}


void DocumentView::startSearch(const QString &text, bool matchCase)
{
    this->cancelSearch();

    if(m_document != 0 && !text.isEmpty())
    {
        m_search = QtConcurrent::run(this, &DocumentView::search, text, matchCase);
    }
}

void DocumentView::cancelSearch()
{
    if(m_search.isRunning())
    {
        m_search.cancel();
        m_search.waitForFinished();
    }

    // TODO: clear results
}

void DocumentView::findPrevious()
{
    // TODO
}

void DocumentView::findNext()
{
    // TODO
}

void DocumentView::startPrint(QPrinter *printer, int fromPage, int toPage)
{
    this->cancelPrint();

    if(m_document != 0 && fromPage >= 1 && fromPage <= m_numberOfPages && toPage >= 1 && toPage <= m_numberOfPages && fromPage <= toPage)
    {
        m_print = QtConcurrent::run(this, &DocumentView::print, printer, fromPage, toPage);
    }
}

void DocumentView::cancelPrint()
{
    if(m_print.isRunning())
    {
        m_print.cancel();
        m_print.waitForFinished();
    }
}

bool DocumentView::eventFilter(QObject*, QEvent *event)
{
    if(event->type() == QEvent::Wheel)
    {
        QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);

        return wheelEvent->modifiers() == Qt::ControlModifier || wheelEvent->modifiers() == Qt::ShiftModifier;
    }

    return false;
}

void DocumentView::resizeEvent(QResizeEvent *event)
{
    m_view->resize(event->size());

    if(m_scaling == FitToPage || m_scaling == FitToPageWidth)
    {
        prepareScene();
        prepareView();
    }
}

void DocumentView::keyPressEvent(QKeyEvent *event)
{
    // TODO
}

void DocumentView::wheelEvent(QWheelEvent *event)
{
    if(event->modifiers() == Qt::NoModifier)
    {
        switch(m_pageLayout)
        {
        case OnePage:
        case TwoPages:
            if(event->delta() > 0 && m_view->verticalScrollBar()->value() == m_view->verticalScrollBar()->minimum())
            {
                previousPage();

                m_view->verticalScrollBar()->setValue(m_view->verticalScrollBar()->maximum());
            }
            else if(event->delta() < 0 && m_view->verticalScrollBar()->value() == m_view->verticalScrollBar()->maximum())
            {
                nextPage();

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
            switch(m_scaling)
            {
            case FitToPage:
                setScaling(FitToPageWidth); break;
            case FitToPageWidth:
                setScaling(ScaleTo50); break;
            case ScaleTo50:
                setScaling(ScaleTo75); break;
            case ScaleTo75:
                setScaling(ScaleTo100); break;
            case ScaleTo100:
                setScaling(ScaleTo125); break;
            case ScaleTo125:
                setScaling(ScaleTo150); break;
            case ScaleTo150:
                setScaling(ScaleTo200); break;
            case ScaleTo200:
                setScaling(ScaleTo400); break;
            case ScaleTo400:
                break;
            }
        }
        else if(event->delta() < 0)
        {
            switch(m_scaling)
            {
            case FitToPage:
                break;
            case FitToPageWidth:
                setScaling(FitToPage); break;
            case ScaleTo50:
                setScaling(FitToPageWidth); break;
            case ScaleTo75:
                setScaling(ScaleTo50); break;
            case ScaleTo100:
                setScaling(ScaleTo75); break;
            case ScaleTo125:
                setScaling(ScaleTo100); break;
            case ScaleTo150:
                setScaling(ScaleTo125); break;
            case ScaleTo200:
                setScaling(ScaleTo150); break;
            case ScaleTo400:
                setScaling(ScaleTo200); break;
            }
        }
    }
    else if(event->modifiers() == Qt::ShiftModifier)
    {
        if(event->delta() > 0)
        {
            switch(m_rotation)
            {
            case RotateBy0:
                setRotation(RotateBy270); break;
            case RotateBy90:
                setRotation(RotateBy0); break;
            case RotateBy180:
                setRotation(RotateBy90); break;
            case RotateBy270:
                setRotation(RotateBy180); break;
            }
        }
        else if(event->delta() < 0)
        {
            switch(m_rotation)
            {
            case RotateBy0:
                setRotation(RotateBy90); break;
            case RotateBy90:
                setRotation(RotateBy180); break;
            case RotateBy180:
                setRotation(RotateBy270); break;
            case RotateBy270:
                setRotation(RotateBy0); break;
            }
        }
    }
}

void DocumentView::slotVerticalScrollBarValueChanged(int value)
{
    if(m_pageLayout == OneColumn || m_pageLayout == TwoColumns)
    {
        QMap< qreal, PageItem* >::iterator iterator = --m_pagesByHeight.lowerBound(value);

        if(iterator != m_pagesByHeight.end())
        {
            if(m_currentPage != iterator.value()->m_index + 1)
            {
                m_currentPage = iterator.value()->m_index + 1;

                emit currentPageChanged(m_currentPage);
            }
        }
    }
}

void DocumentView::slotTabActionTriggered()
{
    if(!this->parent())
    {
        qCritical("!parent");
        return;
    }

    if(!this->parent()->parent())
    {
        qCritical("!parent");
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

void DocumentView::slotOutlineTreeWidgetItemClicked(QTreeWidgetItem *item, int column)
{
    this->setCurrentPage(item->data(column, Qt::UserRole).toInt(), item->data(column, Qt::UserRole+1).toReal());
}

void DocumentView::search(const QString &text, bool matchCase)
{
    QList<int> indices;

    for(int index = m_currentPage - 1; index < m_numberOfPages; index++)
    {
        indices.append(index);
    }

    for(int index = 0; index < m_currentPage - 1; index++)
    {
        indices.append(index);
    }

    foreach(int index, indices)
    {
        if(m_search.isCanceled())
        {
            emit searchCanceled();

            return;
        }

        m_documentMutex.lock();

        Poppler::Page *page = m_document->page(index);

        QList<QRectF> rects;

        double rectLeft = 0.0, rectTop = 0.0, rectRight = 0.0, rectBottom = 0.0;

        while(page->search(text, rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::NextResult, matchCase ? Poppler::Page::CaseSensitive : Poppler::Page::CaseInsensitive))
        {
            QRectF rect;
            rect.setLeft(rectLeft);
            rect.setTop(rectTop);
            rect.setRight(rectRight);
            rect.setBottom(rectBottom);

            rects.append(rect.normalized());
        }

        delete page;

        m_documentMutex.unlock();

        // TODO: update results

        emit searchProgressed((100 * (index+1)) / m_numberOfPages);
    }

    emit searchFinished();
}

void DocumentView::print(QPrinter *printer, int fromPage, int toPage)
{
    QPainter *painter = new QPainter(printer);

    for(int index = fromPage-1; index <= toPage-1; index++)
    {
        if(m_print.isCanceled())
        {
            emit printCanceled();

            delete painter;
            delete printer;
            return;
        }

        m_documentMutex.lock();

        Poppler::Page *page = m_document->page(index);

        qreal fitToWidth = static_cast<qreal>(printer->width()) / (printer->physicalDpiX() * page->pageSizeF().width() / 72.0);
        qreal fitToHeight = static_cast<qreal>(printer->height()) / (printer->physicalDpiY() * page->pageSizeF().height() / 72.0);
        qreal fit = qMin(fitToWidth, fitToHeight);

        QImage image = page->renderToImage(printer->physicalDpiX(), printer->physicalDpiY());

        delete page;

        m_documentMutex.unlock();

        painter->setTransform(QTransform::fromScale(fit, fit));
        painter->drawImage(0.0, 0.0, image);

        if(index < toPage-1)
        {
            printer->newPage();
        }

        emit printProgressed((100 * (index+1 - fromPage + 1)) / (toPage - fromPage + 1));
    }

    emit printFinished();

    delete painter;
    delete printer;
}

void DocumentView::preparePages()
{
    m_scene->clear();
    m_pagesByIndex.clear();

    for(int index = 0; index < m_numberOfPages; index++)
    {
        PageItem* pageItem = new PageItem();

        pageItem->m_page = m_document->page(index);

        pageItem->m_index = index;

        pageItem->m_highlightAll = m_highlightAll;

        foreach(Poppler::Link *link, pageItem->m_page->links())
        {
            if(link->linkType() == Poppler::Link::Goto)
            {
                if(!static_cast<Poppler::LinkGoto*>(link)->isExternal())
                {
                    pageItem->m_links.append(Link(link->linkArea().normalized(),
                                                  static_cast<Poppler::LinkGoto*>(link)->destination().pageNumber(),
                                                  static_cast<Poppler::LinkGoto*>(link)->destination().top()));
                }
            }
            else if(link->linkType() == Poppler::Link::Browse)
            {
                pageItem->m_links.append(Link(link->linkArea().normalized(),
                                              static_cast<Poppler::LinkBrowse*>(link)->url()));
            }

            delete link;
        }

        m_scene->addItem(pageItem);
        m_pagesByIndex.insert(index, pageItem);
    }
}

void DocumentView::prepareOutline()
{
    m_outlineTreeWidget->clear();

    QDomDocument *toc = m_document->toc();

    if(toc)
    {
        prepareOutline(toc->firstChild(), 0, 0);

        delete toc;
    }
}

void DocumentView::prepareOutline(const QDomNode &node, QTreeWidgetItem *parent, QTreeWidgetItem *sibling)
{
    QDomElement element = node.toElement();
    QTreeWidgetItem *item = 0;

    if(parent)
    {
        item = new QTreeWidgetItem(parent);
    }
    else
    {
        item = new QTreeWidgetItem(m_outlineTreeWidget, sibling);
    }

    item->setText(0, element.tagName());

    if(element.hasAttribute("Destination"))
    {
        Poppler::LinkDestination linkDestination(element.attribute("Destination"));

        item->setData(0, Qt::UserRole, linkDestination.pageNumber());
        item->setData(0, Qt::UserRole+1, linkDestination.top());
    }
    else if(element.hasAttribute("DestinationName"))
    {
        Poppler::LinkDestination *linkDestination = m_document->linkDestination(element.attribute("DestinationName"));

        item->setData(0, Qt::UserRole, linkDestination ? linkDestination->pageNumber() : 1);
        item->setData(0, Qt::UserRole+1, linkDestination ? linkDestination->top() : 0.0);

        delete linkDestination;
    }

    QDomNode siblingNode = node.nextSibling();
    if(!siblingNode.isNull())
    {
        prepareOutline(siblingNode, parent, item);
    }

    QDomNode childNode = node.firstChild();
    if(!childNode.isNull())
    {
        prepareOutline(childNode, item, 0);
    }
}

void DocumentView::prepareThumbnails()
{
    m_thumbnailsGraphicsView->scene()->clear();
    m_thumbnailsGraphicsView->scene()->setBackgroundBrush(QBrush(Qt::darkGray));

    qreal width = 10.0, height = 5.0;

    for(int index = 0; index < m_numberOfPages; index++)
    {
        ThumbnailItem* thumbnailItem = new ThumbnailItem();

        thumbnailItem->m_page = m_document->page(index);

        thumbnailItem->m_index = index;

        thumbnailItem->setPos(5.0, height);

        QRectF rect = thumbnailItem->boundingRect();

        width = qMax(width, rect.width() + 10.0);
        height += rect.height() + 5.0;

        m_thumbnailsGraphicsView->scene()->addItem(thumbnailItem);
    }

    m_thumbnailsGraphicsView->scene()->setSceneRect(0.0, 0.0, width, height);
    m_thumbnailsGraphicsView->setSceneRect(0.0, 0.0, width, height);
}

void DocumentView::prepareScene()
{
    // calculate scale

    qreal scale = 4.0;

    if(m_scaling == FitToPage || m_scaling == FitToPageWidth)
    {
        QSizeF size;
        qreal width = 0.0, height = 0.0;

        switch(m_pageLayout)
        {
        case OnePage:
        case OneColumn:
            for(int index = 0; index < m_numberOfPages; index++)
            {
                size = m_pagesByIndex.value(index)->m_page->pageSizeF();

                switch(m_rotation)
                {
                case RotateBy0:
                case RotateBy180:
                    width = size.width();
                    height = size.height();

                    break;
                case RotateBy90:
                case RotateBy270:
                    width = size.height();
                    height = size.width();

                    break;
                }

                scale = qMin(scale, 0.95 * m_view->width() / (width + 20.0));
                if(m_scaling == FitToPage)
                {
                    scale = qMin(scale, 0.95 * m_view->height() / (height + 20.0));
                }
            }

            break;
        case TwoPages:
        case TwoColumns:
            for(int index = 0; index < (m_numberOfPages % 2 == 0 ? m_numberOfPages : m_numberOfPages - 1); index += 2)
            {
                size = m_pagesByIndex.value(index)->m_page->pageSizeF();

                switch(m_rotation)
                {
                case RotateBy0:
                case RotateBy180:
                    width = size.width();
                    height = size.height();

                    break;
                case RotateBy90:
                case RotateBy270:
                    width = size.height();
                    height = size.width();

                    break;
                }

                size = m_pagesByIndex.value(index + 1)->m_page->pageSizeF();

                switch(m_rotation)
                {
                case RotateBy0:
                case RotateBy180:
                    width += size.width();
                    height += size.height();

                    break;
                case RotateBy90:
                case RotateBy270:
                    width += size.height();
                    height += size.width();

                    break;
                }

                scale = qMin(scale, 0.95 * m_view->width() / (width + 30.0));
                if(m_scaling == FitToPage)
                {
                    scale = qMin(scale, 0.95 * m_view->height() / (height + 20.0));
                }
            }

            if(m_numberOfPages % 2 != 0)
            {
                size = m_pagesByIndex.value(m_numberOfPages - 1)->m_page->pageSizeF();

                switch(m_rotation)
                {
                case RotateBy0:
                case RotateBy180:
                    width = size.width();
                    height = size.height();

                    break;
                case RotateBy90:
                case RotateBy270:
                    width = size.height();
                    height = size.width();

                    break;
                }

                scale = qMin(scale, 0.95 * m_view->width() / (width + 20.0));
                if(m_scaling == FitToPage)
                {
                    scale = qMin(scale, 0.95 * m_view->height() / (height + 20.0));
                }
            }

            break;
        }
    }

    for(int index = 0; index < m_numberOfPages; index++)
    {
        PageItem* pageItem = m_pagesByIndex.value(index);

        pageItem->prepareGeometryChange();

        switch(m_scaling)
        {
        case FitToPage:
            pageItem->m_scale = scale; break;
        case FitToPageWidth:
            pageItem->m_scale = scale; break;
        case ScaleTo50:
            pageItem->m_scale = 0.5; break;
        case ScaleTo75:
            pageItem->m_scale = 0.75; break;
        case ScaleTo100:
            pageItem->m_scale = 1.0; break;
        case ScaleTo125:
            pageItem->m_scale = 1.25; break;
        case ScaleTo150:
            pageItem->m_scale = 1.5; break;
        case ScaleTo200:
            pageItem->m_scale = 2.0; break;
        case ScaleTo400:
            pageItem->m_scale = 4.0; break;
        }

        pageItem->m_rotation = m_rotation;

        pageItem->prepareTransforms();
    }

    // calculate layout

    m_pagesByHeight.clear();

    qreal width = 20.0, height = 10.0;

    switch(m_pageLayout)
    {
    case OnePage:
    case OneColumn:
        for(int index = 0; index < m_numberOfPages; index++)
        {
            PageItem* pageItem = m_pagesByIndex.value(index);
            QRectF rect = pageItem->boundingRect();

            pageItem->setPos(10.0, height);
            m_pagesByHeight.insert(height - 0.3 * rect.height(), pageItem);

            width = qMax(width, rect.width() + 20.0);
            height += rect.height() + 10.0;
        }

        break;
    case TwoPages:
    case TwoColumns:
        for(int index = 0; index < (m_numberOfPages % 2 == 0 ? m_numberOfPages : m_numberOfPages - 1); index += 2)
        {
            PageItem* leftPageItem = m_pagesByIndex.value(index);
            PageItem* rightPageItem = m_pagesByIndex.value(index + 1);
            QRectF leftRect = leftPageItem->boundingRect();
            QRectF rightRect = rightPageItem->boundingRect();

            leftPageItem->setPos(10.0, height);
            rightPageItem->setPos(20.0 + leftRect.width(), height);
            m_pagesByHeight.insert(height - 0.3 * leftRect.height(), leftPageItem);

            width = qMax(width, leftRect.width() + rightRect.width() + 30.0);
            height += qMax(leftRect.height(), rightRect.height()) + 10.0;
        }

        if(m_numberOfPages % 2 != 0)
        {
            PageItem* leftPageItem = m_pagesByIndex.value(m_numberOfPages - 1);
            QRectF leftRect = leftPageItem->boundingRect();

            leftPageItem->setPos(10.0, height);
            m_pagesByHeight.insert(height - 0.3 * leftRect.height(), leftPageItem);

            width = qMax(width, leftRect.width() + 20.0);
            height += leftRect.height() + 10.0;
        }

        break;
    }

    m_scene->setSceneRect(0.0, 0.0, width, height);
    m_view->setSceneRect(0.0, 0.0, width, height);
}

void DocumentView::prepareView(qreal top)
{
    PageItem* leftPageItem = m_pagesByIndex.value(m_currentPage - 1, 0);
    PageItem* rightPageItem = m_pagesByIndex.value(m_currentPage, 0);

    switch(m_pageLayout)
    {
    case OnePage:
        foreach(PageItem* pageItem, m_pagesByIndex.values())
        {
            pageItem->setVisible(false);
        }

        if(leftPageItem)
        {
            leftPageItem->setVisible(true);

            m_view->setSceneRect(leftPageItem->boundingRect().translated(leftPageItem->pos()).adjusted(-10.0, -10.0, 10.0, 10.0));

            m_view->horizontalScrollBar()->setValue(qFloor(leftPageItem->x()));
            m_view->verticalScrollBar()->setValue(qFloor(leftPageItem->y() + leftPageItem->boundingRect().height() * top));
        }

        break;
    case TwoPages:
        foreach(PageItem* pageItem, m_pagesByIndex.values())
        {
            pageItem->setVisible(false);
        }

        if(leftPageItem != 0 && rightPageItem != 0)
        {
            leftPageItem->setVisible(true);
            rightPageItem->setVisible(true);

            m_view->setSceneRect(leftPageItem->boundingRect().translated(leftPageItem->pos()).united(rightPageItem->boundingRect().translated(rightPageItem->pos())).adjusted(-10.0, -10.0, 10.0, 10.0));

            m_view->horizontalScrollBar()->setValue(qFloor(leftPageItem->x()));
            m_view->verticalScrollBar()->setValue(qFloor(leftPageItem->y() + leftPageItem->boundingRect().height() * top));
        }
        else if(leftPageItem != 0)
        {
            leftPageItem->setVisible(true);

            m_view->setSceneRect(leftPageItem->boundingRect().translated(leftPageItem->pos()).adjusted(-10.0, -10.0, 10.0, 10.0));

            m_view->horizontalScrollBar()->setValue(qFloor(leftPageItem->x()));
            m_view->verticalScrollBar()->setValue(qFloor(leftPageItem->y() + leftPageItem->boundingRect().height() * top));
        }

        break;
    case OneColumn:
    case TwoColumns:
        foreach(PageItem* pageItem, m_pagesByIndex.values())
        {
            pageItem->setVisible(true);
        }

        if(leftPageItem != 0)
        {
            m_view->horizontalScrollBar()->setValue(qFloor(leftPageItem->x()));
            m_view->verticalScrollBar()->setValue(qFloor(leftPageItem->y() + leftPageItem->boundingRect().height() * top));
        }

        break;
    }
}
