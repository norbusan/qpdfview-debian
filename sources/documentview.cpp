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

// static settings

const qreal DocumentView::pageSpacing = 5.0;
const qreal DocumentView::thumbnailSpacing = 2.5;

qreal DocumentView::thumbnailWidth = 96.0;
qreal DocumentView::thumbnailHeight = 144.0;

const qreal DocumentView::zoomBy = 0.1;

const qreal DocumentView::mininumScaleFactor = 0.1;
const qreal DocumentView::maximumScaleFactor = 5.0;

bool DocumentView::fitToEqualWidth = false;

bool DocumentView::highlightLinks = true;
bool DocumentView::externalLinks = false;

// page item

DocumentView::PageItem::PageItem(QGraphicsItem* parent, QGraphicsScene* scene) : QGraphicsItem(parent, scene),
    m_page(0),
    m_index(-1),
    m_scale(1.0),
    m_links(),
    m_rubberBand(),
    m_size(),
    m_linkTransform(),
    m_highlightTransform(),
    m_render()
{
    setAcceptHoverEvents(true);
}

DocumentView::PageItem::~PageItem()
{
    m_render.waitForFinished();

    if(m_page != 0)
    {
        delete m_page;
    }
}

QRectF DocumentView::PageItem::boundingRect() const
{
    DocumentView* parent = qobject_cast< DocumentView* >(scene()->parent()); Q_ASSERT(parent);

    return QRectF(0.0, 0.0, qCeil(m_scale * parent->m_resolutionX / 72.0 * m_size.width()), qCeil(m_scale * parent->m_resolutionY / 72.0 * m_size.height()));
}

void DocumentView::PageItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    DocumentView* parent = qobject_cast< DocumentView* >(scene()->parent()); Q_ASSERT(parent);

    // page

    painter->fillRect(boundingRect(), QBrush(Qt::white));

#ifdef RENDER_IN_PAINT

    parent->m_documentMutex.lock();

    QImage image = m_page->renderToImage(m_scale * parent->m_resolutionX, m_scale * parent->m_resolutionY);

    parent->m_documentMutex.unlock();

    painter->drawImage(0.0, 0.0, image);

#else

    parent->m_pageCacheMutex.lock();

    DocumentView::PageCacheKey key(m_index, m_scale * parent->m_resolutionX, m_scale * parent->m_resolutionY);

    if(parent->m_pageCache.contains(key))
    {
        DocumentView::PageCacheValue& value = parent->m_pageCache[key];

        value.time = QTime::currentTime();
        painter->drawImage(0.0, 0.0, value.image);
    }
    else
    {
        if(!m_render.isRunning())
        {
            m_render = QtConcurrent::run(this, &DocumentView::PageItem::render, m_scale, false);
        }
    }

    parent->m_pageCacheMutex.unlock();

#endif

    painter->setPen(QPen(Qt::black));
    painter->drawRect(boundingRect());

    // links

    if(DocumentView::highlightLinks)
    {
        painter->save();

        painter->setTransform(m_linkTransform, true);
        painter->setPen(QPen(QColor(255, 0, 0, 127)));

        foreach(Link link, m_links)
        {
            painter->drawRect(link.area);
        }

        painter->restore();
    }

    // highlights

    painter->save();

    painter->setTransform(m_highlightTransform, true);

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

void DocumentView::PageItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    Q_UNUSED(event);
}

void DocumentView::PageItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    DocumentView* parent = qobject_cast< DocumentView* >(scene()->parent()); Q_ASSERT(parent);

    QApplication::restoreOverrideCursor();

    foreach(Link link, m_links)
    {
        if(m_linkTransform.mapRect(link.area).contains(event->pos()))
        {
            QApplication::setOverrideCursor(Qt::PointingHandCursor);

            if(link.page != -1)
            {
                QToolTip::showText(event->screenPos(), tr("Go to page %1.").arg(link.page), parent);
            }
            else if(!link.url.isEmpty())
            {
                QToolTip::showText(event->screenPos(), tr("Open URL \"%1\".").arg(link.url), parent);
            }

            return;
        }
    }

    QToolTip::hideText();
}

void DocumentView::PageItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    Q_UNUSED(event);
}

void DocumentView::PageItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    DocumentView* parent = qobject_cast< DocumentView* >(scene()->parent()); Q_ASSERT(parent);

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
                    if(DocumentView::externalLinks)
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
    DocumentView* parent = qobject_cast< DocumentView* >(scene()->parent()); Q_ASSERT(parent);

    if(!m_rubberBand.isNull())
    {
        m_rubberBand.setBottomRight(event->pos());

        m_rubberBand = m_rubberBand.normalized();

        // copy text or image

        QMenu menu(parent);

        QAction* copyTextAction = menu.addAction(tr("Copy &text"));
        QAction* copyImageAction = menu.addAction(tr("Copy &image"));

        QAction* action = menu.exec(event->screenPos());

        if(action == copyTextAction)
        {
            parent->m_documentMutex.lock();

            QString text = m_page->text(m_highlightTransform.inverted().mapRect(m_rubberBand));

            parent->m_documentMutex.unlock();

            if(!text.isEmpty())
            {
                QApplication::clipboard()->setText(text);
            }
        }
        else if(action == copyImageAction)
        {
            parent->m_documentMutex.lock();

            QImage image = m_page->renderToImage(m_scale * parent->m_resolutionX, m_scale * parent->m_resolutionY, m_rubberBand.x(), m_rubberBand.y(), m_rubberBand.width(), m_rubberBand.height());

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

void DocumentView::PageItem::render(qreal scale, bool prefetch)
{
    DocumentView* parent = qobject_cast< DocumentView* >(scene()->parent()); Q_ASSERT(parent);

    QRectF rect = parent->m_pageTransform.mapRect(boundingRect()).translated(pos());
    QRectF visibleRect = parent->m_view->mapToScene(parent->m_view->viewport()->rect()).boundingRect();

    if(!rect.intersects(visibleRect) && !prefetch)
    {
        return;
    }

#ifdef RENDER_FROM_DISK

    Poppler::Document* document = Poppler::Document::load(parent->m_filePath);

    if(document == 0)
    {
        qFatal("!document");
        return;
    }

    Poppler::Page* page = document->page(m_index);

    if(page == 0)
    {
        qFatal("!page");
        return;
    }

    Poppler::Document::RenderHints renderHints = parent->m_document->renderHints();

    document->setRenderHint(Poppler::Document::Antialiasing, renderHints.testFlag(Poppler::Document::Antialiasing));
    document->setRenderHint(Poppler::Document::TextAntialiasing, renderHints.testFlag(Poppler::Document::TextAntialiasing));
    document->setRenderHint(Poppler::Document::TextHinting, renderHints.testFlag(Poppler::Document::TextHinting));

    QImage image = page->renderToImage(scale * parent->m_resolutionX, scale * parent->m_resolutionY);

    delete page;
    delete document;

#else

    parent->m_documentMutex.lock();

    QImage image = m_page->renderToImage(scale * parent->m_resolutionX, scale * parent->m_resolutionY);

    parent->m_documentMutex.unlock();

#endif

    DocumentView::PageCacheKey key(m_index, scale * parent->m_resolutionX, scale * parent->m_resolutionY);
    DocumentView::PageCacheValue value(image);

    parent->updatePageCache(key, value);

    emit parent->pageItemChanged(m_index);
}

// thumbnail item

DocumentView::ThumbnailItem::ThumbnailItem(QGraphicsItem* parent, QGraphicsScene* scene) : QGraphicsItem(parent, scene),
    m_page(0),
    m_index(-1),
    m_scale(1.0),
    m_size(),
    m_render()
{
}

DocumentView::ThumbnailItem::~ThumbnailItem()
{
    m_render.waitForFinished();

    if(m_page != 0)
    {
        delete m_page;
    }
}

QRectF DocumentView::ThumbnailItem::boundingRect() const
{
    DocumentView* parent = qobject_cast< DocumentView* >(scene()->parent()); Q_ASSERT(parent);

    return QRectF(0.0, 0.0, qCeil(m_scale * parent->physicalDpiX() / 72.0 * m_size.width()), qCeil(m_scale * parent->physicalDpiY() / 72.0 * m_size.height()));
}

void DocumentView::ThumbnailItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    DocumentView* parent = qobject_cast< DocumentView* >(scene()->parent()); Q_ASSERT(parent);

    // page

    painter->fillRect(boundingRect(), QBrush(Qt::white));

#ifdef RENDER_IN_PAINT

    parent->m_documentMutex.lock();

    QImage image = m_page->renderToImage(m_scale * parent->physicalDpiX(), m_scale * parent->physicalDpiY());

    parent->m_documentMutex.unlock();

    painter->drawImage(0.0, 0.0, image);

#else

    parent->m_pageCacheMutex.lock();

    DocumentView::PageCacheKey key(m_index, m_scale * parent->physicalDpiX(), m_scale * parent->physicalDpiY());

    if(parent->m_pageCache.contains(key))
    {
        DocumentView::PageCacheValue& value = parent->m_pageCache[key];

        value.time = QTime::currentTime();
        painter->drawImage(0.0, 0.0, value.image);
    }
    else
    {
        if(!m_render.isRunning())
        {
            m_render = QtConcurrent::run(this, &DocumentView::ThumbnailItem::render, m_scale);
        }
    }

    parent->m_pageCacheMutex.unlock();

#endif

    painter->setPen(QPen(Qt::black));
    painter->drawRect(boundingRect());
}

void DocumentView::ThumbnailItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent*)
{
    DocumentView* parent = qobject_cast< DocumentView* >(scene()->parent()); Q_ASSERT(parent);

    parent->setCurrentPage(m_index + 1);
}

