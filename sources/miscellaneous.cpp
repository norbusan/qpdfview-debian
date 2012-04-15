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

#include "miscellaneous.h"

// recently used action

RecentlyUsedAction::RecentlyUsedAction(QObject *parent) : QAction(tr("Recently &used"), parent),
    m_settings()
{
    m_menu = new QMenu();
    this->setMenu(m_menu);

    m_actionGroup = new QActionGroup(this);
    connect(m_actionGroup, SIGNAL(triggered(QAction*)), this, SLOT(slotActionGroupTriggered(QAction*)));

    m_clearListAction = new QAction(tr("&Clear list"), this);
    connect(m_clearListAction, SIGNAL(triggered()), this, SLOT(clearList()));

    m_separator = m_menu->addSeparator();
    m_menu->addAction(m_clearListAction);

    QStringList filePaths = m_settings.value("mainWindow/recentlyUsed").toStringList();

    foreach(QString filePath, filePaths)
    {
        QAction *action = new QAction(this);
        action->setText(filePath);
        action->setData(filePath);

        m_actionGroup->addAction(action);
        m_menu->insertAction(m_separator, action);
    }
}

RecentlyUsedAction::~RecentlyUsedAction()
{
    QStringList filePaths;

    foreach(QAction *action, m_actionGroup->actions())
    {
        filePaths.append(action->data().toString());
    }

    m_settings.setValue("mainWindow/recentlyUsed", filePaths);

    delete m_menu;
}

void RecentlyUsedAction::addEntry(const QString &filePath)
{
    bool addItem = true;

    foreach(QAction *action, m_actionGroup->actions())
    {
        addItem = addItem && action->data().toString() != filePath;
    }

    if(addItem)
    {
        QAction *action = new QAction(this);
        action->setText(filePath);
        action->setData(filePath);

        m_actionGroup->addAction(action);
        m_menu->insertAction(m_separator, action);
    }

    if(m_actionGroup->actions().size() > 5)
    {
        QAction *first = m_actionGroup->actions().first();

        m_actionGroup->removeAction(first);
        m_menu->removeAction(first);
    }
}

void RecentlyUsedAction::clearList()
{
    foreach(QAction *action, m_actionGroup->actions())
    {
        m_actionGroup->removeAction(action);
        m_menu->removeAction(action);
    }
}

void RecentlyUsedAction::slotActionGroupTriggered(QAction *action)
{
    emit entrySelected(action->data().toString());
}

// bookmarks menu

BookmarksMenu::BookmarksMenu(QWidget *parent) : QMenu(tr("Bookmarks"), parent),
    m_page(1),
    m_top(0.0)
{
    m_actionGroup = new QActionGroup(this);
    connect(m_actionGroup, SIGNAL(triggered(QAction*)), this, SLOT(slotActionGroupTriggered(QAction*)));

    m_addEntryAction = new QAction(tr("&Add entry"), this);
    m_addEntryAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_B));
    connect(m_addEntryAction, SIGNAL(triggered()), this, SLOT(addEntry()));

    m_removeEntriesOnCurrentPageAction = new QAction(tr("&Remove entries on current page"), this);
    m_removeEntriesOnCurrentPageAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_B));
    connect(m_removeEntriesOnCurrentPageAction, SIGNAL(triggered()), this, SLOT(removeEntriesOnCurrentPage()));

    m_goToPreviousEntryAction = new QAction(tr("Go to &previous entry"), this);
    m_goToPreviousEntryAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_PageUp));
    connect(m_goToPreviousEntryAction, SIGNAL(triggered()), this, SLOT(goToPreviousEntry()));

    m_goToNextEntryAction = new QAction(tr("Go to &next entry"), this);
    m_goToNextEntryAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_PageDown));
    connect(m_goToNextEntryAction, SIGNAL(triggered()), this, SLOT(goToNextEntry()));

    m_clearListAction = new QAction(tr("&Clear list"), this);
    m_clearListAction->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_B));
    connect(m_clearListAction, SIGNAL(triggered()), this, SLOT(clearList()));

    this->addAction(m_addEntryAction);
    this->addAction(m_removeEntriesOnCurrentPageAction);
    this->addAction(m_goToPreviousEntryAction);
    this->addAction(m_goToNextEntryAction);
    this->addSeparator();
    this->addAction(m_clearListAction);
    this->addSeparator();
}

