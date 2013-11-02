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

class QDomNode;
class QFileSystemWatcher;
class QPrinter;
class QStandardItemModel;

#include "global.h"
#include "printoptions.h"

namespace Model
{
class Annotation;
class Page;
class Document;
}

class Settings;
class PageItem;
class ThumbnailItem;
class SearchTask;
class PresentationView;
class ShortcutHandler;
class DocumentLayout;

class DocumentView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit DocumentView(QWidget* parent = 0);
    ~DocumentView();

    const QString& filePath() const;
    int numberOfPages() const;
    int currentPage() const;

    bool wasModified() const;

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

    Qt::Orientation thumbnailsOrientation() const;
    void setThumbnailsOrientation(Qt::Orientation thumbnailsOrientation);

    const QVector< ThumbnailItem* >& thumbnailItems() const;
    QGraphicsScene* thumbnailsScene() const;

    QStandardItemModel* outlineModel() const;
    QStandardItemModel* propertiesModel() const;

    QStandardItemModel* fontsModel() const;

signals:
    void documentChanged();

    void filePathChanged(const QString& filePath);
    void numberOfPagesChanged(int numberOfPages);
    void currentPageChanged(int currentPage, bool trackChange = false);

    void canJumpChanged(bool backward, bool forward);

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

    void jumpToPage(int page, bool trackChange = true, qreal changeLeft = 0.0, qreal changeTop = 0.0);

    bool canJumpBackward() const;
    void jumpBackward();

    bool canJumpForward() const;
    void jumpForward();

    void temporaryHighlight(int page, const QRectF& highlight);

    void startSearch(const QString& text, bool matchCase = true);
    void cancelSearch();

    void findPrevious();
    void findNext();

    void zoomIn();
    void zoomOut();
    void originalSize();

    void rotateLeft();
    void rotateRight();

    void startPresentation();

protected slots:
    void on_verticalScrollBar_valueChanged(int value);

    void on_prefetch_timeout();

    void on_temporaryHighlight_timeout();

    void on_searchTask_resultsReady(int index, QList< QRectF > results);

    void on_pages_linkClicked(int page, qreal left, qreal top);
    void on_pages_linkClicked(const QString& url);
    void on_pages_linkClicked(const QString& fileName, int page);

    void on_pages_rubberBandFinished();

    void on_pages_sourceRequested(int page, const QPointF& pos);

    void on_pages_wasModified();

protected:
    void resizeEvent(QResizeEvent* event);

    void keyPressEvent(QKeyEvent* event);
    void wheelEvent(QWheelEvent* event);

    void contextMenuEvent(QContextMenuEvent* event);

private:
    Q_DISABLE_COPY(DocumentView)

    static Settings* s_settings;
    static ShortcutHandler* s_shortcutHandler;

    QFileSystemWatcher* m_autoRefreshWatcher;
    QTimer* m_autoRefreshTimer;

    QTimer* m_prefetchTimer;

    Model::Document* m_document;
    QList< Model::Page* > m_pages;

    QString m_filePath;
    int m_currentPage;

    bool m_wasModified;

#ifdef WITH_CUPS

    bool printUsingCUPS(QPrinter* printer, const PrintOptions& printOptions, int fromPage, int toPage);

#endif // WITH_CUPS

    bool printUsingQt(QPrinter* printer, const PrintOptions& printOptions, int fromPage, int toPage);

    struct Position
    {
        int page;
        qreal left;
        qreal top;

        Position(int page, qreal left, qreal top) : page(page), left(left), top(top) {}

    };

    QList< Position > m_past;
    QList< Position > m_future;

    void saveLeftAndTop(qreal& left, qreal& top) const;

    QScopedPointer< DocumentLayout > m_layout;

    bool m_continuousMode;
    ScaleMode m_scaleMode;
    qreal m_scaleFactor;
    Rotation m_rotation;

    bool m_invertColors;
    bool m_highlightAll;
    RubberBandMode m_rubberBandMode;

    QVector< PageItem* > m_pageItems;
    QVector< ThumbnailItem* > m_thumbnailItems;

    QMap< qreal, int > m_heightToIndex;

    QGraphicsRectItem* m_highlight;

    Qt::Orientation m_thumbnailsOrientation;
    QGraphicsScene* m_thumbnailsScene;

    QStandardItemModel* m_outlineModel;
    QStandardItemModel* m_propertiesModel;

    void loadFallbackOutline();

    void prepareDocument(Model::Document* document);
    void preparePages();
    void prepareThumbnails();
    void prepareBackground();

    void prepareScene();
    void prepareView(qreal changeLeft = 0.0, qreal changeTop = 0.0);

    void prepareThumbnailsScene();

    void prepareHighlight(int index, const QRectF& highlight);

    // search

    typedef QMultiMap< int, QRectF > Results;

    Results m_results;
    Results::iterator m_currentResult;

    Results::iterator previousResult(const Results::iterator& result);

    SearchTask* m_searchTask;

};

#endif // DOCUMENTVIEW_H
