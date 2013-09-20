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

#include <QApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QLineEdit>
#include <QPushButton>
#include <QTextBrowser>
#include <QVBoxLayout>

HelpDialog::HelpDialog(QWidget* parent) : QDialog(parent)
{
    m_textBrowser = new QTextBrowser(this);
    m_textBrowser->setTextInteractionFlags(Qt::TextBrowserInteraction | Qt::TextSelectableByKeyboard);
    m_textBrowser->setSearchPaths(QStringList() << QDir(QApplication::applicationDirPath()).filePath("data") << DATA_INSTALL_PATH);

    //: Please replace by file name of localized help if available, e.g. "help_fr.html".
    m_textBrowser->setSource(QUrl(tr("help.html")));

    if(m_textBrowser->document()->isEmpty())
    {
        m_textBrowser->setSource(QUrl("help.html"));
    }

    m_dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, this);
    connect(m_dialogButtonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(m_dialogButtonBox, SIGNAL(rejected()), SLOT(reject()));

    m_searchLineEdit = new QLineEdit(this);
    connect(m_searchLineEdit, SIGNAL(returnPressed()), SLOT(on_findNext_triggered()));

    m_findPreviousButton = m_dialogButtonBox->addButton(tr("Find previous"), QDialogButtonBox::ActionRole);
    m_findPreviousButton->setShortcut(QKeySequence::FindPrevious);
    connect(m_findPreviousButton, SIGNAL(clicked()), SLOT(on_findPrevious_triggered()));

    m_findNextButton = m_dialogButtonBox->addButton(tr("Find next"), QDialogButtonBox::ActionRole);
    m_findNextButton->setShortcut(QKeySequence::FindNext);
    connect(m_findNextButton, SIGNAL(clicked()), SLOT(on_findNext_triggered()));

    setLayout(new QVBoxLayout(this));
    layout()->addWidget(m_textBrowser);
    layout()->addWidget(m_searchLineEdit);
    layout()->addWidget(m_dialogButtonBox);

    m_searchLineEdit->setFocus();
}

void HelpDialog::on_findPrevious_triggered()
{
    if(!m_searchLineEdit->text().isEmpty() && m_textBrowser->find(m_searchLineEdit->text(), QTextDocument::FindBackward))
    {
        m_textBrowser->setFocus();
    }
}

void HelpDialog::on_findNext_triggered()
{
    if(!m_searchLineEdit->text().isEmpty() && m_textBrowser->find(m_searchLineEdit->text()))
    {
        m_textBrowser->setFocus();
    }
}
