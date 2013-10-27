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

#ifndef DOCUMENTLAYOUT_H
#define DOCUMENTLAYOUT_H

#include <QMap>

#include "global.h"

class PageItem;

struct DocumentLayout
{
    virtual ~DocumentLayout() {}

    virtual LayoutMode layoutMode() const = 0;

    virtual int currentPageForPage(int page, int count) const = 0;

    virtual int leftIndexForIndex(int index, int count) const = 0;
    virtual int rightIndexForIndex(int index, int count) const = 0;

    virtual qreal visibleWidth(int viewportWidth) const = 0;

    virtual void prepareLayout(PageItem* page, int index, int count,
                               QMap< qreal, int >& heightToIndex, qreal& pageHeight,
                               qreal& left, qreal& right, qreal& height) = 0;
};

struct SinglePageLayout
{
    LayoutMode layoutMode() const { return SinglePageMode; }

    int currentPageForPage(int page, int count) const;

    int leftIndexForIndex(int index, int count) const;
    int rightIndexForIndex(int index, int count) const;

    qreal visibleWidth(int viewportWidth) const;

    void prepareLayout(PageItem* page, int index, int count,
                       QMap< qreal, int >& heightToIndex, qreal& pageHeight,
                       qreal& left, qreal& right, qreal& height);
};

struct MultiplePagesLayout
{
    LayoutMode layoutMode() const { return MultiplePagesMode; }

    int currentPageForPage(int page, int count) const;

    int leftIndexForIndex(int index, int count) const;
    int rightIndexForIndex(int index, int count) const;

    qreal visibleWidth(int viewportWidth) const;

    void prepareLayout(PageItem* page, int index, int count,
                       QMap< qreal, int >& heightToIndex, qreal& pageHeight,
                       qreal& left, qreal& right, qreal& height);
};

#endif // DOCUMENTLAYOUT_H
