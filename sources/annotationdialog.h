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

#ifndef ANNOTATIONDIALOG_H
#define ANNOTATIONDIALOG_H

#include <QDialog>

class QMutex;
class QPlainTextEdit;

namespace Poppler
{
class Annotation;
}

class AnnotationDialog : public QDialog
{
    Q_OBJECT

public:
    AnnotationDialog(QMutex* mutex, Poppler::Annotation* annotation, QWidget* parent = 0);

signals:
    void tabPressed();

protected:
    void showEvent(QShowEvent* event);
    void hideEvent(QHideEvent* event);

    void keyPressEvent(QKeyEvent* event);
    
private:
    QMutex* m_mutex;
    Poppler::Annotation* m_annotation;

    QPlainTextEdit* m_plainTextEdit;
    
};

#endif // ANNOTATIONDIALOG_H
