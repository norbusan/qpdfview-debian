#include "renderthread.h"

RenderThread::RenderThread(QObject *parent) :
    QThread(parent),
    m_documentMutex(0),
    m_jobQueue()
{
    m_jobQueueMutex = new QMutex();
    m_jobQueueSemaphore = new QSemaphore();
}

RenderThread::~RenderThread()
{
    delete m_jobQueueMutex;
    delete m_jobQueueSemaphore;
}

void RenderThread::enqueueJob(PageItem *job)
{
    QMutexLocker mutexLocker(m_jobQueueMutex);
    m_jobQueue.enqueue(job);
    mutexLocker.unlock();

    m_jobQueueSemaphore->release();
}

void RenderThread::run()
{

    while(true)
    {
        m_jobQueueSemaphore->acquire();

        QMutexLocker mutexLocker1(m_jobQueueMutex);
        PageItem *job = m_jobQueue.dequeue();
        mutexLocker1.unlock();

        qDebug() << "acquired job:" << job->page()->label();

        QMutexLocker mutexLocker2(m_documentMutex);
        job->setImage(job->page()->renderToImage(job->resolutionX(), job->resolutionY()));
        mutexLocker2.unlock();

        emit jobFinished(job);
    }
}

QRectF PageItem::boundingRect() const
{
    return QRectF(0.0, 0.0, m_pageWidth, m_pageHeight);
}

void PageItem::paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    if(!m_jobDispatched)
    {
        m_jobDispatched = true;

        qDebug() << "enqueued job:" << m_page->label();

        m_renderThread->enqueueJob(this);
    }

    if(!m_jobFinished)
    {
        painter->fillRect(boundingRect(), QBrush(Qt::white));
    }
    else
    {
        painter->drawImage(0, 0, m_image);
    }

    painter->setPen(QPen(Qt::black));
    painter->drawRect(boundingRect());
}
