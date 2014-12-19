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

extern "C"
{

#include <mupdf/fitz/display-list.h>
#include <mupdf/fitz/document.h>

typedef struct pdf_document_s pdf_document;

pdf_document* pdf_specifics(fz_document*);

}

namespace
{

using namespace qpdfview;
using namespace qpdfview::Model;

void loadOutline(fz_outline* outline, QStandardItem* parent)
{
    QStandardItem* item = new QStandardItem(QString::fromUtf8(outline->title));
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

    if(outline->dest.kind != FZ_LINK_NONE)
    {
        const int page = outline->dest.ld.gotor.page + 1;

        item->setData(page, PageRole);

        QStandardItem* pageItem = item->clone();
        pageItem->setText(QString::number(page));
        pageItem->setTextAlignment(Qt::AlignRight);

        parent->appendRow(QList< QStandardItem* >() << item << pageItem);
    }
    else
    {
        parent->appendRow(item);
    }

    if(outline->next != 0)
    {
        loadOutline(outline->next, parent);
    }

    if(outline->down != 0)
    {
        loadOutline(outline->down, item);
    }
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
    fz_free_page(m_parent->m_document, m_page);
}

QSizeF FitzPage::size() const
{
    QMutexLocker mutexLocker(&m_parent->m_mutex);

    fz_rect rect;
    fz_bound_page(m_parent->m_document, m_page, &rect);

    return QSizeF(rect.x1 - rect.x0, rect.y1 - rect.y0);
}

QImage FitzPage::render(qreal horizontalResolution, qreal verticalResolution, Rotation rotation, const QRect& boundingRect) const
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
    fz_bound_page(m_parent->m_document, m_page, &rect);
    fz_transform_rect(&rect, &matrix);

    fz_irect irect;
    fz_round_rect(&irect, &rect);


    fz_context* context = fz_clone_context(m_parent->m_context);
    fz_display_list* display_list = fz_new_display_list(context);

    fz_device* device = fz_new_list_device(context, display_list);
    fz_run_page(m_parent->m_document, m_page, device, &matrix, 0);
    fz_free_device(device);


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

    fz_pixmap* pixmap = fz_new_pixmap_with_data(context, fz_device_bgr(context), image.width(), image.height(), image.bits());

    device = fz_new_draw_device(context, pixmap);
    fz_run_display_list(display_list, device, &tileMatrix, &tileRect, 0);
    fz_free_device(device);

    fz_drop_pixmap(context, pixmap);
    fz_drop_display_list(context, display_list);
    fz_free_context(context);

    return image;
}

QList< Link* > FitzPage::links() const
{
    QMutexLocker mutexLocker(&m_parent->m_mutex);

    QList< Link* > links;

    fz_rect rect;
    fz_bound_page(m_parent->m_document, m_page, &rect);

    const qreal width = qAbs(rect.x1 - rect.x0);
    const qreal height = qAbs(rect.y1 - rect.y0);

    fz_link* first_link = fz_load_links(m_parent->m_document, m_page);

    for(fz_link* link = first_link; link != 0; link = link->next)
    {
        const QRectF boundary = QRectF(link->rect.x0 / width, link->rect.y0 / height, (link->rect.x1 - link->rect.x0) / width, (link->rect.y1 - link->rect.y0) / height).normalized();

        if(link->dest.kind == FZ_LINK_GOTO)
        {
            const int page = link->dest.ld.gotor.page + 1;

            links.append(new Link(boundary, page));
        }
        else if(link->dest.kind == FZ_LINK_GOTOR)
        {
            const int page = link->dest.ld.gotor.page + 1;

            if(link->dest.ld.gotor.file_spec != 0)
            {
                links.append(new Link(boundary, QString::fromUtf8(link->dest.ld.gotor.file_spec), page));
            }
            else
            {
                links.append(new Link(boundary, page));
            }
        }
        else if(link->dest.kind == FZ_LINK_URI)
        {
            const QString url = QString::fromUtf8(link->dest.ld.uri.uri);

            links.append(new Link(boundary, url));
        }
        else if(link->dest.kind == FZ_LINK_LAUNCH)
        {
            const QString url = QString::fromUtf8(link->dest.ld.launch.file_spec);

            links.append(new Link(boundary, url));
        }
    }

    fz_drop_link(m_parent->m_context, first_link);

    return links;
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
    fz_close_document(m_document);
    fz_free_context(m_context);
}

int FitzDocument::numberOfPages() const
{
    QMutexLocker mutexLocker(&m_mutex);

    return fz_count_pages(m_document);
}

Page* FitzDocument::page(int index) const
{
    QMutexLocker mutexLocker(&m_mutex);

    fz_page* page = fz_load_page(m_document, index);

    return page != 0 ? new FitzPage(this, page) : 0;
}

bool FitzDocument::canBePrintedUsingCUPS() const
{
    QMutexLocker mutexLocker(&m_mutex);

    return pdf_specifics(m_document) != 0;
}

void FitzDocument::setPaperColor(const QColor& paperColor)
{
    m_paperColor = paperColor;
}

void FitzDocument::loadOutline(QStandardItemModel* outlineModel) const
{
    Document::loadOutline(outlineModel);

    QMutexLocker mutexLocker(&m_mutex);

    fz_outline* outline = fz_load_outline(m_document);

    if(outline != 0)
    {
        ::loadOutline(outline, outlineModel->invisibleRootItem());

        fz_free_outline(m_context, outline);
    }
}

} // Model

FitzPlugin::FitzPlugin(QObject* parent) : QObject(parent)
{
    setObjectName("FitzPlugin");

    m_locks_context.user = reinterpret_cast< void* >(this);
    m_locks_context.lock = FitzPlugin::lock;
    m_locks_context.unlock = FitzPlugin::unlock;

    m_context = fz_new_context(0, &m_locks_context, FZ_STORE_DEFAULT);

    fz_register_document_handlers(m_context);
}

FitzPlugin::~FitzPlugin()
{
    fz_free_context(m_context);
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

} // qpdfview

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)

Q_EXPORT_PLUGIN2(qpdfview_fitz, qpdfview::FitzPlugin)

#endif // QT_VERSION
