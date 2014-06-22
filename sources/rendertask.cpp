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

#include "rendertask.h"

#include <QThreadPool>

#include "model.h"

namespace
{

enum
{
    NotCanceled = 0,
    CanceledNormally = 1,
    CanceledForcibly = 2
};

void setCancellation(QAtomicInt& wasCanceled, bool force)
{
#if QT_VERSION > QT_VERSION_CHECK(5,0,0)

    wasCanceled.storeRelease(force ? CanceledForcibly : CanceledNormally);

#else

    wasCanceled.fetchAndStoreRelease(force ? CanceledForcibly : CanceledNormally);

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

bool testCancellation(QAtomicInt& wasCanceled, bool prefetch)
{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

    return prefetch ?
                wasCanceled.loadAcquire() == CanceledForcibly :
                wasCanceled.loadAcquire() != NotCanceled;

#else

    return prefetch ?
                wasCanceled.testAndSetAcquire(CanceledForcibly, CanceledForcibly) :
                !wasCanceled.testAndSetAcquire(NotCanceled, NotCanceled);

#endif // QT_VERSION
}

} // anonymous

namespace qpdfview
{

RenderTask::RenderTask(QObject* parent) : QObject(parent), QRunnable(),
    m_isRunning(false),
    m_wasCanceled(NotCanceled),
    m_page(0),
    m_renderParam(),
    m_tile(),
    m_prefetch(false)
{
    setAutoDelete(false);
}

void RenderTask::wait()
{
    QMutexLocker mutexLocker(&m_mutex);

    while(m_isRunning)
    {
        m_waitCondition.wait(&m_mutex);
    }
}

bool RenderTask::isRunning() const
{
    QMutexLocker mutexLocker(&m_mutex);

    return m_isRunning;
}

bool RenderTask::wasCanceled() const
{
    return m_wasCanceled != NotCanceled;
}

bool RenderTask::wasCanceledNormally() const
{
    return m_wasCanceled == CanceledNormally;
}

bool RenderTask::wasCanceledForcibly() const
{
    return m_wasCanceled == CanceledForcibly;
}

void RenderTask::run()
{
    if(testCancellation(m_wasCanceled, m_prefetch))
    {
        finish();

        return;
    }


#if QT_VERSION >= QT_VERSION_CHECK(5,1,0)

    QImage image = m_page->render(m_renderParam.resolution.devicePixelRatio * m_renderParam.adjustedResolutionX(),
                                  m_renderParam.resolution.devicePixelRatio * m_renderParam.adjustedResolutionY(),
                                  m_renderParam.rotation, m_tile);

    image.setDevicePixelRatio(m_renderParam.resolution.devicePixelRatio);

#else

    QImage image = m_page->render(m_renderParam.adjustedResolutionX(), m_renderParam.adjustedResolutionY(),
                                  m_renderParam.rotation, m_tile);

#endif // QT_VERSION


    if(testCancellation(m_wasCanceled, m_prefetch))
    {
        finish();

        return;
    }


    if(m_renderParam.invertColors)
    {
        image.invertPixels();
    }

    emit imageReady(m_renderParam,
                    m_tile, m_prefetch,
                    image);

    finish();
}

void RenderTask::start(Model::Page* page,
                       const RenderParam& renderParam,
                       const QRect& tile, bool prefetch)
{
    m_page = page;

    m_renderParam = renderParam;

    m_tile = tile;
    m_prefetch = prefetch;

    m_mutex.lock();
    m_isRunning = true;
    m_mutex.unlock();

    resetCancellation(m_wasCanceled);

    QThreadPool::globalInstance()->start(this);
}

void RenderTask::cancel(bool force)
{
    setCancellation(m_wasCanceled, force);
}

void RenderTask::finish()
{
    emit finished();

    m_mutex.lock();
    m_isRunning = false;
    m_mutex.unlock();

    m_waitCondition.wakeAll();
}

} // qpdfview
