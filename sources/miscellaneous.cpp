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

#include "miscellaneous.h"

// presentation widget

PresentationView::PresentationView() : QWidget(),
    m_document(0),
    m_pageCache(),
    m_pageCacheSize(0u),
    m_maximumPageCacheSize(33554432u),
    m_filePath(),
    m_numberOfPages(-1),
    m_currentPage(-1),
    m_settings(),
    m_scale(1.0),
    m_boundingRect(),
    m_links(),
    m_linkTransform(),
    m_render()
{
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    setWindowState(windowState() | Qt::WindowFullScreen);

    setMouseTracking(true);

    // prefetch timer

    m_prefetchTimer = new QTimer(this);
    m_prefetchTimer->setInterval(500);
    m_prefetchTimer->setSingleShot(true);

    connect(m_prefetchTimer, SIGNAL(timeout()), SLOT(slotPrefetchTimerTimeout()));
}

PresentationView::~PresentationView()
{
    m_render.waitForFinished();

    if(m_document != 0)
    {
        delete m_document;
    }
}

bool PresentationView::open(const QString& filePath)
{
    m_render.waitForFinished();

    Poppler::Document* document = Poppler::Document::load(filePath);

    if(document != 0)
    {
        if(m_document != 0)
        {
            delete m_document;
        }

        m_document = document;

        m_filePath = filePath;
        m_numberOfPages = m_document->numPages();
        m_currentPage = 1;
    }

    if(m_document != 0)
    {
        m_document->setRenderHint(Poppler::Document::Antialiasing, m_settings.value("documentView/antialiasing", true).toBool());
        m_document->setRenderHint(Poppler::Document::TextAntialiasing, m_settings.value("documentView/textAntialiasing", true).toBool());
        m_document->setRenderHint(Poppler::Document::TextHinting, m_settings.value("documentView/textHinting", false).toBool());
    }

    m_pageCache.clear();
    m_pageCacheSize = 0u;
    m_maximumPageCacheSize = m_settings.value("documentView/maximumPageCacheSize", 33554432u).toUInt();

    prepareView();

    return document != 0;
}

void PresentationView::setCurrentPage(int currentPage)
{
    if(m_currentPage != currentPage && currentPage >= 1 && currentPage <= m_numberOfPages)
    {
        m_currentPage = currentPage;

        prepareView();
    }
}

void PresentationView::previousPage()
{
    if(m_currentPage > 1)
    {
        m_currentPage--;

        prepareView();
    }
}

void PresentationView::nextPage()
{
    if(m_currentPage < m_numberOfPages)
    {
        m_currentPage++;

        prepareView();
    }
}

void PresentationView::firstPage()
{
    if(m_currentPage != 1)
    {
        m_currentPage = 1;

        prepareView();
    }
}

void PresentationView::lastPage()
{
    if(m_currentPage != m_numberOfPages)
    {
        m_currentPage = m_numberOfPages;

        prepareView();
    }
}

void PresentationView::slotPrefetchTimerTimeout()
{
#ifndef RENDER_IN_PAINT

    if(m_currentPage + 1 <= m_numberOfPages)
    {
        if(!m_render.isRunning())
        {
            m_render = QtConcurrent::run(this, &PresentationView::render, m_currentPage);
        }
        else
        {
            m_prefetchTimer->start();
        }
    }

#endif
}

void PresentationView::resizeEvent(QResizeEvent*)
{
    prepareView();
}

void PresentationView::paintEvent(QPaintEvent*)
{
    QPainter painter(this);

    painter.fillRect(rect(), QBrush(Qt::black));
    painter.fillRect(m_boundingRect, QBrush(Qt::white));

#ifdef RENDER_IN_PAINT

    Poppler::Page* page = m_document->page(m_currentPage - 1);

    QImage image = page->renderToImage(m_scale * 72.0, m_scale * 72.0);

    delete page;

    painter.drawImage(m_boundingRect, image);

#else

    PageCacheKey key(m_currentPage - 1, m_scale);

    if(m_pageCache.contains(key))
    {
        PageCacheValue& value = m_pageCache[key];

        value.time = QTime::currentTime();
        painter.drawImage(m_boundingRect, value.image);
    }
    else
    {
        if(!m_render.isRunning())
        {
            m_render = QtConcurrent::run(this, &PresentationView::render, m_currentPage - 1);
        }
    }

#endif
}

