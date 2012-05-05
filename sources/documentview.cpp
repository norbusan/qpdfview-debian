#include "documentview.h"

DocumentView::PageItem::PageItem(QGraphicsItem* parent, QGraphicsScene* scene) : QGraphicsItem(parent, scene),
    m_page(0),
    m_index(-1),
    m_scale(1.0),
    m_links(),
    m_highlight(),
    m_rubberBand(),
    m_size(),
    m_resolutionX(72.0),
    m_resolutionY(72.0),
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
    return QRectF(0.0, 0.0, m_scale * m_resolutionX / 72.0 * m_size.width(), m_scale * m_resolutionY / 72.0 * m_size.height());
}

void DocumentView::PageItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    DocumentView* parent = qobject_cast<DocumentView*>(scene()->parent()); Q_ASSERT(parent);

    // page

    painter->fillRect(boundingRect(), QBrush(Qt::white));

#ifdef RENDER_IN_PAINT

    DocumentView::PageCacheKey key(m_index, m_scale);

    if(!parent->m_pageCache.contains(key))
    {
        render(false);
    }

    painter->drawImage(boundingRect(), parent->m_pageCache.value(key));

#else

    parent->m_pageCacheMutex.lock();

    DocumentView::PageCacheKey key(m_index, m_scale);

    if(parent->m_pageCache.contains(key))
    {
        painter->drawImage(boundingRect(), parent->m_pageCache.value(key));
    }
    else
    {
        if(!m_render.isRunning())
        {
            m_render = QtConcurrent::run(this, &DocumentView::PageItem::render, false);
        }
    }

    parent->m_pageCacheMutex.unlock();

#endif

    painter->setPen(QPen(Qt::black));
    painter->drawRect(boundingRect());

    // links

#ifdef PAINT_LINKS

    painter->save();

    painter->setTransform(m_linkTransform, true);
    painter->setPen(QPen(QColor(255, 0, 0, 127)));

    foreach(Link link, m_links)
    {
        painter->drawRect(link.area);
    }

    painter->restore();

#endif

    // highlights

    painter->save();

    painter->setTransform(m_highlightTransform, true);
    painter->setPen(QPen(QColor(0, 0, 255)));

    if(!m_highlight.isNull())
    {
        painter->drawRect(m_highlight);
        painter->fillRect(m_highlight, QBrush(QColor(0, 0, 255, 127)));
    }

    if(parent->m_highlightAll)
    {
        foreach(QRectF highlight, parent->m_results.values(m_index))
        {
            painter->fillRect(highlight, QBrush(QColor(0, 255, 0, 127)));
        }
    }

    painter->restore();

    // rubber band

    if(!m_rubberBand.isNull())
    {
        painter->setPen(QPen(Qt::DashLine));
        painter->drawRect(m_rubberBand);
    }
}

void DocumentView::PageItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
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

void DocumentView::PageItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    DocumentView* parent = qobject_cast<DocumentView*>(scene()->parent()); Q_ASSERT(parent);

    if(event->button() == Qt::LeftButton)
    {
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
                    if(parent->m_settings.value("documentView/externalLinks", false).toBool())
                    {
                        QDesktopServices::openUrl(QUrl(link.url));
                    }
                    else
                    {
                        QMessageBox::information(parent, tr("Information"), tr("External links are disabled in the settings."));
                    }
                }

                event->accept();

                return;
            }
        }

        if(event->modifiers() == Qt::ShiftModifier)
        {
            m_rubberBand = QRectF(event->pos(), QSizeF());

            event->accept();
        }
        else
        {
            event->ignore();
        }
    }
    else if(event->button() == Qt::MidButton)
    {
        m_highlight = QRectF();

        update(boundingRect());

        event->accept();
    }
    else
    {
        event->ignore();
    }
}

void DocumentView::PageItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if(!m_rubberBand.isNull())
    {
        m_rubberBand.setBottomRight(event->pos());

        update(boundingRect());

        event->accept();
    }
    else
    {
        event->ignore();
    }
}

void DocumentView::PageItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    DocumentView* parent = qobject_cast<DocumentView*>(scene()->parent()); Q_ASSERT(parent);

    if(!m_rubberBand.isNull())
    {
        m_rubberBand.setBottomRight(event->pos());

        m_highlight = m_highlightTransform.inverted().mapRect(m_rubberBand.normalized()).adjusted(-1.0, -1.0, 1.0, 1.0);

        // copy text or image

        QMenu menu(parent);
        QAction *copyTextAction = menu.addAction(tr("Copy &text"));
        QAction *copyImageAction = menu.addAction(tr("Copy &image"));
        QAction *copyAction = menu.exec(event->screenPos());

        if(copyAction == copyTextAction)
        {
            parent->m_documentMutex.lock();

            QString text = m_page->text(m_highlight);

            parent->m_documentMutex.unlock();

            if(!text.isEmpty())
            {
                QApplication::clipboard()->setText(text);
            }
        }
        else if(copyAction == copyImageAction)
        {
            parent->m_documentMutex.lock();

            QImage image = m_page->renderToImage(m_scale * m_resolutionX, m_scale * m_resolutionY, m_rubberBand.x(), m_rubberBand.y(), m_rubberBand.width(), m_rubberBand.height());

            parent->m_documentMutex.unlock();

            if(!image.isNull())
            {
                QApplication::clipboard()->setImage(image);
            }
        }

        m_rubberBand = QRectF();

        update(boundingRect());

        event->accept();
    }
    else
    {
        event->ignore();
    }
}

void DocumentView::PageItem::render(bool prefetch)
{
    DocumentView* parent = qobject_cast<DocumentView*>(scene()->parent()); Q_ASSERT(parent);

    QRectF rect = parent->m_pageTransform.mapRect(boundingRect()).translated(pos());
    QRectF visibleRect = parent->m_view->mapToScene(parent->m_view->viewport()->rect()).boundingRect();

    if(!rect.intersects(visibleRect) && !prefetch)
    {
        return;
    }

#ifdef RENDER_FROM_DISK

    Poppler::Document* document = Poppler::Document::load(parent->m_filePath);

    if(!document)
    {
        qFatal("!document");
        return;
    }

    Poppler::Page* page = document->page(m_index);

    if(!page)
    {
        qFatal("!page");
        return;
    }

    Poppler::Document::RenderHints renderHints = parent->m_document->renderHints();

    document->setRenderHint(Poppler::Document::Antialiasing, renderHints.testFlag(Poppler::Document::Antialiasing));
    document->setRenderHint(Poppler::Document::TextAntialiasing, renderHints.testFlag(Poppler::Document::TextAntialiasing));
    document->setRenderHint(Poppler::Document::TextHinting, renderHints.testFlag(Poppler::Document::TextHinting));

    QImage image = page->renderToImage(m_scale * m_resolutionX, m_scale * m_resolutionY);

    delete page;
    delete document;

#else

    parent->m_documentMutex.lock();

    QImage image = m_page->renderToImage(m_scale * m_resolutionX, m_scale * m_resolutionY);

    parent->m_documentMutex.unlock();

#endif

    parent->m_pageCacheMutex.lock();

    DocumentView::PageCacheKey key(m_index, m_scale);
    uint byteCount = image.byteCount();

    if(parent->m_maximumPageCacheSize < 3 * byteCount)
    {
        parent->m_maximumPageCacheSize = 3 * byteCount;

        qWarning() << tr("Maximum page cache size is too small. Increased it to %1 bytes to hold at least three pages.").arg(3 * byteCount);
    }

    while(parent->m_pageCacheSize + byteCount > parent->m_maximumPageCacheSize)
    {
        QMap< DocumentView::PageCacheKey, QImage >::iterator iterator = parent->m_pageCache.lowerBound(key) != parent->m_pageCache.end() ? --parent->m_pageCache.end() : parent->m_pageCache.begin();

        parent->m_pageCacheSize -= iterator.value().byteCount();
        parent->m_pageCache.remove(iterator.key());
    }

    parent->m_pageCacheSize += byteCount;
    parent->m_pageCache.insert(key, image);

    parent->m_pageCacheMutex.unlock();

    emit parent->pageItemChanged(this);
}

