#ifndef DOCUMENTVIEW_H
#define DOCUMENTVIEW_H

#include <QtCore>
#include <QtGui>

#ifndef QPDFVIEW_ENUMS
#define QPDFVIEW_ENUMS

enum PageLayout { OnePage, TwoPages, OneColumn, TwoColumns };
enum Scaling { FitToPage, FitToPageWidth, ScaleTo50, ScaleTo75, ScaleTo100, ScaleTo125, ScaleTo150, ScaleTo200, ScaleTo400 };
enum Rotation { RotateBy0, RotateBy90, RotateBy180, RotateBy270 };

#endif

class Document;
class PageView;
class AuxiliaryView;
class OutlineView;
class ThumbnailsView;
class PresentationView;
class BookmarksMenu;

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

    friend class AuxiliaryView;
    friend class OutlineView;
    friend class ThumbnailsView;
    friend class PresentationView;

public:
    explicit DocumentView(QWidget *parent = 0);

    // properties

    const QString &filePath() const;
    int numberOfPages() const;

    int currentPage() const;

    PageLayout pageLayout() const;
    void setPageLayout(PageLayout pageLayout);

    Scaling scaling() const;
    void setScaling(Scaling scaling);

    Rotation rotation() const;
    void setRotation(Rotation rotation);

    bool highlightAll() const;
    void setHighlightAll(bool highlightAll);

    QAction *makeCurrentTabAction() const;

public slots:
    bool open(const QString &filePath);
    bool refresh();
    bool saveCopy(const QString &filePath);

    void prefetch();

    void startSearch(const QString &text, bool matchCase = true);
    void cancelSearch();

    void startPrint(QPrinter *printer, int fromPage, int toPage);
    void cancelPrint();

    void setCurrentPage(int currentPage, qreal top = 0.0);

    void previousPage();
    void nextPage();
    void firstPage();
    void lastPage();

    void findPrevious();
    void findNext();

signals:
    void filePathChanged(QString filePath);
    void numberOfPagesChanged(int numberOfPages);

    void currentPageChanged(int currentPage);
    void pageLayoutChanged(PageLayout);
    void scalingChanged(Scaling);
    void rotationChanged(Rotation);
    void highlightAllChanged(bool);

    void searchProgressed(int value);
    void searchCanceled();
    void searchFinished();

    void printProgressed(int value);
    void printCanceled();
    void printFinished();

protected:
    bool eventFilter(QObject*, QEvent *event);
    void resizeEvent(QResizeEvent* event);
    void wheelEvent(QWheelEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);

protected slots:
    void slotFilePathChanged(const QString &filePath);
    void slotNumberOfPagesChanged(int numberOfPages);

    void slotDocumentChanged();

    void slotPageSearched(int index);

    void slotSearchProgressed(int value);
    void slotSearchCanceled();
    void slotSearchFinished();

    void slotPrintProgressed(int value);
    void slotPrintCanceled();
    void slotPrintFinished();

    void slotVerticalScrollBarValueChanged(int value);

    void slotMakeCurrentTab();

    void slotBookmarksEntrySelected(int page, qreal top);

    void slotLinkLeftClicked(int page, qreal top);
    void slotLinkLeftClicked(const QString &url);
    void slotLinkMiddleClicked(int page, qreal top);
    void slotLinkMiddleClicked(const QString &url);

private:
    Document *m_document;

    // properties

    int m_currentPage;
    PageLayout m_pageLayout;
    Scaling m_scaling;
    Rotation m_rotation;
    bool m_highlightAll;

    // settings

    QSettings m_settings;

    // graphics

    QGraphicsView *m_view;
    QGraphicsScene *m_scene;

    QMap<int, PageView*> m_pageToPageView;
    QMap<int, int> m_heightToPage;

    QGraphicsRectItem *m_highlight;

    QMap<int, QRectF> m_searchResults;
    QMap<int, QRectF>::iterator m_searchPosition;

    QTimer *m_prefetchTimer;

    QAction *m_makeCurrentTabAction;

    BookmarksMenu *m_bookmarksMenu;

    void prepareScene();
    void prepareView(qreal top = 0.0);
    
};

#endif // DOCUMENTVIEW_H
