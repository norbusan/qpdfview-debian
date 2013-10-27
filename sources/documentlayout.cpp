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

#include "pageitem.h"

int SinglePageLayout::currentPageForPage(int page, int count) const
{
    Q_UNUSED(count);

    return page;
}

int SinglePageLayout::leftIndexForIndex(int index, int count) const
{
    Q_UNUSED(count);

    return index;
}

int SinglePageLayout::rightIndexForIndex(int index, int count) const
{
    Q_UNUSED(count);

    return index;
}

qreal SinglePageLayout::visibleWidth(int viewportWidth, qreal pageSpacing) const
{
    return viewportWidth - 6.0 - 2.0 * pageSpacing;
}

void SinglePageLayout::prepareLayout(PageItem* page, int index, qreal pageSpacing,
                                     QMap< qreal, int >& heightToIndex, qreal& pageHeight,
                                     qreal& left, qreal& right, qreal& height)
{
    const QRectF boundingRect = page->boundingRect();

    page->setPos(-boundingRect.left() - 0.5 * boundingRect.width(), height - boundingRect.top());

    heightToIndex.insert(-height + pageSpacing + 0.3 * pageHeight, index);

    pageHeight = boundingRect.height();

    left = qMin(left, -0.5f * boundingRect.width() - pageSpacing);
    right = qMax(right, 0.5f * boundingRect.width() + pageSpacing);
    height += pageHeight + pageSpacing;
}
