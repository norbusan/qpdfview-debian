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

#include "printdialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QLineEdit>

#include "settings.h"

namespace qpdfview
{

Settings* PrintDialog::s_settings = 0;

QPrinter* PrintDialog::createPrinter()
{
    QPrinter* printer = new QPrinter();

    const Settings::PrintDialog& settings = Settings::instance()->printDialog();
    printer->setCollateCopies(settings.collateCopies());
    printer->setPageOrder(settings.pageOrder());
    printer->setOrientation(settings.orientation());
    printer->setColorMode(settings.colorMode());
    printer->setDuplex(settings.duplex());

    return printer;
}

PrintDialog::PrintDialog(QPrinter* printer, QWidget* parent) : QPrintDialog(printer, parent)
{
    if(s_settings == 0)
    {
        s_settings = Settings::instance();
    }

    // print options

    m_printOptionsWidget = new QWidget(this);
    m_printOptionsLayout = new QFormLayout(m_printOptionsWidget);

    m_fitToPageCheckBox = new QCheckBox(this);
    m_fitToPageCheckBox->setChecked(s_settings->printDialog().fitToPage());

    m_printOptionsLayout->addRow(tr("Fit to page:"), m_fitToPageCheckBox);

    m_pageRangesLineEdit = new QLineEdit(this);

    m_printOptionsLayout->addRow(tr("Page ranges:"), m_pageRangesLineEdit);

#if QT_VERSION < QT_VERSION_CHECK(5,2,0)

    m_pageSetComboBox = new QComboBox(this);
    m_pageSetComboBox->addItem(tr("All pages"), static_cast< uint >(PrintOptions::AllPages));
    m_pageSetComboBox->addItem(tr("Even pages"), static_cast< uint >(PrintOptions::EvenPages));
    m_pageSetComboBox->addItem(tr("Odd pages"), static_cast< uint >(PrintOptions::OddPages));
    m_pageSetComboBox->setCurrentIndex(m_pageSetComboBox->findData(static_cast< uint >(s_settings->printDialog().pageSet())));

    m_printOptionsLayout->addRow(tr("Page set:"), m_pageSetComboBox);

    m_numberUpComboBox = new QComboBox(this);
    m_numberUpComboBox->addItem(tr("Single page"), static_cast< uint >(PrintOptions::SinglePage));
    m_numberUpComboBox->addItem(tr("Two pages"), static_cast< uint >(PrintOptions::TwoPages));
    m_numberUpComboBox->addItem(tr("Four pages"), static_cast< uint >(PrintOptions::FourPages));
    m_numberUpComboBox->addItem(tr("Six pages"), static_cast< uint >(PrintOptions::SixPages));
    m_numberUpComboBox->addItem(tr("Nine pages"), static_cast< uint >(PrintOptions::NinePages));
    m_numberUpComboBox->addItem(tr("Sixteen pages"), static_cast< uint >(PrintOptions::SixteenPages));
    m_numberUpComboBox->setCurrentIndex(m_numberUpComboBox->findData(static_cast< uint >(s_settings->printDialog().numberUp())));

    m_printOptionsLayout->addRow(tr("Number-up:"), m_numberUpComboBox);

    m_numberUpLayoutComboBox = new QComboBox(this);
    m_numberUpLayoutComboBox->addItem(tr("Bottom to top and left to right"), static_cast< uint >(PrintOptions::BottomTopLeftRight));
    m_numberUpLayoutComboBox->addItem(tr("Bottom to top and right to left"), static_cast< uint >(PrintOptions::BottomTopRightLeft));
    m_numberUpLayoutComboBox->addItem(tr("Left to right and bottom to top"), static_cast< uint >(PrintOptions::LeftRightBottomTop));
    m_numberUpLayoutComboBox->addItem(tr("Left to right and top to bottom"), static_cast< uint >(PrintOptions::LeftRightTopBottom));
    m_numberUpLayoutComboBox->addItem(tr("Right to left and bottom to top"), static_cast< uint >(PrintOptions::RightLeftBottomTop));
    m_numberUpLayoutComboBox->addItem(tr("Right to left and top to bottom"), static_cast< uint >(PrintOptions::RightLeftTopBottom));
    m_numberUpLayoutComboBox->addItem(tr("Top to bottom and left to right"), static_cast< uint >(PrintOptions::TopBottomLeftRight));
    m_numberUpLayoutComboBox->addItem(tr("Top to bottom and right to left"), static_cast< uint >(PrintOptions::TopBottomRightLeft));
    m_numberUpLayoutComboBox->setCurrentIndex(m_numberUpLayoutComboBox->findData(static_cast< uint >(s_settings->printDialog().numberUpLayout())));

    m_printOptionsLayout->addRow(tr("Number-up layout:"), m_numberUpLayoutComboBox);

#endif // QT_VERSION

    m_printOptionsWidget->setWindowTitle(tr("Extended options"));
    setOptionTabs(QList< QWidget* >() << m_printOptionsWidget);
}

PrintOptions PrintDialog::printOptions() const
{
    PrintOptions printOptions;

    printOptions.fitToPage = m_fitToPageCheckBox->isChecked();

    printOptions.pageRanges = m_pageRangesLineEdit->text();

#if QT_VERSION < QT_VERSION_CHECK(5,2,0)

    printOptions.pageSet = static_cast< PrintOptions::PageSet >(m_pageSetComboBox->itemData(m_pageSetComboBox->currentIndex()).toUInt());

    printOptions.numberUp = static_cast< PrintOptions::NumberUp >(m_numberUpComboBox->itemData(m_numberUpComboBox->currentIndex()).toUInt());
    printOptions.numberUpLayout = static_cast< PrintOptions::NumberUpLayout >(m_numberUpLayoutComboBox->itemData(m_numberUpLayoutComboBox->currentIndex()).toUInt());

#endif // QT_VERSION

    return printOptions;
}

void PrintDialog::accept()
{
    QPrintDialog::accept();

    s_settings->printDialog().setCollateCopies(printer()->collateCopies());
    s_settings->printDialog().setPageOrder(printer()->pageOrder());
    s_settings->printDialog().setOrientation(printer()->orientation());
    s_settings->printDialog().setColorMode(printer()->colorMode());
    s_settings->printDialog().setDuplex(printer()->duplex());

    s_settings->printDialog().setFitToPage(m_fitToPageCheckBox->isChecked());

#if QT_VERSION < QT_VERSION_CHECK(5,2,0)

    s_settings->printDialog().setPageSet(static_cast< PrintOptions::PageSet >(m_pageSetComboBox->itemData(m_pageSetComboBox->currentIndex()).toUInt()));

    s_settings->printDialog().setNumberUp(static_cast< PrintOptions::NumberUp >(m_numberUpComboBox->itemData(m_numberUpComboBox->currentIndex()).toUInt()));
    s_settings->printDialog().setNumberUpLayout(static_cast< PrintOptions::NumberUpLayout >(m_numberUpLayoutComboBox->itemData(m_numberUpLayoutComboBox->currentIndex()).toUInt()));

#endif // QT_VERSION
}

} // qpdfview
