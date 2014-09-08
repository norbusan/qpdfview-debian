/*

Copyright 2014 Adam Reichold

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

#include "bookmarkmodel.h"

namespace
{

inline bool operator<(int page, const BookmarkItem& bookmark) { return page < bookmark.page; }
inline bool operator<(const BookmarkItem& bookmark, int page) { return bookmark.page < page; }

}

BookmarkModel::BookmarkModel(QObject* parent) : QAbstractListModel(parent),
    m_bookmarks()
{
}

void BookmarkModel::addBookmark(const BookmarkItem& bookmark)
{
    QList< BookmarkItem >::iterator at = qUpperBound(m_bookmarks.begin(), m_bookmarks.end(), bookmark.page);

    if(at != m_bookmarks.end())
    {
        *at = bookmark;
    }
    else
    {
        m_bookmarks.insert(at, bookmark);
    }
}

void BookmarkModel::removeBookmark(const BookmarkItem& bookmark)
{
    QList< BookmarkItem >::iterator at = qBinaryFind(m_bookmarks.begin(), m_bookmarks.end(), bookmark.page);

    if(at != m_bookmarks.end())
    {
        m_bookmarks.erase(at);
    }
}

void BookmarkModel::findBookmark(BookmarkItem& bookmark) const
{
    QList< BookmarkItem >::const_iterator at = qBinaryFind(m_bookmarks.constBegin(), m_bookmarks.constEnd(), bookmark.page);

    if(at != m_bookmarks.constEnd())
    {
        bookmark = *at;
    }
}

Qt::ItemFlags BookmarkModel::flags(const QModelIndex&) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

int BookmarkModel::columnCount(const QModelIndex&) const
{
    return 2;
}

int BookmarkModel::rowCount(const QModelIndex& parent) const
{
    return !parent.isValid() ? m_bookmarks.count() : 0;
}

QVariant BookmarkModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid() || index.row() < 0 || index.row() >= m_bookmarks.count())
    {
        return QVariant();
    }

    const BookmarkItem& bookmark = m_bookmarks.at(index.row());

    switch(role)
    {
    default:
        return QVariant();
    case PageRole:
        return bookmark.page;
    case LabelRole:
        return bookmark.label;
    case CommentRole:
        return bookmark.comment;
    case ModifiedRole:
        return bookmark.modified;
    case Qt::TextAlignmentRole:
        return index.column() == 0 ? Qt::AlignLeft : Qt::AlignRight;
    }
}
