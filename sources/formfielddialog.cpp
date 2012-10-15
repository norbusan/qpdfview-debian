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
    case Poppler::FormField::FormChoice:
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
    case Poppler::FormField::FormChoice:
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
    }

    m_mutex->unlock();
}

Poppler::FormFieldButton* FormFieldDialog::formFieldButton() const
{
    return static_cast< Poppler::FormFieldButton* >(m_formField);
}

Poppler::FormFieldChoice* FormFieldDialog::formFieldChoice() const
{
    return static_cast< Poppler::FormFieldChoice* >(m_formField);
}

Poppler::FormFieldText* FormFieldDialog::formFieldText() const
{
    return static_cast< Poppler::FormFieldText* >(m_formField);
}

QLineEdit* FormFieldDialog::lineEdit() const
{
    return qobject_cast< QLineEdit* >(m_widget);
}

QPlainTextEdit* FormFieldDialog::plainTextEdit() const
{
    return qobject_cast< QPlainTextEdit* >(m_widget);
}
