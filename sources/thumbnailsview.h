#ifndef THUMBNAILSVIEW_H
#define THUMBNAILSVIEW_H

#include <QtCore>
#include <QtGui>

#include "documentview.h"

class ThumbnailsView : public QDockWidget
{
    Q_OBJECT

public:
    explicit ThumbnailsView(QWidget *parent = 0);
    ~ThumbnailsView();

    void setDocumentView(DocumentView *documentView);

private:
    DocumentView *m_documentView;
    QListWidget *m_listWidget;

private slots:
    void updateThumbnails();

    void followLink(QListWidgetItem *item);
    
};

#endif // THUMBNAILSVIEW_H
