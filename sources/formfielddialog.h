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

#ifndef FORMFIELDDIALOG_H
#define FORMFIELDDIALOG_H

#include <QtCore>
#include <QtGui>

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

#include <QtWidgets>

#endif // QT_VERSION

#include <poppler-form.h>

class FormFieldDialog : public QDialog
{
    Q_OBJECT

public:
    FormFieldDialog(QMutex* mutex, Poppler::FormField* formField, QWidget* parent = 0);
    ~FormFieldDialog();

protected:
    void showEvent(QShowEvent* event);
    void hideEvent(QHideEvent* event);

private:
    QMutex* m_mutex;
    Poppler::FormField* m_formField;

    class FormFieldHandler* m_handler;

};

#endif // FORMFIELDDIALOG_H
