/*

Copyright 2012 Adam Reichold

This file is part of qpdfview.

qpdfview is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

qpdfview is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with qpdfview.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef DOCUMENTVIEW_H
#define DOCUMENTVIEW_H

#include <QtCore>
#include <QtGui>

#include <poppler-qt4.h>

#include "pageitem.h"

class DocumentView : public QGraphicsView
{
    Q_OBJECT
public:
    enum DisplayModes { PagingMode, ScrollingMode, DoublePagingMode, DoubleScrollingMode };
    enum ScaleModes { ScaleFactorMode, FitToPageMode, FitToPageWidthMode };
    enum RotationModes { DoNotRotateMode, RotateBy90Mode, RotateBy180Mode, RotateBy270Mode };

    explicit DocumentView(QWidget *parent = 0);
    ~DocumentView();

    bool load(const QString &filePath);

    bool reload();
    bool save(const QString &filePath) const;
    bool print() const;

    const QString &filePath() const { return m_filePath; }

    const int &index() const { return m_index; }

    const DisplayModes &displayMode() const { return m_displayMode; }

    const ScaleModes &scaleMode() const { return m_scaleMode; }
    const qreal &scaleFactor() const { return m_scaleFactor; }

    const RotationModes &rotationMode() const { return m_rotationMode; }

signals:
    void documentChanged(QString);

    void indexChanged(int);

    void displayModeChanged(DocumentView::DisplayModes);

    void scaleModeChanged(DocumentView::ScaleModes);
    void scaleFactorChanged(qreal);

    void rotationModeChanged(DocumentView::RotationModes);
    
public slots:
    void setIndex(const int &index);
    void previousPage();
    void nextPage();
    void firstPage();
    void lastPage();

    void setDisplayMode(const DisplayModes &displayMode);

    void setScaleMode(const ScaleModes &scaleMode);
    void setScaleFactor(const qreal &scaleFactor);

    void setRotationMode(const RotationModes &rotationMode);

protected:
    void layout();

private:
    QGraphicsScene *m_graphicsScene;

    Poppler::Document *m_document;
    QList<Poppler::Page*> m_pageList;

    QString m_filePath;

    int m_index;

    DisplayModes m_displayMode;

    ScaleModes m_scaleMode;
    qreal m_scaleFactor;

    RotationModes m_rotationMode;

};

#endif // DOCUMENTVIEW_H
