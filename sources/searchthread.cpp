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

#include "searchthread.h"

#include "model.h"

SearchThread::SearchThread(QObject* parent) : QThread(parent),
    m_wasCanceled(false),
    m_progress(0),
    m_document(0),
    m_indices(),
    m_text(),
    m_matchCase(false)
{
}

bool SearchThread::wasCanceled() const
{
    return m_wasCanceled;
}

int SearchThread::progress() const
{
    return m_progress;
}

void SearchThread::run()
{
    int indicesDone = 0;
    int indicesToDo = m_indices.count();

    foreach(int index, m_indices)
    {
        if(m_wasCanceled)
        {
            m_progress = 0;

            emit canceled();

            return;
        }

        Model::Page* page = m_document->page(index);

        QList< QRectF > results = page->search(m_text, m_matchCase);

        delete page;

        emit resultsReady(index, results);

        m_progress = 100 * ++indicesDone / indicesToDo;

        emit progressed(m_progress);
    }

    m_progress = 0;

    emit finished();
}

void SearchThread::start(Model::Document* document, const QList< int >& indices, const QString& text, bool matchCase)
{
    m_document = document;

    m_indices = indices;
    m_text = text;
    m_matchCase = matchCase;

    m_wasCanceled = false;
    m_progress = 0;

    QThread::start();
}

void SearchThread::cancel()
{
    m_wasCanceled = true;
}
