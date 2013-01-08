#ifndef PDFMODEL_H
#define PDFMODEL_H

#include <QSizeF>
#include <QMutex>

namespace Poppler
{
class Annotation;
class Document;
class FormField;
class Page;
}

#include "model.h"

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
    friend class PDFDocument;

public:
    ~PDFPage();

    QSizeF size() const;

    QImage render(qreal horizontalResolution, qreal verticalResolution, Rotation rotation, const QRect& boundingRect) const;

    QList< Link* > links() const;

    QString text(const QRectF &rect) const;
    QList< QRectF > search(const QString &text, bool matchCase) const;

    QList< Annotation* > annotations() const;

    QList< FormField* > formFields() const;

private:
    PDFPage(QMutex* mutex, Poppler::Page* page);

    mutable QMutex* m_mutex;
    Poppler::Page* m_page;

};

class PDFDocument : public Document
{
public:
    static Document* load(const QString &filePath);

    ~PDFDocument();

    int numberOfPages() const;

    Page* page(int index) const;

    void setAntialiasing(bool on);
    void setTextAntialiasing(bool on);
    void setTextHinting(bool on);

    void setOverprintPreview(bool on);

    void setPaperColor(const QColor& paperColor);

private:
    PDFDocument(Poppler::Document* document);

    mutable QMutex m_mutex;
    Poppler::Document* m_document;

};

#endif // PDFMODEL_H