void DocumentView::ThumbnailItem::render(qreal scale)
{
    DocumentView* parent = qobject_cast< DocumentView* >(scene()->parent()); Q_ASSERT(parent);

    QRectF rect = boundingRect().translated(pos());
    QRectF visibleRect = parent->m_thumbnailsGraphicsView->mapToScene(parent->m_thumbnailsGraphicsView->viewport()->rect()).boundingRect();

    if(!rect.intersects(visibleRect))
    {
        return;
    }

#ifdef RENDER_FROM_DISK

    Poppler::Document* document = Poppler::Document::load(parent->m_filePath);

    if(document == 0)
    {
        qFatal("!document");
        return;
    }

    Poppler::Page* page = document->page(m_index);

    if(page == 0)
    {
        qFatal("!page");
        return;
    }

    Poppler::Document::RenderHints renderHints = parent->m_document->renderHints();

    document->setRenderHint(Poppler::Document::Antialiasing, renderHints.testFlag(Poppler::Document::Antialiasing));
    document->setRenderHint(Poppler::Document::TextAntialiasing, renderHints.testFlag(Poppler::Document::TextAntialiasing));
    document->setRenderHint(Poppler::Document::TextHinting, renderHints.testFlag(Poppler::Document::TextHinting));

    QImage image = page->renderToImage(scale * parent->physicalDpiX(), scale * parent->physicalDpiY());

    delete page;
    delete document;

#else

    parent->m_documentMutex.lock();

    QImage image = m_page->renderToImage(scale * parent->physicalDpiX(), scale * parent->physicalDpiY());

    parent->m_documentMutex.unlock();

#endif

    DocumentView::PageCacheKey key(m_index, scale * parent->physicalDpiX(), scale * parent->physicalDpiY());
    DocumentView::PageCacheValue value(image);

    parent->updatePageCache(key, value);

    emit parent->thumbnailItemChanged(m_index);
}

// document view

DocumentView::DocumentView(QWidget* parent) : QWidget(parent),
    m_document(0),
    m_documentMutex(),
    m_pageCache(),
    m_pageCacheMutex(),
    m_pageCacheSize(0u),
    m_maximumPageCacheSize(33554432u),
    m_filePath(),
    m_numberOfPages(-1),
    m_currentPage(-1),
    m_pageLayout(OnePage),
    m_scaleMode(DoNotScale),
    m_scaleFactor(1.0),
    m_rotation(RotateBy0),
    m_highlightAll(false),
    m_pagesByIndex(),
    m_pagesByHeight(),
    m_thumbnailsByIndex(),
    m_resolutionX(72.0),
    m_resolutionY(72.0),
    m_pageTransform(),
    m_autoRefreshWatcher(0),
    m_results(),
    m_resultsMutex(),
    m_currentResult(m_results.end()),
    m_search(),
    m_print()
{
    connect(this, SIGNAL(pageItemChanged(int)), SLOT(slotUpdatePageItem(int)));
    connect(this, SIGNAL(thumbnailItemChanged(int)), SLOT(slotUpdateThumbnailItem(int)));

    connect(this, SIGNAL(currentPageChanged(int)), SLOT(slotThumbnailsEnsureVisible(int)));

    connect(this, SIGNAL(firstResultFound()), SLOT(findNext()));

    // settings

    m_pageLayout = static_cast< PageLayout >(m_settings.value("documentView/pageLayout", static_cast< uint >(m_pageLayout)).toUInt());
    m_scaleMode = static_cast< ScaleMode >(m_settings.value("documentView/scaleMode", static_cast< uint >(m_scaleMode)).toUInt());
    m_scaleFactor = m_settings.value("documentView/scaleFactor", m_scaleFactor).toReal();
    m_rotation = static_cast< Rotation >(m_settings.value("documentView/rotation", static_cast< uint >(m_rotation)).toUInt());

    m_highlightAll = m_settings.value("documentView/highlightAll", m_highlightAll).toBool();

    // graphics

    m_scene = new QGraphicsScene(this);
    m_scene->setBackgroundBrush(QBrush(Qt::darkGray));

    m_view = new QGraphicsView(m_scene, this);
    m_view->setDragMode(QGraphicsView::ScrollHandDrag);

    setFocusProxy(m_view);

    // highlight

    m_highlight = new QGraphicsRectItem();
    m_highlight->setPen(QPen(QColor(0, 255, 0, 255)));
    m_highlight->setBrush(QBrush(QColor(0, 255, 0, 127)));
    m_highlight->setVisible(false);

    m_scene->addItem(m_highlight);

    // vertical scrollbar

    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    connect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), SLOT(slotVerticalScrollBarValueChanged(int)));

    m_view->installEventFilter(this);
    m_view->verticalScrollBar()->installEventFilter(this);

    // auto-refresh timer

    m_autoRefreshTimer = new QTimer(this);
    m_autoRefreshTimer->setInterval(500);
    m_autoRefreshTimer->setSingleShot(true);

    connect(m_autoRefreshTimer, SIGNAL(timeout()), SLOT(refresh()));

    // prefetch timer

    m_prefetchTimer = new QTimer(this);
    m_prefetchTimer->setInterval(500);
    m_prefetchTimer->setSingleShot(true);

    connect(this, SIGNAL(currentPageChanged(int)), m_prefetchTimer, SLOT(start()));
    connect(this, SIGNAL(pageLayoutChanged(DocumentView::PageLayout)), m_prefetchTimer, SLOT(start()));
    connect(this, SIGNAL(scaleModeChanged(DocumentView::ScaleMode)), m_prefetchTimer, SLOT(start()));

    connect(m_prefetchTimer, SIGNAL(timeout()), SLOT(slotPrefetchTimerTimeout()));

    // bookmarks menu

    m_bookmarksMenu = new BookmarksMenu(this);
    m_bookmarksMenu->updateCurrentPage(m_currentPage);
    m_bookmarksMenu->updateValue(m_view->verticalScrollBar()->value());
    m_bookmarksMenu->updateRange(m_view->verticalScrollBar()->minimum(), m_view->verticalScrollBar()->maximum());

    connect(this, SIGNAL(currentPageChanged(int)), m_bookmarksMenu, SLOT(updateCurrentPage(int)));
    connect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), m_bookmarksMenu, SLOT(updateValue(int)));
    connect(m_view->verticalScrollBar(), SIGNAL(rangeChanged(int,int)), m_bookmarksMenu, SLOT(updateRange(int,int)));

    connect(m_bookmarksMenu, SIGNAL(entrySelected(int,int)), SLOT(slotBookmarksMenuEntrySelected(int,int)));

    // tab action

    m_tabAction = new QAction(this);

    connect(m_tabAction, SIGNAL(triggered()), SLOT(slotTabActionTriggered()));

    // outline

    m_outlineTreeWidget = new QTreeWidget();
    m_outlineTreeWidget->setAlternatingRowColors(true);
    m_outlineTreeWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_outlineTreeWidget->header()->setVisible(false);
    m_outlineTreeWidget->header()->setResizeMode(QHeaderView::Stretch);

    connect(m_outlineTreeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)), SLOT(slotOutlineTreeWidgetItemClicked(QTreeWidgetItem*,int)));

    // meta-information

    m_metaInformationTableWidget = new QTableWidget();
    m_metaInformationTableWidget->setAlternatingRowColors(true);
    m_metaInformationTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_metaInformationTableWidget->horizontalHeader()->setVisible(false);
    m_metaInformationTableWidget->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
    m_metaInformationTableWidget->verticalHeader()->setVisible(false);
    m_metaInformationTableWidget->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);

    // thumbnails

    m_thumbnailsGraphicsView = new QGraphicsView(new QGraphicsScene(this));
    m_thumbnailsGraphicsView->scene()->setBackgroundBrush(QBrush(Qt::darkGray));
    m_thumbnailsGraphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
}

