/*

Copyright 2013-2015 Adam Reichold

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

#include <QColor>
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

class Settings;

class RenderTask : public QObject, QRunnable
{
    Q_OBJECT

public:
    explicit RenderTask(Model::Page* page, QObject* parent = 0);

    void wait();

    bool isRunning() const;

    bool wasCanceled() const { return loadCancellation() != NotCanceled; }
    bool wasCanceledNormally() const { return loadCancellation() == CanceledNormally; }
    bool wasCanceledForcibly() const { return loadCancellation() == CanceledForcibly; }

    void run();

signals:
    void finished();

    void imageReady(const RenderParam& renderParam,
                    const QRect& rect, bool prefetch,
                    QImage image, QRectF cropRect);

public slots:
    void start(const RenderParam& renderParam,
               const QRect& rect, bool prefetch);

    void cancel(bool force = false) { setCancellation(force); }

private:
    Q_DISABLE_COPY(RenderTask)

    static Settings* s_settings;

    mutable QMutex m_mutex;
    QWaitCondition m_waitCondition;

    bool m_isRunning;
    QAtomicInt m_wasCanceled;

    enum
    {
        NotCanceled = 0,
        CanceledNormally = 1,
        CanceledForcibly = 2
    };

    void setCancellation(bool force);
    void resetCancellation();
    bool testCancellation();
    int loadCancellation() const;

    void finish();


    Model::Page* m_page;

    RenderParam m_renderParam;

    QRect m_rect;
    bool m_prefetch;

};

#if QT_VERSION > QT_VERSION_CHECK(5,0,0)

inline void RenderTask::setCancellation(bool force)
{
    m_wasCanceled.storeRelease(force ? CanceledForcibly : CanceledNormally);
}

inline void RenderTask::resetCancellation()
{
    m_wasCanceled.storeRelease(NotCanceled);
}

inline bool RenderTask::testCancellation()
{
    return m_prefetch ?
                m_wasCanceled.load() == CanceledForcibly :
                m_wasCanceled.load() != NotCanceled;
}

inline int RenderTask::loadCancellation() const
{
    return m_wasCanceled.load();
}

#else

inline void RenderTask::setCancellation(bool force)
{
    m_wasCanceled.fetchAndStoreRelease(force ? CanceledForcibly : CanceledNormally);
}

inline void RenderTask::resetCancellation()
{
    m_wasCanceled.fetchAndStoreRelease(NotCanceled);
}

inline bool RenderTask::testCancellation()
{
    return m_prefetch ?
                m_wasCanceled.testAndSetRelaxed(CanceledForcibly, CanceledForcibly) :
                !m_wasCanceled.testAndSetRelaxed(NotCanceled, NotCanceled);
}

inline int RenderTask::loadCancellation() const
{
    return m_wasCanceled;
}

#endif // QT_VERSION

} // qpdfview

#endif // RENDERTASK_H
