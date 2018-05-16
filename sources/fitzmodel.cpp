/*

Copyright 2015 Martin Banky
Copyright 2014-2015 Adam Reichold

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

extern "C"
{

#include <mupdf/fitz/bidi.h>
#include <mupdf/fitz/output.h>
#include <mupdf/fitz/display-list.h>
#include <mupdf/fitz/document.h>
#include <mupdf/fitz/pool.h>
#include <mupdf/fitz/structured-text.h>

typedef struct pdf_document_s pdf_document;

pdf_document* pdf_specifics(fz_context*, fz_document*);

}

namespace
{

const int maxSearchResultPerPage = 20;

using namespace qpdfview;
using namespace qpdfview::Model;

Outline loadOutline(fz_outline* item)
{
    Outline outline;

    for(; item; item = item->next)
    {
        outline.push_back(Section());
        Section& section = outline.back();
        section.title = QString::fromUtf8(item->title);

        if(item->page != -1)
        {
            section.link.page = item->page + 1;
        }

        if(fz_outline* childItem = item->down)
        {
            section.children = loadOutline(childItem);
        }
    }

    return outline;
}

} // anonymous

namespace qpdfview
{

namespace Model
{

FitzPage::FitzPage(const FitzDocument* parent, fz_page* page) :
    m_parent(parent),
    m_page(page)
{
}

FitzPage::~FitzPage()
{
    fz_drop_page(m_parent->m_context, m_page);
}

QSizeF FitzPage::size() const
{
    QMutexLocker mutexLocker(&m_parent->m_mutex);

    fz_rect rect;
    fz_bound_page(m_parent->m_context, m_page, &rect);

    return QSizeF(rect.x1 - rect.x0, rect.y1 - rect.y0);
}

QImage FitzPage::render(qreal horizontalResolution, qreal verticalResolution, Rotation rotation, QRect boundingRect) const
{
    QMutexLocker mutexLocker(&m_parent->m_mutex);

    fz_matrix matrix;

    fz_scale(&matrix, horizontalResolution / 72.0f, verticalResolution / 72.0f);

    switch(rotation)
    {
    default:
    case RotateBy0:
        fz_pre_rotate(&matrix, 0.0);
        break;
    case RotateBy90:
        fz_pre_rotate(&matrix, 90.0);
        break;
    case RotateBy180:
        fz_pre_rotate(&matrix, 180.0);
        break;
    case RotateBy270:
        fz_pre_rotate(&matrix, 270.0);
        break;
    }

    fz_rect rect;
    fz_bound_page(m_parent->m_context, m_page, &rect);
    fz_transform_rect(&rect, &matrix);

    fz_irect irect;
    fz_round_rect(&irect, &rect);


    fz_context* context = fz_clone_context(m_parent->m_context);
    fz_display_list* display_list = fz_new_display_list(context, &rect);

    fz_device* device = fz_new_list_device(context, display_list);
    fz_run_page(m_parent->m_context, m_page, device, &matrix, 0);
    fz_close_device(m_parent->m_context, device);
    fz_drop_device(m_parent->m_context, device);


    mutexLocker.unlock();


    fz_matrix tileMatrix;
    fz_translate(&tileMatrix, -rect.x0, -rect.y0);

    fz_rect tileRect = fz_infinite_rect;

    int tileWidth = irect.x1 - irect.x0;
    int tileHeight = irect.y1 - irect.y0;

    if(!boundingRect.isNull())
    {
        fz_pre_translate(&tileMatrix, -boundingRect.x(), -boundingRect.y());

        tileRect.x0 = tileRect.y0 = 0.0;

        tileWidth = tileRect.x1 = boundingRect.width();
        tileHeight = tileRect.y1 = boundingRect.height();
    }


    QImage image(tileWidth, tileHeight, QImage::Format_RGB32);
    image.fill(m_parent->m_paperColor);

    int imageWidth = image.width();
    fz_colorspace* colorSpace = fz_device_bgr(context);

    int n = fz_colorspace_n(context, colorSpace);
    int stride = (n + 1) * imageWidth;

    fz_pixmap* pixmap = fz_new_pixmap_with_data(context, colorSpace, imageWidth, image.height(), NULL, 1, stride, image.bits());

    device = fz_new_draw_device(context, &tileMatrix, pixmap);
    fz_run_display_list(context, display_list, device, &tileMatrix, &tileRect, 0);
    fz_close_device(context, device);
    fz_drop_device(context, device);

    fz_drop_pixmap(context, pixmap);
    fz_drop_display_list(context, display_list);
    fz_drop_context(context);

    return image;
}

QList< Link* > FitzPage::links() const
{
    QMutexLocker mutexLocker(&m_parent->m_mutex);

    QList< Link* > links;

    fz_rect rect;
    fz_bound_page(m_parent->m_context, m_page, &rect);

    const qreal width = qAbs(rect.x1 - rect.x0);
    const qreal height = qAbs(rect.y1 - rect.y0);

    fz_link* first_link = fz_load_links(m_parent->m_context, m_page);

    for(fz_link* link = first_link; link != 0; link = link->next)
    {
        const QRectF boundary = QRectF(link->rect.x0 / width, link->rect.y0 / height, (link->rect.x1 - link->rect.x0) / width, (link->rect.y1 - link->rect.y0) / height).normalized();

        if (link->uri != NULL)
        {
            if (fz_is_external_link(m_parent->m_context, link->uri) == 0)
            {
                float xp;
                float yp;
                const int page = fz_resolve_link(m_parent->m_context, m_parent->m_document, link->uri, &xp, &yp);

                if (page != -1)
                {
                    links.append(new Link(boundary, page + 1, xp / width, yp / height));
                }
            }
            else
            {
                links.append(new Link(boundary, QString::fromUtf8(link->uri)));
            }
        }
    }

    fz_drop_link(m_parent->m_context, first_link);

    return links;
}

QString FitzPage::text(const QRectF &rect) const
{
    QMutexLocker mutexLocker(&m_parent->m_mutex);

    fz_rect mediabox;
    mediabox.x0 = rect.x();
    mediabox.y0 = rect.y();
    mediabox.x1 = rect.right();
    mediabox.y1 = rect.bottom();

    fz_stext_page* stext_page = fz_new_stext_page(m_parent->m_context, &mediabox);
    fz_device* device = fz_new_stext_device(m_parent->m_context, stext_page, NULL);
    fz_run_page(m_parent->m_context, m_page, device, &fz_identity, NULL);
    fz_close_device(m_parent->m_context, device);
    fz_drop_device(m_parent->m_context, device);

    fz_point topLeft;
    topLeft.x = rect.x();
    topLeft.y = rect.y();

    fz_point bottomRight;
    bottomRight.x = rect.right();
    bottomRight.y = rect.bottom();

    char* selection = fz_copy_selection(m_parent->m_context, stext_page, topLeft, bottomRight, 0);
    QString text = QString::fromUtf8(selection);

    fz_drop_stext_page(m_parent->m_context, stext_page);

    return text;
}

QList<QRectF> FitzPage::search(const QString& text, bool matchCase, bool wholeWords) const
{
    Q_UNUSED(matchCase);
    Q_UNUSED(wholeWords);

    QMutexLocker mutexLocker(&m_parent->m_mutex);

    fz_rect rect;
    fz_bound_page(m_parent->m_context, m_page, &rect);

    fz_stext_page* stext_page = fz_new_stext_page(m_parent->m_context, &rect);
    fz_device* device = fz_new_stext_device(m_parent->m_context, stext_page, NULL);
    fz_run_page(m_parent->m_context, m_page, device, &fz_identity, NULL);
    fz_close_device(m_parent->m_context, device);
    fz_drop_device(m_parent->m_context, device);

    fz_rect rects[maxSearchResultPerPage];

    int resultCount = fz_search_stext_page(m_parent->m_context, stext_page, text.toUtf8().constData(), rects, maxSearchResultPerPage);

    fz_drop_stext_page(m_parent->m_context, stext_page);

    QList< QRectF > results;
    results.reserve(resultCount);

    for(int i = 0; i < resultCount; ++i)
    {
        results.append(QRectF(rects[i].x0, rects[i].y0, (rects[i].x1 - rects[i].x0), (rects[i].y1 - rects[i].y0)));
    }

    return results;
}

FitzDocument::FitzDocument(fz_context* context, fz_document* document) :
    m_mutex(),
    m_context(context),
    m_document(document),
    m_paperColor(Qt::white)
{
}

FitzDocument::~FitzDocument()
{
    fz_drop_document(m_context, m_document);
    fz_drop_context(m_context);
}

int FitzDocument::numberOfPages() const
{
    QMutexLocker mutexLocker(&m_mutex);

    return fz_count_pages(m_context, m_document);
}

Page* FitzDocument::page(int index) const
{
    QMutexLocker mutexLocker(&m_mutex);

    if(fz_page* page = fz_load_page(m_context, m_document, index))
    {
        return new FitzPage(this, page);
    }

    return 0;
}

bool FitzDocument::canBePrintedUsingCUPS() const
{
    QMutexLocker mutexLocker(&m_mutex);

    return pdf_specifics(m_context, m_document) != 0;
}

void FitzDocument::setPaperColor(const QColor& paperColor)
{
    m_paperColor = paperColor;
}

Outline FitzDocument::outline() const
{
    Outline outline;

    QMutexLocker mutexLocker(&m_mutex);

    if(fz_outline* rootItem = fz_load_outline(m_context, m_document))
    {
        outline = loadOutline(rootItem);

        fz_drop_outline(m_context, rootItem);
    }

    return outline;
}

} // Model

FitzPlugin::FitzPlugin(QObject* parent) : QObject(parent)
{
    setObjectName("FitzPlugin");

    m_locks_context.user = this;
    m_locks_context.lock = FitzPlugin::lock;
    m_locks_context.unlock = FitzPlugin::unlock;

    m_context = fz_new_context(0, &m_locks_context, FZ_STORE_DEFAULT);

    fz_register_document_handlers(m_context);
}

FitzPlugin::~FitzPlugin()
{
    fz_drop_context(m_context);
}

Model::Document* FitzPlugin::loadDocument(const QString& filePath) const
{
    fz_context* context = fz_clone_context(m_context);

    if(context == 0)
    {
        return 0;
    }

#ifdef _MSC_VER

    fz_document* document = fz_open_document(context, filePath.toUtf8());

#else

    fz_document* document = fz_open_document(context, QFile::encodeName(filePath));

#endif // _MSC_VER

    if(document == 0)
    {
        fz_drop_context(context);

        return 0;
    }

    return new Model::FitzDocument(context, document);
}

void FitzPlugin::lock(void* user, int lock)
{
    static_cast< FitzPlugin* >(user)->m_mutex[lock].lock();
}

void FitzPlugin::unlock(void* user, int lock)
{
    static_cast< FitzPlugin* >(user)->m_mutex[lock].unlock();
}

} // qpdfview

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)

Q_EXPORT_PLUGIN2(qpdfview_fitz, qpdfview::FitzPlugin)

#endif // QT_VERSION
