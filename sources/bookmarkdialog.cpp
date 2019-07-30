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

#include "bookmarkdialog.h"

#include <QDateTime>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QTextEdit>

#include "global.h"
#include "bookmarkmodel.h"

namespace qpdfview
{

BookmarkDialog::BookmarkDialog(BookmarkItem& bookmark, QWidget* parent) : QDialog(parent),
    m_bookmark(bookmark)
{
    setWindowTitle(tr("Bookmark"));

    QFormLayout* formLayout = new QFormLayout(this);
    setLayout(formLayout);

    m_pageEdit = new QLineEdit(this);
    m_pageEdit->setReadOnly(true);
    m_pageEdit->setText(QString::number(m_bookmark.page));

    formLayout->addRow(tr("Page:"), m_pageEdit);

    m_labelEdit = new QLineEdit(this);
    m_labelEdit->setText(m_bookmark.label);

    formLayout->addRow(tr("Label:"), m_labelEdit);

    m_commentEdit = new QTextEdit(this);
    m_commentEdit->setPlainText(m_bookmark.comment);

    formLayout->addRow(tr("Comment:"), m_commentEdit);

    m_modifiedEdit = new QLineEdit(this);
    m_modifiedEdit->setReadOnly(true);
    m_modifiedEdit->setText(m_bookmark.modified.toString(Qt::SystemLocaleLongDate));

    formLayout->addRow(tr("Modified:"), m_modifiedEdit);

    m_dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    connect(m_dialogButtonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(m_dialogButtonBox, SIGNAL(rejected()), SLOT(reject()));

    formLayout->addWidget(m_dialogButtonBox);
}

void BookmarkDialog::accept()
{
    QDialog::accept();

    m_bookmark.label = m_labelEdit->text();
    m_bookmark.comment = m_commentEdit->toPlainText();

    m_bookmark.modified = QDateTime::currentDateTime();
}

void BookmarkDialog::showEvent(QShowEvent*)
{
    m_labelEdit->setFocus();
}

} // qpdfview
