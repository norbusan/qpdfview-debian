#ifndef MISCELLANEOUS_H
#define MISCELLANEOUS_H

#include <QtCore>
#include <QtGui>

#include "documentmodel.h"
#include "documentview.h"

class OutlineView : public QWidget
{
    Q_OBJECT

public:
    explicit OutlineView(QWidget *parent = 0);
    ~OutlineView();

private:
    QTreeWidget *m_treeWidget;
    DocumentView *m_view;

    DocumentModel::Outline *m_outline;

public slots:
    void attachView(DocumentView *view);
    void updateView();

private slots:
    void followLink(QTreeWidgetItem *item, int column);

};

class ThumbnailsView : public QWidget
{
    Q_OBJECT

public:
    explicit ThumbnailsView(QWidget *parent = 0);

private:
    QListWidget *m_listWidget;
    DocumentView *m_view;

public slots:
    void attachView(DocumentView *view);
    void updateView();

private slots:
    void followLink(QListWidgetItem *item);

};

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = 0);

public slots:
    void accept();
    
private:
    QFormLayout *m_layout;
    QDialogButtonBox *m_buttonBox;

    QLineEdit *m_pageCacheSizeLineEdit;
    QIntValidator *m_pageCacheSizeValidator;

    QSettings m_settings;

};

#endif // MISCELLANEOUS_H
