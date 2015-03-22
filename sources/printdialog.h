/*

Copyright 2012-2013 Adam Reichold

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

#ifndef PRINTDIALOG_H
#define PRINTDIALOG_H

#include <QPrintDialog>

class QCheckBox;
class QComboBox;
class QFormLayout;
class QLineEdit;
class QPrinter;

namespace qpdfview
{

struct PrintOptions;
class Settings;

class PrintDialog : public QPrintDialog
{
    Q_OBJECT

public:
    static QPrinter* createPrinter();

    PrintDialog(QPrinter* printer, QWidget* parent = 0);

    PrintOptions printOptions() const;

public slots:
    void accept();

private:
    Q_DISABLE_COPY(PrintDialog)

    static Settings* s_settings;

    QWidget* m_printOptionsWidget;
    QFormLayout* m_printOptionsLayout;

    QCheckBox* m_fitToPageCheckBox;

    QLineEdit* m_pageRangesLineEdit;

#if QT_VERSION < QT_VERSION_CHECK(5,2,0)

    QComboBox* m_pageSetComboBox;

    QComboBox* m_numberUpComboBox;
    QComboBox* m_numberUpLayoutComboBox;

#endif // QT_VERSION

};

} // qpdfview

#endif // PRINTDIALOG_H
