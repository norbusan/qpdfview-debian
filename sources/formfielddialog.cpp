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

#include <QComboBox>
#include <QLineEdit>
#include <QListWidget>
#include <QMutex>
#include <QPlainTextEdit>
#include <QVBoxLayout>

#include <poppler-form.h>

class FormFieldHandler
{
public:
    virtual ~FormFieldHandler() {}

    virtual QWidget* widget() const = 0;

    virtual void showWidget() {}
    virtual void hideWidget() {}

};

class NormalTextFieldHandler : public FormFieldHandler
{
public:
    NormalTextFieldHandler(Poppler::FormFieldText* formField, FormFieldDialog* dialog) : m_formField(formField)
    {
        m_lineEdit = new QLineEdit(dialog);

        m_lineEdit->setText(m_formField->text());
        m_lineEdit->setMaxLength(m_formField->maximumLength());
        m_lineEdit->setAlignment(m_formField->textAlignment());
        m_lineEdit->setEchoMode(m_formField->isPassword() ? QLineEdit::Password : QLineEdit::Normal);

        QObject::connect(m_lineEdit, SIGNAL(returnPressed()), dialog, SLOT(close()));
    }

    QWidget* widget() const { return m_lineEdit; }

    void showWidget()
    {
        m_lineEdit->selectAll();
    }

    void hideWidget()
    {
        m_formField->setText(m_lineEdit->text());
    }

private:
    Poppler::FormFieldText* m_formField;

    QLineEdit* m_lineEdit;

};

class MultilineTextFieldHandler : public FormFieldHandler
{
public:
    MultilineTextFieldHandler(Poppler::FormFieldText* formField, FormFieldDialog* dialog) : m_formField(formField)
    {
        m_plainTextEdit = new QPlainTextEdit(dialog);

        m_plainTextEdit->setPlainText(m_formField->text());

        dialog->setSizeGripEnabled(true);
    }

    QWidget* widget() const { return m_plainTextEdit; }

    void showWidget()
    {
        m_plainTextEdit->moveCursor(QTextCursor::End);
    }

    void hideWidget()
    {
        m_formField->setText(m_plainTextEdit->toPlainText());
    }

private:
    Poppler::FormFieldText* m_formField;

    QPlainTextEdit* m_plainTextEdit;

};

class ComboBoxChoiceFieldHandler : public FormFieldHandler
{
public:
    ComboBoxChoiceFieldHandler(Poppler::FormFieldChoice* formField, FormFieldDialog* dialog) : m_formField(formField)
    {
        m_comboBox = new QComboBox(dialog);

        m_comboBox->addItems(m_formField->choices());

        if(!m_formField->currentChoices().isEmpty())
        {
            m_comboBox->setCurrentIndex(m_formField->currentChoices().first());
        }

#ifdef HAS_POPPLER_22

        if(m_formField->isEditable())
        {
            m_comboBox->setEditable(true);
            m_comboBox->setInsertPolicy(QComboBox::NoInsert);

            m_comboBox->lineEdit()->setText(m_formField->editChoice());

            QObject::connect(m_comboBox->lineEdit(), SIGNAL(returnPressed()), dialog, SLOT(close()));
        }
        else
        {
            QObject::connect(m_comboBox, SIGNAL(activated(int)), dialog, SLOT(close()));
        }

#else

        QObject::connect(m_comboBox, SIGNAL(activated(int)), dialog, SLOT(close()));

#endif // HAS_POPPLER_22

    }

    QWidget* widget() const { return m_comboBox; }

    void showWidget()
    {
#ifdef HAS_POPPLER_22

            if(m_formField->isEditable())
            {
                m_comboBox->lineEdit()->selectAll();
            }

#endif // HAS_POPPLER_22
    }

    void hideWidget()
    {
        m_formField->setCurrentChoices(QList< int >() << m_comboBox->currentIndex());

#ifdef HAS_POPPLER_22

        if(m_formField->isEditable())
        {
            m_formField->setEditChoice(m_comboBox->lineEdit()->text());
        }

#endif // HAS_POPPLER_22
    }

private:
    Poppler::FormFieldChoice* m_formField;

    QComboBox* m_comboBox;

};

class ListBoxChoiceFieldHandler : public FormFieldHandler
{
public:
    ListBoxChoiceFieldHandler(Poppler::FormFieldChoice* formField, FormFieldDialog* dialog) : m_formField(formField)
    {
        m_listWidget = new QListWidget(dialog);

        m_listWidget->addItems(m_formField->choices());
        m_listWidget->setSelectionMode(m_formField->multiSelect() ? QAbstractItemView::MultiSelection : QAbstractItemView::SingleSelection);

        foreach(int index, m_formField->currentChoices())
        {
            if(index >= 0 && index < m_listWidget->count())
            {
                m_listWidget->item(index)->setSelected(true);
            }
        }

        dialog->setSizeGripEnabled(true);
    }

    QWidget* widget() const { return m_listWidget; }

    void hideWidget()
    {
        QList< int > currentChoices;

        for(int index = 0; index < m_listWidget->count(); ++index)
        {
            if(m_listWidget->item(index)->isSelected())
            {
                currentChoices.append(index);
            }
        }

        m_formField->setCurrentChoices(currentChoices);
    }

private:
    Poppler::FormFieldChoice* m_formField;

    QListWidget* m_listWidget;

};

FormFieldDialog::FormFieldDialog(QMutex* mutex, Poppler::FormField* formField, QWidget* parent) : QDialog(parent, Qt::Popup),
    m_mutex(mutex),
    m_handler(0)
{
    switch(formField->type())
    {
    case Poppler::FormField::FormSignature:
    case Poppler::FormField::FormButton:
        break;
    case Poppler::FormField::FormText:
        {
            Poppler::FormFieldText* formFieldText = static_cast< Poppler::FormFieldText* >(formField);

            switch(formFieldText->textType())
            {
            case Poppler::FormFieldText::FileSelect:
                break;
            case Poppler::FormFieldText::Normal:
                m_handler = new NormalTextFieldHandler(formFieldText, this);
                break;
            case Poppler::FormFieldText::Multiline:
                m_handler = new MultilineTextFieldHandler(formFieldText, this);
                break;
            }
        }
        break;
    case Poppler::FormField::FormChoice:
        {
            Poppler::FormFieldChoice* formFieldChoice = static_cast< Poppler::FormFieldChoice* >(formField);

            switch(formFieldChoice->choiceType())
            {
            case Poppler::FormFieldChoice::ComboBox:
                m_handler = new ComboBoxChoiceFieldHandler(formFieldChoice, this);
                break;
            case Poppler::FormFieldChoice::ListBox:
                m_handler = new ListBoxChoiceFieldHandler(formFieldChoice, this);
                break;
            }
        }
        break;
    }

    setLayout(new QVBoxLayout(this));
    layout()->setContentsMargins(QMargins());
    layout()->addWidget(m_handler->widget());
}

FormFieldDialog::~FormFieldDialog()
{
    delete m_handler;
}

void FormFieldDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);

    if(!event->spontaneous())
    {
        m_handler->showWidget();

        m_handler->widget()->setFocus();
    }
}

void FormFieldDialog::hideEvent(QHideEvent* event)
{
    QDialog::hideEvent(event);

    m_mutex->lock();

    m_handler->hideWidget();

    m_mutex->unlock();
}
