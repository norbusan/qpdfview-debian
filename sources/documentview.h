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

#include <poppler-qt4.h>

#ifdef WITH_CUPS

#include <cups/cups.h>

#endif // WITH_CUPS

#include "pageitem.h"
#include "presentationview.h"

class DocumentView : public QGraphicsView
{
    Q_OBJECT

public:
    static qreal pageSpacing();
    static void setPageSpacing(qreal pageSpacing);

    static qreal thumbnailSpacing();
    static void setThumbnailSpacing(qreal thumbnailSpacing);

    static qreal thumbnailSize();
    static void setThumbnailSize(qreal thumbnailSize);

    static qreal minimumScaleFactor();
    static qreal maximumScaleFactor();

    explicit DocumentView(QWidget* parent = 0);
    ~DocumentView();

    const QString& filePath() const;
    int numberOfPages() const;
    int currentPage() const;

    bool continousMode() const;
    void setContinousMode(bool continousMode);

    bool twoPagesMode() const;
    void setTwoPagesMode(bool twoPagesMode);

    enum ScaleMode
    {
        ScaleFactor = 0,
        FitToPageWidth = 1,
        FitToPageSize = 2
    };

    ScaleMode scaleMode() const;
    void setScaleMode(ScaleMode scaleMode);

    qreal scaleFactor() const;
    void setScaleFactor(qreal scaleFactor);

    Poppler::Page::Rotation rotation() const;
    void setRotation(Poppler::Page::Rotation rotation);

    bool highlightAll() const;
    void setHighlightAll(bool highlightAll);

    QGraphicsScene* thumbnailsScene() const;
    QGraphicsItem* thumbnailsItem(int page) const;

    QStandardItemModel* outlineModel() const;
    QStandardItemModel* propertiesModel() const;

    QStandardItemModel* fontsModel();

signals:
    void filePathChanged(const QString& filePath);
    void numberOfPagesChanged(int numberOfPages);
    void currentPageChanged(int currentPage);

    void continousModeChanged(bool continousMode);
    void twoPagesModeChanged(bool twoPagesMode);
    void scaleModeChanged(DocumentView::ScaleMode scaleMode);
    void scaleFactorChanged(qreal scaleFactor);
    void rotationChanged(Poppler::Page::Rotation rotation);

    void highlightAllChanged(bool highlightAll);

    void searchProgressed(int progress);
    void searchFinished();
    void searchCanceled();

public slots:
    bool open(const QString& filePath);
    bool refresh();
    bool saveCopy(const QString& filePath);
    bool print(QPrinter* printer);

    void previousPage();
    void nextPage();
    void firstPage();
    void lastPage();

    void jumpToPage(int page, qreal changeLeft = 0.0, qreal changeTop = 0.0);

    void startSearch(const QString& text, bool matchCase);
    void cancelSearch();

    void findPrevious();
    void findNext();

    void zoomIn();
    void zoomOut();
    void originalSize();

    void rotateLeft();
    void rotateRight();

    void presentation();

protected slots:
    void on_verticalScrollBar_valueChanged(int value);

    void on_prefetch_timeout();

    void on_pages_linkClicked(int page, qreal left, qreal top);
    void on_pages_linkClicked(const QString& url);

    void on_thumbnails_pageClicked(int page);

    void on_search_resultReadyAt(int resultIndex);
    void on_search_progressValueChanged(int progressValue);

protected:
    void showEvent(QShowEvent* event);
    void resizeEvent(QResizeEvent* event);

    void contextMenuEvent(QContextMenuEvent* event);

    void keyPressEvent(QKeyEvent* event);
    void wheelEvent(QWheelEvent* event);

private:
    static qreal s_pageSpacing;
    static qreal s_thumbnailSpacing;

    static qreal s_thumbnailSize;

    static qreal s_minimumScaleFactor;
    static qreal s_maximumScaleFactor;

    QSettings* m_settings;

    QTimer* m_prefetchTimer;

    QFileSystemWatcher* m_autoRefreshWatcher;
    QTimer* m_autoRefreshTimer;

    QMutex m_mutex;
    Poppler::Document* m_document;

    QString m_filePath;
    int m_numberOfPages;
    int m_currentPage;

    int m_returnToPage;
    qreal m_returnToLeft;
    qreal m_returnToTop;

    bool currentPageIsLastPage();
    void saveLeftAndTop(qreal& left, qreal& top);

    bool m_continuousMode;
    bool m_twoPagesMode;
    ScaleMode m_scaleMode;
    qreal m_scaleFactor;
    Poppler::Page::Rotation m_rotation;

    bool m_highlightAll;

    QGraphicsScene* m_pagesScene;
    QList< PageItem* > m_pages;

    QMap< qreal, int > m_heightToIndex;

    QGraphicsScene* m_thumbnailsScene;
    QList< ThumbnailItem* > m_thumbnails;

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
    QMultiMap< int, QRectF >::const_iterator m_currentResult;

    QFutureWatcher< QPair< int, QList< QRectF > > >* m_search;
    class Search
    {
    public:
        Search(QMutex* mutex, Poppler::Document* document, const QString& text, bool matchCase);

        typedef QPair< int, QList< QRectF > > result_type;
        QPair< int, QList< QRectF > > operator()(int index);

    private:
        QMutex* m_mutex;
        Poppler::Document* m_document;

        QString m_text;
        bool m_matchCase;

    };

};

#endif // DOCUMENTVIEW_H
