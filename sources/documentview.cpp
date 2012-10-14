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

#include "documentview.h"

bool DocumentView::s_openUrl = false;

bool DocumentView::s_autoRefresh = false;

bool DocumentView::s_antialiasing = true;
bool DocumentView::s_textAntialiasing = true;
bool DocumentView::s_textHinting = false;

bool DocumentView::s_prefetch = false;

int DocumentView::s_pagesPerRow = 3;

qreal DocumentView::s_pageSpacing = 5.0;
qreal DocumentView::s_thumbnailSpacing = 3.0;

qreal DocumentView::s_thumbnailSize = 150.0;

qreal DocumentView::s_minimumScaleFactor = 0.1;
qreal DocumentView::s_maximumScaleFactor = 10.0;
qreal DocumentView::s_zoomBy = 0.1;

Qt::KeyboardModifiers DocumentView::s_zoomModifiers = Qt::ControlModifier;
Qt::KeyboardModifiers DocumentView::s_rotateModifiers = Qt::ShiftModifier;
Qt::KeyboardModifiers DocumentView::s_horizontalModifiers = Qt::AltModifier;

int DocumentView::s_highlightDuration = 5000;

QString DocumentView::s_sourceEditor;

bool DocumentView::openUrl()
{
    return s_openUrl;
}

void DocumentView::setOpenUrl(bool openUrl)
{
    s_openUrl = openUrl;
}

bool DocumentView::autoRefresh()
{
    return s_autoRefresh;
}

void DocumentView::setAutoRefresh(bool autoRefresh)
{
    s_autoRefresh = autoRefresh;
}

bool DocumentView::antialiasing()
{
    return s_antialiasing;
}

void DocumentView::setAntialiasing(bool antialiasing)
{
    s_antialiasing = antialiasing;
}

bool DocumentView::textAntialiasing()
{
    return s_textAntialiasing;
}

void DocumentView::setTextAntialiasing(bool textAntialiasing)
{
    s_textAntialiasing = textAntialiasing;
}

bool DocumentView::textHinting()
{
    return s_textHinting;
}

void DocumentView::setTextHinting(bool textHinting)
{
    s_textHinting = textHinting;
}

bool DocumentView::prefetch()
{
    return s_prefetch;
}

void DocumentView::setPrefetch(bool prefetch)
{
    s_prefetch = prefetch;
}

int DocumentView::pagesPerRow()
{
    return s_pagesPerRow;
}

void DocumentView::setPagesPerRow(int pagesPerRow)
{
    if(pagesPerRow >= 1)
    {
        s_pagesPerRow = pagesPerRow;
    }
}

qreal DocumentView::pageSpacing()
{
    return s_pageSpacing;
}

void DocumentView::setPageSpacing(qreal pageSpacing)
{
    if(pageSpacing >= 0.0)
    {
        s_pageSpacing = pageSpacing;
    }
}

qreal DocumentView::thumbnailSpacing()
{
    return s_thumbnailSpacing;
}

void DocumentView::setThumbnailSpacing(qreal thumbnailSpacing)
{
    if(thumbnailSpacing >= 0.0)
    {
        s_thumbnailSpacing = thumbnailSpacing;
    }
}

qreal DocumentView::thumbnailSize()
{
    return s_thumbnailSize;
}

void DocumentView::setThumbnailSize(qreal thumbnailSize)
{
    if(thumbnailSize >= 0.0)
    {
        s_thumbnailSize = thumbnailSize;
    }
}

qreal DocumentView::minimumScaleFactor()
{
    return s_minimumScaleFactor;
}

qreal DocumentView::maximumScaleFactor()
{
    return s_maximumScaleFactor;
}

qreal DocumentView::zoomBy()
{
    return s_zoomBy;
}

const Qt::KeyboardModifiers& DocumentView::zoomModifiers()
{
    return s_zoomModifiers;
}

void DocumentView::setZoomModifiers(const Qt::KeyboardModifiers& zoomModifiers)
{
    s_zoomModifiers = zoomModifiers;
}

const Qt::KeyboardModifiers& DocumentView::rotateModifiers()
{
    return s_rotateModifiers;
}

void DocumentView::setRotateModifiers(const Qt::KeyboardModifiers& rotateModifiers)
{
    s_rotateModifiers = rotateModifiers;
}

const Qt::KeyboardModifiers& DocumentView::horizontalModifiers()
{
    return s_horizontalModifiers;
}

void DocumentView::setHorizontalModifiers(const Qt::KeyboardModifiers& horizontalModifiers)
{
    s_horizontalModifiers = horizontalModifiers;
}

int DocumentView::highlightDuration()
{
    return s_highlightDuration;
}

void DocumentView::setHighlightDuration(int highlightDuration)
{
    s_highlightDuration = highlightDuration;
}

const QString &DocumentView::sourceEditor()
{
    return s_sourceEditor;
}

void DocumentView::setSourceEditor(const QString& sourceEditor)
{
    s_sourceEditor = sourceEditor;
}

DocumentView::DocumentView(QWidget* parent) : QGraphicsView(parent),
    m_autoRefreshWatcher(0),
    m_autoRefreshTimer(0),
    m_prefetchTimer(0),
    m_mutex(),
    m_document(0),
    m_filePath(),
    m_numberOfPages(-1),
    m_currentPage(-1),
    m_returnToPage(),
    m_returnToLeft(),
    m_returnToTop(),
    m_continuousMode(false),
    m_layoutMode(SinglePageMode),
    m_scaleMode(ScaleFactor),
    m_scaleFactor(1.0),
    m_rotation(Poppler::Page::Rotate0),
    m_highlightAll(false),
    m_rubberBandMode(PageItem::ModifiersMode),
    m_pagesScene(0),
    m_pages(),
    m_heightToIndex(),
    m_thumbnailsScene(0),
    m_thumbnails(),
    m_highlight(0),
    m_outlineModel(0),
    m_propertiesModel(0),
    m_results(),
    m_currentResult(m_results.end()),
    m_searchThread(0)
{
    m_pagesScene = new QGraphicsScene(this);
    m_thumbnailsScene = new QGraphicsScene(this);

    m_outlineModel = new QStandardItemModel(this);
    m_propertiesModel = new QStandardItemModel(this);

    setScene(m_pagesScene);

    setDragMode(QGraphicsView::ScrollHandDrag);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    setAcceptDrops(false);

    connect(verticalScrollBar(), SIGNAL(valueChanged(int)), SLOT(on_verticalScrollBar_valueChanged(int)));

    // highlight

    m_highlight = new QGraphicsRectItem();

    m_highlight->setVisible(false);
    scene()->addItem(m_highlight);

    QColor highlightColor = QApplication::palette().color(QPalette::Highlight);

    highlightColor.setAlpha(127);
    m_highlight->setBrush(QBrush(highlightColor));

    highlightColor.setAlpha(255);
    m_highlight->setPen(QPen(highlightColor));

    // search

    m_searchThread = new SearchThread(this);

    connect(m_searchThread, SIGNAL(resultsReady(int,QList<QRectF>)), SLOT(on_searchThread_resultsReady(int,QList<QRectF>)));

    connect(m_searchThread, SIGNAL(progressed(int)), SIGNAL(searchProgressed(int)));
    connect(m_searchThread, SIGNAL(finished()), SIGNAL(searchFinished()));
    connect(m_searchThread, SIGNAL(canceled()), SIGNAL(searchCanceled()));

    // auto-refresh

    m_autoRefreshWatcher = new QFileSystemWatcher(this);

    m_autoRefreshTimer = new QTimer(this);
    m_autoRefreshTimer->setInterval(500);
    m_autoRefreshTimer->setSingleShot(true);

    connect(m_autoRefreshWatcher, SIGNAL(fileChanged(QString)), m_autoRefreshTimer, SLOT(start()));
    connect(m_autoRefreshTimer, SIGNAL(timeout()), this, SLOT(refresh()));

    // prefetch

    m_prefetchTimer = new QTimer(this);
    m_prefetchTimer->setInterval(250);
    m_prefetchTimer->setSingleShot(true);

    connect(this, SIGNAL(currentPageChanged(int)), m_prefetchTimer, SLOT(start()));
    connect(this, SIGNAL(scaleModeChanged(DocumentView::ScaleMode)), m_prefetchTimer, SLOT(start()));
    connect(this, SIGNAL(scaleFactorChanged(qreal)), m_prefetchTimer, SLOT(start()));
    connect(this, SIGNAL(rotationChanged(Poppler::Page::Rotation)), m_prefetchTimer, SLOT(start()));

    connect(m_prefetchTimer, SIGNAL(timeout()), SLOT(on_prefetch_timeout()));
}

