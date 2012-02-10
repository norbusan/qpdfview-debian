#ifndef DOCUMENTVIEW_H
#define DOCUMENTVIEW_H

#include <QtCore>
#include <QtGui>

#include "documentmodel.h"
#include "pageitem.h"

class DocumentView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit DocumentView(QWidget *parent = 0);
    
signals:
    
public slots:
    void changeDocument(const QString &filePath);

    void changeIndex(const int &index);

    void changeDisplayMode(const DocumentModel::DisplayModes &displayMode);

    void changeScaleMode(const DocumentModel::ScaleModes &scaleMode);
    void changeScaleFactor(const qreal &scaleFactor);

private:
    QGraphicsScene m_scene;
    QList<PageItem*> m_itemList;
    
};

#endif // DOCUMENTVIEW_H
