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

#include "fontsdialog.h"

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QStandardItemModel>
#include <QTableView>
#include <QVBoxLayout>

#include "settings.h"

namespace qpdfview
{

FontsDialog::FontsDialog(QAbstractItemModel* model, QWidget* parent) : QDialog(parent)
{
    setWindowTitle(tr("Fonts") + QLatin1String(" - qpdfview"));

    m_tableView = new QTableView(this);
    m_tableView->setModel(model);

    m_tableView->setAlternatingRowColors(true);
    m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_tableView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

#else

    m_tableView->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    m_tableView->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);

#endif // QT_VERSION

    m_tableView->verticalHeader()->setVisible(false);

    m_dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, this);
    connect(m_dialogButtonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(m_dialogButtonBox, SIGNAL(rejected()), SLOT(reject()));

    setLayout(new QVBoxLayout(this));
    layout()->addWidget(m_tableView);
    layout()->addWidget(m_dialogButtonBox);

    resize(Settings::instance()->mainWindow().fontsDialogSize(sizeHint()));
}

FontsDialog::~FontsDialog()
{
    Settings::instance()->mainWindow().setFontsDialogSize(size());
}

} // qpdfview
