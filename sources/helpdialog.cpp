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
#include <QTextBrowser>
#include <QVBoxLayout>

HelpDialog::HelpDialog(QWidget* parent) : QDialog(parent)
{
    m_textBrowser = new QTextBrowser(this);
    m_textBrowser->setSearchPaths(QStringList() << QDir(QApplication::applicationDirPath()).filePath("data") << DATA_INSTALL_PATH);
    //: Please replace by file name of localized help if available, e.g. "help_fr.html".
    m_textBrowser->setSource(QUrl(tr("help.html")));

    m_dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, this);
    connect(m_dialogButtonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(m_dialogButtonBox, SIGNAL(rejected()), SLOT(reject()));

    setLayout(new QVBoxLayout(this));
    layout()->addWidget(m_textBrowser);
    layout()->addWidget(m_dialogButtonBox);
}