DocumentView::ThumbnailItem::ThumbnailItem(QGraphicsItem* parent, QGraphicsScene* scene) : QGraphicsItem(parent, scene),
    m_page(0),
    m_index(-1),
    m_size(),
    m_resolutionX(72.0),
    m_resolutionY(72.0),
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
    return QRectF(0.0, 0.0, 0.1 * m_resolutionX / 72.0 * m_size.width(), 0.1 * m_resolutionY / 72.0 * m_size.height());
}

void DocumentView::ThumbnailItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    DocumentView* parent = qobject_cast<DocumentView*>(scene()->parent()); Q_ASSERT(parent);

    // page

    painter->fillRect(boundingRect(), QBrush(Qt::white));

#ifdef RENDER_IN_PAINT

    DocumentView::PageCacheKey key(m_index, 0.1);

    if(!parent->m_pageCache.contains(key))
    {
        render();
    }

    painter->drawImage(boundingRect(), parent->m_pageCache.value(key));

#else

    parent->m_pageCacheMutex.lock();

    DocumentView::PageCacheKey key(m_index, 0.1);

    if(parent->m_pageCache.contains(key))
    {
        painter->drawImage(boundingRect(), parent->m_pageCache.value(key));
    }
    else
    {
        if(!m_render.isRunning())
        {
            m_render = QtConcurrent::run(this, &DocumentView::ThumbnailItem::render);
        }
    }

    parent->m_pageCacheMutex.unlock();

#endif

    painter->setPen(QPen(Qt::black));
    painter->drawRect(boundingRect());
}

void DocumentView::ThumbnailItem::mousePressEvent(QGraphicsSceneMouseEvent*)
{
    DocumentView* parent = qobject_cast<DocumentView*>(scene()->parent()); Q_ASSERT(parent);

    parent->setCurrentPage(m_index + 1);
}

void DocumentView::ThumbnailItem::render()
{
    DocumentView* parent = qobject_cast<DocumentView*>(scene()->parent()); Q_ASSERT(parent);

    QRectF rect = boundingRect().translated(pos());
    QRectF visibleRect = parent->m_thumbnailsGraphicsView->mapToScene(parent->m_thumbnailsGraphicsView->viewport()->rect()).boundingRect();

    if(!rect.intersects(visibleRect))
    {
        return;
    }

#ifdef RENDER_FROM_DISK

    Poppler::Document* document = Poppler::Document::load(parent->m_filePath);

    if(!document)
    {
        qFatal("!document");
        return;
    }

    Poppler::Page* page = document->page(m_index);

    if(!page)
    {
        qFatal("!page");
        return;
    }

    Poppler::Document::RenderHints renderHints = parent->m_document->renderHints();

    document->setRenderHint(Poppler::Document::Antialiasing, renderHints.testFlag(Poppler::Document::Antialiasing));
    document->setRenderHint(Poppler::Document::TextAntialiasing, renderHints.testFlag(Poppler::Document::TextAntialiasing));
    document->setRenderHint(Poppler::Document::TextHinting, renderHints.testFlag(Poppler::Document::TextHinting));

    QImage image = page->renderToImage(0.1 * m_resolutionX, 0.1 * m_resolutionY);

    delete page;
    delete document;

#else

    parent->m_documentMutex.lock();

    QImage image = m_page->renderToImage(0.1 * m_resolutionX, 0.1 * m_resolutionY);

    parent->m_documentMutex.unlock();

#endif

    parent->m_pageCacheMutex.lock();

    DocumentView::PageCacheKey key(m_index, 0.1);
    uint byteCount = image.byteCount();

    if(parent->m_maximumPageCacheSize < 3 * byteCount)
    {
        parent->m_maximumPageCacheSize = 3 * byteCount;

        qWarning() << tr("Maximum page cache size is too small. Increased it to %1 bytes to hold at least three pages.").arg(3 * byteCount);
    }

    while(parent->m_pageCacheSize + byteCount > parent->m_maximumPageCacheSize)
    {
        QMap< DocumentView::PageCacheKey, QImage >::iterator iterator = parent->m_pageCache.lowerBound(key) != parent->m_pageCache.end() ? --parent->m_pageCache.end() : parent->m_pageCache.begin();

        parent->m_pageCacheSize -= iterator.value().byteCount();
        parent->m_pageCache.remove(iterator.key());
    }

    parent->m_pageCacheSize += byteCount;
    parent->m_pageCache.insert(key, image);

    parent->m_pageCacheMutex.unlock();

    emit parent->thumbnailItemChanged(this);
}

