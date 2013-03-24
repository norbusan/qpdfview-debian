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
    m_physicalDpiX(72),
    m_physicalDpiY(72),
    m_scaleFactor(1.0),
    m_rotation(RotateBy0),
    m_invertColors(false),
    m_prefetch(false)
{
    setAutoDelete(false);

    connect(this, SIGNAL(finished()), SLOT(on_finished()));
}

bool RenderTask::isRunning() const
{
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
        emit finished();

        return;
    }

    QImage image = m_page->render(m_physicalDpiX * m_scaleFactor, m_physicalDpiY * m_scaleFactor, m_rotation);

    if(m_wasCanceled && !m_prefetch)
    {
        emit finished();

        return;
    }

    if(m_invertColors)
    {
        image.invertPixels();
    }

    emit imageReady(m_physicalDpiX, m_physicalDpiY, m_scaleFactor, m_rotation, m_invertColors, m_prefetch, image);

    emit finished();
}

void RenderTask::start(Model::Page* page, int physicalDpiX, int physicalDpiY, qreal scaleFactor, Rotation rotation, bool invertColors, bool prefetch)
{
    m_page = page;

    m_physicalDpiX = physicalDpiX;
    m_physicalDpiY = physicalDpiY;

    m_scaleFactor = scaleFactor;
    m_rotation = rotation;

    m_invertColors = invertColors;

    m_prefetch = prefetch;

    m_isRunning = true;
    m_wasCanceled = false;

    QThreadPool::globalInstance()->start(this);
}

void RenderTask::cancel()
{
    m_wasCanceled = true;
}

void RenderTask::on_finished()
{
    m_isRunning = false;
}
