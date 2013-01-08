#include "pdfmodel.h"

#include <QImage>

#include <poppler-qt4.h>

PDFPage::PDFPage(QMutex* mutex, Poppler::Page* page) :
    m_mutex(mutex),
    m_page(page)
{
}

PDFPage::~PDFPage()
{
    delete m_page;
}

QSizeF PDFPage::size() const
{
    QMutexLocker mutexLocker(m_mutex);

    return m_page->pageSizeF();
}

QImage PDFPage::render(qreal horizontalResolution, qreal verticalResolution, Rotation rotation, const QRect& boundingRect) const
{
    QMutexLocker mutexLocker(m_mutex);

    double xres;
    double yres;

    switch(rotation)
    {
    default:
    case RotateBy0:
    case RotateBy180:
        xres = horizontalResolution;
        yres = verticalResolution;
        break;
    case RotateBy90:
    case RotateBy270:
        xres = verticalResolution;
        yres = horizontalResolution;
        break;
    }

    Poppler::Page::Rotation rotate;

    switch(rotation)
    {
    default:
    case RotateBy0:
        rotate = Poppler::Page::Rotate0;
        break;
    case RotateBy90:
        rotate = Poppler::Page::Rotate90;
        break;
    case RotateBy180:
        rotate = Poppler::Page::Rotate180;
        break;
    case RotateBy270:
        rotate = Poppler::Page::Rotate270;
        break;
    }

    int x = -1;
    int y = -1;
    int w = -1;
    int h = -1;

    if(!boundingRect.isNull())
    {
        x = boundingRect.x();
        y = boundingRect.y();
        w = boundingRect.width();
        h = boundingRect.height();
    }

    return m_page->renderToImage(xres, yres, x, y, w, h, rotate);
}

QList< Link > PDFPage::links() const
{
    QMutexLocker mutexLocker(m_mutex);

    QList< Link > links;

    foreach(Poppler::Link* link, m_page->links())
    {
        if(link->linkType() == Poppler::Link::Goto)
        {
            Poppler::LinkGoto* linkGoto = static_cast< Poppler::LinkGoto* >(link);

            if(!linkGoto->isExternal())
            {
                QRectF boundary = linkGoto->linkArea().normalized();
                int page = linkGoto->destination().pageNumber();
                qreal left = linkGoto->destination().isChangeLeft() ? linkGoto->destination().left() : 0.0;
                qreal top = linkGoto->destination().isChangeTop() ? linkGoto->destination().top() : 0.0;

                links.append(Link(boundary, page, left, top));
            }
        }
        else if(link->linkType() == Poppler::Link::Browse)
        {
            Poppler::LinkBrowse* linkBrowse = static_cast< Poppler::LinkBrowse* >(link);

            QRectF boundary = linkBrowse->linkArea().normalized();
            QString url = linkBrowse->url();

            links.append(Link(boundary, url));
        }

        delete link;
    }

    return links;
}

Document* PDFDocument::load(const QString& filePath)
{
    Poppler::Document* document = Poppler::Document::load(filePath);

    return document != 0 ? new PDFDocument(document) : 0;
}

PDFDocument::PDFDocument(Poppler::Document* document) :
    m_mutex(),
    m_document(document)
{
}

PDFDocument::~PDFDocument()
{
    delete m_document;
}

int PDFDocument::numberOfPages() const
{
    QMutexLocker mutexLocker(&m_mutex);

    return m_document->numPages();
}

Page* PDFDocument::page(int index) const
{
    QMutexLocker mutexLocker(&m_mutex);

    Poppler::Page* page = m_document->page(index);

    return page != 0 ? new PDFPage(&m_mutex, page) : 0;
}

void PDFDocument::setAntialiasing(bool on)
{
    QMutexLocker mutexLocker(&m_mutex);

    m_document->setRenderHint(Poppler::Document::Antialiasing, on);
}

void PDFDocument::setTextAntialiasing(bool on)
{
    QMutexLocker mutexLocker(&m_mutex);

    m_document->setRenderHint(Poppler::Document::TextAntialiasing, on);
}

void PDFDocument::setTextHinting(bool on)
{
    QMutexLocker mutexLocker(&m_mutex);

    m_document->setRenderHint(Poppler::Document::TextHinting, on);
}
