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

#include <QImage>
#include <QMutex>
#include <QRunnable>
#include <QSet>
#include <QWaitCondition>

#include "renderparam.h"

namespace qpdfview
{

namespace Model
{
class Page;
}

class Settings;

class RenderTaskDispatcher : public QObject
{
    Q_OBJECT

    friend class RenderTask;

    struct DeleteLaterEvent;
    struct FinishedEvent;
    struct ImageReadyEvent;

public:
    class Parent
    {
        friend struct DeleteLaterEvent;
        friend struct FinishedEvent;
        friend struct ImageReadyEvent;

    public:
        virtual ~Parent();

    private:
        virtual void on_finished() = 0;
        virtual void on_imageReady(const RenderParam& renderParam,
                                   const QRect& rect, bool prefetch,
                                   const QImage& image, const QRectF& cropRect) = 0;
    };

private:
    RenderTaskDispatcher(QObject* parent = 0);

    void deleteLater(Parent* parent);

    void finished(Parent* parent);
    void imageReady(Parent* parent,
                    const RenderParam& renderParam,
                    const QRect& rect, bool prefetch,
                    const QImage& image, const QRectF& cropRect);

public:
    bool event(QEvent* event);

private:
    QSet< Parent* > m_activeParents;

    template< typename RenderTaskEvent >
    void dispatchIfActive(QEvent* event);

    void makeActive(Parent* parent);
    void makeInactive(Parent* parent);

};

class RenderTask : public QRunnable
{
public:
    explicit RenderTask(Model::Page* page, RenderTaskDispatcher::Parent* parent = 0);
    ~RenderTask();

    void wait();

    bool isRunning() const;

    bool wasCanceled() const { return loadCancellation() != NotCanceled; }
    bool wasCanceledNormally() const { return loadCancellation() == CanceledNormally; }
    bool wasCanceledForcibly() const { return loadCancellation() == CanceledForcibly; }

    void run();

    void start(const RenderParam& renderParam,
               const QRect& rect, bool prefetch);

    void cancel(bool force = false) { setCancellation(force); }

    void deleteParentLater();

private:
    Q_DISABLE_COPY(RenderTask)

    static Settings* s_settings;

    static RenderTaskDispatcher* s_dispatcher;
    RenderTaskDispatcher::Parent* m_parent;

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

    static RenderParam s_defaultRenderParam;
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
