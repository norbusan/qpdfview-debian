#include <QtGui/QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCoreApplication::setOrganizationName("qpdfview");
    QCoreApplication::setApplicationName("qpdfview");

    MainWindow w;
    w.show();
    
    return a.exec();
}
