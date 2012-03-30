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

struct Link;
struct TocNode;
class DocumentView;

class AuxiliaryView : public QWidget
{
    Q_OBJECT

public:
    explicit AuxiliaryView(QWidget *parent = 0);

    DocumentView *documentView() const;
    void setDocumentView(DocumentView *documentView);

protected:
    void showEvent(QShowEvent *event);

protected slots:
    virtual void slotDocumentChanged() = 0;

private:
    DocumentView *m_documentView;

};

// outline view

class OutlineView : public AuxiliaryView
{
    Q_OBJECT

public:
    explicit OutlineView(QWidget *parent = 0);

protected slots:
    void slotDocumentChanged();

    void slotItemClicked(QTreeWidgetItem *item, int column);

private:
    QTreeWidget *m_treeWidget;

    void prepareOutline(TocNode *node, QTreeWidgetItem *parent, QTreeWidgetItem *sibling);

};

// thumbnails view

class ThumbnailsView : public AuxiliaryView
{
    Q_OBJECT

public:
    explicit ThumbnailsView(QWidget *parent = 0);

protected slots:
    void slotDocumentChanged();

    void slotItemClicked(QListWidgetItem *item);

private:
    QListWidget *m_listWidget;

};

// presentation view

class PresentationView : public AuxiliaryView
{
    Q_OBJECT

public:
    explicit PresentationView();
    ~PresentationView();

public slots:
    void setCurrentPage(int currentPage);

    void previousPage();
    void nextPage();
    void firstPage();
    void lastPage();

protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent*);
    void keyPressEvent(QKeyEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

protected slots:
    void slotDocumentChanged();

private:
    int m_currentPage;

    QSizeF m_size;
    qreal m_scaleFactor;
    QRectF m_boundingRect;

    QList<Link> m_links;
    QTransform m_linkTransform;

    // internal methods

    void prepareView();

    QFuture<void> m_render;
    void render();
};

// recently used action

class RecentlyUsedAction : public QAction
{
    Q_OBJECT

public:
    RecentlyUsedAction(QObject *parent = 0);
    ~RecentlyUsedAction();

public slots:
    void addEntry(const QString &filePath);

    void clearList();

signals:
    void entrySelected(QString filePath);

protected slots:
    void slotActionGroupTriggered(QAction *action);

private:
    QMenu *m_menu;
    QActionGroup *m_actionGroup;
    QAction *m_separator;

    QAction *m_clearListAction;

    // settings

    QSettings m_settings;
};

// bookmarks menu

class BookmarksMenu : public QMenu
{
    Q_OBJECT

public:
    BookmarksMenu(QWidget *parent = 0);

public slots:
    void addEntry();

    void goToPreviousEntry();
    void goToNextEntry();

    void setPosition(int page, qreal top);

    void clearPage();
    void clearDocument();

signals:
    void entrySelected(int page, qreal top);

protected slots:
    void slotActionGroupTriggered(QAction *action);

private:
    QActionGroup *m_actionGroup;

    QAction *m_addEntryAction;

    QAction *m_goToPreviousEntryAction;
    QAction *m_goToNextEntryAction;

    QAction *m_clearPageAction;
    QAction *m_clearDocumentAction;

    int m_page;
    qreal m_top;

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

    QCheckBox *m_automaticRefreshCheckBox;
    QCheckBox *m_openUrlCheckBox;

    QCheckBox *m_antialiasingCheckBox;
    QCheckBox *m_textAntialiasingCheckBox;

    QComboBox *m_maximumPageCacheSizeComboBox;

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
