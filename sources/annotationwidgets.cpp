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

#include "annotationwidgets.h"

#include <QMutex>
#include <QPlainTextEdit>
#include <QVBoxLayout>

#include <poppler-annotation.h>

AnnotationWidget::AnnotationWidget(QMutex* mutex, Poppler::Annotation* annotation, QWidget* parent) : QPlainTextEdit(parent),
    m_mutex(mutex),
    m_annotation(annotation)
{
#ifndef HAS_POPPLER_24

    QMutexLocker mutexLocker(m_mutex);

#endif // HAS_POPPLER_24

    setPlainText(m_annotation->contents());

    connect(this, SIGNAL(textChanged()), SLOT(on_textChanged()));
    connect(this, SIGNAL(textChanged()), SIGNAL(wasModified()));

    moveCursor(QTextCursor::End);
}

void AnnotationWidget::keyPressEvent(QKeyEvent* event)
{
    if(event->key() == Qt::Key_Escape)
    {
        hide();

        event->accept();
        return;
    }

    QPlainTextEdit::keyPressEvent(event);
}

void AnnotationWidget::on_textChanged()
{
#ifndef HAS_POPPLER_24

    QMutexLocker mutexLocker(m_mutex);

#endif // HAS_POPPLER_24

    m_annotation->setContents(toPlainText());
}
