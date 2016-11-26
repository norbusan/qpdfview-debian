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

#include "rendertask.h"

#include <QApplication>
#include <qmath.h>
#include <QPainter>
#include <QThreadPool>

#include "model.h"
#include "settings.h"

namespace qpdfview
{

namespace
{

void registerEventType(QEvent::Type& registeredType)
{
    if(registeredType == QEvent::None)
    {
        registeredType = QEvent::Type(QEvent::registerEventType());
    }
}

qreal scaledResolutionX(const RenderParam& renderParam)
{
    return renderParam.devicePixelRatio()
            * renderParam.resolutionX()
            * renderParam.scaleFactor();
}

qreal scaledResolutionY(const RenderParam& renderParam)
{
    return renderParam.devicePixelRatio()
            * renderParam.resolutionY()
            * renderParam.scaleFactor();
}

const QRgb alphaMask = 0xffu << 24;

bool columnHasPaperColor(int x, QRgb paperColor, const QImage& image)
{
    const int height = image.height();

    for(int y = 0; y < height; ++y)
    {
        const QRgb color = image.pixel(x, y);

        if(qAlpha(color) != 0 && paperColor != (color | alphaMask))
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
        const QRgb color = image.pixel(x, y);

        if(qAlpha(color) != 0 && paperColor != (color | alphaMask))
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

void composeWithColor(QPainter::CompositionMode mode, const QColor& color, QImage& image)
{
    QPainter painter(&image);

    painter.setCompositionMode(mode);
    painter.fillRect(image.rect(), color);
}

} // anonymous

RenderTaskParent::~RenderTaskParent()
{
}

struct RenderTaskFinishedEvent : public QEvent
{
    static QEvent::Type registeredType;

    RenderTaskFinishedEvent(RenderTaskParent* const parent,
                            const RenderParam& renderParam,
                            const QRect& rect,
                            const bool prefetch,
                            const QImage& image,
                            const QRectF& cropRect)
        : QEvent(registeredType)
        , parent(parent)
        , renderParam(renderParam)
        , rect(rect)
        , prefetch(prefetch)
        , image(image)
        , cropRect(cropRect)
    {
    }
    ~RenderTaskFinishedEvent();

    void dispatch() const
    {
        parent->on_finished(renderParam,
                            rect, prefetch,
                            image, cropRect);
    }

    RenderTaskParent* const parent;

    const RenderParam renderParam;
    const QRect rect;
    const bool prefetch;
    const QImage image;
    const QRectF cropRect;
};

QEvent::Type RenderTaskFinishedEvent::registeredType = QEvent::None;

RenderTaskFinishedEvent::~RenderTaskFinishedEvent()
{
}

struct RenderTaskCanceledEvent : public QEvent
{

    static QEvent::Type registeredType;

    RenderTaskCanceledEvent(RenderTaskParent* parent)
        : QEvent(registeredType)
        , parent(parent)
    {
    }
    ~RenderTaskCanceledEvent();

    void dispatch() const
    {
        parent->on_canceled();
    }

    RenderTaskParent* const parent;
};

QEvent::Type RenderTaskCanceledEvent::registeredType = QEvent::None;

RenderTaskCanceledEvent::~RenderTaskCanceledEvent()
{
}

struct DeleteParentLaterEvent : public QEvent
{
    static QEvent::Type registeredType;

    DeleteParentLaterEvent(RenderTaskParent* const parent)
        : QEvent(registeredType)
        , parent(parent)
    {
    }
    ~DeleteParentLaterEvent();

    void dispatch() const
    {
        delete parent;
    }

    RenderTaskParent* const parent;
};

QEvent::Type DeleteParentLaterEvent::registeredType = QEvent::None;

DeleteParentLaterEvent::~DeleteParentLaterEvent()
{
}

namespace
{

class DispatchChain
{
public:
    DispatchChain(const QEvent* const event, const QSet< RenderTaskParent* >& activeParents) :
        m_event(event),
        m_activeParents(activeParents)
    {
    }

    template< typename Event >
    DispatchChain& dispatch()
    {
        if(m_event != 0 && m_event->type() == Event::registeredType)
        {
            const Event* const event = static_cast< const Event* >(m_event);

            m_event = 0;

            if(m_activeParents.contains(event->parent))
            {
                event->dispatch();
            }
        }

        return *this;
    }

    bool wasDispatched() const
    {
        return m_event == 0;
    }

private:
    const QEvent* m_event;
    const QSet< RenderTaskParent* >& m_activeParents;
};

} // anonymous

RenderTaskDispatcher::RenderTaskDispatcher(QObject* parent) : QObject(parent)
{
    registerEventType(DeleteParentLaterEvent::registeredType);
    registerEventType(RenderTaskFinishedEvent::registeredType);
    registerEventType(RenderTaskCanceledEvent::registeredType);
}

void RenderTaskDispatcher::finished(RenderTaskParent* parent,
                                    const RenderParam& renderParam,
                                    const QRect& rect, bool prefetch,
                                    const QImage& image, const QRectF& cropRect)
{
    RenderTaskFinishedEvent* const event = new RenderTaskFinishedEvent(parent,
                                                                       renderParam,
                                                                       rect, prefetch,
                                                                       image, cropRect);

    QApplication::postEvent(this, event, Qt::HighEventPriority);
}

void RenderTaskDispatcher::canceled(RenderTaskParent* parent)
{
    QApplication::postEvent(this, new RenderTaskCanceledEvent(parent), Qt::HighEventPriority);
}

void RenderTaskDispatcher::deleteParentLater(RenderTaskParent* parent)
{
    QApplication::postEvent(this, new DeleteParentLaterEvent(parent), Qt::LowEventPriority);
}

bool RenderTaskDispatcher::event(QEvent* event)
{
    DispatchChain chain(event, m_activeParents);

    chain.dispatch< RenderTaskFinishedEvent >()
        .dispatch< RenderTaskCanceledEvent >()
        .dispatch< DeleteParentLaterEvent >();

    return chain.wasDispatched() || QObject::event(event);
}

void RenderTaskDispatcher::addActiveParent(RenderTaskParent* parent)
{
    m_activeParents.insert(parent);
}

void RenderTaskDispatcher::removeActiveParent(RenderTaskParent* parent)
{
    m_activeParents.remove(parent);
}

RenderTaskDispatcher* RenderTask::s_dispatcher = 0;

Settings* RenderTask::s_settings = 0;

const RenderParam RenderTask::s_defaultRenderParam;

RenderTask::RenderTask(Model::Page* page, RenderTaskParent* parent) : QRunnable(),
    m_parent(parent),
    m_isRunning(false),
    m_wasCanceled(NotCanceled),
    m_page(page),
    m_renderParam(s_defaultRenderParam),
    m_rect(),
    m_prefetch(false)
{
    if(s_settings == 0)
    {
        s_settings = Settings::instance();
    }

    if(s_dispatcher == 0)
    {
        s_dispatcher = new RenderTaskDispatcher(qApp);
    }

    setAutoDelete(false);

    s_dispatcher->addActiveParent(m_parent);
}

RenderTask::~RenderTask()
{
    s_dispatcher->removeActiveParent(m_parent);
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

void RenderTask::run()
{
#define CANCELLATION_POINT if(testCancellation()) { finish(true); return; }

    CANCELLATION_POINT

    QImage image;
    QRectF cropRect;

#if QT_VERSION >= QT_VERSION_CHECK(5,1,0)

    const qreal devicePixelRatio = m_renderParam.devicePixelRatio();

    const QRect rect =
            qFuzzyCompare(1.0, devicePixelRatio)
            ? m_rect
            : QTransform().scale(devicePixelRatio, devicePixelRatio).mapRect(m_rect);

#else

    const QRect& rect = m_rect;

#endif // QT_VERSION

    image = m_page->render(scaledResolutionX(m_renderParam), scaledResolutionY(m_renderParam),
                           m_renderParam.rotation(), rect);

#if QT_VERSION >= QT_VERSION_CHECK(5,1,0)

    image.setDevicePixelRatio(devicePixelRatio);

#endif // QT_VERSION

    if(m_renderParam.darkenWithPaperColor())
    {
        CANCELLATION_POINT

        composeWithColor(QPainter::CompositionMode_Darken, s_settings->pageItem().paperColor(), image);
    }
    else if(m_renderParam.lightenWithPaperColor())
    {
        CANCELLATION_POINT

        composeWithColor(QPainter::CompositionMode_Lighten, s_settings->pageItem().paperColor(), image);
    }

    if(m_renderParam.trimMargins())
    {
        CANCELLATION_POINT

        cropRect = trimMargins(s_settings->pageItem().paperColor().rgb() | alphaMask, image);
    }

    if(m_renderParam.convertToGrayscale())
    {
        CANCELLATION_POINT

        convertToGrayscale(image);
    }

    if(m_renderParam.invertColors())
    {
        CANCELLATION_POINT

        image.invertPixels();
    }

    CANCELLATION_POINT

    s_dispatcher->finished(m_parent,
                           m_renderParam,
                           m_rect, m_prefetch,
                           image, cropRect);

    finish(false);

#undef CANCELLATION_POINT
}

void RenderTask::start(const RenderParam& renderParam,
                       const QRect& rect, bool prefetch)
{
    m_renderParam = renderParam;

    m_rect = rect;
    m_prefetch = prefetch;

    m_mutex.lock();
    m_isRunning = true;
    m_mutex.unlock();

    resetCancellation();

    QThreadPool::globalInstance()->start(this, prefetch ? 1 : 2);
}

void RenderTask::deleteParentLater()
{
    s_dispatcher->deleteParentLater(m_parent);
}

void RenderTask::finish(bool canceled)
{
    m_renderParam = s_defaultRenderParam;

    if(canceled)
    {
        s_dispatcher->canceled(m_parent);
    }

    m_mutex.lock();
    m_isRunning = false;
    m_mutex.unlock();

    m_waitCondition.wakeAll();
}

} // qpdfview
