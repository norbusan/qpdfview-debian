#ifndef RENDERTHREAD_H
#define RENDERTHREAD_H

#include <QtCore>
#include <QtGui>

#include <poppler-qt4.h>

class RenderThread;

class PageItem : public QGraphicsItem
{
public:
    PageItem(QGraphicsItem *parent = 0, QGraphicsScene *scene = 0) :
        QGraphicsItem(parent, scene), m_pageWidth(-1.0), m_pageHeight(-1.0), m_resolutionX(-1.0), m_resolutionY(-1.0), m_page(0), m_image(), m_jobDispatched(false), m_jobFinished(false), m_renderThread(0) {}

    qreal pageWidth() const { return m_pageWidth; }
    qreal pageHeight() const { return m_pageHeight; }
    void setPageSize(qreal pageWidth, qreal pageHeight)
    {
        m_pageWidth = pageWidth;
        m_pageHeight = pageHeight;
    }

    qreal resolutionX() const { return m_resolutionX; }
    qreal resolutionY() const { return m_resolutionY; }
    void setResolution(qreal resolutionX, qreal resolutionY)
    {
        m_resolutionX = resolutionX;
        m_resolutionY = resolutionY;
    }

    Poppler::Page *page() const { return m_page; }
    void setPage(Poppler::Page *page)
    {
        m_page = page;
    }

    QImage image() const { return m_image; }
    void setImage(QImage image)
    {
        m_image = image;
    }

    bool jobDispatched() const { return m_jobDispatched; }
    void setJobDispatched(bool jobDispatched)
    {
        m_jobDispatched = jobDispatched;
    }

    bool jobFinished() const { return m_jobFinished; }
    void setJobFinished(bool jobFinished)
    {
        m_jobFinished = jobFinished;
    }

    RenderThread *renderThread() const { return m_renderThread; }
    void setRenderThread(RenderThread *renderThread)
    {
        m_renderThread = renderThread;
    }

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

private:
    qreal m_pageWidth;
    qreal m_pageHeight;

    qreal m_resolutionX;
    qreal m_resolutionY;

    Poppler::Page *m_page;
    QImage m_image;

    bool m_jobDispatched;
    bool m_jobFinished;
    RenderThread *m_renderThread;

};

class RenderThread : public QThread
{
    Q_OBJECT
public:
    explicit RenderThread(QObject *parent = 0);
    ~RenderThread();

    void setDocumentMutex(QMutex *documentMutex)
    {
        m_documentMutex = documentMutex;
    }

    void enqueueJob(PageItem *job);
    void run();

signals:
    void jobFinished(PageItem *job);

private:
    QMutex *m_documentMutex;
    QMutex *m_jobQueueMutex;
    QSemaphore *m_jobQueueSemaphore;

    QQueue<PageItem*> m_jobQueue;
};

#endif // RENDERTHREAD_H
