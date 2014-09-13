/*

Copyright 2014 S. Razi Alavizadeh
Copyright 2013-2014 Adam Reichold

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
#include <QPair>

#include "global.h"

class QRectF;

namespace qpdfview
{

class Settings;
class PageItem;

struct DocumentLayout
{
    DocumentLayout();
    virtual ~DocumentLayout() {}

    static DocumentLayout* fromLayoutMode(LayoutMode layoutMode);

    virtual LayoutMode layoutMode() const = 0;

    virtual int currentPage(int page) const = 0;
    virtual int previousPage(int page) const = 0;
    virtual int nextPage(int page, int count) const = 0;

    bool isCurrentPage(const QRectF& visibleRect, const QRectF& pageRect) const;

    virtual QPair< int, int > prefetchRange(int page, int count) const = 0;

    virtual int leftIndex(int index) const = 0;
    virtual int rightIndex(int index, int count) const = 0;

    virtual qreal visibleWidth(int viewportWidth) const = 0;
    qreal visibleHeight(int viewportHeight) const;

    virtual void prepareLayout(const QVector< PageItem* >& pageItems, bool rightToLeft,
                               qreal& left, qreal& right, qreal& height) = 0;

protected:
    static Settings* s_settings;

};

struct SinglePageLayout : public DocumentLayout
{
    LayoutMode layoutMode() const { return SinglePageMode; }

    int currentPage(int page) const;
    int previousPage(int page) const;
    int nextPage(int page, int count) const;

    QPair< int, int > prefetchRange(int page, int count) const;

    int leftIndex(int index) const;
    int rightIndex(int index, int count) const;

    qreal visibleWidth(int viewportWidth) const;

    void prepareLayout(const QVector< PageItem* >& pageItems, bool rightToLeft,
                       qreal& left, qreal& right, qreal& height);

};

struct TwoPagesLayout : public DocumentLayout
{
    LayoutMode layoutMode() const { return TwoPagesMode; }

    int currentPage(int page) const;
    int previousPage(int page) const;
    int nextPage(int page, int count) const;

    QPair< int, int > prefetchRange(int page, int count) const;

    int leftIndex(int index) const;
    int rightIndex(int index, int count) const;

    qreal visibleWidth(int viewportWidth) const;

    void prepareLayout(const QVector< PageItem* >& pageItems, bool rightToLeft,
                       qreal& left, qreal& right, qreal& height);

};

struct TwoPagesWithCoverPageLayout : public TwoPagesLayout
{
    LayoutMode layoutMode() const { return TwoPagesWithCoverPageMode; }

    int currentPage(int page) const;

    int leftIndex(int index) const;
    int rightIndex(int index, int count) const;

};

struct MultiplePagesLayout : public DocumentLayout
{
    LayoutMode layoutMode() const { return MultiplePagesMode; }

    int currentPage(int page) const;
    int previousPage(int page) const;
    int nextPage(int page, int count) const;

    QPair< int, int > prefetchRange(int page, int count) const;

    int leftIndex(int index) const;
    int rightIndex(int index, int count) const;

    qreal visibleWidth(int viewportWidth) const;

    void prepareLayout(const QVector< PageItem* >& pageItems, bool rightToLeft,
                       qreal& left, qreal& right, qreal& height);

};

} // qpdfview

#endif // DOCUMENTLAYOUT_H
