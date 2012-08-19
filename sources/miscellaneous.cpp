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

#include "miscellaneous.h"

TabBar::TabBar(QWidget* parent) : QTabBar(parent)
{
}

void TabBar::mousePressEvent(QMouseEvent* event)
{
    QTabBar::mousePressEvent(event);

    if(event->button() == Qt::MidButton)
    {
        emit tabCloseRequested(tabAt(event->pos()));
    }
}

TabWidget::TabWidget(QWidget* parent) : QTabWidget(parent),
    m_tabBarPolicy(TabBarAsNeeded)
{
    setTabBar(new TabBar(this));
}

TabWidget::TabBarPolicy TabWidget::tabBarPolicy() const
{
    return m_tabBarPolicy;
}

void TabWidget::setTabBarPolicy(TabWidget::TabBarPolicy tabBarPolicy)
{
    m_tabBarPolicy = tabBarPolicy;

    switch(m_tabBarPolicy)
    {
    case TabBarAsNeeded:
        tabBar()->setVisible(count() > 1);
        break;
    case TabBarAlwaysOn:
        tabBar()->setVisible(true);
        break;
    case TabBarAlwaysOff:
        tabBar()->setVisible(false);
        break;
    }
}

void TabWidget::tabInserted(int index)
{
    QTabWidget::tabInserted(index);

    if(m_tabBarPolicy == TabBarAsNeeded)
    {
        tabBar()->setVisible(count() > 1);
    }
}

void TabWidget::tabRemoved(int index)
{
    QTabWidget::tabRemoved(index);

    if(m_tabBarPolicy == TabBarAsNeeded)
    {
        tabBar()->setVisible(count() > 1);
    }
}

LineEdit::LineEdit(QWidget* parent) : QLineEdit(parent)
{
}

void LineEdit::mousePressEvent(QMouseEvent* event)
{
    QLineEdit::mousePressEvent(event);

    selectAll();
}

ComboBox::ComboBox(QWidget* parent) : QComboBox(parent)
{
    setLineEdit(new LineEdit(this));
}

SpinBox::SpinBox(QWidget* parent) : QSpinBox(parent)
{
    setLineEdit(new LineEdit(this));
}

void SpinBox::keyPressEvent(QKeyEvent* event)
{
    QSpinBox::keyPressEvent(event);

    if(event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        emit returnPressed();
    }
}

ProgressLineEdit::ProgressLineEdit(QWidget* parent) : QLineEdit(parent),
    m_progress(0)
{    
}

int ProgressLineEdit::progress() const
{
    return m_progress;
}

void ProgressLineEdit::setProgress(int progress)
{
    if(m_progress != progress && progress >= 0 && progress <= 100)
    {
        m_progress = progress;

        update();
    }
}

void ProgressLineEdit::paintEvent(QPaintEvent* event)
{
    QLineEdit::paintEvent(event);

    QPainter painter(this);

    painter.setCompositionMode(QPainter::CompositionMode_Darken);
    painter.fillRect(rect().x(), rect().y(), m_progress * width() / 100, rect().height(), QApplication::palette().highlight());
}
