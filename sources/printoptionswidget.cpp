/*

Copyright 2012 Adam Reichold

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

#include "printoptionswidget.h"

PrintOptionsWidget::PrintOptionsWidget(QWidget* parent) : QWidget(parent)
{
    setWindowTitle(tr("Extended options"));

    m_formLayout = new QFormLayout(this);

    m_fitToPageCheckBox = new QCheckBox(this);

    m_formLayout->addRow(tr("Fit to page:"), m_fitToPageCheckBox);

    m_landscapeCheckBox = new QCheckBox(this);

    m_formLayout->addRow(tr("Landscape:"), m_landscapeCheckBox);

    m_pageRangesLineEdit = new QLineEdit(this);

    m_formLayout->addRow(tr("Page ranges:"), m_pageRangesLineEdit);

    m_pageSetComboBox = new QComboBox(this);
    m_pageSetComboBox->addItem(tr("All pages"), static_cast< uint >(DocumentView::PrintOptions::AllPages));
    m_pageSetComboBox->addItem(tr("Even pages"), static_cast< uint >(DocumentView::PrintOptions::EvenPages));
    m_pageSetComboBox->addItem(tr("Odd pages"), static_cast< uint >(DocumentView::PrintOptions::OddPages));
    m_pageSetComboBox->setCurrentIndex(0);

    m_formLayout->addRow(tr("Page set:"), m_pageSetComboBox);

    m_numberUpComboBox = new QComboBox(this);
    m_numberUpComboBox->addItem(tr("Single page"), static_cast< uint >(DocumentView::PrintOptions::SinglePage));
    m_numberUpComboBox->addItem(tr("Two pages"), static_cast< uint >(DocumentView::PrintOptions::TwoPages));
    m_numberUpComboBox->addItem(tr("Four pages"), static_cast< uint >(DocumentView::PrintOptions::FourPages));
    m_numberUpComboBox->addItem(tr("Six pages"), static_cast< uint >(DocumentView::PrintOptions::SixPages));
    m_numberUpComboBox->addItem(tr("Nine pages"), static_cast< uint >(DocumentView::PrintOptions::NinePages));
    m_numberUpComboBox->addItem(tr("Sixteen pages"), static_cast< uint >(DocumentView::PrintOptions::SixteenPages));
    m_numberUpComboBox->setCurrentIndex(0);

    m_formLayout->addRow(tr("Number-up:"), m_numberUpComboBox);

    m_numberUpLayoutComboBox = new QComboBox(this);
    m_numberUpLayoutComboBox->addItem(tr("Bottom to top and left to right"), static_cast< uint >(DocumentView::PrintOptions::BottomTopLeftRight));
    m_numberUpLayoutComboBox->addItem(tr("Bottom to top and right to left"), static_cast< uint >(DocumentView::PrintOptions::BottomTopRightLeft));
    m_numberUpLayoutComboBox->addItem(tr("Left to right and bottom to top"), static_cast< uint >(DocumentView::PrintOptions::LeftRightBottomTop));
    m_numberUpLayoutComboBox->addItem(tr("Left to right and top to bottom"), static_cast< uint >(DocumentView::PrintOptions::LeftRightTopBottom));
    m_numberUpLayoutComboBox->addItem(tr("Right to left and bottom to top"), static_cast< uint >(DocumentView::PrintOptions::RightLeftBottomTop));
    m_numberUpLayoutComboBox->addItem(tr("Right to left and top to bottom"), static_cast< uint >(DocumentView::PrintOptions::RightLeftTopBottom));
    m_numberUpLayoutComboBox->addItem(tr("Top to bottom and left to right"), static_cast< uint >(DocumentView::PrintOptions::TopBottomLeftRight));
    m_numberUpLayoutComboBox->addItem(tr("Top to bottom and right to left"), static_cast< uint >(DocumentView::PrintOptions::TopBottomRightLeft));
    m_numberUpLayoutComboBox->setCurrentIndex(3);

    m_formLayout->addRow(tr("Number-up layout:"), m_numberUpLayoutComboBox);
}

DocumentView::PrintOptions PrintOptionsWidget::printOptions() const
{
    DocumentView::PrintOptions printOptions;

    printOptions.fitToPage = m_fitToPageCheckBox->isChecked();
    printOptions.landscape = m_landscapeCheckBox->isChecked();

    printOptions.pageRanges = m_pageRangesLineEdit->text();
    printOptions.pageSet = static_cast< DocumentView::PrintOptions::PageSet >(m_pageSetComboBox->itemData(m_pageSetComboBox->currentIndex()).toUInt());

    printOptions.numberUp = static_cast< DocumentView::PrintOptions::NumberUp >(m_numberUpComboBox->itemData(m_numberUpComboBox->currentIndex()).toUInt());
    printOptions.numberUpLayout = static_cast< DocumentView::PrintOptions::NumberUpLayout >(m_numberUpLayoutComboBox->itemData(m_numberUpLayoutComboBox->currentIndex()).toUInt());

    return printOptions;
}
