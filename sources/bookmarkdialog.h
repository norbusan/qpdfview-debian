/*

Copyright 2014 Adam Reichold

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

#ifndef BOOKMARKDIALOG_H
#define BOOKMARKDIALOG_H

#include <QDialog>

class QDialogButtonBox;
class QLineEdit;
class QTextEdit;

namespace qpdfview
{

struct BookmarkItem;

class BookmarkDialog : public QDialog
{
    Q_OBJECT

public:
    BookmarkDialog(BookmarkItem& bookmark, QWidget* parent = 0);

public slots:
    void accept();

protected:
    void showEvent(QShowEvent*);

private:
    Q_DISABLE_COPY(BookmarkDialog)

    BookmarkItem& m_bookmark;

    QLineEdit* m_pageEdit;
    QLineEdit* m_labelEdit;
    QTextEdit* m_commentEdit;
    QLineEdit* m_modifiedEdit;

    QDialogButtonBox* m_dialogButtonBox;

};

} // qpdfview

#endif // BOOKMARKDIALOG_H
