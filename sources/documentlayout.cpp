/*

Copyright 2013 Adam Reichold

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

#include "documentlayout.h"

#include "settings.h"
#include "pageitem.h"

Settings* DocumentLayout::s_settings = 0;

DocumentLayout::DocumentLayout()
{
    if(s_settings == 0)
    {
        s_settings = Settings::instance();
    }
}

qreal DocumentLayout::visibleHeight(int viewportHeight) const
{
    const qreal pageSpacing = s_settings->documentView().pageSpacing();

    return viewportHeight - 2.0 * pageSpacing;
}


int SinglePageLayout::currentPage(int page) const
{
    return page;
}

int SinglePageLayout::previousPage(int page) const
{
    return qMax(page - 1, 1);
}

int SinglePageLayout::nextPage(int page, int count) const
{
    return qMin(page + 1, count);
}

QPair< int, int > SinglePageLayout::prefetchRange(int page, int count) const
{
    const int prefetchDistance = s_settings->documentView().prefetchDistance();

    return qMakePair(qMax(page - prefetchDistance / 2, 1),
                     qMin(page + prefetchDistance, count));
}

int SinglePageLayout::leftIndex(int index) const
{
    return index;
}

int SinglePageLayout::rightIndex(int index, int count) const
{
    Q_UNUSED(count);

    return index;
}

qreal SinglePageLayout::visibleWidth(int viewportWidth) const
{
    const qreal pageSpacing = s_settings->documentView().pageSpacing();

    return viewportWidth - 6.0 - 2.0 * pageSpacing;
}

void SinglePageLayout::prepareLayout(const QVector<PageItem *>& pageItems,
                                     QMap< qreal, int >& heightToIndex, qreal& pageHeight,
                                     qreal& left, qreal& right, qreal& height)
{
    const qreal pageSpacing = s_settings->documentView().pageSpacing();

    for(int index = 0; index < pageItems.count(); ++index)
    {
        PageItem* page = pageItems.at(index);
        const QRectF boundingRect = page->boundingRect();

        page->setPos(-boundingRect.left() - 0.5 * boundingRect.width(), height - boundingRect.top());

        heightToIndex.insert(-height + pageSpacing + 0.3 * pageHeight, index);

        pageHeight = boundingRect.height();

        left = qMin(left, -0.5f * boundingRect.width() - pageSpacing);
        right = qMax(right, 0.5f * boundingRect.width() + pageSpacing);
        height += pageHeight + pageSpacing;
    }
}


int TwoPagesLayout::currentPage(int page) const
{
    return page % 2 != 0 ? page : page - 1;
}

int TwoPagesLayout::previousPage(int page) const
{
    return qMax(page - 2, 1);
}

int TwoPagesLayout::nextPage(int page, int count) const
{
    return qMin(page + 2, count);
}

QPair< int, int > TwoPagesLayout::prefetchRange(int page, int count) const
{
    const int prefetchDistance = s_settings->documentView().prefetchDistance();

    return qMakePair(qMax(page - prefetchDistance, 1),
                     qMin(page + 2 * prefetchDistance + 1, count));
}

int TwoPagesLayout::leftIndex(int index) const
{
    return index % 2 == 0 ? index : index - 1;
}

int TwoPagesLayout::rightIndex(int index, int count) const
{
    return qMin(index % 2 == 0 ? index + 1 : index, count - 1);
}

qreal TwoPagesLayout::visibleWidth(int viewportWidth) const
{
    const qreal pageSpacing = s_settings->documentView().pageSpacing();

    return (viewportWidth - 6.0 - 3 * pageSpacing) / 2;
}

void TwoPagesLayout::prepareLayout(const QVector<PageItem *>& pageItems,
                                   QMap< qreal, int >& heightToIndex, qreal& pageHeight,
                                   qreal& left, qreal& right, qreal& height)
{
    const qreal pageSpacing = s_settings->documentView().pageSpacing();

    for(int index = 0; index < pageItems.count(); ++index)
    {
        PageItem* page = pageItems.at(index);
        const QRectF boundingRect = page->boundingRect();

        if(index == leftIndex(index))
        {
            page->setPos(-boundingRect.left() - boundingRect.width() - 0.5 * pageSpacing, height - boundingRect.top());

            heightToIndex.insert(-height + pageSpacing + 0.3 * pageHeight, index);

            pageHeight = boundingRect.height();

            left = qMin(left, -boundingRect.width() - 1.5f * pageSpacing);

            if(index == rightIndex(index, pageItems.count()))
            {
                right = qMax(right, 0.5f * pageSpacing);
                height += pageHeight + pageSpacing;
            }
        }
        else
        {
            page->setPos(-boundingRect.left() + 0.5 * pageSpacing, height - boundingRect.top());

            pageHeight = qMax(pageHeight, boundingRect.height());

            right = qMax(right, boundingRect.width() + 1.5f * pageSpacing);
            height += pageHeight + pageSpacing;
        }
    }
}


int TwoPagesWithCoverPageLayout::currentPage(int page) const
{
    return page == 1 ? page : (page % 2 == 0 ? page : page - 1);
}

int TwoPagesWithCoverPageLayout::leftIndex(int index) const
{
    return index == 0 ? index : (index % 2 != 0 ? index : index - 1);
}

int TwoPagesWithCoverPageLayout::rightIndex(int index, int count) const
{
    return qMin(index % 2 != 0 ? index + 1 : index, count - 1);
}


int MultiplePagesLayout::currentPage(int page) const
{
    const int pagesPerRow = s_settings->documentView().pagesPerRow();

    return page - ((page - 1) % pagesPerRow);
}

int MultiplePagesLayout::previousPage(int page) const
{
    const int pagesPerRow = s_settings->documentView().pagesPerRow();

    return qMax(page - pagesPerRow, 1);
}

int MultiplePagesLayout::nextPage(int page, int count) const
{
    const int pagesPerRow = s_settings->documentView().pagesPerRow();

    return qMin(page + pagesPerRow, count);
}

QPair<int, int> MultiplePagesLayout::prefetchRange(int page, int count) const
{
    const int prefetchDistance = s_settings->documentView().prefetchDistance();
    const int pagesPerRow = s_settings->documentView().pagesPerRow();

    return qMakePair(qMax(page - pagesPerRow * (prefetchDistance / 2), 1),
                     qMin(page + pagesPerRow * (prefetchDistance + 1) - 1, count));
}

int MultiplePagesLayout::leftIndex(int index) const
{
    const int pagesPerRow = s_settings->documentView().pagesPerRow();

    return index - (index % pagesPerRow);
}

int MultiplePagesLayout::rightIndex(int index, int count) const
{
    const int pagesPerRow = s_settings->documentView().pagesPerRow();

    return qMin(index - (index % pagesPerRow) + pagesPerRow - 1, count - 1);
}

qreal MultiplePagesLayout::visibleWidth(int viewportWidth) const
{
    const qreal pageSpacing = s_settings->documentView().pageSpacing();
    const int pagesPerRow = s_settings->documentView().pagesPerRow();

    return (viewportWidth - 6.0 - (pagesPerRow + 1) * pageSpacing) / pagesPerRow;
}

void MultiplePagesLayout::prepareLayout(const QVector< PageItem* >& pageItems,
                                        QMap< qreal, int >& heightToIndex, qreal& pageHeight,
                                        qreal& left, qreal& right, qreal& height)
{
    const qreal pageSpacing = s_settings->documentView().pageSpacing();

    for(int index = 0; index < pageItems.count(); ++index)
    {
        PageItem* page = pageItems.at(index);
        const QRectF boundingRect = page->boundingRect();

        page->setPos(left - boundingRect.left() + pageSpacing, height - boundingRect.top());

        pageHeight = qMax(pageHeight, boundingRect.height());
        left += boundingRect.width() + pageSpacing;

        if(index == leftIndex(index))
        {
            heightToIndex.insert(-height + pageSpacing + 0.3 * pageHeight, index);
        }

        if(index == rightIndex(index, pageItems.count()))
        {
            height += pageHeight + pageSpacing;
            pageHeight = 0.0;

            right = qMax(right, left + pageSpacing);
            left = 0.0;
        }
    }
}
