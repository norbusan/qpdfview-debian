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
    m_indices(),
    m_text(),
    m_matchCase(false)
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
    int indicesDone = 0;
    int indicesToDo = m_indices.count();

    foreach(int index, m_indices)
    {
        if(m_wasCanceled)
        {
            m_progress = 0;

            emit finished();

            return;
        }

        QList< QRectF > results = m_pages.at(index)->search(m_text, m_matchCase);

        emit resultsReady(index, results);

        m_progress = 100 * ++indicesDone / indicesToDo;

        emit progressChanged(m_progress);
    }

    m_progress = 0;

    emit finished();
}

void SearchTask::start(const QVector< Model::Page* >& pages, const QVector< int >& indices, const QString& text, bool matchCase)
{
    m_pages = pages;

    m_indices = indices;
    m_text = text;
    m_matchCase = matchCase;

    m_wasCanceled = false;
    m_progress = 0;

    QThread::start();
}

void SearchTask::cancel()
{
    m_wasCanceled = true;
}
