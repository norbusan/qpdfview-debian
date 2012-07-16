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

#include "bookmarkmenu.h"

BookmarkMenu::BookmarkMenu(const QString& filePath, QWidget* parent) : QMenu(parent)
{
    QFileInfo fileInfo(filePath);

    menuAction()->setText(fileInfo.completeBaseName());
    menuAction()->setToolTip(fileInfo.absoluteFilePath());

    menuAction()->setData(filePath);

    m_removeBookmarkAction = addAction(tr("&Remove bookmark"));
    connect(m_removeBookmarkAction, SIGNAL(triggered()), SLOT(on_removeBookmark_triggered()));

    addSeparator();

    m_openAction = addAction(tr("&Open"));
    m_openAction->setIcon(QIcon::fromTheme("document-open", QIcon(":icons/document-open.svg")));
    m_openAction->setIconVisibleInMenu(true);
    connect(m_openAction, SIGNAL(triggered()), SLOT(on_open_triggered()));

    m_openInNewTabAction = addAction(tr("Open in new &tab"));
    m_openInNewTabAction->setIcon(QIcon::fromTheme("tab-new", QIcon(":icons/tab-new.svg")));
    m_openInNewTabAction->setIconVisibleInMenu(true);
    connect(m_openInNewTabAction, SIGNAL(triggered()), SLOT(on_openInNewTab_triggered()));

    m_jumpToPageActionGroup = new QActionGroup(this);
    connect(m_jumpToPageActionGroup, SIGNAL(triggered(QAction*)), SLOT(on_jumpToPage_triggered(QAction*)));
}

void BookmarkMenu::addJumpToPageAction(int page)
{
    QAction* before = 0;

    foreach(QAction* action, m_jumpToPageActionGroup->actions())
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
    m_jumpToPageActionGroup->addAction(action);
}

void BookmarkMenu::removeJumpToPageAction(int page)
{
    foreach(QAction* action, m_jumpToPageActionGroup->actions())
    {
        if(action->data().toInt() == page)
        {
            delete action;

            break;
        }
    }
}

QString BookmarkMenu::filePath() const
{
    return menuAction()->data().toString();
}

QList< int > BookmarkMenu::pages() const
{
    QList< int > pages;

    foreach(QAction* action, m_jumpToPageActionGroup->actions())
    {
        pages.append(action->data().toInt());
    }

    return pages;
}

void BookmarkMenu::on_removeBookmark_triggered()
{
    deleteLater();
}

void BookmarkMenu::on_open_triggered()
{
    emit openTriggered(filePath());
}

void BookmarkMenu::on_openInNewTab_triggered()
{
    emit openInNewTabTriggered(filePath());
}

void BookmarkMenu::on_jumpToPage_triggered(QAction* action)
{
    emit jumpToPageTriggered(filePath(), action->data().toInt());
}
