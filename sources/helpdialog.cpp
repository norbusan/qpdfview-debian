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

#include "helpdialog.h"

#include "miscellaneous.h"

#include <QApplication>
#include <QDebug>
#include <QDialogButtonBox>
#include <QDir>
#include <QMessageBox>
#include <QPushButton>
#include <QTextBrowser>
#include <QVBoxLayout>

HelpDialog::HelpDialog(QWidget* parent) : QDialog(parent)
{
    m_textBrowser = new QTextBrowser(this);
    m_textBrowser->setSearchPaths(QStringList() << QDir(QApplication::applicationDirPath()).filePath("data") << DATA_INSTALL_PATH);
    m_textBrowser->setSource(QUrl("help.html"));

    m_helpSearchLineEdit = new SearchLineEdit(this);
    connect(m_helpSearchLineEdit, SIGNAL(searchInitiated(QString)), SLOT(on_searchInitiated(QString)));

    m_dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, this);
    connect(m_dialogButtonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(m_dialogButtonBox, SIGNAL(rejected()), SLOT(reject()));

    m_findPreviousButton = m_dialogButtonBox->addButton(tr("Find previous"), QDialogButtonBox::ActionRole);
    connect(m_findPreviousButton, SIGNAL(clicked()), SLOT(on_findPrevious_triggered()));

    m_findNextButton = m_dialogButtonBox->addButton(tr("Find next"), QDialogButtonBox::ActionRole);
    connect(m_findNextButton, SIGNAL(clicked()), SLOT(on_findNext_triggered()));

    setLayout(new QVBoxLayout(this));
    layout()->addWidget(m_textBrowser);
    layout()->addWidget(m_helpSearchLineEdit);
    layout()->addWidget(m_dialogButtonBox);
}

void HelpDialog::on_searchInitiated(const QString& searchInput)
{
    if(!m_helpSearchLineEdit->text().isEmpty())
    {
        startSearch(searchInput);
    }
}

void HelpDialog::on_findPrevious_triggered()
{
    if(!m_helpSearchLineEdit->text().isEmpty())
    {
        findPrevious();
    }
}

void HelpDialog::on_findNext_triggered()
{
    if(!m_helpSearchLineEdit->text().isEmpty())
    {
        findNext();
    }
}

void HelpDialog::startSearch(const QString& searchTerm)
{
    // Implement search.
    //m_helpSearchLineEdit->setText(searchTerm);
    //m_helpSearchLineEdit->startSearch();
    QMessageBox msg(QMessageBox::Information, "Search", "Search started with search term: " + searchTerm, QMessageBox::Ok, this);
    msg.exec();
}

void HelpDialog::findPrevious()
{
    // Implement find previous.
    QMessageBox msg(QMessageBox::Information, "Find Previous", "Find Previous clicked", QMessageBox::Ok, this);
    msg.exec();
}

void HelpDialog::findNext()
{
    // Implement find next.
    QMessageBox msg(QMessageBox::Information, "Find Next", "Find Next clicked", QMessageBox::Ok, this);
    msg.exec();
}
