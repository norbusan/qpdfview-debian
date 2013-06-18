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

#include "pluginhandler.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QPluginLoader>

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

#include <QMimeDatabase>

#endif // QT_VERSION

#ifdef WITH_MAGIC

#include <magic.h>

#endif // WITH_MAGIC

#include "model.h"

#ifdef WITH_PDF

Plugin* PluginHandler::s_pdfPlugin = 0;

#endif // WITH_PDF

#ifdef WITH_PS

Plugin* PluginHandler::s_psPlugin = 0;

#endif // WITH_PS

#ifdef WITH_DJVU

Plugin* PluginHandler::s_djvuPlugin = 0;

#endif // WITH_DJVU

Model::Document* PluginHandler::loadDocument(const QString& filePath)
{
    enum { UnknownType = 0, PDF = 1, PS = 2, DjVu = 3 } fileType = UnknownType;

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

    const QMimeType mimeType = QMimeDatabase().mimeTypeForFile(filePath, QMimeDatabase::MatchContent);

    if(mimeType.name() == "application/pdf")
    {
        fileType = PDF;
    }
    else if(mimeType.name() == "application/postscript")
    {
        fileType = PS;
    }
    else if(mimeType.name() == "image/vnd.djvu")
    {
        fileType = DjVu;
    }
    else
    {
        qDebug() << "Unknown file type:" << mimeType.name();
    }

#else

#ifdef WITH_MAGIC

    magic_t cookie = magic_open(MAGIC_MIME_TYPE | MAGIC_SYMLINK);

    if(magic_load(cookie, 0) == 0)
    {
        const char* mime_type = magic_file(cookie, QFile::encodeName(filePath));

        if(qstrncmp(mime_type, "application/pdf", 15) == 0)
        {
            fileType = PDF;
        }
        else if(qstrncmp(mime_type, "application/postscript", 22) == 0)
        {
            fileType = PS;
        }
        else if(qstrncmp(mime_type, "image/vnd.djvu", 14) == 0)
        {
            fileType = DjVu;
        }
        else
        {
            qDebug() << "Unknown file type:" << mime_type;
        }
    }

    magic_close(cookie);

#else

    const QFileInfo fileInfo(filePath);

    if(fileInfo.suffix().toLower() == "pdf")
    {
        fileType = PDF;
    }
    else if(fileInfo.suffix().toLower() == "ps" || fileInfo.suffix().toLower() == "eps")
    {
        fileType = PS;
    }
    else if(fileInfo.suffix().toLower() == "djvu" || fileInfo.suffix().toLower() == "djv")
    {
        fileType = DjVu;
    }
    else
    {
        qDebug() << "Unkown file type:" << fileInfo.suffix().toLower();
    }

#endif // WITH_MAGIC

#endif // QT_VERSION

#ifdef WITH_PDF

    if(fileType == PDF)
    {
        loadPdfPlugin();

        return s_pdfPlugin != 0 ? s_pdfPlugin->loadDocument(filePath) : 0;
    }

#endif // WITH_PDF

#ifdef WITH_PS

    if(fileType == PS)
    {
        loadPsPlugin();

        return s_psPlugin != 0 ? s_psPlugin->loadDocument(filePath) : 0;
    }

#endif // WITH_PS

#ifdef WITH_DJVU

    if(fileType == DjVu)
    {
        loadDjVuPlugin();

        return s_djvuPlugin != 0 ? s_djvuPlugin->loadDocument(filePath) : 0;
    }

#endif // WITH_DJVU

    return 0;
}

#ifdef WITH_PDF

SettingsWidget* PluginHandler::createPdfSettingsWidget(QWidget* parent)
{
    loadPdfPlugin();

    return s_pdfPlugin != 0 ? s_pdfPlugin->createSettingsWidget(parent) : 0;
}

#endif // WITH_PDF

#ifdef WITH_PS

SettingsWidget* PluginHandler::createPsSettingsWidget(QWidget* parent)
{
    loadPsPlugin();

    return s_psPlugin != 0 ? s_psPlugin->createSettingsWidget(parent) : 0;
}

