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

#include "pageobject.h"

#include "mainwindow.h"

PageObject::PageObject(DocumentModel *model, DocumentView *view, int index, QGraphicsItem *parent) : QGraphicsObject(parent),
    m_index(index),
    m_size(),
    m_links(),
    m_selection(),
    m_rubberBand(),
    m_render()
{
    m_model = model;
    m_view = view;

    m_size = m_model->pageSize(m_index);
    m_links = m_model->links(m_index);
    m_results = m_model->results(index);

    qreal scaleX = m_view->resolutionX() / 72.0;
    qreal scaleY = m_view->resolutionY() / 72.0;
    qreal width = m_size.width();
    qreal height = m_size.height();

    switch(m_view->rotation())
    {
    case DocumentView::RotateBy0:
        m_pageTransform = QTransform(1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
        m_linkTransform = QTransform(scaleX * width, 0.0, 0.0, scaleY * height, 0.0, 0.0);
        m_resultTransform = QTransform(scaleX, 0.0, 0.0, scaleY, 0.0, 0.0);

        break;
    case DocumentView::RotateBy90:
        m_pageTransform = QTransform(0.0, 1.0, -1.0, 0.0, scaleX * height, 0.0);
        m_linkTransform = QTransform(0.0, scaleY * width, -scaleX * height, 0.0, scaleX * height, 0.0);
        m_resultTransform = QTransform(0.0, scaleY, -scaleX, 0.0, scaleX * height, 0.0);

        break;
    case DocumentView::RotateBy180:
        m_pageTransform = QTransform(-1.0, 0.0, 0.0, -1.0, scaleX * width, scaleY * height);
        m_linkTransform = QTransform(-scaleX * width, 0.0, 0.0, -scaleY * height, scaleX * width, scaleY * height);
        m_resultTransform = QTransform(-scaleX, 0.0, 0.0, -scaleY, scaleX * width, scaleY * height);

        break;
    case DocumentView::RotateBy270:
        m_pageTransform = QTransform(0.0, -1.0, 1.0, 0.0, 0.0, scaleX * width);
        m_linkTransform = QTransform(0.0, -scaleY * width, scaleX * height, 0.0, 0.0, scaleY * width);
        m_resultTransform = QTransform(0.0, -scaleY, scaleX, 0.0, 0.0, scaleY * width);

        break;
    }

    connect(&m_render, SIGNAL(finished()), this, SLOT(updatePage()));
    connect(m_model, SIGNAL(resultsChanged(int)), this, SLOT(updateResults(int)));
    connect(m_view, SIGNAL(highlightAllChanged(bool)), this, SLOT(updateHighlights()));

    this->setAcceptHoverEvents(true);
}

PageObject::~PageObject()
{
    if(m_render.isRunning())
    {
        m_render.waitForFinished();
    }

    m_model->dropPage(m_index, m_view->resolutionX(), m_view->resolutionY());
}

int PageObject::index() const
{
    return m_index;
}

void PageObject::setIndex(const int &index)
{
    if(m_index != index)
    {
        m_index = index;

        emit indexChanged(m_index);
    }
}

const QTransform &PageObject::pageTransform() const
{
    return m_pageTransform;
}

const QTransform &PageObject::linkTransform() const
{
    return m_linkTransform;
}

const QTransform &PageObject::resultTransform() const
{
    return m_resultTransform;
}

QRectF PageObject::boundingRect() const
{
    QRectF result;

    qreal scaleX = m_view->resolutionX() / 72.0;
    qreal scaleY = m_view->resolutionY() / 72.0;
    qreal width = m_size.width();
    qreal height = m_size.height();

    switch(m_view->rotation())
    {
    case DocumentView::RotateBy0:
    case DocumentView::RotateBy180:
        result = QRectF(0.0, 0.0, qCeil(scaleX * width), qCeil(scaleY * height));

        break;
    case DocumentView::RotateBy90:
    case DocumentView::RotateBy270:
        result = QRectF(0.0, 0.0, qCeil(scaleX * height), qCeil(scaleY * width));

        break;
    }

    return result;
}

void PageObject::paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    // draw page

    painter->fillRect(boundingRect(), QBrush(Qt::white));

    QImage image;

    switch(m_view->rotation())
    {
    case DocumentView::RotateBy0:
    case DocumentView::RotateBy180:
        image = m_model->pullPage(m_index, m_view->resolutionX(), m_view->resolutionY());

        break;
    case DocumentView::RotateBy90:
    case DocumentView::RotateBy270:
        image = m_model->pullPage(m_index, m_view->resolutionY(), m_view->resolutionX());

        break;
    }

    if(!image.isNull())
    {
        painter->setTransform(m_pageTransform, true);
        painter->drawImage(QPointF(0.0, 0.0), image);
        painter->setTransform(m_pageTransform.inverted(), true);
    }
    else
    {
        if(!m_render.isRunning())
        {
            m_render.setFuture(QtConcurrent::run(this, &PageObject::render, false));
        }
    }

    painter->setPen(QPen(Qt::black));
    painter->drawRect(boundingRect());

    // draw links

    painter->setPen(QPen(QColor(255,0,0,127)));
    painter->setTransform(m_linkTransform, true);

    foreach(DocumentModel::Link link, m_links)
    {
        painter->drawRect(link.area);
    }

    painter->setTransform(m_linkTransform.inverted(), true);

    // draw highlights

    if(m_view->highlightAll())
    {
        painter->setTransform(m_resultTransform, true);

        foreach(QRectF result, m_results)
        {
            painter->fillRect(result.adjusted(-1.0, -1.0, 1.0, 1.0), QBrush(QColor(0,255,0,127)));
        }

        painter->setTransform(m_resultTransform.inverted(), true);
    }

    // draw selection

    if(!m_selection.isNull())
    {
        painter->fillRect(m_selection, QBrush(QColor(0,0,255,127)));
    }

    // draw rubber band

    if(!m_rubberBand.isNull())
    {
        QPen pen;
        pen.setColor(Qt::black);
        pen.setStyle(Qt::DashLine);
        painter->setPen(pen);

        painter->drawRect(m_rubberBand);
    }
}

