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

class QString;
class QWidget;

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

    Model::Document* loadDocument(const QString& filePath);

#ifdef WITH_PDF

    SettingsWidget* createPdfSettingsWidget(QWidget* parent = 0);

#endif // WITH_PDF

#ifdef WITH_PS

    SettingsWidget* createPsSettingsWidget(QWidget* parent = 0);

#endif // WITH_PS

private:
    Q_DISABLE_COPY(PluginHandler)

    static PluginHandler* s_instance;
    PluginHandler(QObject* parent = 0);

    Plugin* loadPlugin(const QString& fileName);
    Plugin* loadStaticPlugin(const QString& objectName);

#ifdef WITH_PDF

    Plugin* m_pdfPlugin;

    void loadPdfPlugin();

#endif // WITH_PDF

#ifdef WITH_PS

    Plugin* m_psPlugin;

    void loadPsPlugin();

#endif // WITH_PS

#ifdef WITH_DJVU

    Plugin* m_djvuPlugin;

    void loadDjVuPlugin();

#endif // WITH_DJVU

#ifdef WITH_FITZ

    Plugin* m_fitzPlugin;

    void loadFitzPlugin();

#endif // WITH_FITZ

};

#endif // PLUGINHANDLER_H
