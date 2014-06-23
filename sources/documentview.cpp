/*

Copyright 2012-2014 Adam Reichold
Copyright 2014 Dorian Scholz
Copyright 2013 Thomas Etter

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

#include <QApplication>
#include <QInputDialog>
#include <QDebug>
#include <QDesktopWidget>
#include <QDesktopServices>
#include <QDir>
#include <QFileSystemWatcher>
#include <QKeyEvent>
#include <qmath.h>
#include <QMenu>
#include <QMessageBox>
#include <QPrintEngine>
#include <QProcess>
#include <QProgressDialog>
#include <QScrollBar>
#include <QTemporaryFile>
#include <QTimer>
#include <QUrl>

#ifdef WITH_CUPS

#include <cups/cups.h>
#include <cups/ppd.h>

#endif // WITH_CUPS

#ifdef WITH_SYNCTEX

#include <synctex_parser.h>

#endif // WITH_SYNCTEX

#include "settings.h"
#include "model.h"
#include "pluginhandler.h"
#include "shortcuthandler.h"
#include "pageitem.h"
#include "thumbnailitem.h"
#include "presentationview.h"
#include "searchtask.h"
#include "miscellaneous.h"
#include "documentlayout.h"

namespace
{

using namespace qpdfview;

qreal unscaledPageWidth(PageItem* page)
{
    switch(page->rotation())
    {
    default:
    case RotateBy0:
    case RotateBy180:
        return page->resolutionX() / 72.0 * page->size().width();
    case RotateBy90:
    case RotateBy270:
        return page->resolutionX() / 72.0 * page->size().height();
    }
}

qreal unscaledPageHeight(PageItem* page)
{
    switch(page->rotation())
    {
    default:
    case RotateBy0:
    case RotateBy180:
        return page->resolutionY() / 72.0 * page->size().height();
    case RotateBy90:
    case RotateBy270:
        return page->resolutionY() / 72.0 * page->size().width();
    }
}

#ifdef WITH_CUPS

struct RemovePpdFileDeleter
{
    static inline void cleanup(const char* ppdFileName) { if(ppdFileName != 0) { QFile::remove(ppdFileName); } }
};

struct ClosePpdFileDeleter
{
    static inline void cleanup(ppd_file_t* ppdFile) { if(ppdFile != 0) { ppdClose(ppdFile); } }
};

int addCMYKorRGBColorModel(cups_dest_t* dest, int num_options, cups_option_t** options)
{
    QScopedPointer< const char, RemovePpdFileDeleter > ppdFileName(cupsGetPPD(dest->name));

    if(ppdFileName.isNull())
    {
        return num_options;
    }

    QScopedPointer< ppd_file_t, ClosePpdFileDeleter > ppdFile(ppdOpenFile(ppdFileName.data()));

    if(ppdFile.isNull())
    {
        return num_options;
    }

    ppd_option_t* colorModel = ppdFindOption(ppdFile.data(), "ColorModel");

    if(colorModel == 0)
    {
        return num_options;
    }

    for(int index = 0; index < colorModel->num_choices; ++index)
    {
        if(qstrncmp(colorModel->choices[index].choice, "CMYK", 4) == 0)
        {
            return cupsAddOption("ColorModel", "CMYK", num_options, options);
        }
    }

    for(int index = 0; index < colorModel->num_choices; ++index)
    {
        if(qstrncmp(colorModel->choices[index].choice, "RGB", 3) == 0)
        {
            return cupsAddOption("ColorModel", "RGB", num_options, options);
        }
    }

    return num_options;
}

#endif // WITH_CUPS

} // anonymous

namespace qpdfview
{

Settings* DocumentView::s_settings = 0;
ShortcutHandler* DocumentView::s_shortcutHandler = 0;

DocumentView::DocumentView(QWidget* parent) : QGraphicsView(parent),
    m_autoRefreshWatcher(0),
    m_autoRefreshTimer(0),
    m_prefetchTimer(0),
    m_document(0),
    m_pages(),
    m_fileInfo(),
    m_wasModified(false),
    m_currentPage(-1),
    m_past(),
    m_future(),
    m_layout(new SinglePageLayout),
    m_continuousMode(false),
    m_scaleMode(ScaleFactorMode),
    m_scaleFactor(1.0),
    m_rotation(RotateBy0),
    m_invertColors(false),
    m_highlightAll(false),
    m_rubberBandMode(ModifiersMode),
    m_pageItems(),
    m_thumbnailItems(),
    m_heightToIndex(),
    m_highlight(0),
    m_thumbnailsOrientation(Qt::Vertical),
    m_thumbnailsScene(0),
    m_outlineModel(0),
    m_propertiesModel(0),
    m_results(),
    m_currentResult(m_results.end()),
    m_searchTask(0)
{
    if(s_settings == 0)
    {
        s_settings = Settings::instance();
    }

    if(s_shortcutHandler == 0)
    {
        s_shortcutHandler = ShortcutHandler::instance();
    }

    setScene(new QGraphicsScene(this));

    setAcceptDrops(false);
    setDragMode(QGraphicsView::ScrollHandDrag);

    connect(verticalScrollBar(), SIGNAL(valueChanged(int)), SLOT(on_verticalScrollBar_valueChanged(int)));

    m_thumbnailsScene = new QGraphicsScene(this);

    m_outlineModel = new QStandardItemModel(this);
    m_propertiesModel = new QStandardItemModel(this);

    // highlight

    m_highlight = new QGraphicsRectItem();
    m_highlight->setGraphicsEffect(new GraphicsCompositionModeEffect(QPainter::CompositionMode_Multiply, this));

    m_highlight->setVisible(false);
    scene()->addItem(m_highlight);

    // search

    m_searchTask = new SearchTask(this);

    connect(m_searchTask, SIGNAL(finished()), SIGNAL(searchFinished()));
    connect(m_searchTask, SIGNAL(progressChanged(int)), SIGNAL(searchProgressChanged(int)));

    connect(m_searchTask, SIGNAL(resultsReady(int,QList<QRectF>)), SLOT(on_searchTask_resultsReady(int,QList<QRectF>)));

    // auto-refresh

    m_autoRefreshWatcher = new QFileSystemWatcher(this);

    m_autoRefreshTimer = new QTimer(this);
    m_autoRefreshTimer->setInterval(s_settings->documentView().autoRefreshTimeout());
    m_autoRefreshTimer->setSingleShot(true);

    connect(m_autoRefreshWatcher, SIGNAL(fileChanged(QString)), m_autoRefreshTimer, SLOT(start()));

    connect(m_autoRefreshTimer, SIGNAL(timeout()), this, SLOT(on_autoRefresh_timeout()));

    // prefetch

    m_prefetchTimer = new QTimer(this);
    m_prefetchTimer->setInterval(s_settings->documentView().prefetchTimeout());
    m_prefetchTimer->setSingleShot(true);

    connect(this, SIGNAL(currentPageChanged(int)), m_prefetchTimer, SLOT(start()));
    connect(this, SIGNAL(layoutModeChanged(LayoutMode)), m_prefetchTimer, SLOT(start()));
    connect(this, SIGNAL(scaleModeChanged(ScaleMode)), m_prefetchTimer, SLOT(start()));
    connect(this, SIGNAL(scaleFactorChanged(qreal)), m_prefetchTimer, SLOT(start()));
    connect(this, SIGNAL(rotationChanged(Rotation)), m_prefetchTimer, SLOT(start()));

    connect(m_prefetchTimer, SIGNAL(timeout()), SLOT(on_prefetch_timeout()));

    // settings

    m_continuousMode = s_settings->documentView().continuousMode();
    m_layout.reset(DocumentLayout::fromLayoutMode(s_settings->documentView().layoutMode()));
    m_rightToLeftMode = s_settings->documentView().rightToLeftMode();

    m_scaleMode = s_settings->documentView().scaleMode();
    m_scaleFactor = s_settings->documentView().scaleFactor();
    m_rotation = s_settings->documentView().rotation();

    m_invertColors = s_settings->documentView().invertColors();
    m_highlightAll = s_settings->documentView().highlightAll();
}

DocumentView::~DocumentView()
{
    m_searchTask->cancel();
    m_searchTask->wait();

    qDeleteAll(m_pageItems);
    qDeleteAll(m_thumbnailItems);

    qDeleteAll(m_pages);
    delete m_document;
}

QStringList DocumentView::openFilter()
{
    QStringList openFilter;
    QStringList supportedFormats;

#if defined(WITH_PDF) || defined(WITH_FITZ)

    openFilter.append("Portable document format (*.pdf)");
    supportedFormats.append("*.pdf");

#endif // WITH_PDF // WITH_FITZ

#ifdef WITH_PS

    openFilter.append("PostScript (*.ps)");
    openFilter.append("Encapsulated PostScript (*.eps)");
    supportedFormats.append("*.ps *.eps");

#endif // WITH_PS

#ifdef WITH_DJVU

    openFilter.append("DjVu (*.djvu *.djv)");
    supportedFormats.append("*.djvu");
    supportedFormats.append("*.djv");

#endif // WITH_DJVU

    openFilter.prepend(tr("Supported formats (%1)").arg(supportedFormats.join(" ")));

    return openFilter;
}

QStringList DocumentView::saveFilter() const
{
    return m_document->saveFilter();
}

bool DocumentView::canSave() const
{
    return m_document->canSave();
}

void DocumentView::setContinuousMode(bool continuousMode)
{
    if(m_continuousMode != continuousMode)
    {
        m_continuousMode = continuousMode;

        qreal left = 0.0, top = 0.0;
        saveLeftAndTop(left, top);

        adjustScrollBarPolicy();

        prepareView(left, top);

        emit continuousModeChanged(m_continuousMode);

        s_settings->documentView().setContinuousMode(m_continuousMode);
    }
}

LayoutMode DocumentView::layoutMode() const
{
    return m_layout->layoutMode();
}

void DocumentView::setLayoutMode(LayoutMode layoutMode)
{
    if(m_layout->layoutMode() != layoutMode && layoutMode >= 0 && layoutMode < NumberOfLayoutModes)
    {
        m_layout.reset(DocumentLayout::fromLayoutMode(layoutMode));

        if(m_currentPage != m_layout->currentPage(m_currentPage))
        {
            m_currentPage = m_layout->currentPage(m_currentPage);

            emit currentPageChanged(m_currentPage);
        }

        prepareScene();
        prepareView();

        emit layoutModeChanged(layoutMode);

        s_settings->documentView().setLayoutMode(layoutMode);
    }
}

void DocumentView::setRightToLeftMode(bool rightToLeftMode)
{
    if(m_rightToLeftMode != rightToLeftMode)
    {
        m_rightToLeftMode = rightToLeftMode;

        prepareScene();
        prepareView();

        emit rightToLeftModeChanged(m_rightToLeftMode);

        s_settings->documentView().setRightToLeftMode(m_rightToLeftMode);
    }
}

void DocumentView::setScaleMode(ScaleMode scaleMode)
{
    if(m_scaleMode != scaleMode && scaleMode >= 0 && scaleMode < NumberOfScaleModes)
    {
        m_scaleMode = scaleMode;

        qreal left = 0.0, top = 0.0;
        saveLeftAndTop(left, top);

        adjustScrollBarPolicy();

        prepareScene();
        prepareView(left, top);

        emit scaleModeChanged(m_scaleMode);

        s_settings->documentView().setScaleMode(m_scaleMode);
    }
}

void DocumentView::setScaleFactor(qreal scaleFactor)
{
    if(!qFuzzyCompare(m_scaleFactor, scaleFactor)
            && scaleFactor >= s_settings->documentView().minimumScaleFactor()
            && scaleFactor <= s_settings->documentView().maximumScaleFactor())
    {
        m_scaleFactor = scaleFactor;

        if(m_scaleMode == ScaleFactorMode)
        {
            qreal left = 0.0, top = 0.0;
            saveLeftAndTop(left, top);

            prepareScene();
            prepareView(left, top);
        }

        emit scaleFactorChanged(m_scaleFactor);

        s_settings->documentView().setScaleFactor(m_scaleFactor);
    }
}

void DocumentView::setRotation(Rotation rotation)
{
    if(m_rotation != rotation && rotation >= 0 && rotation < NumberOfRotations)
    {
        m_rotation = rotation;

        prepareScene();
        prepareView();

        emit rotationChanged(m_rotation);

        s_settings->documentView().setRotation(rotation);
    }
}

void DocumentView::setInvertColors(bool invertColors)
{
    if(m_invertColors != invertColors)
    {
        m_invertColors = invertColors;

        foreach(PageItem* page, m_pageItems)
        {
            page->setInvertColors(m_invertColors);
        }

        foreach(PageItem* page, m_thumbnailItems)
        {
            page->setInvertColors(m_invertColors);
        }

        prepareBackground();

        emit invertColorsChanged(m_invertColors);

        s_settings->documentView().setInvertColors(m_invertColors);
    }
}

void DocumentView::setHighlightAll(bool highlightAll)
{
    if(m_highlightAll != highlightAll)
    {
        m_highlightAll = highlightAll;

        for(int index = 0; index < m_pageItems.count(); ++index)
        {
            m_pageItems.at(index)->setHighlights(m_highlightAll ? m_results.values(index) : QList< QRectF >());
        }

        for(int index = 0; index < m_thumbnailItems.count(); ++index)
        {
            m_thumbnailItems.at(index)->setHighlights(m_highlightAll ? m_results.values(index) : QList< QRectF >());
        }

        emit highlightAllChanged(m_highlightAll);

        s_settings->documentView().setHighlightAll(highlightAll);
    }
}

void DocumentView::setRubberBandMode(RubberBandMode rubberBandMode)
{
    if(m_rubberBandMode != rubberBandMode && rubberBandMode >= 0 && rubberBandMode < NumberOfRubberBandModes)
    {
        m_rubberBandMode = rubberBandMode;

        foreach(PageItem* page, m_pageItems)
        {
            page->setRubberBandMode(m_rubberBandMode);
        }

        emit rubberBandModeChanged(m_rubberBandMode);
    }
}

bool DocumentView::searchWasCanceled() const
{
    return m_searchTask->wasCanceled();
}

int DocumentView::searchProgress() const
{
    return m_searchTask->progress();
}

void DocumentView::setThumbnailsOrientation(Qt::Orientation thumbnailsOrientation)
{
    if(m_thumbnailsOrientation != thumbnailsOrientation)
    {
        m_thumbnailsOrientation = thumbnailsOrientation;

        prepareThumbnailsScene();
    }
}

QStandardItemModel* DocumentView::fontsModel() const
{
    QStandardItemModel* fontsModel = new QStandardItemModel();

    m_document->loadFonts(fontsModel);

    return fontsModel;
}

void DocumentView::show()
{
    QGraphicsView::show();

    prepareView();
}

bool DocumentView::open(const QString& filePath)
{
    Model::Document* document = PluginHandler::instance()->loadDocument(filePath);

    if(document != 0)
    {
        if(!checkDocument(filePath, document))
        {
            delete document;
            return false;
        }

        m_fileInfo.setFile(filePath);
        m_wasModified = false;

        m_currentPage = 1;

        m_past.clear();
        m_future.clear();

        prepareDocument(document);

        loadDocumentDefaults();

        prepareScene();
        prepareView();

        prepareThumbnailsScene();

        emit documentChanged();

        emit numberOfPagesChanged(m_pages.count());
        emit currentPageChanged(m_currentPage);

        emit canJumpChanged(false, false);
    }

    return document != 0;
}

bool DocumentView::refresh()
{
    Model::Document* document = PluginHandler::instance()->loadDocument(m_fileInfo.filePath());

    if(document != 0)
    {
        if(!checkDocument(m_fileInfo.filePath(), document))
        {
            delete document;
            return false;
        }

        qreal left = 0.0, top = 0.0;
        saveLeftAndTop(left, top);

        m_wasModified = false;

        m_currentPage = qMin(m_currentPage, document->numberOfPages());

        prepareDocument(document);

        prepareScene();
        prepareView(left, top);

        prepareThumbnailsScene();

        emit documentChanged();

        emit numberOfPagesChanged(m_pages.count());
        emit currentPageChanged(m_currentPage);
    }

    return document != 0;
}

bool DocumentView::save(const QString& filePath, bool withChanges)
{
    QTemporaryFile temporaryFile;
    QFile file(filePath);

    if(temporaryFile.open())
    {
        temporaryFile.close();

        if(m_document->save(temporaryFile.fileName(), withChanges))
        {
            if(temporaryFile.open())
            {
                if(file.open(QIODevice::WriteOnly | QIODevice::Truncate))
                {
                    const qint64 maxSize = 4096;
                    qint64 size = -1;

                    char* data = new char[maxSize];

                    while(!temporaryFile.atEnd())
                    {
                        size = temporaryFile.read(data, maxSize);

                        if(size == -1 || file.write(data, size) == -1)
                        {
                            delete[] data;
                            return false;
                        }
                    }

                    if(withChanges)
                    {
                        m_wasModified = false;
                    }

                    delete[] data;
                    return true;
                }
            }
        }
    }

    return false;
}

bool DocumentView::print(QPrinter* printer, const PrintOptions& printOptions)
{
    const int fromPage = printer->fromPage() != 0 ? printer->fromPage() : 1;
    const int toPage = printer->toPage() != 0 ? printer->toPage() : m_pages.count();

#ifdef WITH_CUPS

    if(m_document->canBePrintedUsingCUPS())
    {
        return printUsingCUPS(printer, printOptions, fromPage, toPage);
    }

#endif // WITH_CUPS

    return printUsingQt(printer, printOptions, fromPage, toPage);
}

void DocumentView::previousPage()
{
    jumpToPage(m_layout->previousPage(m_currentPage));
}

void DocumentView::nextPage()
{
    jumpToPage(m_layout->nextPage(m_currentPage, m_pages.count()));
}

void DocumentView::firstPage()
{
    jumpToPage(1);
}

void DocumentView::lastPage()
{
    jumpToPage(m_pages.count());
}

void DocumentView::jumpToPage(int page, bool trackChange, qreal changeLeft, qreal changeTop)
{
    if(page >= 1 && page <= m_pages.count())
    {
        qreal left = 0.0, top = 0.0;
        saveLeftAndTop(left, top);

        if(m_currentPage != m_layout->currentPage(page) || qAbs(left - changeLeft) > 0.01 || qAbs(top - changeTop) > 0.01)
        {
            if(trackChange)
            {
                m_past.append(Position(m_currentPage, left, top));
                m_future.clear();

                emit canJumpChanged(true, false);
            }

            m_currentPage = m_layout->currentPage(page);

            prepareView(changeLeft, changeTop, page);

            emit currentPageChanged(m_currentPage, trackChange);
        }
    }
}

bool DocumentView::canJumpBackward() const
{
    return !m_past.isEmpty();
}

void DocumentView::jumpBackward()
{
    if(!m_past.isEmpty())
    {
        qreal left = 0.0, top = 0.0;
        saveLeftAndTop(left, top);

        m_future.prepend(Position(m_currentPage, left, top));

        const Position pos = m_past.takeLast();
        jumpToPage(pos.page, false, pos.left, pos.top);

        emit canJumpChanged(!m_past.isEmpty(), !m_future.isEmpty());
    }
}

bool DocumentView::canJumpForward() const
{
    return !m_future.isEmpty();
}

void DocumentView::jumpForward()
{
    if(!m_future.isEmpty())
    {
        qreal left = 0.0, top = 0.0;
        saveLeftAndTop(left, top);

        m_past.append(Position(m_currentPage, left, top));

        const Position pos = m_future.takeFirst();
        jumpToPage(pos.page, false, pos.left, pos.top);

        emit canJumpChanged(!m_past.isEmpty(), !m_future.isEmpty());
    }
}

void DocumentView::temporaryHighlight(int page, const QRectF& highlight)
{
    if(page >= 1 && page <= m_pages.count() && !highlight.isNull())
    {
        prepareHighlight(page - 1, highlight);

        QTimer::singleShot(s_settings->documentView().highlightDuration(), this, SLOT(on_temporaryHighlight_timeout()));
    }
}

void DocumentView::startSearch(const QString& text, bool matchCase)
{
    cancelSearch();

    m_searchTask->start(m_pages, text, matchCase, m_currentPage);
}

void DocumentView::cancelSearch()
{
    m_searchTask->cancel();
    m_searchTask->wait();

    m_results.clear();
    m_currentResult = m_results.end();

    foreach(PageItem* page, m_pageItems)
    {
        page->setHighlights(QList< QRectF >());
    }

    foreach(PageItem* page, m_thumbnailItems)
    {
        page->setHighlights(QList< QRectF >());
    }

    prepareThumbnailsScene();

    m_highlight->setVisible(false);
}

void DocumentView::findPrevious()
{
    if(m_currentResult != m_results.end())
    {
        if(m_layout->leftIndex(m_currentResult.key()) == m_currentPage - 1)
        {
            m_currentResult = previousResult(m_currentResult);
        }
        else
        {
            m_currentResult = previousResult(m_results.upperBound(m_currentPage - 1));
        }
    }
    else
    {
        m_currentResult = previousResult(m_results.upperBound(m_currentPage - 1));
    }

    if(m_currentResult == m_results.end())
    {
        m_currentResult = previousResult(m_results.end());
    }

    if(m_currentResult != m_results.end())
    {
        jumpToPage(m_currentResult.key() + 1);

        prepareHighlight(m_currentResult.key(), m_currentResult.value());
    }
    else
    {
        m_highlight->setVisible(false);
    }
}

void DocumentView::findNext()
{
    if(m_currentResult != m_results.end())
    {
        if(m_layout->leftIndex(m_currentResult.key()) == m_currentPage - 1)
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

    if(m_currentResult != m_results.end())
    {
        jumpToPage(m_currentResult.key() + 1);

        prepareHighlight(m_currentResult.key(), m_currentResult.value());
    }
    else
    {
        m_highlight->setVisible(false);
    }
}

void DocumentView::zoomIn()
{
    if(scaleMode() != ScaleFactorMode)
    {
        setScaleFactor(qMin(m_pageItems.at(m_currentPage - 1)->scaleFactor() * s_settings->documentView().zoomFactor(),
                            s_settings->documentView().maximumScaleFactor()));

        setScaleMode(ScaleFactorMode);
    }
    else
    {
        setScaleFactor(qMin(m_scaleFactor * s_settings->documentView().zoomFactor(),
                            s_settings->documentView().maximumScaleFactor()));
    }
}

void DocumentView::zoomOut()
{
    if(scaleMode() != ScaleFactorMode)
    {
        setScaleFactor(qMax(m_pageItems.at(m_currentPage - 1)->scaleFactor() / s_settings->documentView().zoomFactor(),
                            s_settings->documentView().minimumScaleFactor()));

        setScaleMode(ScaleFactorMode);
    }
    else
    {
        setScaleFactor(qMax(m_scaleFactor / s_settings->documentView().zoomFactor(),
                            s_settings->documentView().minimumScaleFactor()));
    }
}

void DocumentView::originalSize()
{
    setScaleFactor(1.0);
    setScaleMode(ScaleFactorMode);
}

void DocumentView::rotateLeft()
{
    switch(m_rotation)
    {
    default:
    case RotateBy0:
        setRotation(RotateBy270);
        break;
    case RotateBy90:
        setRotation(RotateBy0);
        break;
    case RotateBy180:
        setRotation(RotateBy90);
        break;
    case RotateBy270:
        setRotation(RotateBy180);
        break;
    }
}

void DocumentView::rotateRight()
{
    switch(m_rotation)
    {
    default:
    case RotateBy0:
        setRotation(RotateBy90);
        break;
    case RotateBy90:
        setRotation(RotateBy180);
        break;
    case RotateBy180:
        setRotation(RotateBy270);
        break;
    case RotateBy270:
        setRotation(RotateBy0);
        break;
    }
}

void DocumentView::startPresentation()
{
    const int screen = s_settings->presentationView().screen();

    PresentationView* presentationView = new PresentationView(m_pages);

    presentationView->setGeometry(QApplication::desktop()->screenGeometry(screen));

    presentationView->show();
    presentationView->setAttribute(Qt::WA_DeleteOnClose);

    connect(this, SIGNAL(destroyed()), presentationView, SLOT(close()));
    connect(this, SIGNAL(documentChanged()), presentationView, SLOT(close()));

    presentationView->setRotation(rotation());
    presentationView->setInvertColors(invertColors());

    presentationView->jumpToPage(currentPage(), false);

    if(s_settings->presentationView().synchronize())
    {
        connect(this, SIGNAL(currentPageChanged(int,bool)), presentationView, SLOT(jumpToPage(int,bool)));
        connect(presentationView, SIGNAL(currentPageChanged(int,bool)), this, SLOT(jumpToPage(int,bool)));
    }
}

void DocumentView::on_verticalScrollBar_valueChanged(int value)
{
    if(!m_continuousMode)
    {
        return;
    }

    const QRectF visibleRect = mapToScene(viewport()->rect()).boundingRect();

    foreach(PageItem* page, m_pageItems)
    {
        if(!page->boundingRect().translated(page->pos()).intersects(visibleRect))
        {
            page->cancelRender();
        }
    }

    const QMap< qreal, int >::iterator lowerBound = m_heightToIndex.lowerBound(-value);

    if(lowerBound != m_heightToIndex.end())
    {
        const int page = lowerBound.value() + 1;

        if(m_currentPage != page)
        {
            m_currentPage = page;

            emit currentPageChanged(m_currentPage);

            if(s_settings->documentView().highlightCurrentThumbnail())
            {
                for(int index = 0; index < m_thumbnailItems.count(); ++index)
                {
                    m_thumbnailItems.at(index)->setCurrent(index == m_currentPage - 1);
                }
            }
        }
    }
}

void DocumentView::on_autoRefresh_timeout()
{
    if(m_fileInfo.exists())
    {
        refresh();
    }
    else
    {
        m_wasModified = true;

        emit documentModified();
    }
}

void DocumentView::on_prefetch_timeout()
{
    const QPair< int, int > prefetchRange = m_layout->prefetchRange(m_currentPage, m_pages.count());

    const int maxCost = prefetchRange.second - prefetchRange.first + 1;
    int cost = 0;

    for(int index = m_currentPage - 1; index <= prefetchRange.second - 1; ++index)
    {
        cost += m_pageItems.at(index)->startRender(true);

        if(cost >= maxCost)
        {
            return;
        }
    }

    for(int index = m_currentPage - 1; index >= prefetchRange.first - 1; --index)
    {
        cost += m_pageItems.at(index)->startRender(true);

        if(cost >= maxCost)
        {
            return;
        }
    }
}

void DocumentView::on_temporaryHighlight_timeout()
{
    m_highlight->setVisible(false);
}

void DocumentView::on_searchTask_resultsReady(int index, QList< QRectF > results)
{
    if(m_searchTask->wasCanceled())
    {
        return;
    }

    while(!results.isEmpty())
    {
        m_results.insertMulti(index, results.takeLast());
    }

    if(m_highlightAll)
    {
        m_pageItems.at(index)->setHighlights(m_results.values(index));
        m_thumbnailItems.at(index)->setHighlights(m_results.values(index));
    }

    if(s_settings->documentView().limitThumbnailsToResults())
    {
        prepareThumbnailsScene();
    }

    if(m_results.contains(index) && m_currentResult == m_results.end())
    {
        findNext();
    }
}

void DocumentView::on_pages_linkClicked(bool newTab, int page, qreal left, qreal top)
{
    page = qMax(page, 1);
    page = qMin(page, m_pages.count());

    left = left >= 0.0 ? left : 0.0;
    left = left <= 1.0 ? left : 1.0;

    top = top >= 0.0 ? top : 0.0;
    top = top <= 1.0 ? top : 1.0;

    if(newTab)
    {
        emit linkClicked(page);
    }
    else
    {
        jumpToPage(page, true, left, top);
    }
}

void DocumentView::on_pages_linkClicked(const QString& url)
{
    if(s_settings->documentView().openUrl())
    {
        QUrl resolvedUrl(url);

        if(resolvedUrl.isRelative() && QFileInfo(url).isRelative())
        {
            resolvedUrl.setPath(m_fileInfo.dir().filePath(url));
        }

        QDesktopServices::openUrl(resolvedUrl);
    }
    else
    {
        QMessageBox::information(this, tr("Information"), tr("Opening URL is disabled in the settings."));
    }
}

void DocumentView::on_pages_linkClicked(const QString& fileName, int page)
{
    const QString filePath = QFileInfo(fileName).isAbsolute() ? fileName : m_fileInfo.dir().filePath(fileName);

    emit linkClicked(filePath, page);
}

void DocumentView::on_pages_rubberBandFinished()
{
    setRubberBandMode(ModifiersMode);
}

void DocumentView::on_pages_editSourceRequested(int page, const QPointF& pos)
{
#ifdef WITH_SYNCTEX

    if(s_settings->documentView().sourceEditor().isEmpty())
    {
        return;
    }

    synctex_scanner_t scanner = synctex_scanner_new_with_output_file(m_fileInfo.absoluteFilePath().toLocal8Bit(), 0, 1);

    if(scanner != 0)
    {
        if(synctex_edit_query(scanner, page, pos.x(), pos.y()) > 0)
        {
            for(synctex_node_t node = synctex_next_result(scanner); node != 0; node = synctex_next_result(scanner))
            {
                QString sourceName = QString::fromLocal8Bit(synctex_scanner_get_name(scanner, synctex_node_tag(node)));
                int sourceLine = synctex_node_line(node);
                int sourceColumn = synctex_node_column(node);

                sourceLine = sourceLine >= 0 ? sourceLine : 0;
                sourceColumn = sourceColumn >= 0 ? sourceColumn : 0;

                QProcess::startDetached(s_settings->documentView().sourceEditor().arg(m_fileInfo.dir().absoluteFilePath(sourceName), QString::number(sourceLine), QString::number(sourceColumn)));

                break;
            }
        }

        synctex_scanner_free(scanner);
    }
    else
    {
        QMessageBox::warning(this, tr("Warning"), tr("SyncTeX data for '%1' could not be found.").arg(m_fileInfo.absoluteFilePath()));
    }

#else

    Q_UNUSED(page);
    Q_UNUSED(pos);

#endif // WITH_SYNCTEX
}

void DocumentView::on_pages_zoomToSelectionRequested(int page, const QRectF& rect)
{
    const qreal visibleWidth = m_layout->visibleWidth(viewport()->width());
    const qreal visibleHeight = m_layout->visibleHeight(viewport()->height());

    const qreal unscaledWidth = unscaledPageWidth(m_pageItems.at(page - 1));
    const qreal unscaledHeight = unscaledPageHeight(m_pageItems.at(page - 1));

    setScaleFactor(qMin(qMin(visibleWidth / unscaledWidth / rect.width(),
                             visibleHeight / unscaledHeight / rect.height()),
                        Defaults::DocumentView::maximumScaleFactor()));

    setScaleMode(ScaleFactorMode);

    jumpToPage(page, false, rect.left(), rect.top());
}

void DocumentView::on_pages_wasModified()
{
    m_wasModified = true;

    emit documentModified();
}

void DocumentView::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);

    if(m_scaleMode != ScaleFactorMode)
    {
        qreal left = 0.0, top = 0.0;
        saveLeftAndTop(left, top);

        prepareScene();
        prepareView(left, top);
    }
}

void DocumentView::keyPressEvent(QKeyEvent* event)
{
    const QKeySequence keySequence(event->modifiers() + event->key());

    int maskedKey = -1;

    if(s_shortcutHandler->matchesSkipBackward(keySequence))
    {
        maskedKey = Qt::Key_PageUp;
    }
    else if(s_shortcutHandler->matchesSkipForward(keySequence))
    {
        maskedKey = Qt::Key_PageDown;
    }
    else if(s_shortcutHandler->matchesMoveUp(keySequence))
    {
        maskedKey = Qt::Key_Up;
    }
    else if(s_shortcutHandler->matchesMoveDown(keySequence))
    {
        maskedKey = Qt::Key_Down;
    }
    else if(s_shortcutHandler->matchesMoveLeft(keySequence))
    {
        maskedKey = Qt::Key_Left;
    }
    else if(s_shortcutHandler->matchesMoveRight(keySequence))
    {
        maskedKey = Qt::Key_Right;
    }
    else if(event->key() == Qt::Key_PageUp || event->key() == Qt::Key_PageDown ||
            event->key() == Qt::Key_Up || event->key() == Qt::Key_Down ||
            event->key() == Qt::Key_Left || event->key() == Qt::Key_Right)
    {
        event->ignore();
        return;
    }

    if(maskedKey == -1)
    {
        QGraphicsView::keyPressEvent(event);
    }
    else
    {
        foreach(const PageItem* page, m_pageItems)
        {
            if(page->showsAnnotationOverlay() || page->showsFormFieldOverlay())
            {
                return;
            }
        }

        if(!m_continuousMode)
        {
            if(maskedKey == Qt::Key_PageUp && verticalScrollBar()->value() == verticalScrollBar()->minimum() && m_currentPage != 1)
            {
                previousPage();

                verticalScrollBar()->setValue(verticalScrollBar()->maximum());

                event->accept();
                return;
            }
            else if(maskedKey == Qt::Key_PageDown && verticalScrollBar()->value() == verticalScrollBar()->maximum() && m_currentPage != m_layout->currentPage(m_pages.count()))
            {
                nextPage();

                verticalScrollBar()->setValue(verticalScrollBar()->minimum());

                event->accept();
                return;
            }
        }

        if((maskedKey == Qt::Key_Up && verticalScrollBar()->minimum() == verticalScrollBar()->maximum()) ||
           (maskedKey == Qt::Key_Left && !horizontalScrollBar()->isVisible()))
        {
            previousPage();

            event->accept();
            return;
        }
        else if((maskedKey == Qt::Key_Down && verticalScrollBar()->minimum() == verticalScrollBar()->maximum()) ||
                (maskedKey == Qt::Key_Right && !horizontalScrollBar()->isVisible()))
        {
            nextPage();

            event->accept();
            return;
        }

        QKeyEvent keyEvent(event->type(), maskedKey, Qt::NoModifier, event->text(), event->isAutoRepeat(), event->count());
        QGraphicsView::keyPressEvent(&keyEvent);
    }
}

void DocumentView::wheelEvent(QWheelEvent* event)
{
    if(event->modifiers() == s_settings->documentView().zoomModifiers())
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
    else if(event->modifiers() == s_settings->documentView().rotateModifiers())
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
    else if(event->modifiers() == s_settings->documentView().scrollModifiers())
    {
        QWheelEvent wheelEvent(event->pos(), event->delta(), event->buttons(), Qt::AltModifier, Qt::Horizontal);
        QGraphicsView::wheelEvent(&wheelEvent);

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
            else if(event->delta() < 0 && verticalScrollBar()->value() == verticalScrollBar()->maximum() && m_currentPage != m_layout->currentPage(m_pages.count()))
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

        emit customContextMenuRequested(event->pos());
    }
}

#ifdef WITH_CUPS

bool DocumentView::printUsingCUPS(QPrinter* printer, const PrintOptions& printOptions, int fromPage, int toPage)
{
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

        QStringList cupsOptions = printer->printEngine()->property(QPrintEngine::PrintEnginePropertyKey(0xfe00)).toStringList();

        for(int index = 0; index < cupsOptions.count() - 1; index += 2)
        {
            num_options = cupsAddOption(cupsOptions.at(index).toLocal8Bit(), cupsOptions.at(index + 1).toLocal8Bit(), num_options, &options);
        }

#if QT_VERSION >= QT_VERSION_CHECK(4,7,0)

        num_options = cupsAddOption("copies", QString::number(printer->copyCount()).toLocal8Bit(), num_options, &options);

#endif // QT_VERSION

        num_options = cupsAddOption("Collate", printer->collateCopies() ? "true" : "false", num_options, &options);

        switch(printer->pageOrder())
        {
        case QPrinter::FirstPageFirst:
            num_options = cupsAddOption("outputorder", "normal", num_options, &options);
            break;
        case QPrinter::LastPageFirst:
            num_options = cupsAddOption("outputorder", "reverse", num_options, &options);
            break;
        }

        num_options = cupsAddOption("fit-to-page", printOptions.fitToPage ? "true" : "false", num_options, &options);

        switch(printer->orientation())
        {
        case QPrinter::Portrait:
            num_options = cupsAddOption("landscape", "false", num_options, &options);
            break;
        case QPrinter::Landscape:
            num_options = cupsAddOption("landscape", "true", num_options, &options);
            break;
        }

        switch(printer->colorMode())
        {
        case QPrinter::Color:
            num_options = addCMYKorRGBColorModel(dest, num_options, &options);
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

        int numberUp = 1;

#if QT_VERSION < QT_VERSION_CHECK(5,2,0)

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

#else // QT_VERSION

        {
            bool ok = false;
            int value = QString::fromLocal8Bit(cupsGetOption("number-up", num_options, options)).toInt(&ok);

            numberUp = ok ? value : 1;
        }

#endif // QT_VERSION

        fromPage = (fromPage - 1) / numberUp + 1;
        toPage = (toPage - 1) / numberUp + 1;

        if(printOptions.pageRanges.isEmpty())
        {
            num_options = cupsAddOption("page-ranges", QString("%1-%2").arg(fromPage).arg(toPage).toLocal8Bit(), num_options, &options);
        }
        else
        {
            num_options = cupsAddOption("page-ranges", printOptions.pageRanges.toLocal8Bit(), num_options, &options);
        }

        QTemporaryFile temporaryFile;

        if(temporaryFile.open())
        {
            temporaryFile.close();

            if(m_document->save(temporaryFile.fileName(), true))
            {
                jobId = cupsPrintFile(dest->name, temporaryFile.fileName().toLocal8Bit(), m_fileInfo.completeBaseName().toLocal8Bit(), num_options, options);

                if(jobId < 1)
                {
                    qWarning() << cupsLastErrorString();
                }
            }
        }
    }
    else
    {
        qWarning() << cupsLastErrorString();
    }

    cupsFreeDests(num_dests, dests);
    cupsFreeOptions(num_options, options);

    return jobId >= 1;
}

#endif // WITH_CUPS

bool DocumentView::printUsingQt(QPrinter* printer, const PrintOptions& printOptions, int fromPage, int toPage)
{
    QScopedPointer< QProgressDialog > progressDialog(new QProgressDialog(this));
    progressDialog->setLabelText(tr("Printing '%1'...").arg(m_fileInfo.completeBaseName()));
    progressDialog->setRange(fromPage - 1, toPage);

    QPainter painter(printer);

    for(int index = fromPage - 1; index <= toPage - 1; ++index)
    {
        progressDialog->setValue(index);

        QApplication::processEvents();

        painter.save();

        const Model::Page* page = m_pages.at(index);

        if(printOptions.fitToPage)
        {
            const qreal pageWidth = printer->physicalDpiX() / 72.0 * page->size().width();
            const qreal pageHeight = printer->physicalDpiY() / 72.0 * page->size().width();

            const qreal scaleFactor = qMin(printer->width() / pageWidth, printer->height() / pageHeight);

            painter.setTransform(QTransform::fromScale(scaleFactor, scaleFactor));
        }
        else
        {
            const qreal scaleFactorX = static_cast< qreal >(printer->logicalDpiX()) / static_cast< qreal >(printer->physicalDpiX());
            const qreal scaleFactorY = static_cast< qreal >(printer->logicalDpiY()) / static_cast< qreal >(printer->physicalDpiY());

            painter.setTransform(QTransform::fromScale(scaleFactorX, scaleFactorY));
        }

        painter.drawImage(QPointF(), page->render(printer->physicalDpiX(), printer->physicalDpiY()));

        painter.restore();

        if(index < toPage - 1)
        {
            printer->newPage();
        }

        QApplication::processEvents();

        if(progressDialog->wasCanceled())
        {
            printer->abort();

            return false;
        }
    }

    return true;
}

void DocumentView::saveLeftAndTop(qreal& left, qreal& top) const
{
    const PageItem* page = m_pageItems.at(m_currentPage - 1);

    const QRectF boundingRect = page->boundingRect().translated(page->pos());
    const QPointF topLeft = mapToScene(viewport()->rect().topLeft());

    left = (topLeft.x() - boundingRect.x()) / boundingRect.width();
    top = (topLeft.y() - boundingRect.y()) / boundingRect.height();

    left = left >= 0.0 ? left : 0.0;
    top = top >= 0.0 ? top : 0.0;
}

bool DocumentView::checkDocument(const QString& filePath, Model::Document* document)
{
    if(document->isLocked())
    {
        QString password = QInputDialog::getText(this, tr("Unlock %1").arg(QFileInfo(filePath).completeBaseName()), tr("Password:"), QLineEdit::Password);

        if(document->unlock(password))
        {
            return false;
        }
    }

    if(document->numberOfPages() == 0)
    {
        qWarning() << "No pages were found in document at" << filePath;

        return false;
    }

    return true;
}

void DocumentView::loadFallbackOutline()
{
    for(int page = 1; page <= m_pages.count(); ++page)
    {
        QStandardItem* item = new QStandardItem(tr("Page %1").arg(page));
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

        item->setData(page, Qt::UserRole + 1);

        QStandardItem* pageItem = item->clone();
        pageItem->setText(QString::number(page));
        pageItem->setTextAlignment(Qt::AlignRight);

        m_outlineModel->appendRow(QList< QStandardItem* >() << item << pageItem);
    }
}

void DocumentView::loadDocumentDefaults()
{
    if(m_document->wantsContinuousMode())
    {
        m_continuousMode = true;
    }

    if(m_document->wantsSinglePageMode())
    {
        m_layout.reset(new SinglePageLayout);
    }
    else if(m_document->wantsTwoPagesMode())
    {
        m_layout.reset(new TwoPagesLayout);
    }
    else if(m_document->wantsTwoPagesWithCoverPageMode())
    {
        m_layout.reset(new TwoPagesWithCoverPageLayout);
    }

    if(m_document->wantsRightToLeftMode())
    {
        m_rightToLeftMode = true;
    }
}

void DocumentView::adjustScrollBarPolicy()
{
    switch(m_scaleMode)
    {
    default:
    case ScaleFactorMode:
        setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        break;
    case FitToPageWidthMode:
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        break;
    case FitToPageSizeMode:
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(m_continuousMode ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
        break;
    }
}

void DocumentView::prepareDocument(Model::Document* document)
{
    m_prefetchTimer->blockSignals(true);
    m_prefetchTimer->stop();

    cancelSearch();

    qDeleteAll(m_pageItems);
    qDeleteAll(m_thumbnailItems);

    qDeleteAll(m_pages);

    delete m_document;
    m_document = document;

    const int numberOfPages = m_document->numberOfPages();

    m_pages.clear();
    m_pages.reserve(numberOfPages);

    for(int index = 0; index < numberOfPages; ++index)
    {
        m_pages.append(m_document->page(index));
    }

    if(!m_autoRefreshWatcher->files().isEmpty())
    {
        m_autoRefreshWatcher->removePaths(m_autoRefreshWatcher->files());
    }

    if(s_settings->documentView().autoRefresh())
    {
        m_autoRefreshWatcher->addPath(m_fileInfo.filePath());
    }

    m_document->setPaperColor(s_settings->pageItem().paperColor());

    preparePages();
    prepareThumbnails();
    prepareBackground();

    m_document->loadOutline(m_outlineModel);
    m_document->loadProperties(m_propertiesModel);

    if(m_outlineModel->rowCount() == 0)
    {
        loadFallbackOutline();
    }

    if(s_settings->documentView().prefetch())
    {
        m_prefetchTimer->blockSignals(false);
        m_prefetchTimer->start();
    }
}

void DocumentView::preparePages()
{
    m_pageItems.clear();
    m_pageItems.reserve(m_pages.count());

    for(int index = 0; index < m_pages.count(); ++index)
    {
        PageItem* page = new PageItem(m_pages.at(index), index);

        page->setInvertColors(m_invertColors);
        page->setRubberBandMode(m_rubberBandMode);

        scene()->addItem(page);
        m_pageItems.append(page);

        connect(page, SIGNAL(linkClicked(bool,int,qreal,qreal)), SLOT(on_pages_linkClicked(bool,int,qreal,qreal)));
        connect(page, SIGNAL(linkClicked(QString)), SLOT(on_pages_linkClicked(QString)));
        connect(page, SIGNAL(linkClicked(QString,int)), SLOT(on_pages_linkClicked(QString,int)));

        connect(page, SIGNAL(rubberBandFinished()), SLOT(on_pages_rubberBandFinished()));

        connect(page, SIGNAL(editSourceRequested(int,QPointF)), SLOT(on_pages_editSourceRequested(int,QPointF)));
        connect(page, SIGNAL(zoomToSelectionRequested(int,QRectF)), SLOT(on_pages_zoomToSelectionRequested(int,QRectF)));

        connect(page, SIGNAL(wasModified()), SLOT(on_pages_wasModified()));
    }
}

void DocumentView::prepareThumbnails()
{
    m_thumbnailItems.clear();
    m_thumbnailItems.reserve(m_pages.count());

    for(int index = 0; index < m_pages.count(); ++index)
    {
        ThumbnailItem* page = new ThumbnailItem(m_pages.at(index), index);

        page->setInvertColors(m_invertColors);

        m_thumbnailsScene->addItem(page);
        m_thumbnailItems.append(page);

        connect(page, SIGNAL(linkClicked(bool,int,qreal,qreal)), SLOT(on_pages_linkClicked(bool,int,qreal,qreal)));
    }
}

void DocumentView::prepareBackground()
{
    QColor backgroundColor;

    if(s_settings->pageItem().decoratePages())
    {
        backgroundColor = s_settings->pageItem().backgroundColor();
    }
    else
    {
        backgroundColor = s_settings->pageItem().paperColor();

        if(m_invertColors)
        {
            backgroundColor.setRgb(~backgroundColor.rgb());
        }
    }

    scene()->setBackgroundBrush(QBrush(backgroundColor));
    m_thumbnailsScene->setBackgroundBrush(QBrush(backgroundColor));
}

void DocumentView::prepareScene()
{
    // prepare scale factor and rotation

    const qreal visibleWidth = m_layout->visibleWidth(viewport()->width());
    const qreal visibleHeight = m_layout->visibleHeight(viewport()->height());

    foreach(PageItem* page, m_pageItems)
    {
#if QT_VERSION >= QT_VERSION_CHECK(5,1,0)

        page->setDevicePixelRatio(devicePixelRatio());

#endif // QT_VERSION

        page->setResolution(logicalDpiX(), logicalDpiY());

        page->setRotation(m_rotation);

        const qreal unscaledWidth = unscaledPageWidth(page);
        const qreal unscaledHeight = unscaledPageHeight(page);

        switch(m_scaleMode)
        {
        default:
        case ScaleFactorMode:
            page->setScaleFactor(m_scaleFactor);
            break;
        case FitToPageWidthMode:
            page->setScaleFactor(visibleWidth / unscaledWidth);
            break;
        case FitToPageSizeMode:
            page->setScaleFactor(qMin(visibleWidth / unscaledWidth, visibleHeight / unscaledHeight));
            break;
        }
    }

    // prepare layout

    m_heightToIndex.clear();

    qreal left = 0.0;
    qreal right = 0.0;
    qreal height = s_settings->documentView().pageSpacing();

    m_layout->prepareLayout(m_pageItems, m_rightToLeftMode,
                            m_heightToIndex, left, right, height);

    scene()->setSceneRect(left, 0.0, right - left, height);
}

void DocumentView::prepareView(qreal changeLeft, qreal changeTop, int visiblePage)
{
    const bool highlightCurrentThumbnail = s_settings->documentView().highlightCurrentThumbnail();

    qreal left = scene()->sceneRect().left();
    qreal top = scene()->sceneRect().top();
    qreal width = scene()->sceneRect().width();
    qreal height = scene()->sceneRect().height();

    int horizontalValue = 0;
    int verticalValue = 0;

    visiblePage = visiblePage == 0 ? m_currentPage : visiblePage;

    for(int index = 0; index < m_pageItems.count(); ++index)
    {
        PageItem* page = m_pageItems.at(index);
        const QRectF boundingRect = page->boundingRect().translated(page->pos());

        if(m_continuousMode)
        {
            page->setVisible(true);
        }
        else
        {
            if(m_layout->leftIndex(index) == m_currentPage - 1)
            {
                page->setVisible(true);

                top = boundingRect.top() - s_settings->documentView().pageSpacing();
                height = boundingRect.height() + 2.0 * s_settings->documentView().pageSpacing();
            }
            else
            {
                page->setVisible(false);

                page->cancelRender();
            }
        }

        if(index == visiblePage - 1)
        {
            horizontalValue = qFloor(boundingRect.left() + changeLeft * boundingRect.width());
            verticalValue = qFloor(boundingRect.top() + changeTop * boundingRect.height());
        }

        if(m_currentResult != m_results.end() && m_currentResult.key() == index)
        {
            m_highlight->setPos(page->pos());
            m_highlight->setTransform(page->transform());

            page->stackBefore(m_highlight);
        }

        m_thumbnailItems.at(index)->setCurrent(highlightCurrentThumbnail && (index == m_currentPage - 1));
    }

    setSceneRect(left, top, width, height);

    horizontalScrollBar()->setValue(horizontalValue);
    verticalScrollBar()->setValue(verticalValue);

    viewport()->update();
}

void DocumentView::prepareThumbnailsScene()
{
    const qreal thumbnailSpacing = s_settings->documentView().thumbnailSpacing();

    qreal left = 0.0;
    qreal right = m_thumbnailsOrientation == Qt::Vertical ? 0.0 : thumbnailSpacing;
    qreal top = 0.0;
    qreal bottom = m_thumbnailsOrientation == Qt::Vertical ? thumbnailSpacing : 0.0;

    const bool limitThumbnailsToResults = s_settings->documentView().limitThumbnailsToResults();

    for(int index = 0; index < m_thumbnailItems.count(); ++index)
    {
        PageItem* page = m_thumbnailItems.at(index);

        if(limitThumbnailsToResults && !m_results.isEmpty() && !m_results.contains(index))
        {
            page->setVisible(false);

            page->cancelRender();

            continue;
        }

        page->setVisible(true);

        // prepare scale factor

#if QT_VERSION >= QT_VERSION_CHECK(5,1,0)

        page->setDevicePixelRatio(devicePixelRatio());

#endif // QT_VERSION

        page->setResolution(logicalDpiX(), logicalDpiY());

        page->setScaleFactor(qMin(s_settings->documentView().thumbnailSize() / unscaledPageWidth(page), s_settings->documentView().thumbnailSize() / unscaledPageHeight(page)));

        // prepare layout

        const QRectF boundingRect = page->boundingRect();

        if(m_thumbnailsOrientation == Qt::Vertical)
        {
            page->setPos(-boundingRect.left() - 0.5 * boundingRect.width(), bottom - boundingRect.top());

            left = qMin(left, -0.5f * boundingRect.width() - thumbnailSpacing);
            right = qMax(right, 0.5f * boundingRect.width() + thumbnailSpacing);
            bottom += boundingRect.height() + thumbnailSpacing;
        }
        else
        {
            page->setPos(right - boundingRect.left(), -boundingRect.top() - 0.5 * boundingRect.height());

            top = qMin(top, -0.5f * boundingRect.height() - thumbnailSpacing);
            bottom = qMax(bottom, 0.5f * boundingRect.height() + thumbnailSpacing);
            right += boundingRect.width() + thumbnailSpacing;
        }
    }

    m_thumbnailsScene->setSceneRect(left, top, right - left, bottom - top);
}

void DocumentView::prepareHighlight(int index, const QRectF& rect)
{
    PageItem* page = m_pageItems.at(index);

    m_highlight->setPos(page->pos());
    m_highlight->setTransform(page->transform());

    m_highlight->setRect(rect.normalized());
    m_highlight->setBrush(QBrush(s_settings->pageItem().highlightColor()));

    page->stackBefore(m_highlight);

    m_highlight->setVisible(true);

    disconnect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(on_verticalScrollBar_valueChanged(int)));
    centerOn(m_highlight);
    connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(on_verticalScrollBar_valueChanged(int)));

    viewport()->update();
}

DocumentView::Results::iterator DocumentView::previousResult(const Results::iterator& result)
{
    return result != m_results.begin() ? result - 1 : m_results.end();
}

} // qpdfview