DocumentView::~DocumentView()
{
    clearScene();

    m_search.cancel();
    m_search.waitForFinished();

    m_print.cancel();
    m_print.waitForFinished();

    if(m_document != 0)
    {
        delete m_document;
    }

    if(m_autoRefreshWatcher != 0)
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

        m_settings.setValue("documentView/pageLayout", static_cast< uint >(m_pageLayout));

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

DocumentView::ScaleMode DocumentView::scaleMode() const
{
    return m_scaleMode;
}

void DocumentView::setScaleMode(DocumentView::ScaleMode scaleMode)
{
    if(m_scaleMode != scaleMode)
    {
        m_scaleMode = scaleMode;

        m_settings.setValue("documentView/scaleMode", static_cast< uint >(m_scaleMode));

        prepareScene();
        prepareView();

        emit scaleModeChanged(m_scaleMode);
    }
}

qreal DocumentView::scaleFactor() const
{
    return m_scaleFactor;
}

void DocumentView::setScaleFactor(qreal scaleFactor)
{
    if(m_scaleFactor != scaleFactor && scaleFactor >= mininumScaleFactor && scaleFactor <= maximumScaleFactor)
    {
        m_scaleFactor = scaleFactor;

        m_settings.setValue("documentView/scaleFactor", m_scaleFactor);

        prepareScene();
        prepareView();

        emit scaleFactorChanged(m_scaleFactor);
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

        m_settings.setValue("documentView/rotation", static_cast< uint >(m_rotation));

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

        foreach(PageItem* pageItem, m_pagesByIndex)
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

QTableWidget* DocumentView::fontsTableWidget()
{
    QTableWidget* fontsTableWidget = new QTableWidget();

    fontsTableWidget->setAlternatingRowColors(true);
    fontsTableWidget->setSortingEnabled(true);
    fontsTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    fontsTableWidget->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    fontsTableWidget->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    fontsTableWidget->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    fontsTableWidget->verticalHeader()->setVisible(false);

    m_documentMutex.lock();

    QList< Poppler::FontInfo > fonts = m_document->fonts();

    m_documentMutex.unlock();

    fontsTableWidget->setRowCount(fonts.count());
    fontsTableWidget->setColumnCount(5);

    fontsTableWidget->setHorizontalHeaderLabels(QStringList() << tr("Name") << tr("Type") << tr("Embedded") << tr("Subset") << tr("File"));

    for(int index = 0; index < fonts.count(); index++)
    {
        Poppler::FontInfo font = fonts.at(index);

        fontsTableWidget->setItem(index, 0,new QTableWidgetItem(font.name()));
        fontsTableWidget->setItem(index, 1, new QTableWidgetItem(font.typeName()));
        fontsTableWidget->setItem(index, 2, new QTableWidgetItem(font.isEmbedded() ? tr("Yes") : tr("No")));
        fontsTableWidget->setItem(index, 3, new QTableWidgetItem(font.isSubset() ? tr("Yes") : tr("No")));
        fontsTableWidget->setItem(index, 4, new QTableWidgetItem(font.file()));
    }

    return fontsTableWidget;
}

void DocumentView::clearBookmarks()
{
    m_bookmarksMenu->clearList();
}

bool DocumentView::open(const QString& filePath)
{
    m_prefetchTimer->blockSignals(true);

    Poppler::Document* document = Poppler::Document::load(filePath);

    if(document != 0)
    {
        if(document->isLocked())
        {
            QString password = QInputDialog::getText(0, tr("Password"), tr("Please enter the password required to unlock the document file \"%1\".").arg(filePath), QLineEdit::Password);

            if(document->unlock(password.toLatin1(), password.toLatin1()))
            {
                qWarning() << tr("Could not unlock the document file \"%1\".").arg(filePath);

                delete document;
                return false;
            }
        }

        clearScene();

        cancelSearch();
        cancelPrint();

        if(m_document != 0)
        {
            delete m_document;
        }

        if(m_autoRefreshWatcher != 0)
        {
            delete m_autoRefreshWatcher;
        }

        m_document = document;

        if(m_settings.value("documentView/autoRefresh", false).toBool())
        {
            m_autoRefreshWatcher = new QFileSystemWatcher(QStringList(filePath));

            connect(m_autoRefreshWatcher, SIGNAL(fileChanged(QString)), m_autoRefreshTimer, SLOT(start()));
        }

        m_filePath = filePath;
        m_numberOfPages = m_document->numPages();

        emit filePathChanged(m_filePath);
        emit numberOfPagesChanged(m_numberOfPages);

        m_currentPage = 1;

        emit currentPageChanged(m_currentPage);

        QFileInfo fileInfo(m_filePath);

        m_tabAction->setText(fileInfo.completeBaseName());
        m_tabAction->setToolTip(fileInfo.absoluteFilePath());

        preparePages();

        prepareOutline();
        prepareMetaInformation();
        prepareThumbnails();
    }

    if(m_document != 0)
    {
        m_document->setRenderHint(Poppler::Document::Antialiasing, m_settings.value("documentView/antialiasing", true).toBool());
        m_document->setRenderHint(Poppler::Document::TextAntialiasing, m_settings.value("documentView/textAntialiasing", true).toBool());
        m_document->setRenderHint(Poppler::Document::TextHinting, m_settings.value("documentView/textHinting", false).toBool());
    }

    clearPageCache();

    prepareScene();
    prepareView();

    if(m_settings.value("documentView/prefetch").toBool())
    {
        m_prefetchTimer->blockSignals(false);
        m_prefetchTimer->start();
    }

    return document != 0;
}

bool DocumentView::refresh()
{
    m_prefetchTimer->blockSignals(true);

    Poppler::Document* document = Poppler::Document::load(m_filePath);

    if(document != 0)
    {
        if(document->isLocked())
        {
            QString password = QInputDialog::getText(0, tr("Password"), tr("Please enter the password required to unlock the document file \"%1\".").arg(m_filePath), QLineEdit::Password);

            if(document->unlock(password.toLatin1(), password.toLatin1()))
            {
                qWarning() << tr("Could not unlock the document file \"%1\".").arg(m_filePath);

                delete document;
                return false;
            }
        }

        clearScene();

        cancelSearch();
        cancelPrint();

        if(m_document != 0)
        {
            delete m_document;
        }

        m_document = document;

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

        prepareOutline();
        prepareMetaInformation();
        prepareThumbnails();
    }

    if(m_document != 0)
    {
        m_document->setRenderHint(Poppler::Document::Antialiasing, m_settings.value("documentView/antialiasing", true).toBool());
        m_document->setRenderHint(Poppler::Document::TextAntialiasing, m_settings.value("documentView/textAntialiasing", true).toBool());
        m_document->setRenderHint(Poppler::Document::TextHinting, m_settings.value("documentView/textHinting", false).toBool());
    }

    clearPageCache();

    prepareScene();
    prepareView();

    if(m_settings.value("documentView/prefetch").toBool())
    {
        m_prefetchTimer->blockSignals(false);
        m_prefetchTimer->start();
    }

    return document != 0;
}

bool DocumentView::saveCopy(const QString& filePath)
{
    if(m_document != 0)
    {
        m_documentMutex.lock();

        Poppler::PDFConverter* converter = m_document->pdfConverter();

        converter->setOutputFileName(filePath);

        bool success = converter->convert();

        delete converter;

        m_documentMutex.unlock();

        return success;
    }

    return false;
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
                    m_bookmarksMenu->setReturnPosition(m_currentPage, m_view->verticalScrollBar()->value());

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
                    m_bookmarksMenu->setReturnPosition(m_currentPage, m_view->verticalScrollBar()->value());

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

void DocumentView::zoomIn()
{
    PageItem* pageItem = m_pagesByIndex.value(m_currentPage - 1, 0);

    switch(m_scaleMode)
    {
    case FitToPage:
    case FitToPageWidth:
        if(pageItem != 0)
        {
            setScaleFactor(qMin(pageItem->m_scale + zoomBy, maximumScaleFactor));
            setScaleMode(ScaleFactor);

            break;
        }
    case DoNotScale:
        setScaleFactor(1.0 + zoomBy);
        setScaleMode(ScaleFactor);

        break;
    case ScaleFactor:
        setScaleFactor(qMin(scaleFactor() + zoomBy, maximumScaleFactor));

        break;
    }
}

void DocumentView::zoomOut()
{
    PageItem* pageItem = m_pagesByIndex.value(m_currentPage - 1, 0);

    switch(m_scaleMode)
    {
    case FitToPage:
    case FitToPageWidth:
        if(pageItem != 0)
        {
            setScaleFactor(qMax(pageItem->m_scale - zoomBy, mininumScaleFactor));
            setScaleMode(ScaleFactor);

            break;
        }
    case DoNotScale:
        setScaleFactor(1.0 - zoomBy);
        setScaleMode(ScaleFactor);

        break;
    case ScaleFactor:
        setScaleFactor(qMax(scaleFactor() - zoomBy, mininumScaleFactor));

        break;
    }
}

void DocumentView::rotateLeft()
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

void DocumentView::rotateRight()
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

void DocumentView::startSearch(const QString& text, bool matchCase)
{
    cancelSearch();

    if(m_document != 0 && !text.isEmpty())
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

    m_highlight->setVisible(false);

    if(m_highlightAll)
    {
        foreach(PageItem* pageItem, m_pagesByIndex)
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

    if(m_document != 0 && fromPage >= 1 && fromPage <= m_numberOfPages && toPage >= 1 && toPage <= m_numberOfPages && fromPage <= toPage)
    {
        m_print = QtConcurrent::run(this, &DocumentView::print, printer, fromPage, toPage);
    }
}

void DocumentView::cancelPrint()
{
    m_print.cancel();
    m_print.waitForFinished();
}

bool DocumentView::eventFilter(QObject* object, QEvent* event)
{
    if(object == m_view && event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = static_cast< QKeyEvent* >(event);

        if(keyEvent->modifiers() == Qt::NoModifier && (keyEvent->key() == Qt::Key_PageUp || keyEvent->key() == Qt::Key_PageDown))
        {
            keyPressEvent(keyEvent);

            return true;
        }
    }
    else if(object == m_view->verticalScrollBar() && event->type() == QEvent::Wheel)
    {
        QWheelEvent* wheelEvent = static_cast< QWheelEvent* >(event);

        return wheelEvent->modifiers() == Qt::ControlModifier || wheelEvent->modifiers() == Qt::ShiftModifier || wheelEvent->modifiers() == Qt::AltModifier;
    }

    return false;
}

void DocumentView::showEvent(QShowEvent* event)
{
    if(!event->spontaneous())
    {
        m_view->show();

        prepareScene();
        prepareView();
    }
}

void DocumentView::resizeEvent(QResizeEvent* event)
{
    m_view->resize(event->size());

    if(m_scaleMode == FitToPage || m_scaleMode == FitToPageWidth)
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
    if(event->modifiers() == Qt::NoModifier && (event->key() == Qt::Key_PageUp || event->key() == Qt::Key_Backspace))
    {
        switch(m_pageLayout)
        {
        case OnePage:
        case TwoPages:
            if(m_view->verticalScrollBar()->value() == m_view->verticalScrollBar()->minimum() && m_currentPage > 1)
            {
                previousPage();

                m_view->verticalScrollBar()->setValue(m_view->verticalScrollBar()->maximum());
            }
            else if(m_view->verticalScrollBar()->value() - m_view->verticalScrollBar()->pageStep() < m_view->verticalScrollBar()->minimum())
            {
                m_view->verticalScrollBar()->setValue(m_view->verticalScrollBar()->minimum());
            }
            else
            {
                m_view->verticalScrollBar()->setValue(m_view->verticalScrollBar()->value() - m_view->verticalScrollBar()->pageStep());
            }

            break;
        case OneColumn:
        case TwoColumns:
            m_view->verticalScrollBar()->setValue(m_view->verticalScrollBar()->value() - m_view->verticalScrollBar()->pageStep());

            break;
        }
    }
    else if(event->modifiers() == Qt::NoModifier && (event->key() == Qt::Key_PageDown || event->key() == Qt::Key_Space))
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
            if(m_view->verticalScrollBar()->value() == m_view->verticalScrollBar()->maximum() && m_currentPage < lastPage)
            {
                nextPage();

                m_view->verticalScrollBar()->setValue(m_view->verticalScrollBar()->minimum());
            }
            else if(m_view->verticalScrollBar()->value() + m_view->verticalScrollBar()->pageStep() > m_view->verticalScrollBar()->maximum())
            {
                m_view->verticalScrollBar()->setValue(m_view->verticalScrollBar()->maximum());
            }
            else
            {
                m_view->verticalScrollBar()->setValue(m_view->verticalScrollBar()->value() + m_view->verticalScrollBar()->pageStep());
            }

            break;
        case OneColumn:
        case TwoColumns:
            m_view->verticalScrollBar()->setValue(m_view->verticalScrollBar()->value() + m_view->verticalScrollBar()->pageStep());

            break;
        }
    }
    else
    {
        QKeySequence shortcut(event->modifiers() + event->key());

        foreach(QAction* action, m_bookmarksMenu->actions())
        {
            if(action->shortcut() == shortcut)
            {
                action->trigger();

                return;
            }
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
            zoomIn();
        }
        else if(event->delta() < 0)
        {
            zoomOut();
        }
    }
    else if(event->modifiers() == Qt::ShiftModifier)
    {
        if(event->delta() > 0)
        {
            rotateLeft();
        }
        else if(event->delta() < 0)
        {
            rotateRight();
        }
    }
    else if(event->modifiers() == Qt::AltModifier)
    {
        QWheelEvent wheelEvent(*event);
        wheelEvent.setModifiers(Qt::NoModifier);

        QApplication::sendEvent(m_view->horizontalScrollBar(), &wheelEvent);
    }
}

void DocumentView::slotUpdatePageItem(int index)
{
    PageItem* pageItem = m_pagesByIndex.value(index, 0);

    if(pageItem != 0)
    {
        pageItem->update(pageItem->boundingRect());
    }
}

void DocumentView::slotUpdateThumbnailItem(int index)
{
    ThumbnailItem* thumbnailItem = m_thumbnailsByIndex.value(index, 0);

    if(thumbnailItem != 0)
    {
        thumbnailItem->update(thumbnailItem->boundingRect());
    }
}

void DocumentView::slotVerticalScrollBarValueChanged(int value)
{
    if(m_pageLayout == OneColumn || m_pageLayout == TwoColumns)
    {
        QMap< qreal, PageItem* >::const_iterator iterator = --m_pagesByHeight.lowerBound(value + 0.5 * m_view->verticalScrollBar()->pageStep());

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

void DocumentView::slotThumbnailsEnsureVisible(int currentPage)
{
    ThumbnailItem* thumbnailItem = m_thumbnailsByIndex.value(currentPage - 1, 0);

    if(thumbnailItem != 0)
    {
        m_thumbnailsGraphicsView->ensureVisible(thumbnailItem);
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

    fromPage = qMax(fromPage, 1);
    toPage = qMin(toPage, m_numberOfPages);

    for(int page = fromPage; page <= toPage; page++)
    {
        PageItem* pageItem = m_pagesByIndex.value(page - 1, 0);

        if(pageItem != 0)
        {
            m_pageCacheMutex.lock();

            PageCacheKey key(pageItem->m_index, pageItem->m_scale * m_resolutionX, pageItem->m_scale * m_resolutionY);

            if(!m_pageCache.contains(key))
            {
                if(!pageItem->m_render.isRunning())
                {
                    pageItem->m_render = QtConcurrent::run(pageItem, &DocumentView::PageItem::render, pageItem->m_scale, true);
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
    TabWidget* tabWidget = qobject_cast< TabWidget* >(this->parent()->parent()); Q_ASSERT(tabWidget);

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

    emit searchProgressed(0);

    foreach(int index, indices)
    {
        if(m_search.isCanceled())
        {
            emit searchCanceled();

            return;
        }

        m_documentMutex.lock();

        Poppler::Page* page = m_document->page(index);

        QList< QRectF > results;

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

        if(m_results.contains(index) && firstResult)
        {
            emit firstResultFound();

            firstResult = false;
        }

        m_resultsMutex.unlock();

        if(m_highlightAll)
        {
            emit pageItemChanged(index);
        }

        emit searchProgressed((100 * (indices.indexOf(index) + 1)) / indices.count());
    }

    emit searchFinished();
}

void DocumentView::print(QPrinter* printer, int fromPage, int toPage)
{
#ifdef WITH_CUPS

    emit printProgressed(0);

    int num_dests = 0;
    cups_dest_t* dests = 0;

    num_dests = cupsGetDests(&dests);

    cups_dest_t* dest = 0;

    dest = cupsGetDest(printer->printerName().toLocal8Bit(), 0, num_dests, dests);

    if(dest != 0)
    {
        int num_options = 0;
        cups_option_t* options = 0;

        for(int i = 0; i < dest->num_options; i++)
        {
            num_options = cupsAddOption(dest->options[i].name, dest->options[i].value, num_options, &options);
        }

        // page layout and range

        switch(m_pageLayout)
        {
        case OnePage:
        case OneColumn:
            num_options = cupsAddOption("number-up", QString("%1").arg(1).toLocal8Bit(), num_options, &options);
            num_options = cupsAddOption("page-ranges", QString("%1-%2").arg(fromPage).arg(toPage).toLocal8Bit(), num_options, &options);

            break;
        case TwoPages:
        case TwoColumns:
            num_options = cupsAddOption("number-up", QString("%1").arg(2).toLocal8Bit(), num_options, &options);
            num_options = cupsAddOption("page-ranges", QString("%1-%2").arg(fromPage % 2 == 0 ? fromPage / 2 : (fromPage + 1) / 2).arg(toPage % 2 == 0 ? toPage / 2 : (toPage + 1) / 2).toLocal8Bit(), num_options, &options);

            break;
        }

        // copy count

        num_options = cupsAddOption("copies", QString("%1").arg(printer->copyCount()).toLocal8Bit(), num_options, &options);

        // collate copies

        num_options = cupsAddOption("collate", QString("%1").arg(printer->collateCopies()).toLocal8Bit(), num_options, &options);

        // duplex

        switch(printer->duplex())
        {
        case QPrinter::DuplexNone:
            num_options = cupsAddOption("sides", "one-sided", num_options, &options); break;
        case QPrinter::DuplexAuto:
            break;
        case QPrinter::DuplexLongSide:
            num_options = cupsAddOption("sides", "two-sided-long-edge", num_options, &options); break;
        case QPrinter::DuplexShortSide:
            num_options = cupsAddOption("sides", "two-sided-short-edge", num_options, &options); break;
        }

        // page order

        switch(printer->pageOrder())
        {
        case QPrinter::FirstPageFirst:
            num_options = cupsAddOption("outputorder", "normal", num_options, &options); break;
        case QPrinter::LastPageFirst:
            num_options = cupsAddOption("outputorder", "reverse", num_options, &options); break;
        }

        // color mode

        switch(printer->colorMode())
        {
        case QPrinter::Color:
            break;
        case QPrinter::GrayScale:
            num_options = cupsAddOption("ColorModel", "Gray", num_options, &options); break;
        }

        QFileInfo fileInfo(m_filePath);

        int jobId = cupsPrintFile(dest->name, fileInfo.absoluteFilePath().toLocal8Bit(), fileInfo.completeBaseName().toLocal8Bit(), num_options, options);

        cupsFreeDests(num_dests, dests);
        cupsFreeOptions(num_options, options);

        if(jobId < 1)
        {
            qDebug() << "CUPS:" << cupsLastErrorString();
        }
    }
    else
    {
        qDebug() << "CUPS:" << cupsLastErrorString();
    }

    delete printer;

    emit printProgressed(100);

    emit printFinished();

#else

    printer->setFullPage(true);

    QPainter* painter = new QPainter(printer);

    emit printProgressed(0);

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
        qreal fitToHeight = printer->height() / (printer->physicalDpiY() / 72.0 * page->pageSizeF().height());
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

#endif // WITH_CUPS
}

void DocumentView::clearScene()
{
    m_pagesByIndex.clear();
    m_thumbnailsByIndex.clear();

    m_scene->removeItem(m_highlight);
    m_scene->clear();
    m_scene->addItem(m_highlight);

    m_thumbnailsGraphicsView->scene()->clear();
}

void DocumentView::clearPageCache()
{
    m_pageCacheMutex.lock();

    m_pageCache.clear();
    m_pageCacheSize = 0u;
    m_maximumPageCacheSize = m_settings.value("documentView/maximumPageCacheSize", 33554432u).toUInt();

    m_pageCacheMutex.unlock();
}

void DocumentView::updatePageCache(const DocumentView::PageCacheKey& key, const DocumentView::PageCacheValue& value)
{
    m_pageCacheMutex.lock();

    uint byteCount = value.image.byteCount();

    if(m_maximumPageCacheSize < 3 * byteCount)
    {
        m_maximumPageCacheSize = 3 * byteCount;

        qWarning() << tr("Maximum page cache size is too small. Increased it to %1 bytes to hold at least three pages.").arg(3 * byteCount);
    }

    while(m_pageCacheSize + byteCount > m_maximumPageCacheSize)
    {
        QMap< DocumentView::PageCacheKey, DocumentView::PageCacheValue >::const_iterator first = m_pageCache.begin();
        QMap< DocumentView::PageCacheKey, DocumentView::PageCacheValue >::const_iterator last = --m_pageCache.end();

        if(first.value().time < last.value().time)
        {
            m_pageCacheSize -= first.value().image.byteCount();
            m_pageCache.remove(first.key());
        }
        else
        {
            m_pageCacheSize -= last.value().image.byteCount();
            m_pageCache.remove(last.key());
        }
    }

    m_pageCacheSize += byteCount;
    m_pageCache.insert(key, value);

    m_pageCacheMutex.unlock();
}

void DocumentView::preparePages()
{
    m_scene->removeItem(m_highlight);
    m_scene->clear();
    m_scene->addItem(m_highlight);

    m_pagesByIndex.clear();
    m_pagesByIndex.reserve(m_numberOfPages);

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
                if(!static_cast< Poppler::LinkGoto* >(link)->isExternal())
                {
                    QRectF area = link->linkArea().normalized();
                    int page = static_cast< Poppler::LinkGoto* >(link)->destination().pageNumber();
                    qreal top = static_cast< Poppler::LinkGoto* >(link)->destination().isChangeTop() ? static_cast< Poppler::LinkGoto* >(link)->destination().top() : 0.0;

                    page = qMax(page, 1);
                    page = qMin(page, m_numberOfPages);

                    top = qMax(top, static_cast< qreal >(0.0));
                    top = qMin(top, static_cast< qreal >(1.0));

                    pageItem->m_links.append(Link(area, page, top));
                }
            }
            else if(link->linkType() == Poppler::Link::Browse)
            {
                QRectF area = link->linkArea().normalized();
                QString url = static_cast< Poppler::LinkBrowse* >(link)->url();

                pageItem->m_links.append(Link(area, url));
            }

            delete link;
        }

        m_scene->addItem(pageItem);

        m_pagesByIndex.append(pageItem);
    }
}

void DocumentView::prepareOutline()
{
    m_outlineTreeWidget->clear();

    QDomDocument* toc = m_document->toc();

    if(toc != 0)
    {
        prepareOutline(toc->firstChild(), 0, 0);

        delete toc;
    }
}

void DocumentView::prepareOutline(const QDomNode& node, QTreeWidgetItem* parent, QTreeWidgetItem* sibling)
{
    QDomElement element = node.toElement();

    QTreeWidgetItem* item = 0;

    if(parent != 0)
    {
        item = new QTreeWidgetItem(parent);
    }
    else
    {
        item = new QTreeWidgetItem(m_outlineTreeWidget, sibling);
    }

    item->setText(0, element.tagName());
    item->setToolTip(0, element.tagName());

    if(element.hasAttribute("Destination"))
    {
        Poppler::LinkDestination linkDestination(element.attribute("Destination"));

        int page = linkDestination.pageNumber();
        qreal top = linkDestination.isChangeTop() ? linkDestination.top() : 0.0;

        page = qMax(page, 1);
        page = qMin(page, m_numberOfPages);

        top = qMax(top, static_cast< qreal >(0.0));
        top = qMin(top, static_cast< qreal >(1.0));

        item->setData(0, Qt::UserRole, page);
        item->setData(0, Qt::UserRole+1, top);
    }
    else if(element.hasAttribute("DestinationName"))
    {
        Poppler::LinkDestination* linkDestination = m_document->linkDestination(element.attribute("DestinationName"));

        int page = linkDestination != 0 ? linkDestination->pageNumber() : 1;
        qreal top = linkDestination != 0 ? (linkDestination->isChangeTop() ? linkDestination->top() : 0.0) : 0.0;

        page = qMax(page, 1);
        page = qMin(page, m_numberOfPages);

        top = qMax(top, static_cast< qreal >(0.0));
        top = qMin(top, static_cast< qreal >(1.0));

        item->setData(0, Qt::UserRole, page);
        item->setData(0, Qt::UserRole+1, top);

        delete linkDestination;
    }
    else
    {
        item->setData(0, Qt::UserRole, -1);
    }

    if(QVariant(element.attribute("Open", "false")).toBool())
    {
        m_outlineTreeWidget->expandItem(item);
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

    m_thumbnailsByIndex.clear();
    m_thumbnailsByIndex.reserve(m_numberOfPages);

    qreal sceneWidth = 0.0, sceneHeight = thumbnailSpacing;

    for(int index = 0; index < m_numberOfPages; index++)
    {
        ThumbnailItem* thumbnailItem = new ThumbnailItem();

        thumbnailItem->m_index = index;
        thumbnailItem->m_page = m_document->page(thumbnailItem->m_index);
        thumbnailItem->m_size = thumbnailItem->m_page->pageSizeF();

        thumbnailItem->m_scale = qMin(
                    thumbnailWidth / (physicalDpiX() / 72.0 * thumbnailItem->m_size.width()),
                    thumbnailHeight / (physicalDpiY() / 72.0 * thumbnailItem->m_size.height()));

        thumbnailItem->setPos(thumbnailSpacing, sceneHeight);

        m_thumbnailsGraphicsView->scene()->addItem(thumbnailItem);

        m_thumbnailsByIndex.append(thumbnailItem);

        sceneWidth = qMax(sceneWidth, thumbnailItem->boundingRect().width() + 2 * thumbnailSpacing);
        sceneHeight += thumbnailItem->boundingRect().height() + thumbnailSpacing;

        QGraphicsSimpleTextItem* textItem = new QGraphicsSimpleTextItem(QLocale::system().toString(index + 1));

        textItem->setPos(2 * thumbnailSpacing, sceneHeight);

        m_thumbnailsGraphicsView->scene()->addItem(textItem);

        sceneWidth = qMax(sceneWidth, textItem->boundingRect().width() + 3 * thumbnailSpacing);
        sceneHeight += textItem->boundingRect().height() + thumbnailSpacing;
    }

    m_thumbnailsGraphicsView->scene()->setSceneRect(0.0, 0.0, sceneWidth, sceneHeight);
    m_thumbnailsGraphicsView->setSceneRect(0.0, 0.0, sceneWidth, sceneHeight);

    m_thumbnailsGraphicsView->setMinimumWidth(sceneWidth + 35);
}

void DocumentView::prepareScene()
{
    // calculate scale

    switch(m_rotation)
    {
    case RotateBy0:
    case RotateBy180:
        m_resolutionX = physicalDpiX();
        m_resolutionY = physicalDpiY();

        break;
    case RotateBy90:
    case RotateBy270:
        m_resolutionX = physicalDpiY();
        m_resolutionY = physicalDpiX();

        break;
    }

    if(m_scaleMode == FitToPage || m_scaleMode == FitToPageWidth)
    {
        qreal pageWidth = 0.0, pageHeight = 0.0;
        qreal visibleWidth = m_view->viewport()->width() - 4;
        qreal visibleHeight = m_view->viewport()->height() - 4;

        switch(m_pageLayout)
        {
        case OnePage:
        case OneColumn:
            for(int index = 0; index < m_numberOfPages; index++)
            {
                PageItem* pageItem = m_pagesByIndex.at(index);

                switch(m_rotation)
                {
                case RotateBy0:
                case RotateBy180:
                    pageWidth = m_resolutionX / 72.0 * pageItem->m_size.width();
                    pageHeight = m_resolutionY / 72.0 * pageItem->m_size.height();

                    break;
                case RotateBy90:
                case RotateBy270:
                    pageWidth = m_resolutionY / 72.0 * pageItem->m_size.height();
                    pageHeight = m_resolutionX / 72.0 * pageItem->m_size.width();

                    break;
                }

                qreal scale = (visibleWidth - 2 * pageSpacing) / pageWidth;
                if(m_scaleMode == FitToPage)
                {
                    scale = qMin(scale, (visibleHeight - 2 * pageSpacing) / pageHeight);
                }

                pageItem->m_scale = scale;
            }

            break;
        case TwoPages:
        case TwoColumns:
            for(int index = 0; index < (m_numberOfPages % 2 == 0 ? m_numberOfPages : m_numberOfPages - 1); index += 2)
            {
                PageItem* leftPageItem = m_pagesByIndex.at(index);
                PageItem* rightPageItem = m_pagesByIndex.at(index + 1);

                if(fitToEqualWidth)
                {
                    switch(m_rotation)
                    {
                    case RotateBy0:
                    case RotateBy180:
                        pageWidth = m_resolutionX / 72.0 * 2.0 * qMax(leftPageItem->m_size.width(), rightPageItem->m_size.width());
                        pageHeight = m_resolutionY / 72.0 * qMax(
                                    qMax(leftPageItem->m_size.width(), rightPageItem->m_size.width()) / leftPageItem->m_size.width() * leftPageItem->m_size.height(),
                                    qMax(leftPageItem->m_size.width(), rightPageItem->m_size.width()) / rightPageItem->m_size.width() * rightPageItem->m_size.height());

                        break;
                    case RotateBy90:
                    case RotateBy270:
                        pageWidth = m_resolutionY / 72.0 * 2.0 * qMax(leftPageItem->m_size.height(), rightPageItem->m_size.height());
                        pageHeight = m_resolutionX / 72.0 * qMax(
                                    qMax(leftPageItem->m_size.height(), rightPageItem->m_size.height()) / leftPageItem->m_size.height() * leftPageItem->m_size.width(),
                                    qMax(leftPageItem->m_size.height(), rightPageItem->m_size.height()) / rightPageItem->m_size.height() * rightPageItem->m_size.width());

                        break;
                    }

                    qreal scale = (visibleWidth - 3 * pageSpacing) / pageWidth;
                    if(m_scaleMode == FitToPage)
                    {
                        scale = qMin(scale, (visibleHeight - 2 * pageSpacing) / pageHeight);
                    }

                    switch(m_rotation)
                    {
                    case RotateBy0:
                    case RotateBy180:
                        leftPageItem->m_scale = scale * qMax(leftPageItem->m_size.width(), rightPageItem->m_size.width()) / leftPageItem->m_size.width();
                        rightPageItem->m_scale = scale * qMax(leftPageItem->m_size.width(), rightPageItem->m_size.width()) / rightPageItem->m_size.width();

                        break;
                    case RotateBy90:
                    case RotateBy270:
                        leftPageItem->m_scale = scale * qMax(leftPageItem->m_size.height(), rightPageItem->m_size.height()) / leftPageItem->m_size.height();
                        rightPageItem->m_scale = scale * qMax(leftPageItem->m_size.height(), rightPageItem->m_size.height()) / rightPageItem->m_size.height();

                        break;
                    }
                }
                else
                {
                    switch(m_rotation)
                    {
                    case RotateBy0:
                    case RotateBy180:
                        pageWidth = m_resolutionX / 72.0 * (leftPageItem->m_size.width() + rightPageItem->m_size.width());
                        pageHeight = m_resolutionY / 72.0 * qMax(leftPageItem->m_size.height(), rightPageItem->m_size.height());

                        break;
                    case RotateBy90:
                    case RotateBy270:
                        pageWidth = m_resolutionY / 72.0 * (leftPageItem->m_size.height() + rightPageItem->m_size.height());
                        pageHeight = m_resolutionX / 72.0 * qMax(leftPageItem->m_size.width(), rightPageItem->m_size.width());

                        break;
                    }

                    qreal scale = (visibleWidth - 3 * pageSpacing) / pageWidth;
                    if(m_scaleMode == FitToPage)
                    {
                        scale = qMin(scale, (visibleHeight - 2 * pageSpacing) / pageHeight);
                    }

                    leftPageItem->m_scale = scale;
                    rightPageItem->m_scale = scale;
                }
            }

            if(m_numberOfPages % 2 != 0)
            {
                PageItem* leftPageItem = m_pagesByIndex.at(m_numberOfPages - 1);

                switch(m_rotation)
                {
                case RotateBy0:
                case RotateBy180:
                    pageWidth = m_resolutionX / 72.0 * leftPageItem->m_size.width();
                    pageHeight = m_resolutionY / 72.0 * leftPageItem->m_size.height();

                    break;
                case RotateBy90:
                case RotateBy270:
                    pageWidth = m_resolutionY / 72.0 * leftPageItem->m_size.height();
                    pageHeight = m_resolutionX / 72.0 * leftPageItem->m_size.width();

                    break;
                }

                qreal scale = (visibleWidth - 2 * pageSpacing) / pageWidth;
                if(m_scaleMode == FitToPage)
                {
                    scale = qMin(scale, (visibleHeight - 2 * pageSpacing) / pageHeight);
                }

                leftPageItem->m_scale = scale;
            }

            break;
        }
    }
    else if(m_scaleMode == DoNotScale || m_scaleMode == ScaleFactor)
    {
        foreach(PageItem* pageItem, m_pagesByIndex)
        {
            pageItem->m_scale = m_scaleMode == DoNotScale ? 1.0 : m_scaleFactor;
        }
    }

    // calculate transformations

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

    foreach(PageItem* pageItem, m_pagesByIndex)
    {
        pageItem->prepareGeometryChange();

        pageItem->m_linkTransform.reset();
        pageItem->m_linkTransform.scale(pageItem->m_scale * m_resolutionX / 72.0 * pageItem->m_size.width(), pageItem->m_scale * m_resolutionY / 72.0 * pageItem->m_size.height());

        pageItem->m_highlightTransform.reset();
        pageItem->m_highlightTransform.scale(pageItem->m_scale * m_resolutionX / 72.0, pageItem->m_scale * m_resolutionY / 72.0);

        switch(m_rotation)
        {
        case RotateBy0:
            pageItem->setRotation(0.0); break;
        case RotateBy90:
            pageItem->setRotation(90.0); break;
        case RotateBy180:
            pageItem->setRotation(180.0); break;
        case RotateBy270:
            pageItem->setRotation(270.0); break;
        }
    }

    // calculate layout

    disconnect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(slotVerticalScrollBarValueChanged(int)));

    m_pagesByHeight.clear();
    m_bookmarksMenu->clearList();

    qreal sceneWidth = 0.0, sceneHeight = pageSpacing;

    switch(m_pageLayout)
    {
    case OnePage:
    case OneColumn:
        for(int index = 0; index < m_numberOfPages; index++)
        {
            PageItem* pageItem = m_pagesByIndex.at(index);
            QRectF rect = m_pageTransform.mapRect(pageItem->boundingRect());

            pageItem->setPos(pageSpacing - rect.left(), sceneHeight - rect.top());
            m_pagesByHeight.insert(sceneHeight, pageItem);

            sceneWidth = qMax(sceneWidth, rect.width() + 2 * pageSpacing);
            sceneHeight += rect.height() + pageSpacing;
        }

        break;
    case TwoPages:
    case TwoColumns:
        for(int index = 0; index < (m_numberOfPages % 2 == 0 ? m_numberOfPages : m_numberOfPages - 1); index += 2)
        {
            PageItem* leftPageItem = m_pagesByIndex.at(index);
            PageItem* rightPageItem = m_pagesByIndex.at(index + 1);
            QRectF leftRect = m_pageTransform.mapRect(leftPageItem->boundingRect());
            QRectF rightRect = m_pageTransform.mapRect(rightPageItem->boundingRect());

            leftPageItem->setPos(pageSpacing - leftRect.left(), sceneHeight - leftRect.top());
            rightPageItem->setPos(2 * pageSpacing + leftRect.width() - rightRect.left(), sceneHeight - rightRect.top());
            m_pagesByHeight.insert(sceneHeight, leftPageItem);

            sceneWidth = qMax(sceneWidth, leftRect.width() + rightRect.width() + 3 * pageSpacing);
            sceneHeight += qMax(leftRect.height(), rightRect.height()) + pageSpacing;
        }

        if(m_numberOfPages % 2 != 0)
        {
            PageItem* leftPageItem = m_pagesByIndex.at(m_numberOfPages - 1);
            QRectF leftRect = m_pageTransform.mapRect(leftPageItem->boundingRect());

            leftPageItem->setPos(pageSpacing - leftRect.left(), sceneHeight - leftRect.top());
            m_pagesByHeight.insert(sceneHeight, leftPageItem);

            sceneWidth = qMax(sceneWidth, leftRect.width() + 2 * pageSpacing);
            sceneHeight += leftRect.height() + pageSpacing;
        }

        break;
    }

    m_scene->setSceneRect(0.0, 0.0, sceneWidth, sceneHeight);
    m_view->setSceneRect(0.0, 0.0, sceneWidth, sceneHeight);

    connect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(slotVerticalScrollBarValueChanged(int)));
}

void DocumentView::prepareView(qreal top)
{
    disconnect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(slotVerticalScrollBarValueChanged(int)));

    PageItem* leftPageItem = m_pagesByIndex.value(m_currentPage - 1, 0);
    PageItem* rightPageItem = m_pagesByIndex.value(m_currentPage, 0);

    switch(m_pageLayout)
    {
    case OnePage:
        foreach(PageItem* pageItem, m_pagesByIndex)
        {
            pageItem->setVisible(false);
        }

        if(leftPageItem != 0)
        {
            leftPageItem->setVisible(true);

            QRectF leftRect = m_pageTransform.mapRect(leftPageItem->boundingRect()).translated(leftPageItem->pos());

            m_view->setSceneRect(leftRect.adjusted(-pageSpacing, -pageSpacing, pageSpacing, pageSpacing));
            m_view->verticalScrollBar()->setValue(qFloor(leftRect.top() + leftRect.height() * top));
        }

        break;
    case TwoPages:
        foreach(PageItem* pageItem, m_pagesByIndex)
        {
            pageItem->setVisible(false);
        }

        if(leftPageItem != 0 && rightPageItem != 0)
        {
            leftPageItem->setVisible(true);
            rightPageItem->setVisible(true);

            QRectF leftRect = m_pageTransform.mapRect(leftPageItem->boundingRect()).translated(leftPageItem->pos());
            QRectF rightRect = m_pageTransform.mapRect(rightPageItem->boundingRect()).translated(rightPageItem->pos());

            m_view->setSceneRect(leftRect.united(rightRect).adjusted(-pageSpacing, -pageSpacing, pageSpacing, pageSpacing));
            m_view->verticalScrollBar()->setValue(qFloor(leftRect.top() + leftRect.height() * top));
        }
        else if(leftPageItem != 0)
        {
            leftPageItem->setVisible(true);

            QRectF leftRect = m_pageTransform.mapRect(leftPageItem->boundingRect()).translated(leftPageItem->pos());

            m_view->setSceneRect(leftRect.adjusted(-pageSpacing, -pageSpacing, pageSpacing, pageSpacing));
            m_view->verticalScrollBar()->setValue(qFloor(leftRect.top() + leftRect.height() * top));
        }

        break;
    case OneColumn:
    case TwoColumns:
        foreach(PageItem* pageItem, m_pagesByIndex)
        {
            pageItem->setVisible(true);
        }

        if(leftPageItem != 0)
        {
            QRectF leftRect = m_pageTransform.mapRect(leftPageItem->boundingRect()).translated(leftPageItem->pos());

            m_view->verticalScrollBar()->setValue(qFloor(leftRect.top() + leftRect.height() * top));
        }

        break;
    }

    m_view->update();

    connect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(slotVerticalScrollBarValueChanged(int)));

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

            pageItem->stackBefore(m_highlight);

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