DocumentView::~DocumentView()
{
    m_searchThread->cancel();
    m_searchThread->wait();

    qDeleteAll(m_pages);
    qDeleteAll(m_thumbnails);

    if(m_document != 0)
    {
        delete m_document;
    }
}

const QString& DocumentView::filePath() const
{
    return m_filePath;
}

int DocumentView::numberOfPages() const
{
    return m_numberOfPages;
}

int DocumentView::currentPage() const
{
    return m_currentPage;
}

bool DocumentView::continousMode() const
{
    return m_continuousMode;
}

void DocumentView::setContinousMode(bool continousMode)
{
    if(m_continuousMode != continousMode)
    {
        m_continuousMode = continousMode;

        qreal left = 0.0, top = 0.0;
        saveLeftAndTop(left, top);

        prepareView(left, top);

        emit continousModeChanged(m_continuousMode);
    }
}

DocumentView::LayoutMode DocumentView::layoutMode() const
{
    return m_layoutMode;
}

void DocumentView::setLayoutMode(DocumentView::LayoutMode layoutMode)
{
    if(m_layoutMode != layoutMode)
    {
        m_layoutMode = layoutMode;

        if(m_currentPage != currentPageForPage(m_currentPage))
        {
            m_currentPage = currentPageForPage(m_currentPage);

            emit currentPageChanged(m_currentPage);
        }

        prepareScene();
        prepareView();

        emit layoutModeChanged(m_layoutMode);
    }
}

DocumentView::ScaleMode DocumentView::scaleMode() const
{
    return m_scaleMode;
}

void DocumentView::setScaleMode(ScaleMode scaleMode)
{
    if(m_scaleMode != scaleMode)
    {
        m_scaleMode = scaleMode;

        qreal left = 0.0, top = 0.0;
        saveLeftAndTop(left, top);

        prepareScene();
        prepareView(left, top);

        emit scaleModeChanged(m_scaleMode);
    }
}

qreal DocumentView::scaleFactor() const
{
    return m_scaleFactor;
}

void DocumentView::setScaleFactor(qreal scaleFactor)
{
    if(!qFuzzyCompare(m_scaleFactor, scaleFactor) && scaleFactor >= s_minimumScaleFactor && scaleFactor <= s_maximumScaleFactor)
    {
        m_scaleFactor = scaleFactor;

        if(m_scaleMode == ScaleFactor)
        {
            qreal left = 0.0, top = 0.0;
            saveLeftAndTop(left, top);

            prepareScene();
            prepareView(left, top);
        }

        emit scaleFactorChanged(m_scaleFactor);
    }
}

Poppler::Page::Rotation DocumentView::rotation() const
{
    return m_rotation;
}

void DocumentView::setRotation(Poppler::Page::Rotation rotation)
{
    if(m_rotation != rotation)
    {
        m_rotation = rotation;

        prepareScene();
        prepareView();

        emit rotationChanged(m_rotation);
    }
}

bool DocumentView::highlightAll() const
{
    return m_highlightAll;
}

void DocumentView::setHighlightAll(bool highlightAll)
{
    if(m_highlightAll != highlightAll)
    {
        m_highlightAll = highlightAll;

        if(m_highlightAll)
        {
            for(int index = 0; index < m_numberOfPages; ++index)
            {
                m_pages.at(index)->setHighlights(m_results.values(index));
            }
        }
        else
        {
            foreach(PageItem* page, m_pages)
            {
                page->clearHighlights();
            }
        }

        emit highlightAllChanged(m_highlightAll);
    }
}

PageItem::RubberBandMode DocumentView::rubberBandMode() const
{
    return m_rubberBandMode;
}

void DocumentView::setRubberBandMode(PageItem::RubberBandMode rubberBandMode)
{
    if(m_rubberBandMode != rubberBandMode)
    {
        m_rubberBandMode = rubberBandMode;

        foreach(PageItem* page, m_pages)
        {
            page->setRubberBandMode(m_rubberBandMode);
        }

        emit rubberBandModeChanged(m_rubberBandMode);
    }
}

bool DocumentView::searchWasCanceled() const
{
    return m_searchThread->wasCanceled();
}

int DocumentView::searchProgress() const
{
    return m_searchThread->progress();
}

QStandardItemModel* DocumentView::outlineModel() const
{
    return m_outlineModel;
}

QStandardItemModel* DocumentView::propertiesModel() const
{
    return m_propertiesModel;
}

QStandardItemModel* DocumentView::fontsModel()
{
    m_mutex.lock();

    QList< Poppler::FontInfo > fonts = m_document->fonts();

    m_mutex.unlock();

    QStandardItemModel* fontsModel = new QStandardItemModel();

    fontsModel->setRowCount(fonts.count());
    fontsModel->setColumnCount(5);

    fontsModel->setHorizontalHeaderLabels(QStringList() << tr("Name") << tr("Type") << tr("Embedded") << tr("Subset") << tr("File"));

    for(int index = 0; index < fonts.count(); ++index)
    {
        Poppler::FontInfo font = fonts.at(index);

        fontsModel->setItem(index, 0, new QStandardItem(font.name()));
        fontsModel->setItem(index, 1, new QStandardItem(font.typeName()));
        fontsModel->setItem(index, 2, new QStandardItem(font.isEmbedded() ? tr("Yes") : tr("No")));
        fontsModel->setItem(index, 3, new QStandardItem(font.isSubset() ? tr("Yes") : tr("No")));
        fontsModel->setItem(index, 4, new QStandardItem(font.file()));
    }

    return fontsModel;
}

QGraphicsScene* DocumentView::thumbnailsScene() const
{
    return m_thumbnailsScene;
}

