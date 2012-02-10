#ifndef DOCUMENTMODEL_H
#define DOCUMENTMODEL_H

#include <QtCore>

#include <poppler-qt4.h>

class DocumentModel : public QObject
{
    Q_OBJECT
public:
    enum DisplayModes { PagingMode, ScrollingMode, DoublePagingMode, DoubleScrollingMode };
    enum ScaleModes { ScaleFactorMode, FitToPageMode, FitToPageWidthMode };

    explicit DocumentModel(QObject *parent = 0);
    ~DocumentModel();

    bool load(const QString &filePath);
    bool reload();
    bool save(const QString &filePath) const;

    const QString &filePath() const { return m_filePath; }

    const int &index() const { return m_index; }

    const DisplayModes &displayMode() const { return m_displayMode; }

    const ScaleModes &scaleMode() const { return m_scaleMode; }
    const qreal &scaleFactor() const { return m_scaleFactor; }
    
signals:
    void documentChanged(QString);

    void indexChanged(int);

    void displayModeChanged(DocumentModel::DisplayModes);

    void scaleModeChanged(DocumentModel::ScaleModes);
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

private:
    Poppler::Document *m_document;
    QList<Poppler::Page*> m_pageList;

    QString m_filePath;

    int m_index;

    DisplayModes m_displayMode;

    ScaleModes m_scaleMode;
    qreal m_scaleFactor;

};

#endif // DOCUMENTMODEL_H
