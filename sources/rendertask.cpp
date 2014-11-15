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

#include <qmath.h>
#include <QThreadPool>

#include "model.h"

namespace
{

using namespace qpdfview;

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
                wasCanceled.load() == CanceledForcibly :
                wasCanceled.load() != NotCanceled;

#else

    return prefetch ?
                wasCanceled.testAndSetRelaxed(CanceledForcibly, CanceledForcibly) :
                !wasCanceled.testAndSetRelaxed(NotCanceled, NotCanceled);

#endif // QT_VERSION
}

int loadWasCanceled(const QAtomicInt& wasCanceled)
{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

    return wasCanceled.load();

#else

    return wasCanceled;

#endif // QT_VERSION
}

qreal scaledResolutionX(const RenderParam& renderParam)
{
    return renderParam.resolution.devicePixelRatio *
            renderParam.resolution.resolutionX * renderParam.scaleFactor;
}

qreal scaledResolutionY(const RenderParam& renderParam)
{
    return renderParam.resolution.devicePixelRatio *
            renderParam.resolution.resolutionY * renderParam.scaleFactor;
}

bool columnHasPaperColor(int x, QRgb paperColor, const QImage& image)
{
    const int height = image.height();

    for(int y = 0; y < height; ++y)
    {
        if(paperColor != (image.pixel(x, y) | 0xff000000u))
        {
            return false;
        }
    }

    return true;
}

bool rowHasPaperColor(int y, QRgb paperColor, const QImage& image)
{
    const int width = image.width();

    for(int x = 0; x < width; ++x)
    {
        if(paperColor != (image.pixel(x, y) | 0xff000000u))
        {
            return false;
        }
    }

    return true;
}

QRectF trimMargins(QRgb paperColor, const QImage& image)
{
    if(image.isNull())
    {
        return QRectF(0.0, 0.0, 1.0, 1.0);
    }

    const int width = image.width();
    const int height = image.height();

    int left;
    for(left = 0; left < width; ++left)
    {
        if(!columnHasPaperColor(left, paperColor, image))
        {
            break;
        }
    }
    left = qMin(left, width / 3);

    int right;
    for(right = width - 1; right >= left; --right)
    {
        if(!columnHasPaperColor(right, paperColor, image))
        {
            break;
        }
    }
    right = qMax(right, 2 * width / 3);

    int top;
    for(top = 0; top < height; ++top)
    {
        if(!rowHasPaperColor(top, paperColor, image))
        {
            break;
        }
    }
    top = qMin(top, height / 3);

    int bottom;
    for(bottom = height - 1; bottom >= top; --bottom)
    {
        if(!rowHasPaperColor(bottom, paperColor, image))
        {
            break;
        }
    }
    bottom = qMax(bottom, 2 * height / 3);

    left = qMax(left - width / 100, 0);
    top = qMax(top - height / 100, 0);

    right = qMin(right + width / 100, width);
    bottom = qMin(bottom + height / 100, height);

    return QRectF(static_cast< qreal >(left) / width,
                  static_cast< qreal >(top) / height,
                  static_cast< qreal >(right - left) / width,
                  static_cast< qreal >(bottom - top) / height);
}

void convertToGrayscale(QImage& image)
{
    QRgb* const begin = reinterpret_cast< QRgb* >(image.bits());
    QRgb* const end = reinterpret_cast< QRgb* >(image.bits() + image.byteCount());

    for(QRgb* pointer = begin; pointer != end; ++pointer)
    {
        const int gray = qGray(*pointer);
        const int alpha = qAlpha(*pointer);

        *pointer = qRgba(gray, gray, gray, alpha);
    }
}

} // anonymous

namespace qpdfview
{

RenderTask::RenderTask(Model::Page* page, QObject* parent) : QObject(parent), QRunnable(),
    m_isRunning(false),
    m_wasCanceled(NotCanceled),
    m_page(page),
    m_renderParam(),
    m_rect(),
    m_prefetch(false),
    m_trimMargins(false),
    m_paperColor()
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
    return loadWasCanceled(m_wasCanceled) != NotCanceled;
}

bool RenderTask::wasCanceledNormally() const
{
    return loadWasCanceled(m_wasCanceled) == CanceledNormally;
}

bool RenderTask::wasCanceledForcibly() const
{
    return loadWasCanceled(m_wasCanceled) == CanceledForcibly;
}

void RenderTask::run()
{
#define CANCELLATION_POINT if(testCancellation(m_wasCanceled, m_prefetch)) { finish(); return; }

    CANCELLATION_POINT

    QImage image;
    QRectF cropRect;

    image = m_page->render(scaledResolutionX(m_renderParam), scaledResolutionY(m_renderParam),
                           m_renderParam.rotation, m_rect);

#if QT_VERSION >= QT_VERSION_CHECK(5,1,0)

    image.setDevicePixelRatio(m_renderParam.resolution.devicePixelRatio);

#endif // QT_VERSION

    if(m_trimMargins)
    {
        CANCELLATION_POINT

        cropRect = trimMargins(m_paperColor.rgb(), image);
    }

    if(m_renderParam.convertToGrayscale)
    {
        CANCELLATION_POINT

        convertToGrayscale(image);
    }

    if(m_renderParam.invertColors)
    {
        CANCELLATION_POINT

        image.invertPixels();
    }

    CANCELLATION_POINT

    emit imageReady(m_renderParam,
                    m_rect, m_prefetch,
                    image, cropRect);

    finish();

#undef CANCELLATION_POINT
}

void RenderTask::start(const RenderParam& renderParam,
                       const QRect& rect, bool prefetch,
                       bool trimMargins, const QColor& paperColor)
{
    m_renderParam = renderParam;

    m_rect = rect;
    m_prefetch = prefetch;

    m_trimMargins = trimMargins;
    m_paperColor = paperColor;

    m_mutex.lock();
    m_isRunning = true;
    m_mutex.unlock();

    resetCancellation(m_wasCanceled);

    QThreadPool::globalInstance()->start(this, prefetch ? 0 : 1);
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
