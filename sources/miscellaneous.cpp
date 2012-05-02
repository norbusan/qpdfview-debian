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
    m_maximumPageCacheSize(134217728u),
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
    Qt::WindowFlags flags = this->windowFlags();
    flags = flags | Qt::FramelessWindowHint;
    this->setWindowFlags(flags);

    Qt::WindowStates states = this->windowState();
    states = states | Qt::WindowFullScreen;
    this->setWindowState(states);

    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, Qt::black);
    this->setPalette(palette);

    this->setMouseTracking(true);

    // prefetchTimer

    m_prefetchTimer = new QTimer(this);
    m_prefetchTimer->setInterval(500);
    m_prefetchTimer->setSingleShot(true);

    connect(m_prefetchTimer, SIGNAL(timeout()), this, SLOT(slotPrefetchTimerTimeout()));
}

PresentationView::~PresentationView()
{
    m_render.waitForFinished();

    if(m_document)
    {
        delete m_document;
    }
}

void PresentationView::setCurrentPage(int currentPage)
{
    if(m_currentPage != currentPage && currentPage >= 1 && currentPage <= m_numberOfPages)
    {
        m_currentPage = currentPage;

        this->prepareView();
    }
}

bool PresentationView::open(const QString &filePath)
{
    m_render.waitForFinished();

    Poppler::Document *document = Poppler::Document::load(filePath);

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

        m_filePath = filePath;
        m_numberOfPages = m_document->numPages();
        m_currentPage = 1;
    }

    m_pageCache.clear();
    m_pageCacheSize = 0u;
    m_maximumPageCacheSize = m_settings.value("documentView/maximumPageCacheSize", 67108864u).toUInt();

    prepareView();

    return document != 0;
}

void PresentationView::previousPage()
{
    if(m_currentPage > 1)
    {
        m_currentPage--;

        this->prepareView();
    }
}

void PresentationView::nextPage()
{
    if(m_currentPage < m_numberOfPages)
    {
        m_currentPage++;

        this->prepareView();
    }
}

void PresentationView::firstPage()
{
    if(m_currentPage != 1)
    {
        m_currentPage = 1;

        this->prepareView();
    }
}

void PresentationView::lastPage()
{
    if(m_currentPage != m_numberOfPages)
    {
        m_currentPage = m_numberOfPages;

        this->prepareView();
    }
}

void PresentationView::slotPrefetchTimerTimeout()
{
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
}

void PresentationView::resizeEvent(QResizeEvent*)
{
    this->prepareView();
}

void PresentationView::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    QPainter painter(this);

    painter.fillRect(m_boundingRect, QBrush(Qt::white));

    PageCacheKey key(m_currentPage - 1, m_scale);

    if(m_pageCache.contains(key))
    {
        painter.drawImage(m_boundingRect, m_pageCache.value(key));
    }
    else
    {
        if(!m_render.isRunning())
        {
            m_render = QtConcurrent::run(this, &PresentationView::render, m_currentPage - 1);
        }
    }

    painter.setPen(QPen(Qt::black));
    painter.drawRect(m_boundingRect);
}

void PresentationView::keyPressEvent(QKeyEvent *event)
{
    switch(event->key())
    {
    case Qt::Key_PageUp:
    case Qt::Key_Up:
    case Qt::Key_Left:
    case Qt::Key_Backspace:
        this->previousPage();

        break;
    case Qt::Key_PageDown:
    case Qt::Key_Down:
    case Qt::Key_Right:
    case Qt::Key_Space:
        this->nextPage();

        break;
    case Qt::Key_Home:
        this->firstPage();

        break;
    case Qt::Key_End:
        this->lastPage();

        break;
    case Qt::Key_F12:
    case Qt::Key_Escape:
        this->close();

        break;
    }
}

void PresentationView::mousePressEvent(QMouseEvent *event)
{
    foreach(Link link, m_links)
    {
        if(m_linkTransform.mapRect(link.area).contains(event->posF()))
        {
            this->setCurrentPage(link.page);

            return;
        }
    }
}

void PresentationView::mouseMoveEvent(QMouseEvent *event)
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

void PresentationView::prepareView()
{
    Poppler::Page *page = m_document->page(m_currentPage - 1);

    // graphics

    QSizeF size = page->pageSizeF();

    m_scale = qMin(static_cast<qreal>(this->width()) / size.width(), static_cast<qreal>(this->height()) / size.height());

    m_boundingRect.setLeft(0.5 * (static_cast<qreal>(this->width()) - m_scale * size.width()));
    m_boundingRect.setTop(0.5 * (static_cast<qreal>(this->height()) - m_scale * size.height()));
    m_boundingRect.setWidth(m_scale * size.width());
    m_boundingRect.setHeight(m_scale * size.height());

    // links

    foreach(Poppler::Link *link, page->links())
    {
        if(link->linkType() == Poppler::Link::Goto)
        {
            if(!static_cast<Poppler::LinkGoto*>(link)->isExternal())
            {
                m_links.append(Link(link->linkArea().normalized(),static_cast<Poppler::LinkGoto*>(link)->destination().pageNumber()));
            }
        }

        delete link;
    }

    m_linkTransform = QTransform(m_boundingRect.width(), 0.0, 0.0, m_boundingRect.height(), m_boundingRect.left(), m_boundingRect.top());

    delete page;

    this->update();

    m_prefetchTimer->start();
}

