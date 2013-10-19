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

    void run();

signals:
    void finished();

    void imageReady(int resolutionX, int resolutionY, qreal devicePixelRatio, qreal scaleFactor, Rotation rotation, bool invertColors, bool prefetch, QImage image);

public slots:
    void start(Model::Page* page, int resolutionX, int resolutionY, qreal devicePixelRatio, qreal scaleFactor, Rotation rotation, bool invertColors, bool prefetch);
    void cancel();

private:
    Q_DISABLE_COPY(RenderTask)

    mutable QMutex m_mutex;
    QWaitCondition m_waitCondition;

    void finish();

    bool m_isRunning;
    bool m_wasCanceled;

    Model::Page* m_page;

    int m_resolutionX;
    int m_resolutionY;
    qreal m_devicePixelRatio;

    qreal m_scaleFactor;
    Rotation m_rotation;

    bool m_invertColors;

    bool m_prefetch;

};

#endif // RENDERTASK_H
