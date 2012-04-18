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

#ifndef MISCELLANEOUS_H
#define MISCELLANEOUS_H

#include <QtCore>
#include <QtGui>

#include <poppler-qt4.h>

// presentation view

class PresentationView : public QWidget
{
    Q_OBJECT

private:
    struct Link
    {
        QRectF area;
        int page;

        Link() : area(), page(-1) {}
        Link(QRectF area, int page) : area(area), page(page) {}

    };

public:
    explicit PresentationView();
    ~PresentationView();

    void setCurrentPage(int currentPage);

public slots:
    bool open(const QString &filePath);

    void previousPage();
    void nextPage();
    void firstPage();
    void lastPage();

protected:
    void resizeEvent(QResizeEvent*);
    void paintEvent(QPaintEvent *event);

    void keyPressEvent(QKeyEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

private:
    Poppler::Document *m_document;
    Poppler::Page *m_page;

    // properties

    QString m_filePath;
    int m_numberOfPages;
    int m_currentPage;
    int m_renderedPage;

    // settings

    QSettings m_settings;

    // graphics

    qreal m_scale;
    QRectF m_boundingRect;

    QImage m_image;

    // links

    QList<Link> m_links;
    QTransform m_linkTransform;

    // internal methods

    void prepareView();

    // render

    QFuture<void> m_render;
    void render();
};

// recently used action

class RecentlyUsedAction : public QAction
{
    Q_OBJECT

public:
    RecentlyUsedAction(QObject *parent = 0);
    ~RecentlyUsedAction();

public slots:
    void addEntry(const QString &filePath);

    void clearList();

signals:
    void entrySelected(QString filePath);

protected slots:
    void slotActionGroupTriggered(QAction *action);

private:
    QMenu *m_menu;
    QActionGroup *m_actionGroup;
    QAction *m_separator;

    QAction *m_clearListAction;

    // settings

    QSettings m_settings;
};

// settings dialog

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = 0);

public slots:
    void accept();

private:
    QFormLayout *m_layout;
    QDialogButtonBox *m_buttonBox;

    QCheckBox *m_antialiasingCheckBox;
    QCheckBox *m_textAntialiasingCheckBox;
    QCheckBox *m_textHintingCheckBox;

    // settings

    QSettings m_settings;

};

// help dialog

class HelpDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HelpDialog(QWidget *parent = 0);

    QSize sizeHint() const
    {
        return QSize(500, 700);
    }

private:
    QTextBrowser *m_textBrowser;
    QDialogButtonBox *m_buttonBox;

};

#endif // MISCELLANEOUS_H