QGraphicsItem* DocumentView::thumbnailsItem(int page) const
{
    return m_thumbnails.value(page - 1, 0);
}

void DocumentView::show()
{
    QGraphicsView::show();

    prepareView();
}

bool DocumentView::open(const QString& filePath)
{
    Poppler::Document* document = Poppler::Document::load(filePath);

    if(document != 0)
    {
        if(document->isLocked())
        {
            QString password = QInputDialog::getText(this, tr("Unlock %1").arg(QFileInfo(filePath).completeBaseName()), tr("Password:"), QLineEdit::Password);

            if(document->unlock(password.toLatin1(), password.toLatin1()))
            {
                delete document;
                return false;
            }
        }

        m_filePath = filePath;

        m_numberOfPages = document->numPages();
        m_currentPage = 1;

        m_returnToPage.clear();
        m_returnToLeft.clear();
        m_returnToTop.clear();

        prepareDocument(document);

        prepareScene();
        prepareView();

        emit filePathChanged(m_filePath);
        emit numberOfPagesChanged(m_numberOfPages);
        emit currentPageChanged(m_currentPage);
    }

    return document != 0;
}

bool DocumentView::refresh()
{
    Poppler::Document* document = Poppler::Document::load(m_filePath);

    if(document != 0)
    {
        if(document->isLocked())
        {
            QString password = QInputDialog::getText(this, tr("Unlock %1").arg(QFileInfo(m_filePath).completeBaseName()), tr("Password:"), QLineEdit::Password);

            if(document->unlock(password.toLatin1(), password.toLatin1()))
            {
                delete document;
                return false;
            }
        }

        qreal left = 0.0, top = 0.0;
        saveLeftAndTop(left, top);

        m_numberOfPages = document->numPages();
        m_currentPage = m_currentPage <= m_numberOfPages ? m_currentPage : 1;

        prepareDocument(document);

        prepareScene();
        prepareView(left, top);

        emit numberOfPagesChanged(m_numberOfPages);
        emit currentPageChanged(m_currentPage);
    }

    return document != 0;
}

bool DocumentView::saveCopy(const QString& filePath)
{
    m_mutex.lock();

    Poppler::PDFConverter* pdfConverter = m_document->pdfConverter();

    pdfConverter->setOutputFileName(filePath);
    pdfConverter->setPDFOptions(pdfConverter->pdfOptions() | Poppler::PDFConverter::WithChanges);
    bool ok = pdfConverter->convert();

    delete pdfConverter;

    m_mutex.unlock();

    return ok;
}

bool DocumentView::print(QPrinter* printer, const PrintOptions& printOptions)
{
    int fromPage = printer->fromPage() != 0 ? printer->fromPage() : 1;
    int toPage = printer->toPage() != 0 ? printer->toPage() : m_numberOfPages;
    int numberUp = 1;

#ifdef WITH_CUPS

    int num_dests = 0;
    cups_dest_t* dests = 0;

    int num_options = 0;
    cups_option_t* options = 0;

    cups_dest_t* dest = 0;
    int jobId = 0;

    num_dests = cupsGetDests(&dests);

    dest = cupsGetDest(printer->printerName().toLocal8Bit(), 0, num_dests, dests);

    if(dest != 0)
    {
        for(int index = 0; index < dest->num_options; ++index)
        {
            num_options = cupsAddOption(dest->options[index].name, dest->options[index].value, num_options, &options);
        }

#if QT_VERSION >= QT_VERSION_CHECK(4,7,0)

        num_options = cupsAddOption("copies", QString("%1").arg(printer->copyCount()).toLocal8Bit(), num_options, &options);

#endif // QT_VERSION

        num_options = cupsAddOption("collate", QString("%1").arg(printer->collateCopies()).toLocal8Bit(), num_options, &options);

        switch(printer->pageOrder())
        {
        case QPrinter::FirstPageFirst:
            num_options = cupsAddOption("outputorder", "normal", num_options, &options);
            break;
        case QPrinter::LastPageFirst:
            num_options = cupsAddOption("outputorder", "reverse", num_options, &options);
            break;
        }

        num_options = cupsAddOption("fit-to-page", QString("%1").arg(printOptions.fitToPage).toLocal8Bit(), num_options, &options);

        switch(printer->colorMode())
        {
        case QPrinter::Color:
            break;
        case QPrinter::GrayScale:
            num_options = cupsAddOption("ColorModel", "Gray", num_options, &options);
            break;
        }

        switch(printer->duplex())
        {
        case QPrinter::DuplexNone:
            num_options = cupsAddOption("sides", "one-sided", num_options, &options);
            break;
        case QPrinter::DuplexAuto:
            break;
        case QPrinter::DuplexLongSide:
            num_options = cupsAddOption("sides", "two-sided-long-edge", num_options, &options);
            break;
        case QPrinter::DuplexShortSide:
            num_options = cupsAddOption("sides", "two-sided-short-edge", num_options, &options);
            break;
        }

        switch(printOptions.numberUp)
        {
        case PrintOptions::SinglePage:
            num_options = cupsAddOption("number-up", "1", num_options, &options);
            numberUp = 1;
            break;
        case PrintOptions::TwoPages:
            num_options = cupsAddOption("number-up", "2", num_options, &options);
            numberUp = 2;
            break;
        case PrintOptions::FourPages:
            num_options = cupsAddOption("number-up", "4", num_options, &options);
            numberUp = 4;
            break;
        case PrintOptions::SixPages:
            num_options = cupsAddOption("number-up", "6", num_options, &options);
            numberUp = 6;
            break;
        case PrintOptions::NinePages:
            num_options = cupsAddOption("number-up", "9", num_options, &options);
            numberUp = 9;
            break;
        case PrintOptions::SixteenPages:
            num_options = cupsAddOption("number-up", "16", num_options, &options);
            numberUp = 16;
            break;
        }

        switch(printOptions.numberUpLayout)
        {
        case PrintOptions::BottomTopLeftRight:
            num_options = cupsAddOption("number-up-layout", "btlr", num_options, &options);
            break;
        case PrintOptions::BottomTopRightLeft:
            num_options = cupsAddOption("number-up-layout", "btrl", num_options, &options);
            break;
        case PrintOptions::LeftRightBottomTop:
            num_options = cupsAddOption("number-up-layout", "lrbt", num_options, &options);
            break;
        case PrintOptions::LeftRightTopBottom:
            num_options = cupsAddOption("number-up-layout", "lrtb", num_options, &options);
            break;
        case PrintOptions::RightLeftBottomTop:
            num_options = cupsAddOption("number-up-layout", "rlbt", num_options, &options);
            break;
        case PrintOptions::RightLeftTopBottom:
            num_options = cupsAddOption("number-up-layout", "rltb", num_options, &options);
            break;
        case PrintOptions::TopBottomLeftRight:
            num_options = cupsAddOption("number-up-layout", "tblr", num_options, &options);
            break;
        case PrintOptions::TopBottomRightLeft:
            num_options = cupsAddOption("number-up-layout", "tbrl", num_options, &options);
            break;
        }

        switch(printOptions.pageSet)
        {
        case PrintOptions::AllPages:
            break;
        case PrintOptions::EvenPages:
            num_options = cupsAddOption("page-set", "even", num_options, &options);
            break;
        case PrintOptions::OddPages:
            num_options = cupsAddOption("page-set", "odd", num_options, &options);
            break;
        }

        fromPage = (fromPage - 1) / numberUp + 1;
        toPage = (toPage - 1) / numberUp + 1;

        num_options = cupsAddOption("page-ranges", QString("%1-%2").arg(fromPage).arg(toPage).toLocal8Bit(), num_options, &options);

        // TODO: remove debug output
        for(int index = 0; index < num_options; ++index)
        {
            qDebug() << options[index].name << options[index].value;
        }

        QFileInfo fileInfo(m_filePath);

        jobId = cupsPrintFile(dest->name, fileInfo.absoluteFilePath().toLocal8Bit(), fileInfo.completeBaseName().toLocal8Bit(), num_options, options);

        if(jobId < 1)
        {
            qDebug() << cupsLastErrorString();
        }
    }
    else
    {
        qDebug() << cupsLastErrorString();
    }

    cupsFreeDests(num_dests, dests);
    cupsFreeOptions(num_options, options);

    return jobId >= 1;

#else

    QProgressDialog* progressDialog = new QProgressDialog(this);
    progressDialog->setLabelText(tr("Printing '%1'...").arg(m_filePath));
    progressDialog->setRange(fromPage - 1, toPage);

    QPainter painter;
    painter.begin(printer);

    for(int index = fromPage - 1; index <= toPage - 1; ++index)
    {
        progressDialog->setValue(index);

        QApplication::processEvents();

        {
            m_mutex.lock();

            Poppler::Page* page = m_document->page(index);

            qreal pageWidth =  printer->physicalDpiX() / 72.0 * page->pageSizeF().width();
            qreal pageHeight = printer->physicalDpiY() / 72.0 * page->pageSizeF().height();

            QImage image = page->renderToImage(printer->physicalDpiX(), printer->physicalDpiY());

            delete page;

            m_mutex.unlock();

            qreal scaleFactorX = 1.0, scaleFactorY = 1.0;

            if(printOptions.fitToPage)
            {
                scaleFactorX = scaleFactorY = qMin(printer->width() / pageWidth, printer->height() / pageHeight);
            }
            else
            {
                scaleFactorX = printer->logicalDpiX(); scaleFactorX /= printer->physicalDpiX();
                scaleFactorY = printer->logicalDpiY(); scaleFactorY /= printer->physicalDpiY();
            }

            painter.setTransform(QTransform::fromScale(scaleFactorX, scaleFactorY));
            painter.drawImage(QPointF(), image);
        }

        if(index < toPage - 1)
        {
            printer->newPage();
        }

        QApplication::processEvents();

        if(progressDialog->wasCanceled())
        {
            delete progressDialog;
            return false;
        }
    }

    painter.end();

    delete progressDialog;
    return true;

#endif // WITH_CUPS
}

