#ifndef DOCUMENTVIEW_H
#define DOCUMENTVIEW_H

#include <QtCore>
#include <QtGui>

#include <poppler-qt4.h>

class DocumentView : public QGraphicsView
{
    Q_OBJECT
public:
    enum DisplayModes { PagingMode, ScrollingMode, DoublePagingMode, DoubleScrollingMode };
    enum ScaleModes { ScaleFactorMode, FitToPageMode, FitToPageWidthMode };

    explicit DocumentView(QWidget *parent = 0);
    ~DocumentView();

    bool load(const QString &filePath);
    bool reload();
    bool save(const QString &filePath) const;

    const QString &filePath() const { return m_filePath; }

    const int &index() const { return m_index; }

    const DisplayModes &displayMode() const { return m_displayMode; }

    const ScaleModes &scaleMode() const { return m_scaleMode; }
    const qreal &scaleFactor() const { return m_scaleFactor; }

    int pageCount() const { return m_pageList.size(); }
    Poppler::Page *page(int index) const { return m_pageList[index-1]; }
    
signals:
    void documentChanged(QString);

    void indexChanged(int);

    void displayModeChanged(DocumentView::DisplayModes);

    void scaleModeChanged(DocumentView::ScaleModes);
    void scaleFactorChanged(qreal);
    
public slots:
    void setIndex(const int &index);
    void previousPage();
    void nextPage();
    void firstPage();
    void lastPage();

    void setDisplayMode(const DisplayModes &displayMode);

    void setScaleMode(const ScaleModes &scaleModes);
    void setScaleFactor(const qreal &scaleFactor);

protected:
    void layout();

private:
    class PageItem : public QGraphicsItem
    {
    public:
        PageItem(Poppler::Page *page, QGraphicsItem *parent = 0, QGraphicsScene *scene = 0) : m_page(page) {}

        QRectF boundingRect() const { return QRectF(0.0, 0.0, 72.0 * m_page->pageSizeF().width(), 72.0 * m_page->pageSizeF().height()); }
        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) { m_page->renderToPainter(painter, 72.0, 72.0); }

    private:
        Poppler::Page *m_page;

    };

    QGraphicsScene *m_graphicsScene;

    Poppler::Document *m_document;
    QList<Poppler::Page*> m_pageList;

    QString m_filePath;

    int m_index;

    DisplayModes m_displayMode;

    ScaleModes m_scaleMode;
    qreal m_scaleFactor;

};

#endif // DOCUMENTVIEW_H
