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

#include "document.h"
#include "documentview.h"
#include "miscellaneous.h"

// auxiliary view

AuxiliaryView::AuxiliaryView(QWidget *parent) : QWidget(parent),
    m_documentView(0)
{
}

DocumentView *AuxiliaryView::documentView() const
{
    return m_documentView;
}

void AuxiliaryView::setDocumentView(DocumentView *documentView)
{
    m_documentView = documentView;

    if(m_documentView)
    {
        connect(m_documentView->m_document, SIGNAL(documentChanged()), this, SLOT(slotDocumentChanged()));
    }

    if(this->isVisible())
    {
        this->slotDocumentChanged();
    }
}

void AuxiliaryView::showEvent(QShowEvent *event)
{
    if(!event->spontaneous())
    {
        this->slotDocumentChanged();
    }
}

// outline view

OutlineView::OutlineView(QWidget *parent) : AuxiliaryView(parent)
{
    m_treeWidget = new QTreeWidget(this);

    m_treeWidget->setAlternatingRowColors(true);
    m_treeWidget->header()->setVisible(false);

    this->setLayout(new QVBoxLayout());
    this->layout()->setContentsMargins(0, 0, 0, 0);
    this->layout()->addWidget(m_treeWidget);

    connect(m_treeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(slotItemClicked(QTreeWidgetItem*,int)));
}

void OutlineView::slotDocumentChanged()
{
    m_treeWidget->clear();

    if(documentView())
    {
        Outline *outline = documentView()->m_document->outline();

        if(outline)
        {
            prepareOutline(outline, 0, 0);

            delete outline;
        }
    }
}

void OutlineView::slotItemClicked(QTreeWidgetItem *item, int column)
{
    int page = item->data(column, Qt::UserRole).toInt();
    qreal top = item->data(column, Qt::UserRole+1).toReal();

    documentView()->setCurrentPage(page, top);
}

void OutlineView::prepareOutline(Outline *outline, QTreeWidgetItem *parent, QTreeWidgetItem *sibling)
{
    QTreeWidgetItem *item = 0;

    if(parent)
    {
        item = new QTreeWidgetItem(parent);
    }
    else
    {
        item = new QTreeWidgetItem(m_treeWidget, sibling);
    }

    item->setText(0, outline->text);
    item->setData(0, Qt::UserRole, outline->page);
    item->setData(0, Qt::UserRole+1, outline->top);

    if(outline->child)
    {
        prepareOutline(outline->child, item, 0);
    }

    if(outline->sibling)
    {
        prepareOutline(outline->sibling, parent, item);
    }
}

// thumbnails view

ThumbnailsView::ThumbnailsView(QWidget *parent) : AuxiliaryView(parent)
{
    m_listWidget = new QListWidget();

    m_listWidget->setViewMode(QListView::IconMode);
    m_listWidget->setResizeMode(QListView::Adjust);
    m_listWidget->setMovement(QListView::Static);

    this->setLayout(new QHBoxLayout());
    this->layout()->addWidget(m_listWidget);

    connect(m_listWidget, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(slotItemClicked(QListWidgetItem*)));
}

void ThumbnailsView::slotDocumentChanged()
{
    m_listWidget->clear();

    if(documentView())
    {
        QListWidgetItem *item = 0;
        int itemWidth = 0.0, itemHeight = 0.0;

        for(int index = 0; index < documentView()->m_document->numberOfPages(); index++)
        {
            QImage thumbnail = documentView()->m_document->thumbnail(index);

            if(!thumbnail.isNull())
            {
                item = new QListWidgetItem(m_listWidget);
                item->setText(tr("%1").arg(index+1));
                item->setIcon(QIcon(QPixmap::fromImage(thumbnail)));
                item->setData(Qt::UserRole, index+1);

                itemWidth = qMax(itemWidth, thumbnail.width());
                itemHeight = qMax(itemHeight, thumbnail.height());
            }
        }

        m_listWidget->setIconSize(QSize(itemWidth, itemHeight));
    }
}

