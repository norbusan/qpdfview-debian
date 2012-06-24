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

        bool operator<(const PageCacheKey& key) const
        {
            return (index < key.index) ||
                   (index == key.index && !qFuzzyCompare(scale, key.scale) && scale < key.scale);
        }

    };

    struct PageCacheValue
    {
        QTime time;
        QImage image;

        PageCacheValue() : time(QTime::currentTime()), image() {}
        PageCacheValue(const QImage& image) : time(QTime::currentTime()), image(image) {}

    };

public:
    PresentationView();
    ~PresentationView();

public slots:
    bool open(const QString& filePath);

    void setCurrentPage(int currentPage);

    void previousPage();
    void nextPage();
    void firstPage();
    void lastPage();

protected slots:
    void slotPrefetchTimerTimeout();

protected:
    void resizeEvent(QResizeEvent*);
    void paintEvent(QPaintEvent*);

    void keyPressEvent(QKeyEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void mousePressEvent(QMouseEvent* event);

private:
    // document

    Poppler::Document* m_document;
    QMutex m_documentMutex;

    // page cache

    QMap< PageCacheKey, PageCacheValue > m_pageCache;
    QMutex m_pageCacheMutex;

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

    QList< Link > m_links;
    QTransform m_linkTransform;

    // miscellaneous

    QTimer* m_prefetchTimer;

    // internal methods

    void prepareView();

    // render

    QFuture< void > m_render;
    void render(int index);

};

// tab bar

class TabBar : public QTabBar
{
    Q_OBJECT

public:
    TabBar(QWidget* parent = 0);

protected:
    void contextMenuEvent(QContextMenuEvent* event);

    void mousePressEvent(QMouseEvent* event);

};

// tab widget

class TabWidget : public QTabWidget
{
    Q_OBJECT
    Q_PROPERTY(bool tabBarAsNeeded READ tabBarAsNeeded WRITE setTabBarAsNeeded)

public:
    TabWidget(QWidget* parent = 0);

    bool tabBarAsNeeded() const;
    void setTabBarAsNeeded(bool tabBarAsNeeded);

protected:
    void tabInserted(int);
    void tabRemoved(int);

private:
    bool m_tabBarAsNeeded;

};

// line edit

class LineEdit : public QLineEdit
{
    Q_OBJECT

public:
    LineEdit(QWidget* parent = 0);

protected:
    void mousePressEvent(QMouseEvent* event);

};

// combo box

class ComboBox : public QComboBox
{
    Q_OBJECT

public:
    ComboBox(QWidget* parent = 0);

};

// recently used action

class RecentlyUsedAction : public QAction
{
    Q_OBJECT

public:
    explicit RecentlyUsedAction(QObject* parent = 0);
    ~RecentlyUsedAction();

public slots:
    void addEntry(const QString& filePath);

    void clearList();

signals:
    void entrySelected(const QString& filePath);

protected slots:
    void slotActionGroupTriggered(QAction* action);

private:
    QActionGroup* m_actionGroup;
    QAction* m_separator;

    QAction* m_clearListAction;

    // settings

    QSettings m_settings;
};

// bookmarks menu

class BookmarksMenu : public QMenu
{
    Q_OBJECT

public:
    explicit BookmarksMenu(QWidget* parent = 0);

public slots:
    void addEntry();
    void removeEntriesOnCurrentPage();
    void goToPreviousEntry();
    void goToNextEntry();

    void clearList();

    void updateCurrentPage(int currentPage) { m_currentPage = currentPage; }
    void updateValue(int value) { m_value = value; }
    void updateRange(int minimum, int maximum) { m_minimum = minimum; m_maximum = maximum; }

    void setReturnPosition(int page, int value);

signals:
    void entrySelected(int page, int value);

protected slots:
    void slotReturnActionTriggered();
    void slotActionGroupTriggered(QAction* action);

private:
    QActionGroup* m_actionGroup;
    QAction* m_separator;

    QAction* m_returnAction;

    QAction* m_addEntryAction;

    QAction* m_removeEntriesOnCurrentPageAction;
    QAction* m_goToPreviousEntryAction;
    QAction* m_goToNextEntryAction;

    QAction* m_clearListAction;

    QHash< QAction*, int > m_pages;
    QHash< QAction*, int > m_values;

    int m_currentPage;
    int m_value;
    int m_minimum;
    int m_maximum;

    int m_returnPage;
    int m_returnValue;

};

// settings dialog

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = 0);

public slots:
    void accept();

private:
    // widgets

    QTabWidget* m_tabWidget;
    QDialogButtonBox* m_buttonBox;

    QWidget* m_behaviourWidget;
    QFormLayout* m_behaviourLayout;

    QCheckBox* m_tabBarAsNeededCheckBox;
    QComboBox* m_tabPositionComboBox;

    QCheckBox* m_restoreTabsCheckBox;

    QCheckBox* m_autoRefreshCheckBox;

    QCheckBox* m_fitToEqualWidthCheckBox;

    QCheckBox* m_highlightLinksCheckBox;
    QCheckBox* m_externalLinksCheckBox;

    QWidget* m_graphicsWidget;
    QFormLayout* m_graphicsLayout;

    QCheckBox* m_antialiasingCheckBox;
    QCheckBox* m_textAntialiasingCheckBox;
    QCheckBox* m_textHintingCheckBox;

    QComboBox* m_maximumPageCacheSizeComboBox;
    QCheckBox* m_prefetchCheckBox;

    // settings

    QSettings m_settings;

};

// fonts dialog

class FontsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FontsDialog(QTableWidget* fontsTableWidget, QWidget* parent = 0);

    QSize sizeHint() const;

private:
    QDialogButtonBox* m_buttonBox;

};

// help dialog

class HelpDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HelpDialog(QWidget* parent = 0);

    QSize sizeHint() const;

private:
    QTextBrowser* m_textBrowser;
    QDialogButtonBox* m_buttonBox;

};

#endif // MISCELLANEOUS_H
