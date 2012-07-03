#ifndef SEARCHTHREAD_H
#define SEARCHTHREAD_H

#include <QtCore>

#include <poppler-qt4.h>

class SearchThread : public QThread
{
    Q_OBJECT

public:
    SearchThread(QObject* parent = 0);

    bool wasCanceled() const;

    void run();

public slots:
    void start(QMutex* mutex, Poppler::Document* document, const QList< int >& indices, const QString& text, bool matchCase);
    void cancel();

signals:
    void progressed(int progress);
    void finished();
    void canceled();

    void resultsReady(int index, QList< QRectF > results);

private:
    bool m_wasCanceled;

    QMutex* m_mutex;
    Poppler::Document* m_document;

    QList< int > m_indices;
    QString m_text;
    bool m_matchCase;

};

#endif // SEARCHTHREAD_H
