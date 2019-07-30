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

#ifndef SEARCHTASK_H
#define SEARCHTASK_H

#include <QRectF>
#include <QThread>
#include <QVector>

namespace qpdfview
{

namespace Model
{
class Page;
}

class SearchTask : public QThread
{
    Q_OBJECT

public:
    explicit SearchTask(QObject* parent = 0);

    bool wasCanceled() const { return loadCancellation() != NotCanceled; }
    int progress() const { return acquireProgress(); }

    const QString& text() const { return m_text; }
    bool matchCase() const { return m_matchCase; }
    bool wholeWords() const { return m_wholeWords; }

    void run();

signals:
    void progressChanged(int progress);

    void resultsReady(int index, const QList< QRectF >& results);

public slots:
    void start(const QVector< Model::Page* >& pages,
               const QString& text, bool matchCase, bool wholeWords,
               int beginAtPage = 1, bool parallelExecution = false);

    void cancel() { setCancellation(); }

private:
    Q_DISABLE_COPY(SearchTask)

    QAtomicInt m_wasCanceled;
    mutable QAtomicInt m_progress;

    enum
    {
        NotCanceled = 0,
        Canceled = 1
    };

    void setCancellation();
    void resetCancellation();
    bool testCancellation();
    int loadCancellation() const;

    void releaseProgress(int value);
    int acquireProgress() const;
    int loadProgress() const;

    template< typename Future >
    void processResults(Future future);


    QVector< Model::Page* > m_pages;

    QString m_text;
    bool m_matchCase;
    bool m_wholeWords;
    int m_beginAtPage;
    bool m_parallelExecution;

};

#if QT_VERSION > QT_VERSION_CHECK(5,0,0)

inline void SearchTask::setCancellation()
{
    m_wasCanceled.storeRelease(Canceled);
}

inline void SearchTask::resetCancellation()
{
    m_wasCanceled.storeRelease(NotCanceled);
}

inline bool SearchTask::testCancellation()
{
    return m_wasCanceled.load() != NotCanceled;
}

inline int SearchTask::loadCancellation() const
{
    return m_wasCanceled.load();
}

inline void SearchTask::releaseProgress(int value)
{
    m_progress.storeRelease(value);
}

inline int SearchTask::acquireProgress() const
{
    return m_progress.loadAcquire();
}

inline int SearchTask::loadProgress() const
{
    return m_progress.load();
}

#else

inline void SearchTask::setCancellation()
{
    m_wasCanceled.fetchAndStoreRelease(Canceled);
}

inline void SearchTask::resetCancellation()
{
    m_wasCanceled.fetchAndStoreRelease(NotCanceled);
}

inline bool SearchTask::testCancellation()
{
    return !m_wasCanceled.testAndSetRelaxed(NotCanceled, NotCanceled);
}

inline int SearchTask::loadCancellation() const
{
    return m_wasCanceled;
}

inline void SearchTask::releaseProgress(int value)
{
    m_progress.fetchAndStoreRelease(value);
}

inline int SearchTask::acquireProgress() const
{
    return m_progress.fetchAndAddAcquire(0);
}

inline int SearchTask::loadProgress() const
{
    return m_progress;
}

#endif // QT_VERSION

} // qpdfview

#endif // SEARCHTHREAD_H
