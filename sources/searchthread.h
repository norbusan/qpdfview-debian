/*

Copyright 2012 Adam Reichold

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

#ifndef SEARCHTHREAD_H
#define SEARCHTHREAD_H

#include <QtCore>

#include <poppler-qt4.h>

class SearchThread : public QThread
{
    Q_OBJECT

public:
    explicit SearchThread(QObject* parent = 0);

    bool wasCanceled() const;
    int progress() const;

    void run();

public slots:
    void start(QMutex* mutex, Poppler::Document* document, const QList< int >& indices, const QString& text, bool matchCase);
    void cancel();

signals:
    void progressed(int progress);
    void finished();
    void canceled();

    void resultsReady(int index, QList< QRectF > results);

private:
    bool m_wasCanceled;
    int m_progress;

    QMutex* m_mutex;
    Poppler::Document* m_document;

    QList< int > m_indices;
    QString m_text;
    bool m_matchCase;

};

#endif // SEARCHTHREAD_H
