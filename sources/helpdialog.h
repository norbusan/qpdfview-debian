/*

Copyright 2013 Benjamin Eltzner
Copyright 2013 Adam Reichold

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

#ifndef HELPDIALOG_H
#define HELPDIALOG_H

#include <QDialog>

class QDialogButtonBox;
class QTextBrowser;
class QLineEdit;

namespace qpdfview
{

class HelpDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HelpDialog(QWidget* parent = 0);
    ~HelpDialog();

protected slots:
    void on_findPrevious_triggered();
    void on_findNext_triggered();
    void on_search_textEdited();

private:
    Q_DISABLE_COPY(HelpDialog)

    QTextBrowser* m_textBrowser;

    QDialogButtonBox* m_dialogButtonBox;

    QLineEdit* m_searchLineEdit;
    QPushButton* m_findPreviousButton;
    QPushButton* m_findNextButton;

};

} // qpdfview

#endif // HELPDIALOG_H
