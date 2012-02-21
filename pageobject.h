#ifndef PAGEOBJECT_H
#define PAGEOBJECT_H

#include <QtCore>
#include <QtGui>

#include <poppler-qt4.h>

const int PageCacheLimit = 16;

class PageCache
{
public:
    explicit PageCache();
    ~PageCache();

    bool contains(int page);
    QImage retrieve(int page);

    void clear();
    void insert(int page, QImage image);

private:
    QMap<int, QImage> m_map;
    QMutex m_mutex;
};

class PageObject : public QGraphicsObject
{
    Q_OBJECT
    Q_PROPERTY(qreal resolutionX READ resolutionX WRITE setResolutionX NOTIFY resolutionXChanged)
    Q_PROPERTY(qreal resolutionY READ resolutionY WRITE setResolutionY NOTIFY resolutionYChanged)
    Q_PROPERTY(QString filePath READ filePath WRITE setFilePath NOTIFY filePathChanged)
    Q_PROPERTY(int currentPage READ currentPage WRITE setCurrentPage NOTIFY currentPageChanged)

public:
    explicit PageObject(Poppler::Page *page, PageCache *pageCache, QGraphicsItem *parent = 0);
    ~PageObject();


    Poppler::Page::Rotation rotation() const;
    void setRotation(const Poppler::Page::Rotation &rotation);

    qreal resolutionX() const;
    void setResolutionX(const qreal &resolutionX);

    qreal resolutionY() const;
    void setResolutionY(const qreal &resolutionY);

    QString filePath() const;
    void setFilePath(const QString &filePath);

    int currentPage() const;
    void setCurrentPage(const int &currentPage);


    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    void prefetch();

private:
    Poppler::Page *m_page;
    PageCache *m_pageCache;

    Poppler::Page::Rotation m_rotation;
    qreal m_resolutionX;
    qreal m_resolutionY;
    QString m_filePath;
    int m_currentPage;

    QFutureWatcher<QImage> m_futureWatcher;
    QImage renderPage(bool prefetch);

signals:
    void rotationChanged(Poppler::Page::Rotation);
    void resolutionXChanged(qreal);
    void resolutionYChanged(qreal);
    void filePathChanged(QString);
    void currentPageChanged(int);

private slots:
    void updatePageCache();

};

#endif // PAGEOBJECT_H
