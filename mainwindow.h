#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore>
#include <QtGui>

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

    QSize sizeHint() const { return QSize(500,700); }

private slots:
    void open();
    void closeTab();

    void save();
    void print();

    void previousPage();
    void nextPage();
    void firstPage();
    void lastPage();

private:
    QTabWidget *m_tabWidget;

    QAction *m_openAction;
    QAction *m_closeTabAction;

    QAction *m_saveAction;
    QAction *m_printAction;

    QAction *m_exitAction;

    QAction *m_previousPageAction;
    QAction *m_nextPageAction;
    QAction *m_firstPageAction;
    QAction *m_lastPageAction;

};

#endif // MAINWINDOW_H