DocumentView::DocumentView(QWidget* parent) : QWidget(parent),
    m_document(0),
    m_documentMutex(),
    m_pageCache(),
    m_pageCacheMutex(),
    m_pageCacheSize(0u),
    m_maximumPageCacheSize(67108864u),
    m_filePath(),
    m_numberOfPages(-1),
    m_currentPage(-1),
    m_pageLayout(OnePage),
    m_scaling(ScaleTo100),
    m_rotation(RotateBy0),
    m_highlightAll(false),
    m_pagesByIndex(),
    m_pagesByHeight(),
    m_pageTransform(),
    m_autoRefreshWatcher(0),
    m_results(),
    m_resultsMutex(),
    m_currentResult(m_results.end()),
    m_search(),
    m_print()
{
    connect(this, SIGNAL(pageItemChanged(PageItem*)), SLOT(slotUpdatePageItem(PageItem*)));
    connect(this, SIGNAL(thumbnailItemChanged(ThumbnailItem*)), SLOT(slotUpdateThumbnailItem(ThumbnailItem*)));

    connect(this, SIGNAL(firstResultFound()), SLOT(findNext()));

    // settings

    m_pageLayout = static_cast<PageLayout>(m_settings.value("documentView/pageLayout", 0).toUInt());
    m_scaling = static_cast<Scaling>(m_settings.value("documentView/scaling", 4).toUInt());
    m_rotation = static_cast<Rotation>(m_settings.value("documentView/rotation", 0).toUInt());
    m_highlightAll = m_settings.value("documentView/highlightAll", false).toBool();

    // graphics

    m_scene = new QGraphicsScene(this);
    m_scene->setBackgroundBrush(QBrush(Qt::darkGray));

    m_view = new QGraphicsView(m_scene, this);
    m_view->setDragMode(QGraphicsView::ScrollHandDrag);
    m_view->show();

    m_highlight = new QGraphicsRectItem();
    m_highlight->setPen(QPen(QColor(0, 255, 0, 255)));
    m_highlight->setBrush(QBrush(QColor(0, 255, 0, 127)));

    m_scene->addItem(m_highlight);

    m_highlight->setVisible(false);

    // verticalScrollBar

    m_view->verticalScrollBar()->installEventFilter(this);

    connect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), SLOT(slotVerticalScrollBarValueChanged(int)));

    // prefetchTimer

    m_prefetchTimer = new QTimer(this);
    m_prefetchTimer->setInterval(500);
    m_prefetchTimer->setSingleShot(true);

    connect(this, SIGNAL(currentPageChanged(int)), m_prefetchTimer, SLOT(start()));
    connect(this, SIGNAL(pageLayoutChanged(DocumentView::PageLayout)), m_prefetchTimer, SLOT(start()));
    connect(this, SIGNAL(scalingChanged(DocumentView::Scaling)), m_prefetchTimer, SLOT(start()));

    connect(m_prefetchTimer, SIGNAL(timeout()), SLOT(slotPrefetchTimerTimeout()));

    // bookmarksMenu

    m_bookmarksMenu = new BookmarksMenu(this);
    m_bookmarksMenu->updateCurrentPage(m_currentPage);
    m_bookmarksMenu->updateValue(m_view->verticalScrollBar()->value());
    m_bookmarksMenu->updateRange(m_view->verticalScrollBar()->minimum(), m_view->verticalScrollBar()->maximum());

    connect(this, SIGNAL(currentPageChanged(int)), m_bookmarksMenu, SLOT(updateCurrentPage(int)));
    connect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), m_bookmarksMenu, SLOT(updateValue(int)));
    connect(m_view->verticalScrollBar(), SIGNAL(rangeChanged(int,int)), m_bookmarksMenu, SLOT(updateRange(int,int)));

    connect(m_bookmarksMenu, SIGNAL(entrySelected(int,int)), SLOT(slotBookmarksMenuEntrySelected(int,int)));

    // tabAction

    m_tabAction = new QAction(this);

    connect(m_tabAction, SIGNAL(triggered()), SLOT(slotTabActionTriggered()));

    // outlineTreeWidget

    m_outlineTreeWidget = new QTreeWidget();
    m_outlineTreeWidget->setAlternatingRowColors(true);
    m_outlineTreeWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_outlineTreeWidget->header()->setVisible(false);

    connect(m_outlineTreeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)), SLOT(slotOutlineTreeWidgetItemClicked(QTreeWidgetItem*,int)));

    // metaInformationTableWidget

    m_metaInformationTableWidget = new QTableWidget();
    m_metaInformationTableWidget->setAlternatingRowColors(true);
    m_metaInformationTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_metaInformationTableWidget->horizontalHeader()->setVisible(false);
    m_metaInformationTableWidget->verticalHeader()->setVisible(false);

    // thumbnailsGraphicsView

    m_thumbnailsGraphicsView = new QGraphicsView(new QGraphicsScene(this));
    m_thumbnailsGraphicsView->scene()->setBackgroundBrush(QBrush(Qt::darkGray));
}

DocumentView::~DocumentView()
{
    m_scene->clear();
    m_thumbnailsGraphicsView->scene()->clear();

    m_search.cancel();
    m_search.waitForFinished();

    m_print.cancel();
    m_print.waitForFinished();

    if(m_document)
    {
        delete m_document;
    }

    if(m_autoRefreshWatcher)
    {
        delete m_autoRefreshWatcher;
    }

    delete m_outlineTreeWidget;
    delete m_metaInformationTableWidget;
    delete m_thumbnailsGraphicsView;
}

const QString& DocumentView::filePath() const
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

        foreach(PageItem* pageItem, m_pagesByIndex.values())
        {
            pageItem->update(pageItem->boundingRect());
        }

        emit highlightAllChanged(m_highlightAll);
    }
}

QAction* DocumentView::tabAction() const
{
    return m_tabAction;
}

QTreeWidget* DocumentView::outlineTreeWidget() const
{
    return m_outlineTreeWidget;
}

QTableWidget* DocumentView::metaInformationTableWidget() const
{
    return m_metaInformationTableWidget;
}

QGraphicsView* DocumentView::thumbnailsGraphicsView() const
{
    return m_thumbnailsGraphicsView;
}

bool DocumentView::open(const QString& filePath)
{
    m_scene->clear();
    m_thumbnailsGraphicsView->scene()->clear();

    cancelSearch();
    cancelPrint();

    Poppler::Document* document = Poppler::Document::load(filePath);

    if(document)
    {
        if(m_document)
        {
            delete m_document;
        }

        if(m_autoRefreshWatcher)
        {
            delete m_autoRefreshWatcher;
        }

        m_document = document;

        m_document->setRenderHint(Poppler::Document::Antialiasing, m_settings.value("documentView/antialiasing", true).toBool());
        m_document->setRenderHint(Poppler::Document::TextAntialiasing, m_settings.value("documentView/textAntialiasing", true).toBool());
        m_document->setRenderHint(Poppler::Document::TextHinting, m_settings.value("documentView/textHinting", false).toBool());

        if(m_settings.value("documentView/autoRefresh", false).toBool())
        {
            m_autoRefreshWatcher = new QFileSystemWatcher(QStringList(filePath));

            connect(m_autoRefreshWatcher, SIGNAL(fileChanged(QString)), SLOT(refresh()));
        }

        m_filePath = filePath;
        m_numberOfPages = m_document->numPages();

        emit filePathChanged(m_filePath);
        emit numberOfPagesChanged(m_numberOfPages);

        m_currentPage = 1;

        emit currentPageChanged(m_currentPage);

        preparePages();

        m_tabAction->setText(QFileInfo(m_filePath).completeBaseName());

        prepareOutline();
        prepareMetaInformation();
        prepareThumbnails();
    }

    m_pageCache.clear();
    m_pageCacheSize = 0u;
    m_maximumPageCacheSize = m_settings.value("documentView/maximumPageCacheSize", 67108864u).toUInt();

    prepareScene();
    prepareView();

    m_prefetchTimer->start();

    return document != 0;
}

