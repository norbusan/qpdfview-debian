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

RenderTask::RenderTask(QObject* parent) : QObject(parent), QRunnable(),
    m_isRunning(false),
    m_wasCanceled(false),
    m_page(0),
    m_resolutionX(72),
    m_resolutionY(72),
    m_devicePixelRatio(1.0),
    m_scaleFactor(1.0),
    m_rotation(RotateBy0),
    m_invertColors(false),
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
    return m_wasCanceled;
}

void RenderTask::run()
{
    if(m_wasCanceled && !m_prefetch)
    {
        finish();

        return;
    }

    qreal resolutionX;
    qreal resolutionY;

    switch(m_rotation)
    {
    default:
    case RotateBy0:
    case RotateBy180:
        resolutionX = m_resolutionX * m_scaleFactor;
        resolutionY = m_resolutionY * m_scaleFactor;
        break;
    case RotateBy90:
    case RotateBy270:
        resolutionX = m_resolutionY * m_scaleFactor;
        resolutionY = m_resolutionX * m_scaleFactor;
        break;
    }

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

    QImage image = m_page->render(m_devicePixelRatio * resolutionX, m_devicePixelRatio * resolutionY, m_rotation);

    image.setDevicePixelRatio(m_devicePixelRatio);

#else

    QImage image = m_page->render(resolutionX, resolutionY, m_rotation);

#endif // QT_VERSION

    if(m_wasCanceled && !m_prefetch)
    {
        finish();

        return;
    }

    if(m_invertColors)
    {
        image.invertPixels();
    }

    emit imageReady(m_resolutionX, m_resolutionY, m_devicePixelRatio, m_scaleFactor, m_rotation, m_invertColors, m_prefetch, image);

    finish();
}

void RenderTask::start(Model::Page* page, int resolutionX, int resolutionY, qreal devicePixelRatio, qreal scaleFactor, Rotation rotation, bool invertColors, bool prefetch)
{
    m_page = page;

    m_resolutionX = resolutionX;
    m_resolutionY = resolutionY;
    m_devicePixelRatio = devicePixelRatio;

    m_scaleFactor = scaleFactor;
    m_rotation = rotation;

    m_invertColors = invertColors;

    m_prefetch = prefetch;

    m_mutex.lock();
    m_isRunning = true;
    m_mutex.unlock();

    m_wasCanceled = false;

    QThreadPool::globalInstance()->start(this);
}

void RenderTask::cancel()
{
    m_wasCanceled = true;
}

void RenderTask::finish()
{
    m_mutex.lock();
    m_isRunning = false;
    m_mutex.unlock();

    m_waitCondition.wakeAll();

    emit finished();
}
