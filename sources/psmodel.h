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

namespace qpdfview
{

class PsPlugin;

namespace Model
{
    class PsPage : public Page
    {
        friend class PsDocument;

    public:
        ~PsPage();

        QSizeF size() const;

        QImage render(qreal horizontalResolution, qreal verticalResolution, Rotation rotation, QRect boundingRect) const;

    private:
        Q_DISABLE_COPY(PsPage)

        PsPage(QMutex* mutex, SpectrePage* page, SpectreRenderContext* renderContext);

        mutable QMutex* m_mutex;
        SpectrePage* m_page;
        SpectreRenderContext* m_renderContext;

    };

    class PsDocument : public Document
    {
        Q_DECLARE_TR_FUNCTIONS(Model::PsDocument)

        friend class qpdfview::PsPlugin;

    public:
        ~PsDocument();

        int numberOfPages() const;

        Page* page(int index) const;

        QStringList saveFilter() const;

        bool canSave() const;
        bool save(const QString& filePath, bool withChanges) const;

        bool canBePrintedUsingCUPS() const;

        Properties properties() const;

    private:
        Q_DISABLE_COPY(PsDocument)

        PsDocument(SpectreDocument* document, SpectreRenderContext* renderContext);

        mutable QMutex m_mutex;
        SpectreDocument* m_document;
        SpectreRenderContext* m_renderContext;

    };
}

class PsSettingsWidget : public SettingsWidget
{
    Q_OBJECT

public:
    PsSettingsWidget(QSettings* settings, QWidget* parent = 0);

    void accept();
    void reset();

private:
    Q_DISABLE_COPY(PsSettingsWidget)

    QSettings* m_settings;

    QFormLayout* m_layout;

    QSpinBox* m_graphicsAntialiasBitsSpinBox;
    QSpinBox* m_textAntialiasBitsSpinBox;

};

class PsPlugin : public QObject, Plugin
{
    Q_OBJECT
    Q_INTERFACES(qpdfview::Plugin)

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

    Q_PLUGIN_METADATA(IID "local.qpdfview.Plugin")

#endif // QT_VERSION

public:
    PsPlugin(QObject* parent = 0);

    Model::Document* loadDocument(const QString& filePath) const;

    SettingsWidget* createSettingsWidget(QWidget* parent) const;

private:
    Q_DISABLE_COPY(PsPlugin)

    QSettings* m_settings;

};

} // qpdfview

#endif // PSMODEL_H
