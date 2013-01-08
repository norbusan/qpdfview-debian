#include "pdfmodel.h"

#include <QMessageBox>
#include <QStandardItemModel>

#include <poppler-qt4.h>
#include <poppler-form.h>

#include "annotationdialog.h"
#include "formfielddialog.h"

PDFAnnotation::PDFAnnotation(QMutex* mutex, Poppler::Annotation* annotation) :
    m_mutex(mutex),
    m_annotation(annotation)
{
}

PDFAnnotation::~PDFAnnotation()
{
    delete m_annotation;
}

QRectF PDFAnnotation::boundary() const
{
    QMutexLocker mutexLocker(m_mutex);

    return m_annotation->boundary().normalized();
}

QString PDFAnnotation::contents() const
{
    QMutexLocker mutexLocker(m_mutex);

    return m_annotation->contents();
}

QDialog* PDFAnnotation::showDialog(const QPoint& screenPos)
{
    QMutexLocker mutexLocker(m_mutex);

    AnnotationDialog* annotationDialog = new AnnotationDialog(m_mutex, m_annotation);

    annotationDialog->move(screenPos);

    annotationDialog->setAttribute(Qt::WA_DeleteOnClose);
    annotationDialog->show();

    return annotationDialog;
}

PDFFormField::PDFFormField(QMutex* mutex, Poppler::FormField* formField) :
    m_mutex(mutex),
    m_formField(formField)
{
}

PDFFormField::~PDFFormField()
{
    delete m_formField;
}

QRectF PDFFormField::boundary() const
{
    QMutexLocker mutexLocker(m_mutex);

    return m_formField->rect().normalized();
}

QString PDFFormField::name() const
{
    QMutexLocker mutexLocker(m_mutex);

    return m_formField->name();
}

QDialog* PDFFormField::showDialog(const QPoint& screenPos)
{
    QMutexLocker mutexLocker(m_mutex);

    if(m_formField->type() == Poppler::FormField::FormText || m_formField->type() == Poppler::FormField::FormChoice)
    {
        FormFieldDialog* formFieldDialog = new FormFieldDialog(m_mutex, m_formField);

        formFieldDialog->move(screenPos);

        formFieldDialog->setAttribute(Qt::WA_DeleteOnClose);
        formFieldDialog->show();

        return formFieldDialog;
    }
    else if(m_formField->type() == Poppler::FormField::FormButton)
    {
        Poppler::FormFieldButton* formFieldButton = static_cast< Poppler::FormFieldButton* >(m_formField);

        formFieldButton->setState(!formFieldButton->state());
    }

    return 0;
}

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

QList< Link* > PDFPage::links() const
{
    QMutexLocker mutexLocker(m_mutex);

    QList< Link* > links;

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

                left = left >= 0.0 ? left : 0.0;
                left = left <= 1.0 ? left : 1.0;

                top = top >= 0.0 ? top : 0.0;
                top = top <= 1.0 ? top : 1.0;

                links.append(new Link(boundary, page, left, top));
            }
        }
        else if(link->linkType() == Poppler::Link::Browse)
        {
            Poppler::LinkBrowse* linkBrowse = static_cast< Poppler::LinkBrowse* >(link);

            QRectF boundary = linkBrowse->linkArea().normalized();
            QString url = linkBrowse->url();

            links.append(new Link(boundary, url));
        }

        delete link;
    }

    return links;
}

QString PDFPage::text(const QRectF& rect) const
{
    QMutexLocker mutexLocker(m_mutex);

    return m_page->text(rect);
}

QList< QRectF > PDFPage::search(const QString& text, bool matchCase) const
{
    QMutexLocker mutexLocker(m_mutex);

    QList< QRectF > results;

#if defined(HAS_POPPLER_22)

    results = m_page->search(text, matchCase ? Poppler::Page::CaseSensitive : Poppler::Page::CaseInsensitive);

#elif defined(HAS_POPPLER_14)

    double left = 0.0, top = 0.0, right = 0.0, bottom = 0.0;

    while(m_page->search(text, left, top, right, bottom, Poppler::Page::NextResult, matchCase ? Poppler::Page::CaseSensitive : Poppler::Page::CaseInsensitive))
    {
        QRectF rect;
        rect.setLeft(left);
        rect.setTop(top);
        rect.setRight(right);
        rect.setBottom(bottom);

        results.append(rect);
    }

#else

    QRectF rect;

    while(m_page->search(text, rect, Poppler::Page::NextResult, matchCase ? Poppler::Page::CaseSensitive : Poppler::Page::CaseInsensitive))
    {
        results.append(rect);
    }

#endif

    return results;
}

QList< Annotation* > PDFPage::annotations() const
{
    QMutexLocker mutexLocker(m_mutex);

    QList< Annotation* > annotations;

    foreach(Poppler::Annotation* annotation, m_page->annotations())
    {
        if(annotation->subType() == Poppler::Annotation::AText || annotation->subType() == Poppler::Annotation::AHighlight)
        {
            annotations.append(new PDFAnnotation(m_mutex, annotation));
            continue;
        }

        delete annotation;
    }

    return annotations;
}