void DocumentView::previousPage()
{
    int previousPage = m_currentPage;

    switch(m_layoutMode)
    {
    case SinglePageMode:
        previousPage -= 1;
        break;
    case TwoPagesMode:
    case TwoPagesWithCoverPageMode:
        previousPage -= 2;
        break;
    case MultiplePagesMode:
        previousPage -= s_pagesPerRow;
        break;
    }

    previousPage = previousPage >= 1 ? previousPage : 1;

    jumpToPage(previousPage, false);
}

void DocumentView::nextPage()
{
    int nextPage = m_currentPage;

    switch(m_layoutMode)
    {
    case SinglePageMode:
        nextPage += 1;
        break;
    case TwoPagesMode:
    case TwoPagesWithCoverPageMode:
        nextPage += 2;
        break;
    case MultiplePagesMode:
        nextPage += s_pagesPerRow;
        break;
    }

    nextPage = nextPage <= m_numberOfPages ? nextPage : m_numberOfPages;

    jumpToPage(nextPage, false);
}

void DocumentView::firstPage()
{
    jumpToPage(1);
}

void DocumentView::lastPage()
{
    jumpToPage(m_numberOfPages);
}

void DocumentView::jumpToPage(int page, bool returnTo, qreal changeLeft, qreal changeTop)
{
    if(page >= 1 && page <= m_numberOfPages)
    {
        qreal left = 0.0, top = 0.0;
        saveLeftAndTop(left, top);

        if(m_currentPage != currentPageForPage(page) || qAbs(left - changeLeft) > 0.01 || qAbs(top - changeTop) > 0.01)
        {
            if(returnTo)
            {
                m_returnToPage.push(m_currentPage);
                m_returnToLeft.push(left); m_returnToTop.push(top);
            }

            m_currentPage = currentPageForPage(page);

            prepareView(changeLeft, changeTop);

            emit currentPageChanged(m_currentPage, returnTo);
        }
    }
}

void DocumentView::jumpToHighlight(const QRectF& highlight)
{
    PageItem* page = m_pages.at(m_currentPage - 1);

    page->setHighlights(QList< QRectF >() << highlight, s_highlightDuration);

    disconnect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(on_verticalScrollBar_valueChanged(int)));
    centerOn(page->transform().mapRect(highlight).translated(page->pos()).center());
    connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(on_verticalScrollBar_valueChanged(int)));
}

void DocumentView::startSearch(const QString& text, bool matchCase)
{
    cancelSearch();

    QList< int > indices;

    for(int index = m_currentPage - 1; index < m_numberOfPages; ++index)
    {
        indices.append(index);
    }

    for(int index = 0; index < m_currentPage - 1; ++index)
    {
        indices.append(index);
    }

    m_searchThread->start(&m_mutex, m_document, indices, text, matchCase);
}

void DocumentView::cancelSearch()
{
    m_searchThread->cancel();
    m_searchThread->wait();

    m_results.clear();
    m_currentResult = m_results.end();

    foreach(PageItem* page, m_pages)
    {
        page->clearHighlights();
    }

    prepareHighlight();
}

void DocumentView::findPrevious()
{
    if(m_currentResult != m_results.end())
    {
        if(leftIndexForIndex(m_currentResult.key()) == m_currentPage - 1)
        {
            --m_currentResult;
        }
        else
        {
            m_currentResult = --m_results.upperBound(m_currentPage - 1);
        }
    }
    else
    {
        m_currentResult = --m_results.upperBound(m_currentPage - 1);
    }

    if(m_currentResult == m_results.end())
    {
        m_currentResult = --m_results.end();
    }

    prepareHighlight();
}

void DocumentView::findNext()
{
    if(m_currentResult != m_results.end())
    {
        if(leftIndexForIndex(m_currentResult.key()) == m_currentPage - 1)
        {
            ++m_currentResult;
        }
        else
        {
            m_currentResult = m_results.lowerBound(m_currentPage - 1);
        }
    }
    else
    {
        m_currentResult = m_results.lowerBound(m_currentPage - 1);
    }

    if(m_currentResult == m_results.end())
    {
        m_currentResult = m_results.begin();
    }

    prepareHighlight();
}

