#ifndef DOCUMENTMODEL_H
#define DOCUMENTMODEL_H

#include <QtGlobal>
#include <QList>
#include <QRect>
#include <QRectF>
#include <QString>

class QColor;
class QDialog;
class QImage;
class QPrinter;
class QSizeF;
class QStandardItemModel;

#include "global.h"

struct Link
{
    QRectF boundary;

    int page;
    qreal left;
    qreal top;

    QString url;

    Link() : boundary(), page(-1), left(0.0), top(0.0), url() {}
    Link(const QRectF& boundary, int page, qreal left, qreal top) : boundary(boundary), page(page), left(left), top(top), url() {}
    Link(const QRectF& boundary, const QString& url) : boundary(boundary), page(-1), left(0.0), top(0.0), url(url) {}

};

class Annotation
{
public:
    virtual ~Annotation() {}

    virtual QRectF boundary() const = 0;
    virtual QString contents() const = 0;

    virtual QDialog* showDialog(const QPoint& screenPos) = 0;
};

class FormField
{
public:
    virtual ~FormField() {}

    virtual QRectF boundary() const = 0;
    virtual QString name() const = 0;

    virtual QDialog* showDialog(const QPoint& screenPos) = 0;
};

class Page
{
public:
    virtual ~Page() {}

    virtual QSizeF size() const = 0;

    virtual QImage render(qreal horizontalResolution = 72.0, qreal verticalResolution = 72.0, Rotation rotation = RotateBy0, const QRect& boundingRect = QRect()) const = 0;

    virtual QList< Link* > links() const { return QList< Link* >(); }

    virtual QString text(const QRectF& rect) const { Q_UNUSED(rect); return QString(); }
    virtual QList< QRectF > search(const QString& text, bool matchCase) const { Q_UNUSED(text); Q_UNUSED(matchCase); return QList< QRectF >(); }

    virtual QList< Annotation* > annotations() const { return QList< Annotation* >(); }

    virtual bool canAddAnnotations() const { return false; }
    virtual Annotation* addTextAnnotation(const QRectF& boundary) { Q_UNUSED(boundary); return 0; }
    virtual Annotation* addHighlightAnnotation(const QRectF& boundary) { Q_UNUSED(boundary); return 0; }
    virtual void removeAnnotation(Annotation* annotation) { Q_UNUSED(annotation); }

    virtual QList< FormField* > formFields() const { return QList< FormField* >(); }

};

class Document
{
public:
    static Document* load(const QString& filePath);

    virtual ~Document() {}

    virtual int numberOfPages() const = 0;

    virtual Page* page(int index) const = 0;

    virtual bool isLocked() const { return false; }
    virtual bool unlock(const QString& password) { Q_UNUSED(password); return false; }

    virtual bool canSave() const { return false; }
    virtual QString saveFilter() const { return QString(); }
    virtual bool save(const QString& filePath, bool withChanges) const { Q_UNUSED(filePath); Q_UNUSED(withChanges); return false; }

    virtual bool canPrint() const { return false; }

    virtual void setAntialiasing(bool on) { Q_UNUSED(on); }
    virtual void setTextAntialiasing(bool on) { Q_UNUSED(on); }
    virtual void setTextHinting(bool on) { Q_UNUSED(on); }

    virtual void setOverprintPreview(bool on) { Q_UNUSED(on); }

    virtual void setPaperColor(const QColor& paperColor) { Q_UNUSED(paperColor); }

    virtual void loadOutline(QStandardItemModel* outlineModel) const { Q_UNUSED(outlineModel); }
    virtual void loadProperties(QStandardItemModel* propertiesModel) const { Q_UNUSED(propertiesModel); }
    virtual void loadFonts(QStandardItemModel* fontsModel) const { Q_UNUSED(fontsModel); }

};

#endif // DOCUMENTMODEL_H
