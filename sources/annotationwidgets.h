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

#ifndef ANNOTATIONWIDGETS_H
#define ANNOTATIONWIDGETS_H

#include <QPlainTextEdit>

class QMutex;

namespace Poppler
{
class Annotation;
}

class AnnotationWidget : public QPlainTextEdit
{
    Q_OBJECT

public:
    AnnotationWidget(QMutex* mutex, Poppler::Annotation* annotation, QWidget* parent = 0);

signals:
    void wasModified();

protected:
    void keyPressEvent(QKeyEvent* event);

protected slots:
    void on_textChanged();

private:
    Q_DISABLE_COPY(AnnotationWidget)

    QMutex* m_mutex;
    Poppler::Annotation* m_annotation;

};

#endif // ANNOTATIONWIDGETS_H
