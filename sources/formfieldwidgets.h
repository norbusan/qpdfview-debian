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

#ifndef FORMFIELDWIDGETS_H
#define FORMFIELDWIDGETS_H

#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QRadioButton>

class QMutex;

namespace Poppler
{
class FormField;
class FormFieldText;
class FormFieldChoice;
class FormFieldButton;
}

namespace qpdfview
{

class NormalTextFieldWidget : public QLineEdit
{
    Q_OBJECT

public:
    NormalTextFieldWidget(QMutex* mutex, Poppler::FormFieldText* formField, QWidget* parent = 0);

signals:
    void wasModified();

protected:
    void keyPressEvent(QKeyEvent* event);

protected slots:
    void on_textChanged(const QString& text);

private:
    Q_DISABLE_COPY(NormalTextFieldWidget)

    QMutex* m_mutex;
    Poppler::FormFieldText* m_formField;

};

class MultilineTextFieldWidget : public QPlainTextEdit
{
    Q_OBJECT

public:
    MultilineTextFieldWidget(QMutex* mutex, Poppler::FormFieldText* formField, QWidget* parent = 0);

signals:
    void wasModified();

protected:
    void keyPressEvent(QKeyEvent* event);

protected slots:
    void on_textChanged();

private:
    Q_DISABLE_COPY(MultilineTextFieldWidget)

    QMutex* m_mutex;
    Poppler::FormFieldText* m_formField;

};

class ComboBoxChoiceFieldWidget : public QComboBox
{
    Q_OBJECT

public:
    ComboBoxChoiceFieldWidget(QMutex* mutex, Poppler::FormFieldChoice* formField, QWidget* parent = 0);

signals:
    void wasModified();

protected:
    void keyPressEvent(QKeyEvent* event);

    void showPopup();
    void hidePopup();

protected slots:
    void on_currentIndexChanged(int index);
    void on_currentTextChanged(const QString& text);

private:
    Q_DISABLE_COPY(ComboBoxChoiceFieldWidget)

    QMutex* m_mutex;
    Poppler::FormFieldChoice* m_formField;

};

class ListBoxChoiceFieldWidget : public QListWidget
{
    Q_OBJECT

public:
    ListBoxChoiceFieldWidget(QMutex* mutex, Poppler::FormFieldChoice* formField, QWidget* parent = 0);

signals:
    void wasModified();

protected:
    void keyPressEvent(QKeyEvent* event);

protected slots:
    void on_itemSelectionChanged();

private:
    Q_DISABLE_COPY(ListBoxChoiceFieldWidget)

    QMutex* m_mutex;
    Poppler::FormFieldChoice* m_formField;

};

class CheckBoxChoiceFieldWidget : public QCheckBox
{
    Q_OBJECT

public:
    CheckBoxChoiceFieldWidget(QMutex* mutex, Poppler::FormFieldButton* formField, QWidget* parent = 0);

signals:
    void wasModified();

protected:
    void keyPressEvent(QKeyEvent* event);

protected slots:
    void on_toggled(bool checked);

protected slots:

private:
    Q_DISABLE_COPY(CheckBoxChoiceFieldWidget)

    QMutex* m_mutex;
    Poppler::FormFieldButton* m_formField;

};

class RadioChoiceFieldWidget : public QRadioButton
{
    Q_OBJECT

public:
    RadioChoiceFieldWidget(QMutex* mutex, Poppler::FormFieldButton* formField, QWidget* parent = 0);
    ~RadioChoiceFieldWidget();

signals:
    void wasModified();

protected:
    void keyPressEvent(QKeyEvent* event);

protected slots:
    void on_toggled(bool checked);

private:
    Q_DISABLE_COPY(RadioChoiceFieldWidget)

    typedef QMap< QPair< QMutex*, int >, RadioChoiceFieldWidget* > Siblings;
    static Siblings s_siblings;

    QMutex* m_mutex;
    Poppler::FormFieldButton* m_formField;

};

} // qpdfview

#endif // FORMFIELDWIDGETS_H
