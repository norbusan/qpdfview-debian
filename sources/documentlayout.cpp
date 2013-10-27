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

int SinglePageLayout::currentPageForPage(int page) const
{
    return page;
}

int SinglePageLayout::leftIndexForIndex(int index) const
{
    return index;
}

int SinglePageLayout::rightIndexForIndex(int index, int count) const
{
    Q_UNUSED(count);

    return index;
}

qreal SinglePageLayout::visibleWidth(int viewportWidth) const
{
    const qreal pageSpacing = Settings::instance()->documentView().pageSpacing();

    return viewportWidth - 6.0 - 2.0 * pageSpacing;
}

void SinglePageLayout::prepareLayout(PageItem* page, int index, int count,
                                     QMap< qreal, int >& heightToIndex, qreal& pageHeight,
                                     qreal& left, qreal& right, qreal& height)
{
    Q_UNUSED(count);

    const qreal pageSpacing = Settings::instance()->documentView().pageSpacing();
    const QRectF boundingRect = page->boundingRect();

    page->setPos(-boundingRect.left() - 0.5 * boundingRect.width(), height - boundingRect.top());

    heightToIndex.insert(-height + pageSpacing + 0.3 * pageHeight, index);

    pageHeight = boundingRect.height();

    left = qMin(left, -0.5f * boundingRect.width() - pageSpacing);
    right = qMax(right, 0.5f * boundingRect.width() + pageSpacing);
    height += pageHeight + pageSpacing;
}


int TwoPagesLayout::currentPageForPage(int page) const
{
    return page % 2 != 0 ? page : page - 1;
}

int TwoPagesLayout::leftIndexForIndex(int index) const
{
    return index % 2 == 0 ? index : index - 1;
}

int TwoPagesLayout::rightIndexForIndex(int index, int count) const
{
    return qMin(index % 2 == 0 ? index + 1 : index, count - 1);
}

qreal TwoPagesLayout::visibleWidth(int viewportWidth) const
{
    const qreal pageSpacing = Settings::instance()->documentView().pageSpacing();

    return (viewportWidth - 6.0 - 3 * pageSpacing) / 2;
}

void TwoPagesLayout::prepareLayout(PageItem* page, int index, int count,
                                   QMap< qreal, int >& heightToIndex, qreal& pageHeight,
                                   qreal& left, qreal& right, qreal& height)
{
    const qreal pageSpacing = Settings::instance()->documentView().pageSpacing();
    const QRectF boundingRect = page->boundingRect();

    if(index == leftIndexForIndex(index))
    {
        page->setPos(-boundingRect.left() - boundingRect.width() - 0.5 * pageSpacing, height - boundingRect.top());

        heightToIndex.insert(-height + pageSpacing + 0.3 * pageHeight, index);

        pageHeight = boundingRect.height();

        left = qMin(left, -boundingRect.width() - 1.5f * pageSpacing);

        if(index == rightIndexForIndex(index, count))
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


int TwoPagesWithCoverPageLayout::currentPageForPage(int page) const
{
    return page == 1 ? page : (page % 2 == 0 ? page : page - 1);
}

int TwoPagesWithCoverPageLayout::leftIndexForIndex(int index) const
{
    return index == 0 ? index : (index % 2 != 0 ? index : index - 1);
}

int TwoPagesWithCoverPageLayout::rightIndexForIndex(int index, int count) const
{
    return qMin(index % 2 != 0 ? index + 1 : index, count - 1);
}


int MultiplePagesLayout::currentPageForPage(int page) const
{
    const int pagesPerRow = Settings::instance()->documentView().pagesPerRow();

    return page - ((page - 1) % pagesPerRow);
}

int MultiplePagesLayout::leftIndexForIndex(int index) const
{
    const int pagesPerRow = Settings::instance()->documentView().pagesPerRow();

    return index - (index % pagesPerRow);
}

int MultiplePagesLayout::rightIndexForIndex(int index, int count) const
{
    const int pagesPerRow = Settings::instance()->documentView().pagesPerRow();

    return qMin(index - (index % pagesPerRow) + pagesPerRow - 1, count - 1);
}

qreal MultiplePagesLayout::visibleWidth(int viewportWidth) const
{
    const qreal pageSpacing = Settings::instance()->documentView().pageSpacing();
    const int pagesPerRow = Settings::instance()->documentView().pagesPerRow();

    return (viewportWidth - 6.0 - (pagesPerRow + 1) * pageSpacing) / pagesPerRow;
}

void MultiplePagesLayout::prepareLayout(PageItem* page, int index, int count,
                                        QMap< qreal, int >& heightToIndex, qreal& pageHeight,
                                        qreal& left, qreal& right, qreal& height)
{
    const qreal pageSpacing = Settings::instance()->documentView().pageSpacing();
    const QRectF boundingRect = page->boundingRect();

    page->setPos(left - boundingRect.left() + pageSpacing, height - boundingRect.top());

    pageHeight = qMax(pageHeight, boundingRect.height());
    left += boundingRect.width() + pageSpacing;

    if(index == leftIndexForIndex(index))
    {
        heightToIndex.insert(-height + pageSpacing + 0.3 * pageHeight, index);
    }

    if(index == rightIndexForIndex(index, count))
    {
        height += pageHeight + pageSpacing;
        pageHeight = 0.0;

        right = qMax(right, left + pageSpacing);
        left = 0.0;
    }
}
