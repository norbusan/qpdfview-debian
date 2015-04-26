/*

Copyright 2014 S. Razi Alavizadeh
Copyright 2012-2015 Adam Reichold
Copyright 2014 Dorian Scholz

This file is part of qpdfview.

qpdfview is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
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

#include <QComboBox>
#include <QGraphicsEffect>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>
#include <QProxyStyle>
#include <QSpinBox>
#include <QTreeView>

class QTextLayout;

namespace qpdfview
{

// graphics composition mode effect

class GraphicsCompositionModeEffect : public QGraphicsEffect
{
    Q_OBJECT

public:
    GraphicsCompositionModeEffect(QPainter::CompositionMode compositionMode, QObject* parent = 0);

protected:
    void draw(QPainter* painter);

private:
    QPainter::CompositionMode m_compositionMode;

};

// proxy style

class ProxyStyle : public QProxyStyle
{
    Q_OBJECT

public:
    ProxyStyle();

    bool scrollableMenus() const;
    void setScrollableMenus(bool scrollableMenus);

    int styleHint(StyleHint hint, const QStyleOption* option, const QWidget* widget, QStyleHintReturn* returnData) const;

private:
    Q_DISABLE_COPY(ProxyStyle)

    bool m_scrollableMenus;

};

// searchable menu

class SearchableMenu : public QMenu
{
    Q_OBJECT

public:
    SearchableMenu(const QString& title, QWidget* parent = 0);

    bool isSearchable() const;
    void setSearchable(bool searchable);

protected:
    void hideEvent(QHideEvent* event);
    void keyPressEvent(QKeyEvent* event);

private:
    bool m_searchable;
    QString m_text;

};

// tab bar

class TabBar : public QTabBar
{
    Q_OBJECT

public:
    explicit TabBar(QWidget* parent = 0);

protected:
    QSize tabSizeHint(int index) const;

    void mousePressEvent(QMouseEvent* event);

private:
    Q_DISABLE_COPY(TabBar)

};

// tab widget

class TabWidget : public QTabWidget
{
    Q_OBJECT

public:
    explicit TabWidget(QWidget* parent = 0);

    enum TabBarPolicy
    {
        TabBarAsNeeded = 0,
        TabBarAlwaysOn = 1,
        TabBarAlwaysOff = 2
    };

    TabBarPolicy tabBarPolicy() const;
    void setTabBarPolicy(TabBarPolicy tabBarPolicy);

    bool spreadTabs() const;
    void setSpreadTabs(bool spreadTabs);

signals:
    void tabContextMenuRequested(const QPoint& globalPos, int index);

protected slots:
    void on_tabBar_customContextMenuRequested(const QPoint& pos);

protected:
    void tabInserted(int index);
    void tabRemoved(int index);

private:
    Q_DISABLE_COPY(TabWidget)

    TabBarPolicy m_tabBarPolicy;
    bool m_spreadTabs;

};

// tree view

class TreeView : public QTreeView
{
    Q_OBJECT

public:
    explicit TreeView(int expansionRole, QWidget* parent = 0);

public slots:
    void expandAbove(const QModelIndex& child);

    void expandAll(const QModelIndex& index = QModelIndex());
    void collapseAll(const QModelIndex& index = QModelIndex());

    int expandedDepth(const QModelIndex& index);

    void expandToDepth(const QModelIndex& index, int depth);
    void collapseFromDepth(const QModelIndex& index, int depth);

    void restoreExpansion(const QModelIndex& index = QModelIndex());

protected:
    void keyPressEvent(QKeyEvent* event);
    void wheelEvent(QWheelEvent* event);

    void contextMenuEvent(QContextMenuEvent* event);

protected slots:
    void on_expanded(const QModelIndex& index);
    void on_collapsed(const QModelIndex& index);

private:
    Q_DISABLE_COPY(TreeView)

    int m_expansionRole;

};

// line edit

class LineEdit : public QLineEdit
{
    Q_OBJECT

public:
    explicit LineEdit(QWidget* parent = 0);

protected:
    void mousePressEvent(QMouseEvent* event);

private:
    Q_DISABLE_COPY(LineEdit)

};

// combo box

class ComboBox : public QComboBox
{
    Q_OBJECT

public:
    explicit ComboBox(QWidget* parent = 0);

private:
    Q_DISABLE_COPY(ComboBox)

};

// spin box

class SpinBox : public QSpinBox
{
    Q_OBJECT

public:
    explicit SpinBox(QWidget* parent = 0);

signals:
    void returnPressed();

protected:
    void keyPressEvent(QKeyEvent* event);

private:
    Q_DISABLE_COPY(SpinBox)

};

// mapping spin box

class MappingSpinBox : public SpinBox
{
    Q_OBJECT

public:
    struct TextValueMapper
    {
        virtual ~TextValueMapper() {}

        virtual QString textFromValue(int val, bool& ok) const = 0;
        virtual int valueFromText(const QString& text, bool& ok) const = 0;
    };

    MappingSpinBox(TextValueMapper* mapper, QWidget* parent = 0);

protected:
    QString textFromValue(int val) const;
    int valueFromText(const QString& text) const;

    QValidator::State validate(QString& input, int& pos) const;

private:
    Q_DISABLE_COPY(MappingSpinBox)

    QScopedPointer< TextValueMapper > m_mapper;

};

int getMappedNumber(MappingSpinBox::TextValueMapper* mapper,
                    QWidget* parent, const QString& title, const QString& caption,
                    int value = 0, int min = -2147483647, int max = 2147483647,
                    bool* ok = 0, Qt::WindowFlags flags = 0);

// progress line edit

class ProgressLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    explicit ProgressLineEdit(QWidget* parent = 0);

    int progress() const;
    void setProgress(int progress);

signals:
    void returnPressed(const Qt::KeyboardModifiers& modifiers);

protected:
    void paintEvent(QPaintEvent* event);
    void keyPressEvent(QKeyEvent* event);

private:
    Q_DISABLE_COPY(ProgressLineEdit)

    int m_progress;

};

// search line edit

class SearchLineEdit : public ProgressLineEdit
{
    Q_OBJECT

public:
    explicit SearchLineEdit(QWidget* parent = 0);

public slots:
    void startSearch();

    void startTimer();
    void stopTimer();

signals:
    void searchInitiated(const QString& text, bool modified = false);

protected slots:
    void on_timeout();
    void on_returnPressed(const Qt::KeyboardModifiers& modifiers);

private:
    Q_DISABLE_COPY(SearchLineEdit)

    QTimer* m_timer;

};

// fallback icons

inline QIcon loadIconWithFallback(const QString& name)
{
    QIcon icon = QIcon::fromTheme(name);

    if(icon.isNull())
    {
        icon = QIcon(QLatin1String(":icons/") + name + QLatin1String(".svg"));
    }

    return icon;
}

} // qpdfview

#endif // MISCELLANEOUS_H
