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

#ifndef PSMODEL_H
#define PSMODEL_H

#include <QCoreApplication>
#include <QMutex>

class QFormLayout;
class QSettings;
class QSpinBox;

struct SpectrePage;
struct SpectreDocument;
struct SpectreRenderContext;

#include "model.h"

namespace Model
{

class PSPage : public Page
{
    friend class PSDocument;

public:
    ~PSPage();

    QSizeF size() const;

    QImage render(qreal horizontalResolution, qreal verticalResolution, Rotation rotation, const QRect& boundingRect) const;

private:
    PSPage(QMutex* mutex, SpectrePage* page, SpectreRenderContext* renderContext);

    mutable QMutex* m_mutex;
    SpectrePage* m_page;
    SpectreRenderContext* m_renderContext;

};

class PSDocument : public Document
{
    Q_DECLARE_TR_FUNCTIONS(PSDocument)

    friend class PSDocumentLoader;

public:
    ~PSDocument();

    int numberOfPages() const;

    Page* page(int index) const;

    QStringList saveFilter() const;

    bool canSave() const;
    bool save(const QString& filePath, bool withChanges) const;

    bool canBePrinted() const;

    void setAntialiasing(bool on);
    void setTextAntialiasing(bool on);

    void loadProperties(QStandardItemModel* propertiesModel) const;

private:
    PSDocument(SpectreDocument* document, SpectreRenderContext* renderContext);

    mutable QMutex m_mutex;
    SpectreDocument* m_document;
    SpectreRenderContext* m_renderContext;

};

class PSSettingsWidget : public SettingsWidget
{
    Q_OBJECT

public:
    PSSettingsWidget(QSettings* settings, QWidget* parent = 0);

    void accept();
    void reset();

private:
    QSettings* m_settings;

    QFormLayout* m_layout;

    QSpinBox* m_graphicsAntialiasBitsSpinBox;
    QSpinBox* m_textAntialisBitsSpinBox;

};

class PSDocumentLoader : public QObject, DocumentLoader
{
    Q_OBJECT
    Q_INTERFACES(Model::DocumentLoader)

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

    Q_PLUGIN_METADATA(IID "local.qpdfview.DocumentLoader")

#endif // QT_VERSION

public:
    PSDocumentLoader(QObject* parent = 0);

    Document* loadDocument(const QString& filePath) const;

    SettingsWidget* createSettingsWidget() const;

private:
    QSettings* m_settings;

};

}

#endif // PSMODEL_H
