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

#include "formfielddialog.h"

FormFieldDialog::FormFieldDialog(QMutex* mutex, Poppler::FormField* formField, QWidget* parent) : QDialog(parent, Qt::Popup),
    m_mutex(mutex),
    m_formField(formField),
    m_widget(0)
{
    switch(m_formField->type())
    {
    case Poppler::FormField::FormButton:
    case Poppler::FormField::FormSignature:
        break;
    case Poppler::FormField::FormText:
        switch(formFieldText()->textType())
        {
        case Poppler::FormFieldText::FileSelect:
            break;
        case Poppler::FormFieldText::Normal:
            m_widget = new QLineEdit(this);

            lineEdit()->setText(formFieldText()->text());
            lineEdit()->setMaxLength(formFieldText()->maximumLength());
            lineEdit()->setAlignment(formFieldText()->textAlignment());
            lineEdit()->setEchoMode(formFieldText()->isPassword() ? QLineEdit::Password : QLineEdit::Normal);

            connect(lineEdit(), SIGNAL(returnPressed()), SLOT(close()));

            break;
        case Poppler::FormFieldText::Multiline:
            m_widget = new QPlainTextEdit(this);

            plainTextEdit()->setPlainText(formFieldText()->text());

            break;
        }

        break;
    case Poppler::FormField::FormChoice:
        switch(formFieldChoice()->choiceType())
        {
        case Poppler::FormFieldChoice::ComboBox:
            m_widget = new QComboBox(this);

            comboBox()->addItems(formFieldChoice()->choices());
            comboBox()->setEditable(formFieldChoice()->isEditable());

            if(!formFieldChoice()->currentChoices().isEmpty())
            {
                comboBox()->setCurrentIndex(formFieldChoice()->currentChoices().first());
            }

            connect(comboBox(), SIGNAL(activated(int)), SLOT(close()));

            break;
        case Poppler::FormFieldChoice::ListBox:
            m_widget = new QListWidget(this);

            listWidget()->addItems(formFieldChoice()->choices());
            listWidget()->setSelectionMode(formFieldChoice()->multiSelect() ? QAbstractItemView::MultiSelection : QAbstractItemView::SingleSelection);

            foreach(int index, formFieldChoice()->currentChoices())
            {
                if(index >= 0 && index < listWidget()->count())
                {
                    listWidget()->item(index)->setSelected(true);
                }
            }

            break;
        }

        break;
    }

    setLayout(new QVBoxLayout(this));
    layout()->setContentsMargins(QMargins());
    layout()->addWidget(m_widget);
}

void FormFieldDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);

    if(!event->spontaneous())
    {
        switch(m_formField->type())
        {
        case Poppler::FormField::FormButton:
        case Poppler::FormField::FormChoice:
        case Poppler::FormField::FormSignature:
            break;
        case Poppler::FormField::FormText:
            switch(formFieldText()->textType())
            {
            case Poppler::FormFieldText::FileSelect:
                break;
            case Poppler::FormFieldText::Normal:
                lineEdit()->selectAll();
                break;
            case Poppler::FormFieldText::Multiline:
                plainTextEdit()->moveCursor(QTextCursor::End);
                break;
            }

            break;
        }

        m_widget->setFocus();
    }
}

void FormFieldDialog::hideEvent(QHideEvent* event)
{
    QDialog::hideEvent(event);

    m_mutex->lock();

    switch(m_formField->type())
    {
    case Poppler::FormField::FormButton:
    case Poppler::FormField::FormSignature:
        break;
    case Poppler::FormField::FormText:
        switch(formFieldText()->textType())
        {
        case Poppler::FormFieldText::FileSelect:
            break;
        case Poppler::FormFieldText::Normal:
            formFieldText()->setText(lineEdit()->text());
            break;
        case Poppler::FormFieldText::Multiline:
            formFieldText()->setText(plainTextEdit()->toPlainText());
            break;
        }

        break;
    case Poppler::FormField::FormChoice:
        switch(formFieldChoice()->choiceType())
        {
        case Poppler::FormFieldChoice::ComboBox:
            formFieldChoice()->setCurrentChoices(QList< int >() << comboBox()->currentIndex());
            break;
        case Poppler::FormFieldChoice::ListBox:
            QList< int > currentChoices;

            for(int index = 0; index < listWidget()->count(); ++index)
            {
                if(listWidget()->item(index)->isSelected())
                {
                    currentChoices.append(index);
                }
            }

            formFieldChoice()->setCurrentChoices(currentChoices);

            break;
        }

        break;
    }

    m_mutex->unlock();
}

Poppler::FormFieldText* FormFieldDialog::formFieldText() const
{
    return static_cast< Poppler::FormFieldText* >(m_formField);
}

Poppler::FormFieldChoice* FormFieldDialog::formFieldChoice() const
{
    return static_cast< Poppler::FormFieldChoice* >(m_formField);
}

QLineEdit* FormFieldDialog::lineEdit() const
{
    return qobject_cast< QLineEdit* >(m_widget);
}

QPlainTextEdit* FormFieldDialog::plainTextEdit() const
{
    return qobject_cast< QPlainTextEdit* >(m_widget);
}

QComboBox* FormFieldDialog::comboBox() const
{
    return qobject_cast< QComboBox* >(m_widget);
}

QListWidget* FormFieldDialog::listWidget() const
{
    return qobject_cast< QListWidget* >(m_widget);
}
