/*

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

#ifndef FONTSDIALOG_H
#define FONTSDIALOG_H

#include <QDialog>

class QAbstractItemModel;
class QDialogButtonBox;
class QTableView;

namespace qpdfview
{

class FontsDialog : public QDialog
{
    Q_OBJECT

public:
    FontsDialog(QAbstractItemModel* model, QWidget* parent = 0);
    ~FontsDialog();

private:
    Q_DISABLE_COPY(FontsDialog)

    QTableView* m_tableView;

    QDialogButtonBox* m_dialogButtonBox;

};

} // qpdfview

#endif // FONTSDIALOG_H
