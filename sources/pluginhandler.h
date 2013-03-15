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

#include <QCoreApplication>

class QString;
class QWidget;

namespace Model
{
class Document;
}

class SettingsWidget;
class Plugin;

class PluginHandler
{
    Q_DECLARE_TR_FUNCTIONS(PluginHandler)

public:

    static Model::Document* loadDocument(const QString& filePath);

#ifdef WITH_PDF

    static SettingsWidget* createPdfSettingsWidget(QWidget* parent = 0);

#endif // WITH_PDF

#ifdef WITH_PS

    static SettingsWidget* createPsSettingsWidget(QWidget* parent = 0);

#endif // WITH_PS

private:
    PluginHandler() {}

    static Plugin* loadPlugin(const QString& fileName);
    static Plugin* loadStaticPlugin(const QString& objectName);

#ifdef WITH_PDF

    static Plugin* s_pdfPlugin;

    static void loadPdfPlugin();

#endif // WITH_PDF

#ifdef WITH_PS

    static Plugin* s_psPlugin;

    static void loadPsPlugin();

#endif // WITH_PS

#ifdef WITH_DJVU

    static Plugin* s_djvuPlugin;

    static void loadDjVuPlugin();

#endif // WITH_DJVU

};

#endif // PLUGINHANDLER_H
