/*

Copyright 2012 Adam Reichold

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
#include <QLineEdit>
#include <QSpinBox>
#include <QTabBar>
#include <QTabWidget>
#include <QTreeView>

// tab bar

class TabBar : public QTabBar
{
    Q_OBJECT

public:
    explicit TabBar(QWidget* parent = 0);

protected:
    void mousePressEvent(QMouseEvent* event);

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

protected:
    void tabInserted(int index);
    void tabRemoved(int index);

private:
    TabBarPolicy m_tabBarPolicy;

};

// tree view

class TreeView : public QTreeView
{
    Q_OBJECT

public:
    explicit TreeView(QWidget* parent = 0);

public slots:
    void expandAll(const QModelIndex& index = QModelIndex());
    void collapseAll(const QModelIndex& index = QModelIndex());

protected:
    void contextMenuEvent(QContextMenuEvent* event);

};

// line edit

class LineEdit : public QLineEdit
{
    Q_OBJECT

public:
    explicit LineEdit(QWidget* parent = 0);

protected:
    void mousePressEvent(QMouseEvent* event);

};

// combo box

class ComboBox : public QComboBox
{
    Q_OBJECT

public:
    explicit ComboBox(QWidget* parent = 0);

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

};

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
    int m_progress;

};

#endif // MISCELLANEOUS_H
