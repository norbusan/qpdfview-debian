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
    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, Qt::black);
    this->setPalette(palette);

    this->setMouseTracking(true);

    this->preparePage();
}

PresentationView::~PresentationView()
{
    if(m_render.isRunning())
    {
        m_render.waitForFinished();
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
    m_linkTransform = QTransform(m_resolutionX / 72.0 * m_size.width(), 0.0, 0.0, m_resolutionY / 72.0 * m_size.height(), 0.0, 0.0);
}

void PresentationView::render()
{
    m_model->pushPage(m_currentPage-1, m_resolutionX, m_resolutionY);

    this->update();
}

void PresentationView::previousPage()
{
    if(m_currentPage > 1)
    {
        m_currentPage--;

        this->preparePage();

        this->update();
    }
}

void PresentationView::nextPage()
{
    if(m_currentPage < m_model->pageCount())
    {
        m_currentPage++;

        this->preparePage();

        this->update();
    }
}

void PresentationView::firstPage()
{
    if(m_currentPage != 1)
    {
        m_currentPage = 1;

        this->preparePage();

        this->update();
    }
}

void PresentationView::lastPage()
{
    if(m_currentPage != m_model->pageCount())
    {
        m_currentPage = m_model->pageCount();

        this->preparePage();

        this->update();
    }
}

void PresentationView::gotoPage(int pageNumber)
{
    if(m_currentPage != pageNumber && pageNumber >= 1 && pageNumber <= m_model->pageCount())
    {
        m_currentPage = pageNumber;

        this->preparePage();

        this->update();
    }
}

void PresentationView::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);

    // draw page

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

    // draw links

    // TODO

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
    case Qt::Key_Control+Qt::Key_Home:
    case Qt::Key_Home:
        this->firstPage();

        break;
    case Qt::Key_Control+Qt::Key_End:
    case Qt::Key_End:
        this->lastPage();

        break;
    case Qt::Key_Escape:
        this->close();

        break;
    }
}

void PresentationView::mousePressEvent(QMouseEvent *event)
{
    // TODO
}

void PresentationView::mouseMoveEvent(QMouseEvent *event)
{
    // TODO
}
