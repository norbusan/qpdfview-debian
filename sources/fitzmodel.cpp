/*

Copyright 2014 Adam Reichold

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

#include "fitzmodel.h"

#include <QFile>
#include <qmath.h>

Model::FitzPage::FitzPage(QMutex* mutex, fz_context* context, fz_document* document, fz_page* page) :
    m_mutex(mutex),
    m_context(context),
    m_document(document),
    m_page(page)
{
}

Model::FitzPage::~FitzPage()
{
    fz_free_page(m_document, m_page);
}

QSizeF Model::FitzPage::size() const
{
    QMutexLocker mutexLocker(m_mutex);

    fz_rect rect;
    fz_bound_page(m_document, m_page, &rect);

    return QSize(qCeil(qAbs(rect.x1 - rect.x0)), qCeil(qAbs(rect.y1 - rect.y0)));
}

QImage Model::FitzPage::render(qreal horizontalResolution, qreal verticalResolution, Rotation rotation, const QRect& boundingRect) const
{
    QMutexLocker mutexLocker(m_mutex);

    fz_matrix matrix;

    switch(rotation)
    {
    default:
    case RotateBy0:
        fz_rotate(&matrix, 0.0);
        break;
    case RotateBy90:
        fz_rotate(&matrix, 90.0);
        break;
    case RotateBy180:
        fz_rotate(&matrix, 180.0);
        break;
    case RotateBy270:
        fz_rotate(&matrix, 270.0);
        break;
    }

    fz_pre_scale(&matrix, horizontalResolution / 72.0f, verticalResolution / 72.0f);

    fz_rect rect;
    fz_bound_page(m_document, m_page, &rect);
    fz_transform_rect(&rect, &matrix);

    fz_irect irect;
    fz_round_rect(&irect, &rect);

    fz_context* context = fz_clone_context(m_context);
    fz_display_list* display_list = fz_new_display_list(context);

    fz_device* device = fz_new_list_device(context, display_list);
    fz_run_page(m_document, m_page, device, &fz_identity, 0);
    fz_free_device(device);


    mutexLocker.unlock();


    fz_pixmap* pixmap = fz_new_pixmap_with_bbox(context, fz_device_rgb(context), &irect);
    fz_clear_pixmap_with_value(context, pixmap, 0xff); // TODO: consider configured paper color

    device = fz_new_draw_device(context, pixmap);
    fz_run_display_list(display_list, device, &matrix, &rect, 0);
    fz_free_device(device);

    const int width = fz_pixmap_width(m_context, pixmap);
    const int height = fz_pixmap_height(m_context, pixmap);
    unsigned char* samples = fz_pixmap_samples(m_context, pixmap);

    QImage auxiliaryImage(samples, width, height, QImage::Format_RGB32);
    QImage image(boundingRect.isNull() ? auxiliaryImage.copy(0, 0, width, height) : auxiliaryImage.copy(boundingRect));

    fz_drop_pixmap(context, pixmap);
    fz_drop_display_list(context, display_list);
    fz_free_context(context);

    return image;
}

Model::FitzDocument::FitzDocument(fz_context* context, fz_document* document) :
    m_mutex(),
    m_context(context),
    m_document(document)
{
}

Model::FitzDocument::~FitzDocument()
{
    fz_close_document(m_document);
    fz_free_context(m_context);
}

int Model::FitzDocument::numberOfPages() const
{
    QMutexLocker mutexLocker(&m_mutex);

    return fz_count_pages(m_document);
}

Model::Page* Model::FitzDocument::page(int index) const
{
    QMutexLocker mutexLocker(&m_mutex);

    fz_page* page = fz_load_page(m_document, index);

    return page != 0 ? new FitzPage(&m_mutex, m_context, m_document, page) : 0;
}

FitzPlugin::FitzPlugin(QObject* parent) : QObject(parent)
{
    setObjectName("FitzPlugin");

    m_locks_context.user = reinterpret_cast< void* >(this);
    m_locks_context.lock = FitzPlugin::lock;
    m_locks_context.unlock = FitzPlugin::unlock;

    m_context = fz_new_context(0, &m_locks_context, FZ_STORE_DEFAULT);
}

Model::Document* FitzPlugin::loadDocument(const QString& filePath) const
{
    fz_context* context = fz_clone_context(m_context);
    fz_document* document = fz_open_document(context, QFile::encodeName(filePath));

    if(document == 0)
    {
        fz_free_context(context);

        return 0;
    }

    return new Model::FitzDocument(context, document);
}

void FitzPlugin::lock(void* user, int lock)
{
    reinterpret_cast< FitzPlugin* >(user)->m_mutex[lock].lock();
}

void FitzPlugin::unlock(void* user, int lock)
{
    reinterpret_cast< FitzPlugin* >(user)->m_mutex[lock].unlock();
}

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)

Q_EXPORT_PLUGIN2(qpdfview_fitz, FitzPlugin)

#endif // QT_VERSION
