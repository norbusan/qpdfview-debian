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

int loadWasCanceled(const QAtomicInt& wasCanceled)
{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

    return wasCanceled.load();

#else

    return wasCanceled;

#endif // QT_VERSION
}

void releaseProgress(QAtomicInt& progress, int value)
{
#if QT_VERSION > QT_VERSION_CHECK(5,0,0)

    progress.storeRelease(value);

#else

    progress.fetchAndStoreRelease(value);

#endif // QT_VERSION
}

int acquireProgress(QAtomicInt& progress)
{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

    return progress.loadAcquire();

#else

    return progress.fetchAndAddAcquire(0);

#endif // QT_VERSION
}

int loadProgress(const QAtomicInt& progress)
{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

    return progress.load();
#else

    return progress;

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
    m_wholeWords(false),
    m_beginAtPage(1)
{
}

bool SearchTask::wasCanceled() const
{
    return loadWasCanceled(m_wasCanceled) != NotCanceled;
}

int SearchTask::progress() const
{
    return acquireProgress(m_progress);
}

void SearchTask::run()
{
    for(int index = m_beginAtPage - 1; index < m_pages.count() + m_beginAtPage - 1; ++index)
    {
        if(testCancellation(m_wasCanceled))
        {
            break;
        }

        const QList< QRectF > results = m_pages.at(index % m_pages.count())->search(m_text, m_matchCase, m_wholeWords);

        emit resultsReady(index % m_pages.count(), results);

        releaseProgress(m_progress, 100 * (index + 1 - m_beginAtPage + 1) / m_pages.count());

        emit progressChanged(loadProgress(m_progress));
    }

    releaseProgress(m_progress, 0);
}

void SearchTask::start(const QVector< Model::Page* >& pages,
                       const QString& text, bool matchCase, bool wholeWords, int beginAtPage)
{
    m_pages = pages;

    m_text = text;
    m_matchCase = matchCase;
    m_wholeWords = wholeWords;
    m_beginAtPage = beginAtPage;

    resetCancellation(m_wasCanceled);
    releaseProgress(m_progress, 0);

    QThread::start();
}

void SearchTask::cancel()
{
    setCancellation(m_wasCanceled);
}

} // qpdfview
