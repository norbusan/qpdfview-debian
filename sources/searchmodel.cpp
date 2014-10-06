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

#include "searchmodel.h"
#include "documentview.h"

#include <QApplication>
#include <QVector>
#include <QRectF>

namespace
{

using namespace qpdfview;

inline bool operator<(int page, const QPair< int, QRectF >& result) { return page < result.first; }
inline bool operator<(const QPair< int, QRectF >& result, int page) { return result.first < page; }

}

namespace qpdfview
{

SearchModel* SearchModel::s_instance = 0;

SearchModel::SearchModel(QObject* parent) : QAbstractItemModel(parent)
{
}

SearchModel::~SearchModel()
{
    clear();

    s_instance = 0;
}

SearchModel* SearchModel::instance()
{
    if(s_instance == 0)
    {
        s_instance = new SearchModel(qApp);
    }

    return s_instance;
}

void SearchModel::clear()
{
    beginResetModel();

    qDeleteAll(m_results);

    m_results.clear();
    m_topLevelIndices.clear();

    endResetModel();
}

QModelIndex SearchModel::index(int row, int column, const QModelIndex& parent) const
{
    if(hasIndex(row, column, parent))
    {
        return createIndex(row, column, m_topLevelIndices.key(parent, 0));
    }

    return QModelIndex();
}

QModelIndex SearchModel::parent(const QModelIndex& child) const
{
    return m_topLevelIndices.value(static_cast< DocumentView* >(child.internalPointer()), QModelIndex());
}

int SearchModel::rowCount(DocumentView* document) const
{
    const Results* results = m_results.value(document, 0);

    if(results != 0)
    {
        return results->count();
    }

    return 0;
}

int SearchModel::rowCount(const QModelIndex& parent) const
{
    if(!parent.isValid())
    {
        return m_topLevelIndices.count();
    }
    else
    {
        return rowCount(m_topLevelIndices.key(parent, 0));
    }

    return 0;
}

int SearchModel::columnCount(const QModelIndex&) const
{
    return 1;
}

QVariant SearchModel::data(const QModelIndex& index, int role) const
{
    Q_UNUSED(index);
    Q_UNUSED(role);

    return QVariant();
}

void SearchModel::prependResults(DocumentView* document, int page, const QList< QRectF >& results)
{
    if(document == 0 || results.isEmpty())
    {
        return;
    }

    if(!m_topLevelIndices.contains(document))
    {
        beginInsertRows(QModelIndex(), rowCount(), rowCount());

        m_topLevelIndices.insert(document, QPersistentModelIndex(createIndex(rowCount(), 0)));
        m_results.insert(document, new Results);

        endInsertRows();
    }

    Results* documentResults = m_results.value(document);

    Results::iterator at = qLowerBound(documentResults->begin(), documentResults->end(), page);

    int row = at - documentResults->begin();

    beginInsertRows(m_topLevelIndices.value(document), row, row + results.size() - 1);

    for(int i = results.size() - 1; i >= 0; --i)
    {
        at = documentResults->insert(at, qMakePair(page, results.at(i)));
    }

    endInsertRows();
}

void SearchModel::clearResultOf(DocumentView* document)
{
    if(!m_topLevelIndices.contains(document))
    {
        return;
    }

    const int row = m_topLevelIndices.value(document).row();

    beginRemoveRows(QModelIndex(), row, row);

    delete m_results.take(document);
    m_topLevelIndices.remove(document);

    endRemoveRows();
}

bool SearchModel::isOccurrenceOnPage(DocumentView* document, int page) const
{
    const Results* results = m_results.value(document, 0);

    if(results != 0)
    {
        return qBinaryFind(results->begin(), results->end(), page) != results->end();
    }

    return false;
}

QList< QRectF > SearchModel::resultsRecsOf(DocumentView* document, int page) const
{
    QList< QRectF > list;
    Results* documentResults = m_results.value(document, 0);

    if (documentResults != 0)
    {
        Results::iterator i = qLowerBound(documentResults->begin(), documentResults->end(), page);
        Results::iterator end = qUpperBound(i, documentResults->end(), page);

        list.reserve(end - i);

        while (i != end)
        {
            list << (*i).second;

            ++i;
        }
    }

    return list;
}

} // qpdfview