bool DocumentView::refresh()
{
    m_scene->clear();
    m_thumbnailsGraphicsView->scene()->clear();

    cancelSearch();
    cancelPrint();

    Poppler::Document* document = Poppler::Document::load(m_filePath);

    if(document)
    {
        if(m_document)
        {
            delete m_document;
        }

        m_document = document;

        m_document->setRenderHint(Poppler::Document::Antialiasing, m_settings.value("documentView/antialiasing", true).toBool());
        m_document->setRenderHint(Poppler::Document::TextAntialiasing, m_settings.value("documentView/textAntialiasing", true).toBool());
        m_document->setRenderHint(Poppler::Document::TextHinting, m_settings.value("documentView/textHinting", false).toBool());

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

        preparePages();

        m_tabAction->setText(QFileInfo(m_filePath).completeBaseName());

        prepareOutline();
        prepareMetaInformation();
        prepareThumbnails();
    }

    m_pageCache.clear();
    m_pageCacheSize = 0u;
    m_maximumPageCacheSize = m_settings.value("documentView/maximumPageCacheSize", 67108864u).toUInt();

    prepareScene();
    prepareView();

    m_prefetchTimer->start();

    return document != 0;
}

bool DocumentView::saveCopy(const QString& filePath)
{
    if(m_document)
    {
            Poppler::PDFConverter* converter = m_document->pdfConverter();

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

    cancelSearch();
    cancelPrint();

    if(m_document)
    {
        delete m_document;
    }

    if(m_autoRefreshWatcher)
    {
        delete m_autoRefreshWatcher;
    }

    m_document = 0;

    m_filePath = QString();
    m_numberOfPages = -1;
    m_currentPage = -1;

    emit filePathChanged(m_filePath);
    emit numberOfPagesChanged(m_numberOfPages);
    emit currentPageChanged(m_currentPage);

    preparePages();

    m_tabAction->setText(QString());

    prepareOutline();
    prepareMetaInformation();
    prepareThumbnails();

    m_pageCache.clear();
    m_pageCacheSize = 0u;
    m_maximumPageCacheSize = m_settings.value("documentView/maximumPageCacheSize", 67108864u).toUInt();

    prepareScene();
    prepareView();
}

void DocumentView::setCurrentPage(int currentPage, qreal top)
{
    if(currentPage >= 1 && currentPage <= m_numberOfPages && top >= 0.0 && top <= 1.0)
    {
        PageItem* pageItem = m_pagesByIndex.value(m_currentPage - 1, 0);

        if(pageItem != 0)
        {
            QRectF rect = m_pageTransform.mapRect(pageItem->boundingRect()).translated(pageItem->pos());

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
                else if(!qFuzzyCompare(1.0 + (qCeil(m_view->verticalScrollBar()->value() - rect.top()) / rect.height()), 1.0 + top))
                {
                    prepareView(top);
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
                else if(!qFuzzyCompare(1.0 + (qCeil(m_view->verticalScrollBar()->value() - rect.top()) / rect.height()), 1.0 + top))
                {
                    prepareView(top);
                }

                break;
            }
        }
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

            prepareView();

            emit currentPageChanged(m_currentPage);
        }

        break;
    case TwoPages:
    case TwoColumns:
        if(m_currentPage > 1)
        {
            m_currentPage -= 2;

            prepareView();

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
        if(m_currentPage < m_numberOfPages)
        {
            m_currentPage += 1;

            prepareView();

            emit currentPageChanged(m_currentPage);
        }

        break;
    case TwoPages:
    case TwoColumns:
        if(m_currentPage < (m_numberOfPages % 2 != 0 ? m_numberOfPages : m_numberOfPages - 1))
        {
            m_currentPage += 2;

            prepareView();

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

        prepareView();

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

            prepareView();

            emit currentPageChanged(m_currentPage);
        }

        break;
    case TwoPages:
    case TwoColumns:
        if(m_currentPage != (m_numberOfPages % 2 != 0 ? m_numberOfPages : m_numberOfPages - 1))
        {
            m_currentPage = m_numberOfPages % 2 != 0 ? m_numberOfPages : m_numberOfPages - 1;

            prepareView();

            emit currentPageChanged(m_currentPage);
        }

        break;
    }
}


void DocumentView::startSearch(const QString& text, bool matchCase)
{
    cancelSearch();

    if(!text.isEmpty())
    {
        m_search = QtConcurrent::run(this, &DocumentView::search, text, matchCase);
    }
}

void DocumentView::cancelSearch()
{
    m_search.cancel();
    m_search.waitForFinished();

    m_resultsMutex.lock();

    m_results.clear();
    m_currentResult = m_results.end();

    m_resultsMutex.unlock();

    if(m_highlightAll)
    {
        foreach(PageItem* pageItem, m_pagesByIndex.values())
        {
            pageItem->update(pageItem->boundingRect());
        }
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
            if(m_currentResult.key() != m_currentPage - 1)
            {
                m_currentResult = --m_results.upperBound(m_currentPage - 1);
            }
            else
            {
                --m_currentResult;
            }

            if(m_currentResult == m_results.end())
            {
                m_currentResult = --m_results.upperBound(m_numberOfPages - 1);
            }

            break;
        case TwoPages:
        case TwoColumns:
            if(m_currentResult.key() != m_currentPage - 1 && m_currentResult.key() != m_currentPage)
            {
                m_currentResult = --m_results.upperBound(m_currentPage - 1);
            }
            else
            {
                --m_currentResult;
            }

            if(m_currentResult == m_results.end())
            {
                m_currentResult = --m_results.upperBound(m_numberOfPages - 1);
            }

            break;
        }
    }
    else
    {
        m_currentResult = --m_results.upperBound(m_currentPage - 1);

        if(m_currentResult == m_results.end())
        {
            m_currentResult = --m_results.upperBound(m_numberOfPages - 1);
        }
    }

    if(m_currentResult != m_results.end())
    {
        switch(m_pageLayout)
        {
        case OnePage:
        case OneColumn:
            if(m_currentPage != m_currentResult.key() + 1)
            {
                m_currentPage = m_currentResult.key() + 1;

                emit currentPageChanged(m_currentPage);
            }

            break;
        case TwoPages:
        case TwoColumns:
            if(m_currentPage != (m_currentResult.key() % 2 == 0 ? m_currentResult.key() + 1 : m_currentResult.key()))
            {
                m_currentPage = m_currentResult.key() % 2 == 0 ? m_currentResult.key() + 1 : m_currentResult.key();

                emit currentPageChanged(m_currentPage);
            }
        }

        prepareView();

        disconnect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(slotVerticalScrollBarValueChanged(int)));

        m_view->centerOn(m_highlight);

        connect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(slotVerticalScrollBarValueChanged(int)));
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
            if(m_currentResult.key() != m_currentPage - 1)
            {
                m_currentResult = --m_results.upperBound(m_currentPage - 1);
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
            if(m_currentResult.key() != m_currentPage - 1 && m_currentResult.key() != m_currentPage)
            {
                m_currentResult = --m_results.upperBound(m_currentPage - 1);
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
        m_currentResult = m_results.lowerBound(m_currentPage - 1);

        if(m_currentResult == m_results.end())
        {
            m_currentResult = m_results.lowerBound(0);
        }
    }

    if(m_currentResult != m_results.end())
    {
        switch(m_pageLayout)
        {
        case OnePage:
        case OneColumn:
            if(m_currentPage != m_currentResult.key() + 1)
            {
                m_currentPage = m_currentResult.key() + 1;

                emit currentPageChanged(m_currentPage);
            }

            break;
        case TwoPages:
        case TwoColumns:
            if(m_currentPage != (m_currentResult.key() % 2 == 0 ? m_currentResult.key() + 1 : m_currentResult.key()))
            {
                m_currentPage = m_currentResult.key() % 2 == 0 ? m_currentResult.key() + 1 : m_currentResult.key();

                emit currentPageChanged(m_currentPage);
            }
        }

        prepareView();

        disconnect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(slotVerticalScrollBarValueChanged(int)));

        m_view->centerOn(m_highlight);

        connect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(slotVerticalScrollBarValueChanged(int)));
    }
}

