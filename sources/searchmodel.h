/*

Copyright 2014 S. Razi Alavizadeh
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

#ifndef SEARCHMODEL_H
#define SEARCHMODEL_H

#include <QAbstractItemModel>


namespace qpdfview
{

class DocumentView;

class SearchModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit SearchModel(QObject* parent = 0);
    ~SearchModel();

    static SearchModel* instance();

    void clear();

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex& child) const;

    int rowCount(DocumentView* document) const;
    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    int columnCount(const QModelIndex& parent = QModelIndex()) const;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

    void prependResults(DocumentView* document, int page, const QList< QRectF >& resultsOnPage);
    void clearResultOf(DocumentView* document);

    bool isOccurrenceOnPage(DocumentView* document, int page) const;

private:
    static SearchModel* s_instance;

    typedef QPair< int, QRectF > Result;
    typedef QVector< Result > Results;

    QHash< DocumentView*, Results* > m_results;

    QHash< DocumentView*, QPersistentModelIndex > m_topLevelIndices;

};

} // qpdfview

#endif // SEARCHMODEL_H
