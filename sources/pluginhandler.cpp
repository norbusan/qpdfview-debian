/*

Copyright 2012-2013, 2015-2016 Adam Reichold

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
#include <QImageReader>
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
    QPluginLoader pluginLoader;

    const QString localFileName = QDir(QApplication::applicationDirPath()).absoluteFilePath(fileName);
    pluginLoader.setFileName(localFileName);

    if(!pluginLoader.load())
    {
        const QString localErrorString = pluginLoader.errorString();

        const QString globalFileName = QDir(PLUGIN_INSTALL_PATH).absoluteFilePath(fileName);
        pluginLoader.setFileName(globalFileName);

        if(!pluginLoader.load())
        {
            const QString globalErrorString = pluginLoader.errorString();

            qCritical() << "Could not load local plug-in:" << localFileName;
            qCritical() << localErrorString;

            qCritical() << "Could not load global plug-in:" << globalFileName;
            qCritical() << globalErrorString;

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

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

bool isSupportedImageFormat(const QMimeType& mimeType)
{
    const QByteArray name = mimeType.name().toLocal8Bit();

    return QImageReader::supportedMimeTypes().contains(name);
}

#else

bool isSupportedImageFormat(const QString& fileName)
{
    return !QImageReader::imageFormat(fileName).isEmpty();
}

#endif // QT_VERSION

QStringList supportedImageFormats()
{
    QStringList formats;

    foreach(const QByteArray& format, QImageReader::supportedImageFormats())
    {
        const QString name = QString::fromLocal8Bit(format);

        formats.append(QLatin1String("*.") + name);
    }

    return formats;
}

const char* const pdfMimeType = "application/pdf";
const char* const psMimeType = "application/postscript";
const char* const djvuMimeType = "image/vnd.djvu";

PluginHandler::FileType matchFileType(const QString& filePath)
{
    PluginHandler::FileType fileType = PluginHandler::Unknown;

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

    const QMimeType mimeType = QMimeDatabase().mimeTypeForFile(filePath, QMimeDatabase::MatchContent);

    if(mimeType.inherits(pdfMimeType))
    {
        fileType = PluginHandler::PDF;
    }
    else if(mimeType.inherits(psMimeType))
    {
        fileType = PluginHandler::PS;
    }
    else if(mimeType.inherits(djvuMimeType))
    {
        fileType = PluginHandler::DjVu;
    }
    else if(isSupportedImageFormat(mimeType))
    {
        fileType = PluginHandler::Image;
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
        const char* mimeType = magic_file(cookie, QFile::encodeName(filePath));

        if(qstrncmp(mimeType, pdfMimeType, qstrlen(pdfMimeType)) == 0)
        {
            fileType = PluginHandler::PDF;
        }
        else if(qstrncmp(mimeType, psMimeType, qstrlen(psMimeType)) == 0)
        {
            fileType = PluginHandler::PS;
        }
        else if(qstrncmp(mimeType, djvuMimeType, qstrlen(djvuMimeType)) == 0)
        {
            fileType = PluginHandler::DjVu;
        }
        else if(isSupportedImageFormat(filePath))
        {
            fileType = PluginHandler::Image;
        }
        else
        {
            qDebug() << "Unknown MIME type:" << mimeType;
        }
    }

    magic_close(cookie);

#else

    const QString suffix = QFileInfo(filePath).suffix().toLower();

    if(suffix == QLatin1String("pdf"))
    {
        fileType = PluginHandler::PDF;
    }
    else if(suffix == QLatin1String("ps") || suffix == QLatin1String("eps"))
    {
        fileType = PluginHandler::PS;
    }
    else if(suffix == QLatin1String("djvu") || suffix == QLatin1String("djv"))
    {
        fileType = PluginHandler::DjVu;
    }
    else if(isSupportedImageFormat(filePath))
    {
        fileType = PluginHandler::Image;
    }
    else
    {
        qDebug() << "Unkown file suffix:" << suffix;
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

QString PluginHandler::fileTypeName(PluginHandler::FileType fileType)
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
    case PluginHandler::Image:
        return QLatin1String("Image");
    }
}

QStringList PluginHandler::openFilter()
{
    QStringList openFilter;
    QStringList supportedFormats;

#if defined(WITH_PDF) || defined(WITH_FITZ)

    openFilter.append(QLatin1String("Portable document format (*.pdf)"));
    supportedFormats.append(QLatin1String("*.pdf"));

#endif // WITH_PDF // WITH_FITZ

#ifdef WITH_PS

    openFilter.append(QLatin1String("PostScript (*.ps)"));
    openFilter.append(QLatin1String("Encapsulated PostScript (*.eps)"));
    supportedFormats.append(QLatin1String("*.ps *.eps"));

#endif // WITH_PS

#ifdef WITH_DJVU

    openFilter.append(QLatin1String("DjVu (*.djvu *.djv)"));
    supportedFormats.append(QLatin1String("*.djvu *.djv"));

#endif // WITH_DJVU

#ifdef WITH_IMAGE

    const QStringList imageFormats = supportedImageFormats();

    openFilter.append(tr("Image (%1)").arg(imageFormats.join(QLatin1String(" "))));
    supportedFormats.append(imageFormats);

#endif // WITH_IMAGE

    openFilter.prepend(tr("Supported formats (%1)").arg(supportedFormats.join(QLatin1String(" "))));

    return openFilter;
}

Model::Document* PluginHandler::loadDocument(const QString& filePath)
{
    const FileType fileType = matchFileType(filePath);

    if(fileType == Unknown)
    {
        qWarning() << tr("Could not match file type of '%1'!").arg(filePath);

        return 0;
    }

    if(!loadPlugin(fileType))
    {
        QMessageBox::critical(0, tr("Critical"), tr("Could not load plug-in for file type '%1'!").arg(fileTypeName(fileType)));

        return 0;
    }

    return m_plugins.value(fileType)->loadDocument(filePath);
}

SettingsWidget* PluginHandler::createSettingsWidget(FileType fileType, QWidget* parent)
{
    return loadPlugin(fileType) ? m_plugins.value(fileType)->createSettingsWidget(parent) : 0;
}

PluginHandler::PluginHandler(QObject* parent) : QObject(parent),
    m_plugins()
{
#ifdef WITH_IMAGE
#ifdef STATIC_IMAGE_PLUGIN
    m_objectNames.insertMulti(Image, QLatin1String("ImagePlugin"));
#else
    m_fileNames.insertMulti(Image, QLatin1String(IMAGE_PLUGIN_NAME));
#endif // STATIC_IMAGE_PLUGIN
#endif // WITH_IMAGE

#ifdef WITH_FITZ
#ifdef STATIC_FITZ_PLUGIN
    m_objectNames.insertMulti(PDF, QLatin1String("FitzPlugin"));
#else
    m_fileNames.insertMulti(PDF, QLatin1String(FITZ_PLUGIN_NAME));
#endif // STATIC_FITZ_PLUGIN
#endif // WITH_FITZ

#ifdef WITH_PDF
#ifdef STATIC_PDF_PLUGIN
    m_objectNames.insertMulti(PDF, QLatin1String("PdfPlugin"));
#else
    m_fileNames.insertMulti(PDF, QLatin1String(PDF_PLUGIN_NAME));
#endif // STATIC_PDF_PLUGIN
#endif // WITH_PDF

#ifdef WITH_PS
#ifdef STATIC_PS_PLUGIN
    m_objectNames.insertMulti(PS, QLatin1String("PsPlugin"));
#else
    m_fileNames.insertMulti(PS, QLatin1String(PS_PLUGIN_NAME));
#endif // STATIC_PS_PLUGIN
#endif // WITH_PS

#ifdef WITH_DJVU
#ifdef STATIC_DJVU_PLUGIN
    m_objectNames.insertMulti(DjVu, QLatin1String("DjVuPlugin"));
#else
    m_fileNames.insertMulti(DjVu, QLatin1String(DJVU_PLUGIN_NAME));
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
        if(Plugin* plugin = ::loadStaticPlugin(objectName))
        {
            m_plugins.insert(fileType, plugin);

            return true;
        }
    }

    foreach(const QString& fileName, m_fileNames.values(fileType))
    {
        if(Plugin* plugin = ::loadPlugin(fileName))
        {
            m_plugins.insert(fileType, plugin);

            return true;
        }
    }

    return false;
}

} // qpdfview

#ifdef STATIC_IMAGE_PLUGIN
    #if QT_VERSION < QT_VERSION_CHECK(5,0,0)
        Q_IMPORT_PLUGIN(qpdfview_image)
    #else
        Q_IMPORT_PLUGIN(ImagePlugin)
    #endif // QT_VERSION
#endif // STATIC_IMAGE_PLUGIN

#ifdef STATIC_FITZ_PLUGIN
    #if QT_VERSION < QT_VERSION_CHECK(5,0,0)
        Q_IMPORT_PLUGIN(qpdfview_fitz)
    #else
        Q_IMPORT_PLUGIN(FitzPlugin)
    #endif // QT_VERSION
#endif // STATIC_FITZ_PLUGIN

#ifdef STATIC_PDF_PLUGIN
    #if QT_VERSION < QT_VERSION_CHECK(5,0,0)
        Q_IMPORT_PLUGIN(qpdfview_pdf)
    #else
        Q_IMPORT_PLUGIN(PdfPlugin)
    #endif // QT_VERSION
#endif // STATIC_PDF_PLUGIN

#ifdef STATIC_PS_PLUGIN
    #if QT_VERSION < QT_VERSION_CHECK(5,0,0)
        Q_IMPORT_PLUGIN(qpdfview_ps)
    #else
        Q_IMPORT_PLUGIN(PsPlugin)
    #endif // QT_VERSION
#endif // STATIC_PS_PLUGIN

#ifdef STATIC_DJVU_PLUGIN
    #if QT_VERSION < QT_VERSION_CHECK(5,0,0)
        Q_IMPORT_PLUGIN(qpdfview_djvu)
    #else
        Q_IMPORT_PLUGIN(DjvuPlugin)
    #endif // QT_VERSION
#endif // STATIC_DJVU_PLUGIN
