/*

Copyright 2013 Adam Reichold
Copyright 2013 Alexander Volkov

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

#include <stdio.h>

#include <QFile>
#include <qmath.h>
#include <QStringList>
#include <QStandardItemModel>

#include <libdjvu/ddjvuapi.h>
#include <libdjvu/miniexp.h>

static void clearMessageQueue(ddjvu_context_t* context, bool wait)
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

static void waitForMessageTag(ddjvu_context_t* context, ddjvu_message_tag_t tag)
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

Model::DjVuPage::DjVuPage(const DjVuDocument* parent, int index, const ddjvu_pageinfo_t& pageinfo) :
    m_parent(parent),
    m_index(index),
    m_size(pageinfo.width, pageinfo.height),
    m_resolution(pageinfo.dpi)
{
}

Model::DjVuPage::~DjVuPage()
{
}

QSizeF Model::DjVuPage::size() const
{
    return 72.0 / m_resolution * m_size;
}

QImage Model::DjVuPage::render(qreal horizontalResolution, qreal verticalResolution, Rotation rotation, const QRect& boundingRect) const
{
    QMutexLocker mutexLocker(&m_parent->m_mutex);

    ddjvu_status_t status;
    ddjvu_page_t* page = ddjvu_page_create_by_pageno(m_parent->m_document, m_index);

    if(page == 0)
    {
        return QImage();
    }

    while(true)
    {
        status = ddjvu_page_decoding_status(page);

        if(status < DDJVU_JOB_OK)
        {
            clearMessageQueue(m_parent->m_context, true);
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
        pagerect.w = qRound(horizontalResolution / m_resolution * m_size.width());
        pagerect.h = qRound(verticalResolution / m_resolution * m_size.height());
        break;
    case RotateBy90:
    case RotateBy270:
        pagerect.w = qRound(horizontalResolution / m_resolution * m_size.height());
        pagerect.h = qRound(verticalResolution / m_resolution * m_size.width());
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

    int ok = ddjvu_page_render(page, DDJVU_RENDER_COLOR, &pagerect, &renderrect, m_parent->m_format, image.bytesPerLine(), reinterpret_cast< char* >(image.bits()));

    clearMessageQueue(m_parent->m_context, false);

    ddjvu_page_release(page);
    return ok == FALSE ? QImage() : image;
}

QList< Model::Link* > Model::DjVuPage::links() const
{
    QList< Link* > links;

    miniexp_t annots;
    while ( ( annots = ddjvu_document_get_pageanno( m_parent->m_document, m_index ) ) == miniexp_dummy )
        clearMessageQueue( m_parent->m_context, true );

    if ( !miniexp_listp( annots ) )
        return links;

    int l = miniexp_length( annots );

    for ( int i = 0; i < l; ++i )
    {
        miniexp_t cur = miniexp_nth( i, annots );
        int num = miniexp_length( cur );
        if ( ( num < 4 ) || !miniexp_symbolp( miniexp_nth( 0, cur ) ) ||
             ( qstrncmp( miniexp_to_name( miniexp_nth( 0, cur ) ), "maparea", 7 ) != 0 ) )
            continue;

        QString type;

        if ( miniexp_symbolp( miniexp_nth( 0, miniexp_nth( 3, cur ) ) ) )
            type = QString::fromUtf8( miniexp_to_name( miniexp_nth( 0, miniexp_nth( 3, cur ) ) ) );

        if ( type == QLatin1String( "rect" ) ||
             type == QLatin1String( "oval" ) ||
             type == QLatin1String( "poly" ) )
        {
            QPainterPath linkBoundary;

            miniexp_t area = miniexp_nth( 3, cur );
            int arealength = miniexp_length( area );
            if ( ( arealength == 5 ) && ( type == QLatin1String( "rect" ) || type == QLatin1String( "oval" ) ) )
            {
                QPoint p = QPoint( miniexp_to_int( miniexp_nth( 1, area ) ), miniexp_to_int( miniexp_nth( 2, area ) ) );
                QSize s = QSize( miniexp_to_int( miniexp_nth( 3, area ) ), miniexp_to_int( miniexp_nth( 4, area ) ) );
                p.setY(m_size.height() - p.y() - s.height());
                QRectF rect(p, s);

                if ( type == QLatin1String( "rect" ) )
                {
                    linkBoundary.addRect(rect);
                }
                else
                {
                    linkBoundary.addEllipse(rect);
                }
            }
            else if ( ( arealength > 0 ) && ( arealength % 2 == 1 ) &&
                      type == QLatin1String( "poly" ) )
            {
                QPolygon poly;
                for ( int j = 1; j < arealength; j += 2 )
                {
                    int x = miniexp_to_int( miniexp_nth( j, area ) );
                    int y = m_size.height() - miniexp_to_int( miniexp_nth( j + 1, area ) );
                    poly << QPoint( x, y );
                }

                linkBoundary.addPolygon(poly);
            }

            if ( linkBoundary.isEmpty() )
                continue;

            linkBoundary = QTransform::fromScale(1.0 / m_size.width(), 1.0 / m_size.height()).map(linkBoundary);

            QString target;

            miniexp_t urlexp = miniexp_nth( 1, cur );
            if ( miniexp_stringp( urlexp ) )
            {
                target = QString::fromUtf8( miniexp_to_str( miniexp_nth( 1, cur ) ) );
            }
            else if ( miniexp_listp( urlexp ) && ( miniexp_length( urlexp ) == 3 ) &&
                      miniexp_symbolp( miniexp_nth( 0, urlexp ) ) &&
                      ( qstrncmp( miniexp_to_name( miniexp_nth( 0, urlexp ) ), "url", 3 ) == 0 ) )
            {
                target = QString::fromUtf8( miniexp_to_str( miniexp_nth( 1, urlexp ) ) );
            }

            if ( target.isEmpty() )
                continue;

            Link* link = 0;

            if( ( target.length() > 0 ) && target.at(0) == QLatin1Char( '#' ) )
            {
                target.remove( 0, 1 );

                bool ok = false;
                int targetPage = target.toInt(&ok);

                if (!ok)
                {
                    if ( m_parent->m_indexByName.contains( target ) )
                    {
                        targetPage = m_parent->m_indexByName[ target ] + 1;
                    }
                    else
                    {
                        continue;
                    }
                }
                else
                {
                    targetPage = (target.at(0) == QLatin1Char('+') || target.at(0) == QLatin1Char('-')) ? m_index + targetPage : targetPage;
                }

                link = new Link(linkBoundary, targetPage);
            }
            else
            {
                link = new Link(linkBoundary, target);
            }

            if (link)
                links.append(link);
        }
    }

    return links;
}

Model::DjVuDocument::DjVuDocument(ddjvu_context_t* context, ddjvu_document_t* document) :
    m_mutex(),
    m_context(context),
    m_document(document),
    m_format(0),
    m_indexByName()
{
    unsigned int mask[] = {0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000};

    m_format = ddjvu_format_create(DDJVU_FORMAT_RGBMASK32, 4, mask);
    ddjvu_format_set_row_order(m_format, 1);
    ddjvu_format_set_y_direction(m_format, 1);

    int fileNum = ddjvu_document_get_filenum(m_document);

    for(int index = 0; index < fileNum; ++index)
    {
        ddjvu_fileinfo_t fileinfo;

        if(ddjvu_document_get_fileinfo(m_document, index, &fileinfo) != DDJVU_JOB_OK)
        {
            continue;
        }

        if(fileinfo.type != 'P')
        {
            continue;
        }

        m_indexByName[QString::fromUtf8(fileinfo.id)] = m_indexByName[QString::fromUtf8(fileinfo.name)] = m_indexByName[QString::fromUtf8(fileinfo.title)] = fileinfo.pageno;
    }
}

Model::DjVuDocument::~DjVuDocument()
{
    ddjvu_document_release(m_document);
    ddjvu_context_release(m_context);
    ddjvu_format_release(m_format);
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
            clearMessageQueue(m_context, true);
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

    return new DjVuPage(this, index, pageinfo);
}

QStringList Model::DjVuDocument::saveFilter() const
{
    return QStringList() << "DjVu (*.djvu *.djv)";
}

bool Model::DjVuDocument::canSave() const
{
    return true;
}

bool Model::DjVuDocument::save(const QString& filePath, bool withChanges) const
{
    Q_UNUSED(withChanges);

    QMutexLocker mutexLocker(&m_mutex);

    FILE* file = fopen(QFile::encodeName(filePath), "w+");

    if(file == 0)
    {
        return false;
    }

    ddjvu_job_t* job = ddjvu_document_save(m_document, file, 0, 0);

    while(!ddjvu_job_done(job))
    {
        clearMessageQueue(m_context, true);
    }

    fclose(file);

    return !ddjvu_job_error(job);
}

void Model::DjVuDocument::loadProperties(QStandardItemModel* propertiesModel) const
{
    QMutexLocker mutexLocker(&m_mutex);

    propertiesModel->clear();
    propertiesModel->setColumnCount(2);

    miniexp_t annoExp;

    while(true)
    {
        annoExp = ddjvu_document_get_anno(m_document, TRUE);

        if(annoExp == miniexp_dummy)
        {
            clearMessageQueue(m_context, true);
        }
        else
        {
            break;
        }
    }

    if(!miniexp_listp(annoExp) || miniexp_length(annoExp) == 0)
    {
        return;
    }

    for(int annoN = 0; annoN < miniexp_length(annoExp); ++annoN)
    {
        miniexp_t listExp = miniexp_nth(annoN, annoExp);

        if(miniexp_length(listExp) <= 1)
        {
            continue;
        }

        if(qstrncmp(miniexp_to_name(miniexp_nth(0, listExp)), "metadata", 8) != 0)
        {
            continue;
        }

        for(int listN = 0; listN < miniexp_length(listExp); ++listN)
        {
            miniexp_t keyValueExp = miniexp_nth(listN, listExp);

            if(miniexp_length(keyValueExp) != 2)
            {
                continue;
            }

            QString key = QString::fromUtf8(miniexp_to_name(miniexp_nth(0, keyValueExp)));
            QString value = QString::fromUtf8(miniexp_to_str(miniexp_nth(1, keyValueExp)));

            if(!key.isEmpty() && !value.isEmpty())
            {
                propertiesModel->appendRow(QList< QStandardItem* >() << new QStandardItem(key) << new QStandardItem(value));
            }
        }
    }
}

Model::DjVuDocumentLoader::DjVuDocumentLoader(QObject* parent) : QObject(parent)
{
    setObjectName("DjVuDocumentLoader");
}

Model::Document* Model::DjVuDocumentLoader::loadDocument(const QString& filePath) const
{
    ddjvu_context_t* context = ddjvu_context_create("qpdfview");
    ddjvu_document_t* document = ddjvu_document_create_by_filename(context, QFile::encodeName(filePath), FALSE);

    if(document == 0)
    {
        ddjvu_context_release(context);
        return 0;
    }

    waitForMessageTag(context, DDJVU_DOCINFO);

    if(ddjvu_document_decoding_error(document))
    {
        ddjvu_document_release(document);
        ddjvu_context_release(context);
        return 0;
    }

    return new DjVuDocument(context, document);
}

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)

Q_EXPORT_PLUGIN2(qpdfview_djvu, Model::DjVuDocumentLoader)

#endif // QT_VERSION