void BookmarksMenu::addEntry()
{
    qreal position = m_page + m_top;

    bool addItem = true;
    QAction *before = 0;

    foreach(QAction *action, m_actionGroup->actions())
    {
        if(action->data().toReal() == position)
        {
            addItem = false;

            break;
        }
        else if(action->data().toReal() > position)
        {
            if(before)
            {
                before = action->data().toReal() < before->data().toReal() ? action : before;
            }
            else
            {
                before = action;
            }
        }
    }

    if(addItem)
    {
        QAction *action = new QAction(this);
        action->setText(tr("Page %1 at %2%").arg(m_page).arg(qFloor(100.0 * m_top)));
        action->setData(position);

        m_actionGroup->addAction(action);
        this->insertAction(before, action);
    }

    if(m_actionGroup->actions().size() > 10)
    {
        QAction *first = m_actionGroup->actions().first();

        m_actionGroup->removeAction(first);
        this->removeAction(first);
    }
}

void BookmarksMenu::removeEntriesOnCurrentPage()
{
    foreach(QAction *action, m_actionGroup->actions())
    {
        if(qFloor(action->data().toReal()) == m_page)
        {
            m_actionGroup->removeAction(action);
            this->removeAction(action);
        }
    }
}

void BookmarksMenu::goToPreviousEntry()
{
    qreal position = m_page + m_top;

    QAction *previous = 0;

    foreach(QAction *action, m_actionGroup->actions())
    {
        if(action->data().toReal() < position)
        {
            if(previous)
            {
                previous = action->data().toReal() > previous->data().toReal() ? action : previous;
            }
            else
            {
                previous = action;
            }
        }
    }

    if(previous)
    {
        previous->trigger();
    }
    else if(!m_actionGroup->actions().isEmpty())
    {
        QAction *last = m_actionGroup->actions().last();

        foreach(QAction *action, m_actionGroup->actions())
        {
            if(action->data().toReal() > last->data().toReal())
            {
                last = action;
            }
        }

        last->trigger();
    }
}

void BookmarksMenu::goToNextEntry()
{
    qreal position = m_page + m_top;

    QAction *next = 0;

    foreach(QAction *action, m_actionGroup->actions())
    {
        if(action->data().toReal() > position)
        {
            if(next)
            {
                next = action->data().toReal() < next->data().toReal() ? action : next;
            }
            else
            {
                next = action;
            }
        }
    }

    if(next)
    {
        next->trigger();
    }
    else if(!m_actionGroup->actions().isEmpty())
    {
        QAction *first = m_actionGroup->actions().first();

        foreach(QAction *action, m_actionGroup->actions())
        {
            if(action->data().toReal() < first->data().toReal())
            {
                first = action;
            }
        }

        first->trigger();
    }
}

void BookmarksMenu::clearList()
{
    foreach(QAction *action, m_actionGroup->actions())
    {
        m_actionGroup->removeAction(action);
        this->removeAction(action);
    }
}

void BookmarksMenu::setPosition(int page, qreal top)
{
    m_page = page;
    m_top = top;
}

void BookmarksMenu::slotActionGroupTriggered(QAction *action)
{
    qreal position = action->data().toReal();

    int page = qFloor(position);
    double top = position - qFloor(position);

    emit entrySelected(page, top);
}

// help dialog

HelpDialog::HelpDialog(QWidget *parent) : QDialog(parent)
{
    m_textBrowser = new QTextBrowser(this);
#ifdef DATA_INSTALL_PATH
    m_textBrowser->setSource(QUrl(QString(DATA_INSTALL_PATH) + "/help.html"));
#else
    m_textBrowser->setSource(QUrl("qrc:/miscellaneous/help.html"));
#endif

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, this);
    connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    this->setLayout(new QVBoxLayout());
    this->layout()->addWidget(m_textBrowser);
    this->layout()->addWidget(m_buttonBox);
}