bool PDFPage::canAddAnnotations() const
{
#ifdef HAS_POPPLER_20

    return true;

#else

    QMessageBox::information(0, QObject::tr("Information"), QObject::tr("Version 0.20.1 or higher of the Poppler library is required to add or remove annotations."));

    return false;

#endif // HAS_POPPLER_20
}

Annotation* PDFPage::addTextAnnotation(const QRectF& boundary)
{
#ifdef HAS_POPPLER_20

    Poppler::Annotation::Style style;
    style.setColor(Qt::yellow);

    Poppler::Annotation::Popup popup;
    popup.setFlags(Poppler::Annotation::Hidden | Poppler::Annotation::ToggleHidingOnMouse);

    Poppler::HighlightAnnotation* annotation = new Poppler::HighlightAnnotation();

    Poppler::HighlightAnnotation::Quad quad;
    quad.points[0] = boundary.topLeft();
    quad.points[1] = boundary.topRight();
    quad.points[2] = boundary.bottomRight();
    quad.points[3] = boundary.bottomLeft();

    annotation->setHighlightQuads(QList< Poppler::HighlightAnnotation::Quad >() << quad);

    annotation->setBoundary(boundary);
    annotation->setStyle(style);
    annotation->setPopup(popup);

    m_page->addAnnotation(annotation);

    return new PDFAnnotation(m_mutex, annotation);

#else

    Q_UNUSED(boundary);

    return 0;

#endif // HAS_POPPLER_20

}

Annotation* PDFPage::addHighlightAnnotation(const QRectF& boundary)
{
#ifdef HAS_POPPLER_20

    Poppler::Annotation::Style style;
    style.setColor(Qt::yellow);

    Poppler::Annotation::Popup popup;
    popup.setFlags(Poppler::Annotation::Hidden | Poppler::Annotation::ToggleHidingOnMouse);

    Poppler::Annotation* annotation = new Poppler::TextAnnotation(Poppler::TextAnnotation::Linked);

    annotation->setBoundary(boundary);
    annotation->setStyle(style);
    annotation->setPopup(popup);

    m_page->addAnnotation(annotation);

    return new PDFAnnotation(m_mutex, annotation);

#else

    Q_UNUSED(boundary);

    return 0;

#endif // HAS_POPPLER_20
}

void PDFPage::removeAnnotation(Annotation* annotation)
{
    QMutexLocker mutexLocker(m_mutex);

#ifdef HAS_POPPLER_20

    m_page->removeAnnotation(static_cast< PDFAnnotation* >(annotation)->m_annotation);

#endif // HAS_POPPLER_20
}

