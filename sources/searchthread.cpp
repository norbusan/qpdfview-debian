#include "searchthread.h"

SearchThread::SearchThread(QObject* parent) : QThread(parent),
    m_wasCanceled(false),
    m_mutex(0),
    m_document(0),
    m_indices(),
    m_text(),
    m_matchCase(false)
{
    qRegisterMetaType< QList< QRectF > >("QList<QRectF>");
}

bool SearchThread::wasCanceled() const
{
    return m_wasCanceled;
}

void SearchThread::run()
{
    int indicesDone = 0;
    int indicesToDo = m_indices.count();

    foreach(int index, m_indices)
    {
        if(m_wasCanceled)
        {
            emit canceled();

            return;
        }

        QList< QRectF > results;

        m_mutex->lock();

        Poppler::Page* page = m_document->page(index);

#if defined(HAS_POPPLER_22)

        results = page->search(m_text, m_matchCase ? Poppler::Page::CaseSensitive : Poppler::Page::CaseInsensitive);

#elif defined(HAS_POPPLER_14)

        double left = 0.0, top = 0.0, right = 0.0, bottom = 0.0;

        while(page->search(m_text, left, top, right, bottom, Poppler::Page::NextResult, m_matchCase ? Poppler::Page::CaseSensitive : Poppler::Page::CaseInsensitive))
        {
            QRectF rect;
            rect.setLeft(left);
            rect.setTop(top);
            rect.setRight(right);
            rect.setBottom(bottom);

            results.append(rect);
        }

#else

        QRectF rect;

        while(page->search(m_text, rect, Poppler::Page::NextResult, m_matchCase ? Poppler::Page::CaseSensitive : Poppler::Page::CaseInsensitive))
        {
            results.append(rect);
        }

#endif

        delete page;

        m_mutex->unlock();

        if(!results.isEmpty())
        {
            emit resultsReady(index, results);
        }

        emit progressed(100 * ++indicesDone / indicesToDo);
    }

    emit finished();
}

void SearchThread::start(QMutex* mutex, Poppler::Document* document, const QList< int >& indices, const QString& text, bool matchCase)
{
    m_mutex = mutex;
    m_document = document;

    m_indices = indices;
    m_text = text;
    m_matchCase = matchCase;

    m_wasCanceled = false;

    QThread::start();
}

void SearchThread::cancel()
{
    m_wasCanceled = true;
}