void PresentationView::render(int index)
{
    Poppler::Page *page = m_document->page(index);

    QSizeF size = page->pageSizeF();

    qreal scale = qMin(static_cast<qreal>(this->width()) / size.width(), static_cast<qreal>(this->height()) / size.height());

    QImage image = page->renderToImage(scale * 72.0, scale * 72.0);

    delete page;

    PageCacheKey key(index, scale);
    uint byteCount = image.byteCount();

    if(m_maximumPageCacheSize < 3 * byteCount)
    {
        m_maximumPageCacheSize = 3 * byteCount;

        qWarning() << tr("Maximum page cache size is too small. Increased it to %1 bytes to hold at least three pages.").arg(3 * byteCount);
    }

    while(m_pageCacheSize + byteCount > m_maximumPageCacheSize)
    {
        QMap< PageCacheKey, QImage >::iterator iterator = m_pageCache.lowerBound(key) != m_pageCache.end() ? --m_pageCache.end() : m_pageCache.begin();

        m_pageCacheSize -= iterator.value().byteCount();
        m_pageCache.remove(iterator.key());
    }

    m_pageCacheSize += byteCount;
    m_pageCache.insert(key, image);

    this->update();
}

// recently used action

RecentlyUsedAction::RecentlyUsedAction(QObject *parent) : QAction(tr("Recently &used"), parent),
    m_settings()
{
    m_actionGroup = new QActionGroup(this);
    connect(m_actionGroup, SIGNAL(triggered(QAction*)), this, SLOT(slotActionGroupTriggered(QAction*)));

    m_separatorAction = new QAction(this);
    m_separatorAction->setSeparator(true);

    m_clearListAction = new QAction(tr("&Clear list"), this);
    connect(m_clearListAction, SIGNAL(triggered()), this, SLOT(clearList()));

    setMenu(new QMenu());
    menu()->addAction(m_separatorAction);
    menu()->addAction(m_clearListAction);

    QStringList filePaths = m_settings.value("mainWindow/recentlyUsed").toStringList();

    foreach(QString filePath, filePaths)
    {
        QAction *action = new QAction(this);
        action->setText(filePath);
        action->setData(filePath);

        m_actionGroup->addAction(action);
        menu()->insertAction(m_separatorAction, action);
    }
}

RecentlyUsedAction::~RecentlyUsedAction()
{
    QStringList filePaths;

    foreach(QAction *action, m_actionGroup->actions())
    {
        filePaths.append(action->data().toString());
    }

    m_settings.setValue("mainWindow/recentlyUsed", filePaths);
}

void RecentlyUsedAction::addEntry(const QString &filePath)
{
    bool addItem = true;

    foreach(QAction *action, m_actionGroup->actions())
    {
        addItem = addItem && action->data().toString() != filePath;
    }

    if(addItem)
    {
        QAction *action = new QAction(this);
        action->setText(filePath);
        action->setData(filePath);

        m_actionGroup->addAction(action);
        menu()->insertAction(m_separatorAction, action);
    }

    if(m_actionGroup->actions().size() > 5)
    {
        QAction *first = m_actionGroup->actions().first();

        m_actionGroup->removeAction(first);
        menu()->removeAction(first);

        first->deleteLater();
    }
}

void RecentlyUsedAction::clearList()
{
    foreach(QAction *action, m_actionGroup->actions())
    {
        m_actionGroup->removeAction(action);
        menu()->removeAction(action);

        action->deleteLater();
    }
}

void RecentlyUsedAction::slotActionGroupTriggered(QAction *action)
{
    emit entrySelected(action->data().toString());
}

// bookmarks menu

