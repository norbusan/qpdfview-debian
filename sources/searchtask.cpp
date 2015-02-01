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

void SearchTask::run()
{
    for(int index = m_beginAtPage - 1; index < m_pages.count() + m_beginAtPage - 1; ++index)
    {
        if(testCancellation())
        {
            break;
        }

        const QList< QRectF > results = m_pages.at(index % m_pages.count())->search(m_text, m_matchCase, m_wholeWords);

        emit resultsReady(index % m_pages.count(), results);

        releaseProgress(100 * (index + 1 - m_beginAtPage + 1) / m_pages.count());

        emit progressChanged(loadProgress());
    }

    releaseProgress(0);
}

void SearchTask::start(const QVector< Model::Page* >& pages,
                       const QString& text, bool matchCase, bool wholeWords, int beginAtPage)
{
    m_pages = pages;

    m_text = text;
    m_matchCase = matchCase;
    m_wholeWords = wholeWords;
    m_beginAtPage = beginAtPage;

    resetCancellation();
    releaseProgress(0);

    QThread::start();
}

} // qpdfview
