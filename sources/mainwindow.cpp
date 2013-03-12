/*

Copyright 2012-2013 Adam Reichold
Copyright 2012 Michał Trybus
Copyright 2012 Alexander Volkov

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

#include "mainwindow.h"

#include <QApplication>
#include <QCheckBox>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QHeaderView>
#include <QInputDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPrintDialog>
#include <QPrinter>
#include <QScrollBar>
#include <QShortcut>
#include <QStandardItemModel>
#include <QTableView>
#include <QTextBrowser>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

#include <QStandardPaths>

#else

#include <QDesktopServices>

#endif // QT_VERSION

#ifdef WITH_SQL

#include <QSqlError>
#include <QSqlQuery>

#endif // WITH_SQL

#include "pageitem.h"
#include "documentview.h"
#include "printoptionswidget.h"
#include "settings.h"
#include "settingsdialog.h"
#include "recentlyusedmenu.h"
#include "bookmarkmenu.h"

MainWindow::MainWindow(const QString& instanceName, QWidget* parent) : QMainWindow(parent)
{
    setObjectName(instanceName);

    {
        // settings

        m_settings = new Settings(this);

        m_settings->refresh();

        if(m_settings->mainWindow()->hasIconTheme())
        {
            QIcon::setThemeName(m_settings->mainWindow()->iconTheme());
        }

        if(m_settings->mainWindow()->hasStyleSheet())
        {
            qApp->setStyleSheet(m_settings->mainWindow()->styleSheet());
        }
    }

    setAcceptDrops(true);

    createWidgets();
    createActions();
    createToolBars();
    createDocks();
    createMenus();

    restoreGeometry(m_settings->mainWindow()->geometry());
    restoreState(m_settings->mainWindow()->state());

    createDatabase();

    restoreTabs();
    restoreBookmarks();

    on_tabWidget_currentChanged(m_tabWidget->currentIndex());
}

QSize MainWindow::sizeHint() const
{
    return QSize(600, 800);
}

QMenu* MainWindow::createPopupMenu()
{
    QMenu* menu = new QMenu();

    menu->addAction(m_fileToolBar->toggleViewAction());
    menu->addAction(m_editToolBar->toggleViewAction());
    menu->addAction(m_viewToolBar->toggleViewAction());
    menu->addSeparator();
    menu->addAction(m_outlineDock->toggleViewAction());
    menu->addAction(m_propertiesDock->toggleViewAction());
    menu->addAction(m_thumbnailsDock->toggleViewAction());

    return menu;
}

bool MainWindow::open(const QString& filePath, int page, const QRectF& highlight)
{
    if(m_tabWidget->currentIndex() != -1)
    {
        savePerFileSettings(currentTab());

        if(currentTab()->open(filePath))
        {
            QFileInfo fileInfo(filePath);

            m_settings->mainWindow()->setOpenPath(fileInfo.absolutePath());
            m_recentlyUsedMenu->addOpenAction(filePath);

            m_tabWidget->setTabText(m_tabWidget->currentIndex(), fileInfo.completeBaseName());
            m_tabWidget->setTabToolTip(m_tabWidget->currentIndex(), fileInfo.absoluteFilePath());

            restorePerFileSettings(currentTab());

            currentTab()->jumpToPage(page, false);
            currentTab()->setFocus();

            if(!highlight.isNull())
            {
                currentTab()->jumpToHighlight(highlight);
            }

            return true;
        }
        else
        {
            QMessageBox::warning(this, tr("Warning"), tr("Could not open '%1'.").arg(filePath));
        }
    }

    return false;
}

bool MainWindow::openInNewTab(const QString& filePath, int page, const QRectF& highlight)
{
    DocumentView* newTab = new DocumentView(this);

    if(newTab->open(filePath))
    {
        newTab->setContinousMode(m_settings->documentView()->continuousMode());
        newTab->setLayoutMode(m_settings->documentView()->layoutMode());
        newTab->setScaleMode(m_settings->documentView()->scaleMode());
        newTab->setScaleFactor(m_settings->documentView()->scaleFactor());
        newTab->setRotation(m_settings->documentView()->rotation());
        newTab->setInvertColors(m_settings->documentView()->invertColors());
        newTab->setHighlightAll(m_settings->documentView()->highlightAll());

        QFileInfo fileInfo(filePath);

        m_settings->mainWindow()->setOpenPath(fileInfo.absolutePath());
        m_recentlyUsedMenu->addOpenAction(filePath);

        int index = m_tabWidget->insertTab(m_tabWidget->currentIndex() + 1, newTab, fileInfo.completeBaseName());
        m_tabWidget->setTabToolTip(index, fileInfo.absoluteFilePath());
        m_tabWidget->setCurrentIndex(index);

        QAction* tabAction = new QAction(m_tabWidget->tabText(index), newTab);
        connect(tabAction, SIGNAL(triggered()), SLOT(on_tabAction_triggered()));

        m_tabsMenu->addAction(tabAction);

        connect(newTab, SIGNAL(filePathChanged(QString)), SLOT(on_currentTab_filePathChanged(QString)));
        connect(newTab, SIGNAL(numberOfPagesChanged(int)), SLOT(on_currentTab_numberOfPagesChaned(int)));
        connect(newTab, SIGNAL(currentPageChanged(int)), SLOT(on_currentTab_currentPageChanged(int)));

        connect(newTab, SIGNAL(continousModeChanged(bool)), SLOT(on_currentTab_continuousModeChanged(bool)));
        connect(newTab, SIGNAL(layoutModeChanged(LayoutMode)), SLOT(on_currentTab_layoutModeChanged(LayoutMode)));
        connect(newTab, SIGNAL(scaleModeChanged(ScaleMode)), SLOT(on_currentTab_scaleModeChanged(ScaleMode)));
        connect(newTab, SIGNAL(scaleFactorChanged(qreal)), SLOT(on_currentTab_scaleFactorChanged(qreal)));
        connect(newTab, SIGNAL(rotationChanged(Rotation)), SLOT(on_currentTab_rotationChanged(Rotation)));

        connect(newTab, SIGNAL(linkClicked(QString,int)), SLOT(on_currentTab_linkClicked(QString,int)));

        connect(newTab, SIGNAL(invertColorsChanged(bool)), SLOT(on_currentTab_invertColorsChanged(bool)));
        connect(newTab, SIGNAL(highlightAllChanged(bool)), SLOT(on_currentTab_highlightAllChanged(bool)));
        connect(newTab, SIGNAL(rubberBandModeChanged(RubberBandMode)), SLOT(on_currentTab_rubberBandModeChanged(RubberBandMode)));

        connect(newTab, SIGNAL(searchProgressed(int)), SLOT(on_currentTab_searchProgressed(int)));
        connect(newTab, SIGNAL(searchFinished()), SLOT(on_currentTab_searchFinished()));
        connect(newTab, SIGNAL(searchCanceled()), SLOT(on_currentTab_searchCanceled()));

        connect(newTab, SIGNAL(customContextMenuRequested(QPoint)), SLOT(on_currentTab_customContextMenuRequested(QPoint)));

        newTab->show();

        restorePerFileSettings(newTab);

        newTab->jumpToPage(page, false);
        newTab->setFocus();

        if(!highlight.isNull())
        {
            newTab->jumpToHighlight(highlight);
        }

        return true;
    }
    else
    {
        delete newTab;

        QMessageBox::warning(this, tr("Warning"), tr("Could not open '%1'.").arg(filePath));
    }

    return false;
}

bool MainWindow::jumpToPageOrOpenInNewTab(const QString& filePath, int page, bool refreshBeforeJump, const QRectF& highlight)
{
    QFileInfo fileInfo(filePath);

    for(int index = 0; index < m_tabWidget->count(); ++index)
    {
        if(QFileInfo(tab(index)->filePath()).absoluteFilePath() == fileInfo.absoluteFilePath())
        {
            m_tabWidget->setCurrentIndex(index);

            if(refreshBeforeJump)
            {
                if(!currentTab()->refresh())
                {
                    return false;
                }
            }

            currentTab()->jumpToPage(page);
            currentTab()->setFocus();

            if(!highlight.isNull())
            {
                currentTab()->jumpToHighlight(highlight);
            }

            return true;
        }
    }

    return openInNewTab(filePath, page);
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    if(index != -1)
    {
        m_refreshAction->setEnabled(true);
        m_saveCopyAction->setEnabled(currentTab()->canSave());
        m_saveAsAction->setEnabled(currentTab()->canSave());
        m_printAction->setEnabled(true);

        m_previousPageAction->setEnabled(true);
        m_nextPageAction->setEnabled(true);
        m_firstPageAction->setEnabled(true);
        m_lastPageAction->setEnabled(true);

        m_jumpToPageAction->setEnabled(true);

        m_searchAction->setEnabled(true);
        m_findPreviousAction->setEnabled(true);
        m_findNextAction->setEnabled(true);
        m_cancelSearchAction->setEnabled(true);

        m_copyToClipboardModeAction->setEnabled(true);
        m_addAnnotationModeAction->setEnabled(true);

        m_continuousModeAction->setEnabled(true);
        m_twoPagesModeAction->setEnabled(true);
        m_twoPagesWithCoverPageModeAction->setEnabled(true);
        m_multiplePagesModeAction->setEnabled(true);

        m_zoomInAction->setEnabled(true);
        m_zoomOutAction->setEnabled(true);
        m_originalSizeAction->setEnabled(true);
        m_fitToPageWidthModeAction->setEnabled(true);
        m_fitToPageSizeModeAction->setEnabled(true);

        m_rotateLeftAction->setEnabled(true);
        m_rotateRightAction->setEnabled(true);

        m_invertColorsAction->setEnabled(true);

        m_fontsAction->setEnabled(true);

        m_presentationAction->setEnabled(true);

        m_previousTabAction->setEnabled(true);
        m_nextTabAction->setEnabled(true);
        m_closeTabAction->setEnabled(true);
        m_closeAllTabsAction->setEnabled(true);
        m_closeAllTabsButCurrentTabAction->setEnabled(true);

        m_previousBookmarkAction->setEnabled(true);
        m_nextBookmarkAction->setEnabled(true);
        m_addBookmarkAction->setEnabled(true);
        m_removeBookmarkAction->setEnabled(true);

        m_currentPageSpinBox->setEnabled(true);
        m_scaleFactorComboBox->setEnabled(true);
        m_searchProgressLineEdit->setEnabled(true);
        m_matchCaseCheckBox->setEnabled(true);
        m_highlightAllCheckBox->setEnabled(true);

        if(m_searchToolBar->isVisible())
        {
            m_searchTimer->stop();
            m_searchProgressLineEdit->setProgress(currentTab()->searchProgress());
        }

        m_outlineView->setModel(currentTab()->outlineModel());
        m_propertiesView->setModel(currentTab()->propertiesModel());

        m_thumbnailsView->setScene(currentTab()->thumbnailsScene());

        on_currentTab_filePathChanged(currentTab()->filePath());
        on_currentTab_numberOfPagesChaned(currentTab()->numberOfPages());
        on_currentTab_currentPageChanged(currentTab()->currentPage());

        on_currentTab_continuousModeChanged(currentTab()->continousMode());
        on_currentTab_layoutModeChanged(currentTab()->layoutMode());
        on_currentTab_scaleModeChanged(currentTab()->scaleMode());
        on_currentTab_scaleFactorChanged(currentTab()->scaleFactor());
        on_currentTab_rotationChanged(currentTab()->rotation());

        on_currentTab_invertColorsChanged(currentTab()->invertColors());
        on_currentTab_highlightAllChanged(currentTab()->highlightAll());
        on_currentTab_rubberBandModeChanged(currentTab()->rubberBandMode());
    }
    else
    {
        m_refreshAction->setEnabled(false);
        m_saveCopyAction->setEnabled(false);
        m_saveAsAction->setEnabled(false);
        m_printAction->setEnabled(false);

        m_previousPageAction->setEnabled(false);
        m_nextPageAction->setEnabled(false);
        m_firstPageAction->setEnabled(false);
        m_lastPageAction->setEnabled(false);

        m_jumpToPageAction->setEnabled(false);

        m_searchAction->setEnabled(false);
        m_findPreviousAction->setEnabled(false);
        m_findNextAction->setEnabled(false);
        m_cancelSearchAction->setEnabled(false);

        m_copyToClipboardModeAction->setEnabled(false);
        m_addAnnotationModeAction->setEnabled(false);

        m_continuousModeAction->setEnabled(false);
        m_twoPagesModeAction->setEnabled(false);
        m_twoPagesWithCoverPageModeAction->setEnabled(false);
        m_multiplePagesModeAction->setEnabled(false);

        m_zoomInAction->setEnabled(false);
        m_zoomOutAction->setEnabled(false);
        m_originalSizeAction->setEnabled(false);
        m_fitToPageWidthModeAction->setEnabled(false);
        m_fitToPageSizeModeAction->setEnabled(false);

        m_rotateLeftAction->setEnabled(false);
        m_rotateRightAction->setEnabled(false);

        m_invertColorsAction->setEnabled(false);

        m_fontsAction->setEnabled(false);

        m_presentationAction->setEnabled(false);

        m_previousTabAction->setEnabled(false);
        m_nextTabAction->setEnabled(false);
        m_closeTabAction->setEnabled(false);
        m_closeAllTabsAction->setEnabled(false);
        m_closeAllTabsButCurrentTabAction->setEnabled(false);

        m_previousBookmarkAction->setEnabled(false);
        m_nextBookmarkAction->setEnabled(false);
        m_addBookmarkAction->setEnabled(false);
        m_removeBookmarkAction->setEnabled(false);

        m_currentPageSpinBox->setEnabled(false);
        m_scaleFactorComboBox->setEnabled(false);
        m_searchProgressLineEdit->setEnabled(false);
        m_matchCaseCheckBox->setEnabled(false);
        m_highlightAllCheckBox->setEnabled(false);

        if(m_searchToolBar->isVisible())
        {
            m_searchTimer->stop();
            m_searchProgressLineEdit->setProgress(0);

            m_searchToolBar->setVisible(false);
        }

        m_outlineView->setModel(0);
        m_propertiesView->setModel(0);

        m_thumbnailsView->setScene(0);

        setWindowTitle("qpdfview");

        m_currentPageSpinBox->setValue(1);
        m_currentPageSpinBox->setSuffix(" / 1");
        m_scaleFactorComboBox->setCurrentIndex(4);

        m_copyToClipboardModeAction->setChecked(false);
        m_addAnnotationModeAction->setChecked(false);

        m_continuousModeAction->setChecked(false);
        m_twoPagesModeAction->setChecked(false);
        m_twoPagesWithCoverPageModeAction->setChecked(false);
        m_multiplePagesModeAction->setChecked(false);

        m_fitToPageSizeModeAction->setChecked(false);
        m_fitToPageWidthModeAction->setChecked(false);

        m_invertColorsAction->setChecked(false);
    }
}

void MainWindow::on_tabWidget_tabCloseRequested(int index)
{
    savePerFileSettings(tab(index));

    delete m_tabWidget->widget(index);
}

void MainWindow::on_currentTab_filePathChanged(const QString& filePath)
{
    for(int index = 0; index < m_tabWidget->count(); ++index)
    {
        if(sender() == m_tabWidget->widget(index))
        {
            QFileInfo fileInfo(filePath);

            m_tabWidget->setTabText(index, fileInfo.completeBaseName());
            m_tabWidget->setTabToolTip(index, fileInfo.absoluteFilePath());

            foreach(QAction* tabAction, m_tabsMenu->actions())
            {
                if(tabAction->parent() == m_tabWidget->widget(index))
                {
                    tabAction->setText(m_tabWidget->tabText(index));

                    break;
                }
            }

            break;
        }
    }

    if(senderIsCurrentTab())
    {
        setWindowTitle(m_tabWidget->tabText(m_tabWidget->currentIndex()) + " - qpdfview");
    }
}

void MainWindow::on_currentTab_numberOfPagesChaned(int numberOfPages)
{
    if(senderIsCurrentTab())
    {
        m_currentPageSpinBox->setRange(1, numberOfPages);
        m_currentPageSpinBox->setSuffix(QString(" / %1").arg(numberOfPages));
    }
}

void MainWindow::on_currentTab_currentPageChanged(int currentPage)
{
    if(senderIsCurrentTab())
    {
        m_currentPageSpinBox->setValue(currentPage);

        m_thumbnailsView->ensureVisible(currentTab()->thumbnails().at(currentPage - 1));
    }
}

void MainWindow::on_currentTab_continuousModeChanged(bool continuousMode)
{
    if(senderIsCurrentTab())
    {
        m_continuousModeAction->setChecked(continuousMode);

        m_settings->documentView()->setContinuousMode(continuousMode);
    }
}

void MainWindow::on_currentTab_layoutModeChanged(LayoutMode layoutMode)
{
    if(senderIsCurrentTab())
    {
        m_twoPagesModeAction->setChecked(layoutMode == TwoPagesMode);
        m_twoPagesWithCoverPageModeAction->setChecked(layoutMode == TwoPagesWithCoverPageMode);
        m_multiplePagesModeAction->setChecked(layoutMode == MultiplePagesMode);

        m_settings->documentView()->setLayoutMode(layoutMode);
    }
}

void MainWindow::on_currentTab_scaleModeChanged(ScaleMode scaleMode)
{
    if(senderIsCurrentTab())
    {
        switch(scaleMode)
        {
        default:
        case ScaleFactorMode:
            m_fitToPageWidthModeAction->setChecked(false);
            m_fitToPageSizeModeAction->setChecked(false);

            on_currentTab_scaleFactorChanged(currentTab()->scaleFactor());
            break;
        case FitToPageWidthMode:
            m_fitToPageWidthModeAction->setChecked(true);
            m_fitToPageSizeModeAction->setChecked(false);

            m_scaleFactorComboBox->setCurrentIndex(0);

            m_zoomInAction->setEnabled(true);
            m_zoomOutAction->setEnabled(true);
            break;
        case FitToPageSizeMode:
            m_fitToPageWidthModeAction->setChecked(false);
            m_fitToPageSizeModeAction->setChecked(true);

            m_scaleFactorComboBox->setCurrentIndex(1);

            m_zoomInAction->setEnabled(true);
            m_zoomOutAction->setEnabled(true);
            break;
        }

        m_settings->documentView()->setScaleMode(scaleMode);
    }
}

void MainWindow::on_currentTab_scaleFactorChanged(qreal scaleFactor)
{
    if(senderIsCurrentTab())
    {
        if(currentTab()->scaleMode() == ScaleFactorMode)
        {
            m_scaleFactorComboBox->setCurrentIndex(m_scaleFactorComboBox->findData(scaleFactor));
            m_scaleFactorComboBox->lineEdit()->setText(QString("%1 %").arg(qRound(scaleFactor * 100.0)));

            m_zoomInAction->setDisabled(qFuzzyCompare(scaleFactor, DocumentView::maximumScaleFactor()));
            m_zoomOutAction->setDisabled(qFuzzyCompare(scaleFactor, DocumentView::minimumScaleFactor()));
        }

        m_settings->documentView()->setScaleFactor(scaleFactor);
    }
}

void MainWindow::on_currentTab_rotationChanged(Rotation rotation)
{
    if(senderIsCurrentTab())
    {
        m_settings->documentView()->setRotation(rotation);
    }
}

void MainWindow::on_currentTab_linkClicked(const QString& filePath, int page)
{
    jumpToPageOrOpenInNewTab(filePath, page, true);
}

void MainWindow::on_currentTab_invertColorsChanged(bool invertColors)
{
    if(senderIsCurrentTab())
    {
        m_invertColorsAction->setChecked(invertColors);

        m_settings->documentView()->setInvertColors(invertColors);
    }
}

void MainWindow::on_currentTab_highlightAllChanged(bool highlightAll)
{
    if(senderIsCurrentTab())
    {
        m_highlightAllCheckBox->setChecked(highlightAll);

        m_settings->documentView()->setHighlightAll(highlightAll);
    }
}

void MainWindow::on_currentTab_rubberBandModeChanged(RubberBandMode rubberBandMode)
{
    if(senderIsCurrentTab())
    {
        m_copyToClipboardModeAction->setChecked(rubberBandMode == CopyToClipboardMode);
        m_addAnnotationModeAction->setChecked(rubberBandMode == AddAnnotationMode);
    }
}

void MainWindow::on_currentTab_searchProgressed(int progress)
{
    if(senderIsCurrentTab())
    {
        m_searchProgressLineEdit->setProgress(progress);
    }
}

void MainWindow::on_currentTab_searchFinished()
{
    if(senderIsCurrentTab())
    {
        m_searchProgressLineEdit->setProgress(0);
    }
}

void MainWindow::on_currentTab_searchCanceled()
{
    if(senderIsCurrentTab())
    {
        m_searchProgressLineEdit->setProgress(0);
    }
}

void MainWindow::on_currentTab_customContextMenuRequested(const QPoint& pos)
{
    if(senderIsCurrentTab())
    {
        QMenu menu;

        const QVector< int >& visitedPages = currentTab()->visitedPages();

        if(!visitedPages.isEmpty())
        {
            QAction* returnToPageAction = menu.addAction(tr("&Return to page %1").arg(visitedPages.first()),
                                                         currentTab(), SLOT(returnToPage()), DocumentView::returnToPageShortcut());

            returnToPageAction->setIcon(QIcon::fromTheme("go-jump", QIcon(":icons/go-jump.svg")));
            returnToPageAction->setIconVisibleInMenu(true);
        }

        if(m_searchToolBar->isVisible())
        {
            menu.addSeparator();
            menu.addActions(QList< QAction* >() << m_findPreviousAction << m_findNextAction << m_cancelSearchAction);
        }

        menu.addSeparator();
        menu.addActions(QList< QAction* >() << m_previousPageAction << m_nextPageAction << m_firstPageAction << m_lastPageAction);

        menu.addSeparator();
        menu.addActions(QList< QAction* >() << m_refreshAction << m_saveCopyAction << m_saveAsAction << m_printAction);

        menu.exec(currentTab()->mapToGlobal(pos));
    }
}

void MainWindow::on_currentPage_editingFinished()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        currentTab()->jumpToPage(m_currentPageSpinBox->value());
    }
}

void MainWindow::on_currentPage_returnPressed()
{
    currentTab()->setFocus();
}

void MainWindow::on_scaleFactor_activated(int index)
{
    if(index == 0)
    {
        currentTab()->setScaleMode(FitToPageWidthMode);
    }
    else if(index == 1)
    {
        currentTab()->setScaleMode(FitToPageSizeMode);
    }
    else
    {
        bool ok = false;
        qreal scaleFactor = m_scaleFactorComboBox->itemData(index).toReal(&ok);

        if(ok)
        {
            currentTab()->setScaleFactor(scaleFactor);
            currentTab()->setScaleMode(ScaleFactorMode);
        }
    }

    currentTab()->setFocus();
}

void MainWindow::on_scaleFactor_editingFinished()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        bool ok = false;
        qreal scaleFactor = m_scaleFactorComboBox->lineEdit()->text().toInt(&ok) / 100.0;

        scaleFactor = scaleFactor >= DocumentView::minimumScaleFactor() ? scaleFactor : DocumentView::minimumScaleFactor();
        scaleFactor = scaleFactor <= DocumentView::maximumScaleFactor() ? scaleFactor : DocumentView::maximumScaleFactor();

        if(ok)
        {
            currentTab()->setScaleFactor(scaleFactor);
            currentTab()->setScaleMode(ScaleFactorMode);
        }

        on_currentTab_scaleFactorChanged(currentTab()->scaleFactor());
        on_currentTab_scaleModeChanged(currentTab()->scaleMode());
    }
}

void MainWindow::on_scaleFactor_returnPressed()
{
    currentTab()->setFocus();
}

void MainWindow::on_open_triggered()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        QString path = m_settings->mainWindow()->openPath();
        QString filePath = QFileDialog::getOpenFileName(this, tr("Open"), path, DocumentView::openFilter().join(";;"));

        if(!filePath.isEmpty())
        {
            open(filePath);
        }
    }
    else
    {
        on_openInNewTab_triggered();
    }
}

void MainWindow::on_openInNewTab_triggered()
{
    QString path = m_settings->mainWindow()->openPath();
    QStringList filePaths = QFileDialog::getOpenFileNames(this, tr("Open in new tab"), path, DocumentView::openFilter().join(";;"));

    if(!filePaths.isEmpty())
    {
        disconnect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(on_tabWidget_currentChanged(int)));

        foreach(QString filePath, filePaths)
        {
            openInNewTab(filePath);
        }

        connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(on_tabWidget_currentChanged(int)));

        on_tabWidget_currentChanged(m_tabWidget->currentIndex());
    }
}

void MainWindow::on_refresh_triggered()
{
    if(!currentTab()->refresh())
    {
        QMessageBox::warning(this, tr("Warning"), tr("Could not refresh '%1'.").arg(currentTab()->filePath()));
    }
}

void MainWindow::on_saveCopy_triggered()
{
    QString path = m_settings->mainWindow()->savePath();
    QString filePath = QFileDialog::getSaveFileName(this, tr("Save copy"), QFileInfo(QDir(path), QFileInfo(currentTab()->filePath()).fileName()).filePath(), currentTab()->saveFilter().join(";;"));

    if(!filePath.isEmpty())
    {
        if(currentTab()->save(filePath, false))
        {
            m_settings->mainWindow()->setSavePath(QFileInfo(filePath).absolutePath());
        }
        else
        {
            QMessageBox::warning(this, tr("Warning"), tr("Could not save copy at '%1'.").arg(filePath));
        }
    }
}

void MainWindow::on_saveAs_triggered()
{
    QString path = m_settings->mainWindow()->savePath();
    QString filePath = QFileDialog::getSaveFileName(this, tr("Save as"), QFileInfo(QDir(path), QFileInfo(currentTab()->filePath()).fileName()).filePath(), currentTab()->saveFilter().join(";;"));

    if(!filePath.isEmpty())
    {
        if(currentTab()->save(filePath, true))
        {
            open(filePath, currentTab()->currentPage());

            m_settings->mainWindow()->setSavePath(QFileInfo(filePath).absolutePath());
        }
        else
        {
            QMessageBox::warning(this, tr("Warning"), tr("Could not save as '%1'.").arg(filePath));
        }
    }
}

void MainWindow::on_print_triggered()
{
    QPrinter* printer = new QPrinter();
    QPrintDialog* printDialog = new QPrintDialog(printer, this);

    printer->setDocName(QFileInfo(currentTab()->filePath()).completeBaseName());
    printer->setFullPage(true);

    printDialog->setMinMax(1, currentTab()->numberOfPages());
    printDialog->setOption(QPrintDialog::PrintToFile, false);

#if QT_VERSION >= QT_VERSION_CHECK(4,7,0)

    printDialog->setOption(QPrintDialog::PrintCurrentPage, true);

#endif // QT_VERSION

    PrintOptionsWidget* printOptionsWidget = new PrintOptionsWidget(this);

    printDialog->setOptionTabs(QList< QWidget* >() << printOptionsWidget);

    if(printDialog->exec() == QDialog::Accepted)
    {

#if QT_VERSION >= QT_VERSION_CHECK(4,7,0)

        if(printDialog->printRange() == QPrintDialog::CurrentPage)
        {
            printer->setFromTo(currentTab()->currentPage(), currentTab()->currentPage());
        }

#endif // QT_VERSION

        if(!currentTab()->print(printer, printOptionsWidget->printOptions()))
        {
            QMessageBox::warning(this, tr("Warning"), tr("Could not print '%1'.").arg(currentTab()->filePath()));
        }
    }

    delete printer;
    delete printDialog;
}

void MainWindow::on_recentlyUsed_openTriggered(const QString& filePath)
{
    if(!jumpToPageOrOpenInNewTab(filePath, -1, true))
    {
        m_recentlyUsedMenu->removeOpenAction(filePath);
    }
}

void MainWindow::on_previousPage_triggered()
{
    currentTab()->previousPage();
}

void MainWindow::on_nextPage_triggered()
{
    currentTab()->nextPage();
}

void MainWindow::on_firstPage_triggered()
{
    currentTab()->firstPage();
}

void MainWindow::on_lastPage_triggered()
{
    currentTab()->lastPage();
}

void MainWindow::on_jumpToPage_triggered()
{
    bool ok = false;
    int page = QInputDialog::getInt(this, tr("Jump to page"), tr("Page:"), currentTab()->currentPage(), 1, currentTab()->numberOfPages(), 1, &ok);

    if(ok)
    {
        currentTab()->jumpToPage(page);
    }
}

void MainWindow::on_search_triggered()
{
    if(!m_searchToolBar->isVisible())
    {
        m_searchToolBar->setVisible(true);
    }

    m_searchProgressLineEdit->selectAll();
    m_searchProgressLineEdit->setFocus();
}

void MainWindow::on_search_returnPressed(const Qt::KeyboardModifiers& modifiers)
{
    if(modifiers == Qt::ShiftModifier)
    {
        m_searchTimer->stop();

        if(!m_searchProgressLineEdit->text().isEmpty())
        {
            for(int index = 0; index < m_tabWidget->count(); ++index)
            {
                tab(index)->startSearch(m_searchProgressLineEdit->text(), m_matchCaseCheckBox->isChecked());
            }
        }
    }
    else
    {
        on_search_timeout();
    }
}

void MainWindow::on_search_timeout()
{
    m_searchTimer->stop();

    if(!m_searchProgressLineEdit->text().isEmpty())
    {
        currentTab()->startSearch(m_searchProgressLineEdit->text(), m_matchCaseCheckBox->isChecked());
    }
}

void MainWindow::on_findPrevious_triggered()
{
    if(!m_searchToolBar->isVisible())
    {
        on_search_triggered();
    }
    else
    {
        if(!m_searchProgressLineEdit->text().isEmpty())
        {
            currentTab()->findPrevious();
        }
    }
}

void MainWindow::on_findNext_triggered()
{
    if(!m_searchToolBar->isVisible())
    {
        on_search_triggered();
    }
    else
    {
        if(!m_searchProgressLineEdit->text().isEmpty())
        {
            currentTab()->findNext();
        }
    }
}

void MainWindow::on_cancelSearch_triggered()
{
    m_searchTimer->stop();
    m_searchProgressLineEdit->setProgress(0);

    m_searchToolBar->setVisible(false);

    for(int index = 0; index < m_tabWidget->count(); ++index)
    {
        tab(index)->cancelSearch();
    }
}

void MainWindow::on_copyToClipboardMode_triggered(bool checked)
{
    currentTab()->setRubberBandMode(checked ? CopyToClipboardMode : ModifiersMode);
}

void MainWindow::on_addAnnotationMode_triggered(bool checked)
{
    currentTab()->setRubberBandMode(checked ? AddAnnotationMode : ModifiersMode);
}

void MainWindow::on_settings_triggered()
{
    SettingsDialog* settingsDialog = new SettingsDialog(m_settings, this);

    if(settingsDialog->exec() == QDialog::Accepted)
    {
        m_settings->refresh();

        m_tabWidget->setTabPosition(m_settings->mainWindow()->tabPosition());
        m_tabWidget->setTabBarPolicy(m_settings->mainWindow()->tabVisibility());

        for(int index = 0; index < m_tabWidget->count(); ++index)
        {
            if(!tab(index)->refresh())
            {
                QMessageBox::warning(this, tr("Warning"), tr("Could not refresh '%1'.").arg(currentTab()->filePath()));
            }
        }
    }

    delete settingsDialog;
}

void MainWindow::on_continuousMode_triggered(bool checked)
{
    currentTab()->setContinousMode(checked);
}

void MainWindow::on_twoPagesMode_triggered(bool checked)
{
    currentTab()->setLayoutMode(checked ? TwoPagesMode : SinglePageMode);
}

void MainWindow::on_twoPagesWithCoverPageMode_triggered(bool checked)
{
    currentTab()->setLayoutMode(checked ? TwoPagesWithCoverPageMode : SinglePageMode);
}

void MainWindow::on_multiplePagesMode_triggered(bool checked)
{
    currentTab()->setLayoutMode(checked ? MultiplePagesMode : SinglePageMode);
}

void MainWindow::on_zoomIn_triggered()
{
    currentTab()->zoomIn();
}

void MainWindow::on_zoomOut_triggered()
{
    currentTab()->zoomOut();
}

void MainWindow::on_originalSize_triggered()
{
    currentTab()->originalSize();
}

void MainWindow::on_fitToPageWidthMode_triggered(bool checked)
{
    currentTab()->setScaleMode(checked ? FitToPageWidthMode : ScaleFactorMode);
}

void MainWindow::on_fitToPageSizeMode_triggered(bool checked)
{
    currentTab()->setScaleMode(checked ? FitToPageSizeMode : ScaleFactorMode);
}

void MainWindow::on_rotateLeft_triggered()
{
    currentTab()->rotateLeft();
}

void MainWindow::on_rotateRight_triggered()
{
    currentTab()->rotateRight();
}

void MainWindow::on_invertColors_triggered(bool checked)
{
    currentTab()->setInvertColors(checked);
}

void MainWindow::on_fonts_triggered()
{
    QStandardItemModel* fontsModel = currentTab()->fontsModel();
    QDialog* dialog = new QDialog(this);

    QTableView* tableView = new QTableView(dialog);
    tableView->setModel(fontsModel);

    tableView->setAlternatingRowColors(true);
    tableView->setSortingEnabled(true);
    tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    tableView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

#else

    tableView->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    tableView->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);

#endif // QT_VERSION

    tableView->verticalHeader()->setVisible(false);

    QDialogButtonBox* dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, dialog);
    connect(dialogButtonBox, SIGNAL(accepted()), dialog, SLOT(accept()));
    connect(dialogButtonBox, SIGNAL(rejected()), dialog, SLOT(reject()));

    dialog->setLayout(new QVBoxLayout(dialog));
    dialog->layout()->addWidget(tableView);
    dialog->layout()->addWidget(dialogButtonBox);

    dialog->resize(m_settings->mainWindow()->fontsDialogSize(dialog->sizeHint()));
    dialog->exec();
    m_settings->mainWindow()->setFontsDialogSize(dialog->size());

    delete fontsModel;
    delete dialog;
}

void MainWindow::on_fullscreen_triggered(bool checked)
{
    if(checked)
    {
        m_fullscreenAction->setData(saveGeometry());

        showFullScreen();
    }
    else
    {
        restoreGeometry(m_fullscreenAction->data().toByteArray());

        showNormal();

        restoreGeometry(m_fullscreenAction->data().toByteArray());
    }
}

void MainWindow::on_presentation_triggered()
{
    bool sync = m_settings->presentationView()->sync();
    int screen = m_settings->presentationView()->screen();

    currentTab()->presentation(sync, screen);
}

void MainWindow::on_previousTab_triggered()
{
    if(m_tabWidget->currentIndex() > 0)
    {
        m_tabWidget->setCurrentIndex(m_tabWidget->currentIndex() - 1);
    }
    else
    {
        m_tabWidget->setCurrentIndex(m_tabWidget->count() - 1);
    }
}

void MainWindow::on_nextTab_triggered()
{
    if(m_tabWidget->currentIndex() < m_tabWidget->count() - 1)
    {
        m_tabWidget->setCurrentIndex(m_tabWidget->currentIndex() + 1);
    }
    else
    {
        m_tabWidget->setCurrentIndex(0);
    }
}

void MainWindow::on_closeTab_triggered()
{
    savePerFileSettings(currentTab());

    delete m_tabWidget->currentWidget();
}

void MainWindow::on_closeAllTabs_triggered()
{
    disconnect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(on_tabWidget_currentChanged(int)));

    while(m_tabWidget->count() > 0)
    {
        savePerFileSettings(tab(0));

        delete m_tabWidget->widget(0);
    }

    connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(on_tabWidget_currentChanged(int)));

    on_tabWidget_currentChanged(-1);
}

void MainWindow::on_closeAllTabsButCurrentTab_triggered()
{
    DocumentView* newTab = currentTab();

    {
        disconnect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(on_tabWidget_currentChanged(int)));

        m_tabWidget->removeTab(m_tabWidget->currentIndex());

        while(m_tabWidget->count() > 0)
        {
            savePerFileSettings(tab(0));

            delete m_tabWidget->widget(0);
        }

        connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(on_tabWidget_currentChanged(int)));
    }

    QFileInfo fileInfo(newTab->filePath());

    int index = m_tabWidget->addTab(newTab, fileInfo.completeBaseName());
    m_tabWidget->setTabToolTip(index, fileInfo.absoluteFilePath());
    m_tabWidget->setCurrentIndex(index);
}

void MainWindow::on_tabAction_triggered()
{
    for(int index = 0; index < m_tabWidget->count(); ++index)
    {
        if(sender()->parent() == m_tabWidget->widget(index))
        {
            m_tabWidget->setCurrentIndex(index);

            break;
        }
    }
}

void MainWindow::on_tabShortcut_activated()
{
    for(int index = 0; index < 9; ++index)
    {
        if(sender() == m_tabShortcuts[index])
        {
            m_tabWidget->setCurrentIndex(index);

            break;
        }
    }
}

void MainWindow::on_previousBookmark_triggered()
{
    BookmarkMenu* bookmark = bookmarkForCurrentTab();

    if(bookmark != 0)
    {
        QList< int > pages = bookmark->pages();

        if(!pages.isEmpty())
        {
            qSort(pages);

            QList< int >::const_iterator lowerBound = --qLowerBound(pages, currentTab()->currentPage());

            if(lowerBound >= pages.constBegin())
            {
                currentTab()->jumpToPage(*lowerBound);
            }
            else
            {
                currentTab()->jumpToPage(pages.last());
            }
        }
    }
}

void MainWindow::on_nextBookmark_triggered()
{
    BookmarkMenu* bookmark = bookmarkForCurrentTab();

    if(bookmark != 0)
    {
        QList< int > pages = bookmark->pages();

        if(!pages.isEmpty())
        {
            qSort(pages);

            QList< int >::const_iterator upperBound = qUpperBound(pages, currentTab()->currentPage());

            if(upperBound < pages.constEnd())
            {
                currentTab()->jumpToPage(*upperBound);
            }
            else
            {
                currentTab()->jumpToPage(pages.first());
            }
        }
    }
}

void MainWindow::on_addBookmark_triggered()
{
    BookmarkMenu* bookmark = bookmarkForCurrentTab();

    if(bookmark != 0)
    {
        bookmark->addJumpToPageAction(currentTab()->currentPage());
    }
    else
    {
        bookmark = new BookmarkMenu(currentTab()->filePath(), this);

        bookmark->addJumpToPageAction(currentTab()->currentPage());

        connect(bookmark, SIGNAL(openTriggered(QString)), SLOT(on_bookmark_openTriggered(QString)));
        connect(bookmark, SIGNAL(openInNewTabTriggered(QString)), SLOT(on_bookmark_openInNewTabTriggered(QString)));
        connect(bookmark, SIGNAL(jumpToPageTriggered(QString,int)), SLOT(on_bookmark_jumpToPageTriggered(QString,int)));

        m_bookmarksMenu->addMenu(bookmark);
    }
}

void MainWindow::on_removeBookmark_triggered()
{
    BookmarkMenu* bookmark = bookmarkForCurrentTab();

    if(bookmark != 0)
    {
        bookmark->removeJumpToPageAction(currentTab()->currentPage());
    }
}

void MainWindow::on_removeAllBookmarks_triggered()
{
    foreach(QAction* action, m_bookmarksMenu->actions())
    {
        BookmarkMenu* bookmark = qobject_cast< BookmarkMenu* >(action->menu());

        if(bookmark != 0)
        {
            delete bookmark;
        }
    }
}

void MainWindow::on_bookmark_openTriggered(const QString& filePath)
{
    if(m_tabWidget->currentIndex() != -1)
    {
        open(filePath);
    }
    else
    {
        openInNewTab(filePath);
    }
}

void MainWindow::on_bookmark_openInNewTabTriggered(const QString& filePath)
{
    openInNewTab(filePath);
}

void MainWindow::on_bookmark_jumpToPageTriggered(const QString& filePath, int page)
{
    jumpToPageOrOpenInNewTab(filePath, page);
}

void MainWindow::on_contents_triggered()
{
    QDialog* dialog = new QDialog(this);

    QTextBrowser* textBrowser = new QTextBrowser(dialog);
    textBrowser->setSearchPaths(QStringList() << QDir(QApplication::applicationDirPath()).filePath("data") << DATA_INSTALL_PATH);
    textBrowser->setSource(QUrl("help.html"));

    QDialogButtonBox* dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, dialog);
    connect(dialogButtonBox, SIGNAL(accepted()), dialog, SLOT(accept()));
    connect(dialogButtonBox, SIGNAL(rejected()), dialog, SLOT(reject()));

    dialog->setLayout(new QVBoxLayout(dialog));
    dialog->layout()->addWidget(textBrowser);
    dialog->layout()->addWidget(dialogButtonBox);

    dialog->resize(m_settings->mainWindow()->contentsDialogSize(dialog->sizeHint()));
    dialog->exec();
    m_settings->mainWindow()->setContentsDialogSize(dialog->size());

    delete dialog;
}

void MainWindow::on_about_triggered()
{
    QMessageBox::about(this, tr("About qpdfview"), (tr("<p><b>qpdfview %1</b></p><p>qpdfview is a tabbed document viewer using Qt.</p>"
                                                      "<p>This version includes:"
                                                      "<ul>")
#ifdef WITH_PDF
                                                      + tr("<li>PDF support using Poppler</li>")
#endif // WITH_PDF
#ifdef WITH_PS
                                                      + tr("<li>PS support using libspectre</li>")
#endif // WITH_PS
#ifdef WITH_DJVU
                                                      + tr("<li>DjVu support using DjVuLibre</li>")
#endif // WITH_DJVU
#ifdef WITH_CUPS
                                                      + tr("<li>Printing support using CUPS</li>")
#endif // WITH_CUPS
                                                      + tr("</ul>"
                                                      "<p>See <a href=\"https://launchpad.net/qpdfview\">launchpad.net/qpdfview</a> for more information.</p><p>&copy; 2012-2013 The qpdfview developers</p>")).arg(QApplication::applicationVersion()));
}

void MainWindow::on_highlightAll_clicked(bool checked)
{
    currentTab()->setHighlightAll(checked);
}

void MainWindow::on_outline_clicked(const QModelIndex& index)
{
    bool ok = false;
    int page = m_outlineView->model()->data(index, Qt::UserRole + 1).toInt(&ok);
    qreal left = m_outlineView->model()->data(index, Qt::UserRole + 2).toReal();
    qreal top = m_outlineView->model()->data(index, Qt::UserRole + 3).toReal();

    if(ok)
    {
        currentTab()->jumpToPage(page, true, left, top);
    }
}

void MainWindow::on_thumbnails_verticalScrollBar_valueChanged(int value)
{
    Q_UNUSED(value);

    if(m_thumbnailsView->scene() != 0)
    {
        QRectF visibleRect = m_thumbnailsView->mapToScene(m_thumbnailsView->viewport()->rect()).boundingRect();

        foreach(ThumbnailItem* page, currentTab()->thumbnails())
        {
            if(!page->boundingRect().translated(page->pos()).intersects(visibleRect))
            {
                page->cancelRender();
            }
        }
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    saveTabs();
    saveBookmarks();

    for(int index = 0; index < m_tabWidget->count(); ++index)
    {
        savePerFileSettings(tab(index));
    }

    removeToolBar(m_searchToolBar);

    m_settings->mainWindow()->setRecentlyUsed(m_settings->mainWindow()->trackRecentlyUsed() ? m_recentlyUsedMenu->filePaths() : QStringList());

    m_settings->mainWindow()->setGeometry(m_fullscreenAction->isChecked() ? m_fullscreenAction->data().toByteArray() : saveGeometry());
    m_settings->mainWindow()->setState(saveState());

    QMainWindow::closeEvent(event);
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if(event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* event)
{
    if(event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();

        disconnect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(on_tabWidget_currentChanged(int)));

        foreach(QUrl url, event->mimeData()->urls())
        {
#if QT_VERSION >= QT_VERSION_CHECK(4,8,0)
            if(url.isLocalFile())
#else
            if(url.scheme() == "file")
#endif // QT_VERSION
            {
                openInNewTab(url.toLocalFile());
            }
        }

        connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(on_tabWidget_currentChanged(int)));

        on_tabWidget_currentChanged(m_tabWidget->currentIndex());
    }
}

DocumentView* MainWindow::currentTab() const
{
    return qobject_cast< DocumentView* >(m_tabWidget->currentWidget());
}

DocumentView* MainWindow::tab(int index) const
{
    return qobject_cast< DocumentView* >(m_tabWidget->widget(index));
}

bool MainWindow::senderIsCurrentTab() const
{
    return sender() == m_tabWidget->currentWidget() || qobject_cast< DocumentView* >(sender()) == 0;
}

BookmarkMenu* MainWindow::bookmarkForCurrentTab() const
{
    foreach(QAction* action, m_bookmarksMenu->actions())
    {
        BookmarkMenu* bookmark = qobject_cast< BookmarkMenu* >(action->menu());

        if(bookmark != 0)
        {
            if(QFileInfo(bookmark->filePath()).absoluteFilePath() == QFileInfo(currentTab()->filePath()).absoluteFilePath())
            {
                return bookmark;
            }
        }
    }

    return 0;
}

void MainWindow::createWidgets()
{
    m_tabWidget = new TabWidget(this);

    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setElideMode(Qt::ElideRight);

    m_tabWidget->setTabPosition(m_settings->mainWindow()->tabPosition());
    m_tabWidget->setTabBarPolicy(m_settings->mainWindow()->tabVisibility());

    setCentralWidget(m_tabWidget);

    connect(m_tabWidget, SIGNAL(currentChanged(int)), SLOT(on_tabWidget_currentChanged(int)));
    connect(m_tabWidget, SIGNAL(tabCloseRequested(int)), SLOT(on_tabWidget_tabCloseRequested(int)));

    // current page

    m_currentPageSpinBox = new SpinBox(this);

    m_currentPageSpinBox->setAlignment(Qt::AlignCenter);
    m_currentPageSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_currentPageSpinBox->setKeyboardTracking(false);

    connect(m_currentPageSpinBox, SIGNAL(editingFinished()), SLOT(on_currentPage_editingFinished()));
    connect(m_currentPageSpinBox, SIGNAL(returnPressed()), SLOT(on_currentPage_returnPressed()));

    m_currentPageSpinBox->setVisible(false);

    // scale factor

    m_scaleFactorComboBox = new ComboBox(this);

    m_scaleFactorComboBox->setEditable(true);
    m_scaleFactorComboBox->setInsertPolicy(QComboBox::NoInsert);

    m_scaleFactorComboBox->addItem(tr("Page width"));
    m_scaleFactorComboBox->addItem(tr("Page size"));
    m_scaleFactorComboBox->addItem("50 %", 0.5);
    m_scaleFactorComboBox->addItem("75 %", 0.75);
    m_scaleFactorComboBox->addItem("100 %", 1.0);
    m_scaleFactorComboBox->addItem("125 %", 1.25);
    m_scaleFactorComboBox->addItem("150 %", 1.5);
    m_scaleFactorComboBox->addItem("200 %", 2.0);
    m_scaleFactorComboBox->addItem("400 %", 4.0);

    connect(m_scaleFactorComboBox, SIGNAL(activated(int)), SLOT(on_scaleFactor_activated(int)));
    connect(m_scaleFactorComboBox->lineEdit(), SIGNAL(editingFinished()), SLOT(on_scaleFactor_editingFinished()));
    connect(m_scaleFactorComboBox->lineEdit(), SIGNAL(returnPressed()), SLOT(on_scaleFactor_returnPressed()));

    m_scaleFactorComboBox->setVisible(false);

    // search

    m_searchProgressLineEdit = new ProgressLineEdit(this);
    m_searchTimer = new QTimer(this);

    m_searchTimer->setInterval(2000);
    m_searchTimer->setSingleShot(true);

    connect(m_searchProgressLineEdit, SIGNAL(textEdited(QString)), m_searchTimer, SLOT(start()));
    connect(m_searchProgressLineEdit, SIGNAL(returnPressed(Qt::KeyboardModifiers)), SLOT(on_search_returnPressed(Qt::KeyboardModifiers)));
    connect(m_searchTimer, SIGNAL(timeout()), SLOT(on_search_timeout()));

    m_matchCaseCheckBox = new QCheckBox(tr("Match &case"), this);
    m_highlightAllCheckBox = new QCheckBox(tr("Highlight &all"), this);

    connect(m_highlightAllCheckBox, SIGNAL(clicked(bool)), SLOT(on_highlightAll_clicked(bool)));
}

void MainWindow::createActions()
{
    // open

    m_openAction = new QAction(tr("&Open..."), this);
    m_openAction->setObjectName(QLatin1String("open"));

    m_openAction->setShortcut(QKeySequence::Open);
    m_settings->shortcuts()->addAction(m_openAction);

    m_openAction->setIcon(QIcon::fromTheme("document-open", QIcon(":icons/document-open.svg")));
    m_openAction->setIconVisibleInMenu(true);

    connect(m_openAction, SIGNAL(triggered()), SLOT(on_open_triggered()));

    // open in new tab

    m_openInNewTabAction = new QAction(tr("Open in new &tab..."), this);
    m_openInNewTabAction->setObjectName(QLatin1String("openInNewTab"));

    m_openInNewTabAction->setShortcut(QKeySequence::AddTab);
    m_settings->shortcuts()->addAction(m_openInNewTabAction);

    m_openInNewTabAction->setIcon(QIcon::fromTheme("tab-new", QIcon(":icons/tab-new.svg")));
    m_openInNewTabAction->setIconVisibleInMenu(true);

    connect(m_openInNewTabAction, SIGNAL(triggered()), SLOT(on_openInNewTab_triggered()));

    // refresh

    m_refreshAction = new QAction(tr("&Refresh"), this);
    m_refreshAction->setObjectName(QLatin1String("refresh"));

    m_refreshAction->setShortcut(QKeySequence::Refresh);
    m_settings->shortcuts()->addAction(m_refreshAction);

    m_refreshAction->setIcon(QIcon::fromTheme("view-refresh", QIcon(":icons/view-refresh.svg")));
    m_refreshAction->setIconVisibleInMenu(true);

    connect(m_refreshAction, SIGNAL(triggered()), SLOT(on_refresh_triggered()));

    // save copy

    m_saveCopyAction = new QAction(tr("&Save copy..."), this);
    m_saveCopyAction->setObjectName(QLatin1String("saveCopy"));

    m_saveCopyAction->setShortcut(QKeySequence::Save);
    m_settings->shortcuts()->addAction(m_saveCopyAction);

    m_saveCopyAction->setIcon(QIcon::fromTheme("document-save", QIcon(":icons/document-save.svg")));
    m_saveCopyAction->setIconVisibleInMenu(true);

    connect(m_saveCopyAction, SIGNAL(triggered()), SLOT(on_saveCopy_triggered()));

    // save as

    m_saveAsAction = new QAction(tr("Save &as..."), this);
    m_saveAsAction->setObjectName(QLatin1String("saveAs"));

    m_saveAsAction->setShortcut(QKeySequence::SaveAs);
    m_settings->shortcuts()->addAction(m_saveAsAction);

    m_saveAsAction->setIcon(QIcon::fromTheme("document-save-as", QIcon(":icons/document-save-as.svg")));
    m_saveAsAction->setIconVisibleInMenu(true);

    connect(m_saveAsAction, SIGNAL(triggered()), SLOT(on_saveAs_triggered()));

    // print

    m_printAction = new QAction(tr("&Print..."), this);
    m_printAction->setObjectName(QLatin1String("print"));

    m_printAction->setShortcut(QKeySequence::Print);
    m_settings->shortcuts()->addAction(m_printAction);

    m_printAction->setIcon(QIcon::fromTheme("document-print", QIcon(":icons/document-print.svg")));
    m_printAction->setIconVisibleInMenu(true);

    connect(m_printAction, SIGNAL(triggered()), SLOT(on_print_triggered()));

    // exit

    m_exitAction = new QAction(tr("E&xit"), this);
    m_exitAction->setObjectName(QLatin1String("exit"));

    m_exitAction->setShortcut(QKeySequence::Quit);
    m_exitAction->setIcon(QIcon::fromTheme("application-exit"));

    m_exitAction->setIconVisibleInMenu(true);
    connect(m_exitAction, SIGNAL(triggered()), SLOT(close()));

    // previous page

    m_previousPageAction = new QAction(tr("&Previous page"), this);
    m_previousPageAction->setObjectName(QLatin1String("previousPage"));

    m_previousPageAction->setShortcut(QKeySequence(Qt::Key_Backspace));
    m_settings->shortcuts()->addAction(m_previousPageAction);

    m_previousPageAction->setIcon(QIcon::fromTheme("go-previous", QIcon(":icons/go-previous.svg")));
    m_previousPageAction->setIconVisibleInMenu(true);

    connect(m_previousPageAction, SIGNAL(triggered()), SLOT(on_previousPage_triggered()));

    // next page

    m_nextPageAction = new QAction(tr("&Next page"), this);
    m_nextPageAction->setObjectName(QLatin1String("nextPage"));

    m_nextPageAction->setShortcut(QKeySequence(Qt::Key_Space));
    m_settings->shortcuts()->addAction(m_nextPageAction);

    m_nextPageAction->setIcon(QIcon::fromTheme("go-next", QIcon(":icons/go-next.svg")));
    m_nextPageAction->setIconVisibleInMenu(true);

    connect(m_nextPageAction, SIGNAL(triggered()), SLOT(on_nextPage_triggered()));

    // first page

    m_firstPageAction = new QAction(tr("&First page"), this);
    m_firstPageAction->setObjectName(QLatin1String("firstPage"));

    m_firstPageAction->setShortcut(QKeySequence(Qt::Key_Home));
    m_settings->shortcuts()->addAction(m_firstPageAction);

    m_firstPageAction->setIcon(QIcon::fromTheme("go-first", QIcon(":icons/go-first.svg")));
    m_firstPageAction->setIconVisibleInMenu(true);

    connect(m_firstPageAction, SIGNAL(triggered()), SLOT(on_firstPage_triggered()));

    // last page

    m_lastPageAction = new QAction(tr("&Last page"), this);
    m_lastPageAction->setObjectName(QLatin1String("lastPage"));

    m_lastPageAction->setShortcut(QKeySequence(Qt::Key_End));
    m_settings->shortcuts()->addAction(m_lastPageAction);

    m_lastPageAction->setIcon(QIcon::fromTheme("go-last", QIcon(":icons/go-last.svg")));
    m_lastPageAction->setIconVisibleInMenu(true);

    connect(m_lastPageAction, SIGNAL(triggered()), SLOT(on_lastPage_triggered()));

    // jump to page

    m_jumpToPageAction = new QAction(tr("&Jump to page..."), this);
    m_jumpToPageAction->setObjectName(QLatin1String("jumpToPage"));

    m_jumpToPageAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_J));
    m_settings->shortcuts()->addAction(m_jumpToPageAction);

    m_jumpToPageAction->setIcon(QIcon::fromTheme("go-jump", QIcon(":icons/go-jump.svg")));
    m_jumpToPageAction->setIconVisibleInMenu(true);

    connect(m_jumpToPageAction, SIGNAL(triggered()), SLOT(on_jumpToPage_triggered()));

    // search

    m_searchAction = new QAction(tr("&Search..."), this);
    m_searchAction->setObjectName(QLatin1String("search"));

    m_searchAction->setShortcut(QKeySequence::Find);
    m_settings->shortcuts()->addAction(m_searchAction);

    m_searchAction->setIcon(QIcon::fromTheme("edit-find", QIcon(":icons/edit-find.svg")));
    m_searchAction->setIconVisibleInMenu(true);

    connect(m_searchAction, SIGNAL(triggered()), SLOT(on_search_triggered()));

    // find previous

    m_findPreviousAction = new QAction(tr("Find previous"), this);
    m_findPreviousAction->setObjectName(QLatin1String("findPrevious"));

    m_findPreviousAction->setShortcut(QKeySequence::FindPrevious);
    m_settings->shortcuts()->addAction(m_findPreviousAction);

    m_findPreviousAction->setIcon(QIcon::fromTheme("go-up", QIcon(":icons/go-up.svg")));
    m_findPreviousAction->setIconVisibleInMenu(true);

    connect(m_findPreviousAction, SIGNAL(triggered()), SLOT(on_findPrevious_triggered()));

    // find next

    m_findNextAction = new QAction(tr("Find next"), this);
    m_findNextAction->setObjectName(QLatin1String("findNext"));

    m_findNextAction->setShortcut(QKeySequence::FindNext);
    m_settings->shortcuts()->addAction(m_findNextAction);

    m_findNextAction->setIcon(QIcon::fromTheme("go-down", QIcon(":icons/go-down.svg")));
    m_findNextAction->setIconVisibleInMenu(true);

    connect(m_findNextAction, SIGNAL(triggered()), SLOT(on_findNext_triggered()));

    // cancel search

    m_cancelSearchAction = new QAction(tr("Cancel search"), this);
    m_cancelSearchAction->setObjectName(QLatin1String("cancelSearch"));

    m_cancelSearchAction->setShortcut(QKeySequence(Qt::Key_Escape));
    m_settings->shortcuts()->addAction(m_cancelSearchAction);

    m_cancelSearchAction->setIcon(QIcon::fromTheme("process-stop", QIcon(":icons/process-stop.svg")));
    m_cancelSearchAction->setIconVisibleInMenu(true);

    connect(m_cancelSearchAction, SIGNAL(triggered()), SLOT(on_cancelSearch_triggered()));

    // copy to clipboard mode

    m_copyToClipboardModeAction = new QAction(tr("&Copy to clipboard"), this);
    m_copyToClipboardModeAction->setObjectName(QLatin1String("copyToClipboardMode"));

    m_copyToClipboardModeAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_C));
    m_settings->shortcuts()->addAction(m_copyToClipboardModeAction);

    m_copyToClipboardModeAction->setIcon(QIcon::fromTheme("edit-copy", QIcon(":icons/edit-copy.svg")));

    m_copyToClipboardModeAction->setCheckable(true);
    connect(m_copyToClipboardModeAction, SIGNAL(triggered(bool)), SLOT(on_copyToClipboardMode_triggered(bool)));

    // add annotation mode

    m_addAnnotationModeAction = new QAction(tr("&Add annotation"), this);
    m_addAnnotationModeAction->setObjectName(QLatin1String("addAnnotationMode"));

    m_addAnnotationModeAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_A));
    m_settings->shortcuts()->addAction(m_addAnnotationModeAction);

    m_addAnnotationModeAction->setIcon(QIcon::fromTheme("mail-attachment", QIcon(":icons/mail-attachment.svg")));

    m_addAnnotationModeAction->setCheckable(true);
    connect(m_addAnnotationModeAction, SIGNAL(triggered(bool)), SLOT(on_addAnnotationMode_triggered(bool)));

    // settings

    m_settingsAction = new QAction(tr("Settings..."), this);

    connect(m_settingsAction, SIGNAL(triggered()), SLOT(on_settings_triggered()));

    // continuous mode

    m_continuousModeAction = new QAction(tr("&Continuous"), this);
    m_continuousModeAction->setObjectName(QLatin1String("continuousMode"));

    m_continuousModeAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_7));
    m_settings->shortcuts()->addAction(m_continuousModeAction);

    m_continuousModeAction->setIcon(QIcon(":icons/continuous.svg"));

    m_continuousModeAction->setCheckable(true);
    connect(m_continuousModeAction, SIGNAL(triggered(bool)), SLOT(on_continuousMode_triggered(bool)));

    // two pages mode

    m_twoPagesModeAction = new QAction(tr("&Two pages"), this);
    m_twoPagesModeAction->setObjectName(QLatin1String("twoPagesMode"));

    m_twoPagesModeAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_6));
    m_settings->shortcuts()->addAction(m_twoPagesModeAction);

    m_twoPagesModeAction->setIcon(QIcon(":icons/two-pages.svg"));

    m_twoPagesModeAction->setCheckable(true);
    connect(m_twoPagesModeAction, SIGNAL(triggered(bool)), SLOT(on_twoPagesMode_triggered(bool)));

    // two pages with cover page mode

    m_twoPagesWithCoverPageModeAction = new QAction(tr("Two pages &with cover page"), this);
    m_twoPagesWithCoverPageModeAction->setObjectName(QLatin1String("twoPagesWithCoverPageMode"));

    m_twoPagesWithCoverPageModeAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_5));
    m_settings->shortcuts()->addAction(m_twoPagesWithCoverPageModeAction);

    m_twoPagesWithCoverPageModeAction->setIcon(QIcon(":icons/two-pages-with-cover-page.svg"));

    m_twoPagesWithCoverPageModeAction->setCheckable(true);
    connect(m_twoPagesWithCoverPageModeAction, SIGNAL(triggered(bool)), SLOT(on_twoPagesWithCoverPageMode_triggered(bool)));

    // multiple pages mode

    m_multiplePagesModeAction = new QAction(tr("&Multiple pages"), this);
    m_multiplePagesModeAction->setObjectName(QLatin1String("multiplePagesMode"));

    m_multiplePagesModeAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_4));
    m_settings->shortcuts()->addAction(m_multiplePagesModeAction);

    m_multiplePagesModeAction->setIcon(QIcon(":icons/multiple-pages.svg"));

    m_multiplePagesModeAction->setCheckable(true);
    connect(m_multiplePagesModeAction, SIGNAL(triggered(bool)), SLOT(on_multiplePagesMode_triggered(bool)));

    // zoom in

    m_zoomInAction = new QAction(tr("Zoom &in"), this);
    m_zoomInAction->setObjectName(QLatin1String("zoomIn"));

    m_zoomInAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Up));
    m_settings->shortcuts()->addAction(m_zoomInAction);

    m_zoomInAction->setIcon(QIcon::fromTheme("zoom-in", QIcon(":icons/zoom-in.svg")));
    m_zoomInAction->setIconVisibleInMenu(true);

    connect(m_zoomInAction, SIGNAL(triggered()), SLOT(on_zoomIn_triggered()));

    // zoom out

    m_zoomOutAction = new QAction(tr("Zoom &out"), this);
    m_zoomOutAction->setObjectName(QLatin1String("zoomOut"));

    m_zoomOutAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Down));
    m_settings->shortcuts()->addAction(m_zoomOutAction);

    m_zoomOutAction->setIcon(QIcon::fromTheme("zoom-out", QIcon(":icons/zoom-out.svg")));
    m_zoomOutAction->setIconVisibleInMenu(true);

    connect(m_zoomOutAction, SIGNAL(triggered()), SLOT(on_zoomOut_triggered()));

    // original size

    m_originalSizeAction = new QAction(tr("Original &size"), this);
    m_originalSizeAction->setObjectName(QLatin1String("originalSize"));

    m_originalSizeAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_0));
    m_settings->shortcuts()->addAction(m_originalSizeAction);

    m_originalSizeAction->setIcon(QIcon::fromTheme("zoom-original", QIcon(":icons/zoom-original.svg")));
    m_originalSizeAction->setIconVisibleInMenu(true);

    connect(m_originalSizeAction, SIGNAL(triggered()), SLOT(on_originalSize_triggered()));

    // fit to page width mode

    m_fitToPageWidthModeAction = new QAction(tr("Fit to page width"), this);
    m_fitToPageWidthModeAction->setObjectName(QLatin1String("fitToPageWidthMode"));

    m_fitToPageWidthModeAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_9));
    m_settings->shortcuts()->addAction(m_fitToPageWidthModeAction);

    m_fitToPageWidthModeAction->setIcon(QIcon(":icons/fit-to-page-width.svg"));

    m_fitToPageWidthModeAction->setCheckable(true);
    connect(m_fitToPageWidthModeAction, SIGNAL(triggered(bool)), SLOT(on_fitToPageWidthMode_triggered(bool)));

    // fit to page size mode

    m_fitToPageSizeModeAction = new QAction(tr("Fit to page size"), this);
    m_fitToPageSizeModeAction->setObjectName(QLatin1String("fitToPageSizeMode"));

    m_fitToPageSizeModeAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_8));
    m_settings->shortcuts()->addAction(m_fitToPageSizeModeAction);

    m_fitToPageSizeModeAction->setIcon(QIcon(":icons/fit-to-page-size.svg"));

    m_fitToPageSizeModeAction->setCheckable(true);
    connect(m_fitToPageSizeModeAction, SIGNAL(triggered(bool)), SLOT(on_fitToPageSizeMode_triggered(bool)));

    // rotate left

    m_rotateLeftAction = new QAction(tr("Rotate &left"), this);
    m_rotateLeftAction->setObjectName(QLatin1String("rotateLeft"));

    m_rotateLeftAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Left));
    m_settings->shortcuts()->addAction(m_rotateLeftAction);

    m_rotateLeftAction->setIcon(QIcon::fromTheme("object-rotate-left", QIcon(":icons/object-rotate-left.svg")));
    m_rotateLeftAction->setIconVisibleInMenu(true);

    connect(m_rotateLeftAction, SIGNAL(triggered()), SLOT(on_rotateLeft_triggered()));

    // rotate right

    m_rotateRightAction = new QAction(tr("Rotate &right"), this);
    m_rotateRightAction->setObjectName(QLatin1String("rotateRight"));

    m_rotateRightAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Right));
    m_settings->shortcuts()->addAction(m_rotateRightAction);

    m_rotateRightAction->setIcon(QIcon::fromTheme("object-rotate-right", QIcon(":icons/object-rotate-right.svg")));
    m_rotateRightAction->setIconVisibleInMenu(true);

    connect(m_rotateRightAction, SIGNAL(triggered()), SLOT(on_rotateRight_triggered()));

    // invert colors

    m_invertColorsAction = new QAction(tr("Invert colors"), this);
    m_invertColorsAction->setObjectName(QLatin1String("invertColors"));

    m_invertColorsAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_I));
    m_settings->shortcuts()->addAction(m_invertColorsAction);

    m_invertColorsAction->setCheckable(true);
    connect(m_invertColorsAction, SIGNAL(triggered(bool)), SLOT(on_invertColors_triggered(bool)));

    // fonts

    m_fontsAction = new QAction(tr("Fonts..."), this);
    connect(m_fontsAction, SIGNAL(triggered()), SLOT(on_fonts_triggered()));

    // fullscreen

    m_fullscreenAction = new QAction(tr("&Fullscreen"), this);
    m_fullscreenAction->setObjectName(QLatin1String("fullscreen"));

    m_fullscreenAction->setShortcut(QKeySequence(Qt::Key_F11));
    m_settings->shortcuts()->addAction(m_fullscreenAction);

    m_fullscreenAction->setIcon(QIcon::fromTheme("view-fullscreen", QIcon(":icons/view-fullscreen.svg")));

    m_fullscreenAction->setCheckable(true);
    connect(m_fullscreenAction, SIGNAL(triggered(bool)), SLOT(on_fullscreen_triggered(bool)));

    // presentation

    m_presentationAction = new QAction(tr("&Presentation..."), this);
    m_presentationAction->setObjectName(QLatin1String("presentation"));

    m_presentationAction->setShortcut(QKeySequence(Qt::Key_F12));
    m_settings->shortcuts()->addAction(m_presentationAction);

    m_presentationAction->setIcon(QIcon::fromTheme("x-office-presentation", QIcon(":icons/x-office-presentation.svg")));
    m_presentationAction->setIconVisibleInMenu(true);

    connect(m_presentationAction, SIGNAL(triggered()), SLOT(on_presentation_triggered()));

    // previous tab

    m_previousTabAction = new QAction(tr("&Previous tab"), this);
    m_previousTabAction->setObjectName(QLatin1String("previousTab"));

    m_previousTabAction->setShortcut(QKeySequence::PreviousChild);
    m_settings->shortcuts()->addAction(m_previousTabAction);

    connect(m_previousTabAction, SIGNAL(triggered()), SLOT(on_previousTab_triggered()));

    // next tab

    m_nextTabAction = new QAction(tr("&Next tab"), this);
    m_nextTabAction->setObjectName(QLatin1String("nextTab"));

    m_nextTabAction->setShortcut(QKeySequence::NextChild);
    m_settings->shortcuts()->addAction(m_nextTabAction);

    connect(m_nextTabAction, SIGNAL(triggered()), SLOT(on_nextTab_triggered()));

    // close tab

    m_closeTabAction = new QAction(tr("&Close tab"), this);
    m_closeTabAction->setObjectName(QLatin1String("closeTab"));

    m_closeTabAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_W));
    m_settings->shortcuts()->addAction(m_closeTabAction);

    m_closeTabAction->setIcon(QIcon::fromTheme("window-close"));
    m_closeTabAction->setIconVisibleInMenu(true);

    connect(m_closeTabAction, SIGNAL(triggered()), SLOT(on_closeTab_triggered()));

    // close all tabs

    m_closeAllTabsAction = new QAction(tr("Close &all tabs"), this);
    m_closeAllTabsAction->setObjectName(QLatin1String("closeAllTabs"));

    m_closeAllTabsAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_W));
    m_settings->shortcuts()->addAction(m_closeAllTabsAction);

    connect(m_closeAllTabsAction, SIGNAL(triggered()), SLOT(on_closeAllTabs_triggered()));

    // close all tabs but current tab

    m_closeAllTabsButCurrentTabAction = new QAction(tr("Close all tabs &but current tab"), this);
    m_closeAllTabsButCurrentTabAction->setObjectName(QLatin1String("closeAllTabsButCurrent"));

    m_closeAllTabsButCurrentTabAction->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_W));
    m_settings->shortcuts()->addAction(m_closeAllTabsButCurrentTabAction);

    connect(m_closeAllTabsButCurrentTabAction, SIGNAL(triggered()), SLOT(on_closeAllTabsButCurrentTab_triggered()));

    // tab shortcuts

    for(int index = 0; index < 9; ++index)
    {
        m_tabShortcuts[index] = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_1 + index), this, SLOT(on_tabShortcut_activated()));
    }

    // previous bookmark

    m_previousBookmarkAction = new QAction(tr("&Previous bookmark"), this);
    m_previousBookmarkAction->setObjectName(QLatin1String("previousBookmarkAction"));

    m_previousBookmarkAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_PageUp));
    m_settings->shortcuts()->addAction(m_previousBookmarkAction);

    connect(m_previousBookmarkAction, SIGNAL(triggered()), SLOT(on_previousBookmark_triggered()));

    // next bookmark

    m_nextBookmarkAction = new QAction(tr("&Next bookmark"), this);
    m_nextBookmarkAction->setObjectName(QLatin1String("nextBookmarkAction"));

    m_nextBookmarkAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_PageDown));
    m_settings->shortcuts()->addAction(m_nextBookmarkAction);

    connect(m_nextBookmarkAction, SIGNAL(triggered()), SLOT(on_nextBookmark_triggered()));

    // add bookmark

    m_addBookmarkAction = new QAction(tr("&Add bookmark"), this);
    m_addBookmarkAction->setObjectName(QLatin1String("addBookmark"));

    m_addBookmarkAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_B));
    m_settings->shortcuts()->addAction(m_addBookmarkAction);

    connect(m_addBookmarkAction, SIGNAL(triggered()), SLOT(on_addBookmark_triggered()));

    // remove bookmark

    m_removeBookmarkAction = new QAction(tr("&Remove bookmark"), this);
    m_removeBookmarkAction->setObjectName(QLatin1String("removeBookmark"));

    m_removeBookmarkAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_B));
    m_settings->shortcuts()->addAction(m_removeBookmarkAction);

    connect(m_removeBookmarkAction, SIGNAL(triggered()), SLOT(on_removeBookmark_triggered()));

    // remove all bookmarks

    m_removeAllBookmarksAction = new QAction(tr("Remove all bookmarks"), this);
    m_removeAllBookmarksAction->setObjectName(QLatin1String("removeAllBookmark"));

    m_removeAllBookmarksAction->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_B));
    m_settings->shortcuts()->addAction(m_removeAllBookmarksAction);

    connect(m_removeAllBookmarksAction, SIGNAL(triggered()), SLOT(on_removeAllBookmarks_triggered()));

    // contents

    m_contentsAction = new QAction(tr("&Contents"), this);
    m_contentsAction->setObjectName(QLatin1String("contents"));

    m_contentsAction->setShortcut(QKeySequence::HelpContents);
    m_settings->shortcuts()->addAction(m_contentsAction);

    m_contentsAction->setIcon(QIcon::fromTheme("help-contents"));
    m_contentsAction->setIconVisibleInMenu(true);

    connect(m_contentsAction, SIGNAL(triggered()), SLOT(on_contents_triggered()));

    // about

    m_aboutAction = new QAction(tr("&About"), this);

    m_aboutAction->setIcon(QIcon::fromTheme("help-about"));
    m_aboutAction->setIconVisibleInMenu(true);

    connect(m_aboutAction, SIGNAL(triggered()), SLOT(on_about_triggered()));
}

void MainWindow::createToolBars()
{
    // file

    m_fileToolBar = addToolBar(tr("&File"));
    m_fileToolBar->setObjectName(QLatin1String("fileToolBar"));

    foreach(QString action, m_settings->mainWindow()->fileToolBar())
    {
        if(action == "open") { m_fileToolBar->addAction(m_openAction); }
        else if(action == "openInNewTab") { m_fileToolBar->addAction(m_openInNewTabAction); }
        else if(action == "refresh") { m_fileToolBar->addAction(m_refreshAction); }
        else if(action == "saveCopy") { m_fileToolBar->addAction(m_saveCopyAction); }
        else if(action == "saveAs") { m_fileToolBar->addAction(m_saveAsAction); }
        else if(action == "print") { m_fileToolBar->addAction(m_printAction); }
    }

    // edit

    m_editToolBar = addToolBar(tr("&Edit"));
    m_editToolBar->setObjectName(QLatin1String("editToolBar"));

    foreach(QString action, m_settings->mainWindow()->editToolBar())
    {
        if(action == "currentPage") { m_currentPageSpinBox->setVisible(true); m_editToolBar->addWidget(m_currentPageSpinBox); }
        else if(action == "previousPage") { m_editToolBar->addAction(m_previousPageAction); }
        else if(action == "nextPage") { m_editToolBar->addAction(m_nextPageAction); }
        else if(action == "firstPage") { m_editToolBar->addAction(m_firstPageAction); }
        else if(action == "lastPage") { m_editToolBar->addAction(m_lastPageAction); }
        else if(action == "jumpToPage") { m_editToolBar->addAction(m_jumpToPageAction); }
        else if(action == "search") { m_editToolBar->addAction(m_searchAction); }
        else if(action == "copyToClipboardMode") { m_editToolBar->addAction(m_copyToClipboardModeAction); }
        else if(action == "addAnnotationMode") { m_editToolBar->addAction(m_addAnnotationModeAction); }
    }

    // view

    m_viewToolBar = addToolBar(tr("&View"));
    m_viewToolBar->setObjectName(QLatin1String("viewToolBar"));

    foreach(QString action, m_settings->mainWindow()->viewToolBar())
    {
        if(action == "continuousMode") { m_viewToolBar->addAction(m_continuousModeAction); }
        else if(action == "twoPagesMode") { m_viewToolBar->addAction(m_twoPagesModeAction); }
        else if(action == "twoPagesWithCoverPageMode") { m_viewToolBar->addAction(m_twoPagesWithCoverPageModeAction); }
        else if(action == "multiplePagesMode") { m_viewToolBar->addAction(m_multiplePagesModeAction); }
        else if(action == "scaleFactor") { m_scaleFactorComboBox->setVisible(true); m_viewToolBar->addWidget(m_scaleFactorComboBox); }
        else if(action == "zoomIn") { m_viewToolBar->addAction(m_zoomInAction); }
        else if(action == "zoomOut") { m_viewToolBar->addAction(m_zoomOutAction); }
        else if(action == "originalSize") { m_viewToolBar->addAction(m_originalSizeAction); }
        else if(action == "fitToPageWidthMode") { m_viewToolBar->addAction(m_fitToPageWidthModeAction); }
        else if(action == "fitToPageSizeMode") { m_viewToolBar->addAction(m_fitToPageSizeModeAction); }
        else if(action == "rotateLeft") { m_viewToolBar->addAction(m_rotateLeftAction); }
        else if(action == "rotateRight") { m_viewToolBar->addAction(m_rotateRightAction); }
        else if(action == "fullscreen") { m_viewToolBar->addAction(m_fullscreenAction); }
        else if(action == "presentation") { m_viewToolBar->addAction(m_presentationAction); }
    }

    // search

    m_searchToolBar = new QToolBar(tr("&Search"), this);
    m_searchToolBar->setObjectName(QLatin1String("searchToolBar"));

    m_searchToolBar->setHidden(true);
    m_searchToolBar->setMovable(false);

    addToolBar(Qt::BottomToolBarArea, m_searchToolBar);

    m_searchToolBar->addWidget(m_searchProgressLineEdit);
    m_searchToolBar->addWidget(m_matchCaseCheckBox);
    m_searchToolBar->addWidget(m_highlightAllCheckBox);
    m_searchToolBar->addAction(m_findPreviousAction);
    m_searchToolBar->addAction(m_findNextAction);
    m_searchToolBar->addAction(m_cancelSearchAction);
}

void MainWindow::createDocks()
{
    // outline

    m_outlineDock = new QDockWidget(tr("&Outline"), this);
    m_outlineDock->setObjectName(QLatin1String("outlineDock"));
    m_outlineDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_outlineDock->setFeatures(QDockWidget::AllDockWidgetFeatures);

    addDockWidget(Qt::LeftDockWidgetArea, m_outlineDock);

    m_outlineDock->toggleViewAction()->setObjectName(QLatin1String("outlineDockToggleView"));
    m_outlineDock->toggleViewAction()->setShortcut(QKeySequence(Qt::Key_F6));
    m_settings->shortcuts()->addAction(m_outlineDock->toggleViewAction());

    m_outlineDock->hide();

    m_outlineView = new TreeView(this);
    m_outlineView->setAlternatingRowColors(true);
    m_outlineView->setEditTriggers(QAbstractItemView::NoEditTriggers);

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

    m_outlineView->header()->setSectionResizeMode(QHeaderView::Stretch);

#else

    m_outlineView->header()->setResizeMode(QHeaderView::Stretch);

#endif // QT_VERSION

    m_outlineView->header()->setVisible(false);

    connect(m_outlineView, SIGNAL(clicked(QModelIndex)), SLOT(on_outline_clicked(QModelIndex)));

    m_outlineDock->setWidget(m_outlineView);

    // properties

    m_propertiesDock = new QDockWidget(tr("&Properties"), this);
    m_propertiesDock->setObjectName(QLatin1String("propertiesDock"));
    m_propertiesDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_propertiesDock->setFeatures(QDockWidget::AllDockWidgetFeatures);

    addDockWidget(Qt::LeftDockWidgetArea, m_propertiesDock);

    m_propertiesDock->toggleViewAction()->setObjectName(QLatin1String("propertiesDockToggleView"));
    m_propertiesDock->toggleViewAction()->setShortcut(QKeySequence(Qt::Key_F7));
    m_settings->shortcuts()->addAction(m_propertiesDock->toggleViewAction());

    m_propertiesDock->hide();

    m_propertiesView = new QTableView(this);
    m_propertiesView->setAlternatingRowColors(true);
    m_propertiesView->setEditTriggers(QAbstractItemView::NoEditTriggers);

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

    m_propertiesView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_propertiesView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

#else

    m_propertiesView->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
    m_propertiesView->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);

#endif // QT_VERSION

    m_propertiesView->horizontalHeader()->setVisible(false);
    m_propertiesView->verticalHeader()->setVisible(false);

    m_propertiesDock->setWidget(m_propertiesView);

    // thumbnails

    m_thumbnailsDock = new QDockWidget(tr("&Thumbnails"), this);
    m_thumbnailsDock->setObjectName(QLatin1String("thumbnailsDock"));
    m_thumbnailsDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_thumbnailsDock->setFeatures(QDockWidget::AllDockWidgetFeatures);

    addDockWidget(Qt::RightDockWidgetArea, m_thumbnailsDock);

    m_thumbnailsDock->setObjectName(QLatin1String("thumbnailsDockToggleView"));
    m_thumbnailsDock->toggleViewAction()->setShortcut(QKeySequence(Qt::Key_F8));
    m_settings->shortcuts()->addAction(m_thumbnailsDock->toggleViewAction());

    m_thumbnailsDock->hide();

    m_thumbnailsView = new QGraphicsView(this);

    connect(m_thumbnailsView->verticalScrollBar(), SIGNAL(valueChanged(int)), SLOT(on_thumbnails_verticalScrollBar_valueChanged(int)));

    m_thumbnailsDock->setWidget(m_thumbnailsView);
}

void MainWindow::createMenus()
{
    // file

    m_fileMenu = menuBar()->addMenu(tr("&File"));
    m_fileMenu->addAction(m_openAction);
    m_fileMenu->addAction(m_openInNewTabAction);

    m_recentlyUsedMenu = new RecentlyUsedMenu(this);

    if(m_settings->mainWindow()->trackRecentlyUsed())
    {
        foreach(QString filePath, m_settings->mainWindow()->recentlyUsed())
        {
            m_recentlyUsedMenu->addOpenAction(filePath);
        }

        connect(m_recentlyUsedMenu, SIGNAL(openTriggered(QString)), SLOT(on_recentlyUsed_openTriggered(QString)));

        m_fileMenu->addMenu(m_recentlyUsedMenu);

        QToolButton* openToolButton = qobject_cast< QToolButton* >(m_fileToolBar->widgetForAction(m_openAction));
        if(openToolButton != 0)
        {
            openToolButton->setMenu(m_recentlyUsedMenu);
        }

        QToolButton* openInNewTabToolButton = qobject_cast< QToolButton* >(m_fileToolBar->widgetForAction(m_openInNewTabAction));
        if(openInNewTabToolButton != 0)
        {
            openInNewTabToolButton->setMenu(m_recentlyUsedMenu);
        }
    }

    m_fileMenu->addAction(m_refreshAction);
    m_fileMenu->addAction(m_saveCopyAction);
    m_fileMenu->addAction(m_saveAsAction);
    m_fileMenu->addAction(m_printAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_exitAction);

    // edit

    m_editMenu = menuBar()->addMenu(tr("&Edit"));
    m_editMenu->addAction(m_previousPageAction);
    m_editMenu->addAction(m_nextPageAction);
    m_editMenu->addAction(m_firstPageAction);
    m_editMenu->addAction(m_lastPageAction);
    m_editMenu->addAction(m_jumpToPageAction);
    m_editMenu->addSeparator();
    m_editMenu->addAction(m_searchAction);
    m_editMenu->addAction(m_findPreviousAction);
    m_editMenu->addAction(m_findNextAction);
    m_editMenu->addAction(m_cancelSearchAction);
    m_editMenu->addSeparator();
    m_editMenu->addAction(m_copyToClipboardModeAction);
    m_editMenu->addAction(m_addAnnotationModeAction);
    m_editMenu->addSeparator();
    m_editMenu->addAction(m_settingsAction);

    // view

    m_viewMenu = menuBar()->addMenu(tr("&View"));
    m_viewMenu->addAction(m_continuousModeAction);
    m_viewMenu->addAction(m_twoPagesModeAction);
    m_viewMenu->addAction(m_twoPagesWithCoverPageModeAction);
    m_viewMenu->addAction(m_multiplePagesModeAction);
    m_viewMenu->addSeparator();
    m_viewMenu->addAction(m_zoomInAction);
    m_viewMenu->addAction(m_zoomOutAction);
    m_viewMenu->addAction(m_originalSizeAction);
    m_viewMenu->addAction(m_fitToPageWidthModeAction);
    m_viewMenu->addAction(m_fitToPageSizeModeAction);
    m_viewMenu->addSeparator();
    m_viewMenu->addAction(m_rotateLeftAction);
    m_viewMenu->addAction(m_rotateRightAction);
    m_viewMenu->addSeparator();
    m_viewMenu->addAction(m_invertColorsAction);
    m_viewMenu->addSeparator();

    QMenu* toolBarsMenu = m_viewMenu->addMenu(tr("&Tool bars"));
    toolBarsMenu->addAction(m_fileToolBar->toggleViewAction());
    toolBarsMenu->addAction(m_editToolBar->toggleViewAction());
    toolBarsMenu->addAction(m_viewToolBar->toggleViewAction());

    QMenu* docksMenu = m_viewMenu->addMenu(tr("&Docks"));
    docksMenu->addAction(m_outlineDock->toggleViewAction());
    docksMenu->addAction(m_propertiesDock->toggleViewAction());
    docksMenu->addAction(m_thumbnailsDock->toggleViewAction());

    m_viewMenu->addAction(m_fontsAction);
    m_viewMenu->addSeparator();
    m_viewMenu->addAction(m_fullscreenAction);
    m_viewMenu->addAction(m_presentationAction);

    // tabs

    m_tabsMenu = menuBar()->addMenu(tr("&Tabs"));
    m_tabsMenu->addAction(m_previousTabAction);
    m_tabsMenu->addAction(m_nextTabAction);
    m_tabsMenu->addSeparator();
    m_tabsMenu->addAction(m_closeTabAction);
    m_tabsMenu->addAction(m_closeAllTabsAction);
    m_tabsMenu->addAction(m_closeAllTabsButCurrentTabAction);
    m_tabsMenu->addSeparator();

    // bookmarks

    m_bookmarksMenu = menuBar()->addMenu(tr("&Bookmarks"));
    m_bookmarksMenu->addAction(m_previousBookmarkAction);
    m_bookmarksMenu->addAction(m_nextBookmarkAction);
    m_bookmarksMenu->addSeparator();
    m_bookmarksMenu->addAction(m_addBookmarkAction);
    m_bookmarksMenu->addAction(m_removeBookmarkAction);
    m_bookmarksMenu->addAction(m_removeAllBookmarksAction);
    m_bookmarksMenu->addSeparator();

    // help

    m_helpMenu = menuBar()->addMenu(tr("&Help"));
    m_helpMenu->addAction(m_contentsAction);
    m_helpMenu->addAction(m_aboutAction);
}

void MainWindow::createDatabase()
{
#ifdef WITH_SQL

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

    QString path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);

#else

    QString path = QDesktopServices::storageLocation(QDesktopServices::DataLocation);

#endif // QT_VERSION

    QDir().mkpath(path);

    m_database = QSqlDatabase::addDatabase("QSQLITE");
    m_database.setDatabaseName(QDir(path).filePath("database"));
    m_database.open();

    if(m_database.isOpen())
    {
        m_database.transaction();

        QStringList tables = m_database.tables();
        QSqlQuery query(m_database);

        // tabs

        if(!tables.contains("tabs_v2"))
        {
            query.exec("CREATE TABLE tabs_v2 "
                       "(filePath TEXT"
                       ",instanceName TEXT"
                       ",currentPage INTEGER"
                       ",continuousMode INTEGER"
                       ",layoutMode INTEGER"
                       ",scaleMode INTEGER"
                       ",scaleFactor REAL"
                       ",rotation INTEGER)");

            if(!query.isActive())
            {
                qDebug() << query.lastError();
            }
        }

        // bookmarks

        if(!tables.contains("bookmarks_v1"))
        {
            query.exec("CREATE TABLE bookmarks_v1 "
                       "(filePath TEXT"
                       ",pages TEXT)");

            if(!query.isActive())
            {
                qDebug() << query.lastError();
            }
        }

        // per-file settings

        if(!tables.contains("perfilesettings_v1"))
        {
            query.exec("CREATE TABLE perfilesettings_v1 "
                       "(lastUsed INTEGER"
                       ",filePath TEXT PRIMARY KEY"
                       ",currentPage INTEGER"
                       ",continuousMode INTEGER"
                       ",layoutMode INTEGER"
                       ",scaleMode INTEGER"
                       ",scaleFactor REAL"
                       ",rotation INTEGER)");

            if(!query.isActive())
            {
                qDebug() << query.lastError();
            }
        }

        if(m_settings->mainWindow()->restorePerFileSettings())
        {
            query.exec("DELETE FROM perfilesettings_v1 WHERE filePath IN (SELECT filePath FROM perfilesettings_v1 ORDER BY lastUsed DESC LIMIT -1 OFFSET 1000)");
        }
        else
        {
            query.exec("DELETE FROM perfilesettings_v1");
        }

        if(!query.isActive())
        {
            qDebug() << query.lastError();
        }

        m_database.commit();
    }
    else
    {
        qDebug() << m_database.lastError();
    }

#endif // WITH_SQL
}

void MainWindow::restoreTabs()
{
#ifdef WITH_SQL

    if(m_database.isOpen())
    {
        m_database.transaction();

        QSqlQuery query(m_database);
        query.prepare("SELECT filePath,currentPage,continuousMode,layoutMode,scaleMode,scaleFactor,rotation FROM tabs_v2 WHERE instanceName==?");

        query.bindValue(0, objectName());

        query.exec();

        while(query.next())
        {
            if(!query.isActive())
            {
                qDebug() << query.lastError();
                break;
            }

            if(openInNewTab(query.value(0).toString()))
            {
                currentTab()->setContinousMode(static_cast< bool >(query.value(2).toUInt()));
                currentTab()->setLayoutMode(static_cast< LayoutMode >(query.value(3).toUInt()));

                currentTab()->setScaleMode(static_cast< ScaleMode >(query.value(4).toUInt()));
                currentTab()->setScaleFactor(query.value(5).toReal());

                currentTab()->setRotation(static_cast< Rotation >(query.value(6).toUInt()));

                currentTab()->jumpToPage(query.value(1).toInt());
            }
        }

        m_database.commit();
    }

#endif // WITH_SQL
}

void MainWindow::saveTabs()
{
#ifdef WITH_SQL

    if(m_database.isOpen())
    {
        m_database.transaction();

        QSqlQuery query(m_database);

        if(m_settings->mainWindow()->restoreTabs())
        {
            query.prepare("DELETE FROM tabs_v2 WHERE instanceName==?");

            query.bindValue(0, objectName());

            query.exec();

            if(!query.isActive())
            {
                qDebug() << query.lastError();
            }

            query.prepare("INSERT INTO tabs_v2 "
                          "(filePath,instanceName,currentPage,continuousMode,layoutMode,scaleMode,scaleFactor,rotation)"
                          " VALUES (?,?,?,?,?,?,?,?)");

            for(int index = 0; index < m_tabWidget->count(); ++index)
            {
                query.bindValue(0, QFileInfo(tab(index)->filePath()).absoluteFilePath());
                query.bindValue(1, objectName());
                query.bindValue(2, tab(index)->currentPage());

                query.bindValue(3, static_cast< uint >(tab(index)->continousMode()));
                query.bindValue(4, static_cast< uint >(tab(index)->layoutMode()));

                query.bindValue(5, static_cast< uint >(tab(index)->scaleMode()));
                query.bindValue(6, tab(index)->scaleFactor());

                query.bindValue(7, static_cast< uint >(tab(index)->rotation()));

                query.exec();

                if(!query.isActive())
                {
                    qDebug() << query.lastError();
                    break;
                }
            }
        }
        else
        {
            query.exec("DELETE FROM tabs_v2");

            if(!query.isActive())
            {
                qDebug() << query.lastError();
            }
        }

        m_database.commit();
    }

#endif // WITH_SQL
}

void MainWindow::restoreBookmarks()
{
#ifdef WITH_SQL

    if(m_database.isOpen())
    {
        m_database.transaction();

        QSqlQuery query(m_database);
        query.exec("SELECT filePath,pages FROM bookmarks_v1");

        while(query.next())
        {
            if(!query.isActive())
            {
                qDebug() << query.lastError();
                break;
            }

            BookmarkMenu* bookmark = new BookmarkMenu(query.value(0).toString(), this);

            QStringList pages = query.value(1).toString().split(",", QString::SkipEmptyParts);

            foreach(QString page, pages)
            {
                bookmark->addJumpToPageAction(page.toInt());
            }

            connect(bookmark, SIGNAL(openTriggered(QString)), SLOT(on_bookmark_openTriggered(QString)));
            connect(bookmark, SIGNAL(openInNewTabTriggered(QString)), SLOT(on_bookmark_openInNewTabTriggered(QString)));
            connect(bookmark, SIGNAL(jumpToPageTriggered(QString,int)), SLOT(on_bookmark_jumpToPageTriggered(QString,int)));

            m_bookmarksMenu->addMenu(bookmark);
        }

        m_database.commit();
    }

#endif // WITH_SQL
}

void MainWindow::saveBookmarks()
{
#ifdef WITH_SQL

    if(m_database.isOpen())
    {
        m_database.transaction();

        QSqlQuery query(m_database);
        query.exec("DELETE FROM bookmarks_v1");

        if(!query.isActive())
        {
            qDebug() << query.lastError();
        }

        if(m_settings->mainWindow()->restoreBookmarks())
        {
            query.prepare("INSERT INTO bookmarks_v1 "
                          "(filePath,pages)"
                          " VALUES (?,?)");

            foreach(QAction* action, m_bookmarksMenu->actions())
            {
                BookmarkMenu* bookmark = qobject_cast< BookmarkMenu* >(action->menu());

                if(bookmark != 0)
                {
                    QStringList pages;

                    foreach(int page, bookmark->pages())
                    {
                        pages.append(QString::number(page));
                    }

                    query.bindValue(0, QFileInfo(bookmark->filePath()).absoluteFilePath());
                    query.bindValue(1, pages.join(","));

                    query.exec();

                    if(!query.isActive())
                    {
                        qDebug() << query.lastError();
                        break;
                    }
                }
            }
        }

        m_database.commit();
    }

#endif // WITH_SQL
}

void MainWindow::restorePerFileSettings(DocumentView* tab)
{
#ifdef WITH_SQL

    if(m_settings->mainWindow()->restorePerFileSettings() && m_database.isOpen() && tab != 0)
    {
        m_database.transaction();

        QSqlQuery query(m_database);
        query.prepare("SELECT currentPage,continuousMode,layoutMode,scaleMode,scaleFactor,rotation FROM perfilesettings_v1 WHERE filePath==?");

        query.bindValue(0, QCryptographicHash::hash(QFileInfo(tab->filePath()).absoluteFilePath().toUtf8(), QCryptographicHash::Sha1).toBase64());

        query.exec();

        if(query.next())
        {
            tab->setContinousMode(query.value(1).toBool());
            tab->setLayoutMode(static_cast< LayoutMode >(query.value(2).toUInt()));

            tab->setScaleMode(static_cast< ScaleMode >(query.value(3).toUInt()));
            tab->setScaleFactor(query.value(4).toReal());

            tab->setRotation(static_cast< Rotation >(query.value(5).toUInt()));

            tab->jumpToPage(query.value(0).toInt(), false);
        }

        if(!query.isActive())
        {
            qDebug() << query.lastError();
        }

        m_database.commit();
    }

#else

    Q_UNUSED(tab);

#endif // WITH_SQL
}

void MainWindow::savePerFileSettings(const DocumentView* tab)
{
#ifdef WITH_SQL

    if(m_settings->mainWindow()->restorePerFileSettings() && m_database.isOpen() && tab != 0)
    {
        m_database.transaction();

        QSqlQuery query(m_database);
        query.prepare("INSERT OR REPLACE INTO perfilesettings_v1 "
                      "(lastUsed,filePath,currentPage,continuousMode,layoutMode,scaleMode,scaleFactor,rotation)"
                      " VALUES (?,?,?,?,?,?,?,?)");

        query.bindValue(0, QDateTime::currentDateTime().toTime_t());

        query.bindValue(1, QCryptographicHash::hash(QFileInfo(tab->filePath()).absoluteFilePath().toUtf8(), QCryptographicHash::Sha1).toBase64());
        query.bindValue(2, tab->currentPage());

        query.bindValue(3, static_cast< uint >(tab->continousMode()));
        query.bindValue(4, static_cast< uint >(tab->layoutMode()));

        query.bindValue(5, static_cast< uint >(tab->scaleMode()));
        query.bindValue(6, tab->scaleFactor());

        query.bindValue(7, static_cast< uint >(tab->rotation()));

        query.exec();

        if(!query.isActive())
        {
            qDebug() << query.lastError();
        }

        m_database.commit();
    }

#else

    Q_UNUSED(tab);

#endif // WITH_SQL
}

#ifdef WITH_DBUS

MainWindowAdaptor::MainWindowAdaptor(MainWindow* mainWindow) : QDBusAbstractAdaptor(mainWindow)
{
}

bool MainWindowAdaptor::open(const QString& filePath, int page, const QRectF& highlight)
{
    return mainWindow()->open(filePath, page, highlight);
}

bool MainWindowAdaptor::openInNewTab(const QString& filePath, int page, const QRectF& highlight)
{
    return mainWindow()->openInNewTab(filePath, page, highlight);
}

bool MainWindowAdaptor::jumpToPageOrOpenInNewTab(const QString& filePath, int page, bool refreshBeforeJump, const QRectF& highlight)
{
    return mainWindow()->jumpToPageOrOpenInNewTab(filePath, page, refreshBeforeJump, highlight);
}

void MainWindowAdaptor::raiseAndActivate()
{
    mainWindow()->raise();
    mainWindow()->activateWindow();
}

MainWindow* MainWindowAdaptor::mainWindow() const
{
    return qobject_cast< MainWindow* >(parent());
}

# endif // WITH_DBUS
