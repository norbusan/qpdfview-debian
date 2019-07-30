/*

Copyright 2015 Adam Reichold

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

#include "imagemodel.h"

#include <QDebug>
#include <QImageWriter>

namespace
{

using namespace qpdfview;
using namespace qpdfview::Model;

inline qreal dotsPerInchX(const QImage& image)
{
    return 0.0254 * image.dotsPerMeterX();
}

inline qreal dotsPerInchY(const QImage& image)
{
    return 0.0254 * image.dotsPerMeterY();
}

} // anonymous

namespace qpdfview
{

namespace Model
{

ImagePage::ImagePage(const QImage& image) :
    m_image(image)
{
}

QSizeF ImagePage::size() const
{
    return QSizeF(m_image.width() * 72.0 / dotsPerInchX(m_image),
                  m_image.height() * 72.0 / dotsPerInchY(m_image));
}

QImage ImagePage::render(qreal horizontalResolution, qreal verticalResolution, Rotation rotation, QRect boundingRect) const
{
    QTransform transform;

    transform.scale(horizontalResolution / dotsPerInchX(m_image),
                    verticalResolution / dotsPerInchY(m_image));

    switch(rotation)
    {
    default:
    case RotateBy0:
        break;
    case RotateBy90:
        transform.rotate(90.0);
        break;
    case RotateBy180:
        transform.rotate(180.0);
        break;
    case RotateBy270:
        transform.rotate(270.0);
        break;
    }

    QImage image = m_image.transformed(transform, Qt::SmoothTransformation);

    if(!boundingRect.isNull())
    {
        image = image.copy(boundingRect);
    }

    return image;
}

ImageDocument::ImageDocument(const QImage& image) :
    m_image(image)
{
}

int ImageDocument::numberOfPages() const
{
    return 1;
}

Page* ImageDocument::page(int index) const
{
    return index == 0 ? new ImagePage(m_image) : 0;
}

QStringList ImageDocument::saveFilter() const
{
    QStringList formats;

    foreach(const QByteArray& format, QImageWriter::supportedImageFormats())
    {
        const QString name = QString::fromLocal8Bit(format);

        formats.append(QLatin1String("*.") + name);
    }

    return QStringList() << tr("Image (%1)").arg(formats.join(QLatin1String(" ")));
}

bool ImageDocument::canSave() const
{
    return true;
}

bool ImageDocument::save(const QString& filePath, bool withChanges) const
{
    Q_UNUSED(withChanges);

    QImageWriter writer(filePath);

    if(!writer.write(m_image))
    {
        qWarning() << writer.errorString();

        return false;
    }

    return true;
}

Properties ImageDocument::properties() const
{
    Properties properties;

    properties.push_back(qMakePair(tr("Size"), QString("%1 px x %2 px").arg(m_image.width()).arg(m_image.height())));
    properties.push_back(qMakePair(tr("Resolution"), QString("%1 dpi x %2 dpi").arg(dotsPerInchX(m_image), 0, 'f', 1).arg(dotsPerInchY(m_image), 0, 'f', 1)));
    properties.push_back(qMakePair(tr("Depth"), QString("%1 bits").arg(m_image.depth())));

    switch(m_image.format())
    {
    default:
        break;
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
        properties.push_back(qMakePair(tr("Format"), tr("Monochrome")));
        break;
    case QImage::Format_Indexed8:
        properties.push_back(qMakePair(tr("Format"), tr("Indexed")));
        break;
    case QImage::Format_RGB32:
        properties.push_back(qMakePair(tr("Format"), tr("32 bits RGB")));
        break;
    case QImage::Format_ARGB32:
        properties.push_back(qMakePair(tr("Format"), tr("32 bits ARGB")));
        break;
    case QImage::Format_RGB16:
    case QImage::Format_RGB555:
    case QImage::Format_RGB444:
        properties.push_back(qMakePair(tr("Format"), tr("16 bits RGB")));
        break;
    case QImage::Format_RGB666:
    case QImage::Format_RGB888:
        properties.push_back(qMakePair(tr("Format"), tr("24 bits RGB")));
        break;
    }

    foreach(const QString& key, m_image.textKeys())
    {
        properties.push_back(qMakePair(key, m_image.text(key)));
    }

    return properties;
}

} // Model

ImagePlugin::ImagePlugin(QObject* parent) : QObject(parent)
{
    setObjectName("ImagePlugin");
}

Model::Document* ImagePlugin::loadDocument(const QString& filePath) const
{
    QImage image(filePath);

    return !image.isNull() ? new Model::ImageDocument(image) : 0;
}

} // qpdfview

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)

Q_EXPORT_PLUGIN2(qpdfview_image, qpdfview::ImagePlugin)

#endif // QT_VERSION
