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

SearchTask::SearchTask(QObject* parent) : QThread(parent),
    m_wasCanceled(false),
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
    return m_progress;
}

void SearchTask::run()
{
    for(int index = m_beginAtPage - 1; index < m_pages.count() + m_beginAtPage - 1; ++index)
    {
        if(m_wasCanceled)
        {
            m_progress = 0;

            return;
        }

        const QList< QRectF > results = m_pages.at(index % m_pages.count())->search(m_text, m_matchCase);

        emit resultsReady(index % m_pages.count(), results);

        m_progress = 100 * (index - m_beginAtPage)/ m_pages.count();

        emit progressChanged(m_progress);
    }

    m_progress = 0;
}

void SearchTask::start(const QList< Model::Page* >& pages, const QString& text, bool matchCase, int beginAtPage)
{
    m_pages = pages;

    m_text = text;
    m_matchCase = matchCase;
    m_beginAtPage = beginAtPage;

    m_wasCanceled = false;
    m_progress = 0;

    QThread::start();
}

void SearchTask::cancel()
{
    m_wasCanceled = true;
}