void PageObject::render(bool prefetch)
{
    bool visible = false;

    QRectF pageRect = boundingRect().translated(pos());

    foreach(QGraphicsView *view, this->scene()->views())
    {
       QRectF viewRect = view->mapToScene(view->rect()).boundingRect();

       visible = visible || viewRect.intersects(pageRect);
    }

    if(visible || prefetch)
    {
       switch(m_view->rotation())
       {
       case DocumentView::RotateBy0:
       case DocumentView::RotateBy180:
           m_model->pushPage(m_index, m_view->resolutionX(), m_view->resolutionY());

           break;
       case DocumentView::RotateBy90:
       case DocumentView::RotateBy270:
           m_model->pushPage(m_index, m_view->resolutionY(), m_view->resolutionX());

           break;
       }
    }
}

void PageObject::prefetch()
{
    QImage image = m_model->pullPage(m_index, m_view->resolutionX(), m_view->resolutionY());

    if(image.isNull())
    {
        if(!m_render.isRunning())
        {
            m_render.setFuture(QtConcurrent::run(this, &PageObject::render, true));
        }
    }
}

void PageObject::updatePage()
{
    this->scene()->update(boundingRect().translated(pos()));
}

void PageObject::updateResults(int index)
{
    if(m_index == index)
    {
        m_results = m_model->results(index);

        this->updatePage();
    }
}

void PageObject::updateHighlights()
{
    if(!m_results.isEmpty())
    {
        this->updatePage();
    }
}

void PageObject::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        foreach(DocumentModel::Link link, m_links)
        {
            if(m_linkTransform.mapRect(link.area).contains(event->scenePos() - pos()))
            {
                return;
            }
        }

        m_rubberBand = QRectF(event->scenePos() - pos(), QSizeF());

        return;
    }
}

void PageObject::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        // rubber band

        if(!m_rubberBand.isNull())
        {
            m_selection = m_rubberBand.adjusted(-5.0, -5.0, 5.0, 5.0);
            m_rubberBand = QRectF();

            QString text = m_model->text(m_index, m_resultTransform.inverted().mapRect(m_selection));

            if(!text.isEmpty())
            {
                QApplication::clipboard()->setText(text);
            }

            this->updatePage();
            return;
        }

        // links

        foreach(DocumentModel::Link link, m_links)
        {
            if(m_linkTransform.mapRect(link.area).contains(event->scenePos() - pos()))
            {
                if(link.pageNumber != -1)
                {
                    if(link.filePath.isEmpty())
                    {
                        m_view->setCurrentPage(link.pageNumber, link.top);
                    }
                    else
                    {
                        if(DocumentModel::openExternalLinks())
                        {
                            MainWindow *mainWindow = qobject_cast<MainWindow*>(m_view->parent()->parent()->parent());

                            if(mainWindow)
                            {
                                mainWindow->openExternalLink(link.filePath, link.pageNumber, link.top, false);
                            }
                        }
                        else
                        {
                            QMessageBox::information(0, tr("Information"), tr("Opening external links is disabled in the settings."));
                        }
                    }
                }
                else if(!link.url.isEmpty())
                {
                    if(DocumentModel::openUrlLinks())
                    {
                        QDesktopServices::openUrl(QUrl(link.url));
                    }
                    else
                    {
                        QMessageBox::information(0, tr("Information"), tr("Opening links to URL is disabled in the settings."));
                    }
                }

                return;
            }
        }
    }
    else if(event->button() == Qt::MiddleButton)
    {
        // rubber band

        if(!m_selection.isNull())
        {
            m_selection = QRectF();

            this->updatePage();
            return;
        }

        // external links

        foreach(DocumentModel::Link link, m_links)
        {
            if(m_linkTransform.mapRect(link.area).contains(event->scenePos() - pos()))
            {
                if(link.pageNumber != -1)
                {
                    if(!link.filePath.isEmpty())
                    {
                        if(DocumentModel::openExternalLinks())
                        {
                            MainWindow *mainWindow = qobject_cast<MainWindow*>(m_view->parent()->parent()->parent());

                            if(mainWindow)
                            {
                                mainWindow->openExternalLink(link.filePath, link.pageNumber, link.top, true);
                            }
                        }
                        else
                        {
                            QMessageBox::information(0, tr("Information"), tr("Opening external links is disabled in the settings."));
                        }
                    }
                }

                return;
            }
        }
    }
}

void PageObject::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if(!m_rubberBand.isNull())
    {
        m_rubberBand.setBottomRight(event->scenePos() - pos());

        this->updatePage();
        return;
    }
}

void PageObject::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    QApplication::restoreOverrideCursor();

    foreach(DocumentModel::Link link, m_links)
    {
        if(m_linkTransform.mapRect(link.area).contains(event->scenePos() - pos()))
        {
            QApplication::setOverrideCursor(Qt::PointingHandCursor);

            if(link.pageNumber != -1)
            {
                QToolTip::showText(event->screenPos(), tr("Go to page %1.").arg(link.pageNumber));
            }
            else if(!link.url.isEmpty())
            {
                QToolTip::showText(event->screenPos(), tr("Open URL %1.").arg(link.url));
            }

            return;
        }
    }

    QToolTip::hideText();
}