void ThumbnailsView::slotItemClicked(QListWidgetItem *item)
{
    int page = item->data(Qt::UserRole).toInt();

    documentView()->setCurrentPage(page);
}

// presentation view

PresentationView::PresentationView() : AuxiliaryView(),
    m_currentPage(1),
    m_size(),
    m_scaleFactor(1.0),
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
}

PresentationView::~PresentationView()
{
    if(m_render.isRunning())
    {
        m_render.waitForFinished();
    }
}

void PresentationView::setCurrentPage(int currentPage)
{
    if(m_currentPage != currentPage && currentPage >= 1 && currentPage <= documentView()->m_document->numberOfPages())
    {
        m_currentPage = currentPage;

        this->prepareView();
    }
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
    if(m_currentPage < documentView()->m_document->numberOfPages())
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
    if(m_currentPage != documentView()->m_document->numberOfPages())
    {
        m_currentPage = documentView()->m_document->numberOfPages();

        this->prepareView();
    }
}

void PresentationView::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);

    painter.fillRect(m_boundingRect, QBrush(Qt::white));

    QImage image = documentView()->m_document->pullPage(m_currentPage-1, m_scaleFactor);

    if(image.isNull())
    {
        if(!m_render.isRunning())
        {
            m_render = QtConcurrent::run(this, &PresentationView::render);
        }
    }
    else
    {
        painter.drawImage(m_boundingRect.topLeft(), image);
    }

    painter.setPen(QPen(Qt::black));
    painter.drawRect(m_boundingRect);
}

void PresentationView::resizeEvent(QResizeEvent*)
{
    this->prepareView();
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
    case Qt::Key_F10:
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
            if(!link.url.isEmpty())
            {
                if(Document::openUrl())
                {
                    QDesktopServices::openUrl(QUrl(link.url));
                }
            }
            else
            {
                this->setCurrentPage(link.page);
            }

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

            if(!link.url.isEmpty())
            {
                QToolTip::showText(event->globalPos(), tr("Open URL \"%1\".").arg(link.url));
            }
            else
            {
                QToolTip::showText(event->globalPos(), tr("Go to page %1.").arg(link.page));
            }

            return;
        }
    }

    QToolTip::hideText();
}

void PresentationView::slotDocumentChanged()
{
    m_currentPage = documentView()->currentPage();

    this->prepareView();
}

void PresentationView::prepareView()
{
    m_size = documentView()->m_document->size(m_currentPage-1);

    m_scaleFactor = qMin(static_cast<qreal>(this->width()) / m_size.width(), static_cast<qreal>(this->height()) / m_size.height());

    m_boundingRect.setLeft(0.5 * (static_cast<qreal>(this->width()) - m_scaleFactor * m_size.width()));
    m_boundingRect.setTop(0.5 * (static_cast<qreal>(this->height()) - m_scaleFactor * m_size.height()));
    m_boundingRect.setWidth(m_scaleFactor * m_size.width());
    m_boundingRect.setHeight(m_scaleFactor * m_size.height());

    m_links = documentView()->m_document->links(m_currentPage-1);
    m_linkTransform = QTransform(m_scaleFactor * m_size.width(), 0.0, 0.0, m_scaleFactor * m_size.height(), m_boundingRect.left(), m_boundingRect.top());

    this->update();
}

void PresentationView::render()
{
    documentView()->m_document->pushPage(m_currentPage-1, m_scaleFactor);

    this->update();
}

// recently used action