void PresentationView::keyPressEvent(QKeyEvent* event)
{
    switch(event->key())
    {
    case Qt::Key_PageUp:
    case Qt::Key_Up:
    case Qt::Key_Left:
    case Qt::Key_Backspace:
        previousPage();

        break;
    case Qt::Key_PageDown:
    case Qt::Key_Down:
    case Qt::Key_Right:
    case Qt::Key_Space:
        nextPage();

        break;
    case Qt::Key_Home:
        firstPage();

        break;
    case Qt::Key_End:
        lastPage();

        break;
    case Qt::Key_F12:
    case Qt::Key_Escape:
        close();

        break;
    }
}

void PresentationView::mouseMoveEvent(QMouseEvent* event)
{
    QApplication::restoreOverrideCursor();

    foreach(Link link, m_links)
    {
        if(m_linkTransform.mapRect(link.area).contains(event->posF()))
        {
            QApplication::setOverrideCursor(Qt::PointingHandCursor);

            QToolTip::showText(event->globalPos(), tr("Go to page %1.").arg(link.page));

            return;
        }
    }

    QToolTip::hideText();
}

void PresentationView::mousePressEvent(QMouseEvent* event)
{
    foreach(Link link, m_links)
    {
        if(m_linkTransform.mapRect(link.area).contains(event->posF()))
        {
            setCurrentPage(link.page);

            return;
        }
    }
}

void PresentationView::prepareView()
{
    Poppler::Page* page = m_document->page(m_currentPage - 1);
    QSizeF size = page->pageSizeF();

    // graphics

    m_scale = qMin(width() / size.width(), height() / size.height());

    m_boundingRect.setLeft(0.5 * (width() - m_scale * size.width()));
    m_boundingRect.setTop(0.5 * (height() - m_scale * size.height()));
    m_boundingRect.setWidth(m_scale * size.width());
    m_boundingRect.setHeight(m_scale * size.height());

    // links

    m_links.clear();

    foreach(Poppler::Link* link, page->links())
    {
        if(link->linkType() == Poppler::Link::Goto)
        {
            if(!static_cast< Poppler::LinkGoto* >(link)->isExternal())
            {
                QRectF area = link->linkArea().normalized();
                int page = static_cast< Poppler::LinkGoto* >(link)->destination().pageNumber();

                page = page >= 1 ? page : 1;
                page = page <= m_numberOfPages ? page : m_numberOfPages;

                m_links.append(Link(area, page));
            }
        }

        delete link;
    }

    m_linkTransform = QTransform(m_boundingRect.width(), 0.0, 0.0, m_boundingRect.height(), m_boundingRect.left(), m_boundingRect.top());

    delete page;

    m_prefetchTimer->start();

    update();
}

