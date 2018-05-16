/*

Copyright 2014-2015 Adam Reichold

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

#ifndef BOOKMARKMODEL_H
#define BOOKMARKMODEL_H

#include <QAbstractListModel>

#include <QDateTime>
#include <QHash>
#include <QVector>

namespace qpdfview
{

struct BookmarkItem
{
    int page;

    QString label;
    QString comment;
    QDateTime modified;

    BookmarkItem(int page = -1, const QString& label = QString(), const QString& comment = QString(), const QDateTime& modified = QDateTime::currentDateTime()) :
        page(page),
        label(label),
        comment(comment),
        modified(modified) {}

};

} // namespace qpdfview

Q_DECLARE_TYPEINFO(qpdfview::BookmarkItem, Q_MOVABLE_TYPE);

namespace qpdfview
{

class BookmarkModel : public QAbstractListModel
{
    Q_OBJECT

public:
    static BookmarkModel* fromPath(const QString& path, bool create = false);

    static QList< QString > paths();

    static void removePath(const QString& path);
    static void removeAllPaths();


    bool isEmpty() const { return m_bookmarks.isEmpty(); }

    void addBookmark(const BookmarkItem& bookmark);
    void removeBookmark(const BookmarkItem& bookmark);

    void findBookmark(BookmarkItem& bookmark) const;


    enum
    {
        PageRole = Qt::UserRole + 1,
        LabelRole,
        CommentRole,
        ModifiedRole
    };

    Qt::ItemFlags flags(const QModelIndex&) const;

    int columnCount(const QModelIndex& parent = QModelIndex()) const;
    int rowCount(const QModelIndex& parent = QModelIndex()) const;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

private:
    Q_DISABLE_COPY(BookmarkModel)

    static QHash< QString, BookmarkModel* > s_instances;
    BookmarkModel(QObject* parent = 0);

    QVector< BookmarkItem > m_bookmarks;

};

} // qpdfview

#endif // BOOKMARKMODEL_H
