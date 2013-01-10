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

#include <QGraphicsView>
#include <QMap>
#include <QMutex>
#include <QStack>

class QDomNode;
class QFileSystemWatcher;
class QMutex;
class QPrinter;
class QStandardItem;
class QStandardItemModel;

#include "global.h"
#include "printoptions.h"

class Document;
class DocumentLoader;
class PageItem;
class ThumbnailItem;
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

    static QStringList openFilter();
    QStringList saveFilter() const;

    bool canSave() const;

    bool continousMode() const;
    void setContinousMode(bool continousMode);

    LayoutMode layoutMode() const;
    void setLayoutMode(LayoutMode layoutMode);

    ScaleMode scaleMode() const;
    void setScaleMode(ScaleMode scaleMode);

    qreal scaleFactor() const;
    void setScaleFactor(qreal scaleFactor);

    Rotation rotation() const;
    void setRotation(Rotation rotation);

    bool highlightAll() const;
    void setHighlightAll(bool highlightAll);

    RubberBandMode rubberBandMode() const;
    void setRubberBandMode(RubberBandMode rubberBandMode);

    bool searchWasCanceled() const;
    int searchProgress() const;

    QGraphicsScene* thumbnailsScene() const;
    const QVector< ThumbnailItem* >& thumbnails() const;

    QStandardItemModel* outlineModel() const;
    QStandardItemModel* propertiesModel() const;

    QStandardItemModel* fontsModel();

signals:
    void filePathChanged(const QString& filePath);
    void numberOfPagesChanged(int numberOfPages);
    void currentPageChanged(int currentPage, bool returnTo = false);

    void continousModeChanged(bool continousMode);
    void layoutModeChanged(LayoutMode layoutMode);
    void scaleModeChanged(ScaleMode scaleMode);
    void scaleFactorChanged(qreal scaleFactor);
    void rotationChanged(Rotation rotation);

    void highlightAllChanged(bool highlightAll);
    void rubberBandModeChanged(RubberBandMode rubberBandMode);

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

    static DocumentLoader* s_pdfDocumentLoader;
    static DocumentLoader* s_psDocumentLoader;

    bool loadPlugin(DocumentLoader*& documentLoader, const QString& name);
    Document* loadDocument(const QString& filePath);

    QFileSystemWatcher* m_autoRefreshWatcher;
    QTimer* m_autoRefreshTimer;

    QTimer* m_prefetchTimer;

    Document* m_document;

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
    Rotation m_rotation;

    bool m_highlightAll;
    RubberBandMode m_rubberBandMode;

    QGraphicsScene* m_pagesScene;
    QVector< PageItem* > m_pages;

    QMap< qreal, int > m_heightToIndex;

    QGraphicsScene* m_thumbnailsScene;
    QVector< ThumbnailItem* > m_thumbnails;

    QGraphicsRectItem* m_highlight;

    QStandardItemModel* m_outlineModel;
    QStandardItemModel* m_propertiesModel;

    void prepareDocument(Document* document);

    void preparePages();

    void prepareThumbnails();

    void prepareOutline();
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