void PresentationView::render(int index)
{
    Poppler::Page* page = m_document->page(index);
    QSizeF size = page->pageSizeF();

    qreal scale = qMin(width() / size.width(), height() / size.height());
    QImage image = page->renderToImage(scale * 72.0, scale * 72.0);

    delete page;

    PageCacheKey key(index, scale);
    PageCacheValue value(image);

    uint byteCount = image.byteCount();

    if(m_maximumPageCacheSize < 3 * byteCount)
    {
        m_maximumPageCacheSize = 3 * byteCount;

        qWarning() << tr("Maximum page cache size is too small. Increased it to %1 bytes to hold at least three pages.").arg(3 * byteCount);
    }

    while(m_pageCacheSize + byteCount > m_maximumPageCacheSize)
    {
        QMap< PageCacheKey, PageCacheValue >::const_iterator first = m_pageCache.begin();
        QMap< PageCacheKey, PageCacheValue >::const_iterator last = --m_pageCache.end();

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

    update();
}

// tab bar

TabBar::TabBar(QWidget* parent) : QTabBar(parent)
{
}

void TabBar::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);

    QAction* asNeededAction = menu.addAction(tr("&As needed"));
    menu.addSeparator();
    QAction* northAction = menu.addAction(tr("&North"));
    QAction* southAction = menu.addAction(tr("&South"));
    QAction* westAction = menu.addAction(tr("&West"));
    QAction* eastAction = menu.addAction(tr("&East"));

    TabWidget* tabWidget = qobject_cast< TabWidget* >(parent()); Q_ASSERT(tabWidget);

    asNeededAction->setCheckable(true);

    asNeededAction->setChecked(tabWidget->tabBarAsNeeded());

    northAction->setCheckable(true);
    southAction->setCheckable(true);
    westAction->setCheckable(true);
    eastAction->setCheckable(true);

    QActionGroup positionGroup(this);
    positionGroup.addAction(northAction);
    positionGroup.addAction(southAction);
    positionGroup.addAction(westAction);
    positionGroup.addAction(eastAction);

    switch(tabWidget->tabPosition())
    {
    case QTabWidget::North:
        northAction->setChecked(true); break;
    case QTabWidget::South:
        southAction->setChecked(true); break;
    case QTabWidget::West:
        westAction->setChecked(true); break;
    case QTabWidget::East:
        eastAction->setChecked(true); break;
    }

    QAction* action = menu.exec(event->globalPos());

    if(action == asNeededAction)
    {
        tabWidget->setTabBarAsNeeded(asNeededAction->isChecked());
    }
    else if(action == northAction)
    {
        tabWidget->setTabPosition(QTabWidget::North);
    }
    else if(action == southAction)
    {
        tabWidget->setTabPosition(QTabWidget::South);
    }
    else if(action == westAction)
    {
        tabWidget->setTabPosition(QTabWidget::West);
    }
    else if(action == eastAction)
    {
        tabWidget->setTabPosition(QTabWidget::East);
    }
}

void TabBar::mousePressEvent(QMouseEvent* event)
{
    QTabBar::mousePressEvent(event);

    if(event->button() == Qt::MidButton)
    {
        emit tabCloseRequested(tabAt(event->pos()));
    }
}

// tab widget

TabWidget::TabWidget(QWidget* parent) : QTabWidget(parent),
    m_tabBarAsNeeded(false)
{
    setTabBar(new TabBar(this));
}

bool TabWidget::tabBarAsNeeded() const
{
    return m_tabBarAsNeeded;
}

void TabWidget::setTabBarAsNeeded(bool tabBarAsNeeded)
{
    m_tabBarAsNeeded = tabBarAsNeeded;

    tabBar()->setVisible(!m_tabBarAsNeeded || count() > 1);
}

void TabWidget::tabInserted(int)
{
    if(m_tabBarAsNeeded)
    {
        tabBar()->setVisible(count() > 1);
    }
}

void TabWidget::tabRemoved(int)
{
    if(m_tabBarAsNeeded)
    {
        tabBar()->setVisible(count() > 1);
    }
}

// line edit

LineEdit::LineEdit(QWidget* parent) : QLineEdit(parent)
{
}

void LineEdit::mousePressEvent(QMouseEvent *event)
{
    QLineEdit::mousePressEvent(event);

    selectAll();
}

// combo box

ComboBox::ComboBox(QWidget* parent) : QComboBox(parent)
{
    setLineEdit(new LineEdit(this));
}

// recently used action

RecentlyUsedAction::RecentlyUsedAction(QObject* parent) : QAction(tr("Recently &used"), parent),
    m_settings()
{
    m_actionGroup = new QActionGroup(this);
    connect(m_actionGroup, SIGNAL(triggered(QAction*)), SLOT(slotActionGroupTriggered(QAction*)));

    m_clearListAction = new QAction(tr("&Clear list"), this);
    connect(m_clearListAction, SIGNAL(triggered()), SLOT(clearList()));

    setMenu(new QMenu());
    m_separator = menu()->addSeparator();
    menu()->addAction(m_clearListAction);

    QStringList filePaths = m_settings.value("mainWindow/recentlyUsed", QStringList()).toStringList();

    foreach(QString filePath, filePaths)
    {
        QFileInfo fileInfo(filePath);

        QAction* action = new QAction(this);
        action->setText(fileInfo.completeBaseName());
        action->setToolTip(fileInfo.absoluteFilePath());
        action->setData(fileInfo.absoluteFilePath());

        m_actionGroup->addAction(action);
        menu()->insertAction(m_separator, action);
    }
}