void DocumentView::startPrint(QPrinter* printer, int fromPage, int toPage)
{
    cancelPrint();

    if(fromPage >= 1 && fromPage <= m_numberOfPages && toPage >= 1 && toPage <= m_numberOfPages && fromPage <= toPage)
    {
        m_print = QtConcurrent::run(this, &DocumentView::print, printer, fromPage, toPage);
    }
}

void DocumentView::cancelPrint()
{
    m_print.cancel();
    m_print.waitForFinished();
}

bool DocumentView::eventFilter(QObject*, QEvent* event)
{
    if(event->type() == QEvent::Wheel)
    {
        QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);

        return wheelEvent->modifiers() == Qt::ControlModifier || wheelEvent->modifiers() == Qt::ShiftModifier || wheelEvent->modifiers() == Qt::AltModifier;
    }

    return false;
}

void DocumentView::showEvent(QShowEvent* event)
{
    if(!event->spontaneous())
    {
        prepareScene();
        prepareView();
    }
}

void DocumentView::resizeEvent(QResizeEvent* event)
{
    m_view->resize(event->size());

    if(m_scaling == FitToPage || m_scaling == FitToPageWidth)
    {
        prepareScene();
        prepareView();
    }
}

void DocumentView::contextMenuEvent(QContextMenuEvent* event)
{
    m_bookmarksMenu->exec(event->globalPos());
}

void DocumentView::keyPressEvent(QKeyEvent* event)
{
    QKeySequence shortcut(event->modifiers() + event->key());

    foreach(QAction* action, m_bookmarksMenu->actions())
    {
        if(action->shortcut() == shortcut)
        {
            action->trigger();
        }
    }
}

