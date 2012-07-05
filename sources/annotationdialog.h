#ifndef ANNOTATIONDIALOG_H
#define ANNOTATIONDIALOG_H

#include <QtCore>
#include <QtGui>

#include <poppler-qt4.h>

class AnnotationDialog : public QDialog
{
    Q_OBJECT

public:
    AnnotationDialog(QMutex* mutex, Poppler::Annotation* annotation, QWidget *parent = 0);

protected slots:
    void on_textEdit_textChanged();

protected:
    void showEvent(QShowEvent* event);
    void keyPressEvent(QKeyEvent* event);
    
private:
    QMutex* m_mutex;
    Poppler::Annotation* m_annotation;

    QTextEdit* m_textEdit;
    
};

#endif // ANNOTATIONDIALOG_H
