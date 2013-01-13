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

#include "djvumodel.h"

#include <QFile>
#include <QImage>
#include <qmath.h>

#include <libdjvu/ddjvuapi.h>

static void clear_message_queue(ddjvu_context_t* context, bool wait)
{
    if(wait)
    {
        ddjvu_message_wait(context);
    }

    while(true)
    {
        if(ddjvu_message_peek(context) != 0)
        {
            ddjvu_message_pop(context);
        }
        else
        {
            break;
        }
    }
}

static void wait_for_message_tag(ddjvu_context_t* context, ddjvu_message_tag_t tag)
{
    ddjvu_message_wait(context);

    while(true)
    {
        ddjvu_message_t* message = ddjvu_message_peek(context);

        if(message != 0)
        {
            if(message->m_any.tag == tag)
            {
                break;
            }

            ddjvu_message_pop(context);
        }
        else
        {
            break;
        }
    }
}

Model::DjVuPage::DjVuPage(QMutex* mutex, ddjvu_context_t* context, ddjvu_format_t* format, ddjvu_document_t* document, int index, const QSizeF& size) :
    m_mutex(mutex),
    m_context(context),
    m_format(format),
    m_document(document),
    m_index(index),
    m_size(size)
{
}

Model::DjVuPage::~DjVuPage()
{
}

QSizeF Model::DjVuPage::size() const
{
    return m_size;
}

QImage Model::DjVuPage::render(qreal horizontalResolution, qreal verticalResolution, Rotation rotation, const QRect& boundingRect) const
{
    QMutexLocker mutexLocker(m_mutex);

    ddjvu_status_t status;
    ddjvu_page_t* page = ddjvu_page_create_by_pageno(m_document, m_index);

    if(page == 0)
    {
        return QImage();
    }

    while(true)
    {
        status = ddjvu_page_decoding_status(page);

        if(status < DDJVU_JOB_OK)
        {
            clear_message_queue(m_context, true);
        }
        else
        {
            break;
        }
    }

    if(status >= DDJVU_JOB_FAILED)
    {
        ddjvu_page_release(page);
        return QImage();
    }

    ddjvu_page_rotation_t rot;

    switch(rotation)
    {
    default:
    case RotateBy0:
        rot = DDJVU_ROTATE_0;
        break;
    case RotateBy90:
        rot = DDJVU_ROTATE_270;
        break;
    case RotateBy180:
        rot = DDJVU_ROTATE_180;
        break;
    case RotateBy270:
        rot = DDJVU_ROTATE_90;
        break;
    }

    ddjvu_page_set_rotation(page, rot);

    ddjvu_rect_t pagerect;

    pagerect.x = 0;
    pagerect.y = 0;

    switch(rotation)
    {
    default:
    case RotateBy0:
    case RotateBy180:
        pagerect.w = qRound(horizontalResolution / 72.0 * m_size.width());
        pagerect.h = qRound(verticalResolution / 72.0 * m_size.height());
        break;
    case RotateBy90:
    case RotateBy270:
        pagerect.w = qRound(horizontalResolution / 72.0 * m_size.height());
        pagerect.h = qRound(verticalResolution / 72.0 * m_size.width());
        break;
    }

    ddjvu_rect_t renderrect;

    if(boundingRect.isNull())
    {
        renderrect.x = pagerect.x;
        renderrect.y = pagerect.y;
        renderrect.w = pagerect.w;
        renderrect.h = pagerect.h;
    }
    else
    {
        renderrect.x = boundingRect.x();
        renderrect.y = boundingRect.y();
        renderrect.w = boundingRect.width();
        renderrect.h = boundingRect.height();
    }

    QImage image(renderrect.w, renderrect.h, QImage::Format_RGB32);

    int ok = ddjvu_page_render(page, DDJVU_RENDER_COLOR, &pagerect, &renderrect, m_format, image.bytesPerLine(), reinterpret_cast< char* >(image.bits()));

    clear_message_queue(m_context, false);

    ddjvu_page_release(page);
    return ok == FALSE ? QImage() : image;
}

Model::DjVuDocument::DjVuDocument(ddjvu_context_t* context, ddjvu_format_t* format, ddjvu_document_t* document) :
    m_mutex(),
    m_context(context),
    m_format(format),
    m_document(document)
{
}

Model::DjVuDocument::~DjVuDocument()
{
    ddjvu_document_release(m_document);
    ddjvu_format_release(m_format);
    ddjvu_context_release(m_context);
}

int Model::DjVuDocument::numberOfPages() const
{
    QMutexLocker mutexLocker(&m_mutex);

    return ddjvu_document_get_pagenum(m_document);
}

Model::Page* Model::DjVuDocument::page(int index) const
{
    QMutexLocker mutexLocker(&m_mutex);

    ddjvu_status_t status;
    ddjvu_pageinfo_t pageinfo;

    while(true)
    {
        status = ddjvu_document_get_pageinfo(m_document, index, &pageinfo);

        if(status < DDJVU_JOB_OK)
        {
            clear_message_queue(m_context, true);
        }
        else
        {
            break;
        }
    }

    if(status >= DDJVU_JOB_FAILED)
    {
        return 0;
    }

    return new DjVuPage(&m_mutex, m_context, m_format, m_document, index, 72.0 / pageinfo.dpi * QSizeF(pageinfo.width, pageinfo.height));
}

Model::DjVuDocumentLoader::DjVuDocumentLoader(QObject* parent) : QObject(parent)
{
    setObjectName("DjVuDocumentLoader");
}

Model::Document* Model::DjVuDocumentLoader::loadDocument(const QString& filePath) const
{
    ddjvu_context_t* context = ddjvu_context_create("qpdfview");

    unsigned int mask[] = {0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000};

    ddjvu_format_t* format = ddjvu_format_create(DDJVU_FORMAT_RGBMASK32, 4, mask);
    ddjvu_format_set_row_order(format, 1);
    ddjvu_format_set_y_direction(format, 1);

    ddjvu_document_t* document = ddjvu_document_create_by_filename(context, QFile::encodeName(filePath), FALSE);

    if(document == 0)
    {
        ddjvu_format_release(format);
        ddjvu_context_release(context);
        return 0;
    }

    wait_for_message_tag(context, DDJVU_DOCINFO);

    if(ddjvu_document_decoding_error(document))
    {
        ddjvu_document_release(document);
        ddjvu_format_release(format);
        ddjvu_context_release(context);
        return 0;
    }

    return new DjVuDocument(context, format, document);
}

Q_EXPORT_PLUGIN2(qpdfview_djvu, Model::DjVuDocumentLoader)
