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

#include <poppler-qt4.h>

// presentation view

class PresentationView : public QWidget
{
    Q_OBJECT

private:
    struct Link
    {
        QRectF area;
        int page;

        Link() : area(), page(-1) {}
        Link(QRectF area, int page) : area(area), page(page) {}

    };

    struct PageCacheKey
    {
        int index;
        qreal scale;

        PageCacheKey() : index(-1), scale(1.0) {}
        PageCacheKey(int index, qreal scale) : index(index), scale(scale) {}

        bool operator<(const PageCacheKey &key) const
        {
            return (index < key.index) || (index == key.index && scale < key.scale);
        }
    };

public:
    explicit PresentationView();
    ~PresentationView();

    void setCurrentPage(int currentPage);

public slots:
    bool open(const QString &filePath);

    void previousPage();
    void nextPage();
    void firstPage();
    void lastPage();

protected slots:
    void slotPrefetchTimerTimeout();

protected:
    void resizeEvent(QResizeEvent*);
    void paintEvent(QPaintEvent *event);

    void keyPressEvent(QKeyEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

private:
    // document

    Poppler::Document *m_document;

    // page cache

    QMap< PageCacheKey, QImage > m_pageCache;

    uint m_pageCacheSize;
    uint m_maximumPageCacheSize;

    // properties

    QString m_filePath;
    int m_numberOfPages;
    int m_currentPage;

    // settings

    QSettings m_settings;

    // graphics

    qreal m_scale;
    QRectF m_boundingRect;

    // links

    QList<Link> m_links;
    QTransform m_linkTransform;

    // miscellaneous

    QTimer *m_prefetchTimer;

    // internal methods

    void prepareView();

    // render

    QFuture<void> m_render;
    void render(int index);

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
    void removeEntry(const QString &filePath);

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

    void clearList();

    void setCurrentPage(int currentPage);
    void setCurrentTop(qreal currentTop);

signals:
    void entrySelected(int page, qreal top);

protected slots:
    void slotActionGroupTriggered(QAction *action);

private:
    QActionGroup *m_actionGroup;

    QAction *m_addEntryAction;

    QAction *m_goToPreviousEntryAction;
    QAction *m_goToNextEntryAction;

    QAction *m_clearListAction;

    QHash<QAction*, int> m_pages;
    QHash<QAction*, qreal> m_tops;

    int m_currentPage;
    qreal m_currentTop;

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

    QCheckBox *m_autoRefreshCheckBox;
    QCheckBox *m_externalLinksCheckBox;

    QCheckBox *m_antialiasingCheckBox;
    QCheckBox *m_textAntialiasingCheckBox;
    QCheckBox *m_textHintingCheckBox;

    QCheckBox *m_uniformFitCheckBox;

    QComboBox *m_maximumPageCacheSizeComboBox;

    // settings

    QSettings m_settings;

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
