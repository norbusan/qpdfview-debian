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

namespace qpdfview
{

namespace Model
{

ImagePage::ImagePage(QImage image) :
    m_image(image)
{
}

QSizeF ImagePage::size() const
{
    return QSizeF(m_image.width(), m_image.height());
}

QImage ImagePage::render(qreal horizontalResolution, qreal verticalResolution, Rotation rotation, const QRect& boundingRect) const
{
    QTransform transform;

    transform.scale(horizontalResolution / 72.0, verticalResolution / 72.0);

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

ImageDocument::ImageDocument(QImage image) :
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
