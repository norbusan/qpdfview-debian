/*

Copyright 2013 Adam Reichold

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

#ifndef RENDERTASK_H
#define RENDERTASK_H

#include <QImage>
#include <QMutex>
#include <QRunnable>
#include <QWaitCondition>

#include "global.h"

namespace qpdfview
{

namespace Model
{
class Page;
}

class RenderTask : public QObject, QRunnable
{
    Q_OBJECT

public:
    explicit RenderTask(QObject* parent = 0);

    void wait();

    bool isRunning() const;

    bool wasCanceled() const;
    bool wasCanceledNormally() const;
    bool wasCanceledForcibly() const;

    void run();

signals:
    void finished();

    void imageReady(const RenderParam& renderParam,
                    const QRect& rect, bool prefetch,
                    QImage image);

public slots:
    void start(Model::Page* page,
               const RenderParam& renderParam,
               const QRect& rect, bool prefetch);

    void cancel(bool force = false);

private:
    Q_DISABLE_COPY(RenderTask)

    mutable QMutex m_mutex;
    QWaitCondition m_waitCondition;

    bool m_isRunning;
    QAtomicInt m_wasCanceled;

    void finish();


    Model::Page* m_page;

    RenderParam m_renderParam;

    QRect m_rect;
    bool m_prefetch;

};

} // qpdfview

#endif // RENDERTASK_H
