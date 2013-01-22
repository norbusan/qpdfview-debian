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
#include <QShortcut>
#include <QTimer>
#include <QToolTip>

#include "model.h"
#include "pageitem.h"
#include "documentview.h"

PresentationView::PresentationView(Model::Document* document, QWidget* parent) : QGraphicsView(parent),
    m_prefetchTimer(0),
    m_document(0),
    m_numberOfPages(-1),
    m_currentPage(1),
    m_rotation(RotateBy0),
    m_returnToPage(),
    m_pagesScene(0),
    m_pages()
{
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    setWindowState(windowState() | Qt::WindowFullScreen);

    setAcceptDrops(false);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Left), this, SLOT(rotateLeft()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Right), this, SLOT(rotateRight()));

    m_document = document;

    m_numberOfPages = m_document->numberOfPages();

    // pages

    m_pagesScene = new QGraphicsScene(this);
    setScene(m_pagesScene);

    m_pages.reserve(m_numberOfPages);

    for(int index = 0; index < m_numberOfPages; ++index)
    {
        PageItem* page = new PageItem(m_document->page(index), index);

        page->setPresentationMode(true);
        page->setPhysicalDpi(physicalDpiX(), physicalDpiY());

        m_pagesScene->addItem(page);
        m_pages.append(page);

        connect(page, SIGNAL(linkClicked(int,qreal,qreal)), SLOT(on_pages_linkClicked(int,qreal,qreal)));
    }

    QColor backgroundColor = backgroundColor = PageItem::paperColor();

    if(PageItem::invertColors())
    {
        backgroundColor.setRgb(~backgroundColor.rgb());
    }

    m_pagesScene->setBackgroundBrush(QBrush(backgroundColor));

    // prefetch

    m_prefetchTimer = new QTimer(this);
    m_prefetchTimer->setInterval(250);
    m_prefetchTimer->setSingleShot(true);

    connect(this, SIGNAL(currentPageChanged(int)), m_prefetchTimer, SLOT(start()));
    connect(this, SIGNAL(rotationChanged(Rotation)), m_prefetchTimer, SLOT(start()));

    connect(m_prefetchTimer, SIGNAL(timeout()), SLOT(on_prefetch_timeout()));

    if(DocumentView::prefetch())
    {
        m_prefetchTimer->blockSignals(false);
        m_prefetchTimer->start();
    }
    else
    {
        m_prefetchTimer->blockSignals(true);
        m_prefetchTimer->stop();
    }

    prepareScene();
    prepareView();
}

PresentationView::~PresentationView()
{
    qDeleteAll(m_pages);
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

void PresentationView::rotateLeft()
{
    switch(m_rotation)
    {
    default:
    case RotateBy0:
        m_rotation = RotateBy270;
        break;
    case RotateBy90:
        m_rotation = RotateBy0;
        break;
    case RotateBy180:
        m_rotation = RotateBy90;
        break;
    case RotateBy270:
        m_rotation = RotateBy180;
        break;
    }

    prepareScene();
    prepareView();

    emit rotationChanged(m_rotation);
}

void PresentationView::rotateRight()
{
    switch(m_rotation)
    {
    default:
    case RotateBy0:
        m_rotation = RotateBy90;
        break;
    case RotateBy90:
        m_rotation = RotateBy180;
        break;
    case RotateBy180:
        m_rotation = RotateBy270;
        break;
    case RotateBy270:
        m_rotation = RotateBy0;
        break;
    }

    prepareScene();
    prepareView();

    emit rotationChanged(m_rotation);
}

void PresentationView::on_prefetch_timeout()
{
    int fromPage = m_currentPage, toPage = m_currentPage;

    fromPage -= DocumentView::prefetchDistance() / 2;
    toPage += DocumentView::prefetchDistance();

    fromPage = fromPage >= 1 ? fromPage : 1;
    toPage = toPage <= m_numberOfPages ? toPage : m_numberOfPages;

    for(int index = fromPage - 1; index <= toPage - 1; ++index)
    {
        m_pages.at(index)->startRender(true);
    }
}

void PresentationView::on_pages_linkClicked(int page, qreal left, qreal top)
{
    Q_UNUSED(left);
    Q_UNUSED(top);

    page = page >= 1 ? page : 1;
    page = page <= m_numberOfPages ? page : m_numberOfPages;

    jumpToPage(page, true);
}

void PresentationView::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);

    prepareScene();
    prepareView();
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

void PresentationView::wheelEvent(QWheelEvent* event)
{
    if(event->modifiers() == DocumentView::rotateModifiers())
    {
        if(event->delta() > 0)
        {
            rotateLeft();
        }
        else
        {
            rotateRight();
        }

        event->accept();
        return;
    }
    else if(event->modifiers() == Qt::NoModifier)
    {
        if(event->delta() > 0 && m_currentPage != 1)
        {
            previousPage();

            event->accept();
            return;
        }
        else if(event->delta() < 0 && m_currentPage != m_numberOfPages)
        {
            nextPage();

            event->accept();
            return;
        }
    }

    QGraphicsView::wheelEvent(event);
}

void PresentationView::prepareScene()
{
    for(int index = 0; index < m_numberOfPages; ++index)
    {
        PageItem* page = m_pages.at(index);
        QSizeF size = page->size();

        qreal visibleWidth = viewport()->width();
        qreal visibleHeight = viewport()->height();

        qreal pageWidth = 0.0;
        qreal pageHeight = 0.0;

        switch(m_rotation)
        {
        default:
        case RotateBy0:
        case RotateBy180:
            pageWidth = physicalDpiX() / 72.0 * size.width();
            pageHeight = physicalDpiY() / 72.0 * size.height();
            break;
        case RotateBy90:
        case RotateBy270:
            pageWidth = physicalDpiX() / 72.0 * size.height();
            pageHeight = physicalDpiY() / 72.0 * size.width();
            break;
        }

        qreal scaleFactor = qMin(visibleWidth / pageWidth, visibleHeight / pageHeight);

        page->setScaleFactor(scaleFactor);
        page->setRotation(m_rotation);
    }
}

void PresentationView::prepareView()
{
    for(int index = 0; index < m_pages.count(); ++index)
    {
        PageItem* page = m_pages.at(index);

        if(index == m_currentPage - 1)
        {
            page->setVisible(true);

            setSceneRect(page->boundingRect().translated(page->pos()));
        }
        else
        {
            page->setVisible(false);

            page->cancelRender();
        }

    }

    viewport()->update();
}
