/*

Copyright 2013 Adam Reichold
Copyright 2013 Alexander Volkov

This file is part of qpdfview.

The implementation is based on KDjVu by Pino Toscano.

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

#include <cstdio>

#include <QFile>
#include <qmath.h>

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

    switch(rotation)
    {
    default:
    case RotateBy0:
        ddjvu_page_set_rotation(page, DDJVU_ROTATE_0);
        break;
    case RotateBy90:
        ddjvu_page_set_rotation(page, DDJVU_ROTATE_270);
        break;
    case RotateBy180:
        ddjvu_page_set_rotation(page, DDJVU_ROTATE_180);
        break;
    case RotateBy270:
        ddjvu_page_set_rotation(page, DDJVU_ROTATE_90);
        break;
    }

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

    if(!ddjvu_page_render(page, DDJVU_RENDER_COLOR, &pagerect, &renderrect, m_parent->m_format, image.bytesPerLine(), reinterpret_cast< char* >(image.bits())))
    {
        image = QImage();
    }

    clearMessageQueue(m_parent->m_context, false);

    ddjvu_page_release(page);

    return image;
}

QList< Model::Link* > Model::DjVuPage::links() const
{
    QMutexLocker mutexLocker(&m_parent->m_mutex);

    QList< Link* > links;

    miniexp_t pageAnnoExp;

    while(true)
    {
        pageAnnoExp = ddjvu_document_get_pageanno(m_parent->m_document, m_index);

        if(pageAnnoExp == miniexp_dummy)
        {
            clearMessageQueue(m_parent->m_context, true);
        }
        else
        {
            break;
        }
    }

    const int pageAnnoLength = miniexp_length(pageAnnoExp);

    for(int pageAnnoN = 0; pageAnnoN < pageAnnoLength; ++pageAnnoN)
    {
        miniexp_t linkExp = miniexp_nth(pageAnnoN, pageAnnoExp);

        if(miniexp_length(linkExp) <= 3 || qstrncmp(miniexp_to_name(miniexp_nth(0, linkExp)), "maparea", 7 ) != 0 || !miniexp_symbolp(miniexp_nth(0, miniexp_nth(3, linkExp))))
        {
            continue;
        }

        const QString type = QString::fromUtf8(miniexp_to_name(miniexp_nth(0, miniexp_nth(3, linkExp))));

        if(type == QLatin1String("rect") || type == QLatin1String("oval") || type == QLatin1String("poly"))
        {
            // boundary

            QPainterPath boundary;

            miniexp_t areaExp = miniexp_nth(3, linkExp);
            const int areaLength = miniexp_length( areaExp );

            if(areaLength == 5 && (type == QLatin1String("rect") || type == QLatin1String("oval")))
            {
                QPoint p(miniexp_to_int(miniexp_nth(1, areaExp)), miniexp_to_int(miniexp_nth(2, areaExp)));
                QSize s(miniexp_to_int(miniexp_nth(3, areaExp)), miniexp_to_int(miniexp_nth(4, areaExp)));

                p.setY(m_size.height() - s.height() - p.y());

                const QRectF r(p, s);

                if(type == QLatin1String("rect"))
                {
                    boundary.addRect(r);
                }
                else
                {
                    boundary.addEllipse(r);
                }
            }
            else if(areaLength > 0 && areaLength % 2 == 1 && type == QLatin1String("poly"))
            {
                QPolygon polygon;

                for(int areaExpN = 1; areaExpN < areaLength; areaExpN += 2)
                {
                    QPoint p(miniexp_to_int(miniexp_nth(areaExpN, areaExp)), miniexp_to_int(miniexp_nth(areaExpN + 1, areaExp)));

                    p.setY(m_size.height() - p.y());

                    polygon << p;
                }

                boundary.addPolygon(polygon);
            }

            if(boundary.isEmpty())
            {
                continue;
            }

            boundary = QTransform::fromScale(1.0 / m_size.width(), 1.0 / m_size.height()).map(boundary);

            // target

            QString target;

            miniexp_t targetExp = miniexp_nth(1, linkExp);

            if(miniexp_stringp(targetExp))
            {
                target = QString::fromUtf8(miniexp_to_str(miniexp_nth(1, linkExp)));
            }
            else if(miniexp_length(targetExp) == 3 && qstrncmp(miniexp_to_name(miniexp_nth(0, targetExp)), "url", 3) == 0)
            {
                target = QString::fromUtf8(miniexp_to_str(miniexp_nth(1, targetExp)));
            }

            if(target.isEmpty())
            {
                continue;
            }

            if(target.at(0) == QLatin1Char('#'))
            {
                target.remove(0, 1);

                bool ok = false;
                int targetPage = target.toInt(&ok);

                if(!ok)
                {
                    if(m_parent->m_indexByName.contains(target))
                    {
                        targetPage = m_parent->m_indexByName[target] + 1;
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

                links.append(new Link(boundary, targetPage));
            }
            else
            {
                links.append(new Link(boundary, target));
            }
        }
    }

    ddjvu_miniexp_release(m_parent->m_document, pageAnnoExp);

    return links;
}

static QString loadText(miniexp_t textExp, const QRect& rect, int pageHeight)
{
    const int textLength = miniexp_length(textExp);

    if(textLength >= 6 && miniexp_symbolp(miniexp_nth(0, textExp)))
    {
        const int xmin = miniexp_to_int(miniexp_nth(1, textExp));
        const int ymin = miniexp_to_int(miniexp_nth(2, textExp));
        const int xmax = miniexp_to_int(miniexp_nth(3, textExp));
        const int ymax = miniexp_to_int(miniexp_nth(4, textExp));

        if(rect.intersects(QRect(xmin, pageHeight - ymax, xmax - xmin, ymax - ymin)))
        {
            if(qstrncmp(miniexp_to_name(miniexp_nth(0, textExp)), "word", 4) == 0)
            {
                return QString::fromUtf8(miniexp_to_str(miniexp_nth(5, textExp)));
            }
            else
            {
                QStringList text;

                for(int textN = 5; textN < textLength; ++textN)
                {
                    text.append(loadText(miniexp_nth(textN, textExp), rect, pageHeight));
                }

                if(qstrncmp(miniexp_to_name(miniexp_nth(0, textExp)), "line", 4) == 0)
                {
                    return text.join(" ");
                }
                else
                {
                    return text.join("\n");
                }
            }
        }
    }

    return QString();
}

QString Model::DjVuPage::text(const QRectF& rect) const
{
    QMutexLocker mutexLocker(&m_parent->m_mutex);

    miniexp_t pageTextExp;

    while(true)
    {
        pageTextExp = ddjvu_document_get_pagetext(m_parent->m_document, m_index, "word");

        if(pageTextExp == miniexp_dummy)
        {
            clearMessageQueue(m_parent->m_context, true);
        }
        else
        {
            break;
        }
    }

    const QString text = loadText(pageTextExp, QTransform::fromScale(m_resolution / 72.0, m_resolution / 72.0).mapRect(rect).toRect(), m_size.height());

    ddjvu_miniexp_release(m_parent->m_document, pageTextExp);

    return text.trimmed();
}

QList< QRectF > Model::DjVuPage::search(const QString& text, bool matchCase) const
{
    QMutexLocker mutexLocker(&m_parent->m_mutex);

    miniexp_t pageTextExp;

    while(true)
    {
        pageTextExp = ddjvu_document_get_pagetext(m_parent->m_document, m_index, "word");

        if(pageTextExp == miniexp_dummy)
        {
            clearMessageQueue(m_parent->m_context, true);
        }
        else
        {
            break;
        }
    }

    QList< miniexp_t > words;
    QList< QRectF > results;

    words.append(pageTextExp);

    QRectF rect;
    int index = 0;

    while(!words.isEmpty())
    {
        miniexp_t textExp = words.takeFirst();

        const int textLength = miniexp_length(textExp);

        if(textLength >= 6 && miniexp_symbolp(miniexp_nth(0, textExp)))
        {
            if(qstrncmp(miniexp_to_name(miniexp_nth(0, textExp)), "word", 4) == 0)
            {
                const QString word = QString::fromUtf8(miniexp_to_str(miniexp_nth(5, textExp)));

                if(text.indexOf(word, index, matchCase ? Qt::CaseSensitive : Qt::CaseInsensitive) == index)
                {
                    const int xmin = miniexp_to_int(miniexp_nth(1, textExp));
                    const int ymin = miniexp_to_int(miniexp_nth(2, textExp));
                    const int xmax = miniexp_to_int(miniexp_nth(3, textExp));
                    const int ymax = miniexp_to_int(miniexp_nth(4, textExp));

                    rect = rect.united(QRectF(xmin, m_size.height() - ymax, xmax - xmin, ymax - ymin));

                    index += word.length();

                    while(text.length() > index && text.at(index).isSpace())
                    {
                        ++index;
                    }

                    if(text.length() == index)
                    {
                        results.append(rect);

                        rect = QRectF();
                        index = 0;
                    }
                }
                else
                {
                    rect = QRectF();
                    index = 0;
                }
            }
            else
            {
                for(int textN = 5; textN < textLength; ++textN)
                {
                    words.append(miniexp_nth(textN, textExp));
                }
            }
        }
    }

    ddjvu_miniexp_release(m_parent->m_document, pageTextExp);

    QTransform transform = QTransform::fromScale(72.0 / m_resolution, 72.0 / m_resolution);

    for(int index = 0; index < results.size(); ++index)
    {
        results[index] = transform.mapRect(results[index]);
    }

    return results;
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

    const int fileNum = ddjvu_document_get_filenum(m_document);

    for(int index = 0; index < fileNum; ++index)
    {
        ddjvu_fileinfo_t fileinfo;

        if(ddjvu_document_get_fileinfo(m_document, index, &fileinfo) != DDJVU_JOB_OK || fileinfo.type != 'P')
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

static void loadOutline(miniexp_t outlineExp, int offset, QStandardItem* parent, const QHash< QString, int >& indexByName)
{
    const int outlineLength = miniexp_length(outlineExp);

    for(int outlineN = qMax(0, offset); outlineN < outlineLength; ++outlineN)
    {
        miniexp_t bookmarkExp = miniexp_nth(outlineN, outlineExp);
        const int bookmarkLength = miniexp_length(bookmarkExp);

        if(bookmarkLength <= 1 || !miniexp_stringp(miniexp_nth(0, bookmarkExp)) || !miniexp_stringp(miniexp_nth(1, bookmarkExp)))
        {
            continue;
        }

        const QString title = QString::fromUtf8(miniexp_to_str(miniexp_nth(0, bookmarkExp)));
        QString destination = QString::fromUtf8(miniexp_to_str(miniexp_nth(1, bookmarkExp)));

        if(!title.isEmpty() && !destination.isEmpty())
        {
            if(destination.at(0) == QLatin1Char('#'))
            {
                destination.remove(0,1);

                bool ok = false;
                int destinationPage = destination.toInt(&ok);

                if(!ok)
                {
                    if(indexByName.contains(destination))
                    {
                        destinationPage = indexByName[destination] + 1;
                    }
                    else
                    {
                        continue;
                    }
                }

                QStandardItem* item = new QStandardItem(title);
                item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

                item->setData(destinationPage, Qt::UserRole + 1);

                QStandardItem* pageItem = item->clone();
                pageItem->setText(QString::number(destinationPage));
                pageItem->setTextAlignment(Qt::AlignRight);

                parent->appendRow(QList< QStandardItem* >() << item << pageItem);

                if(bookmarkLength >= 3)
                {
                    loadOutline(bookmarkExp, 2, item, indexByName);
                }
            }
        }
    }
}

void Model::DjVuDocument::loadOutline(QStandardItemModel* outlineModel) const
{
    Document::loadOutline(outlineModel);

    QMutexLocker mutexLocker(&m_mutex);

    miniexp_t outlineExp;

    while(true)
    {
        outlineExp = ddjvu_document_get_outline(m_document);

        if(outlineExp == miniexp_dummy)
        {
            clearMessageQueue(m_context, true);
        }
        else
        {
            break;
        }
    }

    if(miniexp_length(outlineExp) <= 1)
    {
        return;
    }

    if(qstrncmp(miniexp_to_name(miniexp_nth(0, outlineExp)), "bookmarks", 9) != 0)
    {
        return;
    }

    ::loadOutline(outlineExp, 1, outlineModel->invisibleRootItem(), m_indexByName);

    ddjvu_miniexp_release(m_document, outlineExp);
}

void Model::DjVuDocument::loadProperties(QStandardItemModel* propertiesModel) const
{
    Document::loadProperties(propertiesModel);

    QMutexLocker mutexLocker(&m_mutex);

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

    const int annoLength = miniexp_length(annoExp);

    for(int annoN = 0; annoN < annoLength; ++annoN)
    {
        miniexp_t listExp = miniexp_nth(annoN, annoExp);
        const int listLength = miniexp_length(listExp);

        if(listLength <= 1 || qstrncmp(miniexp_to_name(miniexp_nth(0, listExp)), "metadata", 8) != 0)
        {
            continue;
        }

        for(int listN = 1; listN < listLength; ++listN)
        {
            miniexp_t keyValueExp = miniexp_nth(listN, listExp);

            if(miniexp_length(keyValueExp) != 2)
            {
                continue;
            }

            const QString key = QString::fromUtf8(miniexp_to_name(miniexp_nth(0, keyValueExp)));
            const QString value = QString::fromUtf8(miniexp_to_str(miniexp_nth(1, keyValueExp)));

            if(!key.isEmpty() && !value.isEmpty())
            {
                propertiesModel->appendRow(QList< QStandardItem* >() << new QStandardItem(key) << new QStandardItem(value));
            }
        }
    }

    ddjvu_miniexp_release(m_document, annoExp);
}

DjVuPlugin::DjVuPlugin(QObject* parent) : QObject(parent)
{
    setObjectName("DjVuPlugin");
}

Model::Document* DjVuPlugin::loadDocument(const QString& filePath) const
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

    return new Model::DjVuDocument(context, document);
}

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)

Q_EXPORT_PLUGIN2(qpdfview_djvu, DjVuPlugin)

#endif // QT_VERSION
