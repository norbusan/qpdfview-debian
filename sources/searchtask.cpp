/*

Copyright 2012-2015 Adam Reichold

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

#include "searchtask.h"

#include "model.h"

#include <QtConcurrentMap>

namespace qpdfview
{

namespace
{

struct Search
{
    Search(const QString& text, const bool matchCase, const bool wholeWords) :
        text(text),
        matchCase(matchCase),
        wholeWords(wholeWords)
    {
    }

    const QString& text;
    const bool matchCase;
    const bool wholeWords;

    typedef QList< QRectF > result_type;

    result_type operator()(const Model::Page* const page) const
    {
        return page->search(text, matchCase, wholeWords);
    }
};

struct FutureWrapper
{
    FutureWrapper(const QVector< const Model::Page* >& pages, const Search& search) :
        pages(pages),
        search(search)
    {
    }

    const QVector< const Model::Page* >& pages;
    const Search& search;

    void cancel()
    {
    }

    Search::result_type resultAt(int index)
    {
        return search(pages.at(index));
    }
};

}

SearchTask::SearchTask(QObject* parent) : QThread(parent),
    m_wasCanceled(NotCanceled),
    m_progress(0),
    m_pages(),
    m_text(),
    m_matchCase(false),
    m_wholeWords(false),
    m_beginAtPage(1),
    m_parallelExecution(false)
{
}

void SearchTask::run()
{
    QVector< const Model::Page* > pages;
    pages.reserve(m_pages.count());

    for(int index = 0, count = m_pages.count(); index < count; ++index)
    {
        const int shiftedIndex = (index + m_beginAtPage - 1) % count;
        pages.append(m_pages.at(shiftedIndex));
    }

    const Search search(m_text, m_matchCase, m_wholeWords);

    if(m_parallelExecution)
    {
        processResults(QtConcurrent::mapped(pages, search));
    }
    else
    {
        processResults(FutureWrapper(pages, search));
    }
}

void SearchTask::start(const QVector< Model::Page* >& pages,
                       const QString& text, bool matchCase, bool wholeWords,
                       int beginAtPage, bool parallelExecution)
{
    m_pages = pages;

    m_text = text;
    m_matchCase = matchCase;
    m_wholeWords = wholeWords;
    m_beginAtPage = beginAtPage;
    m_parallelExecution = parallelExecution;

    resetCancellation();
    releaseProgress(0);

    QThread::start();
}

template< typename Future >
void SearchTask::processResults(Future future)
{
    for(int index = 0, count = m_pages.count(); index < count; ++index)
    {
        if(testCancellation())
        {
            future.cancel();
            break;
        }

        const int shiftedIndex = (index + m_beginAtPage - 1) % count;
        const QList< QRectF > results = future.resultAt(index);

        emit resultsReady(shiftedIndex, results);

        const int progress = 100 * (index + 1) / m_pages.count();

        releaseProgress(progress);

        emit progressChanged(progress);
    }

    releaseProgress(0);
}

} // qpdfview
