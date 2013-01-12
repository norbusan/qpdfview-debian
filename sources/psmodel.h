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
#include <QSizeF>

struct SpectrePage;
struct SpectreDocument;
struct SpectreRenderContext;

#include "model.h"

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
    PSDocument(SpectreDocument* document);

    mutable QMutex m_mutex;
    SpectreDocument* m_document;
    SpectreRenderContext* m_renderContext;

};

class PSDocumentLoader : public QObject, DocumentLoader
{
    Q_OBJECT
    Q_INTERFACES(DocumentLoader)

public:
    PSDocumentLoader(QObject* parent = 0);

    Document* loadDocument(const QString& filePath) const;

};

#endif // PSMODEL_H
