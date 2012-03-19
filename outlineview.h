#ifndef OUTLINEVIEW_H
#define OUTLINEVIEW_H

#include <QtCore>
#include <QtGui>

class OutlineView : public QDockWidget
{
    Q_OBJECT

public:
    explicit OutlineView(QWidget *parent = 0);
    ~OutlineView();

private:
    QTreeWidget *m_treeWidget;

};

#endif // OUTLINEVIEW_H
