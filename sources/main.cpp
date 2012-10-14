/*

Copyright 2012 Adam Reichold

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

#include "mainwindow.h"

struct File
{
    QString filePath;
    int page;

    QString sourceName;
    int sourceLine;
    int sourceColumn;
    QRectF enclosingBox;

    File() : filePath(), page(1), sourceName(), sourceLine(-1), sourceColumn(-1), enclosingBox() {}

};

int main(int argc, char** argv)
{
    qRegisterMetaType< QList< QRectF > >("QList<QRectF>");
    qRegisterMetaType< Poppler::Page::Rotation >("Poppler::Page::Rotation");

    QApplication application(argc, argv);

    QApplication::setOrganizationDomain("local.qpdfview");
    QApplication::setOrganizationName("qpdfview");
    QApplication::setApplicationName("qpdfview");

    QApplication::setApplicationVersion("0.3.5");

    QApplication::setWindowIcon(QIcon(":icons/qpdfview.svg"));

    QTranslator translator;

#if QT_VERSION >= QT_VERSION_CHECK(4,8,0)

    if(translator.load(QLocale::system(), "qpdfview", "_", DATA_INSTALL_PATH)) { application.installTranslator(&translator); }
    else if(translator.load(QLocale::system(), "qpdfview", "_", QApplication::applicationDirPath())) { application.installTranslator(&translator); }

#else

    if(translator.load("qpdfview_" + QLocale::system().name(), DATA_INSTALL_PATH)) { application.installTranslator(&translator); }
    else if(translator.load("qpdfview_" + QLocale::system().name(), QApplication::applicationDirPath())) { application.installTranslator(&translator); }

#endif // QT_VERSION

    bool unique = false;
    QList< File > files;

    {
        // command-line arguments

        QRegExp regExp1("(.+)#(\\d+)");
        QRegExp regExp2("(.+)#src:(.+):(\\d+):(\\d+)");

        QStringList arguments = QApplication::arguments();

        if(!arguments.isEmpty())
        {
            arguments.removeFirst();
        }

        foreach(QString argument, arguments)
        {
            if(argument == "--unique")
            {
                unique = true;
            }
            else
            {
                File file;

                if(regExp1.exactMatch(argument))
                {
                    file.filePath = QFileInfo(regExp1.cap(1)).absoluteFilePath();
                    file.page = regExp1.cap(2).toInt();
                }
                else if(regExp2.exactMatch(argument))
                {
                    file.filePath = QFileInfo(regExp2.cap(1)).absoluteFilePath();
                    file.sourceName = regExp2.cap(2);
                    file.sourceLine = regExp2.cap(3).toInt();
                    file.sourceColumn = regExp2.cap(4).toInt();
                }
                else
                {
                    file.filePath = QFileInfo(argument).absoluteFilePath();
                }

                files.append(file);
            }
        }
    }

    MainWindow* mainWindow = 0;

#ifdef WITH_SYNCTEX

    {
        // SyncTeX

        for(int index = 0; index < files.count(); ++index)
        {
            File& file = files[index];

            if(!file.sourceName.isNull())
            {
                synctex_scanner_t scanner = synctex_scanner_new_with_output_file(file.filePath.toLocal8Bit(), 0, 1);

                if(scanner != 0)
                {
                    if(synctex_display_query(scanner, file.sourceName.toLocal8Bit(), file.sourceLine, file.sourceColumn) > 0)
                    {
                        for(synctex_node_t node = synctex_next_result(scanner); node != 0; node = synctex_next_result(scanner))
                        {
                            int page = synctex_node_page(node);
                            QRectF enclosingBox(synctex_node_box_visible_h(node), synctex_node_box_visible_v(node), synctex_node_box_visible_width(node), synctex_node_box_visible_height(node));

                            if(file.page != page)
                            {
                                file.page = page;
                                file.enclosingBox = enclosingBox;
                            }
                            else
                            {
                                file.enclosingBox = file.enclosingBox.united(enclosingBox);
                            }
                        }
                    }

                    synctex_scanner_free(scanner);
                }
                else
                {
                    qWarning() << QObject::tr("SyncTeX data for '%1' could not be found.").arg(file.filePath);
                }
            }
        }
    }

#endif // WITH_SYNCTEX

#ifdef WITH_DBUS

    {
        // D-Bus

        if(unique)
        {
            QDBusInterface* interface = new QDBusInterface("local.qpdfview", "/MainWindow", "local.qpdfview.MainWindow", QDBusConnection::sessionBus());

            if(interface->isValid())
            {
                interface->call("raiseAndActivate");

                foreach(File file, files)
                {
                    QDBusReply< bool > reply = interface->call("jumpToPageOrOpenInNewTab", file.filePath, file.page, true, file.enclosingBox);

                    if(!reply.isValid())
                    {
                        qDebug() << QDBusConnection::sessionBus().lastError().message();

                        delete interface;
                        return 1;
                    }
                }

                delete interface;
                return 0;
            }
            else
            {
                mainWindow = new MainWindow();

                new MainWindowAdaptor(mainWindow);

                if(!QDBusConnection::sessionBus().registerService("local.qpdfview"))
                {
                    qDebug() << QDBusConnection::sessionBus().lastError().message();

                    delete mainWindow;
                    return 1;
                }

                if(!QDBusConnection::sessionBus().registerObject("/MainWindow", mainWindow))
                {
                    qDebug() << QDBusConnection::sessionBus().lastError().message();

                    delete mainWindow;
                    return 1;
                }
            }
        }
        else
        {
            mainWindow = new MainWindow();
        }
    }

#else

    mainWindow = new MainWindow();

#endif // WITH_DBUS

    mainWindow->show();
    mainWindow->setAttribute(Qt::WA_DeleteOnClose);

    foreach(File file, files)
    {
        mainWindow->openInNewTab(file.filePath, file.page, file.enclosingBox);
    }
    
    return application.exec();
}
