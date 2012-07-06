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

#include "annotationdialog.h"

AnnotationDialog::AnnotationDialog(QMutex* mutex, Poppler::Annotation* annotation, QWidget* parent) : QDialog(parent, Qt::Popup),
    m_mutex(mutex),
    m_annotation(annotation)
{
    m_textEdit = new QTextEdit(this);
    m_textEdit->setAcceptRichText(false);

    m_mutex->lock();

    m_textEdit->setPlainText(m_annotation->contents());

    m_mutex->unlock();

    connect(m_textEdit, SIGNAL(textChanged()), SLOT(on_textEdit_textChanged()));

    setLayout(new QVBoxLayout());
    layout()->setContentsMargins(2, 2, 2, 2);
    layout()->addWidget(m_textEdit);
}

void AnnotationDialog::on_textEdit_textChanged()
{
    m_mutex->lock();

    m_annotation->setContents(m_textEdit->toPlainText());

    m_mutex->unlock();
}

void AnnotationDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);

    if(!event->spontaneous())
    {
        m_textEdit->setFocus();
        m_textEdit->moveCursor(QTextCursor::End);
    }
}

void AnnotationDialog::keyPressEvent(QKeyEvent *event)
{
    if(event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_Return)
    {
        close();

        event->accept();
        return;
    }

    QDialog::keyPressEvent(event);
}
