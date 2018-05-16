/*

Copyright 2014 S. Razi Alavizadeh
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

#include "searchmodel.h"

#include <QApplication>
#include <QtConcurrentRun>

#include "documentview.h"

static inline bool operator<(int page, const QPair< int, QRectF >& result) { return page < result.first; }
static inline bool operator<(const QPair< int, QRectF >& result, int page) { return result.first < page; }

namespace qpdfview
{

SearchModel* SearchModel::s_instance = 0;

SearchModel* SearchModel::instance()
{
    if(s_instance == 0)
    {
        s_instance = new SearchModel(qApp);
    }

    return s_instance;
}

SearchModel::~SearchModel()
{
    foreach(TextWatcher* watcher, m_textWatchers)
    {
        watcher->waitForFinished();
        watcher->deleteLater();
    }

    qDeleteAll(m_results);

    s_instance = 0;
}

QModelIndex SearchModel::index(int row, int column, const QModelIndex& parent) const
{
    if(hasIndex(row, column, parent))
    {
        if(!parent.isValid())
        {
            return createIndex(row, column);
        }
        else
        {
            DocumentView* view = m_views.value(parent.row(), 0);

            return createIndex(row, column, view);
        }
    }

    return QModelIndex();
}

QModelIndex SearchModel::parent(const QModelIndex& child) const
{
    if(child.internalPointer() != 0)
    {
        DocumentView* view = static_cast< DocumentView* >(child.internalPointer());

        return findView(view);
    }

    return QModelIndex();
}

int SearchModel::rowCount(const QModelIndex& parent) const
{
    if(!parent.isValid())
    {
        return m_views.count();
    }
    else if(parent.internalPointer() == 0)
    {
        DocumentView* view = m_views.value(parent.row(), 0);
        const Results* results = m_results.value(view, 0);

        if(results != 0)
        {
            return results->count();
        }
    }

    return 0;
}

int SearchModel::columnCount(const QModelIndex&) const
{
    return 2;
}

QVariant SearchModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid())
    {
        return QVariant();
    }

    if(index.internalPointer() == 0)
    {
        DocumentView* view = m_views.value(index.row(), 0);
        const Results* results = m_results.value(view, 0);

        if(results == 0)
        {
            return QVariant();
        }

        switch(role)
        {
        default:
            return QVariant();
        case CountRole:
            return results->count();
        case ProgressRole:
            return view->searchProgress();
        case Qt::DisplayRole:
            switch(index.column())
            {
            case 0:
                return view->title();
            case 1:
                return results->count();
            }
        case Qt::ToolTipRole:
            return tr("<b>%1</b> occurrences").arg(results->count());
        }
    }
    else
    {
        DocumentView* view = static_cast< DocumentView* >(index.internalPointer());
        const Results* results = m_results.value(view, 0);

        if(results == 0 || index.row() >= results->count())
        {
            return QVariant();
        }

        const Result& result = results->at(index.row());

        switch(role)
        {
        default:
            return QVariant();
        case PageRole:
            return result.first;
        case RectRole:
            return result.second;
        case TextRole:
            return view->searchText();
        case MatchCaseRole:
            return view->searchMatchCase();
        case WholeWordsRole:
            return view->searchWholeWords();
        case MatchedTextRole:
            return fetchMatchedText(view, result);
        case SurroundingTextRole:
            return fetchSurroundingText(view, result);
        case Qt::DisplayRole:
            switch(index.column())
            {
            case 0:
                return QVariant();
            case 1:
                return result.first;
            }
        case Qt::ToolTipRole:
            return tr("<b>%1</b> occurrences on page <b>%2</b>").arg(numberOfResultsOnPage(view, result.first)).arg(result.first);
        }
    }

    return QVariant();
}

DocumentView* SearchModel::viewForIndex(const QModelIndex& index) const
{
    if(index.internalPointer() == 0)
    {
        return m_views.value(index.row(), 0);
    }
    else
    {
        return static_cast< DocumentView* >(index.internalPointer());
    }
}

bool SearchModel::hasResults(DocumentView* view) const
{
    const Results* results = m_results.value(view, 0);

    return results != 0 && !results->isEmpty();
}

bool SearchModel::hasResultsOnPage(DocumentView* view, int page) const
{
    const Results* results = m_results.value(view, 0);

    return results != 0 && qBinaryFind(results->begin(), results->end(), page) != results->end();
}

int SearchModel::numberOfResultsOnPage(DocumentView* view, int page) const
{
    const Results* results = m_results.value(view, 0);

    if(results == 0)
    {
        return 0;
    }

    const Results::const_iterator pageBegin = qLowerBound(results->constBegin(), results->constEnd(), page);
    const Results::const_iterator pageEnd = qUpperBound(pageBegin, results->constEnd(), page);

    return pageEnd - pageBegin;
}

QList< QRectF > SearchModel::resultsOnPage(DocumentView* view, int page) const
{
    QList< QRectF > resultsOnPage;

    const Results* results = m_results.value(view, 0);

    if(results != 0)
    {
        const Results::const_iterator pageBegin = qLowerBound(results->constBegin(), results->constEnd(), page);
        const Results::const_iterator pageEnd = qUpperBound(pageBegin, results->constEnd(), page);

        for(Results::const_iterator iterator = pageBegin; iterator != pageEnd; ++iterator)
        {
            resultsOnPage.append(iterator->second);
        }
    }

    return resultsOnPage;
}

QPersistentModelIndex SearchModel::findResult(DocumentView* view, const QPersistentModelIndex& currentResult, int currentPage, FindDirection direction) const
{
    const Results* results = m_results.value(view, 0);

    if(results == 0 || results->isEmpty())
    {
        return QPersistentModelIndex();
    }

    const int rows = results->count();
    int row;

    if(currentResult.isValid())
    {
        switch(direction)
        {
        default:
        case FindNext:
            row = (currentResult.row() + 1) % rows;
            break;
        case FindPrevious:
            row = (currentResult.row() + rows - 1) % rows;
            break;
        }
    }
    else
    {
        switch(direction)
        {
        default:
        case FindNext:
        {
            Results::const_iterator lowerBound = qLowerBound(results->constBegin(), results->constEnd(), currentPage);

            row = (lowerBound - results->constBegin()) % rows;
            break;
        }
        case FindPrevious:
        {
            Results::const_iterator upperBound = qUpperBound(results->constBegin(), results->constEnd(), currentPage);

            row = ((upperBound - results->constBegin()) + rows - 1) % rows;
            break;
        }
        }
    }

    return createIndex(row, 0, view);
}

void SearchModel::insertResults(DocumentView* view, int page, const QList< QRectF >& resultsOnPage)
{
    if(resultsOnPage.isEmpty())
    {
        return;
    }

    const QModelIndex parent = findOrInsertView(view);

    Results* results = m_results.value(view);

    Results::iterator at = qLowerBound(results->begin(), results->end(), page);
    const int row = at - results->begin();

    beginInsertRows(parent, row, row + resultsOnPage.size() - 1);

    for(int index = resultsOnPage.size() - 1; index >= 0; --index)
    {
        at = results->insert(at, qMakePair(page, resultsOnPage.at(index)));
    }

    endInsertRows();
}

void SearchModel::clearResults(DocumentView* view)
{
    typedef QHash< TextCacheKey, TextWatcher* >::iterator WatcherIterator;

    for(WatcherIterator iterator = m_textWatchers.begin(); iterator != m_textWatchers.end(); ++iterator)
    {
        const TextCacheKey& key = iterator.key();

        if(key.first == view)
        {
            TextWatcher* const watcher = iterator.value();
            watcher->cancel();
            watcher->waitForFinished();
            watcher->deleteLater();

            iterator = m_textWatchers.erase(iterator);
            continue;
        }
    }

    foreach(const TextCacheKey& key, m_textCache.keys())
    {
        if(key.first == view)
        {
            m_textCache.remove(key);
        }
    }

    const QVector< DocumentView* >::iterator at = qBinaryFind(m_views.begin(), m_views.end(), view);
    const int row = at - m_views.begin();

    if(at == m_views.end())
    {
        return;
    }

    beginRemoveRows(QModelIndex(), row, row);

    m_views.erase(at);
    delete m_results.take(view);

    endRemoveRows();
}

void SearchModel::updateProgress(DocumentView* view)
{
    QModelIndex index = findView(view);

    if(index.isValid())
    {
        emit dataChanged(index, index);
    }
}

void SearchModel::on_fetchSurroundingText_finished()
{
    TextWatcher* watcher = dynamic_cast< TextWatcher* >(sender());

    if(watcher == 0 || watcher->isCanceled())
    {
        return;
    }

    const TextJob job = watcher->result();

    m_textWatchers.remove(job.key);
    watcher->deleteLater();

    DocumentView* view = job.key.first;
    const Results* results = m_results.value(view, 0);

    if(results == 0)
    {
        return;
    }

    const int cost = job.object->first.length() + job.object->second.length();

    m_textCache.insert(job.key, job.object, cost);

    emit dataChanged(createIndex(0, 0, view), createIndex(results->count() - 1, 0, view));
}

SearchModel::SearchModel(QObject* parent) : QAbstractItemModel(parent),
    m_views(),
    m_results(),
    m_textCache(1 << 16),
    m_textWatchers()
{
}

QModelIndex SearchModel::findView(DocumentView *view) const
{
    const QVector< DocumentView* >::const_iterator at = qBinaryFind(m_views.constBegin(), m_views.constEnd(), view);
    const int row = at - m_views.constBegin();

    if(at == m_views.constEnd())
    {
        return QModelIndex();
    }

    return createIndex(row, 0);
}

QModelIndex SearchModel::findOrInsertView(DocumentView* view)
{
    const QVector< DocumentView* >::iterator at = qLowerBound(m_views.begin(), m_views.end(), view);
    const int row = at - m_views.begin();

    if(at == m_views.end() || *at != view)
    {
        beginInsertRows(QModelIndex(), row, row);

        m_views.insert(at, view);
        m_results.insert(view, new Results);

        endInsertRows();
    }

    return createIndex(row, 0);
}

QString SearchModel::fetchMatchedText(DocumentView* view, const SearchModel::Result& result) const
{
    const TextCacheObject* object = fetchText(view, result);

    return object != 0 ? object->first : QString();
}

QString SearchModel::fetchSurroundingText(DocumentView* view, const Result& result) const
{
    const TextCacheObject* object = fetchText(view, result);

    return object != 0 ? object->second : QString();
}

const SearchModel::TextCacheObject* SearchModel::fetchText(DocumentView* view, const SearchModel::Result& result) const
{
    const TextCacheKey key = textCacheKey(view, result);

    if(const TextCacheObject* object = m_textCache.object(key))
    {
        return object;
    }

    if(m_textWatchers.size() < 20 && !m_textWatchers.contains(key))
    {
        TextWatcher* watcher = new TextWatcher();
        m_textWatchers.insert(key, watcher);

        connect(watcher, SIGNAL(finished()), SLOT(on_fetchSurroundingText_finished()));

        watcher->setFuture(QtConcurrent::run(textJob, key, result));
    }

    return 0;
}

inline SearchModel::TextCacheKey SearchModel::textCacheKey(DocumentView* view, const Result& result)
{
    QByteArray key;

    QDataStream(&key, QIODevice::WriteOnly)
            << result.first
            << result.second;

    return qMakePair(view, key);
}

SearchModel::TextJob SearchModel::textJob(const TextCacheKey& key, const Result& result)
{
    const QPair< QString, QString >& text = key.first->searchContext(result.first, result.second);

    return TextJob(key, new TextCacheObject(text));
}

} // qpdfview
