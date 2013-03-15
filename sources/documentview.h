/*

Copyright 2012-2013 Adam Reichold

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
#include <QStack>

class QDomNode;
class QFileSystemWatcher;
class QPrinter;
class QStandardItemModel;

#include "global.h"
#include "printoptions.h"

namespace Model
{
class Page;
class Document;
}

class PageItem;
class ThumbnailItem;
class SearchTask;
class PresentationView;

class DocumentView : public QGraphicsView
{
    Q_OBJECT

public:
    static bool openUrl();
    static void setOpenUrl(bool openUrl);

    static bool autoRefresh();
    static void setAutoRefresh(bool autoRefresh);

    static bool prefetch();
    static void setPrefetch(bool prefetch);

    static int prefetchDistance();
    static void setPrefetchDistance(int prefetchDistance);

    static int pagesPerRow();
    static void setPagesPerRow(int pagesPerRow);

    static bool limitThumbnailsToResults();
    static void setLimitThumbnailsToResults(bool limitThumbnailsToResults);

    static qreal pageSpacing();
    static void setPageSpacing(qreal pageSpacing);

    static qreal thumbnailSpacing();
    static void setThumbnailSpacing(qreal thumbnailSpacing);

    static qreal thumbnailSize();
    static void setThumbnailSize(qreal thumbnailSize);

    static qreal minimumScaleFactor();
    static qreal maximumScaleFactor();

    static const QKeySequence& skipBackwardShortcut();
    static void setSkipBackwardShortcut(const QKeySequence& shortcut);

    static const QKeySequence& skipForwardShortcut();
    static void setSkipForwardShortcut(const QKeySequence& shortcut);

    static const QKeySequence& moveUpShortcut();
    static void setMoveUpShortcut(const QKeySequence& shortcut);

    static const QKeySequence& moveDownShortcut();
    static void setMoveDownShortcut(const QKeySequence& shortcut);

    static const QKeySequence& moveLeftShortcut();
    static void setMoveLeftShortcut(const QKeySequence& shortcut);

    static const QKeySequence& moveRightShortcut();
    static void setMoveRightShortcut(const QKeySequence& shortcut);

    static const QKeySequence& returnToPageShortcut();
    static void setReturnToPageShortcut(const QKeySequence& shortcut);

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

    const QVector< int >& visitedPages() const;

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

    bool invertColors() const;
    void setInvertColors(bool invertColors);

    bool highlightAll() const;
    void setHighlightAll(bool highlightAll);

    RubberBandMode rubberBandMode() const;
    void setRubberBandMode(RubberBandMode rubberBandMode);

    bool searchWasCanceled() const;
    int searchProgress() const;

    const QVector< ThumbnailItem* >& thumbnailItems() const;
    QGraphicsScene* thumbnailsScene() const;

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

    void linkClicked(const QString& filePath, int page);

    void invertColorsChanged(bool invertColors);
    void highlightAllChanged(bool highlightAll);
    void rubberBandModeChanged(RubberBandMode rubberBandMode);

    void searchFinished();
    void searchProgressChanged(int progress);

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

    void returnToPage();

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

    void on_searchTask_resultsReady(int index, QList< QRectF > results);

    void on_prefetch_timeout();

    void on_pages_linkClicked(int page, qreal left, qreal top);
    void on_pages_linkClicked(const QString& url);
    void on_pages_linkClicked(const QString& fileName, int page);

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

    static bool s_prefetch;
    static int s_prefetchDistance;

    static int s_pagesPerRow;

    static bool s_limitThumbnailsToResults;

    static qreal s_pageSpacing;
    static qreal s_thumbnailSpacing;

    static qreal s_thumbnailSize;

    static qreal s_minimumScaleFactor;
    static qreal s_maximumScaleFactor;

    static qreal s_zoomBy;

    static QKeySequence s_skipBackwardShortcut;
    static QKeySequence s_skipForwardShortcut;

    static QKeySequence s_moveUpShortcut;
    static QKeySequence s_moveDownShortcut;
    static QKeySequence s_moveLeftShortcut;
    static QKeySequence s_moveRightShortcut;

    static QKeySequence s_returnToPageShortcut;

    static Qt::KeyboardModifiers s_zoomModifiers;
    static Qt::KeyboardModifiers s_rotateModifiers;
    static Qt::KeyboardModifiers s_scrollModifiers;

    static int s_highlightDuration;

    static QString s_sourceEditor;

    QFileSystemWatcher* m_autoRefreshWatcher;
    QTimer* m_autoRefreshTimer;

    QTimer* m_prefetchTimer;

    Model::Document* m_document;

    QString m_filePath;
    int m_numberOfPages;
    int m_currentPage;

#ifdef WITH_CUPS

    bool printUsingCUPS(QPrinter* printer, const PrintOptions& printOptions);

#endif // WITH_CUPS

    bool printUsingQt(QPrinter* printer, const PrintOptions& printOptions);

    QStack< int > m_visitedPages;
    QStack< qreal > m_leftOfVisitedPages;
    QStack< qreal > m_topOfVisitedPages;

    int currentPageForPage(int page) const;

    int leftIndexForIndex(int index) const;
    int rightIndexForIndex(int index) const;

    void saveLeftAndTop(qreal& left, qreal& top) const;

    bool m_continuousMode;
    LayoutMode m_layoutMode;
    ScaleMode m_scaleMode;
    qreal m_scaleFactor;
    Rotation m_rotation;

    bool m_invertColors;
    bool m_highlightAll;
    RubberBandMode m_rubberBandMode;

    QVector< Model::Page* > m_pages;

    QVector< PageItem* > m_pageItems;
    QVector< ThumbnailItem* > m_thumbnailItems;

    QMap< qreal, int > m_heightToIndex;

    QGraphicsRectItem* m_highlight;

    QGraphicsScene* m_thumbnailsScene;

    QStandardItemModel* m_outlineModel;
    QStandardItemModel* m_propertiesModel;

    void prepareDocument(Model::Document* document);
    void preparePages();
    void prepareThumbnails();
    void prepareBackground();

    void prepareScene();
    void prepareView(qreal changeLeft = 0.0, qreal changeTop = 0.0);

    void prepareThumbnailsScene();

    void prepareHighlight();

    // search

    QMultiMap< int, QRectF > m_results;
    QMultiMap< int, QRectF >::iterator m_currentResult;

    SearchTask* m_searchTask;

};

#endif // DOCUMENTVIEW_H
