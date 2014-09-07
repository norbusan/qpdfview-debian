/*

Copyright 2014 S. Razi Alavizadeh
Copyright 2012-2014 Adam Reichold

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

#include <QFileInfo>

namespace qpdfview
{

BookmarkMenu::BookmarkMenu(const QFileInfo& fileInfo, QWidget* parent) : QMenu(parent)
{
    menuAction()->setText(fileInfo.completeBaseName());
    menuAction()->setToolTip(fileInfo.absoluteFilePath());

    menuAction()->setData(fileInfo.absoluteFilePath());

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

    m_separatorAction = addSeparator();

    m_removeBookmarkAction = addAction(tr("&Remove bookmark"));
    connect(m_removeBookmarkAction, SIGNAL(triggered()), SLOT(on_removeBookmark_triggered()));
}

void BookmarkMenu::addJumpToPageAction(int page, const QString& label)
{
    QAction* before = m_separatorAction;

    foreach(QAction* action, m_jumpToPageActionGroup->actions())
    {
        if(action->data().toInt() == page)
        {
            action->setText(label);

            return;
        }
        else if(action->data().toInt() > page)
        {
            before = action;

            break;
        }
    }

    QAction* action = new QAction(label, this);
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

QString BookmarkMenu::absoluteFilePath() const
{
    return menuAction()->data().toString();
}

JumpList BookmarkMenu::jumps() const
{
    JumpList jumps;

    foreach(const QAction* action, m_jumpToPageActionGroup->actions())
    {
        jumps.append(qMakePair(action->data().toInt(), action->text()));
    }

    return jumps;
}

void BookmarkMenu::on_open_triggered()
{
    emit openTriggered(absoluteFilePath());
}

void BookmarkMenu::on_openInNewTab_triggered()
{
    emit openInNewTabTriggered(absoluteFilePath());
}

void BookmarkMenu::on_jumpToPage_triggered(QAction* action)
{
    emit jumpToPageTriggered(absoluteFilePath(), action->data().toInt());
}

void BookmarkMenu::on_removeBookmark_triggered()
{
    emit removeBookmarkTriggered(absoluteFilePath());
}

} // qpdfview