QList< FormField* > PDFPage::formFields() const
{
    QMutexLocker mutexLocker(m_mutex);

    QList< FormField* > formFields;

    foreach(Poppler::FormField* formField, m_page->formFields())
    {
        if(!formField->isVisible() || formField->isReadOnly())
        {
            delete formField;
            continue;
        }

        switch(formField->type())
        {
        default:
        case Poppler::FormField::FormSignature:
            delete formField;
            break;
        case Poppler::FormField::FormText:
            switch(static_cast< Poppler::FormFieldText* >(formField)->textType())
            {
            default:
            case Poppler::FormFieldText::FileSelect:
                delete formField;
                break;
            case Poppler::FormFieldText::Normal:
            case Poppler::FormFieldText::Multiline:
                formFields.append(new PDFFormField(m_mutex, formField));
                break;
            }

            break;
        case Poppler::FormField::FormChoice:
            formFields.append(new PDFFormField(m_mutex, formField));
            break;
        case Poppler::FormField::FormButton:
            switch(static_cast< Poppler::FormFieldButton* >(formField)->buttonType())
            {
            default:
            case Poppler::FormFieldButton::Push:
                delete formField;
                break;
            case Poppler::FormFieldButton::CheckBox:
            case Poppler::FormFieldButton::Radio:
                formFields.append(new PDFFormField(m_mutex, formField));
                break;
            }

            break;
        }
    }

    return formFields;
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

bool PDFDocument::isLocked() const
{
    QMutexLocker mutexLocker(&m_mutex);

    return m_document->isLocked();
}

bool PDFDocument::unlock(const QString& password)
{
    QMutexLocker mutexLocker(&m_mutex);

    return m_document->unlock(password.toLatin1(), password.toLatin1());
}

bool PDFDocument::canSave() const
{
    return true;
}

QString PDFDocument::saveFilter() const
{
    return "Portable document format (*.pdf)";
}

bool PDFDocument::save(const QString& filePath, bool withChanges) const
{
    QMutexLocker mutexLocker(&m_mutex);

    Poppler::PDFConverter* pdfConverter = m_document->pdfConverter();

    pdfConverter->setOutputFileName(filePath);

    if(withChanges)
    {
        pdfConverter->setPDFOptions(pdfConverter->pdfOptions() | Poppler::PDFConverter::WithChanges);
    }

    bool ok = pdfConverter->convert();

    delete pdfConverter;

    return ok;
}

bool PDFDocument::canPrint() const
{
    return true;
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

void PDFDocument::setOverprintPreview(bool on)
{
    QMutexLocker mutexLocker(&m_mutex);

#ifdef HAS_POPPLER_22

    m_document->setRenderHint(Poppler::Document::OverprintPreview, on);

#else

    Q_UNUSED(on);

#endif // HAS_POPPLER_22
}

void PDFDocument::setPaperColor(const QColor& paperColor)
{
    QMutexLocker mutexLocker(&m_mutex);

    m_document->setPaperColor(paperColor);
}

static void loadOutline(Poppler::Document* document, const QDomNode& node, QStandardItem* parent)
{
    QDomElement element = node.toElement();

    QStandardItem* item = new QStandardItem();

    item->setFlags(Qt::ItemIsEnabled);

    item->setText(element.tagName());
    item->setToolTip(element.tagName());

    Poppler::LinkDestination* linkDestination = 0;

    if(element.hasAttribute("Destination"))
    {
        linkDestination = new Poppler::LinkDestination(element.attribute("Destination"));
    }
    else if(element.hasAttribute("DestinationName"))
    {
        linkDestination = document->linkDestination(element.attribute("DestinationName"));
    }

    if(linkDestination != 0)
    {
        int page = linkDestination->pageNumber();
        qreal left = 0.0;
        qreal top = 0.0;

        page = page >= 1 ? page : 1;
        page = page <= document->numPages() ? page : document->numPages();

        if(linkDestination->isChangeLeft())
        {
            left = linkDestination->left();

            left = left >= 0.0 ? left : 0.0;
            left = left <= 1.0 ? left : 1.0;
        }

        if(linkDestination->isChangeTop())
        {
            top = linkDestination->top();

            top = top >= 0.0 ? top : 0.0;
            top = top <= 1.0 ? top : 1.0;
        }

        item->setData(page, Qt::UserRole + 1);
        item->setData(left, Qt::UserRole + 2);
        item->setData(top, Qt::UserRole + 3);

        delete linkDestination;
    }

    parent->appendRow(item);

    QDomNode siblingNode = node.nextSibling();
    if(!siblingNode.isNull())
    {
        loadOutline(document, siblingNode, parent);
    }

    QDomNode childNode = node.firstChild();
    if(!childNode.isNull())
    {
        loadOutline(document, childNode, item);
    }
}

void PDFDocument::loadOutline(QStandardItemModel* outlineModel) const
{
    QMutexLocker mutexLocker(&m_mutex);

    QDomDocument* toc = m_document->toc();

    if(toc != 0)
    {
        ::loadOutline(m_document, toc->firstChild(), outlineModel->invisibleRootItem());

        delete toc;
    }
}

void PDFDocument::loadProperties(QStandardItemModel* propertiesModel) const
{
    QMutexLocker mutexLocker(&m_mutex);

    QStringList keys = m_document->infoKeys();

    propertiesModel->setRowCount(keys.count());
    propertiesModel->setColumnCount(2);

    for(int index = 0; index < keys.count(); ++index)
    {
        QString key = keys.at(index);
        QString value = m_document->info(key);

        if(value.startsWith("D:"))
        {
            value = m_document->date(key).toString();
        }

        propertiesModel->setItem(index, 0, new QStandardItem(key));
        propertiesModel->setItem(index, 1, new QStandardItem(value));
    }
}

void PDFDocument::loadFonts(QStandardItemModel* fontsModel) const
{
    QMutexLocker mutexLocker(&m_mutex);

    QList< Poppler::FontInfo > fonts = m_document->fonts();

    mutexLocker.unlock();

    fontsModel->setRowCount(fonts.count());
    fontsModel->setColumnCount(5);

    fontsModel->setHorizontalHeaderLabels(QStringList() << QObject::tr("Name") << QObject::tr("Type") << QObject::tr("Embedded") << QObject::tr("Subset") << QObject::tr("File"));

    for(int index = 0; index < fonts.count(); ++index)
    {
        Poppler::FontInfo& font = fonts[index];

        fontsModel->setItem(index, 0, new QStandardItem(font.name()));
        fontsModel->setItem(index, 1, new QStandardItem(font.typeName()));
        fontsModel->setItem(index, 2, new QStandardItem(font.isEmbedded() ? QObject::tr("Yes") : QObject::tr("No")));
        fontsModel->setItem(index, 3, new QStandardItem(font.isSubset() ? QObject::tr("Yes") : QObject::tr("No")));
        fontsModel->setItem(index, 4, new QStandardItem(font.file()));
    }
}
