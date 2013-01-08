#ifndef PDFMODEL_H
#define PDFMODEL_H

#include <QSizeF>
#include <QMutex>

namespace Poppler
{
class Page;
class Document;
}

#include "model.h"

class PDFPage : public Page
{
    friend class PDFDocument;

public:
    ~PDFPage();

    QSizeF size() const;

    QImage render(qreal horizontalResolution, qreal verticalResolution, Rotation rotation, const QRect& boundingRect) const;

    QList< Link > links() const;

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

private:
    PDFDocument(Poppler::Document* document);

    mutable QMutex m_mutex;
    Poppler::Document* m_document;

};

#endif // PDFMODEL_H
