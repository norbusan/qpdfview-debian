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

#ifndef MISCELLANEOUS_H
#define MISCELLANEOUS_H

#include <QtCore>
#include <QtGui>

// recently used action

class RecentlyUsedAction : public QAction
{
    Q_OBJECT

public:
    RecentlyUsedAction(QObject *parent = 0);
    ~RecentlyUsedAction();

public slots:
    void addEntry(const QString &filePath);

    void clearList();

signals:
    void entrySelected(QString filePath);

protected slots:
    void slotActionGroupTriggered(QAction *action);

private:
    QMenu *m_menu;
    QActionGroup *m_actionGroup;
    QAction *m_separator;

    QAction *m_clearListAction;

    // settings

    QSettings m_settings;
};

// bookmarks menu

class BookmarksMenu : public QMenu
{
    Q_OBJECT

public:
    BookmarksMenu(QWidget *parent = 0);

public slots:
    void setPosition(int page, qreal top);

    void addEntry();
    void removeEntriesOnCurrentPage();

    void goToPreviousEntry();
    void goToNextEntry();

    void clearList();

signals:
    void entrySelected(int page, qreal top);

protected slots:
    void slotActionGroupTriggered(QAction *action);

private:
    QActionGroup *m_actionGroup;

    QAction *m_addEntryAction;
    QAction *m_removeEntriesOnCurrentPageAction;

    QAction *m_goToPreviousEntryAction;
    QAction *m_goToNextEntryAction;

    QAction *m_clearListAction;

    int m_page;
    qreal m_top;

};

// help dialog

class HelpDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HelpDialog(QWidget *parent = 0);

    QSize sizeHint() const
    {
        return QSize(500, 700);
    }

private:
    QTextBrowser *m_textBrowser;
    QDialogButtonBox *m_buttonBox;

};

#endif // MISCELLANEOUS_H
