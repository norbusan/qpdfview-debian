#ifndef PAGEOBJECT_H
#define PAGEOBJECT_H

#include <QtCore>
#include <QtGui>

#include <poppler-qt4.h>

class PageObject : public QGraphicsObject
{
    Q_OBJECT
    Q_PROPERTY(qreal resolutionX READ resolutionX WRITE setResolutionX NOTIFY resolutionXChanged)
    Q_PROPERTY(qreal resolutionY READ resolutionY WRITE setResolutionY NOTIFY resolutionYChanged)
    Q_PROPERTY(uint rotation READ rotation WRITE setRotation NOTIFY rotationChanged)
    Q_PROPERTY(QString filePath READ filePath WRITE setFilePath NOTIFY filePathChanged)
    Q_PROPERTY(int currentPage READ currentPage WRITE setCurrentPage NOTIFY currentPageChanged)

public:
    explicit PageObject(Poppler::Page *page, QGraphicsItem *parent = 0);
    ~PageObject();


    qreal resolutionX() const;
    void setResolutionX(const qreal &resolutionX);

    qreal resolutionY() const;
    void setResolutionY(const qreal &resolutionY);

    uint rotation() const;
    void setRotation(const uint &rotation);

    QString filePath() const;
    void setFilePath(const QString &filePath);

    int currentPage() const;
    void setCurrentPage(const int &currentPage);

    bool findNext(const QString &text);
    void clearHighlight();

    QRectF highlightedArea() const;
    QString highlightedText() const;

    void prefetch();

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

private:
    Poppler::Page *m_page;

    qreal m_resolutionX;
    qreal m_resolutionY;
    uint m_rotation;

    QString m_filePath;
    int m_currentPage;

    QList<Poppler::LinkGoto*> m_links;
    QRectF m_highlight;

    QFutureWatcher<QImage> m_renderWatcher;
    QImage renderPage(bool prefetch);

    static QMutex s_mutex;
    static QMap<QPair<QString, int>, QImage> s_pageCache;

signals:
    void rotationChanged(uint);
    void resolutionXChanged(qreal);
    void resolutionYChanged(qreal);
    void filePathChanged(QString);
    void currentPageChanged(int);

    void linkClicked(int gotoPage);

private slots:
    void renderFinished();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

};

#endif // PAGEOBJECT_H
