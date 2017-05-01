/*

Copyright 2014 S. Razi Alavizadeh
Copyright 2013-2014 Adam Reichold

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

#ifndef PDFMODEL_H
#define PDFMODEL_H

#include <QCoreApplication>
#include <QMutex>
#include <QScopedPointer>

class QCheckBox;
class QComboBox;
class QFormLayout;
class QSettings;

namespace Poppler
{
class Annotation;
class Document;
class FormField;
class Page;
}

#include "model.h"

namespace qpdfview
{

class PdfPlugin;

namespace Model
{
    class PdfAnnotation : public Annotation
    {
        Q_OBJECT

        friend class PdfPage;

    public:
        ~PdfAnnotation();

        QRectF boundary() const;
        QString contents() const;

        QWidget* createWidget();

    private:
        Q_DISABLE_COPY(PdfAnnotation)

        PdfAnnotation(QMutex* mutex, Poppler::Annotation* annotation);

        mutable QMutex* m_mutex;
        Poppler::Annotation* m_annotation;

    };

    class PdfFormField : public FormField
    {
        Q_OBJECT

        friend class PdfPage;

    public:
        ~PdfFormField();

        QRectF boundary() const;
        QString name() const;

        QWidget* createWidget();

    private:
        Q_DISABLE_COPY(PdfFormField)

        PdfFormField(QMutex* mutex, Poppler::FormField* formField);

        mutable QMutex* m_mutex;
        Poppler::FormField* m_formField;

    };

    class PdfPage : public Page
    {
        Q_DECLARE_TR_FUNCTIONS(Model::PdfPage)

        friend class PdfDocument;

    public:
        ~PdfPage();

        QSizeF size() const;

        QImage render(qreal horizontalResolution, qreal verticalResolution, Rotation rotation, QRect boundingRect) const;

        QString label() const;

        QList< Link* > links() const;

        QString text(const QRectF& rect) const;
        QString cachedText(const QRectF& rect) const;

        QList< QRectF > search(const QString& text, bool matchCase, bool wholeWords) const;

        QList< Annotation* > annotations() const;

        bool canAddAndRemoveAnnotations() const;
        Annotation* addTextAnnotation(const QRectF& boundary, const QColor& color);
        Annotation* addHighlightAnnotation(const QRectF& boundary, const QColor& color);
        void removeAnnotation(Annotation* annotation);

        QList< FormField* > formFields() const;

    private:
        Q_DISABLE_COPY(PdfPage)

        PdfPage(QMutex* mutex, Poppler::Page* page);

        mutable QMutex* m_mutex;
        Poppler::Page* m_page;

    };

    class PdfDocument : public Document
    {
        Q_DECLARE_TR_FUNCTIONS(Model::PdfDocument)

        friend class qpdfview::PdfPlugin;

    public:
        ~PdfDocument();

        int numberOfPages() const;

        Page* page(int index) const;

        bool isLocked() const;
        bool unlock(const QString& password);

        QStringList saveFilter() const;

        bool canSave() const;
        bool save(const QString& filePath, bool withChanges) const;

        bool canBePrintedUsingCUPS() const;

        void setPaperColor(const QColor& paperColor);

        Outline outline() const;
        Properties properties() const;

        QAbstractItemModel* fonts() const;

        bool wantsContinuousMode() const;
        bool wantsSinglePageMode() const;
        bool wantsTwoPagesMode() const;
        bool wantsTwoPagesWithCoverPageMode() const;
        bool wantsRightToLeftMode() const;

    private:
        Q_DISABLE_COPY(PdfDocument)

        PdfDocument(Poppler::Document* document);

        mutable QMutex m_mutex;
        Poppler::Document* m_document;

    };
}

class PdfSettingsWidget : public SettingsWidget
{
    Q_OBJECT

public:
    PdfSettingsWidget(QSettings* settings, QWidget* parent = 0);

    void accept();
    void reset();

private:
    Q_DISABLE_COPY(PdfSettingsWidget)

    QSettings* m_settings;

    QFormLayout* m_layout;

    QCheckBox* m_antialiasingCheckBox;
    QCheckBox* m_textAntialiasingCheckBox;

#ifdef HAS_POPPLER_18

    QComboBox* m_textHintingComboBox;

#else

    QCheckBox* m_textHintingCheckBox;

#endif // HAS_POPPLER_18

#ifdef HAS_POPPLER_35

    QCheckBox* m_ignorePaperColorCheckBox;

#endif // HAS_POPPLER_35

#ifdef HAS_POPPLER_22

    QCheckBox* m_overprintPreviewCheckBox;

#endif // HAS_POPPLER_22

#ifdef HAS_POPPLER_24

    QComboBox* m_thinLineModeComboBox;

#endif // HAS_POPPLER_24

    QComboBox* m_backendComboBox;

};

class PdfPlugin : public QObject, Plugin
{
    Q_OBJECT
    Q_INTERFACES(qpdfview::Plugin)

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

    Q_PLUGIN_METADATA(IID "local.qpdfview.Plugin")

#endif // QT_VERSION

public:
    PdfPlugin(QObject* parent = 0);

    Model::Document* loadDocument(const QString& filePath) const;

    SettingsWidget* createSettingsWidget(QWidget* parent) const;

private:
    Q_DISABLE_COPY(PdfPlugin)

    QSettings* m_settings;

};

} // qpdfview

#endif // PDFMODEL_H
