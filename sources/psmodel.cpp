/*

Copyright 2013 Alexander Volkov
Copyright 2013 Adam Reichold

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

#include "psmodel.h"

#include <QFile>
#include <QFormLayout>
#include <qmath.h>
#include <QSettings>
#include <QSpinBox>
#include <QStandardItemModel>

#include <libspectre/spectre-document.h>

Model::PSPage::PSPage(QMutex* mutex, SpectrePage* page, SpectreRenderContext* renderContext) :
    m_mutex(mutex),
    m_page(page),
    m_renderContext(renderContext)
{
}

Model::PSPage::~PSPage()
{
    spectre_page_free(m_page);
    m_page = 0;
}

QSizeF Model::PSPage::size() const
{
    QMutexLocker mutexLocker(m_mutex);

    int w;
    int h;

    spectre_page_get_size(m_page, &w, &h);

    return QSizeF(w, h);
}

QImage Model::PSPage::render(qreal horizontalResolution, qreal verticalResolution, Rotation rotation, const QRect& boundingRect) const
{
    QMutexLocker mutexLocker(m_mutex);

    double xscale;
    double yscale;

    switch(rotation)
    {
    default:
    case RotateBy0:
    case RotateBy180:
        xscale = horizontalResolution / 72.0;
        yscale = verticalResolution / 72.0;
        break;
    case RotateBy90:
    case RotateBy270:
        xscale = verticalResolution / 72.0;
        yscale = horizontalResolution / 72.0;
        break;
    }

    spectre_render_context_set_scale(m_renderContext, xscale, yscale);

    switch(rotation)
    {
    default:
    case RotateBy0:
        spectre_render_context_set_rotation(m_renderContext, 0);
        break;
    case RotateBy90:
        spectre_render_context_set_rotation(m_renderContext, 90);
        break;
    case RotateBy180:
        spectre_render_context_set_rotation(m_renderContext, 180);
        break;
    case RotateBy270:
        spectre_render_context_set_rotation(m_renderContext, 270);
        break;
    }

    int w;
    int h;

    spectre_page_get_size(m_page, &w, &h);

    w = qRound(w * xscale);
    h = qRound(h * yscale);

    if(rotation == RotateBy90 || rotation == RotateBy270)
    {
        qSwap(w, h);
    }

    unsigned char* pageData = 0;
    int rowLength = 0;

    spectre_page_render(m_page, m_renderContext, &pageData, &rowLength);

    if (spectre_page_status(m_page) != SPECTRE_STATUS_SUCCESS)
    {
        free(pageData);
        pageData = 0;

        return QImage();
    }

    QImage auxiliaryImage(pageData, rowLength / 4, h, QImage::Format_RGB32);
    QImage image(boundingRect.isNull() ? auxiliaryImage.copy(0, 0, w, h) : auxiliaryImage.copy(boundingRect));

    free(pageData);
    pageData = 0;

    return image;
}

Model::PSDocument::PSDocument(SpectreDocument* document, SpectreRenderContext* renderContext) :
    m_mutex(),
    m_document(document),
    m_renderContext(renderContext)
{
}

Model::PSDocument::~PSDocument()
{
    spectre_render_context_free(m_renderContext);
    m_renderContext = 0;

    spectre_document_free(m_document);
    m_document = 0;
}

int Model::PSDocument::numberOfPages() const
{
    QMutexLocker mutexLocker(&m_mutex);

    return spectre_document_get_n_pages(m_document);
}

Model::Page* Model::PSDocument::page(int index) const
{
    QMutexLocker mutexLocker(&m_mutex);

    SpectrePage* page = spectre_document_get_page(m_document, index);

    return page != 0 ? new PSPage(&m_mutex, page, m_renderContext) : 0;
}

QStringList Model::PSDocument::saveFilter() const
{
    QMutexLocker mutexLocker(&m_mutex);

    if(spectre_document_is_eps(m_document))
    {
        return QStringList() << "Encapsulated PostScript (*.eps)";
    }
    else
    {
        return QStringList() << "PostScript (*.ps)";
    }
}

bool Model::PSDocument::canSave() const
{
    return true;
}

bool Model::PSDocument::save(const QString& filePath, bool withChanges) const
{
    Q_UNUSED(withChanges)

    QMutexLocker mutexLocker(&m_mutex);

    spectre_document_save(m_document, QFile::encodeName(filePath));

    return (spectre_document_status(m_document) == SPECTRE_STATUS_SUCCESS);
}

bool Model::PSDocument::canBePrinted() const
{
    return true;
}

void Model::PSDocument::loadProperties(QStandardItemModel* propertiesModel) const
{
    QMutexLocker mutexLocker(&m_mutex);

    QString title = QString::fromLocal8Bit(spectre_document_get_title(m_document));
    QString createdFor = QString::fromLocal8Bit(spectre_document_get_for(m_document));
    QString creator = QString::fromLocal8Bit(spectre_document_get_creator(m_document));
    QString creationDate = QString::fromLocal8Bit(spectre_document_get_creation_date(m_document));
    QString format = QString::fromLocal8Bit(spectre_document_get_format(m_document));
    QString languageLevel = QString::number(spectre_document_get_language_level(m_document));

    propertiesModel->clear();
    propertiesModel->setColumnCount(2);

    propertiesModel->appendRow(QList< QStandardItem* >() << new QStandardItem(tr("Title")) << new QStandardItem(title));
    propertiesModel->appendRow(QList< QStandardItem* >() << new QStandardItem(tr("Created for")) << new QStandardItem(createdFor));
    propertiesModel->appendRow(QList< QStandardItem* >() << new QStandardItem(tr("Creator")) << new QStandardItem(creator));
    propertiesModel->appendRow(QList< QStandardItem* >() << new QStandardItem(tr("Creation date")) << new QStandardItem(creationDate));
    propertiesModel->appendRow(QList< QStandardItem* >() << new QStandardItem(tr("Format")) << new QStandardItem(format));
    propertiesModel->appendRow(QList< QStandardItem* >() << new QStandardItem(tr("Language level")) << new QStandardItem(languageLevel));
}

Model::PSSettingsWidget::PSSettingsWidget(QSettings* settings, QWidget* parent) : SettingsWidget(parent),
    m_settings(settings)
{
    m_layout = new QFormLayout(this);

    // graphics antialias bits

    m_graphicsAntialiasBitsSpinBox = new QSpinBox(this);
    m_graphicsAntialiasBitsSpinBox->setRange(1, 4);
    m_graphicsAntialiasBitsSpinBox->setValue(m_settings->value("graphicsAntialiasBits", 4).toInt());

    m_layout->addRow(tr("Graphics antialias bits:"), m_graphicsAntialiasBitsSpinBox);

    // text antialias bits

    m_textAntialisBitsSpinBox = new QSpinBox(this);
    m_textAntialisBitsSpinBox->setRange(1, 2);
    m_textAntialisBitsSpinBox->setValue(m_settings->value("textAntialiasBits", 2).toInt());

    m_layout->addRow(tr("Text antialias bits:"), m_textAntialisBitsSpinBox);
}

void Model::PSSettingsWidget::accept()
{
    m_settings->setValue("graphicsAntialiasBits", m_graphicsAntialiasBitsSpinBox->value());
    m_settings->setValue("textAntialiasBits", m_textAntialisBitsSpinBox->value());
}

void Model::PSSettingsWidget::reset()
{
    m_graphicsAntialiasBitsSpinBox->setValue(4);
    m_textAntialisBitsSpinBox->setValue(2);
}

Model::PSDocumentLoader::PSDocumentLoader(QObject* parent) : QObject(parent)
{
    setObjectName("PSDocumentLoader");

    m_settings = new QSettings("qpdfview", "ps-plugin", this);
}

Model::Document* Model::PSDocumentLoader::loadDocument(const QString& filePath) const
{
    SpectreDocument* document = spectre_document_new();

    spectre_document_load(document, QFile::encodeName(filePath));

    if (spectre_document_status(document) != SPECTRE_STATUS_SUCCESS)
    {
        spectre_document_free(document);

        return 0;
    }

    SpectreRenderContext* renderContext = spectre_render_context_new();

    spectre_render_context_set_antialias_bits(renderContext,
                                              m_settings->value("graphicsAntialiasBits", 4).toInt(),
                                              m_settings->value("textAntialiasBits", 2).toInt());

    return new PSDocument(document, renderContext);
}

Model::SettingsWidget* Model::PSDocumentLoader::createSettingsWidget(QWidget* parent) const
{
    return new PSSettingsWidget(m_settings, parent);
}

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)

Q_EXPORT_PLUGIN2(qpdfview_ps, Model::PSDocumentLoader)

#endif // QT_VERSION
