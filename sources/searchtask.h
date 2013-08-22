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

#ifndef SEARCHTASK_H
#define SEARCHTASK_H

#include <QRectF>
#include <QThread>

namespace Model
{
class Page;
}

class SearchTask : public QThread
{
    Q_OBJECT

public:
    explicit SearchTask(QObject* parent = 0);

    bool wasCanceled() const;

    int progress() const;

    void run();

signals:
    void progressChanged(int progress);

    void resultsReady(int index, QList< QRectF > results);

public slots:
    void start(const QList< Model::Page* >& pages, const QString& text, bool matchCase, int beginAtPage = 1);
    void cancel();

private:
    Q_DISABLE_COPY(SearchTask)

    bool m_wasCanceled;

    int m_progress;

    QList< Model::Page* > m_pages;

    QString m_text;
    bool m_matchCase;
    int m_beginAtPage;

};

#endif // SEARCHTHREAD_H
