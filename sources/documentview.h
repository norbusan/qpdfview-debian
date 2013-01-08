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

#ifndef DOCUMENTVIEW_H
#define DOCUMENTVIEW_H

#include <QtCore>
#include <QtXml>
#include <QtGui>

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

#include <QtWidgets>
#include <QtPrintSupport>

#endif // QT_VERSION

#include <poppler-qt4.h>

#include "pageitem.h"

class SearchThread;
class PresentationView;

class DocumentView : public QGraphicsView
{
    Q_OBJECT

public:
    static bool openUrl();
    static void setOpenUrl(bool openUrl);

    static bool autoRefresh();
    static void setAutoRefresh(bool autoRefresh);

    static bool antialiasing();
    static void setAntialiasing(bool antialiasing);

    static bool textAntialiasing();
    static void setTextAntialiasing(bool textAntialiasing);

    static bool textHinting();
    static void setTextHinting(bool textHinting);

    static bool overprintPreview();
    static void setOverprintPreview(bool overprintPreview);

    static bool prefetch();
    static void setPrefetch(bool prefetch);

    static int pagesPerRow();
    static void setPagesPerRow(int pagesPerRow);

    static qreal pageSpacing();
    static void setPageSpacing(qreal pageSpacing);

    static qreal thumbnailSpacing();
    static void setThumbnailSpacing(qreal thumbnailSpacing);

    static qreal thumbnailSize();
    static void setThumbnailSize(qreal thumbnailSize);

    static qreal minimumScaleFactor();
    static qreal maximumScaleFactor();
    static qreal zoomBy();

    static const Qt::KeyboardModifiers& zoomModifiers();
    static void setZoomModifiers(const Qt::KeyboardModifiers& zoomModifiers);

    static const Qt::KeyboardModifiers& rotateModifiers();
    static void setRotateModifiers(const Qt::KeyboardModifiers& rotateModifiers);

    static const Qt::KeyboardModifiers& scrollModifiers();
    static void setScrollModifiers(const Qt::KeyboardModifiers& scrollModifiers);

    static int highlightDuration();
    static void setHighlightDuration(int highlightDuration);

    static const QString& sourceEditor();
    static void setSourceEditor(const QString& sourceEditor);

    explicit DocumentView(QWidget* parent = 0);
    ~DocumentView();

    const QString& filePath() const;
    int numberOfPages() const;
    int currentPage() const;

    bool continousMode() const;
    void setContinousMode(bool continousMode);

    enum LayoutMode
    {
        SinglePageMode = 0,
        TwoPagesMode = 1,
        TwoPagesWithCoverPageMode = 2,
        MultiplePagesMode = 3,
        NumberOfLayoutModes = 4
    };

    LayoutMode layoutMode() const;
    void setLayoutMode(LayoutMode layoutMode);

    enum ScaleMode
    {
        ScaleFactor = 0,
        FitToPageWidth = 1,
        FitToPageSize = 2,
        NumberOfScaleModes = 3
    };

    ScaleMode scaleMode() const;
    void setScaleMode(ScaleMode scaleMode);

    qreal scaleFactor() const;
    void setScaleFactor(qreal scaleFactor);

    Poppler::Page::Rotation rotation() const;
    void setRotation(Poppler::Page::Rotation rotation);

    bool highlightAll() const;
    void setHighlightAll(bool highlightAll);

    PageItem::RubberBandMode rubberBandMode() const;
    void setRubberBandMode(PageItem::RubberBandMode rubberBandMode);

    bool searchWasCanceled() const;
    int searchProgress() const;

    QGraphicsScene* thumbnailsScene() const;
    const QVector< ThumbnailItem* >& thumbnails() const;

    QStandardItemModel* outlineModel() const;
    QStandardItemModel* propertiesModel() const;

    QStandardItemModel* fontsModel();

    struct PrintOptions
    {
        bool fitToPage;
        bool landscape;

        QString pageRanges;

        enum PageSet
        {
            AllPages = 0,
            EvenPages = 1,
            OddPages = 2
        };

        PageSet pageSet;

        enum NumberUp
        {
            SinglePage = 0,
            TwoPages = 1,
            FourPages = 2,
            SixPages = 3,
            NinePages = 4,
            SixteenPages = 5
        };

        NumberUp numberUp;

        enum NumberUpLayout
        {
            BottomTopLeftRight = 0,
            BottomTopRightLeft = 1,
            LeftRightBottomTop = 2,
            LeftRightTopBottom = 3,
            RightLeftBottomTop = 4,
            RightLeftTopBottom = 5,
            TopBottomLeftRight = 6,
            TopBottomRightLeft = 7
        };

        NumberUpLayout numberUpLayout;