RecentlyUsedAction::~RecentlyUsedAction()
{
    QStringList filePaths;

    foreach(QAction* action, m_actionGroup->actions())
    {
        filePaths.append(action->data().toString());
    }

    m_settings.setValue("mainWindow/recentlyUsed", filePaths);

    menu()->deleteLater();
}

void RecentlyUsedAction::addEntry(const QString& filePath)
{
    bool addItem = true;
    QFileInfo fileInfo(filePath);

    foreach(QAction* action, m_actionGroup->actions())
    {
        addItem = addItem && action->data().toString() != fileInfo.absoluteFilePath();
    }

    if(addItem)
    {
        QAction* action = new QAction(this);
        action->setText(fileInfo.completeBaseName());
        action->setToolTip(fileInfo.absoluteFilePath());
        action->setData(fileInfo.absoluteFilePath());

        m_actionGroup->addAction(action);
        menu()->insertAction(m_separator, action);
    }

    if(m_actionGroup->actions().size() > 5)
    {
        QAction* first = m_actionGroup->actions().first();

        m_actionGroup->removeAction(first);
        menu()->removeAction(first);

        first->deleteLater();
    }
}

void RecentlyUsedAction::clearList()
{
    foreach(QAction* action, m_actionGroup->actions())
    {
        m_actionGroup->removeAction(action);
        menu()->removeAction(action);

        action->deleteLater();
    }
}

void RecentlyUsedAction::slotActionGroupTriggered(QAction* action)
{
    emit entrySelected(action->data().toString());
}

// bookmarks menu

BookmarksMenu::BookmarksMenu(QWidget* parent) : QMenu(tr("Bookmarks"), parent),
    m_pages(),
    m_values(),
    m_currentPage(-1),
    m_value(-1),
    m_minimum(-1),
    m_maximum(-1)
{
    m_actionGroup = new QActionGroup(this);
    connect(m_actionGroup, SIGNAL(triggered(QAction*)), SLOT(slotActionGroupTriggered(QAction*)));

    m_addEntryAction = new QAction(tr("&Add entry"), this);
    m_addEntryAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_B));
    connect(m_addEntryAction, SIGNAL(triggered()), SLOT(addEntry()));

    m_goToPreviousEntryAction = new QAction(tr("Go to &previous entry"), this);
    m_goToPreviousEntryAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_PageUp));
    connect(m_goToPreviousEntryAction, SIGNAL(triggered()), SLOT(goToPreviousEntry()));

    m_goToNextEntryAction = new QAction(tr("Go to &next entry"), this);
    m_goToNextEntryAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_PageDown));
    connect(m_goToNextEntryAction, SIGNAL(triggered()), SLOT(goToNextEntry()));

    m_removeEntriesOnCurrentPageAction = new QAction(tr("&Remove entries on current page"), this);
    m_removeEntriesOnCurrentPageAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_B));
    connect(m_removeEntriesOnCurrentPageAction, SIGNAL(triggered()), SLOT(removeEntriesOnCurrentPage()));

    m_clearListAction = new QAction(tr("&Clear list"), this);
    m_clearListAction->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_B));
    connect(m_clearListAction, SIGNAL(triggered()), SLOT(clearList()));

    addAction(m_addEntryAction);
    addAction(m_goToPreviousEntryAction);
    addAction(m_goToNextEntryAction);
    addAction(m_removeEntriesOnCurrentPageAction);
    addSeparator();
    m_separator = addSeparator();
    addAction(m_clearListAction);
}

