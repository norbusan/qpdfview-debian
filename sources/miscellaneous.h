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

class DocumentView;

class AuxiliaryView : public QWidget
{
    Q_OBJECT

public:
    explicit AuxiliaryView(QWidget *parent);

    DocumentView *view() const;
    void setView(DocumentView *view);

private:
    DocumentView *m_view;

public slots:
    void updateVisibility(bool visible);

private slots:
    virtual void updateModel() = 0;

};

// outline view

class OutlineView : public AuxiliaryView
{
    Q_OBJECT

public:
    explicit OutlineView(QWidget *parent = 0);

private:
    QTreeWidget *m_treeWidget;

public slots:
    void updateModel();

private slots:
    void followLink(QTreeWidgetItem *item, int column);

};

// thumbnails view

class ThumbnailsView : public AuxiliaryView
{
    Q_OBJECT

public:
    explicit ThumbnailsView(QWidget *parent = 0);

private:
    QListWidget *m_listWidget;

public slots:
    void updateModel();

private slots:
    void followLink(QListWidgetItem *item);

};

// recently used action

class RecentlyUsedAction : public QAction
{
    Q_OBJECT

public:
    RecentlyUsedAction(QObject *parent = 0);
    ~RecentlyUsedAction();

private:
    QMenu *m_menu;
    QActionGroup *m_actionGroup;
    QAction *m_separator;

    QAction *m_clearListAction;

    QSettings m_settings;

signals:
    void entrySelected(QString);

private slots:
    void selectEntry(QAction *action);

    void clearList();

public slots:
    void addEntry(const QString &filePath);

};

// bookmarks menu

class BookmarksMenu : public QMenu
{
    Q_OBJECT

public:
    BookmarksMenu(QWidget *parent = 0);

private:
    QActionGroup *m_actionGroup;

    QAction *m_addEntryAction;
    QAction *m_selectPreviousEntryAction;
    QAction *m_selectNextEntryAction;
    QAction *m_removeEntriesOnCurrentPageAction;
    QAction *m_clearListAction;

    int m_currentPage;
    qreal m_top;

signals:
    void entrySelected(int currentPage, qreal top);

private slots:
    void selectEntry(QAction *action);

public slots:
    void addEntry();
    void selectPreviousEntry();
    void selectNextEntry();
    void removeEntriesOnCurrentPage();
    void clearList();

    void updateCurrrentPage(int currentPage, qreal top = 0.0);

};

// settings dialog

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

    QCheckBox *m_watchFilePathCheckBox;
    QCheckBox *m_openUrlLinksCheckBox;
    QCheckBox *m_openExternalLinksCheckBox;

    QCheckBox *m_antialiasingCheckBox;
    QCheckBox *m_textAntialiasingCheckBox;
    QCheckBox *m_textHintingCheckBox;

    QComboBox *m_pageCacheSizeComboBox;

};

// help dialog

class HelpDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HelpDialog(QWidget *parent = 0);

    QSize sizeHint() const
    {
        return QSize(500, 700);
    }

private:
    QTextBrowser *m_textBrowser;
    QDialogButtonBox *m_buttonBox;

};

#endif // MISCELLANEOUS_H
