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

#include <QKeyEvent>
#include <QShortcut>
#include <QTimer>

#include "settings.h"
#include "model.h"
#include "pageitem.h"
#include "documentview.h"

Settings* PresentationView::s_settings = 0;

PresentationView::PresentationView(const QVector< Model::Page* >& pages, QWidget* parent) : QGraphicsView(parent),
    m_prefetchTimer(0),
    m_numberOfPages(-1),
    m_currentPage(-1),
    m_rotation(RotateBy0),
    m_invertColors(false),
    m_past(),
    m_future(),
    m_pages(),
    m_pageItems()
{
    if(s_settings == 0)
    {
        s_settings = Settings::instance();
    }

    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    setWindowState(windowState() | Qt::WindowFullScreen);

    setFrameShape(QFrame::NoFrame);

    setAcceptDrops(false);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    new QShortcut(QKeySequence(Qt::ALT + Qt::Key_Left), this, SLOT(jumpBackward()));
    new QShortcut(QKeySequence(Qt::ALT + Qt::Key_Right), this, SLOT(jumpForward()));

    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Left), this, SLOT(rotateLeft()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Right), this, SLOT(rotateRight()));

    m_pages = pages;

    m_numberOfPages = m_pages.count();
    m_currentPage = 1;

    // pages

    setScene(new QGraphicsScene(this));

    m_pageItems.reserve(m_numberOfPages);

    for(int index = 0; index < m_numberOfPages; ++index)
    {
        PageItem* page = new PageItem(m_pages.at(index), index, true);

        page->setPhysicalDpi(physicalDpiX(), physicalDpiY());

        scene()->addItem(page);
        m_pageItems.append(page);

        connect(page, SIGNAL(linkClicked(int,qreal,qreal)), SLOT(on_pages_linkClicked(int,qreal,qreal)));
    }

    scene()->setBackgroundBrush(QBrush(s_settings->pageItem().paperColor()));

    // prefetch

    m_prefetchTimer = new QTimer(this);
    m_prefetchTimer->setInterval(250);
    m_prefetchTimer->setSingleShot(true);

    connect(this, SIGNAL(currentPageChanged(int)), m_prefetchTimer, SLOT(start()));
    connect(this, SIGNAL(rotationChanged(Rotation)), m_prefetchTimer, SLOT(start()));

    connect(m_prefetchTimer, SIGNAL(timeout()), SLOT(on_prefetch_timeout()));

    if(s_settings->documentView().prefetch())
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
    qDeleteAll(m_pageItems);
}

int PresentationView::numberOfPages() const
{
    return m_numberOfPages;
}

int PresentationView::currentPage() const
{
    return m_currentPage;
}

Rotation PresentationView::rotation() const
{
    return m_rotation;
}

void PresentationView::setRotation(Rotation rotation)
{
    if(m_rotation != rotation)
    {
        m_rotation = rotation;

        prepareScene();
        prepareView();

        emit rotationChanged(m_rotation);
    }
}

bool PresentationView::invertColors() const
{
    return m_invertColors;
}

void PresentationView::setInvertColors(bool invertColors)
{
    if(m_invertColors != invertColors)
    {
        m_invertColors = invertColors;

        foreach(PageItem* page, m_pageItems)
        {
            page->setInvertColors(m_invertColors);
        }

        QColor backgroundColor = s_settings->pageItem().paperColor();

        if(m_invertColors)
        {
            backgroundColor.setRgb(~backgroundColor.rgb());
        }

        scene()->setBackgroundBrush(QBrush(backgroundColor));

        emit invertColorsChanged(m_invertColors);
    }
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

void PresentationView::jumpToPage(int page, bool trackChange)
{
    if(m_currentPage != page && page >= 1 && page <= m_numberOfPages)
    {
        if(trackChange)
        {
            m_past.append(m_currentPage);
        }

        m_currentPage = page;

        prepareView();

        emit currentPageChanged(m_currentPage, trackChange);
    }
}

void PresentationView::jumpBackward()
{
    if(!m_past.isEmpty())
    {
        m_future.prepend(m_currentPage);

        jumpToPage(m_past.takeLast(), false);
    }
}

void PresentationView::jumpForward()
{
    if(!m_future.isEmpty())
    {
        m_past.append(m_currentPage);

        jumpToPage(m_future.takeFirst(), false);
    }
}

void PresentationView::rotateLeft()
{
    switch(m_rotation)
    {
    default:
    case RotateBy0:
        setRotation(RotateBy270);
        break;
    case RotateBy90:
        setRotation(RotateBy0);
        break;
    case RotateBy180:
        setRotation(RotateBy90);
        break;
    case RotateBy270:
        setRotation(RotateBy180);
        break;
    }
}

void PresentationView::rotateRight()
{
    switch(m_rotation)
    {
    default:
    case RotateBy0:
        setRotation(RotateBy90);
        break;
    case RotateBy90:
        setRotation(RotateBy180);
        break;
    case RotateBy180:
        setRotation(RotateBy270);
        break;
    case RotateBy270:
        setRotation(RotateBy0);
        break;
    }
}

void PresentationView::on_prefetch_timeout()
{
    int fromPage = m_currentPage, toPage = m_currentPage;

    fromPage -= s_settings->documentView().prefetchDistance() / 2;
    toPage += s_settings->documentView().prefetchDistance();

    fromPage = fromPage >= 1 ? fromPage : 1;
    toPage = toPage <= m_numberOfPages ? toPage : m_numberOfPages;

    for(int index = fromPage - 1; index <= toPage - 1; ++index)
    {
        m_pageItems.at(index)->startRender(true);
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
    case Qt::Key_F12:
    case Qt::Key_Escape:
        close();

        event->accept();
        return;
    }

    if(event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_I)
    {
        setInvertColors(!invertColors());

        event->accept();
        return;
    }

    QWidget::keyPressEvent(event);
}

void PresentationView::wheelEvent(QWheelEvent* event)
{
    if(event->modifiers() == s_settings->documentView().rotateModifiers())
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
        PageItem* page = m_pageItems.at(index);
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
    for(int index = 0; index < m_numberOfPages; ++index)
    {
        PageItem* page = m_pageItems.at(index);

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
