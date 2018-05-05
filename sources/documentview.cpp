/*

Copyright 2014 S. Razi Alavizadeh
Copyright 2013 Thomas Etter
Copyright 2012-2015, 2018 Adam Reichold
Copyright 2014 Dorian Scholz
Copyright 2018 Egor Zenkov

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
#include <QDateTime>
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

#ifndef HAS_SYNCTEX_2

typedef synctex_scanner_t synctex_scanner_p;
typedef synctex_node_t synctex_node_p;

#define synctex_scanner_next_result(scanner) synctex_next_result(scanner)

#endif // HAS_SYNCTEX_2

#endif // WITH_SYNCTEX

#include "settings.h"
#include "model.h"
#include "pluginhandler.h"
#include "shortcuthandler.h"
#include "thumbnailitem.h"
#include "presentationview.h"
#include "searchmodel.h"
#include "searchtask.h"
#include "miscellaneous.h"
#include "documentlayout.h"

namespace
{

using namespace qpdfview;

// taken from http://rosettacode.org/wiki/Roman_numerals/Decode#C.2B.2B
int romanToInt(const QString& text)
{
    if(text.size() == 1)
    {
        switch(text.at(0).toLower().toLatin1())
        {
        case 'i': return 1;
        case 'v': return 5;
        case 'x': return 10;
        case 'l': return 50;
        case 'c': return 100;
        case 'd': return 500;
        case 'm': return 1000;
        }

        return 0;
    }

    int result = 0;
    int previous = 0, current = 0;

    for(int i = text.size() - 1; i >= 0; --i)
    {
        current = romanToInt(text.at(i));

        result += current < previous ? -current : current;

        previous = current;
    }

    return result;
}

// taken from http://rosettacode.org/wiki/Roman_numerals/Encode#C.2B.2B
QString intToRoman(int number)
{
    struct romandata_t
    {
        int value;
        char const* numeral;
    };

    static const romandata_t romandata[] =
    {
        { 1000, "m" },
        { 900, "cm" },
        { 500, "d" },
        { 400, "cd" },
        { 100, "c" },
        { 90, "xc" },
        { 50, "l" },
        { 40, "xl" },
        { 10, "x" },
        { 9, "ix" },
        { 5, "v" },
        { 4, "iv" },
        { 1, "i" },
        { 0, NULL }
    };

    if(number >= 4000)
    {
        return QLatin1String("?");
    }

    QString result;

    for(const romandata_t* current = romandata; current->value > 0; ++current)
    {
        while(number >= current->value)
        {
            number -= current->value;
            result += QLatin1String(current->numeral);
        }
    }

    return result;
}

bool copyFile(QFile& source, QFile& destination)
{
    const qint64 maxSize = 4096;
    qint64 size = -1;

    QScopedArrayPointer< char > buffer(new char[maxSize]);

    do
    {
        if((size = source.read(buffer.data(), maxSize)) < 0)
        {
            return false;
        }

        if(destination.write(buffer.data(), size) < 0)
        {
            return false;
        }
    }
    while(size > 0);

    return true;
}

inline void adjustFileTemplateSuffix(QTemporaryFile& temporaryFile, const QString& suffix)
{
    temporaryFile.setFileTemplate(temporaryFile.fileTemplate() + QLatin1String(".") + suffix);
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
        if(qstrcmp(colorModel->choices[index].choice, "CMYK") == 0)
        {
            return cupsAddOption("ColorModel", "CMYK", num_options, options);
        }
    }

    for(int index = 0; index < colorModel->num_choices; ++index)
    {
        if(qstrcmp(colorModel->choices[index].choice, "RGB") == 0)
        {
            return cupsAddOption("ColorModel", "RGB", num_options, options);
        }
    }

    return num_options;
}

#endif // WITH_CUPS

#ifdef WITH_SYNCTEX

DocumentView::SourceLink scanForSourceLink(const QString& filePath, const int page, QPointF pos)
{
    DocumentView::SourceLink sourceLink;

    if(synctex_scanner_p scanner = synctex_scanner_new_with_output_file(filePath.toLocal8Bit(), 0, 1))
    {
        if(synctex_edit_query(scanner, page, pos.x(), pos.y()) > 0)
        {
            for(synctex_node_p node = synctex_scanner_next_result(scanner); node != 0; node = synctex_scanner_next_result(scanner))
            {
                sourceLink.name = QString::fromLocal8Bit(synctex_scanner_get_name(scanner, synctex_node_tag(node)));
                sourceLink.line = qMax(synctex_node_line(node), 0);
                sourceLink.column = qMax(synctex_node_column(node), 0);
                break;
            }
        }

        synctex_scanner_free(scanner);
    }

    return sourceLink;
}

#endif // WITH_SYNCTEX

inline bool modifiersAreActive(const QWheelEvent* event, Qt::KeyboardModifiers modifiers)
{
    if(modifiers == Qt::NoModifier)
    {
        return false;
    }

    return event->modifiers() == modifiers || (event->buttons() & modifiers) != 0;
}

inline bool modifiersUseMouseButton(Settings* settings, Qt::MouseButton mouseButton)
{
    return ((settings->documentView().zoomModifiers() | settings->documentView().rotateModifiers() | settings->documentView().scrollModifiers()) & mouseButton) != 0;
}

inline void adjustScaleFactor(RenderParam& renderParam, qreal scaleFactor)
{
    if(!qFuzzyCompare(renderParam.scaleFactor(), scaleFactor))
    {
        renderParam.setScaleFactor(scaleFactor);
    }
}

inline void setValueIfNotVisible(QScrollBar* scrollBar, int value)
{
    if(value < scrollBar->value() || value > scrollBar->value() + scrollBar->pageStep())
    {
        scrollBar->setValue(value);
    }
}

inline int pageOfResult(const QModelIndex& index)
{
    return index.data(SearchModel::PageRole).toInt();
}

inline QRectF rectOfResult(const QModelIndex& index)
{
    return index.data(SearchModel::RectRole).toRectF();
}

class OutlineModel : public QAbstractItemModel
{
public:
    OutlineModel(const Model::Outline& outline, DocumentView* parent) : QAbstractItemModel(parent),
        m_outline(outline)
    {
    }

    QModelIndex index(int row, int column, const QModelIndex& parent) const
    {
        if(!hasIndex(row, column, parent))
        {
            return QModelIndex();
        }

        if(parent.isValid())
        {
            const Model::Section* section = resolveIndex(parent);

            return createIndex(row, column, &section->children);
        }
        else
        {
            return createIndex(row, column, &m_outline);
        }
    }

    QModelIndex parent(const QModelIndex& child) const
    {
        if(!child.isValid())
        {
            return QModelIndex();
        }

        const Model::Outline* children = static_cast< const Model::Outline* >(child.internalPointer());

        if(&m_outline != children)
        {
            return findParent(&m_outline, children);
        }

        return QModelIndex();
    }

    int columnCount(const QModelIndex&) const
    {
        return 2;
    }

    int rowCount(const QModelIndex& parent) const
    {
        if(parent.isValid())
        {
            const Model::Section* section = resolveIndex(parent);

            return section->children.size();
        }
        else
        {
            return m_outline.size();
        }
    }

    QVariant data(const QModelIndex& index, int role) const
    {
        if(!index.isValid())
        {
            return QVariant();
        }

        const Model::Section* section = resolveIndex(index);

        switch(role)
        {
        case Qt::DisplayRole:
            switch(index.column())
            {
            case 0:
                return section->title;
            case 1:
                return pageLabel(section->link.page);
            default:
                return QVariant();
            }
        case Model::Document::PageRole:
            return section->link.page;
        case Model::Document::LeftRole:
            return section->link.left;
        case Model::Document::TopRole:
            return section->link.top;
        case Model::Document::FileNameRole:
            return section->link.urlOrFileName;
        case Model::Document::ExpansionRole:
            return m_expanded.contains(section);
        default:
            return QVariant();
        }
    }

    bool setData(const QModelIndex& index, const QVariant& value, int role)
    {
        if(!index.isValid() || role != Model::Document::ExpansionRole)
        {
            return false;
        }

        const Model::Section* section = resolveIndex(index);

        if(value.toBool())
        {
            m_expanded.insert(section);
        }
        else
        {
            m_expanded.remove(section);
        }

        return true;
    }

private:
    const Model::Outline m_outline;

    DocumentView* documentView() const
    {
        return static_cast< DocumentView* >(QObject::parent());
    }

    QString pageLabel(int pageNumber) const
    {
        return documentView()->pageLabelFromNumber(pageNumber);
    }

    QSet< const Model::Section* > m_expanded;

    const Model::Section* resolveIndex(const QModelIndex& index) const
    {
        return &static_cast< const Model::Outline* >(index.internalPointer())->at(index.row());
    }

    QModelIndex createIndex(int row, int column, const Model::Outline* outline) const
    {
        return QAbstractItemModel::createIndex(row, column, const_cast< void* >(static_cast< const void* >(outline)));
    }

    QModelIndex findParent(const Model::Outline* outline, const Model::Outline* children) const
    {
        for(Model::Outline::const_iterator section = outline->begin(); section != outline->end(); ++section)
        {
            if(&section->children == children)
            {
                return createIndex(section - outline->begin(), 0, outline);
            }
        }

        for(Model::Outline::const_iterator section = outline->begin(); section != outline->end(); ++section)
        {
            const QModelIndex parent = findParent(&section->children, children);

            if(parent.isValid())
            {
                return parent;
            }
        }

        return QModelIndex();
    }

};

class FallbackOutlineModel : public QAbstractTableModel
{
public:
    FallbackOutlineModel(DocumentView* parent) : QAbstractTableModel(parent)
    {
    }

    int columnCount(const QModelIndex&) const
    {
        return 2;
    }

    int rowCount(const QModelIndex& parent) const
    {
        if(parent.isValid())
        {
            return 0;
        }

        return numberOfPages();
    }

    QVariant data(const QModelIndex& index, int role) const
    {
        if(!index.isValid())
        {
            return QVariant();
        }

        const int pageNumber = index.row() + 1;

        switch(role)
        {
        case Qt::DisplayRole:
            switch(index.column())
            {
            case 0:
                return DocumentView::tr("Page %1").arg(pageLabel(pageNumber));
            case 1:
                return pageLabel(pageNumber);
            default:
                return QVariant();
            }
        case Model::Document::PageRole:
            return pageNumber;
        case Model::Document::LeftRole:
        case Model::Document::TopRole:
            return qQNaN();
        default:
            return QVariant();
        }
    }

private:
    DocumentView* documentView() const
    {
        return static_cast< DocumentView* >(QObject::parent());
    }

    int numberOfPages() const
    {
        return documentView()->numberOfPages();
    }

    QString pageLabel(int pageNumber) const
    {
        return documentView()->pageLabelFromNumber(pageNumber);
    }

};

class PropertiesModel : public QAbstractTableModel
{
public:
    PropertiesModel(const Model::Properties& properties, DocumentView* parent = 0) : QAbstractTableModel(parent),
        m_properties(properties)
    {
    }

    int columnCount(const QModelIndex&) const
    {
        return 2;
    }

    int rowCount(const QModelIndex& parent) const
    {
        if(parent.isValid())
        {
            return 0;
        }

        return m_properties.size();
    }

    QVariant data(const QModelIndex& index, int role) const
    {
        if(!index.isValid() || role != Qt::DisplayRole)
        {
            return QVariant();
        }

        switch (index.column())
        {
        case 0:
            return m_properties[index.row()].first;
        case 1:
            return m_properties[index.row()].second;
        default:
            return QVariant();
        }
    }

private:
    const Model::Properties m_properties;

};

void addProperty(Model::Properties& properties, const char* name, const QString& value)
{
    properties.append(qMakePair(DocumentView::tr(name), value));
}

QString formatFileSize(qint64 size)
{
    static const char* const units[] = { "B", "kB", "MB", "GB" };
    static const char* const* const lastUnit = &units[sizeof(units) / sizeof(units[0]) - 1];
    const char* const* unit = &units[0];

    while(size > 2048 && unit < lastUnit)
    {
        size /= 1024;
        unit++;
    }

    return QString("%1 %2").arg(size).arg(*unit);
}

void addFileProperties(Model::Properties& properties, const QFileInfo& fileInfo)
{
    addProperty(properties, "File path", fileInfo.absoluteFilePath());
    addProperty(properties, "File size", formatFileSize(fileInfo.size()));
    addProperty(properties, "File created", fileInfo.created().toString());
    addProperty(properties, "File last modified", fileInfo.lastModified().toString());
    addProperty(properties, "File owner", fileInfo.owner());
    addProperty(properties, "File group", fileInfo.owner());
}

void appendToPath(const QModelIndex& index, QByteArray& path)
{
    path.append(index.data(Qt::DisplayRole).toByteArray()).append('\0');
}

void saveExpandedPaths(const QAbstractItemModel* model, QSet< QByteArray >& paths, const QModelIndex& index = QModelIndex(), QByteArray path = QByteArray())
{
    appendToPath(index, path);

    if(model->data(index, Model::Document::ExpansionRole).toBool())
    {
        paths.insert(path);
    }

    for(int row = 0, rowCount = model->rowCount(index); row < rowCount; ++row)
    {
        saveExpandedPaths(model, paths, model->index(row, 0, index), path);
    }
}

void restoreExpandedPaths(QAbstractItemModel* model, const QSet< QByteArray >& paths, const QModelIndex& index = QModelIndex(), QByteArray path = QByteArray())
{
    appendToPath(index, path);

    if(paths.contains(path))
    {
        model->setData(index, true, Model::Document::ExpansionRole);
    }

    for(int row = 0, rowCount = model->rowCount(index); row < rowCount; ++row)
    {
        restoreExpandedPaths(model, paths, model->index(row, 0, index), path);
    }
}

} // anonymous

namespace qpdfview
{

class DocumentView::VerticalScrollBarChangedBlocker
{
    Q_DISABLE_COPY(VerticalScrollBarChangedBlocker)

private:
    DocumentView* const that;

public:

    VerticalScrollBarChangedBlocker(DocumentView* that) : that(that)
    {
        that->m_verticalScrollBarChangedBlocked = true;
    }

    ~VerticalScrollBarChangedBlocker()
    {
        that->m_verticalScrollBarChangedBlocked = false;
    }

};

Settings* DocumentView::s_settings = 0;
ShortcutHandler* DocumentView::s_shortcutHandler = 0;
SearchModel* DocumentView::s_searchModel = 0;

DocumentView::DocumentView(QWidget* parent) : QGraphicsView(parent),
    m_autoRefreshWatcher(0),
    m_autoRefreshTimer(0),
    m_prefetchTimer(0),
    m_document(0),
    m_pages(),
    m_fileInfo(),
    m_wasModified(false),
    m_currentPage(-1),
    m_firstPage(-1),
    m_past(),
    m_future(),
    m_layout(new SinglePageLayout),
    m_continuousMode(false),
    m_scaleMode(ScaleFactorMode),
    m_scaleFactor(1.0),
    m_rotation(RotateBy0),
    m_renderFlags(0),
    m_highlightAll(false),
    m_rubberBandMode(ModifiersMode),
    m_pageItems(),
    m_thumbnailItems(),
    m_highlight(0),
    m_thumbnailsViewportSize(),
    m_thumbnailsOrientation(Qt::Vertical),
    m_thumbnailsScene(0),
    m_outlineModel(0),
    m_propertiesModel(0),
    m_verticalScrollBarChangedBlocked(false),
    m_currentResult(),
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

    if(s_searchModel == 0)
    {
        s_searchModel = SearchModel::instance();
    }

    setScene(new QGraphicsScene(this));

    setFocusPolicy(Qt::StrongFocus);
    setAcceptDrops(false);
    setDragMode(QGraphicsView::ScrollHandDrag);

    connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(on_verticalScrollBar_valueChanged()));

    m_thumbnailsScene = new QGraphicsScene(this);

    // highlight

    m_highlight = new QGraphicsRectItem();
    m_highlight->setGraphicsEffect(new GraphicsCompositionModeEffect(QPainter::CompositionMode_Multiply, this));

    m_highlight->setVisible(false);
    scene()->addItem(m_highlight);

    // search

    m_searchTask = new SearchTask(this);

    connect(m_searchTask, SIGNAL(finished()), SIGNAL(searchFinished()));

    connect(m_searchTask, SIGNAL(progressChanged(int)), SLOT(on_searchTask_progressChanged(int)));
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
    connect(this, SIGNAL(renderFlagsChanged(qpdfview::RenderFlags)), m_prefetchTimer, SLOT(start()));

    connect(m_prefetchTimer, SIGNAL(timeout()), SLOT(on_prefetch_timeout()));

    // settings

    m_continuousMode = s_settings->documentView().continuousMode();
    m_layout.reset(DocumentLayout::fromLayoutMode(s_settings->documentView().layoutMode()));
    m_rightToLeftMode = s_settings->documentView().rightToLeftMode();

    m_scaleMode = s_settings->documentView().scaleMode();
    m_scaleFactor = s_settings->documentView().scaleFactor();
    m_rotation = s_settings->documentView().rotation();

    if(s_settings->documentView().invertColors())
    {
        m_renderFlags |= InvertColors;
    }

    if(s_settings->documentView().convertToGrayscale())
    {
        m_renderFlags |= ConvertToGrayscale;
    }

    if(s_settings->documentView().trimMargins())
    {
        m_renderFlags |= TrimMargins;
    }

    switch(s_settings->documentView().compositionMode())
    {
    default:
    case DefaultCompositionMode:
        break;
    case DarkenWithPaperColorMode:
        m_renderFlags |= DarkenWithPaperColor;
        break;
    case LightenWithPaperColorMode:
        m_renderFlags |= LightenWithPaperColor;
        break;
    }

    m_highlightAll = s_settings->documentView().highlightAll();
}

DocumentView::~DocumentView()
{
    m_searchTask->cancel();
    m_searchTask->wait();

    s_searchModel->clearResults(this);

    qDeleteAll(m_pageItems);
    qDeleteAll(m_thumbnailItems);

    qDeleteAll(m_pages);
    delete m_document;
}

void DocumentView::setFirstPage(int firstPage)
{
    if(m_firstPage != firstPage)
    {
        m_firstPage = firstPage;

        for(int index = 0; index < m_thumbnailItems.count(); ++index)
        {
            m_thumbnailItems.at(index)->setText(pageLabelFromNumber(index + 1));
        }

        prepareThumbnailsScene();

        emit numberOfPagesChanged(m_pages.count());
        emit currentPageChanged(m_currentPage);
    }
}

QString DocumentView::defaultPageLabelFromNumber(int number) const
{
    QLocale modifiedLocale = locale();

    modifiedLocale.setNumberOptions(modifiedLocale.numberOptions() | QLocale::OmitGroupSeparator);

    return modifiedLocale.toString(number);
}

QString DocumentView::pageLabelFromNumber(int number) const
{
    QString label;

    if(hasFrontMatter())
    {
        if(number < m_firstPage)
        {
            label = number < 4000 ? intToRoman(number) : defaultPageLabelFromNumber(-number);
        }
        else
        {
            label = defaultPageLabelFromNumber(number - m_firstPage + 1);
        }
    }
    else if(number >= 1 && number <= m_pages.count())
    {
        const QString& pageLabel = m_pages.at(number - 1)->label();

        if(number != pageLabel.toInt())
        {
            label = pageLabel;
        }
    }

    if(label.isEmpty())
    {
        label = defaultPageLabelFromNumber(number);
    }

    return label;
}

int DocumentView::pageNumberFromLabel(const QString& label) const
{
    if(hasFrontMatter())
    {
        bool ok = false;
        int value = locale().toInt(label, &ok);

        if(ok)
        {
            if(value < 0)
            {
                value = -value; // front matter
            }
            else
            {
                value = value + m_firstPage - 1; // body matter
            }
        }
        else
        {
            value = romanToInt(label);
        }

        return value;
    }

    for(int index = 0; index < m_pages.count(); ++index)
    {
        if(m_pages.at(index)->label() == label)
        {
            return index + 1;
        }
    }

    return locale().toInt(label);
}

QString DocumentView::title() const
{
    QString title;

    if(s_settings->mainWindow().documentTitleAsTabTitle())
    {
        for(int row = 0, rowCount = m_propertiesModel->rowCount(); row < rowCount; ++row)
        {
            const QString key = m_propertiesModel->index(row, 0).data().toString();
            const QString value = m_propertiesModel->index(row, 1).data().toString();

            if(QLatin1String("Title") == key)
            {
                title = value;
                break;
            }
        }
    }

    if(title.isEmpty())
    {
        title = m_fileInfo.completeBaseName();
    }

    return title;
}

QStringList DocumentView::openFilter()
{
    return PluginHandler::openFilter();
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

        s_settings->documentView().setRotation(m_rotation);
    }
}

void DocumentView::setRenderFlags(qpdfview::RenderFlags renderFlags)
{
    if(m_renderFlags != renderFlags)
    {
        const qpdfview::RenderFlags changedFlags = m_renderFlags ^ renderFlags;

        m_renderFlags = renderFlags;

        qreal left = 0.0, top = 0.0;
        saveLeftAndTop(left, top);

        prepareScene();
        prepareView(left, top);

        prepareThumbnailsScene();

        if(changedFlags.testFlag(InvertColors))
        {
            prepareBackground();

            emit invertColorsChanged(invertColors());

            s_settings->documentView().setInvertColors(invertColors());
        }

        if(changedFlags.testFlag(ConvertToGrayscale))
        {
            emit convertToGrayscaleChanged(convertToGrayscale());

            s_settings->documentView().setConvertToGrayscale(convertToGrayscale());
        }

        if(changedFlags.testFlag(TrimMargins))
        {
            emit trimMarginsChanged(trimMargins());

            s_settings->documentView().setTrimMargins(trimMargins());
        }

        if(changedFlags.testFlag(DarkenWithPaperColor) || changedFlags.testFlag(LightenWithPaperColor))
        {
            emit compositionModeChanged(compositionMode());

            s_settings->documentView().setCompositionMode(compositionMode());
        }

        emit renderFlagsChanged(m_renderFlags);
    }
}

void DocumentView::setRenderFlag(qpdfview::RenderFlag renderFlag, bool enabled)
{
    if(enabled)
    {
        setRenderFlags(m_renderFlags | renderFlag);
    }
    else
    {
        setRenderFlags(m_renderFlags & ~renderFlag);
    }
}

CompositionMode DocumentView::compositionMode() const
{
    if(m_renderFlags.testFlag(DarkenWithPaperColor))
    {
        return DarkenWithPaperColorMode;
    }
    else if(m_renderFlags.testFlag(LightenWithPaperColor))
    {
        return LightenWithPaperColorMode;
    }
    else
    {
        return DefaultCompositionMode;
    }
}

void DocumentView::setCompositionMode(CompositionMode compositionMode)
{
    switch(compositionMode)
    {
    default:
    case DefaultCompositionMode:
        setRenderFlags((renderFlags() & ~DarkenWithPaperColor) & ~LightenWithPaperColor);
        break;
    case DarkenWithPaperColorMode:
        setRenderFlags((renderFlags() | DarkenWithPaperColor) & ~LightenWithPaperColor);
        break;
    case LightenWithPaperColorMode:
        setRenderFlags((renderFlags() & ~DarkenWithPaperColor) | LightenWithPaperColor);
        break;
    }
}

void DocumentView::setHighlightAll(bool highlightAll)
{
    if(m_highlightAll != highlightAll)
    {
        m_highlightAll = highlightAll;

        if(m_highlightAll)
        {
            for(int index = 0; index < m_pages.count(); ++index)
            {
                const QList< QRectF >& results = s_searchModel->resultsOnPage(this, index + 1);

                m_pageItems.at(index)->setHighlights(results);
                m_thumbnailItems.at(index)->setHighlights(results);
            }
        }
        else
        {
            for(int index = 0; index < m_pages.count(); ++index)
            {
                m_pageItems.at(index)->setHighlights(QList< QRectF >());
                m_thumbnailItems.at(index)->setHighlights(QList< QRectF >());
            }
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

void DocumentView::setThumbnailsViewportSize(QSize thumbnailsViewportSize)
{
    if(m_thumbnailsViewportSize != thumbnailsViewportSize)
    {
        m_thumbnailsViewportSize = thumbnailsViewportSize;

        prepareThumbnailsScene();
    }
}

void DocumentView::setThumbnailsOrientation(Qt::Orientation thumbnailsOrientation)
{
    if(m_thumbnailsOrientation != thumbnailsOrientation)
    {
        m_thumbnailsOrientation = thumbnailsOrientation;

        prepareThumbnailsScene();
    }
}

QSet< QByteArray > DocumentView::saveExpandedPaths() const
{
    QSet< QByteArray > expandedPaths;

    ::saveExpandedPaths(m_outlineModel.data(), expandedPaths);

    return expandedPaths;
}

void DocumentView::restoreExpandedPaths(const QSet< QByteArray >& expandedPaths)
{
    ::restoreExpandedPaths(m_outlineModel.data(), expandedPaths);
}

QAbstractItemModel* DocumentView::fontsModel() const
{
    return m_document->fonts();
}

bool DocumentView::searchWasCanceled() const
{
    return m_searchTask->wasCanceled();
}

int DocumentView::searchProgress() const
{
    return m_searchTask->progress();
}

QString DocumentView::searchText() const
{
    return m_searchTask->text();
}

bool DocumentView::searchMatchCase() const
{
    return m_searchTask->matchCase();
}

bool DocumentView::searchWholeWords() const
{
    return m_searchTask->wholeWords();
}

QPair< QString, QString > DocumentView::searchContext(int page, const QRectF& rect) const
{
    if(page < 1 || page > m_pages.size() || rect.isEmpty())
    {
        return qMakePair(QString(), QString());
    }

    // Fetch at most half of a line as centered on the given rectangle as possible.
    const qreal pageWidth = m_pages.at(page - 1)->size().width();
    const qreal width = qMax(rect.width(), pageWidth / qreal(2));
    const qreal x = qBound(qreal(0), rect.x() + rect.width() / qreal(2) - width / qreal(2), pageWidth - width);

    const QRectF surroundingRect(x, rect.top(), width, rect.height());

    const QString& matchedText = m_pages.at(page - 1)->cachedText(rect);
    const QString& surroundingText = m_pages.at(page - 1)->cachedText(surroundingRect);

    return qMakePair(matchedText, surroundingText);
}

bool DocumentView::hasSearchResults()
{
    return s_searchModel->hasResults(this);
}

QString DocumentView::resolveFileName(QString fileName) const
{
    if(QFileInfo(fileName).isRelative())
    {
        fileName = m_fileInfo.dir().filePath(fileName);
    }

    return fileName;
}

QUrl DocumentView::resolveUrl(QUrl url) const
{
    const QString path = url.path();

    if(url.isRelative() && QFileInfo(path).isRelative())
    {
        url.setPath(m_fileInfo.dir().filePath(path));
    }

    return url;
}

DocumentView::SourceLink DocumentView::sourceLink(QPoint pos)
{
    SourceLink sourceLink;

#ifdef WITH_SYNCTEX

    if(const PageItem* page = dynamic_cast< PageItem* >(itemAt(pos)))
    {
        const int sourcePage = page->index() + 1;
        const QPointF sourcePos = page->sourcePos(page->mapFromScene(mapToScene(pos)));

        sourceLink = scanForSourceLink(m_fileInfo.absoluteFilePath(), sourcePage, sourcePos);
    }

#else

    Q_UNUSED(pos);

#endif // WITH_SYNCTEX

    return sourceLink;
}

void DocumentView::openInSourceEditor(const DocumentView::SourceLink& sourceLink)
{
    if(!s_settings->documentView().sourceEditor().isEmpty())
    {
        const QString absoluteFilePath = m_fileInfo.dir().absoluteFilePath(sourceLink.name);
        const QString sourceEditorCommand = s_settings->documentView().sourceEditor().arg(absoluteFilePath, QString::number(sourceLink.line), QString::number(sourceLink.column));

        QProcess::startDetached(sourceEditorCommand);
    }
    else
    {
        QMessageBox::information(this, tr("Information"), tr("The source editor has not been set."));
    }
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
        QVector< Model::Page* > pages;

        if(!checkDocument(filePath, document, pages))
        {
            delete document;
            qDeleteAll(pages);

            return false;
        }

        m_fileInfo.setFile(filePath);
        m_wasModified = false;

        m_currentPage = 1;

        m_past.clear();
        m_future.clear();

        prepareDocument(document, pages);

        loadDocumentDefaults();

        adjustScrollBarPolicy();

        prepareScene();
        prepareView();

        prepareThumbnailsScene();

        emit documentChanged();

        emit numberOfPagesChanged(m_pages.count());
        emit currentPageChanged(m_currentPage);

        emit canJumpChanged(false, false);

        emit continuousModeChanged(m_continuousMode);
        emit layoutModeChanged(m_layout->layoutMode());
        emit rightToLeftModeChanged(m_rightToLeftMode);
    }

    return document != 0;
}

bool DocumentView::refresh()
{
    Model::Document* document = PluginHandler::instance()->loadDocument(m_fileInfo.filePath());

    if(document != 0)
    {
        QVector< Model::Page* > pages;

        if(!checkDocument(m_fileInfo.filePath(), document, pages))
        {
            delete document;
            qDeleteAll(pages);

            return false;
        }

        qreal left = 0.0, top = 0.0;
        saveLeftAndTop(left, top);

        m_wasModified = false;

        m_currentPage = qMin(m_currentPage, document->numberOfPages());

        QSet< QByteArray > expandedPaths;
        ::saveExpandedPaths(m_outlineModel.data(), expandedPaths);

        prepareDocument(document, pages);

        ::restoreExpandedPaths(m_outlineModel.data(), expandedPaths);

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
    // Save document to temporary file...
    QTemporaryFile temporaryFile;

    adjustFileTemplateSuffix(temporaryFile, QFileInfo(filePath).suffix());

    if(!temporaryFile.open())
    {
        return false;
    }

    temporaryFile.close();

    if(!m_document->save(temporaryFile.fileName(), withChanges))
    {
        return false;
    }

    // Copy from temporary file to actual file...
    QFile file(filePath);

    if(!temporaryFile.open())
    {
        return false;
    }

    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        return false;
    }

    if(!copyFile(temporaryFile, file))
    {
        return false;
    }

    if(withChanges)
    {
        m_wasModified = false;
    }

    return true;
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

void DocumentView::jumpToPage(int page, bool trackChange, qreal newLeft, qreal newTop)
{
    if(page >= 1 && page <= m_pages.count())
    {
        qreal left = 0.0, top = 0.0;
        saveLeftAndTop(left, top);

        if(qIsNaN(newLeft))
        {
            newLeft = qBound(qreal(0.0), left, qreal(1.0));
        }

        if(qIsNaN(newTop))
        {
            newTop = qBound(qreal(0.0), top, qreal(1.0));
        }

        if(m_currentPage != m_layout->currentPage(page) || qAbs(left - newLeft) > 0.01 || qAbs(top - newTop) > 0.01)
        {
            if(trackChange)
            {
                m_past.append(Position(m_currentPage, left, top));
                m_future.clear();

                emit canJumpChanged(true, false);
            }

            m_currentPage = m_layout->currentPage(page);

            prepareView(newLeft, newTop, false, page);

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

void DocumentView::startSearch(const QString& text, bool matchCase, bool wholeWords)
{
    cancelSearch();
    clearResults();

    m_searchTask->start(m_pages, text, matchCase, wholeWords, m_currentPage, s_settings->documentView().parallelSearchExecution());
}

void DocumentView::cancelSearch()
{
    m_searchTask->cancel();
    m_searchTask->wait();
}

void DocumentView::clearResults()
{
    s_searchModel->clearResults(this);

    m_currentResult = QModelIndex();

    m_highlight->setVisible(false);

    foreach(PageItem* page, m_pageItems)
    {
        page->setHighlights(QList< QRectF >());
    }

    foreach(ThumbnailItem* page, m_thumbnailItems)
    {
        page->setHighlights(QList< QRectF >());
    }

    if(s_settings->documentView().limitThumbnailsToResults())
    {
        prepareThumbnailsScene();
    }
}

void DocumentView::findPrevious()
{
    checkResult();

    m_currentResult = s_searchModel->findResult(this, m_currentResult, m_currentPage, SearchModel::FindPrevious);

    applyResult();
}

void DocumentView::findNext()
{
    checkResult();

    m_currentResult = s_searchModel->findResult(this, m_currentResult, m_currentPage, SearchModel::FindNext);

    applyResult();
}

void DocumentView::findResult(const QModelIndex& index)
{
    const int page = pageOfResult(index);
    const QRectF rect = rectOfResult(index);

    if(page >= 1 && page <= m_pages.count() && !rect.isEmpty())
    {
        m_currentResult = index;

        applyResult();
    }
}

void DocumentView::zoomIn()
{
    if(scaleMode() != ScaleFactorMode)
    {
        const qreal currentScaleFactor = m_pageItems.at(m_currentPage - 1)->renderParam().scaleFactor();

        setScaleFactor(qMin(currentScaleFactor * s_settings->documentView().zoomFactor(),
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
        const qreal currentScaleFactor = m_pageItems.at(m_currentPage - 1)->renderParam().scaleFactor();

        setScaleFactor(qMax(currentScaleFactor / s_settings->documentView().zoomFactor(),
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
    presentationView->setRenderFlags(renderFlags());

    presentationView->jumpToPage(currentPage(), false);

    if(s_settings->presentationView().synchronize())
    {
        connect(this, SIGNAL(currentPageChanged(int,bool)), presentationView, SLOT(jumpToPage(int,bool)));
        connect(presentationView, SIGNAL(currentPageChanged(int,bool)), this, SLOT(jumpToPage(int,bool)));
    }
}

void DocumentView::on_verticalScrollBar_valueChanged()
{
    if(m_verticalScrollBarChangedBlocked || !m_continuousMode)
    {
        return;
    }

    int currentPage = -1;
    const QRectF visibleRect = mapToScene(viewport()->rect()).boundingRect();

    for(int index = 0, count = m_pageItems.count(); index < count; ++index)
    {
        PageItem* page = m_pageItems.at((m_currentPage - 1 + index) % count);

        const int pageNumber = page->index() + 1;
        const QRectF pageRect = page->boundingRect().translated(page->pos());

        if(!pageRect.intersects(visibleRect))
        {
            page->cancelRender();
        }
        else if(currentPage == -1 &&
                 m_layout->currentPage(pageNumber) == pageNumber &&
                 m_layout->isCurrentPage(visibleRect, pageRect))
        {
            currentPage = pageNumber;
        }
    }

    if(currentPage != -1 && m_currentPage != currentPage)
    {
        m_currentPage = currentPage;

        emit currentPageChanged(m_currentPage);

        if(s_settings->documentView().highlightCurrentThumbnail())
        {
            for(int index = 0; index < m_thumbnailItems.count(); ++index)
            {
                m_thumbnailItems.at(index)->setHighlighted(index == m_currentPage - 1);
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

void DocumentView::on_searchTask_progressChanged(int progress)
{
    s_searchModel->updateProgress(this);

    emit searchProgressChanged(progress);
}

void DocumentView::on_searchTask_resultsReady(int index, const QList< QRectF >& results)
{
    if(m_searchTask->wasCanceled())
    {
        return;
    }

    s_searchModel->insertResults(this, index + 1, results);

    if(m_highlightAll)
    {
        m_pageItems.at(index)->setHighlights(results);
        m_thumbnailItems.at(index)->setHighlights(results);
    }

    if(s_settings->documentView().limitThumbnailsToResults())
    {
        prepareThumbnailsScene();
    }

    if(!results.isEmpty() && !m_currentResult.isValid())
    {
        setFocus();

        findNext();
    }
}

void DocumentView::on_pages_cropRectChanged()
{
    qreal left = 0.0, top = 0.0;
    saveLeftAndTop(left, top);

    prepareScene();
    prepareView(left, top);
}

void DocumentView::on_thumbnails_cropRectChanged()
{
    prepareThumbnailsScene();
}

void DocumentView::on_pages_linkClicked(bool newTab, int page, qreal left, qreal top)
{
    if(newTab)
    {
        emit linkClicked(page);
    }
    else
    {
        jumpToPage(page, true, left, top);
    }
}

void DocumentView::on_pages_linkClicked(bool newTab, const QString& fileName, int page)
{
    emit linkClicked(newTab, resolveFileName(fileName), page);
}

void DocumentView::on_pages_linkClicked(const QString& url)
{
    if(s_settings->documentView().openUrl())
    {
        QDesktopServices::openUrl(resolveUrl(url));
    }
    else
    {
        QMessageBox::information(this, tr("Information"), tr("Opening URL is disabled in the settings."));
    }
}

void DocumentView::on_pages_rubberBandFinished()
{
    setRubberBandMode(ModifiersMode);
}

void DocumentView::on_pages_zoomToSelection(int page, const QRectF& rect)
{
    if(rect.isEmpty())
    {
        return;
    }

    const qreal visibleWidth = m_layout->visibleWidth(viewport()->width());
    const qreal visibleHeight = m_layout->visibleHeight(viewport()->height());

    const QSizeF displayedSize = m_pageItems.at(page - 1)->displayedSize();

    setScaleFactor(qMin(qMin(visibleWidth / displayedSize.width() / rect.width(),
                             visibleHeight / displayedSize.height() / rect.height()),
                        Defaults::DocumentView::maximumScaleFactor()));

    setScaleMode(ScaleFactorMode);

    jumpToPage(page, false, rect.left(), rect.top());
}

void DocumentView::on_pages_openInSourceEditor(int page, QPointF pos)
{
#ifdef WITH_SYNCTEX

    if(const DocumentView::SourceLink sourceLink = scanForSourceLink(m_fileInfo.absoluteFilePath(), page, pos))
    {
        openInSourceEditor(sourceLink);
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

void DocumentView::on_pages_wasModified()
{
    m_wasModified = true;

    emit documentModified();
}

void DocumentView::resizeEvent(QResizeEvent* event)
{
    qreal left = 0.0, top = 0.0;
    saveLeftAndTop(left, top);

    QGraphicsView::resizeEvent(event);

    if(m_scaleMode != ScaleFactorMode)
    {
        prepareScene();
        prepareView(left, top);
    }
}

void DocumentView::keyPressEvent(QKeyEvent* event)
{
    foreach(const PageItem* page, m_pageItems)
    {
        if(page->showsAnnotationOverlay() || page->showsFormFieldOverlay())
        {
            QGraphicsView::keyPressEvent(event);
            return;
        }
    }

    const QKeySequence keySequence(event->modifiers() + event->key());

    int maskedKey = -1;
    bool maskedKeyActive = false;

    switch(event->key())
    {
    case Qt::Key_PageUp:
    case Qt::Key_PageDown:
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_Left:
    case Qt::Key_Right:
        maskedKeyActive = true;
        break;
    }

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
    else if(maskedKeyActive)
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

void DocumentView::mousePressEvent(QMouseEvent* event)
{
    if(event->button() == Qt::XButton1)
    {
        event->accept();

        jumpBackward();
    }
    else if(event->button() == Qt::XButton2)
    {
        event->accept();

        jumpForward();
    }

    QGraphicsView::mousePressEvent(event);
}

void DocumentView::wheelEvent(QWheelEvent* event)
{
    const bool noModifiersActive = event->modifiers() == Qt::NoModifier;
    const bool zoomModifiersActive = modifiersAreActive(event, s_settings->documentView().zoomModifiers());
    const bool rotateModifiersActive = modifiersAreActive(event, s_settings->documentView().rotateModifiers());
    const bool scrollModifiersActive = modifiersAreActive(event, s_settings->documentView().scrollModifiers());

    if(zoomModifiersActive)
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
    else if(rotateModifiersActive)
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
    else if(scrollModifiersActive)
    {
        QWheelEvent wheelEvent(event->pos(), event->delta(), event->buttons(), Qt::AltModifier, Qt::Horizontal);
        QGraphicsView::wheelEvent(&wheelEvent);

        event->accept();
        return;
    }
    else if(noModifiersActive && !m_continuousMode)
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

    QGraphicsView::wheelEvent(event);
}

void DocumentView::contextMenuEvent(QContextMenuEvent* event)
{
    if(event->reason() == QContextMenuEvent::Mouse && modifiersUseMouseButton(s_settings, Qt::RightButton))
    {
        event->accept();
        return;
    }

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

    if(dest == 0)
    {
        qWarning() << cupsLastErrorString();

        cupsFreeDests(num_dests, dests);

        return false;
    }

    for(int index = 0; index < dest->num_options; ++index)
    {
        num_options = cupsAddOption(dest->options[index].name, dest->options[index].value, num_options, &options);
    }

    const QStringList cupsOptions = printer->printEngine()->property(QPrintEngine::PrintEnginePropertyKey(0xfe00)).toStringList();

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
        num_options = cupsAddOption("Ink", "COLOR", num_options, &options);
        break;
    case QPrinter::GrayScale:
        num_options = cupsAddOption("ColorModel", "Gray", num_options, &options);
        num_options = cupsAddOption("Ink", "MONO", num_options, &options);
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

#else

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

    adjustFileTemplateSuffix(temporaryFile, m_fileInfo.suffix());

    if(!temporaryFile.open())
    {
        cupsFreeDests(num_dests, dests);
        cupsFreeOptions(num_options, options);

        return false;
    }

    temporaryFile.close();

    if(!m_document->save(temporaryFile.fileName(), true))
    {
        cupsFreeDests(num_dests, dests);
        cupsFreeOptions(num_options, options);

        return false;
    }

    jobId = cupsPrintFile(dest->name, temporaryFile.fileName().toLocal8Bit(), m_fileInfo.completeBaseName().toLocal8Bit(), num_options, options);

    if(jobId < 1)
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

    QPainter painter;

    if(!painter.begin(printer))
    {
        return false;
    }

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
    const QRectF boundingRect = page->uncroppedBoundingRect().translated(page->pos());

    const QPointF topLeft = mapToScene(viewport()->rect().topLeft());

    left = (topLeft.x() - boundingRect.x()) / boundingRect.width();
    top = (topLeft.y() - boundingRect.y()) / boundingRect.height();
}

bool DocumentView::checkDocument(const QString& filePath, Model::Document* document, QVector< Model::Page* >& pages)
{
    if(document->isLocked())
    {
        QString password = QInputDialog::getText(this, tr("Unlock %1").arg(QFileInfo(filePath).completeBaseName()), tr("Password:"), QLineEdit::Password);

        if(document->unlock(password))
        {
            return false;
        }
    }

    const int numberOfPages = document->numberOfPages();

    if(numberOfPages == 0)
    {
        qWarning() << "No pages were found in document at" << filePath;

        return false;
    }

    pages.reserve(numberOfPages);

    for(int index = 0; index < numberOfPages; ++index)
    {
        Model::Page* page = document->page(index);

        if(page == 0)
        {
            qWarning() << "No page" << index << "was found in document at" << filePath;

            return false;
        }

        pages.append(page);
    }

    return true;
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

void DocumentView::prepareDocument(Model::Document* document, const QVector< Model::Page* >& pages)
{
    m_prefetchTimer->blockSignals(true);
    m_prefetchTimer->stop();

    cancelSearch();
    clearResults();

    qDeleteAll(m_pageItems);
    qDeleteAll(m_thumbnailItems);

    qDeleteAll(m_pages);
    m_pages = pages;

    delete m_document;
    m_document = document;

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

    const Model::Outline outline = m_document->outline();

    if(!outline.empty())
    {
        m_outlineModel.reset(new OutlineModel(outline, this));
    }
    else
    {
        m_outlineModel.reset(new FallbackOutlineModel(this));
    }

    Model::Properties properties = m_document->properties();

    addFileProperties(properties, m_fileInfo);

    m_propertiesModel.reset(new PropertiesModel(properties, this));

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

        page->setRubberBandMode(m_rubberBandMode);

        scene()->addItem(page);
        m_pageItems.append(page);

        connect(page, SIGNAL(cropRectChanged()), SLOT(on_pages_cropRectChanged()));

        connect(page, SIGNAL(linkClicked(bool,int,qreal,qreal)), SLOT(on_pages_linkClicked(bool,int,qreal,qreal)));
        connect(page, SIGNAL(linkClicked(bool,QString,int)), SLOT(on_pages_linkClicked(bool,QString,int)));
        connect(page, SIGNAL(linkClicked(QString)), SLOT(on_pages_linkClicked(QString)));

        connect(page, SIGNAL(rubberBandFinished()), SLOT(on_pages_rubberBandFinished()));

        connect(page, SIGNAL(zoomToSelection(int,QRectF)), SLOT(on_pages_zoomToSelection(int,QRectF)));
        connect(page, SIGNAL(openInSourceEditor(int,QPointF)), SLOT(on_pages_openInSourceEditor(int,QPointF)));

        connect(page, SIGNAL(wasModified()), SLOT(on_pages_wasModified()));
    }
}

void DocumentView::prepareThumbnails()
{
    m_thumbnailItems.clear();
    m_thumbnailItems.reserve(m_pages.count());

    for(int index = 0; index < m_pages.count(); ++index)
    {
        ThumbnailItem* page = new ThumbnailItem(m_pages.at(index), pageLabelFromNumber(index + 1), index);

        m_thumbnailsScene->addItem(page);
        m_thumbnailItems.append(page);

        connect(page, SIGNAL(cropRectChanged()), SLOT(on_thumbnails_cropRectChanged()));

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

        if(invertColors())
        {
            backgroundColor.setRgb(~backgroundColor.rgb());
        }
    }

    scene()->setBackgroundBrush(QBrush(backgroundColor));
    m_thumbnailsScene->setBackgroundBrush(QBrush(backgroundColor));
}

void DocumentView::prepareScene()
{
    // prepare render parameters and adjust scale factor

    RenderParam renderParam(logicalDpiX(), logicalDpiY(), 1.0,
                            scaleFactor(), rotation(), renderFlags());

#if QT_VERSION >= QT_VERSION_CHECK(5,1,0)

    if(s_settings->pageItem().useDevicePixelRatio())
    {
        renderParam.setDevicePixelRatio(devicePixelRatio());
    }

#endif // QT_VERSION

    const qreal visibleWidth = m_layout->visibleWidth(viewport()->width());
    const qreal visibleHeight = m_layout->visibleHeight(viewport()->height());

    foreach(PageItem* page, m_pageItems)
    {
        const QSizeF displayedSize = page->displayedSize(renderParam);

        if(m_scaleMode == FitToPageWidthMode)
        {
            adjustScaleFactor(renderParam, visibleWidth / displayedSize.width());
        }
        else if(m_scaleMode == FitToPageSizeMode)
        {
            adjustScaleFactor(renderParam, qMin(visibleWidth / displayedSize.width(), visibleHeight / displayedSize.height()));
        }

        page->setRenderParam(renderParam);
    }

    // prepare layout

    qreal left = 0.0;
    qreal right = 0.0;
    qreal height = s_settings->documentView().pageSpacing();

    m_layout->prepareLayout(m_pageItems, m_rightToLeftMode,
                            left, right, height);

    scene()->setSceneRect(left, 0.0, right - left, height);
}

void DocumentView::prepareView(qreal newLeft, qreal newTop, bool forceScroll, int scrollToPage)
{
    const QRectF sceneRect = scene()->sceneRect();

    qreal top = sceneRect.top();
    qreal height = sceneRect.height();

    int horizontalValue = 0;
    int verticalValue = 0;

    scrollToPage = scrollToPage != 0 ? scrollToPage : m_currentPage;

    const int highlightIsOnPage = m_currentResult.isValid() ? pageOfResult(m_currentResult) : 0;
    const bool highlightCurrentThumbnail = s_settings->documentView().highlightCurrentThumbnail();

    for(int index = 0; index < m_pageItems.count(); ++index)
    {
        PageItem* page = m_pageItems.at(index);

        if(m_continuousMode)
        {
            page->setVisible(true);
        }
        else
        {
            if(m_layout->leftIndex(index) == m_currentPage - 1)
            {
                page->setVisible(true);

                const QRectF boundingRect = page->boundingRect().translated(page->pos());

                top = boundingRect.top() - s_settings->documentView().pageSpacing();
                height = boundingRect.height() + 2.0 * s_settings->documentView().pageSpacing();
            }
            else
            {
                page->setVisible(false);

                page->cancelRender();
            }
        }

        if(index == scrollToPage - 1)
        {
            const QRectF boundingRect = page->uncroppedBoundingRect().translated(page->pos());

            horizontalValue = qFloor(boundingRect.left() + newLeft * boundingRect.width());
            verticalValue = qFloor(boundingRect.top() + newTop * boundingRect.height());
        }

        if(index == highlightIsOnPage - 1)
        {
            m_highlight->setPos(page->pos());
            m_highlight->setTransform(page->transform());

            page->stackBefore(m_highlight);
        }

        m_thumbnailItems.at(index)->setHighlighted(highlightCurrentThumbnail && (index == m_currentPage - 1));
    }

    setSceneRect(sceneRect.left(), top, sceneRect.width(), height);

    if(!forceScroll && s_settings->documentView().minimalScrolling())
    {
        setValueIfNotVisible(horizontalScrollBar(), horizontalValue);
        setValueIfNotVisible(verticalScrollBar(), verticalValue);
    }
    else
    {
        horizontalScrollBar()->setValue(horizontalValue);
        verticalScrollBar()->setValue(verticalValue);
    }

    viewport()->update();
}

void DocumentView::prepareThumbnailsScene()
{
    // prepare render parameters and adjust scale factor

    RenderParam renderParam(logicalDpiX(), logicalDpiY(), 1.0,
                            scaleFactor(), rotation(), renderFlags());

#if QT_VERSION >= QT_VERSION_CHECK(5,1,0)

    if(s_settings->pageItem().useDevicePixelRatio())
    {
        renderParam.setDevicePixelRatio(devicePixelRatio());
    }

#endif // QT_VERSION

    const qreal thumbnailSize = s_settings->documentView().thumbnailSize();
    const qreal thumbnailSpacing = s_settings->documentView().thumbnailSpacing();

    qreal visibleWidth = Defaults::DocumentView::thumbnailSize();
    qreal visibleHeight = Defaults::DocumentView::thumbnailSize();

    if(!m_thumbnailsViewportSize.isNull())
    {
        visibleWidth = m_thumbnailsViewportSize.width() - 3.0 * thumbnailSpacing;
        visibleHeight = m_thumbnailsViewportSize.height() - 3.0 * thumbnailSpacing;
    }

    foreach(ThumbnailItem* page, m_thumbnailItems)
    {
        const QSizeF displayedSize = page->displayedSize(renderParam);

        if(thumbnailSize != 0.0)
        {
            adjustScaleFactor(renderParam, qMin(thumbnailSize / displayedSize.width(), thumbnailSize / displayedSize.height()));
        }
        else
        {
            if(m_thumbnailsOrientation == Qt::Vertical)
            {
                adjustScaleFactor(renderParam, visibleWidth / displayedSize.width());
            }
            else
            {
                adjustScaleFactor(renderParam, (visibleHeight - page->textHeight()) / displayedSize.height());
            }
        }

        page->setRenderParam(renderParam);
    }

    // prepare layout

    qreal left = 0.0;
    qreal right = m_thumbnailsOrientation == Qt::Vertical ? 0.0 : thumbnailSpacing;
    qreal top = 0.0;
    qreal bottom = m_thumbnailsOrientation == Qt::Vertical ? thumbnailSpacing : 0.0;

    const bool limitThumbnailsToResults = s_settings->documentView().limitThumbnailsToResults();

    for(int index = 0; index < m_thumbnailItems.count(); ++index)
    {
        ThumbnailItem* page = m_thumbnailItems.at(index);

        // prepare visibility

        if(limitThumbnailsToResults && s_searchModel->hasResults(this) && !s_searchModel->hasResultsOnPage(this, index + 1))
        {
            page->setVisible(false);

            page->cancelRender();

            continue;
        }

        page->setVisible(true);

        // prepare position

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

    {
        VerticalScrollBarChangedBlocker verticalScrollBarChangedBlocker(this);

        centerOn(m_highlight);
    }

    viewport()->update();
}

void DocumentView::checkResult()
{
    if(m_currentResult.isValid() && m_layout->currentPage(pageOfResult(m_currentResult)) != m_currentPage)
    {
        m_currentResult = QModelIndex();
    }
}

void DocumentView::applyResult()
{
    if(m_currentResult.isValid())
    {
        const int page = pageOfResult(m_currentResult);
        const QRectF rect = rectOfResult(m_currentResult);

        jumpToPage(page);

        prepareHighlight(page - 1, rect);
    }
    else
    {
        m_highlight->setVisible(false);
    }
}

} // qpdfview