void DocumentView::zoomIn()
{
    if(scaleMode() != ScaleFactor)
    {
        setScaleFactor(qMin(m_pages.at(m_currentPage - 1)->scaleFactor() + s_zoomBy, s_maximumScaleFactor));
        setScaleMode(ScaleFactor);
    }
    else
    {
        setScaleFactor(qMin(scaleFactor() + s_zoomBy, s_maximumScaleFactor));
    }
}

void DocumentView::zoomOut()
{
    if(scaleMode() != ScaleFactor)
    {
        setScaleFactor(qMax(m_pages.at(m_currentPage - 1)->scaleFactor() - s_zoomBy, s_minimumScaleFactor));
        setScaleMode(ScaleFactor);
    }
    else
    {
        setScaleFactor(qMax(scaleFactor() - s_zoomBy, s_minimumScaleFactor));
    }
}

void DocumentView::originalSize()
{
    setScaleFactor(1.0);
    setScaleMode(ScaleFactor);
}

void DocumentView::rotateLeft()
{
    switch(rotation())
    {
    case Poppler::Page::Rotate0:
        setRotation(Poppler::Page::Rotate270);
        break;
    case Poppler::Page::Rotate90:
        setRotation(Poppler::Page::Rotate0);
        break;
    case Poppler::Page::Rotate180:
        setRotation(Poppler::Page::Rotate90);
        break;
    case Poppler::Page::Rotate270:
        setRotation(Poppler::Page::Rotate180);
        break;
    }
}

void DocumentView::rotateRight()
{
    switch(rotation())
    {
    case Poppler::Page::Rotate0:
        setRotation(Poppler::Page::Rotate90);
        break;
    case Poppler::Page::Rotate90:
        setRotation(Poppler::Page::Rotate180);
        break;
    case Poppler::Page::Rotate180:
        setRotation(Poppler::Page::Rotate270);
        break;
    case Poppler::Page::Rotate270:
        setRotation(Poppler::Page::Rotate0);
        break;
    }
}

void DocumentView::presentation(bool sync, int screen)
{
    if(screen < -1 || screen >= QApplication::desktop()->screenCount())
    {
        screen = -1;
    }

    PresentationView* presentationView = new PresentationView(&m_mutex, m_document);

    presentationView->setGeometry(QApplication::desktop()->screenGeometry(screen));

    presentationView->show();
    presentationView->setAttribute(Qt::WA_DeleteOnClose);

    connect(this, SIGNAL(destroyed()), presentationView, SLOT(close()));
    connect(this, SIGNAL(filePathChanged(QString)), presentationView, SLOT(close()));
    connect(this, SIGNAL(numberOfPagesChanged(int)), presentationView, SLOT(close()));

    presentationView->jumpToPage(currentPage(), false);

    if(sync)
    {
        connect(this, SIGNAL(currentPageChanged(int,bool)), presentationView, SLOT(jumpToPage(int,bool)));
        connect(presentationView, SIGNAL(currentPageChanged(int,bool)), this, SLOT(jumpToPage(int,bool)));
    }
}

void DocumentView::on_verticalScrollBar_valueChanged(int value)
{
    if(m_continuousMode)
    {
        QRectF visibleRect = mapToScene(viewport()->rect()).boundingRect();

        foreach(PageItem* page, m_pages)
        {
            if(!page->boundingRect().translated(page->pos()).intersects(visibleRect))
            {
                page->cancelRender();
            }
        }

        QMap< qreal, int >::const_iterator lowerBound = m_heightToIndex.lowerBound(-value);

        if(lowerBound != m_heightToIndex.constEnd())
        {
            int page = lowerBound.value() + 1;

            if(m_currentPage != page)
            {
                m_currentPage = page;

                emit currentPageChanged(m_currentPage);
            }
        }
    }
}

void DocumentView::on_searchThread_resultsReady(int index, QList< QRectF > results)
{
    if(m_searchThread->wasCanceled())
    {
        return;
    }

    while(!results.isEmpty())
    {
        m_results.insertMulti(index, results.takeLast());
    }

    if(m_highlightAll)
    {
        m_pages.at(index)->setHighlights(m_results.values(index));
    }

    if(m_results.contains(index) && m_currentResult == m_results.end())
    {
        findNext();
    }
}

void DocumentView::on_prefetch_timeout()
{
    int fromPage = m_currentPage, toPage = m_currentPage;

    switch(m_layoutMode)
    {
    case SinglePageMode:
        fromPage -= 1;
        toPage += 1;
        break;
    case TwoPagesMode:
    case TwoPagesWithCoverPageMode:
        fromPage -= 2;
        toPage += 3;
        break;
    case MultiplePagesMode:
        fromPage -= s_pagesPerRow;
        toPage += 2 * s_pagesPerRow - 1;
        break;
    }

    fromPage = fromPage >= 1 ? fromPage : 1;
    toPage = toPage <= m_numberOfPages ? toPage : m_numberOfPages;

    for(int index = fromPage - 1; index <= toPage - 1; ++index)
    {
        m_pages.at(index)->startRender(true);
    }
}

void DocumentView::on_pages_linkClicked(int page, qreal left, qreal top)
{
    page = page >= 1 ? page : 1;
    page = page <= m_numberOfPages ? page : m_numberOfPages;

    left = left >= 0.0 ? left : 0.0;
    left = left <= 1.0 ? left : 1.0;

    top = top >= 0.0 ? top : 0.0;
    top = top <= 1.0 ? top : 1.0;

    jumpToPage(page, true, left, top);
}

void DocumentView::on_pages_linkClicked(const QString& url)
{
    if(s_openUrl)
    {
        QDesktopServices::openUrl(QUrl(url));
    }
    else
    {
        QMessageBox::information(this, tr("Information"), tr("Opening URL is disabled in the settings."));
    }
}

void DocumentView::on_pages_rubberBandFinished()
{
    setRubberBandMode(PageItem::ModifiersMode);
}

void DocumentView::on_pages_sourceRequested(int page, const QPointF& pos)
{
#ifdef WITH_SYNCTEX

    if(s_sourceEditor.isEmpty())
    {
        return;
    }

    synctex_scanner_t scanner = synctex_scanner_new_with_output_file(QFileInfo(m_filePath).absoluteFilePath().toLocal8Bit(), 0, 1);

    if(scanner != 0)
    {
        if(synctex_edit_query(scanner, page, pos.x(), pos.y()) > 0)
        {
            for(synctex_node_t node = synctex_next_result(scanner); node != 0; node = synctex_next_result(scanner))
            {
                QString path = QFileInfo(m_filePath).path();

                QString sourceName = QString::fromLocal8Bit(synctex_scanner_get_name(scanner, synctex_node_tag(node)));
                int sourceLine = synctex_node_line(node);
                int sourceColumn = synctex_node_column(node);

                sourceLine = sourceLine >= 0 ? sourceLine : 0;
                sourceColumn = sourceColumn >= 0 ? sourceColumn : 0;

                QProcess::startDetached(s_sourceEditor.arg(QFileInfo(QDir(path), sourceName).absoluteFilePath(), QString::number(sourceLine), QString::number(sourceColumn)));

                break;
            }
        }

        synctex_scanner_free(scanner);
    }
    else
    {
        QMessageBox::warning(this, tr("Warning"), tr("SyncTeX data for '%1' could not be found.").arg(QFileInfo(m_filePath).absoluteFilePath()));
    }

#endif // WITH_SYNCTEX
}

