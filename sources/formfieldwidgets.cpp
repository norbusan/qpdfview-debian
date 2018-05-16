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

#include "formfieldwidgets.h"

#include <QGraphicsProxyWidget>
#include <QMutex>

#include <poppler-form.h>

#ifndef HAS_POPPLER_24

#define LOCK_FORM_FIELD QMutexLocker mutexLocker(m_mutex);

#else

#define LOCK_FORM_FIELD

#endif // HAS_POPPLER_24

namespace
{

bool hideOnEscape(QWidget* widget, QKeyEvent* event)
{
    if(event->key() == Qt::Key_Escape)
    {
        widget->hide();

        event->accept();
        return true;
    }

    return false;
}

} // anonymous

namespace qpdfview
{

NormalTextFieldWidget::NormalTextFieldWidget(QMutex* mutex, Poppler::FormFieldText* formField, QWidget* parent) : QLineEdit(parent),
    m_mutex(mutex),
    m_formField(formField)
{
    LOCK_FORM_FIELD

    setText(m_formField->text());
    setMaxLength(m_formField->maximumLength());
    setAlignment(m_formField->textAlignment());
    setEchoMode(m_formField->isPassword() ? QLineEdit::Password : QLineEdit::Normal);

    connect(this, SIGNAL(textChanged(QString)), SLOT(on_textChanged(QString)));
    connect(this, SIGNAL(textChanged(QString)), SIGNAL(wasModified()));

    connect(this, SIGNAL(returnPressed()), SLOT(hide()));
}

void NormalTextFieldWidget::keyPressEvent(QKeyEvent* event)
{
    if(!hideOnEscape(this, event))
    {
        QLineEdit::keyPressEvent(event);
    }
}

void NormalTextFieldWidget::on_textChanged(const QString& text)
{
    LOCK_FORM_FIELD

    m_formField->setText(text);
}

MultilineTextFieldWidget::MultilineTextFieldWidget(QMutex* mutex, Poppler::FormFieldText* formField, QWidget* parent) : QPlainTextEdit(parent),
    m_mutex(mutex),
    m_formField(formField)
{
    LOCK_FORM_FIELD

    setPlainText(m_formField->text());

    connect(this, SIGNAL(textChanged()), SLOT(on_textChanged()));
    connect(this, SIGNAL(textChanged()), SIGNAL(wasModified()));

    moveCursor(QTextCursor::End);
}

void MultilineTextFieldWidget::keyPressEvent(QKeyEvent* event)
{
    if(!hideOnEscape(this, event))
    {
        QPlainTextEdit::keyPressEvent(event);
    }
}

void MultilineTextFieldWidget::on_textChanged()
{
    LOCK_FORM_FIELD

    m_formField->setText(toPlainText());
}

ComboBoxChoiceFieldWidget::ComboBoxChoiceFieldWidget(QMutex* mutex, Poppler::FormFieldChoice* formField, QWidget* parent) : QComboBox(parent),
    m_mutex(mutex),
    m_formField(formField)
{
    LOCK_FORM_FIELD

    addItems(m_formField->choices());

    if(!m_formField->currentChoices().isEmpty())
    {
        setCurrentIndex(m_formField->currentChoices().at(0));
    }

    connect(this, SIGNAL(currentIndexChanged(int)), SLOT(on_currentIndexChanged(int)));
    connect(this, SIGNAL(currentIndexChanged(int)), SIGNAL(wasModified()));

#ifdef HAS_POPPLER_22

    if(m_formField->isEditable())
    {
        setEditable(true);
        setInsertPolicy(QComboBox::NoInsert);

        lineEdit()->setText(m_formField->editChoice());

        connect(lineEdit(), SIGNAL(textChanged(QString)), SLOT(on_currentTextChanged(QString)));
        connect(lineEdit(), SIGNAL(textChanged(QString)), SIGNAL(wasModified()));

        connect(lineEdit(), SIGNAL(returnPressed()), SLOT(hide()));
    }
    else
    {
        connect(this, SIGNAL(activated(int)), SLOT(hide()));
    }

#else

    connect(this, SIGNAL(activated(int)), SLOT(hide()));

#endif // HAS_POPPLER_22
}

void ComboBoxChoiceFieldWidget::keyPressEvent(QKeyEvent* event)
{
    if(!hideOnEscape(this, event))
    {
        QComboBox::keyPressEvent(event);
    }
}

void ComboBoxChoiceFieldWidget::showPopup()
{
    QComboBox::showPopup();

    graphicsProxyWidget()->setZValue(1.0);
}

void ComboBoxChoiceFieldWidget::hidePopup()
{
    QComboBox::hidePopup();

    graphicsProxyWidget()->setZValue(0.0);
}

void ComboBoxChoiceFieldWidget::on_currentIndexChanged(int index)
{
    LOCK_FORM_FIELD

    m_formField->setCurrentChoices(QList< int >() << index);
}

void ComboBoxChoiceFieldWidget::on_currentTextChanged(const QString& text)
{
    LOCK_FORM_FIELD

#ifdef HAS_POPPLER_22

    m_formField->setEditChoice(text);

#endif // HAS_POPPLER_22
}

ListBoxChoiceFieldWidget::ListBoxChoiceFieldWidget(QMutex* mutex, Poppler::FormFieldChoice* formField, QWidget* parent) : QListWidget(parent),
    m_mutex(mutex),
    m_formField(formField)
{
    LOCK_FORM_FIELD

    addItems(m_formField->choices());
    setSelectionMode(m_formField->multiSelect() ? QAbstractItemView::MultiSelection : QAbstractItemView::SingleSelection);

    foreach(int index, m_formField->currentChoices())
    {
        if(index >= 0 && index < count())
        {
            item(index)->setSelected(true);
        }
    }

    connect(this, SIGNAL(itemSelectionChanged()), SLOT(on_itemSelectionChanged()));
    connect(this, SIGNAL(itemSelectionChanged()), SIGNAL(wasModified()));
}

void ListBoxChoiceFieldWidget::keyPressEvent(QKeyEvent* event)
{
    if(!hideOnEscape(this, event))
    {
        QListWidget::keyPressEvent(event);
    }
}

void ListBoxChoiceFieldWidget::on_itemSelectionChanged()
{
    LOCK_FORM_FIELD

    QList< int > currentChoices;

    for(int index = 0; index < count(); ++index)
    {
        if(item(index)->isSelected())
        {
            currentChoices.append(index);
        }
    }

    m_formField->setCurrentChoices(currentChoices);
}

CheckBoxChoiceFieldWidget::CheckBoxChoiceFieldWidget(QMutex* mutex, Poppler::FormFieldButton* formField, QWidget* parent) : QCheckBox(parent),
    m_mutex(mutex),
    m_formField(formField)
{
    LOCK_FORM_FIELD

    setChecked(m_formField->state());

    connect(this, SIGNAL(toggled(bool)), SLOT(on_toggled(bool)));
    connect(this, SIGNAL(toggled(bool)), SIGNAL(wasModified()));
}

void CheckBoxChoiceFieldWidget::keyPressEvent(QKeyEvent* event)
{
    if(!hideOnEscape(this, event))
    {
        QCheckBox::keyPressEvent(event);
    }
}

void CheckBoxChoiceFieldWidget::on_toggled(bool checked)
{
    LOCK_FORM_FIELD

    m_formField->setState(checked);
}

RadioChoiceFieldWidget::Siblings RadioChoiceFieldWidget::s_siblings;

RadioChoiceFieldWidget::RadioChoiceFieldWidget(QMutex* mutex, Poppler::FormFieldButton* formField, QWidget* parent) : QRadioButton(parent),
    m_mutex(mutex),
    m_formField(formField)
{
    LOCK_FORM_FIELD

    s_siblings.insert(qMakePair(m_mutex, m_formField->id()), this);

    setAutoExclusive(false);
    setChecked(m_formField->state());

    connect(this, SIGNAL(toggled(bool)), SLOT(on_toggled(bool)));
    connect(this, SIGNAL(toggled(bool)), SIGNAL(wasModified()));
}

RadioChoiceFieldWidget::~RadioChoiceFieldWidget()
{
    LOCK_FORM_FIELD

    s_siblings.remove(qMakePair(m_mutex, m_formField->id()));
}

void RadioChoiceFieldWidget::keyPressEvent(QKeyEvent* event)
{
    if(!hideOnEscape(this, event))
    {
        QRadioButton::keyPressEvent(event);
    }
}

void RadioChoiceFieldWidget::on_toggled(bool checked)
{
    LOCK_FORM_FIELD

    m_formField->setState(checked);

    if(checked)
    {
        const QList< int > siblings = m_formField->siblings();

#ifndef HAS_POPPLER_24

        mutexLocker.unlock();

#endif // HAS_POPPLER_24

        foreach(int id, siblings)
        {
            const QPair< QMutex*, int > key = qMakePair(m_mutex, id);

            if(s_siblings.contains(key))
            {
                s_siblings.value(key)->setChecked(false);
            }
        }
    }
}

} // qpdfview
