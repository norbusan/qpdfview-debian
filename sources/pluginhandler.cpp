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

namespace
{

using namespace qpdfview;

Plugin* loadStaticPlugin(const QString& objectName)
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

Plugin* loadPlugin(const QString& fileName)
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

PluginHandler::FileType matchFileType(const QString& filePath)
{
    PluginHandler::FileType fileType = PluginHandler::Unknown;

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

    const QMimeType mimeType = QMimeDatabase().mimeTypeForFile(filePath, QMimeDatabase::MatchContent);

    if(mimeType.name() == QLatin1String("application/pdf"))
    {
        fileType = PluginHandler::PDF;
    }
    else if(mimeType.name() == QLatin1String("application/postscript"))
    {
        fileType = PluginHandler::PS;
    }
    else if(mimeType.name() == QLatin1String("image/vnd.djvu"))
    {
        fileType = PluginHandler::DjVu;
    }
    else
    {
        qDebug() << "Unknown MIME type:" << mimeType.name();
    }

#else

#ifdef WITH_MAGIC

    magic_t cookie = magic_open(MAGIC_MIME_TYPE | MAGIC_SYMLINK);

    if(magic_load(cookie, 0) == 0)
    {
        const char* mime_type = magic_file(cookie, QFile::encodeName(filePath));

        if(qstrncmp(mime_type, "application/pdf", 15) == 0)
        {
            fileType = PluginHandler::PDF;
        }
        else if(qstrncmp(mime_type, "application/postscript", 22) == 0)
        {
            fileType = PluginHandler::PS;
        }
        else if(qstrncmp(mime_type, "image/vnd.djvu", 14) == 0)
        {
            fileType = PluginHandler::DjVu;
        }
        else
        {
            qDebug() << "Unknown MIME type:" << mime_type;
        }
    }

    magic_close(cookie);

#else

    const QFileInfo fileInfo(filePath);

    if(fileInfo.suffix().toLower() == QLatin1String("pdf"))
    {
        fileType = PluginHandler::PDF;
    }
    else if(fileInfo.suffix().toLower() == QLatin1String("ps") || fileInfo.suffix().toLower() == QLatin1String("eps"))
    {
        fileType = PluginHandler::PS;
    }
    else if(fileInfo.suffix().toLower() == QLatin1String("djvu") || fileInfo.suffix().toLower() == QLatin1String("djv"))
    {
        fileType = PluginHandler::DjVu;
    }
    else
    {
        qDebug() << "Unkown file suffix:" << fileInfo.suffix().toLower();
    }

#endif // WITH_MAGIC

#endif // QT_VERSION

    return fileType;
}

} // anonymous

namespace qpdfview
{

PluginHandler* PluginHandler::s_instance = 0;

PluginHandler* PluginHandler::instance()
{
    if(s_instance == 0)
    {
        s_instance = new PluginHandler(qApp);
    }

    return s_instance;
}

PluginHandler::~PluginHandler()
{
    s_instance = 0;
}

Model::Document* PluginHandler::loadDocument(const QString& filePath)
{
    FileType fileType = matchFileType(filePath);

    if(fileType == Unknown)
    {
        QMessageBox::warning(0, tr("Warning"), tr("Could not match file type of '%1'!").arg(filePath));

        return 0;
    }

    if(loadPlugin(fileType))
    {
        return m_plugins.value(fileType)->loadDocument(filePath);
    }

    QMessageBox::critical(0, tr("Critical"), tr("Could not load plug-in for file type '%1'!").arg(fileTypeName(fileType)));

    return 0;
}

SettingsWidget* PluginHandler::createSettingsWidget(FileType fileType, QWidget* parent)
{
    return loadPlugin(fileType) ? m_plugins.value(fileType)->createSettingsWidget(parent) : 0;
}

PluginHandler::PluginHandler(QObject* parent) : QObject(parent),
    m_plugins()
{
#ifdef WITH_FITZ
#ifdef STATIC_FITZ_PLUGIN
    m_objectNames.insertMulti(PDF, QLatin1String("FitzPlugin"));
#else
    m_fileNames.insertMulti(PDF, FITZ_PLUGIN_NAME);
#endif // STATIC_FITZ_PLUGIN
#endif // WITH_FITZ

#ifdef WITH_PDF
    #ifdef STATIC_PDF_PLUGIN
        m_objectNames.insertMulti(PDF, QLatin1String("PdfPlugin"));
    #else
        m_fileNames.insertMulti(PDF, PDF_PLUGIN_NAME);
    #endif // STATIC_PDF_PLUGIN
#endif // WITH_PDF

#ifdef WITH_PS
#ifdef STATIC_PS_PLUGIN
    m_objectNames.insertMulti(PS, QLatin1String("PsPlugin"));
#else
    m_fileNames.insertMulti(PS, PS_PLUGIN_NAME);
#endif // STATIC_PS_PLUGIN
#endif // WITH_PS

#ifdef WITH_DJVU
#ifdef STATIC_DJVU_PLUGIN
    m_objectNames.insertMulti(DjVu, QLatin1String("DjVuPlugin"));
#else
    m_fileNames.insertMulti(DjVu, DJVU_PLUGIN_NAME);
#endif // STATIC_DJVU_PLUGIN
#endif // WITH_DJVU
}

bool PluginHandler::loadPlugin(FileType fileType)
{
    if(m_plugins.contains(fileType))
    {
        return true;
    }

    foreach(const QString& objectName, m_objectNames.values(fileType))
    {
        Plugin* plugin = ::loadStaticPlugin(objectName);

        if(plugin != 0)
        {
            m_plugins.insert(fileType, plugin);

            return true;
        }
    }

    foreach(const QString& fileName, m_fileNames.values(fileType))
    {
        Plugin* plugin = ::loadPlugin(fileName);

        if(plugin != 0)
        {
            m_plugins.insert(fileType, plugin);

            return true;
        }
    }

    return false;
}

} // qpdfview

#ifdef STATIC_FITZ_PLUGIN
    #if QT_VERSION < QT_VERSION_CHECK(5,0,0)
        Q_IMPORT_PLUGIN(qpdfview_fitz)
    #else
        Q_IMPORT_PLUGIN(qpdfview::FitzPlugin)
    #endif // QT_VERSION
#endif // STATIC_FITZ_PLUGIN

#ifdef STATIC_PDF_PLUGIN
    #if QT_VERSION < QT_VERSION_CHECK(5,0,0)
        Q_IMPORT_PLUGIN(qpdfview_pdf)
    #else
        Q_IMPORT_PLUGIN(qpdfview::PdfPlugin)
    #endif // QT_VERSION
#endif // STATIC_PDF_PLUGIN

#ifdef STATIC_PS_PLUGIN
    #if QT_VERSION < QT_VERSION_CHECK(5,0,0)
        Q_IMPORT_PLUGIN(qpdfview_ps)
    #else
        Q_IMPORT_PLUGIN(qpdfview::PsPlugin)
    #endif // QT_VERSION
#endif // STATIC_PS_PLUGIN

#ifdef STATIC_DJVU_PLUGIN
    #if QT_VERSION < QT_VERSION_CHECK(5,0,0)
        Q_IMPORT_PLUGIN(qpdfview_djvu)
    #else
        Q_IMPORT_PLUGIN(qpdfview::DjvuPlugin)
    #endif // QT_VERSION
#endif // STATIC_DJVU_PLUGIN