void DocumentView::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);

    if(m_scaleMode != ScaleFactor)
    {
        qreal left = 0.0, top = 0.0;
        saveLeftAndTop(left, top);

        prepareScene();
        prepareView(left, top);
    }
}

void DocumentView::keyPressEvent(QKeyEvent* event)
{
    if(event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        if(!m_returnToPage.isEmpty())
        {
            jumpToPage(m_returnToPage.pop(), false, m_returnToLeft.pop(), m_returnToTop.pop());
        }

        event->accept();
        return;
    }

    if(!m_continuousMode && event->modifiers() == Qt::NoModifier)
    {
        if(event->key() == Qt::Key_PageUp && verticalScrollBar()->value() == verticalScrollBar()->minimum() && m_currentPage != 1)
        {
            previousPage();

            verticalScrollBar()->setValue(verticalScrollBar()->maximum());

            event->accept();
            return;
        }
        else if(event->key() == Qt::Key_PageDown && verticalScrollBar()->value() == verticalScrollBar()->maximum() && m_currentPage != currentPageForPage(m_numberOfPages))
        {
            nextPage();

            verticalScrollBar()->setValue(verticalScrollBar()->minimum());

            event->accept();
            return;
        }
    }

    QGraphicsView::keyPressEvent(event);
}

void DocumentView::wheelEvent(QWheelEvent* event)
{
    if(event->modifiers() == s_zoomModifiers)
    {
        if(event->delta() > 0)
        {
            zoomIn();
        }
        else
        {
            zoomOut();
        }

        event->accept();
        return;
    }
    else if(event->modifiers() == s_rotateModifiers)
    {
        if(event->delta() > 0)
        {
            rotateLeft();
        }
        else
        {
            rotateRight();
        }

        event->accept();
        return;
    }
    else if(event->modifiers() == s_horizontalModifiers)
    {
        QWheelEvent wheelEvent(event->pos(), event->delta(), event->buttons(), Qt::AltModifier, Qt::Horizontal);
        QApplication::sendEvent(horizontalScrollBar(), &wheelEvent);

        event->accept();
        return;
    }
    else if(event->modifiers() == Qt::NoModifier)
    {
        if(!m_continuousMode)
        {
            if(event->delta() > 0 && verticalScrollBar()->value() == verticalScrollBar()->minimum() && m_currentPage != 1)
            {
                previousPage();

                verticalScrollBar()->setValue(verticalScrollBar()->maximum());

                event->accept();
                return;
            }
            else if(event->delta() < 0 && verticalScrollBar()->value() == verticalScrollBar()->maximum() && m_currentPage != currentPageForPage(m_numberOfPages))
            {
                nextPage();

                verticalScrollBar()->setValue(verticalScrollBar()->minimum());

                event->accept();
                return;
            }
        }
    }

    QGraphicsView::wheelEvent(event);
}

void DocumentView::contextMenuEvent(QContextMenuEvent* event)
{
    event->setAccepted(false);

    QGraphicsView::contextMenuEvent(event);

    if(!event->isAccepted())
    {
        event->setAccepted(true);

        QMenu* menu = new QMenu();

        QAction* returnToPageAction = menu->addAction(tr("&Return to page %1").arg(!m_returnToPage.isEmpty() ? m_returnToPage.top() : -1));
        returnToPageAction->setShortcut(QKeySequence(Qt::Key_Return));
        returnToPageAction->setIcon(QIcon::fromTheme("go-jump", QIcon(":icons/go-jump.svg")));
        returnToPageAction->setIconVisibleInMenu(true);
        returnToPageAction->setVisible(!m_returnToPage.isEmpty());

        menu->addSeparator();

        QAction* previousPageAction = menu->addAction(tr("&Previous page"));
        previousPageAction->setShortcut(QKeySequence(Qt::Key_Backspace));
        previousPageAction->setIcon(QIcon::fromTheme("go-previous", QIcon(":icons/go-previous.svg")));
        previousPageAction->setIconVisibleInMenu(true);

        QAction* nextPageAction = menu->addAction(tr("&Next page"));
        nextPageAction->setShortcut(QKeySequence(Qt::Key_Space));
        nextPageAction->setIcon(QIcon::fromTheme("go-next", QIcon(":icons/go-next.svg")));
        nextPageAction->setIconVisibleInMenu(true);

        QAction* firstPageAction = menu->addAction(tr("&First page"));
        firstPageAction->setShortcut(QKeySequence(Qt::Key_Home));
        firstPageAction->setIcon(QIcon::fromTheme("go-first", QIcon(":icons/go-first.svg")));
        firstPageAction->setIconVisibleInMenu(true);

        QAction* lastPageAction = menu->addAction(tr("&Last page"));
        lastPageAction->setShortcut(QKeySequence(Qt::Key_End));
        lastPageAction->setIcon(QIcon::fromTheme("go-last", QIcon(":icons/go-last.svg")));
        lastPageAction->setIconVisibleInMenu(true);

        menu->addSeparator();

        QAction* refreshAction = menu->addAction(tr("&Refresh"));
        refreshAction->setShortcut(QKeySequence::Refresh);
        refreshAction->setIcon(QIcon::fromTheme("view-refresh", QIcon(":icons/view-refresh.svg")));
        refreshAction->setIconVisibleInMenu(true);

        QAction* action = menu->exec(event->globalPos());

        if(action == returnToPageAction)
        {
            jumpToPage(m_returnToPage.pop(), false, m_returnToLeft.pop(), m_returnToTop.pop());
        }
        else if(action == previousPageAction) { previousPage(); }
        else if(action == nextPageAction) { nextPage(); }
        else if(action == firstPageAction) { firstPage(); }
        else if(action == lastPageAction) { lastPage(); }
        else if(action == refreshAction) { refresh(); }

        delete menu;
    }
}

int DocumentView::currentPageForPage(int page) const
{
    int currentPage = -1;

    switch(m_layoutMode)
    {
    case SinglePageMode:
        currentPage = page;
        break;
    case TwoPagesMode:
        currentPage = page % 2 != 0 ? page : page - 1;
        break;
    case TwoPagesWithCoverPageMode:
        currentPage = page == 1 ? page : (page % 2 == 0 ? page : page - 1);
        break;
    case MultiplePagesMode:
        currentPage = page - ((page - 1) % s_pagesPerRow);
        break;
    }

    return currentPage;
}

int DocumentView::leftIndexForIndex(int index) const
{
    int leftIndex = -1;

    switch(m_layoutMode)
    {
    case SinglePageMode:
        leftIndex = index;
        break;
    case TwoPagesMode:
        leftIndex = index % 2 == 0 ? index : index - 1;
        break;
    case TwoPagesWithCoverPageMode:
        leftIndex = index == 0 ? index : (index % 2 != 0 ? index : index - 1);
        break;
    case MultiplePagesMode:
        leftIndex = index - (index % s_pagesPerRow);
        break;
    }

    return leftIndex;
}

