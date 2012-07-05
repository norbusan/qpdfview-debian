#ifndef ANNOTATIONEDIT_H
#define ANNOTATIONEDIT_H

#include <QtCore>
#include <QtGui>

#include <poppler-qt4.h>

class AnnotationEdit : public QFrame
{
    Q_OBJECT

public:
    explicit AnnotationEdit(Poppler::Annotation* annotation, QWidget* parent = 0);

protected slots:
    void on_textEdit_textChanged();

protected:
    void showEvent(QShowEvent* event);
    void keyPressEvent(QKeyEvent* event);
    
private:
    Poppler::Annotation* m_annotation;
    QTextEdit* m_textEdit;

};

#endif // ANNOTATIONEDIT_H
