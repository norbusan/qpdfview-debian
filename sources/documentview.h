#ifndef DOCUMENTVIEW_H
#define DOCUMENTVIEW_H

#include <QtCore>
#include <QtXml>
#include <QtGui>

#include <poppler-qt4.h>

class DocumentView : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString filePath READ filePath NOTIFY filePathChanged)
    Q_PROPERTY(int numberOfPages READ numberOfPages NOTIFY numberOfPagesChanged)
    Q_PROPERTY(int currentPage READ currentPage WRITE setCurrentPage NOTIFY currentPageChanged)
    Q_PROPERTY(PageLayout pageLayout READ pageLayout WRITE setPageLayout NOTIFY pageLayoutChanged)
    Q_PROPERTY(Scaling scaling READ scaling WRITE setScaling NOTIFY scalingChanged)
    Q_PROPERTY(Rotation rotation READ rotation WRITE setRotation NOTIFY rotationChanged)
    Q_PROPERTY(bool highlightAll READ highlightAll WRITE setHighlightAll NOTIFY highlightAllChanged)
    Q_ENUMS(PageLayout Scaling Rotation)

public:
    enum PageLayout { OnePage, TwoPages, OneColumn, TwoColumns };
    enum Scaling { FitToPage, FitToPageWidth, ScaleTo50, ScaleTo75, ScaleTo100, ScaleTo125, ScaleTo150, ScaleTo200, ScaleTo400 };
    enum Rotation { RotateBy0, RotateBy90, RotateBy180, RotateBy270 };

private:
    struct Link
    {
        QRectF area;

        int page;
        qreal top;

        QString url;

        Link() : area(), page(-1), top(0.0), url() {}
        Link(QRectF area, int page, qreal top) : area(area), page(page), top(top), url() {}
        Link(QRectF area, const QString &url) : area(area), page(-1), top(0.0), url(url) {}

    };

    struct PageCacheKey
    {
        int index;
        qreal scale;

        PageCacheKey() : index(-1), scale(1.0) {}
        PageCacheKey(int index, qreal scale) : index(index), scale(scale) {}

        bool operator<(const PageCacheKey &key) const
        {
            return (index < key.index) || (index == key.index && scale < key.scale);
        }
    };

    friend class PageItem;

    class PageItem : public QGraphicsItem
    {
        friend class DocumentView;

    public:
        explicit PageItem(QGraphicsItem* parent = 0, QGraphicsScene* scene = 0);
        ~PageItem();

        QRectF boundingRect() const;
        void paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*);

    protected:
        void hoverMoveEvent(QGraphicsSceneHoverEvent *event);
        void mousePressEvent(QGraphicsSceneMouseEvent *event);
        void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
        void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    private:
        // page

        Poppler::Page *m_page;

        // properties

        int m_index;
        qreal m_scale;

        // links

        QList<DocumentView::Link> m_links;

        // highlight

        QRectF m_highlight;
        QRectF m_rubberBand;

        // transforms

        QSizeF m_size;

        qreal m_resolutionX;
        qreal m_resolutionY;

        QTransform m_linkTransform;
        QTransform m_highlightTransform;

        // render

        QFuture<void> m_render;
        void render(bool prefetch = false);

    };

    friend class ThumbnailItem;

    class ThumbnailItem : public QGraphicsItem
    {
        friend class DocumentView;

    public:
        explicit ThumbnailItem(QGraphicsItem *parent = 0, QGraphicsScene *scene = 0);
        ~ThumbnailItem();

        QRectF boundingRect() const;
        void paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*);

    protected:
        void mousePressEvent(QGraphicsSceneMouseEvent*);

    private:
        // page

        Poppler::Page *m_page;

        // properties

        int m_index;

        // transforms

        QSizeF m_size;

        qreal m_resolutionX;
        qreal m_resolutionY;

        // render

        QFuture<void> m_render;
        void render();

    };

