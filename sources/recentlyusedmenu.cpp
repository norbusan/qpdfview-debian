/*

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

#include "recentlyusedmenu.h"

#include <QFileInfo>

namespace qpdfview
{

RecentlyUsedMenu::RecentlyUsedMenu(const QStringList& absoluteFilePaths, int count, QWidget* parent) : ToolTipMenu(parent),
    m_count(count)
{
    menuAction()->setText(tr("Recently &used"));
    menuAction()->setIcon(QIcon::fromTheme("document-open-recent"));
    menuAction()->setIconVisibleInMenu(true);

    m_openActionGroup = new QActionGroup(this);
    connect(m_openActionGroup, SIGNAL(triggered(QAction*)), SLOT(on_open_triggered(QAction*)));

    m_separatorAction = addSeparator();

    m_clearListAction = addAction(tr("&Clear list"));
    connect(m_clearListAction, SIGNAL(triggered()), SLOT(on_clearList_triggered()));

    foreach(const QString& absoluteFilePath, absoluteFilePaths)
    {
        addOpenAction(QFileInfo(absoluteFilePath));
    }
}

void RecentlyUsedMenu::addOpenAction(const QFileInfo& fileInfo)
{
    foreach(QAction* action, m_openActionGroup->actions())
    {
        if(action->data().toString() == fileInfo.absoluteFilePath())
        {
            removeAction(action);
            m_openActionGroup->removeAction(action);

            insertAction(actions().at(0), action);
            m_openActionGroup->addAction(action);

            return;
        }
    }

    if(m_openActionGroup->actions().count() >= m_count)
    {
        QAction* first = m_openActionGroup->actions().at(0);

        removeAction(first);
        m_openActionGroup->removeAction(first);

        delete first;
    }

    QAction* action = new QAction(fileInfo.completeBaseName(), this);
    action->setToolTip(fileInfo.absoluteFilePath());
    action->setData(fileInfo.absoluteFilePath());

    insertAction(actions().at(0), action);
    m_openActionGroup->addAction(action);
}

void RecentlyUsedMenu::removeOpenAction(const QString& filePath)
{
    const QFileInfo fileInfo(filePath);

    foreach(QAction* action, m_openActionGroup->actions())
    {
        if(action->data().toString() == fileInfo.absoluteFilePath())
        {
            delete action;

            break;
        }
    }
}

QStringList RecentlyUsedMenu::filePaths() const
{
    QStringList filePaths;

    foreach(const QAction* action, m_openActionGroup->actions())
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

} // qpdfview