#endif // WITH_PS


Plugin* PluginHandler::loadPlugin(const QString& fileName)
{
    QPluginLoader pluginLoader(QDir(QApplication::applicationDirPath()).absoluteFilePath(fileName));

    if(!pluginLoader.load())
    {
        const QString firstFileName = pluginLoader.fileName();
        const QString firstErrorString = pluginLoader.errorString();

        pluginLoader.setFileName(QDir(PLUGIN_INSTALL_PATH).absoluteFilePath(fileName));

        if(!pluginLoader.load())
        {
            qCritical() << "Could not load plug-in in first attempt:" << firstFileName;
            qCritical() << firstErrorString;

            qCritical() << "Could not load plug-in in second attempt:" << pluginLoader.fileName();
            qCritical() << pluginLoader.errorString();

            return 0;
        }
    }

    Plugin* plugin = qobject_cast< Plugin* >(pluginLoader.instance());

    if(plugin == 0)
    {
        qCritical() << "Could not instantiate plug-in:" << pluginLoader.fileName();
        qCritical() << pluginLoader.errorString();
    }

    return plugin;
}

Plugin* PluginHandler::loadStaticPlugin(const QString& objectName)
{
    foreach(QObject* object, QPluginLoader::staticInstances())
    {
        if(object->objectName() == objectName)
        {
            Plugin* plugin = qobject_cast< Plugin* >(object);

            if(plugin != 0)
            {
                return plugin;
            }
        }
    }

    qCritical() << "Could not load static plug-in:" << objectName;

    return 0;
}


#ifdef WITH_PDF

#ifdef STATIC_PDF_PLUGIN

Q_IMPORT_PLUGIN(qpdfview_pdf)

#endif // STATIC_PDF_PLUGIN

void PluginHandler::loadPdfPlugin()
{
    if(s_pdfPlugin == 0)
    {
#ifndef STATIC_PDF_PLUGIN
        Plugin* pdfPlugin = loadPlugin(PDF_PLUGIN_NAME);
#else
        Plugin* pdfPlugin = loadStaticPlugin("PdfPlugin");
#endif // STATIC_PDF_PLUGIN

        if(pdfPlugin != 0)
        {
            s_pdfPlugin = pdfPlugin;
        }
        else
        {
            QMessageBox::critical(0, tr("Critical"), tr("Could not load PDF plug-in!"));
        }
    }
}

#endif // WITH_PDF


#ifdef WITH_PS

#ifdef STATIC_PS_PLUGIN

Q_IMPORT_PLUGIN(qpdfview_ps)

#endif // STATIC_PS_PLUGIN

void PluginHandler::loadPsPlugin()
{
    if(s_psPlugin == 0)
    {
#ifndef STATIC_PS_PLUGIN
        Plugin* psPlugin = loadPlugin(PS_PLUGIN_NAME);
#else
        Plugin* psPlugin = loadStaticPlugin("PsPlugin");
#endif // STATIC_PS_PLUGIN

        if(psPlugin != 0)
        {
            s_psPlugin = psPlugin;
        }
        else
        {
            QMessageBox::critical(0, tr("Critical"), tr("Could not load PS plug-in!"));
        }

    }
}

#endif // WITH_PS


#ifdef WITH_DJVU

#ifdef STATIC_DJVU_PLUGIN

Q_IMPORT_PLUGIN(qpdfview_djvu)

#endif // STATIC_DJVU_PLUGIN

void PluginHandler::loadDjVuPlugin()
{
    if(s_djvuPlugin == 0)
    {
#ifndef STATIC_DJVU_PLUGIN
        Plugin* djvuPlugin = loadPlugin(DJVU_PLUGIN_NAME);
#else
        Plugin* djvuPlugin = loadStaticPlugin("DjVuPlugin");
#endif // STATIC_DJVU_PLUGIN

        if(djvuPlugin != 0)
        {
            s_djvuPlugin = djvuPlugin;
        }
        else
        {
            QMessageBox::critical(0, tr("Critical"), tr("Could not load DjVu plug-in!"));
        }

    }
}

#endif // WITH_DJVU