int DocumentView::rightIndexForIndex(int index) const
{
    int rightIndex = -1;

    switch(m_layoutMode)
    {
    case SinglePageMode:
        rightIndex = index;
        break;
    case TwoPagesMode:
        rightIndex = index % 2 == 0 ? index + 1 : index;
        break;
    case TwoPagesWithCoverPageMode:
        rightIndex = index % 2 != 0 ? index + 1 : index;
        break;
    case MultiplePagesMode:
        rightIndex = index - (index % s_pagesPerRow) + s_pagesPerRow - 1;
        break;
    }

    rightIndex = rightIndex <= m_numberOfPages - 1 ? rightIndex : m_numberOfPages - 1;

    return rightIndex;
}

void DocumentView::saveLeftAndTop(qreal& left, qreal& top) const
{
    PageItem* page = m_pages.at(m_currentPage - 1);

    QRectF boundingRect = page->boundingRect().translated(page->pos());
    QPointF topLeft = mapToScene(viewport()->rect().topLeft());

    left = (topLeft.x() - boundingRect.x()) / boundingRect.width();
    top = (topLeft.y() - boundingRect.y()) / boundingRect.height();
}

void DocumentView::prepareDocument(Poppler::Document* document)
{
    m_prefetchTimer->blockSignals(true);
    m_prefetchTimer->stop();

    cancelSearch();

    qDeleteAll(m_pages);
    qDeleteAll(m_thumbnails);

    if(m_document != 0)
    {
        delete m_document;

        if(!m_autoRefreshWatcher->files().isEmpty())
        {
            m_autoRefreshWatcher->removePaths(m_autoRefreshWatcher->files());
        }
    }

    m_document = document;

    if(s_autoRefresh)
    {
        m_autoRefreshWatcher->addPath(m_filePath);
    }

    m_document->setRenderHint(Poppler::Document::Antialiasing, s_antialiasing);
    m_document->setRenderHint(Poppler::Document::TextAntialiasing, s_textAntialiasing);
    m_document->setRenderHint(Poppler::Document::TextHinting, s_textHinting);

    preparePages();
    prepareThumbnails();
    prepareOutline();
    prepareProperties();

    if(s_prefetch)
    {
        m_prefetchTimer->blockSignals(false);
        m_prefetchTimer->start();
    }
}

void DocumentView::preparePages()
{
    m_pages.clear();
    m_pages.reserve(m_numberOfPages);

    for(int index = 0; index < m_numberOfPages; ++index)
    {
        PageItem* page = new PageItem(&m_mutex, m_document->page(index), index);

        page->setPhysicalDpi(physicalDpiX(), physicalDpiY());
        page->setRubberBandMode(m_rubberBandMode);

        m_pagesScene->addItem(page);
        m_pages.append(page);

        connect(page, SIGNAL(linkClicked(int,qreal,qreal)), SLOT(on_pages_linkClicked(int,qreal,qreal)));
        connect(page, SIGNAL(linkClicked(QString)), SLOT(on_pages_linkClicked(QString)));

        connect(page, SIGNAL(rubberBandFinished()), SLOT(on_pages_rubberBandFinished()));

        connect(page, SIGNAL(sourceRequested(int,QPointF)), SLOT(on_pages_sourceRequested(int,QPointF)));
    }

    if(PageItem::decoratePages())
    {
        m_pagesScene->setBackgroundBrush(QBrush(Qt::darkGray));
    }
    else
    {
        m_pagesScene->setBackgroundBrush(QBrush(PageItem::invertColors() ? Qt::black : Qt::white));
    }
}

void DocumentView::prepareThumbnails()
{
    m_thumbnails.clear();
    m_thumbnails.reserve(m_numberOfPages);

    m_thumbnailsScene->clear();

    qreal left = 0.0;
    qreal right = 0.0;
    qreal height = s_thumbnailSpacing;

    for(int index = 0; index < m_numberOfPages; ++index)
    {
        ThumbnailItem* page = new ThumbnailItem(&m_mutex, m_document->page(index), index);

        page->setPhysicalDpi(physicalDpiX(), physicalDpiY());

        m_thumbnailsScene->addItem(page);
        m_thumbnails.append(page);

        connect(page, SIGNAL(linkClicked(int,qreal,qreal)), SLOT(on_pages_linkClicked(int,qreal,qreal)));

        {
            // prepare scale factor

            QSizeF size = page->size();

            qreal pageWidth = physicalDpiX() / 72.0 * size.width();
            qreal pageHeight = physicalDpiY() / 72.0 * size.height();

            page->setScaleFactor(qMin(s_thumbnailSize / pageWidth, s_thumbnailSize / pageHeight));
        }

        {
            // prepare layout

            QRectF boundingRect = page->boundingRect();

            page->setPos(-boundingRect.left() - 0.5 * boundingRect.width(), height - boundingRect.top());

            left = qMin(left, -0.5f * boundingRect.width() - s_thumbnailSpacing);
            right = qMax(right, 0.5f * boundingRect.width() + s_thumbnailSpacing);
            height += boundingRect.height() + s_thumbnailSpacing;
        }

        QGraphicsSimpleTextItem* text = m_thumbnailsScene->addSimpleText(QString::number(index + 1));

        text->setPos(-0.5 * text->boundingRect().width(), height);

        height += text->boundingRect().height() + s_thumbnailSpacing;
    }

    if(PageItem::decoratePages())
    {
        m_thumbnailsScene->setBackgroundBrush(QBrush(Qt::darkGray));
    }
    else
    {
        m_thumbnailsScene->setBackgroundBrush(QBrush(PageItem::invertColors() ? Qt::black : Qt::white));
    }

    m_thumbnailsScene->setSceneRect(left, 0.0, right - left, height);
}

void DocumentView::prepareOutline()
{
    m_outlineModel->clear();

    QDomDocument* toc = m_document->toc();

    if(toc != 0)
    {
        prepareOutline(toc->firstChild(), m_outlineModel->invisibleRootItem());

        delete toc;
    }
}

void DocumentView::prepareOutline(const QDomNode& node, QStandardItem* parent)
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
        linkDestination = m_document->linkDestination(element.attribute("DestinationName"));
    }

    if(linkDestination != 0)
    {
        int page = linkDestination->pageNumber();
        qreal left = 0.0;
        qreal top = 0.0;

        page = page >= 1 ? page : 1;
        page = page <= m_numberOfPages ? page : m_numberOfPages;

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
        prepareOutline(siblingNode, parent);
    }

    QDomNode childNode = node.firstChild();
    if(!childNode.isNull())
    {
        prepareOutline(childNode, item);
    }
}

void DocumentView::prepareProperties()
{
    m_propertiesModel->clear();

    QStringList keys = m_document->infoKeys();

    m_propertiesModel->setRowCount(keys.count());
    m_propertiesModel->setColumnCount(2);

    for(int index = 0; index < keys.count(); ++index)
    {
        QString key = keys.at(index);
        QString value = m_document->info(key);

        if(value.startsWith("D:"))
        {
            value = m_document->date(key).toString();
        }

        m_propertiesModel->setItem(index, 0, new QStandardItem(key));
        m_propertiesModel->setItem(index, 1, new QStandardItem(value));
    }
}

