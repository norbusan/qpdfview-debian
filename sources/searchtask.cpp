/*

Copyright 2012-2013 Adam Reichold

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

namespace
{

using namespace qpdfview;

enum
{
    NotCanceled = 0,
    Canceled = 1
};

void setCancellation(QAtomicInt& wasCanceled)
{
#if QT_VERSION > QT_VERSION_CHECK(5,0,0)

    wasCanceled.storeRelease(Canceled);

#else

    wasCanceled.fetchAndStoreRelease(Canceled);

#endif // QT_VERSION
}

void resetCancellation(QAtomicInt& wasCanceled)
{
#if QT_VERSION > QT_VERSION_CHECK(5,0,0)

    wasCanceled.storeRelease(NotCanceled);

#else

    wasCanceled.fetchAndStoreRelease(NotCanceled);

#endif // QT_VERSION
}

bool testCancellation(QAtomicInt& wasCanceled)
{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

    return wasCanceled.load() != NotCanceled;

#else

    return !wasCanceled.testAndSetRelaxed(NotCanceled, NotCanceled);

#endif // QT_VERSION
}

void storeProgress(QAtomicInt& progress, int value)
{
#if QT_VERSION > QT_VERSION_CHECK(5,0,0)

    progress.storeRelease(value);

#else

    progress.fetchAndStoreRelease(value);

#endif // QT_VERSION
}

int loadProgress(QAtomicInt& progress)
{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

    return progress.loadAcquire();

#else

    return progress.fetchAndAddAcquire(0);

#endif // QT_VERSION
}

} // anonymous

namespace qpdfview
{

SearchTask::SearchTask(QObject* parent) : QThread(parent),
    m_wasCanceled(NotCanceled),
    m_progress(0),
    m_pages(),
    m_text(),
    m_matchCase(false),
    m_beginAtPage(1)
{
}

bool SearchTask::wasCanceled() const
{
    return m_wasCanceled;
}

int SearchTask::progress() const
{
    return loadProgress(m_progress);
}

void SearchTask::run()
{
    for(int index = m_beginAtPage - 1; index < m_pages.count() + m_beginAtPage - 1; ++index)
    {
        if(testCancellation(m_wasCanceled))
        {
            break;
        }

        const QList< QRectF > results = m_pages.at(index % m_pages.count())->search(m_text, m_matchCase);

        emit resultsReady(index % m_pages.count(), results);

        storeProgress(m_progress, 100 * (index + 1 - m_beginAtPage + 1) / m_pages.count());

        emit progressChanged(m_progress);
    }

    storeProgress(m_progress, 0);
}

void SearchTask::start(const QVector< Model::Page* >& pages,
                       const QString& text, bool matchCase, int beginAtPage)
{
    m_pages = pages;

    m_text = text;
    m_matchCase = matchCase;
    m_beginAtPage = beginAtPage;

    resetCancellation(m_wasCanceled);
    storeProgress(m_progress, 0);

    QThread::start();
}

void SearchTask::cancel()
{
    setCancellation(m_wasCanceled);
}

} // qpdfview