        PrintOptions() : fitToPage(false), landscape(false), pageRanges(), pageSet(AllPages), numberUp(SinglePage), numberUpLayout(LeftRightTopBottom) {}

    };

signals:
    void filePathChanged(const QString& filePath);
    void numberOfPagesChanged(int numberOfPages);
    void currentPageChanged(int currentPage, bool returnTo = false);

    void continousModeChanged(bool continousMode);
    void layoutModeChanged(DocumentView::LayoutMode layoutMode);
    void scaleModeChanged(DocumentView::ScaleMode scaleMode);
    void scaleFactorChanged(qreal scaleFactor);
    void rotationChanged(Poppler::Page::Rotation rotation);

    void highlightAllChanged(bool highlightAll);
    void rubberBandModeChanged(PageItem::RubberBandMode rubberBandMode);

    void searchProgressed(int progress);
    void searchFinished();
    void searchCanceled();

public slots:
    void show();

    bool open(const QString& filePath);
    bool refresh();
    bool save(const QString& filePath, bool withChanges);
    bool print(QPrinter* printer, const PrintOptions& printOptions = PrintOptions());

    void previousPage();
    void nextPage();
    void firstPage();
    void lastPage();

    void jumpToPage(int page, bool returnTo = true, qreal changeLeft = 0.0, qreal changeTop = 0.0);
    void jumpToHighlight(const QRectF& highlight);

    void startSearch(const QString& text, bool matchCase = true);
    void cancelSearch();

    void findPrevious();
    void findNext();

    void zoomIn();
    void zoomOut();
    void originalSize();

    void rotateLeft();
    void rotateRight();

    void presentation(bool sync = false, int screen = -1);

protected slots:
    void on_verticalScrollBar_valueChanged(int value);

    void on_searchThread_resultsReady(int index, QList< QRectF > results);

    void on_prefetch_timeout();

    void on_pages_linkClicked(int page, qreal left, qreal top);
    void on_pages_linkClicked(const QString& url);

    void on_pages_rubberBandFinished();

    void on_pages_sourceRequested(int page, const QPointF& pos);

protected:
    void resizeEvent(QResizeEvent* event);

    void keyPressEvent(QKeyEvent* event);
    void wheelEvent(QWheelEvent* event);

    void contextMenuEvent(QContextMenuEvent* event);

private:
    static bool s_openUrl;

    static bool s_autoRefresh;

    static bool s_antialiasing;
    static bool s_textAntialiasing;
    static bool s_textHinting;

    static bool s_overprintPreview;

    static bool s_prefetch;

    static int s_pagesPerRow;

    static qreal s_pageSpacing;
    static qreal s_thumbnailSpacing;

    static qreal s_thumbnailSize;

    static qreal s_minimumScaleFactor;
    static qreal s_maximumScaleFactor;
    static qreal s_zoomBy;

    static Qt::KeyboardModifiers s_zoomModifiers;
    static Qt::KeyboardModifiers s_rotateModifiers;
    static Qt::KeyboardModifiers s_scrollModifiers;

    static int s_highlightDuration;

    static QString s_sourceEditor;

    QFileSystemWatcher* m_autoRefreshWatcher;
    QTimer* m_autoRefreshTimer;

    QTimer* m_prefetchTimer;

    QMutex m_mutex;
    Poppler::Document* m_document;

    QString m_filePath;
    int m_numberOfPages;
    int m_currentPage;

    QStack< int > m_returnToPage;
    QStack< qreal > m_returnToLeft;
    QStack< qreal > m_returnToTop;

    int currentPageForPage(int page) const;

    int leftIndexForIndex(int index) const;
    int rightIndexForIndex(int index) const;

    void saveLeftAndTop(qreal& left, qreal& top) const;

    bool m_continuousMode;
    LayoutMode m_layoutMode;
    ScaleMode m_scaleMode;
    qreal m_scaleFactor;
    Poppler::Page::Rotation m_rotation;

    bool m_highlightAll;
    PageItem::RubberBandMode m_rubberBandMode;

    QGraphicsScene* m_pagesScene;
    QVector< PageItem* > m_pages;

    QMap< qreal, int > m_heightToIndex;

    QGraphicsScene* m_thumbnailsScene;
    QVector< ThumbnailItem* > m_thumbnails;

    QGraphicsRectItem* m_highlight;

    QStandardItemModel* m_outlineModel;
    QStandardItemModel* m_propertiesModel;

    void prepareDocument(Poppler::Document* document);

    void preparePages();

    void prepareThumbnails();

    void prepareOutline();
    void prepareOutline(const QDomNode& node, QStandardItem* parent);

    void prepareProperties();

    void prepareScene();
    void prepareView(qreal changeLeft = 0.0, qreal changeTop = 0.0);

    void prepareHighlight();

    // search

    QMultiMap< int, QRectF > m_results;
    QMultiMap< int, QRectF >::iterator m_currentResult;

    SearchThread* m_searchThread;

};

#endif // DOCUMENTVIEW_H
