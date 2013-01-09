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

#ifndef DOCUMENTMODEL_H
#define DOCUMENTMODEL_H

#include <QList>
#include <QRect>
#include <QRectF>
#include <QString>

class QColor;
class QDialog;
class QImage;
class QPrinter;
class QSizeF;
class QStandardItemModel;

#include "global.h"

struct Link
{
    QRectF boundary;

    int page;
    qreal left;
    qreal top;

    QString url;

    Link() : boundary(), page(-1), left(0.0), top(0.0), url() {}
    Link(const QRectF& boundary, int page, qreal left, qreal top) : boundary(boundary), page(page), left(left), top(top), url() {}
    Link(const QRectF& boundary, const QString& url) : boundary(boundary), page(-1), left(0.0), top(0.0), url(url) {}

};

class Annotation
{
public:
    virtual ~Annotation() {}

    virtual QRectF boundary() const = 0;
    virtual QString contents() const = 0;

    virtual QDialog* showDialog(const QPoint& screenPos) = 0;
};

class FormField
{
public:
    virtual ~FormField() {}

    virtual QRectF boundary() const = 0;
    virtual QString name() const = 0;

    virtual QDialog* showDialog(const QPoint& screenPos) = 0;
};

class Page
{
public:
    virtual ~Page() {}

    virtual QSizeF size() const = 0;

    virtual QImage render(qreal horizontalResolution = 72.0, qreal verticalResolution = 72.0, Rotation rotation = RotateBy0, const QRect& boundingRect = QRect()) const = 0;

    virtual QList< Link* > links() const;

    virtual QString text(const QRectF& rect) const;
    virtual QList< QRectF > search(const QString& text, bool matchCase) const;

    virtual QList< Annotation* > annotations() const;

    virtual bool canAddAndRemoveAnnotations() const;
    virtual Annotation* addTextAnnotation(const QRectF& boundary);
    virtual Annotation* addHighlightAnnotation(const QRectF& boundary);
    virtual void removeAnnotation(Annotation* annotation);

    virtual QList< FormField* > formFields() const;

};

class Document
{
public:
    static Document* load(const QString& filePath);

    virtual ~Document() {}

    virtual int numberOfPages() const = 0;

    virtual Page* page(int index) const = 0;

    virtual bool isLocked() const;
    virtual bool unlock(const QString& password);

    virtual bool canSave() const;
    virtual QString saveFilter() const;
    virtual bool save(const QString& filePath, bool withChanges) const;

    virtual bool canBePrinted() const;

    virtual void setAntialiasing(bool on);
    virtual void setTextAntialiasing(bool on);
    virtual void setTextHinting(bool on);

    virtual void setOverprintPreview(bool on);

    virtual void setPaperColor(const QColor& paperColor);

    virtual void loadOutline(QStandardItemModel* outlineModel) const;
    virtual void loadProperties(QStandardItemModel* propertiesModel) const;

    virtual void loadFonts(QStandardItemModel* fontsModel) const;

};

#endif // DOCUMENTMODEL_H