RecentlyUsedAction::RecentlyUsedAction(QObject *parent) : QAction(tr("Recently &used"), parent),
    m_settings()
{
    m_menu = new QMenu();
    this->setMenu(m_menu);

    m_actionGroup = new QActionGroup(this);
    connect(m_actionGroup, SIGNAL(triggered(QAction*)), this, SLOT(slotActionGroupTriggered(QAction*)));

    m_clearListAction = new QAction(tr("&Clear list"), this);
    connect(m_clearListAction, SIGNAL(triggered()), this, SLOT(clearList()));

    m_separator = m_menu->addSeparator();
    m_menu->addAction(m_clearListAction);

    QStringList filePaths = m_settings.value("mainWindow/recentlyUsed").toStringList();

    foreach(QString filePath, filePaths)
    {
        QAction *action = new QAction(this);
        action->setText(filePath);
        action->setData(filePath);

        m_actionGroup->addAction(action);
        m_menu->insertAction(m_separator, action);
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

    delete m_menu;
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
        m_menu->insertAction(m_separator, action);
    }

    if(m_actionGroup->actions().size() > 5)
    {
        QAction *first = m_actionGroup->actions().first();

        m_actionGroup->removeAction(first);
        m_menu->removeAction(first);
    }
}

void RecentlyUsedAction::clearList()
{
    foreach(QAction *action, m_actionGroup->actions())
    {
        m_actionGroup->removeAction(action);
        m_menu->removeAction(action);
    }
}

void RecentlyUsedAction::slotActionGroupTriggered(QAction *action)
{
    emit entrySelected(action->data().toString());
}

// bookmarks menu

BookmarksMenu::BookmarksMenu(QWidget *parent) : QMenu(tr("Bookmarks"), parent),
    m_page(1),
    m_top(0.0)
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

    m_clearPageAction = new QAction(tr("&Clear page"), this);
    m_clearPageAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_B));
    connect(m_clearPageAction, SIGNAL(triggered()), this, SLOT(clearPage()));

    m_clearDocumentAction = new QAction(tr("Clear &document"), this);
    m_clearDocumentAction->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_B));
    connect(m_clearDocumentAction, SIGNAL(triggered()), this, SLOT(clearDocument()));

    this->addAction(m_addEntryAction);
    this->addAction(m_goToPreviousEntryAction);
    this->addAction(m_goToNextEntryAction);
    this->addAction(m_clearPageAction);
    this->addAction(m_clearDocumentAction);
    this->addSeparator();
}