BookmarksMenu::BookmarksMenu(QWidget *parent) : QMenu(tr("Bookmarks"), parent),
    m_pages(),
    m_values(),
    m_currentPage(-1),
    m_value(-1),
    m_minimum(-1),
    m_maximum(-1)
{
    m_actionGroup = new QActionGroup(this);
    connect(m_actionGroup, SIGNAL(triggered(QAction*)), this, SLOT(slotActionGroupTriggered(QAction*)));

    m_addEntryAction = new QAction(tr("&Add entry"), this);
    m_addEntryAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_B));
    connect(m_addEntryAction, SIGNAL(triggered()), this, SLOT(addEntry()));

    m_goToPreviousEntryAction = new QAction(tr("Go to &previous entry"), this);
    m_goToPreviousEntryAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_PageUp));
    connect(m_goToPreviousEntryAction, SIGNAL(triggered()), this, SLOT(goToPreviousEntry()));

    m_goToNextEntryAction = new QAction(tr("Go to &next entry"), this);
    m_goToNextEntryAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_PageDown));
    connect(m_goToNextEntryAction, SIGNAL(triggered()), this, SLOT(goToNextEntry()));

    m_removeEntriesOnCurrentPageAction = new QAction(tr("&Remove entries on current page"), this);
    m_removeEntriesOnCurrentPageAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_B));
    connect(m_removeEntriesOnCurrentPageAction, SIGNAL(triggered()), this, SLOT(removeEntriesOnCurrentPage()));

    m_separatorAction = new QAction(this);
    m_separatorAction->setSeparator(true);

    m_clearListAction = new QAction(tr("&Clear list"), this);
    m_clearListAction->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_B));
    connect(m_clearListAction, SIGNAL(triggered()), this, SLOT(clearList()));

    addAction(m_addEntryAction);
    addAction(m_goToPreviousEntryAction);
    addAction(m_goToNextEntryAction);
    addAction(m_removeEntriesOnCurrentPageAction);
    addSeparator();
    addAction(m_separatorAction);
    addAction(m_clearListAction);
}

void BookmarksMenu::addEntry()
{
    bool addItem = true;
    QAction *before = 0;

    foreach(QAction *action, m_actionGroup->actions())
    {
        if(QPair<int, int>(m_pages.value(action), m_values.value(action)) == QPair<int, int>(m_currentPage, m_value))
        {
            addItem = false;

            break;
        }
        else if(QPair<int, int>(m_pages.value(action), m_values.value(action)) > QPair<int, int>(m_currentPage, m_value))
        {
            if(before)
            {
                before = QPair<int, int>(m_pages.value(action), m_values.value(action)) < QPair<int, int>(m_pages.value(before), m_values.value(before)) ? action : before;
            }
            else
            {
                before = action;
            }
        }
    }

    if(addItem)
    {
        QAction *action = new QAction(this);

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
            insertAction(m_separatorAction, action);
        }
    }

    if(m_actionGroup->actions().size() > 10)
    {
        QAction *first = m_actionGroup->actions().first();

        m_actionGroup->removeAction(first);
        this->removeAction(first);

        m_pages.remove(first);
        m_values.remove(first);

        first->deleteLater();
    }
}

