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

#include "presentationview.h"

#include "documentmodel.h"

PresentationView::PresentationView(DocumentModel *model, int currentPage) : QWidget(),
    m_model(model),
    m_currentPage(currentPage),
    m_size(),
    m_resolutionX(72.0),
    m_resolutionY(72.0),
    m_boundingRect(),
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

    this->preparePage();

    m_prefetchTimer = new QTimer(this);
    m_prefetchTimer->setSingleShot(true);
    m_prefetchTimer->setInterval(500);

    connect(m_prefetchTimer, SIGNAL(timeout()), this, SLOT(prefetchTimeout()));

    m_prefetchTimer->start();
}

PresentationView::~PresentationView()
{
    if(m_render.isRunning())
    {
        m_render.waitForFinished();
    }

    if(m_prefetch.isRunning())
    {
        m_prefetch.waitForFinished();
    }
}

void PresentationView::preparePage()
{
    int screenWidth = QApplication::desktop()->screenGeometry().width();
    int screenHeight = QApplication::desktop()->screenGeometry().height();

    m_size = m_model->pageSize(m_currentPage-1);

    m_resolutionX = 72.0 * qMin(screenWidth / m_size.width(), screenHeight / m_size.height());
    m_resolutionY = 72.0 * qMin(screenWidth / m_size.width(), screenHeight / m_size.height());

    m_boundingRect.setLeft(0.5 * (screenWidth - m_resolutionX / 72.0 * m_size.width()));
    m_boundingRect.setTop(0.5 * (screenHeight - m_resolutionY / 72.0 * m_size.height()));
    m_boundingRect.setWidth(m_resolutionX / 72.0 * m_size.width());
    m_boundingRect.setHeight(m_resolutionY / 72.0 * m_size.height());

    m_links = m_model->links(m_currentPage-1);
    m_linkTransform = QTransform(m_resolutionX / 72.0 * m_size.width(), 0.0, 0.0, m_resolutionY / 72.0 * m_size.height(), m_boundingRect.left(), m_boundingRect.top());
}

void PresentationView::render()
{
    m_model->pushPage(m_currentPage-1, m_resolutionX, m_resolutionY);

    this->update();
}

void PresentationView::prefetch()
{
    int fromPage = qMax(m_currentPage-1, 1);
    int toPage = qMin(m_currentPage+2, m_model->pageCount());

    for(int page = fromPage; page <= toPage; page++)
    {
        QImage image = m_model->pullPage(page-1, m_resolutionX, m_resolutionY);

        if(image.isNull())
        {
            m_model->pushPage(page-1, m_resolutionX, m_resolutionY);
        }
    }

    this->update();
}

void PresentationView::previousPage()
{
    if(m_currentPage > 1)
    {
        m_currentPage--;

        this->preparePage();

        m_prefetchTimer->start();
        this->update();
    }
}

void PresentationView::nextPage()
{
    if(m_currentPage < m_model->pageCount())
    {
        m_currentPage++;

        this->preparePage();

        m_prefetchTimer->start();
        this->update();
    }
}

void PresentationView::firstPage()
{
    if(m_currentPage != 1)
    {
        m_currentPage = 1;

        this->preparePage();

        m_prefetchTimer->start();
        this->update();
    }
}

void PresentationView::lastPage()
{
    if(m_currentPage != m_model->pageCount())
    {
        m_currentPage = m_model->pageCount();

        this->preparePage();

        m_prefetchTimer->start();
        this->update();
    }
}

void PresentationView::gotoPage(int pageNumber)
{
    if(m_currentPage != pageNumber && pageNumber >= 1 && pageNumber <= m_model->pageCount())
    {
        m_currentPage = pageNumber;

        this->preparePage();

        m_prefetchTimer->start();
        this->update();
    }
}

void PresentationView::prefetchTimeout()
{
    if(!m_prefetch.isRunning())
    {
        m_prefetch = QtConcurrent::run(this, &PresentationView::prefetch);
    }
}

void PresentationView::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);

    painter.fillRect(m_boundingRect, QBrush(Qt::white));

    QImage image = m_model->pullPage(m_currentPage-1, m_resolutionX, m_resolutionY);

    if(!image.isNull())
    {
        painter.drawImage(m_boundingRect.topLeft(), image);
    }
    else
    {
        if(!m_render.isRunning())
        {
            m_render = QtConcurrent::run(this, &PresentationView::render);
        }
    }

    painter.setPen(QPen(Qt::black));
    painter.drawRect(m_boundingRect);

    painter.end();
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
    foreach(DocumentModel::Link link, m_links)
    {
        if(m_linkTransform.mapRect(link.area).contains(event->posF()))
        {
            if(link.pageNumber != -1)
            {
                this->gotoPage(link.pageNumber);
            }
            else if(!link.url.isEmpty())
            {
                QDesktopServices::openUrl(QUrl(link.url));
            }

            return;
        }
    }
}

void PresentationView::mouseMoveEvent(QMouseEvent *event)
{
    QApplication::restoreOverrideCursor();

    foreach(DocumentModel::Link link, m_links)
    {
        if(m_linkTransform.mapRect(link.area).contains(event->posF()))
        {
            QApplication::setOverrideCursor(Qt::PointingHandCursor);

            if(link.pageNumber != -1)
            {
                QToolTip::showText(event->globalPos(), tr("Go to page %1.").arg(link.pageNumber));
            }
            else if(!link.url.isEmpty())
            {
                QToolTip::showText(event->globalPos(), tr("Open URL %1.").arg(link.url));
            }

            return;
        }
    }

    QToolTip::hideText();
}
