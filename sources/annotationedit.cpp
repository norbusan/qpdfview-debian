#include "annotationedit.h"

AnnotationEdit::AnnotationEdit(Poppler::Annotation* annotation, QWidget* parent) : QFrame(parent, Qt::Popup),
    m_annotation(annotation)
{
    m_textEdit = new QTextEdit(this);
    m_textEdit->setAcceptRichText(false);
    m_textEdit->setText(m_annotation->contents());

    connect(m_textEdit, SIGNAL(textChanged()), SLOT(on_textEdit_textChanged()));

    setFrameStyle(Panel);

    setLayout(new QVBoxLayout());
    layout()->setContentsMargins(2, 2, 2, 2);
    layout()->addWidget(m_textEdit);
}

void AnnotationEdit::on_textEdit_textChanged()
{
    m_annotation->setContents(m_textEdit->toPlainText());
}

void AnnotationEdit::showEvent(QShowEvent* event)
{
    QFrame::showEvent(event);

    if(!event->spontaneous())
    {
        m_textEdit->setFocus();
        m_textEdit->moveCursor(QTextCursor::End);
    }
}

void AnnotationEdit::keyPressEvent(QKeyEvent* event)
{
    if(event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_Return)
    {
        close();

        event->accept();
        return;
    }

    QFrame::keyPressEvent(event);
}
