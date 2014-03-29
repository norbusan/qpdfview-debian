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
#include <QToolButton>

class QMutex;

namespace Poppler
{
class Annotation;
class FileAttachmentAnnotation;
}

namespace qpdfview
{

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


class FileAttachmentAnnotationWidget : public QToolButton
{
    Q_OBJECT

public:
    FileAttachmentAnnotationWidget(QMutex* mutex, Poppler::FileAttachmentAnnotation* annotation, QWidget* parent = 0);

protected:
    void keyPressEvent(QKeyEvent* event);

protected slots:
    void on_aboutToShow();
    void on_aboutToHide();

    void on_save_triggered();
    void on_saveAndOpen_triggered();

private:
    Q_DISABLE_COPY(FileAttachmentAnnotationWidget)

    QMutex* m_mutex;
    Poppler::FileAttachmentAnnotation* m_annotation;

    void save(bool open = false);

    QMenu* m_menu;
    QAction* m_saveAction;
    QAction* m_saveAndOpenAction;

};

} // qpdfview

#endif // ANNOTATIONWIDGETS_H
