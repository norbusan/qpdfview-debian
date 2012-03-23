/*

Copyright 2012 Adam Reichold

This file is part of qpdfview.

qpdfview is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

qpdfview is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with qpdfview.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef MISCELLANEOUS_H
#define MISCELLANEOUS_H

#include <QtCore>
#include <QtGui>

#include "documentmodel.h"
#include "documentview.h"

class AuxiliaryView : public QWidget
{
    Q_OBJECT

public:
    explicit AuxiliaryView(QWidget *parent);

protected:
    DocumentView *m_view;

public slots:
    void attachTo(DocumentView *view);
    virtual void updateContent();
};

class OutlineView : public AuxiliaryView
{
    Q_OBJECT

public:
    explicit OutlineView(QWidget *parent = 0);
    ~OutlineView();

private:
    QTreeWidget *m_treeWidget;
    DocumentModel::Outline *m_outline;

public slots:
    void updateContent();

private slots:
    void followLink(QTreeWidgetItem *item, int column);

};

class ThumbnailsView : public AuxiliaryView
{
    Q_OBJECT

public:
    explicit ThumbnailsView(QWidget *parent = 0);

private:
    QListWidget *m_listWidget;

public slots:
    void updateContent();

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