void BookmarksMenu::addEntry()
{
    bool addItem = true;
    QAction* before = 0;

    foreach(QAction* action, m_actionGroup->actions())
    {
        if(QPair< int, int >(m_pages.value(action), m_values.value(action)) == QPair< int, int >(m_currentPage, m_value))
        {
            addItem = false;

            break;
        }
        else if(QPair< int, int >(m_pages.value(action), m_values.value(action)) > QPair< int, int >(m_currentPage, m_value))
        {
            if(before)
            {
                before = QPair< int, int >(m_pages.value(action), m_values.value(action)) < QPair< int, int >(m_pages.value(before), m_values.value(before)) ? action : before;
            }
            else
            {
                before = action;
            }
        }
    }

    if(addItem)
    {
        QAction* action = new QAction(this);

        if(m_maximum > m_minimum)
        {
            action->setText(tr("%1% on page %2").arg(100.0 * (m_value - m_minimum) / (m_maximum - m_minimum), 0, 'f', 2).arg(m_currentPage));
        }
        else
        {
            action->setText(tr("Page %1").arg(m_currentPage));
        }

        m_pages.insert(action, m_currentPage);
        m_values.insert(action, m_value);

        m_actionGroup->addAction(action);

        if(before)
        {
            insertAction(before, action);
        }
        else
        {
            insertAction(m_separator, action);
        }
    }

    if(m_actionGroup->actions().size() > 10)
    {
        QAction* first = m_actionGroup->actions().first();

        m_actionGroup->removeAction(first);
        removeAction(first);

        m_pages.remove(first);
        m_values.remove(first);

        first->deleteLater();
    }
}

void BookmarksMenu::removeEntriesOnCurrentPage()
{
    foreach(QAction* action, m_actionGroup->actions())
    {
        if(m_pages.value(action) == m_currentPage)
        {
            m_actionGroup->removeAction(action);
            removeAction(action);

            m_pages.remove(action);
            m_values.remove(action);

            action->deleteLater();
        }
    }
}

void BookmarksMenu::goToPreviousEntry()
{
    QAction* previous = 0;

    foreach(QAction* action, m_actionGroup->actions())
    {
        if(QPair< int, int >(m_pages.value(action), m_values.value(action)) < QPair< int, int >(m_currentPage, m_value))
        {
            if(previous)
            {
                previous = QPair< int, int >(m_pages.value(action), m_values.value(action)) > QPair< int, int >(m_pages.value(previous), m_values.value(previous)) ? action : previous;
            }
            else
            {
                previous = action;
            }
        }
    }

    if(previous)
    {
        previous->trigger();
    }
    else if(!m_actionGroup->actions().isEmpty())
    {
        QAction* last = m_actionGroup->actions().last();

        foreach(QAction* action, m_actionGroup->actions())
        {
            if(QPair< int, int >(m_pages.value(action), m_values.value(action)) > QPair< int, int >(m_pages.value(last), m_values.value(last)))
            {
                last = action;
            }
        }

        last->trigger();
    }
}

void BookmarksMenu::goToNextEntry()
{
    QAction* next = 0;

    foreach(QAction* action, m_actionGroup->actions())
    {
        if(QPair< int, int >(m_pages.value(action), m_values.value(action)) > QPair< int, int >(m_currentPage, m_value))
        {
            if(next)
            {
                next = QPair< int, int >(m_pages.value(action), m_values.value(action)) < QPair< int, int >(m_pages.value(next), m_values.value(next)) ? action : next;
            }
            else
            {
                next = action;
            }
        }
    }

    if(next)
    {
        next->trigger();
    }
    else if(!m_actionGroup->actions().isEmpty())
    {
        QAction* first = m_actionGroup->actions().first();

        foreach(QAction* action, m_actionGroup->actions())
        {
            if(QPair< int, int >(m_pages.value(action), m_values.value(action)) < QPair< int, int >(m_pages.value(first), m_values.value(first)))
            {
                first = action;
            }
        }

        first->trigger();
    }
}



void BookmarksMenu::clearList()
{
    foreach(QAction* action, m_actionGroup->actions())
    {
        m_actionGroup->removeAction(action);
        removeAction(action);

        m_pages.remove(action);
        m_values.remove(action);

        action->deleteLater();
    }
}

void BookmarksMenu::slotActionGroupTriggered(QAction* action)
{
    emit entrySelected(m_pages.value(action), m_values.value(action));
}