void BookmarksMenu::removeEntriesOnCurrentPage()
{
    foreach(QAction *action, m_actionGroup->actions())
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
    QAction *previous = 0;

    foreach(QAction *action, m_actionGroup->actions())
    {
        if(QPair<int, int>(m_pages.value(action), m_values.value(action)) < QPair<int, int>(m_currentPage, m_value))
        {
            if(previous)
            {
                previous = QPair<int, int>(m_pages.value(action), m_values.value(action)) > QPair<int, int>(m_pages.value(previous), m_values.value(previous)) ? action : previous;
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
        QAction *last = m_actionGroup->actions().last();

        foreach(QAction *action, m_actionGroup->actions())
        {
            if(QPair<int, int>(m_pages.value(action), m_values.value(action)) > QPair<int, int>(m_pages.value(last), m_values.value(last)))
            {
                last = action;
            }
        }

        last->trigger();
    }
}

void BookmarksMenu::goToNextEntry()
{
    QAction *next = 0;

    foreach(QAction *action, m_actionGroup->actions())
    {
        if(QPair<int, int>(m_pages.value(action), m_values.value(action)) > QPair<int, int>(m_currentPage, m_value))
        {
            if(next)
            {
                next = QPair<int, int>(m_pages.value(action), m_values.value(action)) < QPair<int, int>(m_pages.value(next), m_values.value(next)) ? action : next;
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
        QAction *first = m_actionGroup->actions().first();

        foreach(QAction *action, m_actionGroup->actions())
        {
            if(QPair<int, int>(m_pages.value(action), m_values.value(action)) < QPair<int, int>(m_pages.value(first), m_values.value(first)))
            {
                first = action;
            }
        }

        first->trigger();
    }
}



void BookmarksMenu::clearList()
{
    foreach(QAction *action, m_actionGroup->actions())
    {
        m_actionGroup->removeAction(action);
        removeAction(action);

        m_pages.remove(action);
        m_values.remove(action);

        action->deleteLater();
    }
}

void BookmarksMenu::slotActionGroupTriggered(QAction *action)
{
    emit entrySelected(m_pages.value(action), m_values.value(action));
}

// settings dialog

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent),
    m_settings()
{
    m_autoRefreshCheckBox = new QCheckBox(this);
    m_autoRefreshCheckBox->setChecked(m_settings.value("documentView/autoRefresh", false).toBool());

    m_externalLinksCheckBox = new QCheckBox(this);
    m_externalLinksCheckBox->setChecked(m_settings.value("documentView/externalLinks", false).toBool());

    m_antialiasingCheckBox = new QCheckBox(this);
    m_antialiasingCheckBox->setChecked(m_settings.value("documentView/antialiasing", true).toBool());

    m_textAntialiasingCheckBox = new QCheckBox(this);
    m_textAntialiasingCheckBox->setChecked(m_settings.value("documentView/textAntialiasing", true).toBool());

    m_textHintingCheckBox = new QCheckBox(this);
    m_textHintingCheckBox->setChecked(m_settings.value("documentView/textHinting", false).toBool());

    m_uniformFitCheckBox = new QCheckBox(this);
    m_uniformFitCheckBox->setChecked(m_settings.value("documentView/uniformFit", false).toBool());

    m_maximumPageCacheSizeComboBox = new QComboBox(this);
    m_maximumPageCacheSizeComboBox->addItem(tr("%1 MB").arg(8), 8388608u);
    m_maximumPageCacheSizeComboBox->addItem(tr("%1 MB").arg(16), 16777216u);
    m_maximumPageCacheSizeComboBox->addItem(tr("%1 MB").arg(32), 33554432u);
    m_maximumPageCacheSizeComboBox->addItem(tr("%1 MB").arg(64), 67108864u);
    m_maximumPageCacheSizeComboBox->addItem(tr("%1 MB").arg(128), 134217728u);
    m_maximumPageCacheSizeComboBox->addItem(tr("%1 MB").arg(256), 268435456u);
    m_maximumPageCacheSizeComboBox->addItem(tr("%1 MB").arg(512), 536870912u);

    uint maximumPageCacheSize = m_settings.value("documentView/maximumPageCacheSize", 67108864u).toUInt();

    if(m_maximumPageCacheSizeComboBox->findData(maximumPageCacheSize) != -1)
    {
        m_maximumPageCacheSizeComboBox->setCurrentIndex(m_maximumPageCacheSizeComboBox->findData(maximumPageCacheSize));
    }
    else
    {
        m_maximumPageCacheSizeComboBox->insertItem(0, tr("%1 MB").arg(maximumPageCacheSize / 1024 / 1024), maximumPageCacheSize);
        m_maximumPageCacheSizeComboBox->setCurrentIndex(0);
    }

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    m_layout = new QFormLayout(this);
    this->setLayout(m_layout);

    m_layout->addRow(tr("Auto-&refresh:"), m_autoRefreshCheckBox);
    m_layout->addRow(tr("External &links:"), m_externalLinksCheckBox);

    m_layout->addRow(tr("&Antialiasing:"), m_antialiasingCheckBox);
    m_layout->addRow(tr("&Text antialiasing:"), m_textAntialiasingCheckBox);
    m_layout->addRow(tr("Text &hinting:"), m_textHintingCheckBox);

    m_layout->addRow(tr("Uniform &fit:"), m_uniformFitCheckBox);

    m_layout->addRow(tr("Maximum page cache &size:"), m_maximumPageCacheSizeComboBox);

    m_layout->addRow(m_buttonBox);
}

void SettingsDialog::accept()
{
    m_settings.setValue("documentView/autoRefresh", m_autoRefreshCheckBox->isChecked());
    m_settings.setValue("documentView/externalLinks", m_externalLinksCheckBox->isChecked());

    m_settings.setValue("documentView/antialiasing", m_antialiasingCheckBox->isChecked());
    m_settings.setValue("documentView/textAntialiasing", m_textAntialiasingCheckBox->isChecked());
    m_settings.setValue("documentView/textHinting", m_textHintingCheckBox->isChecked());

    m_settings.setValue("documentView/uniformFit", m_uniformFitCheckBox->isChecked());

    m_settings.setValue("documentView/maximumPageCacheSize", m_maximumPageCacheSizeComboBox->itemData(m_maximumPageCacheSizeComboBox->currentIndex()));

    QDialog::accept();
}

// help dialog

HelpDialog::HelpDialog(QWidget *parent) : QDialog(parent)
{
    m_textBrowser = new QTextBrowser(this);
#ifdef DATA_INSTALL_PATH
    m_textBrowser->setSource(QUrl(QString(DATA_INSTALL_PATH) + "/help.html"));
#else
    m_textBrowser->setSource(QUrl("qrc:/miscellaneous/help.html"));
#endif

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, this);
    connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    this->setLayout(new QVBoxLayout());
    this->layout()->addWidget(m_textBrowser);
    this->layout()->addWidget(m_buttonBox);
}
