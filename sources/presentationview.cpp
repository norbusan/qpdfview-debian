/*

Copyright 2012-2013 Adam Reichold

This file is part of qpdfview.

qpdfview is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

qpdfview is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with qpdfview.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "presentationview.h"

#include <QtConcurrentRun>
#include <QKeyEvent>
#include <QPainter>
#include <QToolTip>

#include "model.h"
#include "pageitem.h"

PresentationView::PresentationView(Model::Document* document, QWidget* parent) : QWidget(parent),
    m_document(0),
    m_numberOfPages(-1),
    m_currentPage(1),
    m_returnToPage(),
    m_links(),
    m_scaleFactor(1.0),
    m_normalizedTransform(),
    m_boundingRect(),
    m_image(),
    m_render(0)
{
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    setWindowState(windowState() | Qt::WindowFullScreen);
    setMouseTracking(true);

    m_render = new QFutureWatcher< void >(this);
    connect(m_render, SIGNAL(finished()), SLOT(on_render_finished()));

    connect(this, SIGNAL(imageReady(int,qreal,QImage)), SLOT(on_imageReady(int,qreal,QImage)));

    m_document = document;

    m_numberOfPages = m_document->numberOfPages();

    prepareView();
}

PresentationView::~PresentationView()
{
    m_render->cancel();
    m_render->waitForFinished();

    qDeleteAll(m_links);
}

int PresentationView::numberOfPages() const
{
    return m_numberOfPages;
}

int PresentationView::currentPage() const
{
    return m_currentPage;
}

void PresentationView::show()
{
    QWidget::show();

    prepareView();
}

void PresentationView::previousPage()
{
    jumpToPage(m_currentPage - 1, false);
}

void PresentationView::nextPage()
{
    jumpToPage(m_currentPage + 1, false);
}

void PresentationView::firstPage()
{
    jumpToPage(1);
}

void PresentationView::lastPage()
{
    jumpToPage(m_numberOfPages);
}

void PresentationView::jumpToPage(int page, bool returnTo)
{
    if(m_currentPage != page && page >= 1 && page <= m_numberOfPages)
    {
        if(returnTo)
        {
            m_returnToPage.push(m_currentPage);
        }

        m_currentPage = page;

        prepareView();

        emit currentPageChanged(m_currentPage, returnTo);
    }
}

void PresentationView::startRender()
{
    if(!m_render->isRunning())
    {
        m_render->setFuture(QtConcurrent::run(this, &PresentationView::render, m_currentPage - 1, m_scaleFactor));
    }
}

void PresentationView::cancelRender()
{
    m_render->cancel();

    m_image = QImage();
}

void PresentationView::on_render_finished()
{
    update();
}

void PresentationView::on_imageReady(int index, qreal scaleFactor, QImage image)
{
    if(m_currentPage - 1 != index || !qFuzzyCompare(m_scaleFactor, scaleFactor))
    {
        return;
    }

    if(PageItem::invertColors())
    {
        image.invertPixels();
    }

    if(!m_render->isCanceled())
    {
        m_image = image;
    }
}

void PresentationView::resizeEvent(QResizeEvent*)
{
    prepareView();
}

void PresentationView::paintEvent(QPaintEvent*)
{
    QPainter painter(this);

    QColor backgroundColor = PageItem::paperColor();

    if(PageItem::invertColors())
    {
        backgroundColor.setRgb(~backgroundColor.rgb());
    }

    painter.fillRect(rect(), QBrush(backgroundColor));

    if(!m_image.isNull())
    {
        painter.drawImage(m_boundingRect.topLeft(), m_image);
    }
    else
    {
        startRender();
    }
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

        event->accept();
        return;
    case Qt::Key_PageDown:
    case Qt::Key_Down:
    case Qt::Key_Right:
    case Qt::Key_Space:
        nextPage();

        event->accept();
        return;
    case Qt::Key_Home:
        firstPage();

        event->accept();
        return;
    case Qt::Key_End:
        lastPage();

        event->accept();
        return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if(!m_returnToPage.isEmpty())
        {
            jumpToPage(m_returnToPage.pop(), false);
        }

        event->accept();
        return;
    case Qt::Key_F12:
    case Qt::Key_Escape:
        close();

        event->accept();
        return;
    }

    QWidget::keyPressEvent(event);
}

void PresentationView::mousePressEvent(QMouseEvent* event)
{
    foreach(Model::Link* link, m_links)
    {
        if(m_normalizedTransform.mapRect(link->boundary).contains(event->pos()))
        {
            if(link->page != -1)
            {
                jumpToPage(link->page);

                event->accept();
                return;
            }
        }
    }

    QWidget::mousePressEvent(event);
}

void PresentationView::mouseMoveEvent(QMouseEvent* event)
{
    foreach(Model::Link* link, m_links)
    {
        if(m_normalizedTransform.mapRect(link->boundary).contains(event->pos()))
        {
            if(link->page != -1)
            {
                setCursor(Qt::PointingHandCursor);
                QToolTip::showText(event->globalPos(), tr("Go to page %1.").arg(link->page));

                return;
            }
        }
    }

    unsetCursor();
    QToolTip::hideText();
}

void PresentationView::prepareView()
{
    Model::Page* page = m_document->page(m_currentPage - 1);

    QSizeF size = page->size();

    qDeleteAll(m_links);

    m_links = page->links();

    delete page;

    {
        m_scaleFactor = qMin(width() / size.width(), height() / size.height());

        m_boundingRect.setLeft(0.5 * (width() - m_scaleFactor * size.width()));
        m_boundingRect.setTop(0.5 * (height() - m_scaleFactor * size.height()));
        m_boundingRect.setWidth(m_scaleFactor * size.width());
        m_boundingRect.setHeight(m_scaleFactor * size.height());

        m_normalizedTransform.reset();
        m_normalizedTransform.translate(m_boundingRect.left(), m_boundingRect.top());
        m_normalizedTransform.scale(m_boundingRect.width(), m_boundingRect.height());
    }

    cancelRender();

    update();
}

void PresentationView::render(int index, qreal scaleFactor)
{
    if(m_render->isCanceled())
    {
        return;
    }

    Model::Page* page = m_document->page(index);

    QImage image = page->render(72.0 * scaleFactor, 72.0 * scaleFactor);

    delete page;

    if(m_render->isCanceled())
    {
        return;
    }

    emit imageReady(index, scaleFactor, image);
}
