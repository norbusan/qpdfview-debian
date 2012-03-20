#ifndef OUTLINEVIEW_H
#define OUTLINEVIEW_H

#include <QtCore>
#include <QtGui>

#include "documentview.h"

class OutlineView : public QDockWidget
{
    Q_OBJECT

public:
    explicit OutlineView(QWidget *parent = 0);
    ~OutlineView();

    void setDocumentView(DocumentView *documentView);

private:
    DocumentView *m_documentView;
    QTreeWidget *m_treeWidget;

    QDomDocument *m_outline;
    void updateOutline(const QDomNode &parentNode, QTreeWidgetItem *parentItem);

private slots:
    void updateOutline();

    void followLink(QTreeWidgetItem *item, const int &column);

};

#endif // OUTLINEVIEW_H
