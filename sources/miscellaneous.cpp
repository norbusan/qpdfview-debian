/*

Copyright 2012-2013 Adam Reichold

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

#include <QMenu>
#include <QMouseEvent>
#include <QTimer>

GraphicsCompositionModeEffect::GraphicsCompositionModeEffect(QPainter::CompositionMode compositionMode, QObject* parent) : QGraphicsEffect(parent),
    m_compositionMode(compositionMode)
{
}

void GraphicsCompositionModeEffect::draw(QPainter* painter)
{
    painter->save();

    painter->setCompositionMode(m_compositionMode);
    drawSource(painter);

    painter->restore();

}

TabBar::TabBar(QWidget* parent) : QTabBar(parent)
{
    setContextMenuPolicy(Qt::CustomContextMenu);
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

    connect(tabBar(), SIGNAL(customContextMenuRequested(QPoint)), SLOT(on_tabBar_customContextMenuRequested(QPoint)));
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

void TabWidget::on_tabBar_customContextMenuRequested(const QPoint &pos)
{
    emit tabContextMenuRequested(tabBar()->mapToGlobal(pos), tabBar()->tabAt(pos));
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

TreeView::TreeView(int expansionRole, QWidget* parent) : QTreeView(parent),
    m_expansionRole(expansionRole)
{
    connect(this, SIGNAL(expanded(QModelIndex)), SLOT(on_expanded(QModelIndex)));
    connect(this, SIGNAL(collapsed(QModelIndex)), SLOT(on_collapsed(QModelIndex)));
}

void TreeView::expandAbove(const QModelIndex& child)
{
    for(QModelIndex index = child.parent(); index.isValid(); index = index.parent())
    {
        expand(index);
    }
}

void TreeView::expandAll(const QModelIndex& index)
{
    if(index.isValid())
    {
        expand(index);

        for(int row = 0; row < model()->rowCount(index); ++row)
        {
            expandAll(index.child(row, 0));
        }
    }
    else
    {
        QTreeView::expandAll();
    }
}

void TreeView::collapseAll(const QModelIndex& index)
{
    if(index.isValid())
    {
        collapse(index);

        for(int row = 0; row < model()->rowCount(index); ++row)
        {
            collapseAll(index.child(row, 0));
        }
    }
    else
    {
        QTreeView::collapseAll();
    }
}

void TreeView::restoreExpansion(const QModelIndex& index)
{
    if(index.isValid())
    {
        if(model()->data(index, m_expansionRole).toBool())
        {
            expand(index);
        }
        else
        {
            collapse(index);
        }
    }

    for(int row = 0; row < model()->rowCount(index); ++row)
    {
        restoreExpansion(model()->index(row, 0, index));
    }
}

void TreeView::contextMenuEvent(QContextMenuEvent* event)
{
    QTreeView::contextMenuEvent(event);

    if(!event->isAccepted())
    {
        QMenu menu;

        const QAction* expandAllAction = menu.addAction(tr("&Expand all"));
        const QAction* collapseAllAction = menu.addAction(tr("&Collapse all"));

        const QAction* action = menu.exec(event->globalPos());

        if(action == expandAllAction)
        {
            expandAll(indexAt(event->pos()));
        }
        else if(action == collapseAllAction)
        {
            collapseAll(indexAt(event->pos()));
        }
    }
}

void TreeView::on_expanded(const QModelIndex& index)
{
    model()->setData(index, true, m_expansionRole);
}

void TreeView::on_collapsed(const QModelIndex& index)
{
    model()->setData(index, false, m_expansionRole);
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

    painter.setCompositionMode(QPainter::CompositionMode_Multiply);
    painter.fillRect(0, 0, m_progress * width() / 100, height(), palette().highlight());
}

void ProgressLineEdit::keyPressEvent(QKeyEvent* event)
{
    QLineEdit::keyPressEvent(event);

    if(event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        emit returnPressed(event->modifiers());
    }
}

// search line edit

SearchLineEdit::SearchLineEdit(QWidget* parent) : ProgressLineEdit(parent)
{
    m_timer = new QTimer(this);

    m_timer->setInterval(2000);
    m_timer->setSingleShot(true);

    connect(this, SIGNAL(textEdited(QString)), m_timer, SLOT(start()));
    connect(this, SIGNAL(returnPressed(Qt::KeyboardModifiers)), SLOT(on_returnPressed(Qt::KeyboardModifiers)));
    connect(m_timer, SIGNAL(timeout()), SLOT(on_timeout()));
}

void SearchLineEdit::startSearch()
{
    QTimer::singleShot(0, this, SLOT(on_timeout()));
}

void SearchLineEdit::stopTimer()
{
    m_timer->stop();
}

void SearchLineEdit::on_timeout()
{
    emit searchInitiated(text());
}

void SearchLineEdit::on_returnPressed(const Qt::KeyboardModifiers& modifiers)
{
    stopTimer();

    emit searchInitiated(text(), modifiers == Qt::ShiftModifier);
}
