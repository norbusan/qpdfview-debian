/*

Copyright 2012 Adam Reichold

This file is part of qpdfview.

qpdfview is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

qpdfview is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with qpdfview.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "mainwindow.h"

int main(int argc, char *argv[])
{
#ifdef DATA_INSTALL_PATH
    QString dataInstallPath(DATA_INSTALL_PATH);
#endif

    QApplication a(argc, argv);
    QApplication::setOrganizationName("qpdfview");
    QApplication::setApplicationName("qpdfview");

#ifdef DATA_INSTALL_PATH
    QApplication::setWindowIcon(QIcon(dataInstallPath + "/qpdfview.svg"));
#else
    QApplication::setWindowIcon(QIcon(":/icons/qpdfview.svg"));
#endif

    QTranslator t;
#ifdef DATA_INSTALL_PATH
    if(t.load(QString(dataInstallPath + "/qpdfview_") + QLocale::system().name()))
#else
    if(t.load(QString(":/translations/qpdfview_") + QLocale::system().name()))
#endif
    {
        a.installTranslator(&t);
    }

    MainWindow w;
    w.show();

    // command line arguments

    QStringList arguments = QCoreApplication::arguments();

    if(!arguments.isEmpty())
    {
        arguments.removeFirst();
    }

    foreach(QString argument, arguments)
    {
        QString filePath;
        int page = 1;
        float top = 0.0;

        QStringList fields = argument.split('#');

        if(fields.count() > 1)
        {
            page = fields.at(1).toInt();
        }
        if(fields.count() > 2)
        {
            top = fields.at(2).toFloat();
        }

        filePath = fields.at(0);

        if(QFileInfo(filePath).exists())
        {
            w.openInNewTab(filePath, page, top);
        }
    }

    return a.exec();
}
