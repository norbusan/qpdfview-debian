/*

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

#ifndef PDFMODEL_H
#define PDFMODEL_H

#include <QCoreApplication>
#include <QMutex>

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

namespace Model
{

class PDFAnnotation : public Annotation
{
    friend class PDFPage;

public:
    ~PDFAnnotation();

    QRectF boundary() const;
    QString contents() const;

    QDialog* showDialog(const QPoint& screenPos);

private:
    PDFAnnotation(QMutex* mutex, Poppler::Annotation* annotation);

    mutable QMutex* m_mutex;
    Poppler::Annotation* m_annotation;

};

class PDFFormField : public FormField
{
    friend class PDFPage;

public:
    ~PDFFormField();

    QRectF boundary() const;
    QString name() const;

    QDialog* showDialog(const QPoint& screenPos);

private:
    PDFFormField(QMutex* mutex, Poppler::FormField* formField);

    mutable QMutex* m_mutex;
    Poppler::FormField* m_formField;

};

class PDFPage : public Page
{
    Q_DECLARE_TR_FUNCTIONS(PDFPage)

    friend class PDFDocument;

public:
    ~PDFPage();

    QSizeF size() const;

    QImage render(qreal horizontalResolution, qreal verticalResolution, Rotation rotation, const QRect& boundingRect) const;

    QList< Link* > links() const;

    QString text(const QRectF& rect) const;
    QList< QRectF > search(const QString& text, bool matchCase) const;

    QList< Annotation* > annotations() const;

    bool canAddAndRemoveAnnotations() const;
    Annotation* addTextAnnotation(const QRectF& boundary);
    Annotation* addHighlightAnnotation(const QRectF& boundary);
    void removeAnnotation(Annotation* annotation);

    QList< FormField* > formFields() const;

private:
    PDFPage(QMutex* mutex, Poppler::Page* page);

    mutable QMutex* m_mutex;
    Poppler::Page* m_page;

};

class PDFDocument : public Document
{
    Q_DECLARE_TR_FUNCTIONS(PDFDocument)

    friend class PDFDocumentLoader;

public:
    ~PDFDocument();

    int numberOfPages() const;

    Page* page(int index) const;

    bool isLocked() const;
    bool unlock(const QString& password);

    QStringList saveFilter() const;

    bool canSave() const;
    bool save(const QString& filePath, bool withChanges) const;

    bool canBePrinted() const;

    void setPaperColor(const QColor& paperColor);

    void loadOutline(QStandardItemModel* outlineModel) const;
    void loadProperties(QStandardItemModel* propertiesModel) const;
    void loadFonts(QStandardItemModel* fontsModel) const;

private:
    PDFDocument(Poppler::Document* document);

    mutable QMutex m_mutex;
    Poppler::Document* m_document;

};

class PDFSettingsWidget : public SettingsWidget
{
    Q_OBJECT

public:
    PDFSettingsWidget(QSettings* settings, QWidget* parent = 0);

    void accept();
    void reset();

private:
    QSettings* m_settings;

    QFormLayout* m_layout;

    QCheckBox* m_antialiasingCheckBox;
    QCheckBox* m_textAntialiasingCheckBox;

#ifdef HAS_POPPLER_18

    QComboBox* m_textHintingComboBox;

#else

    QCheckBox* m_textHintingCheckBox;

#endif // HAS_POPPLER_18

#ifdef HAS_POPPLER_22

    QCheckBox* m_overprintPreviewCheckBox;

#endif // HAS_POPPLER_22

#ifdef HAS_POPPLER_24

    QComboBox* m_thinLineModeComboBox;

#endif // HAS_POPPLER_24

};

class PDFDocumentLoader : public QObject, DocumentLoader
{
    Q_OBJECT
    Q_INTERFACES(Model::DocumentLoader)

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

    Q_PLUGIN_METADATA(IID "local.qpdfview.DocumentLoader")

#endif // QT_VERSION

public:
    PDFDocumentLoader(QObject* parent = 0);

    Document* loadDocument(const QString& filePath) const;

    SettingsWidget* createSettingsWidget() const;

private:
    QSettings* m_settings;

};

}

#endif // PDFMODEL_H