void DocumentView::wheelEvent(QWheelEvent* event)
{
    if(event->modifiers() == Qt::NoModifier)
    {
        int lastPage = -1;

        switch(m_pageLayout)
        {
            case OnePage:
            case OneColumn:
                lastPage = m_numberOfPages; break;
            case TwoPages:
            case TwoColumns:
                lastPage = m_numberOfPages % 2 != 0 ? m_numberOfPages : m_numberOfPages - 1; break;
        }

        switch(m_pageLayout)
        {
        case OnePage:
        case TwoPages:
            if(event->delta() > 0 && m_view->verticalScrollBar()->value() == m_view->verticalScrollBar()->minimum() && m_currentPage > 1)
            {
                previousPage();

                m_view->verticalScrollBar()->setValue(m_view->verticalScrollBar()->maximum());
            }
            else if(event->delta() < 0 && m_view->verticalScrollBar()->value() == m_view->verticalScrollBar()->maximum() && m_currentPage < lastPage)
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
    else if(event->modifiers() == Qt::AltModifier)
    {
        QWheelEvent wheelEvent(*event);
        wheelEvent.setModifiers(Qt::NoModifier);

        QApplication::sendEvent(m_view->horizontalScrollBar(), &wheelEvent);
    }
}

void DocumentView::slotUpdatePageItem(PageItem* pageItem)
{
    if(pageItem != 0)
    {
        pageItem->update(pageItem->boundingRect());
    }
}

void DocumentView::slotUpdateThumbnailItem(ThumbnailItem* thumbnailItem)
{
    if(thumbnailItem != 0)
    {
        thumbnailItem->update(thumbnailItem->boundingRect());
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

void DocumentView::slotPrefetchTimerTimeout()
{
#ifndef RENDER_IN_PAINT

    int fromPage = -1, toPage = -1;

    switch(m_pageLayout)
    {
    case OnePage:
    case OneColumn:
        fromPage = m_currentPage - 1;
        toPage = m_currentPage + 2;

        break;
    case TwoPages:
    case TwoColumns:
        fromPage = m_currentPage - 2;
        toPage = m_currentPage + 4;

        break;
    }

    fromPage = fromPage >= 1 ? fromPage : 1;
    toPage = toPage <= m_numberOfPages ? toPage : m_numberOfPages;

    for(int page = fromPage; page <= toPage; page++)
    {
        PageItem* pageItem = m_pagesByIndex.value(page - 1, 0);

        if(pageItem != 0)
        {
            m_pageCacheMutex.lock();

            PageCacheKey key(pageItem->m_index, pageItem->m_scale);

            if(!m_pageCache.contains(key))
            {
                if(!pageItem->m_render.isRunning())
                {
                    pageItem->m_render = QtConcurrent::run(pageItem, &DocumentView::PageItem::render, true);
                }
            }

            m_pageCacheMutex.unlock();
        }
    }

#endif
}

void DocumentView::slotBookmarksMenuEntrySelected(int page, int value)
{
    setCurrentPage(page);

    m_view->verticalScrollBar()->setValue(value);
}

void DocumentView::slotTabActionTriggered()
{
    TabWidget* tabWidget = qobject_cast<TabWidget*>(this->parent()->parent()); Q_ASSERT(tabWidget);

    tabWidget->setCurrentIndex(tabWidget->indexOf(this));
}

void DocumentView::slotOutlineTreeWidgetItemClicked(QTreeWidgetItem* item, int column)
{
    setCurrentPage(item->data(column, Qt::UserRole).toInt(), item->data(column, Qt::UserRole+1).toReal());
}

void DocumentView::search(const QString& text, bool matchCase)
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

    bool firstResult = true;

    foreach(int index, indices)
    {
        if(m_search.isCanceled())
        {
            emit searchCanceled();

            return;
        }

        m_documentMutex.lock();

        Poppler::Page* page = m_document->page(index);

        QList<QRectF> results;

#ifdef HAS_POPPLER_14
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
#else
        QRectF rect;

        while(page->search(text, rect, Poppler::Page::NextResult, matchCase ? Poppler::Page::CaseSensitive : Poppler::Page::CaseInsensitive))
        {
            results.append(rect.normalized());
        }
#endif

        delete page;

        m_documentMutex.unlock();

        m_resultsMutex.lock();

        while(!results.isEmpty())
        {
            m_results.insertMulti(index, results.takeLast());
        }

        m_resultsMutex.unlock();

        if(m_results.contains(index) && firstResult)
        {
            emit firstResultFound();

            firstResult = false;
        }

        if(m_highlightAll)
        {
            emit pageItemChanged(m_pagesByIndex.value(index, 0));
        }

        emit searchProgressed((100 * (indices.indexOf(index) + 1)) / indices.count());
    }

    emit searchFinished();
}

void DocumentView::print(QPrinter* printer, int fromPage, int toPage)
{
    QPainter* painter = new QPainter(printer);

    for(int index = fromPage - 1; index <= toPage - 1; index++)
    {
        if(m_print.isCanceled())
        {
            emit printCanceled();

            delete painter;
            delete printer;
            return;
        }

        m_documentMutex.lock();

        Poppler::Page* page = m_document->page(index);

        qreal fitToWidth = printer->width() / (printer->physicalDpiX() / 72.0 * page->pageSizeF().width());
        qreal fitToHeight = printer->height() / (printer->physicalDpiY() / 72.0 * page->pageSizeF().height() );
        qreal fit = qMin(fitToWidth, fitToHeight);

        QImage image = page->renderToImage(printer->physicalDpiX(), printer->physicalDpiY());

        delete page;

        m_documentMutex.unlock();

        painter->setTransform(QTransform::fromScale(fit, fit));
        painter->drawImage(0.0, 0.0, image);

        if(index < toPage - 1)
        {
            printer->newPage();
        }

        emit printProgressed((100 * (index + 1 - fromPage + 1)) / (toPage - fromPage + 1));
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

        pageItem->m_index = index;
        pageItem->m_page = m_document->page(pageItem->m_index);
        pageItem->m_size = pageItem->m_page->pageSizeF();

        foreach(Poppler::Link* link, pageItem->m_page->links())
        {
            if(link->linkType() == Poppler::Link::Goto)
            {
                if(!static_cast<Poppler::LinkGoto*>(link)->isExternal())
                {
                    QRectF area = link->linkArea().normalized();
                    int page = static_cast<Poppler::LinkGoto*>(link)->destination().pageNumber();
                    qreal top = static_cast<Poppler::LinkGoto*>(link)->destination().isChangeTop() ? static_cast<Poppler::LinkGoto*>(link)->destination().top() : 0.0;

                    page = page >= 1 ? page : 1;
                    page = page <= m_numberOfPages ? page : m_numberOfPages;

                    top = top >= 0.0 ? top : 0.0;
                    top = top <= 1.0 ? top : 1.0;

                    pageItem->m_links.append(Link(area, page, top));
                }
            }
            else if(link->linkType() == Poppler::Link::Browse)
            {
                QRectF area = link->linkArea().normalized();
                QString url = static_cast<Poppler::LinkBrowse*>(link)->url();

                pageItem->m_links.append(Link(area, url));
            }

            delete link;
        }

        m_scene->addItem(pageItem);
        m_pagesByIndex.insert(index, pageItem);
    }

    // highlight

    m_highlight = new QGraphicsRectItem();
    m_highlight->setPen(QPen(QColor(0, 255, 0, 255)));
    m_highlight->setBrush(QBrush(QColor(0, 255, 0, 127)));

    m_scene->addItem(m_highlight);
}

void DocumentView::prepareOutline()
{
    m_outlineTreeWidget->clear();

    QDomDocument* toc = m_document->toc();

    if(toc)
    {
        prepareOutline(toc->firstChild(), 0, 0);

        delete toc;
    }
}

void DocumentView::prepareOutline(const QDomNode& node, QTreeWidgetItem* parent, QTreeWidgetItem* sibling)
{
    QDomElement element = node.toElement();

    QTreeWidgetItem* item = 0;

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
        item->setData(0, Qt::UserRole+1, linkDestination.isChangeTop() ? linkDestination.top() : 0.0);
    }
    else if(element.hasAttribute("DestinationName"))
    {
        Poppler::LinkDestination *linkDestination = m_document->linkDestination(element.attribute("DestinationName"));

        item->setData(0, Qt::UserRole, linkDestination ? linkDestination->pageNumber() : 1);
        item->setData(0, Qt::UserRole+1, linkDestination ? (linkDestination->isChangeTop() ? linkDestination->top() : 0.0) : 0.0);

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

void DocumentView::prepareMetaInformation()
{
    m_metaInformationTableWidget->clear();

    QStringList keys = m_document->infoKeys();

    m_metaInformationTableWidget->setRowCount(keys.count());
    m_metaInformationTableWidget->setColumnCount(2);

    for(int index = 0; index < keys.count(); index++)
    {
        QString key = keys.at(index);
        QString value = m_document->info(key);

        if(value.startsWith("D:"))
        {
            value = QLocale::system().toString(m_document->date(key));
        }

        m_metaInformationTableWidget->setItem(index, 0, new QTableWidgetItem(key));
        m_metaInformationTableWidget->setItem(index, 1, new QTableWidgetItem(value));
    }
}

void DocumentView::prepareThumbnails()
{
    m_thumbnailsGraphicsView->scene()->clear();

    qreal sceneWidth = 0.0, sceneHeight = 5.0;

    for(int index = 0; index < m_numberOfPages; index++)
    {
        ThumbnailItem* thumbnailItem = new ThumbnailItem();

        thumbnailItem->m_index = index;
        thumbnailItem->m_page = m_document->page(thumbnailItem->m_index);
        thumbnailItem->m_size = thumbnailItem->m_page->pageSizeF();
        thumbnailItem->m_resolutionX = physicalDpiX();
        thumbnailItem->m_resolutionY = physicalDpiY();

        thumbnailItem->setPos(5.0, sceneHeight);

        m_thumbnailsGraphicsView->scene()->addItem(thumbnailItem);

        sceneWidth = qMax(sceneWidth, thumbnailItem->boundingRect().width() + 10.0);
        sceneHeight += thumbnailItem->boundingRect().height() + 5.0;

        QGraphicsSimpleTextItem* textItem = new QGraphicsSimpleTextItem(QLocale::system().toString(index + 1));

        textItem->setPos(10.0, sceneHeight);

        m_thumbnailsGraphicsView->scene()->addItem(textItem);

        sceneWidth = qMax(sceneWidth, textItem->boundingRect().width() + 15.0);
        sceneHeight += textItem->boundingRect().height() + 5.0;
    }

    m_thumbnailsGraphicsView->scene()->setSceneRect(0.0, 0.0, sceneWidth, sceneHeight);
    m_thumbnailsGraphicsView->setSceneRect(0.0, 0.0, sceneWidth, sceneHeight);

    m_thumbnailsGraphicsView->setMinimumWidth(sceneWidth + 35);
}

void DocumentView::prepareScene()
{
    // calculate scale

    if(m_scaling == FitToPage || m_scaling == FitToPageWidth)
    {
        qreal pageWidth = 0.0, pageHeight = 0.0;
        qreal visibleWidth = m_view->mapToScene(m_view->viewport()->rect()).boundingRect().width();
        qreal visibleHeight = m_view->mapToScene(m_view->viewport()->rect()).boundingRect().height();

        bool uniformFit = m_settings.value("documentView/uniformFit", false).toBool();

        qreal scale = 4.0;

        switch(m_pageLayout)
        {
        case OnePage:
        case OneColumn:
            for(int index = 0; index < m_numberOfPages; index++)
            {
                PageItem* pageItem = m_pagesByIndex.value(index);

                switch(m_rotation)
                {
                case RotateBy0:
                case RotateBy180:
                    pageWidth = physicalDpiX() / 72.0 * pageItem->m_size.width();
                    pageHeight = physicalDpiY() / 72.0 * pageItem->m_size.height();

                    break;
                case RotateBy90:
                case RotateBy270:
                    pageWidth = physicalDpiX() / 72.0 * pageItem->m_size.height();
                    pageHeight = physicalDpiY() / 72.0 * pageItem->m_size.width();

                    break;
                }

                if(uniformFit)
                {
                    scale = qMin(scale, (visibleWidth - 10.0) / pageWidth);
                    if(m_scaling == FitToPage)
                    {
                        scale = qMin(scale, (visibleHeight - 10.0) / pageHeight);
                    }
                }
                else
                {
                    scale = (visibleWidth - 10.0) / pageWidth;
                    if(m_scaling == FitToPage)
                    {
                        scale = qMin(scale, (visibleHeight - 10.0) / pageHeight);
                    }

                    pageItem->prepareGeometryChange();
                    pageItem->m_scale = scale;
                }
            }

            break;
        case TwoPages:
        case TwoColumns:
            for(int index = 0; index < (m_numberOfPages % 2 == 0 ? m_numberOfPages : m_numberOfPages - 1); index += 2)
            {
                PageItem* leftPageItem = m_pagesByIndex.value(index);

                switch(m_rotation)
                {
                case RotateBy0:
                case RotateBy180:
                    pageWidth = physicalDpiX() / 72.0 * leftPageItem->m_size.width();
                    pageHeight = physicalDpiY() / 72.0 * leftPageItem->m_size.height();

                    break;
                case RotateBy90:
                case RotateBy270:
                    pageWidth = physicalDpiX() / 72.0 * leftPageItem->m_size.height();
                    pageHeight = physicalDpiY() / 72.0 * leftPageItem->m_size.width();

                    break;
                }

                PageItem* rightPageItem = m_pagesByIndex.value(index + 1);

                switch(m_rotation)
                {
                case RotateBy0:
                case RotateBy180:
                    pageWidth += physicalDpiX() / 72.0 * rightPageItem->m_size.width();
                    pageHeight = qMax(pageHeight, physicalDpiY() / 72.0 * rightPageItem->m_size.height());

                    break;
                case RotateBy90:
                case RotateBy270:
                    pageWidth += physicalDpiX() / 72.0 * rightPageItem->m_size.height();
                    pageHeight = qMax(pageHeight, physicalDpiY() / 72.0 * rightPageItem->m_size.width());

                    break;
                }

                if(uniformFit)
                {
                    scale = qMin(scale, (m_view->viewport()->width() - 15.0) / pageWidth);
                    if(m_scaling == FitToPage)
                    {
                        scale = qMin(scale, (m_view->viewport()->height() - 10.0) / pageHeight);
                    }
                }
                else
                {
                    scale = (m_view->viewport()->width() - 15.0) / pageWidth;
                    if(m_scaling == FitToPage)
                    {
                        scale = qMin(scale, (m_view->viewport()->height() - 10.0) / pageHeight);
                    }

                    leftPageItem->prepareGeometryChange();
                    leftPageItem->m_scale = scale;

                    rightPageItem->prepareGeometryChange();
                    rightPageItem->m_scale = scale;
                }
            }

            if(m_numberOfPages % 2 != 0)
            {
                PageItem* leftPageItem = m_pagesByIndex.value(m_numberOfPages - 1);

                switch(m_rotation)
                {
                case RotateBy0:
                case RotateBy180:
                    pageWidth = physicalDpiX() / 72.0 * leftPageItem->m_size.width();
                    pageHeight = physicalDpiY() / 72.0 * leftPageItem->m_size.height();

                    break;
                case RotateBy90:
                case RotateBy270:
                    pageWidth = physicalDpiX() / 72.0 * leftPageItem->m_size.height();
                    pageHeight = physicalDpiY() / 72.0 * leftPageItem->m_size.width();

                    break;
                }

                if(uniformFit)
                {
                    scale = qMin(scale, (m_view->viewport()->width() - 10.0) / pageWidth);
                    if(m_scaling == FitToPage)
                    {
                        scale = qMin(scale, (m_view->viewport()->height() - 10.0) / pageHeight);
                    }
                }
                else
                {
                    scale = (m_view->viewport()->width() - 10.0) / pageWidth;
                    if(m_scaling == FitToPage)
                    {
                        scale = qMin(scale, (m_view->viewport()->height() - 10.0) / pageHeight);
                    }

                    leftPageItem->prepareGeometryChange();
                    leftPageItem->m_scale = scale;
                }
            }

            break;
        }

        if(uniformFit)
        {
            for(int index = 0; index < m_numberOfPages; index++)
            {
                PageItem* pageItem = m_pagesByIndex.value(index);

                pageItem->prepareGeometryChange();
                pageItem->m_scale = scale;
            }
        }
    }
    else
    {
        for(int index = 0; index < m_numberOfPages; index++)
        {
            PageItem* pageItem = m_pagesByIndex.value(index);

            pageItem->prepareGeometryChange();

            switch(m_scaling)
            {
            case FitToPage: break;
            case FitToPageWidth: break;
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
        }
    }

    // calculate transformations

    for(int index = 0; index < m_numberOfPages; index++)
    {
        PageItem* pageItem = m_pagesByIndex.value(index);

        pageItem->prepareGeometryChange();

        switch(m_rotation)
        {
        case RotateBy0:
            pageItem->setRotation(0.0); break;
        case RotateBy90:
            pageItem->setRotation(90.0); break;
        case RotateBy180:
            pageItem->setRotation(180); break;
        case RotateBy270:
            pageItem->setRotation(270.0); break;
        }

        switch(m_rotation)
        {
        case RotateBy0:
        case RotateBy180:
            pageItem->m_resolutionX = physicalDpiX();
            pageItem->m_resolutionY = physicalDpiY();

            break;
        case RotateBy90:
        case RotateBy270:
            pageItem->m_resolutionX = physicalDpiY();
            pageItem->m_resolutionY = physicalDpiX();

            break;
        }

        pageItem->m_linkTransform.reset();
        pageItem->m_linkTransform.scale(pageItem->m_scale * pageItem->m_resolutionX / 72.0 * pageItem->m_size.width(), pageItem->m_scale * pageItem->m_resolutionY / 72.0 * pageItem->m_size.height());

        pageItem->m_highlightTransform.reset();
        pageItem->m_highlightTransform.scale(pageItem->m_scale * pageItem->m_resolutionX / 72.0, pageItem->m_scale * pageItem->m_resolutionY / 72.0);
    }

    m_pageTransform.reset();

    switch(m_rotation)
    {
    case RotateBy0:
        m_pageTransform.rotate(0.0); break;
    case RotateBy90:
        m_pageTransform.rotate(90.0); break;
    case RotateBy180:
        m_pageTransform.rotate(180.0); break;
    case RotateBy270:
        m_pageTransform.rotate(270.0); break;
    }

    // calculate layout

    m_pagesByHeight.clear();

    qreal sceneWidth = 0.0, sceneHeight = 5.0;

    switch(m_pageLayout)
    {
    case OnePage:
    case OneColumn:
        for(int index = 0; index < m_numberOfPages; index++)
        {
            PageItem* pageItem = m_pagesByIndex.value(index);
            QRectF rect = m_pageTransform.mapRect(pageItem->boundingRect());

            pageItem->setPos(5.0 - rect.left(), sceneHeight - rect.top());
            m_pagesByHeight.insert(sceneHeight - 0.4 * rect.height(), pageItem);

            pageItem->update(pageItem->boundingRect());

            sceneWidth = qMax(sceneWidth, rect.width() + 10.0);
            sceneHeight += rect.height() + 5.0;
        }

        break;
    case TwoPages:
    case TwoColumns:
        for(int index = 0; index < (m_numberOfPages % 2 == 0 ? m_numberOfPages : m_numberOfPages - 1); index += 2)
        {
            PageItem* leftPageItem = m_pagesByIndex.value(index);
            PageItem* rightPageItem = m_pagesByIndex.value(index + 1);
            QRectF leftRect = m_pageTransform.mapRect(leftPageItem->boundingRect());
            QRectF rightRect = m_pageTransform.mapRect(rightPageItem->boundingRect());

            leftPageItem->setPos(5.0 - leftRect.left(), sceneHeight - leftRect.top());
            rightPageItem->setPos(10.0 + leftRect.width() - rightRect.left(), sceneHeight - rightRect.top());
            m_pagesByHeight.insert(sceneHeight - 0.4 * leftRect.height(), leftPageItem);

            leftPageItem->update(leftPageItem->boundingRect());
            rightPageItem->update(rightPageItem->boundingRect());

            sceneWidth = qMax(sceneWidth, leftRect.width() + rightRect.width() + 15.0);
            sceneHeight += qMax(leftRect.height(), rightRect.height()) + 5.0;
        }

        if(m_numberOfPages % 2 != 0)
        {
            PageItem* leftPageItem = m_pagesByIndex.value(m_numberOfPages - 1);
            QRectF leftRect = m_pageTransform.mapRect(leftPageItem->boundingRect());

            leftPageItem->setPos(5.0 - leftRect.left(), sceneHeight - leftRect.top());
            m_pagesByHeight.insert(sceneHeight - 0.4 * leftRect.height(), leftPageItem);

            leftPageItem->update(leftPageItem->boundingRect());

            sceneWidth = qMax(sceneWidth, leftRect.width() + 10.0);
            sceneHeight += leftRect.height() + 5.0;
        }

        break;
    }

    m_scene->setSceneRect(0.0, 0.0, sceneWidth, sceneHeight);
    m_view->setSceneRect(0.0, 0.0, sceneWidth, sceneHeight);

    // bookmarks

    m_bookmarksMenu->clearList();
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

        if(leftPageItem != 0)
        {
            leftPageItem->setVisible(true);

            QRectF leftRect = m_pageTransform.mapRect(leftPageItem->boundingRect()).translated(leftPageItem->pos());

            m_view->setSceneRect(leftRect.adjusted(-5.0, -5.0, 5.0, 5.0));

            m_view->horizontalScrollBar()->setValue(qFloor(leftRect.left()));
            m_view->verticalScrollBar()->setValue(qFloor(leftRect.top() + leftRect.height() * top));
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

            QRectF leftRect = m_pageTransform.mapRect(leftPageItem->boundingRect()).translated(leftPageItem->pos());
            QRectF rightRect = m_pageTransform.mapRect(rightPageItem->boundingRect()).translated(rightPageItem->pos());

            m_view->setSceneRect(leftRect.united(rightRect).adjusted(-5.0, -5.0, 5.0, 5.0));

            m_view->horizontalScrollBar()->setValue(qFloor(leftRect.left()));
            m_view->verticalScrollBar()->setValue(qFloor(leftRect.top() + leftRect.height() * top));
        }
        else if(leftPageItem != 0)
        {
            leftPageItem->setVisible(true);

            QRectF leftRect = m_pageTransform.mapRect(leftPageItem->boundingRect()).translated(leftPageItem->pos());

            m_view->setSceneRect(leftRect.adjusted(-5.0, -5.0, 5.0, 5.0));

            m_view->horizontalScrollBar()->setValue(qFloor(leftRect.left()));
            m_view->verticalScrollBar()->setValue(qFloor(leftRect.top() + leftRect.height() * top));
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
            QRectF leftRect = m_pageTransform.mapRect(leftPageItem->boundingRect()).translated(leftPageItem->pos());

            m_view->horizontalScrollBar()->setValue(qFloor(leftRect.left()));
            m_view->verticalScrollBar()->setValue(qFloor(leftRect.top() + leftRect.height() * top));
        }

        break;
    }

    // highlight

    if(m_currentResult != m_results.end())
    {
        PageItem* pageItem = m_pagesByIndex.value(m_currentResult.key(), 0);

        if(pageItem != 0)
        {
            m_highlight->setPos(pageItem->pos());
            m_highlight->setTransform(pageItem->m_highlightTransform);
            m_highlight->setTransform(m_pageTransform, true);

            m_highlight->setRect(m_currentResult.value().adjusted(-1.0, -1.0, 1.0, 1.0));

            m_highlight->setVisible(true);
        }
        else
        {
            m_highlight->setVisible(false);
        }
    }
    else
    {
        m_highlight->setVisible(false);
    }
}
