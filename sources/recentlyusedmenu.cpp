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

#include "recentlyusedmenu.h"

RecentlyUsedMenu::RecentlyUsedMenu(QWidget* parent) : QMenu(parent)
{
    menuAction()->setText(tr("Recently &used"));
    menuAction()->setIcon(QIcon::fromTheme("document-open-recent"));
    menuAction()->setIconVisibleInMenu(true);

    m_openActionGroup = new QActionGroup(this);
    connect(m_openActionGroup, SIGNAL(triggered(QAction*)), SLOT(on_open_triggered(QAction*)));

    m_separatorAction = addSeparator();

    m_clearListAction = addAction(tr("&Clear list"));
    connect(m_clearListAction, SIGNAL(triggered()), SLOT(on_clearList_triggered()));
}

void RecentlyUsedMenu::addOpenAction(const QString& filePath)
{
    foreach(QAction* action, m_openActionGroup->actions())
    {
        if(action->data().toString() == QFileInfo(filePath).absoluteFilePath())
        {
            removeAction(action);
            m_openActionGroup->removeAction(action);

            insertAction(actions().first(), action);
            m_openActionGroup->addAction(action);

            return;
        }
    }

    if(m_openActionGroup->actions().count() == 10)
    {
        QAction* first = m_openActionGroup->actions().first();

        removeAction(first);
        m_openActionGroup->removeAction(first);

        delete first;
    }

    QFileInfo fileInfo(filePath);

    QAction* action = new QAction(fileInfo.completeBaseName(), this);
    action->setToolTip(fileInfo.absoluteFilePath());
    action->setData(fileInfo.absoluteFilePath());

    insertAction(actions().first(), action);
    m_openActionGroup->addAction(action);
}

void RecentlyUsedMenu::removeOpenAction(const QString& filePath)
{
    foreach(QAction* action, m_openActionGroup->actions())
    {
        if(action->data().toString() == QFileInfo(filePath).absoluteFilePath())
        {
            delete action;

            break;
        }
    }
}

QStringList RecentlyUsedMenu::filePaths() const
{
    QStringList filePaths;

    foreach(QAction* action, m_openActionGroup->actions())
    {
        filePaths.append(action->data().toString());
    }

    return filePaths;
}

void RecentlyUsedMenu::on_open_triggered(QAction* action)
{
    emit openTriggered(action->data().toString());
}

void RecentlyUsedMenu::on_clearList_triggered()
{
    qDeleteAll(m_openActionGroup->actions());
}
