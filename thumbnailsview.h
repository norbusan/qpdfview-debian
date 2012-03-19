#ifndef THUMBNAILSVIEW_H
#define THUMBNAILSVIEW_H

#include <QDockWidget>

class ThumbnailsView : public QDockWidget
{
    Q_OBJECT

public:
    explicit ThumbnailsView(QWidget *parent = 0);
    ~ThumbnailsView();
    
};

#endif // THUMBNAILSVIEW_H
