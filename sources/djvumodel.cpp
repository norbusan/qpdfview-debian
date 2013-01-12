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

#include <QDebug>

#include <libdjvu/ddjvuapi.h>

DjVuDocumentLoader::DjVuDocumentLoader(QObject* parent) : QObject(parent)
{
    setObjectName("DjVuDocumentLoader");
}

Document* DjVuDocumentLoader::loadDocument(const QString& filePath) const
{
    qDebug() << "DjVu plug-in not implemented yet!";

    return 0;
}

Q_EXPORT_PLUGIN2(qpdfview_djvu, DjVuDocumentLoader)