public:
    explicit DocumentView(QWidget *parent = 0);
    ~DocumentView();

    uint maximumPageCacheSize() const;
    void setMaximumPageCacheSize(uint maximumPageCacheSize);

    // properties

    const QString &filePath() const;
    int numberOfPages() const;

    int currentPage() const;
    void setCurrentPage(int currentPage, qreal top = 0.0);

    PageLayout pageLayout() const;
    void setPageLayout(PageLayout pageLayout);

    Scaling scaling() const;
    void setScaling(Scaling scaling);

    Rotation rotation() const;
    void setRotation(Rotation rotation);

    bool highlightAll() const;
    void setHighlightAll(bool highlightAll);

    QAction *tabAction() const;
    QTreeWidget *outlineTreeWidget() const;
    QTableWidget *metaInformationTableWidget() const;
    QGraphicsView *thumbnailsGraphicsView() const;

public slots:
    bool open(const QString &filePath);
    bool refresh();
    bool saveCopy(const QString &filePath);
    void close();

    void previousPage();
    void nextPage();
    void firstPage();
    void lastPage();

    void startSearch(const QString &text, bool matchCase = true);
    void cancelSearch();

    void findPrevious();
    void findNext();

    void startPrint(QPrinter *printer, int fromPage, int toPage);
    void cancelPrint();

signals:
    void filePathChanged(const QString &filePath);
    void numberOfPagesChanged(int numberOfPages);

    void currentPageChanged(int currentPage);
    void pageLayoutChanged(DocumentView::PageLayout pageLayout);
    void scalingChanged(DocumentView::Scaling scaling);
    void rotationChanged(DocumentView::Rotation rotation);

    void highlightAllChanged(bool highlightAll);

    void searchProgressed(int value);
    void searchCanceled();
    void searchFinished();

    void firstResultFound();

    void printProgressed(int value);
    void printCanceled();
    void printFinished();

protected:
    bool eventFilter(QObject*, QEvent *event);
    void resizeEvent(QResizeEvent* event);

    void keyPressEvent(QKeyEvent *event);
    void wheelEvent(QWheelEvent *event);

protected slots:
    void slotVerticalScrollBarValueChanged(int value);

    void slotPrefetchTimerTimeout();
    void slotTabActionTriggered();
    void slotOutlineTreeWidgetItemClicked(QTreeWidgetItem *item, int column);

private:
    // document

    Poppler::Document *m_document;
    QMutex m_documentMutex;

    // page cache

    QMap< PageCacheKey, QImage > m_pageCache;
    QMutex m_pageCacheMutex;

    uint m_pageCacheSize;
    uint m_maximumPageCacheSize;

    // properties

    QString m_filePath;
    int m_numberOfPages;

    int m_currentPage;
    PageLayout m_pageLayout;
    Scaling m_scaling;
    Rotation m_rotation;

    bool m_highlightAll;

    // settings

    QSettings m_settings;

    // graphics

    QGraphicsScene *m_scene;
    QGraphicsView *m_view;

    QMap< int, PageItem* > m_pagesByIndex;
    QMap< qreal, PageItem* > m_pagesByHeight;

    QTransform m_pageTransform;

    // miscellaneous

    QFileSystemWatcher *m_autoRefreshWatcher;
    QTimer *m_prefetchTimer;
    QAction *m_tabAction;
    QTreeWidget *m_outlineTreeWidget;
    QTableWidget *m_metaInformationTableWidget;
    QGraphicsView *m_thumbnailsGraphicsView;

    // search

    QMap< int, QRectF > m_results;
    QMutex m_resultsMutex;

    QFuture<void> m_search;
    void search(const QString &text, bool matchCase = true);

    // print

    QFuture<void> m_print;
    void print(QPrinter *printer, int fromPage, int toPage);

    // internal methods

    void preparePages();

    void prepareOutline();
    void prepareOutline(const QDomNode &node, QTreeWidgetItem *parent, QTreeWidgetItem *sibling);

    void prepareMetaInformation();

    void prepareThumbnails();

    void prepareScene();
    void prepareView(qreal top = 0.0);
    
};

#endif // DOCUMENTVIEW_H
