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

#ifndef PDFMODEL_H
#define PDFMODEL_H

#include <QCoreApplication>
#include <QMutex>

namespace Poppler
{
class Annotation;
class Document;
class FormField;
class Page;
}

#include "model.h"

namespace Model
{

class PDFAnnotation : public Annotation
{
    friend class PDFPage;

public:
    ~PDFAnnotation();

    QRectF boundary() const;
    QString contents() const;

    QDialog* showDialog(const QPoint& screenPos);

private:
    PDFAnnotation(QMutex* mutex, Poppler::Annotation* annotation);

    mutable QMutex* m_mutex;
    Poppler::Annotation* m_annotation;

};

class PDFFormField : public FormField
{
    friend class PDFPage;

public:
    ~PDFFormField();

    QRectF boundary() const;
    QString name() const;

    QDialog* showDialog(const QPoint& screenPos);

private:
    PDFFormField(QMutex* mutex, Poppler::FormField* formField);

    mutable QMutex* m_mutex;
    Poppler::FormField* m_formField;

};

class PDFPage : public Page
{
    Q_DECLARE_TR_FUNCTIONS(PDFPage)

    friend class PDFDocument;

public:
    ~PDFPage();

    QSizeF size() const;

    QImage render(qreal horizontalResolution, qreal verticalResolution, Rotation rotation, const QRect& boundingRect) const;

    QList< Link* > links() const;

    QString text(const QRectF &rect) const;
    QList< QRectF > search(const QString &text, bool matchCase) const;

    QList< Annotation* > annotations() const;

    bool canAddAndRemoveAnnotations() const;
    Annotation* addTextAnnotation(const QRectF& boundary);
    Annotation* addHighlightAnnotation(const QRectF& boundary);
    void removeAnnotation(Annotation* annotation);

    QList< FormField* > formFields() const;

private:
    PDFPage(QMutex* mutex, Poppler::Page* page);

    mutable QMutex* m_mutex;
    Poppler::Page* m_page;

};

class PDFDocument : public Document
{
    Q_DECLARE_TR_FUNCTIONS(PDFDocument)

    friend class PDFDocumentLoader;

public:
    ~PDFDocument();

    int numberOfPages() const;

    Page* page(int index) const;

    bool isLocked() const;
    bool unlock(const QString& password);

    QStringList saveFilter() const;

    bool canSave() const;
    bool save(const QString& filePath, bool withChanges) const;

    bool canBePrinted() const;

    void setAntialiasing(bool on);
    void setTextAntialiasing(bool on);
    void setTextHinting(bool on);

    void setOverprintPreview(bool on);

    void setPaperColor(const QColor& paperColor);

    void loadOutline(QStandardItemModel *outlineModel) const;
    void loadProperties(QStandardItemModel *propertiesModel) const;
    void loadFonts(QStandardItemModel* fontsModel) const;

private:
    PDFDocument(Poppler::Document* document);

    mutable QMutex m_mutex;
    Poppler::Document* m_document;

};

class PDFDocumentLoader : public QObject, DocumentLoader
{
    Q_OBJECT
    Q_INTERFACES(Model::DocumentLoader)

public:
    PDFDocumentLoader(QObject* parent = 0);

    Document* loadDocument(const QString& filePath) const;

};

}

#endif // PDFMODEL_H