// settings dialog

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent),
    m_settings()
{
    // tab bar as needed

    m_tabBarAsNeededCheckBox = new QCheckBox(this);
    m_tabBarAsNeededCheckBox->setChecked(m_settings.value("mainWindow/tabBarAsNeeded", false).toBool());

    // tab position

    m_tabPositionComboBox = new QComboBox(this);
    m_tabPositionComboBox->addItem(tr("North"), QTabWidget::North);
    m_tabPositionComboBox->addItem(tr("South"), QTabWidget::South);
    m_tabPositionComboBox->addItem(tr("West"), QTabWidget::West);
    m_tabPositionComboBox->addItem(tr("East"), QTabWidget::East);

    uint tabPosition = m_settings.value("mainWindow/tabPosition", 0).toUInt();

    m_tabPositionComboBox->setCurrentIndex(m_tabPositionComboBox->findData(tabPosition));

    // restore tabs

    m_restoreTabsCheckBox = new QCheckBox(this);
    m_restoreTabsCheckBox->setChecked(m_settings.value("mainWindow/restoreTabs", false).toBool());

    // auto-refresh

    m_autoRefreshCheckBox = new QCheckBox(this);
    m_autoRefreshCheckBox->setChecked(m_settings.value("documentView/autoRefresh", false).toBool());

    // fit to equal width

    m_fitToEqualWidthCheckBox = new QCheckBox(this);
    m_fitToEqualWidthCheckBox->setChecked(m_settings.value("documentView/fitToEqualWidth", false).toBool());

    // links

    m_highlightLinksCheckBox = new QCheckBox(this);
    m_highlightLinksCheckBox->setChecked(m_settings.value("documentView/highlightLinks", true).toBool());

    m_externalLinksCheckBox = new QCheckBox(this);
    m_externalLinksCheckBox->setChecked(m_settings.value("documentView/externalLinks", false).toBool());

    // antialiasing and hinting

    m_antialiasingCheckBox = new QCheckBox(this);
    m_antialiasingCheckBox->setChecked(m_settings.value("documentView/antialiasing", true).toBool());

    m_textAntialiasingCheckBox = new QCheckBox(this);
    m_textAntialiasingCheckBox->setChecked(m_settings.value("documentView/textAntialiasing", true).toBool());

    m_textHintingCheckBox = new QCheckBox(this);
    m_textHintingCheckBox->setChecked(m_settings.value("documentView/textHinting", false).toBool());

    // maximum page cache size

    m_maximumPageCacheSizeComboBox = new QComboBox(this);
    m_maximumPageCacheSizeComboBox->addItem(tr("%1 MB").arg(8), 8388608u);
    m_maximumPageCacheSizeComboBox->addItem(tr("%1 MB").arg(16), 16777216u);
    m_maximumPageCacheSizeComboBox->addItem(tr("%1 MB").arg(32), 33554432u);
    m_maximumPageCacheSizeComboBox->addItem(tr("%1 MB").arg(64), 67108864u);
    m_maximumPageCacheSizeComboBox->addItem(tr("%1 MB").arg(128), 134217728u);
    m_maximumPageCacheSizeComboBox->addItem(tr("%1 MB").arg(256), 268435456u);
    m_maximumPageCacheSizeComboBox->addItem(tr("%1 MB").arg(512), 536870912u);

    uint maximumPageCacheSize = m_settings.value("documentView/maximumPageCacheSize", 33554432u).toUInt();

    if(m_maximumPageCacheSizeComboBox->findData(maximumPageCacheSize) != -1)
    {
        m_maximumPageCacheSizeComboBox->setCurrentIndex(m_maximumPageCacheSizeComboBox->findData(maximumPageCacheSize));
    }
    else
    {
        m_maximumPageCacheSizeComboBox->insertItem(0, tr("%1 MB").arg(maximumPageCacheSize / 1024 / 1024), maximumPageCacheSize);
        m_maximumPageCacheSizeComboBox->setCurrentIndex(0);
    }

    // prefetch

    m_prefetchCheckBox = new QCheckBox(this);
    m_prefetchCheckBox->setChecked(m_settings.value("documentView/prefetch", true).toBool());

    // layout

    m_tabWidget = new QTabWidget(this);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    connect(m_buttonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(m_buttonBox, SIGNAL(rejected()), SLOT(reject()));

    setLayout(new QVBoxLayout());
    layout()->addWidget(m_tabWidget);
    layout()->addWidget(m_buttonBox);

    m_behaviourWidget = new QWidget(m_tabWidget);
    m_behaviourLayout = new QFormLayout(m_behaviourWidget);
    m_behaviourWidget->setLayout(m_behaviourLayout);

    m_behaviourLayout->addRow(tr("Tab bar as &needed:"), m_tabBarAsNeededCheckBox);
    m_behaviourLayout->addRow(tr("Tab &position:"), m_tabPositionComboBox);

    m_behaviourLayout->addRow(tr("Restore &tabs:"), m_restoreTabsCheckBox);

    m_behaviourLayout->addRow(tr("Auto-&refresh:"), m_autoRefreshCheckBox);

    m_behaviourLayout->addRow(tr("Fit to equal &width:"), m_fitToEqualWidthCheckBox);

    m_behaviourLayout->addRow(tr("&Highlight links:"), m_highlightLinksCheckBox);
    m_behaviourLayout->addRow(tr("External &links:"), m_externalLinksCheckBox);

    m_tabWidget->addTab(m_behaviourWidget, tr("&Behaviour"));

    m_graphicsWidget = new QWidget(m_tabWidget);
    m_graphicsLayout = new QFormLayout(m_graphicsWidget);
    m_graphicsWidget->setLayout(m_graphicsLayout);

    m_graphicsLayout->addRow(tr("&Antialiasing:"), m_antialiasingCheckBox);
    m_graphicsLayout->addRow(tr("&Text antialiasing:"), m_textAntialiasingCheckBox);
    m_graphicsLayout->addRow(tr("Text &hinting:"), m_textHintingCheckBox);

    m_graphicsLayout->addRow(tr("Maximum page &cache size:"), m_maximumPageCacheSizeComboBox);
    m_graphicsLayout->addRow(tr("&Prefetch:"), m_prefetchCheckBox);

    m_tabWidget->addTab(m_graphicsWidget, tr("&Graphics"));
}

void SettingsDialog::accept()
{
    m_settings.setValue("mainWindow/tabBarAsNeeded", m_tabBarAsNeededCheckBox->isChecked());
    m_settings.setValue("mainWindow/tabPosition", m_tabPositionComboBox->itemData(m_tabPositionComboBox->currentIndex()));

    m_settings.setValue("mainWindow/restoreTabs", m_restoreTabsCheckBox->isChecked());

    m_settings.setValue("documentView/autoRefresh", m_autoRefreshCheckBox->isChecked());

    m_settings.setValue("documentView/fitToEqualWidth", m_fitToEqualWidthCheckBox->isChecked());

    m_settings.setValue("documentView/highlightLinks", m_highlightLinksCheckBox->isChecked());
    m_settings.setValue("documentView/externalLinks", m_externalLinksCheckBox->isChecked());

    m_settings.setValue("documentView/antialiasing", m_antialiasingCheckBox->isChecked());
    m_settings.setValue("documentView/textAntialiasing", m_textAntialiasingCheckBox->isChecked());
    m_settings.setValue("documentView/textHinting", m_textHintingCheckBox->isChecked());

    m_settings.setValue("documentView/maximumPageCacheSize", m_maximumPageCacheSizeComboBox->itemData(m_maximumPageCacheSizeComboBox->currentIndex()));
    m_settings.setValue("documentView/prefetch", m_prefetchCheckBox->isChecked());

    m_settings.sync();

    QDialog::accept();
}

// help dialog

HelpDialog::HelpDialog(QWidget* parent) : QDialog(parent)
{
    m_textBrowser = new QTextBrowser(this);

#ifdef DATA_INSTALL_PATH
    m_textBrowser->setSource(QUrl(QString("file:%1/help.html").arg(DATA_INSTALL_PATH)));
#else
    m_textBrowser->setSource(QUrl("qrc:/miscellaneous/help.html"));
#endif

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, this);
    connect(m_buttonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(m_buttonBox, SIGNAL(rejected()), SLOT(reject()));

    setLayout(new QVBoxLayout());
    layout()->addWidget(m_textBrowser);
    layout()->addWidget(m_buttonBox);
}

QSize HelpDialog::sizeHint() const
{
    return QSize(500, 700);
}