void DocumentView::prepareScene()
{
    // prepare scale factor and rotation

    for(int index = 0; index < m_numberOfPages; ++index)
    {
        PageItem* page = m_pages.at(index);
        QSizeF size = page->size();

        if(m_scaleMode != ScaleFactor)
        {
            qreal visibleWidth = 0.0;
            qreal visibleHeight = 0.0;

            qreal pageWidth = 0.0;
            qreal pageHeight = 0.0;

            qreal scaleFactor = 1.0;

            switch(m_layoutMode)
            {
            case SinglePageMode:
                visibleWidth = viewport()->width() - 6.0 - 2.0 * s_pageSpacing;
                break;
            case TwoPagesMode:
            case TwoPagesWithCoverPageMode:
                visibleWidth = (viewport()->width() - 6.0 - 3 * s_pageSpacing) / 2;
                break;
            case MultiplePagesMode:
                visibleWidth = (viewport()->width() - 6.0 - (s_pagesPerRow + 1) * s_pageSpacing) / s_pagesPerRow;
                break;
            }

            visibleHeight = viewport()->height() - 2.0 * s_pageSpacing;

            switch(m_rotation)
            {
            case Poppler::Page::Rotate0:
            case Poppler::Page::Rotate180:
                pageWidth = physicalDpiX() / 72.0 * size.width();
                pageHeight = physicalDpiY() / 72.0 * size.height();
                break;
            case Poppler::Page::Rotate90:
            case Poppler::Page::Rotate270:
                pageWidth = physicalDpiX() / 72.0 * size.height();
                pageHeight = physicalDpiY() / 72.0 * size.width();
                break;
            }

            switch(m_scaleMode)
            {
            case ScaleFactor:
                break;
            case FitToPageWidth:
                scaleFactor = visibleWidth / pageWidth;
                break;
            case FitToPageSize:
                scaleFactor = qMin(visibleWidth / pageWidth, visibleHeight / pageHeight);
                break;
            }

            page->setScaleFactor(scaleFactor);
        }
        else
        {
            page->setScaleFactor(m_scaleFactor);
        }

        page->setRotation(m_rotation);
    }

    // prepare layout

    m_heightToIndex.clear();

    qreal pageHeight = 0.0;

    qreal left = 0.0;
    qreal right = 0.0;
    qreal height = s_pageSpacing;

    for(int index = 0; index < m_numberOfPages; ++index)
    {
        PageItem* page = m_pages.at(index);
        QRectF boundingRect = page->boundingRect();

        switch(m_layoutMode)
        {
        case SinglePageMode:
            page->setPos(-boundingRect.left() - 0.5 * boundingRect.width(), height - boundingRect.top());

            m_heightToIndex.insert(-height + s_pageSpacing + 0.3 * pageHeight, index);

            pageHeight = boundingRect.height();

            left = qMin(left, -0.5f * boundingRect.width() - s_pageSpacing);
            right = qMax(right, 0.5f * boundingRect.width() + s_pageSpacing);
            height += pageHeight + s_pageSpacing;

            break;
        case TwoPagesMode:
        case TwoPagesWithCoverPageMode:
            if(index == leftIndexForIndex(index))
            {
                page->setPos(-boundingRect.left() - boundingRect.width() - 0.5 * s_pageSpacing, height - boundingRect.top());

                m_heightToIndex.insert(-height + s_pageSpacing + 0.3 * pageHeight, index);

                pageHeight = boundingRect.height();

                left = qMin(left, -boundingRect.width() - 1.5f * s_pageSpacing);

                if(index == rightIndexForIndex(index))
                {
                    right = qMax(right, 0.5f * s_pageSpacing);
                    height += pageHeight + s_pageSpacing;
                }
            }
            else
            {
                page->setPos(-boundingRect.left() + 0.5 * s_pageSpacing, height - boundingRect.top());

                pageHeight = qMax(pageHeight, boundingRect.height());

                right = qMax(right, boundingRect.width() + 1.5f * s_pageSpacing);
                height += pageHeight + s_pageSpacing;
            }

            break;
        case MultiplePagesMode:
            page->setPos(left - boundingRect.left() + s_pageSpacing, height - boundingRect.top());

            pageHeight = qMax(pageHeight, boundingRect.height());
            left += boundingRect.width() + s_pageSpacing;

            if(index == leftIndexForIndex(index))
            {
                m_heightToIndex.insert(-height + s_pageSpacing + 0.3 * pageHeight, index);
            }

            if(index == rightIndexForIndex(index))
            {
                height += pageHeight + s_pageSpacing;
                pageHeight = 0.0;

                right = qMax(right, left + s_pageSpacing);
                left = 0.0;
            }

            break;
        }
    }

    m_pagesScene->setSceneRect(left, 0.0, right - left, height);
}

void DocumentView::prepareView(qreal changeLeft, qreal changeTop)
{
    qreal left = m_pagesScene->sceneRect().left();
    qreal top = m_pagesScene->sceneRect().top();
    qreal width = m_pagesScene->sceneRect().width();
    qreal height = m_pagesScene->sceneRect().height();

    int horizontalValue = 0;
    int verticalValue = 0;

    for(int index = 0; index < m_pages.count(); ++index)
    {
        PageItem* page = m_pages.at(index);

        if(m_continuousMode)
        {
            page->setVisible(true);

            if(index == m_currentPage - 1)
            {
                QRectF boundingRect = page->boundingRect().translated(page->pos());

                horizontalValue = qFloor(boundingRect.left() + changeLeft * boundingRect.width());
                verticalValue = qFloor(boundingRect.top() + changeTop * boundingRect.height());
            }
        }
        else
        {
            if(leftIndexForIndex(index) == m_currentPage - 1)
            {
                page->setVisible(true);

                QRectF boundingRect = page->boundingRect().translated(page->pos());

                top = boundingRect.top() - s_pageSpacing;
                height = boundingRect.height() + 2.0 * s_pageSpacing;

                if(index == m_currentPage - 1)
                {
                    horizontalValue = qFloor(boundingRect.left() + changeLeft * boundingRect.width());
                    verticalValue = qFloor(boundingRect.top() + changeTop * boundingRect.height());
                }
            }
            else
            {
                page->setVisible(false);

                page->cancelRender();
            }
        }

        if(m_currentResult != m_results.end())
        {
            if(m_currentResult.key() == index)
            {
                m_highlight->setPos(page->pos());
                m_highlight->setTransform(page->transform());

                page->stackBefore(m_highlight);
            }
        }
    }

    setSceneRect(left, top, width, height);

    horizontalScrollBar()->setValue(horizontalValue);
    verticalScrollBar()->setValue(verticalValue);

    viewport()->update();
}

void DocumentView::prepareHighlight()
{
    if(m_currentResult != m_results.end())
    {
        jumpToPage(m_currentResult.key() + 1);

        PageItem* page = m_pages.at(m_currentResult.key());

        m_highlight->setPos(page->pos());
        m_highlight->setTransform(page->transform());

        page->stackBefore(m_highlight);

        m_highlight->setRect(m_currentResult.value().normalized());

        disconnect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(on_verticalScrollBar_valueChanged(int)));
        centerOn(m_highlight);
        connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(on_verticalScrollBar_valueChanged(int)));

        m_highlight->setVisible(true);
    }
    else
    {
        m_highlight->setVisible(false);
    }
}