void BookmarksMenu::addEntry()
{
    qreal position = m_page + m_top;

    bool addItem = true;
    QAction *before = 0;

    foreach(QAction *action, m_actionGroup->actions())
    {
        if(action->data().toReal() == position)
        {
            addItem = false;

            break;
        }
        else if(action->data().toReal() > position)
        {
            if(before)
            {
                before = action->data().toReal() < before->data().toReal() ? action : before;
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
        action->setText(tr("Page %1 at %2%").arg(m_page).arg(qFloor(100.0 * m_top)));
        action->setData(position);

        m_actionGroup->addAction(action);
        this->insertAction(before, action);
    }

    if(m_actionGroup->actions().size() > 10)
    {
        QAction *first = m_actionGroup->actions().first();

        m_actionGroup->removeAction(first);
        this->removeAction(first);
    }
}

void BookmarksMenu::goToPreviousEntry()
{
    qreal position = m_page + m_top;

    QAction *previous = 0;

    foreach(QAction *action, m_actionGroup->actions())
    {
        if(action->data().toReal() < position)
        {
            if(previous)
            {
                previous = action->data().toReal() > previous->data().toReal() ? action : previous;
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
            if(action->data().toReal() > last->data().toReal())
            {
                last = action;
            }
        }

        last->trigger();
    }
}

void BookmarksMenu::goToNextEntry()
{
    qreal position = m_page + m_top;

    QAction *next = 0;

    foreach(QAction *action, m_actionGroup->actions())
    {
        if(action->data().toReal() > position)
        {
            if(next)
            {
                next = action->data().toReal() < next->data().toReal() ? action : next;
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
            if(action->data().toReal() < first->data().toReal())
            {
                first = action;
            }
        }

        first->trigger();
    }
}

void BookmarksMenu::setPosition(int page, qreal top)
{
    m_page = page;
    m_top = top;
}

void BookmarksMenu::clearPage()
{
    foreach(QAction *action, m_actionGroup->actions())
    {
        if(qFloor(action->data().toReal()) == m_page)
        {
            m_actionGroup->removeAction(action);
            this->removeAction(action);
        }
    }
}

void BookmarksMenu::clearDocument()
{
    foreach(QAction *action, m_actionGroup->actions())
    {
        m_actionGroup->removeAction(action);
        this->removeAction(action);
    }
}

void BookmarksMenu::slotActionGroupTriggered(QAction *action)
{
    qreal position = action->data().toReal();

    int page = qFloor(position);
    double top = position - qFloor(position);

    emit entrySelected(page, top);
}

// settings dialog

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{

    m_automaticRefreshCheckBox = new QCheckBox(this);
    m_openUrlCheckBox = new QCheckBox(this);

    m_antialiasingCheckBox = new QCheckBox(this);
    m_textAntialiasingCheckBox = new QCheckBox(this);

    m_maximumPageCacheSizeComboBox = new QComboBox(this);
    m_maximumPageCacheSizeComboBox->addItem(tr("%1 MB").arg(32), 33554432u);
    m_maximumPageCacheSizeComboBox->addItem(tr("%1 MB").arg(64), 67108864u);
    m_maximumPageCacheSizeComboBox->addItem(tr("%1 MB").arg(128), 134217728u);
    m_maximumPageCacheSizeComboBox->addItem(tr("%1 MB").arg(256), 268435456u);
    m_maximumPageCacheSizeComboBox->addItem(tr("%1 MB").arg(512), 536870912u);
    m_maximumPageCacheSizeComboBox->addItem(tr("%1 MB").arg(1024), 1073741824u);
    m_maximumPageCacheSizeComboBox->addItem(tr("%1 MB").arg(2048), 2147483648u);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    m_layout = new QFormLayout(this);
    this->setLayout(m_layout);

    m_layout->addRow(tr("Automatic &refresh:"), m_automaticRefreshCheckBox);
    m_layout->addRow(tr("Open &URL:"), m_openUrlCheckBox);
    m_layout->addRow(tr("&Antialiasing:"), m_antialiasingCheckBox);
    m_layout->addRow(tr("&Text antialiasing:"), m_textAntialiasingCheckBox);
    m_layout->addRow(tr("Maximum page cache &size:"), m_maximumPageCacheSizeComboBox);
    m_layout->addRow(m_buttonBox);

    m_automaticRefreshCheckBox->setChecked(Document::automaticRefresh());
    m_openUrlCheckBox->setChecked(Document::openUrl());

    m_antialiasingCheckBox->setChecked(Document::antialiasing());
    m_textAntialiasingCheckBox->setChecked(Document::textAntialiasing());

    // maximumPageCacheSize

    bool addItem = true;

    for(int index = 0; index < m_maximumPageCacheSizeComboBox->count(); index++)
    {
        uint itemData = m_maximumPageCacheSizeComboBox->itemData(index).toUInt();

        if(itemData == Document::maximumPageCacheSize())
        {
            addItem = false;

            m_maximumPageCacheSizeComboBox->setCurrentIndex(index);
        }
    }

    if(addItem)
    {
        m_maximumPageCacheSizeComboBox->addItem(tr("%1 MB").arg(Document::maximumPageCacheSize() / 1024 / 1024), Document::maximumPageCacheSize());

        m_maximumPageCacheSizeComboBox->setCurrentIndex(m_maximumPageCacheSizeComboBox->count()-1);
    }
}

void SettingsDialog::accept()
{
    Document::setAutomaticRefresh(m_automaticRefreshCheckBox->isChecked());
    Document::setOpenUrl(m_openUrlCheckBox->isChecked());

    Document::setAntialiasing(m_antialiasingCheckBox->isChecked());
    Document::setTextAntialiasing(m_textAntialiasingCheckBox->isChecked());

    Document::setMaximumPageCacheSize(m_maximumPageCacheSizeComboBox->itemData(m_maximumPageCacheSizeComboBox->currentIndex()).toUInt());

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
