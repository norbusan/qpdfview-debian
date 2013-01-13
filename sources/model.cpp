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

#include "model.h"

#include <QStringList>

QList< Model::Link* > Model::Page::links() const
{
    return QList< Link* >();
}

QString Model::Page::text(const QRectF& rect) const
{
    Q_UNUSED(rect);

    return QString();
}

QList< QRectF > Model::Page::search(const QString& text, bool matchCase) const
{
    Q_UNUSED(text);
    Q_UNUSED(matchCase);

    return QList< QRectF >();
}

QList< Model::Annotation* > Model::Page::annotations() const
{
    return QList< Annotation* >();
}

bool Model::Page::canAddAndRemoveAnnotations() const
{
    return false;
}

Model::Annotation* Model::Page::addTextAnnotation(const QRectF& boundary)
{
    Q_UNUSED(boundary);

    return 0;
}

Model::Annotation* Model::Page::addHighlightAnnotation(const QRectF& boundary)
{
    Q_UNUSED(boundary);

    return 0;
}

void Model::Page::removeAnnotation(Annotation* annotation)
{
    Q_UNUSED(annotation);
}

QList< Model::FormField* > Model::Page::formFields() const
{
     return QList< FormField* >();
}

bool Model::Document::isLocked() const
{
    return false;
}

bool Model::Document::unlock(const QString& password)
{
    Q_UNUSED(password);

    return false;
}

QStringList Model::Document::saveFilter() const
{
    return QStringList();
}

bool Model::Document::canSave() const
{
    return false;
}

bool Model::Document::save(const QString& filePath, bool withChanges) const
{
    Q_UNUSED(filePath);
    Q_UNUSED(withChanges);

    return false;
}

bool Model::Document::canBePrinted() const
{
    return false;
}

void Model::Document::setAntialiasing(bool on)
{
    Q_UNUSED(on);
}

void Model::Document::setTextAntialiasing(bool on)
{
    Q_UNUSED(on);
}

void Model::Document::setTextHinting(bool on)
{
    Q_UNUSED(on);
}

void Model::Document::setOverprintPreview(bool on)
{
    Q_UNUSED(on);
}

void Model::Document::setPaperColor(const QColor& paperColor)
{
    Q_UNUSED(paperColor);
}

void Model::Document::loadOutline(QStandardItemModel* outlineModel) const
{
    Q_UNUSED(outlineModel);
}

void Model::Document::loadProperties(QStandardItemModel* propertiesModel) const
{
    Q_UNUSED(propertiesModel);
}

void Model::Document::loadFonts(QStandardItemModel* fontsModel) const
{
    Q_UNUSED(fontsModel);
}
