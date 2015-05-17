/*

Copyright 2014 S. Razi Alavizadeh
Copyright 2012-2014 Adam Reichold
Copyright 2014 Dorian Scholz

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

#include <QFileInfo>
#include <QGraphicsView>
#include <QMap>
#include <QPersistentModelIndex>

class QDomNode;
class QFileSystemWatcher;
class QPrinter;
class QStandardItemModel;

#include "renderparam.h"
#include "printoptions.h"

namespace qpdfview
{

namespace Model
{
class Annotation;
class Page;
class Document;
}

class Settings;
class PageItem;
class ThumbnailItem;
class SearchModel;
class SearchTask;
class PresentationView;
class ShortcutHandler;
struct DocumentLayout;

class DocumentView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit DocumentView(QWidget* parent = 0);
    ~DocumentView();

    const QFileInfo& fileInfo() const { return m_fileInfo; }
    bool wasModified() const { return m_wasModified; }

    int numberOfPages() const { return m_pages.count(); }
    int currentPage() const { return m_currentPage; }

    bool hasFrontMatter() const { return m_firstPage > 1; }

    int firstPage() const { return m_firstPage; }
    void setFirstPage(int firstPage);

    QString defaultPageLabelFromNumber(int number) const;
    QString pageLabelFromNumber(int number) const;
    int pageNumberFromLabel(const QString& label) const;

    QString title() const;

    static QStringList openFilter();
    QStringList saveFilter() const;

    bool canSave() const;

    bool continuousMode() const { return m_continuousMode; }
    void setContinuousMode(bool continuousMode);

    LayoutMode layoutMode() const;
    void setLayoutMode(LayoutMode layoutMode);

    bool rightToLeftMode() const { return m_rightToLeftMode; }
    void setRightToLeftMode(bool rightToLeftMode);

    ScaleMode scaleMode() const { return m_scaleMode; }
    void setScaleMode(ScaleMode scaleMode);

    qreal scaleFactor() const { return m_scaleFactor; }
    void setScaleFactor(qreal scaleFactor);

    Rotation rotation() const { return m_rotation; }
    void setRotation(Rotation rotation);

    qpdfview::RenderFlags renderFlags() const;
    void setRenderFlags(qpdfview::RenderFlags flags);

    bool invertColors() const { return m_invertColors; }
    void setInvertColors(bool invertColors);

    bool convertToGrayscale() const { return m_convertToGrayscale; }
    void setConvertToGrayscale(bool convertToGrayscale);

    bool trimMargins() const { return m_trimMargins; }
    void setTrimMargins(bool trimMargins);

    bool highlightAll() const { return m_highlightAll; }
    void setHighlightAll(bool highlightAll);

    RubberBandMode rubberBandMode() const { return m_rubberBandMode; }
    void setRubberBandMode(RubberBandMode rubberBandMode);

    bool searchWasCanceled() const;
    int searchProgress() const;

    Qt::Orientation thumbnailsOrientation() const { return m_thumbnailsOrientation; }
    void setThumbnailsOrientation(Qt::Orientation thumbnailsOrientation);

    const QVector< ThumbnailItem* >& thumbnailItems() const { return m_thumbnailItems; }
    QGraphicsScene* thumbnailsScene() const { return m_thumbnailsScene; }

    QStandardItemModel* outlineModel() const { return m_outlineModel; }
    QStandardItemModel* propertiesModel() const { return m_propertiesModel; }

    QStandardItemModel* fontsModel() const;

    QString searchText() const;
    bool searchMatchCase() const;
    bool searchWholeWords() const;

    QPair< QString, QString > searchContext(int page, const QRectF& rect) const;

signals:
    void documentChanged();
    void documentModified();

    void numberOfPagesChanged(int numberOfPages);
    void currentPageChanged(int currentPage, bool trackChange = false);

    void canJumpChanged(bool backward, bool forward);

    void continuousModeChanged(bool continuousMode);
    void layoutModeChanged(LayoutMode layoutMode);
    void rightToLeftModeChanged(bool rightToLeftMode);
    void scaleModeChanged(ScaleMode scaleMode);
    void scaleFactorChanged(qreal scaleFactor);
    void rotationChanged(Rotation rotation);

    void linkClicked(int page);
    void linkClicked(bool newTab, const QString& filePath, int page);

    void invertColorsChanged(bool invertColors);
    void convertToGrayscaleChanged(bool convertToGrayscale);
    void trimMarginsChanged(bool trimMargins);

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

    void jumpToPage(int page, bool trackChange = true, qreal newLeft = qQNaN(), qreal newTop = qQNaN());

    bool canJumpBackward() const;
    void jumpBackward();

    bool canJumpForward() const;
    void jumpForward();

    void temporaryHighlight(int page, const QRectF& highlight);

    void startSearch(const QString& text, bool matchCase, bool wholeWords);
    void cancelSearch();

    void clearResults();

    void findPrevious();
    void findNext();
    void findResult(const QModelIndex& index);

    void zoomIn();
    void zoomOut();
    void originalSize();

    void rotateLeft();
    void rotateRight();

    void startPresentation();

protected slots:
    void on_verticalScrollBar_valueChanged();

    void on_autoRefresh_timeout();
    void on_prefetch_timeout();

    void on_temporaryHighlight_timeout();

    void on_searchTask_progressChanged(int progress);
    void on_searchTask_resultsReady(int index, const QList< QRectF >& results);

    void on_pages_cropRectChanged();
    void on_thumbnails_cropRectChanged();

    void on_pages_linkClicked(bool newTab, int page, qreal left, qreal top);
    void on_pages_linkClicked(bool newTab, const QString& fileName, int page);
    void on_pages_linkClicked(const QString& url);

    void on_pages_rubberBandFinished();

    void on_pages_editSourceRequested(int page, const QPointF& pos);
    void on_pages_zoomToSelectionRequested(int page, const QRectF& rect);

    void on_pages_wasModified();

protected:
    void resizeEvent(QResizeEvent* event);

    void keyPressEvent(QKeyEvent* event);
    void mousePressEvent(QMouseEvent* event);
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
    QVector< Model::Page* > m_pages;

    QFileInfo m_fileInfo;
    bool m_wasModified;

    int m_currentPage;
    int m_firstPage;

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
    bool m_rightToLeftMode;
    ScaleMode m_scaleMode;
    qreal m_scaleFactor;
    Rotation m_rotation;

    bool m_invertColors;
    bool m_convertToGrayscale;
    bool m_trimMargins;

    bool m_highlightAll;
    RubberBandMode m_rubberBandMode;

    QVector< PageItem* > m_pageItems;
    QVector< ThumbnailItem* > m_thumbnailItems;

    QGraphicsRectItem* m_highlight;

    Qt::Orientation m_thumbnailsOrientation;
    QGraphicsScene* m_thumbnailsScene;

    QStandardItemModel* m_outlineModel;
    QStandardItemModel* m_propertiesModel;

    bool checkDocument(const QString& filePath, Model::Document* document, QVector< Model::Page* >& pages);

    void loadFallbackOutline();
    void loadDocumentDefaults();

    void adjustScrollBarPolicy();
    void connectVerticalScrollBar();
    void disconnectVerticalScrollBar();

    void prepareDocument(Model::Document* document, const QVector< Model::Page* >& pages);
    void preparePages();
    void prepareThumbnails();
    void prepareBackground();

    void prepareScene();
    void prepareView(qreal newLeft = 0.0, qreal newTop = 0.0, bool forceScroll = true, int scrollToPage = 0);

    void prepareThumbnailsScene();

    void prepareHighlight(int index, const QRectF& highlight);

    // search

    static SearchModel* s_searchModel;

    QPersistentModelIndex m_currentResult;

    SearchTask* m_searchTask;

    void checkResult();
    void applyResult();

};

} // qpdfview

#endif // DOCUMENTVIEW_H
