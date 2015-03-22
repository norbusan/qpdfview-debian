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

#ifndef PLUGINHANDLER_H
#define PLUGINHANDLER_H

#include <QObject>
#include <QMap>

class QString;
class QWidget;

namespace qpdfview
{

namespace Model
{
class Document;
}

class SettingsWidget;
class Plugin;

class PluginHandler : public QObject
{
    Q_OBJECT

public:
    static PluginHandler* instance();
    ~PluginHandler();

    enum FileType
    {
        Unknown = 0,
        PDF = 1,
        PS = 2,
        DjVu = 3
    };

    static QString fileTypeName(FileType fileType)
    {
        switch(fileType)
        {
        default:
        case PluginHandler::Unknown:
            return QLatin1String("Unknown");
        case PluginHandler::PDF:
            return QLatin1String("PDF");
        case PluginHandler::PS:
            return QLatin1String("PS");
        case PluginHandler::DjVu:
            return QLatin1String("DjVu");
        }
    }

    Model::Document* loadDocument(const QString& filePath);

    SettingsWidget* createSettingsWidget(FileType fileType, QWidget* parent = 0);

private:
    Q_DISABLE_COPY(PluginHandler)

    static PluginHandler* s_instance;
    PluginHandler(QObject* parent = 0);

    QMap< FileType, Plugin* > m_plugins;

    QMultiMap< FileType, QString > m_objectNames;
    QMultiMap< FileType, QString > m_fileNames;

    bool loadPlugin(FileType fileType);

};

} // qpdfview

#endif // PLUGINHANDLER_H
