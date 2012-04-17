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
    m_page(0),
    m_filePath(),
    m_numberOfPages(-1),
    m_currentPage(-1),
    m_settings(),
    m_scale(1.0),
    m_boundingRect(),
    m_image(),
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
    m_render.waitForFinished();

    if(m_page)
    {
        delete m_page;
    }

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

void PresentationView::resizeEvent(QResizeEvent*)
{
    this->prepareView();
}

void PresentationView::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);

    painter.fillRect(m_boundingRect, QBrush(Qt::white));

    if(m_image.isNull())
    {
        if(!m_render.isRunning())
        {
            m_render = QtConcurrent::run(this, &PresentationView::render);
        }
    }
    else
    {
        painter.drawImage(m_boundingRect.topLeft(), m_image);
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
    if(m_page)
    {
        delete m_page;
    }

    m_page = m_document->page(m_currentPage - 1);

    // graphics

    QSizeF size = m_page->pageSizeF();

    m_scale = qMin(static_cast<qreal>(this->width()) / size.width(), static_cast<qreal>(this->height()) / size.height());

    m_boundingRect.setLeft(0.5 * (static_cast<qreal>(this->width()) - m_scale * size.width()));
    m_boundingRect.setTop(0.5 * (static_cast<qreal>(this->height()) - m_scale * size.height()));
    m_boundingRect.setWidth(m_scale * size.width());
    m_boundingRect.setHeight(m_scale * size.height());

    m_image = QImage();

    // links

    foreach(Poppler::Link *link, m_page->links())
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

    this->update();
}

void PresentationView::render()
{
    m_image = m_page->renderToImage(m_scale * 72.0, m_scale * 72.0);

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

// settings dialog

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent),
    m_settings()
{
    m_antialiasingCheckBox = new QCheckBox(this);
    m_antialiasingCheckBox->setChecked(m_settings.value("documentView/antialiasing", true).toBool());

    m_textAntialiasingCheckBox = new QCheckBox(this);
    m_textAntialiasingCheckBox->setChecked(m_settings.value("documentView/textAntialiasing", true).toBool());

    m_textHintingCheckBox = new QCheckBox(this);
    m_textHintingCheckBox->setChecked(m_settings.value("documentView/textHinting", false).toBool());

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    m_layout = new QFormLayout(this);
    this->setLayout(m_layout);

    m_layout->addRow(tr("&Antialiasing:"), m_antialiasingCheckBox);
    m_layout->addRow(tr("&Text antialiasing:"), m_textAntialiasingCheckBox);
    m_layout->addRow(tr("Text &hinting:"), m_textHintingCheckBox);
    m_layout->addRow(m_buttonBox);
}

void SettingsDialog::accept()
{
    m_settings.setValue("documentView/antialiasing", m_antialiasingCheckBox->isChecked());
    m_settings.setValue("documentView/textAntialiasing", m_textAntialiasingCheckBox->isChecked());
    m_settings.setValue("documentView/textHinting", m_textHintingCheckBox->isChecked());

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
