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

    if(event->button() == Qt::MiddleButton)
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

ProgressLineEdit::ProgressLineEdit(QWidget* parent) : QLineEdit(parent),
    m_progress(0)
{
    QPalette p = palette();
    p.setColor(QPalette::Base, Qt::transparent);
    setPalette(p);
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
    QPainter painter;
    painter.begin(this);

    QRect r1 = rect();
    r1.setWidth(m_progress * width() / 100);

    painter.fillRect(r1, QApplication::palette().highlight());

    QRect r2 = rect();
    r2.setLeft(m_progress * width() / 100);
    r2.setWidth((100 - m_progress) * width() / 100);

    painter.fillRect(r2, QApplication::palette().base());

    painter.end();

    QLineEdit::paintEvent(event);
}

Bookmark::Bookmark(const QString& filePath, QWidget* parent) : QMenu(parent)
{
    QFileInfo fileInfo(filePath);

    menuAction()->setText(fileInfo.completeBaseName());
    menuAction()->setToolTip(fileInfo.absoluteFilePath());

    menuAction()->setData(filePath);

    m_removeBookmarkAction = new QAction(tr("&Remove bookmark"), this);
    connect(m_removeBookmarkAction, SIGNAL(triggered()), SLOT(on_removeBookmark_triggered()));

    m_openAction = new QAction(tr("&Open"), this);
    m_openAction->setIcon(QIcon::fromTheme("document-open", QIcon(":icons/document-open.svg")));
    m_openAction->setIconVisibleInMenu(true);
    connect(m_openAction, SIGNAL(triggered()), SLOT(on_open_triggered()));

    m_openInNewTabAction = new QAction(tr("Open in new &tab"), this);
    m_openInNewTabAction->setIcon(QIcon::fromTheme("tab-new", QIcon(":icons/tab-new.svg")));
    m_openInNewTabAction->setIconVisibleInMenu(true);
    connect(m_openInNewTabAction, SIGNAL(triggered()), SLOT(on_openInNewTab_triggered()));

    addAction(m_removeBookmarkAction);
    addSeparator();
    addAction(m_openAction);
    addAction(m_openInNewTabAction);

    m_jumpToPageGroup = new QActionGroup(this);
    connect(m_jumpToPageGroup, SIGNAL(triggered(QAction*)), SLOT(on_jumpToPage_triggered(QAction*)));
}

void Bookmark::addJumpToPage(int page)
{
    QAction* before = 0;

    foreach(QAction* action, m_jumpToPageGroup->actions())
    {
        if(action->data().toInt() == page)
        {
            return;
        }
        else if(action->data().toInt() > page)
        {
            before = action;

            break;
        }
    }

    QAction* action = new QAction(tr("Jump to page %1").arg(page), this);
    action->setIcon(QIcon::fromTheme("go-jump", QIcon(":icons/go-jump.svg")));
    action->setIconVisibleInMenu(true);
    action->setData(page);

    insertAction(before, action);
    m_jumpToPageGroup->addAction(action);
}

void Bookmark::removeJumpToPage(int page)
{
    foreach(QAction* action, m_jumpToPageGroup->actions())
    {
        if(action->data().toInt() == page)
        {
            action->deleteLater();

            break;
        }
    }
}

QString Bookmark::filePath() const
{
    return menuAction()->data().toString();
}

QList< int > Bookmark::pages() const
{
    QList< int > pages;

    foreach(QAction* action, m_jumpToPageGroup->actions())
    {
        pages.append(action->data().toInt());
    }

    return pages;
}

void Bookmark::on_removeBookmark_triggered()
{
    deleteLater();
}

void Bookmark::on_open_triggered()
{
    emit openTriggered(filePath());
}

void Bookmark::on_openInNewTab_triggered()
{
    emit openInNewTabTriggered(filePath());
}

void Bookmark::on_jumpToPage_triggered(QAction* action)
{
    emit jumpToPageTriggered(filePath(), action->data().toInt());
}
