/*

Copyright 2012 Adam Reichold

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

#include "model.h"

#include <QStringList>

QList< Link* > Page::links() const
{
    return QList< Link* >();
}

QString Page::text(const QRectF& rect) const
{
    Q_UNUSED(rect);

    return QString();
}

QList< QRectF > Page::search(const QString& text, bool matchCase) const
{
    Q_UNUSED(text);
    Q_UNUSED(matchCase);

    return QList< QRectF >();
}

QList< Annotation* > Page::annotations() const
{
    return QList< Annotation* >();
}

bool Page::canAddAndRemoveAnnotations() const
{
    return false;
}

Annotation* Page::addTextAnnotation(const QRectF& boundary)
{
    Q_UNUSED(boundary);

    return 0;
}

Annotation* Page::addHighlightAnnotation(const QRectF& boundary)
{
    Q_UNUSED(boundary);

    return 0;
}

void Page::removeAnnotation(Annotation* annotation)
{
    Q_UNUSED(annotation);
}

QList< FormField* > Page::formFields() const
{
     return QList< FormField* >();
}

bool Document::isLocked() const
{
    return false;
}

bool Document::unlock(const QString& password)
{
    Q_UNUSED(password);

    return false;
}

QStringList Document::saveFilter() const
{
    return QStringList();
}

bool Document::canSave() const
{
    return false;
}

bool Document::save(const QString& filePath, bool withChanges) const
{
    Q_UNUSED(filePath);
    Q_UNUSED(withChanges);

    return false;
}

bool Document::canBePrinted() const
{
    return false;
}

void Document::setAntialiasing(bool on)
{
    Q_UNUSED(on);
}

void Document::setTextAntialiasing(bool on)
{
    Q_UNUSED(on);
}

void Document::setTextHinting(bool on)
{
    Q_UNUSED(on);
}

void Document::setOverprintPreview(bool on)
{
    Q_UNUSED(on);
}

void Document::setPaperColor(const QColor& paperColor)
{
    Q_UNUSED(paperColor);
}

void Document::loadOutline(QStandardItemModel* outlineModel) const
{
    Q_UNUSED(outlineModel);
}

void Document::loadProperties(QStandardItemModel* propertiesModel) const
{
    Q_UNUSED(propertiesModel);
}

void Document::loadFonts(QStandardItemModel* fontsModel) const
{
    Q_UNUSED(fontsModel);
}
