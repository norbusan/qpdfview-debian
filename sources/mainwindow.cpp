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

#include "mainwindow.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent),
    m_settings(),
    m_geometry()
{
    createActions();
    createWidgets();
    createToolBars();
    createDocks();
    createMenus();

    setAcceptDrops(true);

    slotTabWidgetCurrentChanged(-1);

    // settings

    DocumentView::fitToEqualWidth = m_settings.value("documentView/fitToEqualWidth", false).toBool();

    DocumentView::highlightLinks = m_settings.value("documentView/highlightLinks", true).toBool();
    DocumentView::externalLinks = m_settings.value("documentView/externalLinks", false).toBool();

    m_matchCaseCheckBox->setChecked(m_settings.value("mainWindow/matchCase", true).toBool());

    restoreGeometry(m_settings.value("mainWindow/geometry").toByteArray());
    restoreState(m_settings.value("mainWindow/state").toByteArray());

    m_tabWidget->setTabPosition(static_cast< QTabWidget::TabPosition >(m_settings.value("mainWindow/tabPosition", static_cast< uint >(m_tabWidget->tabPosition())).toUInt()));

    // restore tabs

    if(m_settings.value("mainWindow/restoreTabs", false).toBool())
    {
        QStringList filePaths = m_settings.value("mainWindow/tabs/filePaths", QStringList()).toStringList();
        QList< QVariant > currentPages = m_settings.value("mainWindow/tabs/currentPages", QList< QVariant >()).toList();
        QList< QVariant > pageLayouts = m_settings.value("mainWindow/tabs/pageLayouts", QList< QVariant >()).toList();
        QList< QVariant > scaleModes = m_settings.value("mainWindow/tabs/scaleModes", QList< QVariant >()).toList();
        QList< QVariant > scaleFactors = m_settings.value("mainWindow/tabs/scaleFactors", QList< QVariant >()).toList();
        QList< QVariant > rotations = m_settings.value("mainWindow/tabs/rotations", QList< QVariant >()).toList();

        for(int index = 0; index < filePaths.count(); index++)
        {
            openInNewTab(filePaths.at(index));

            DocumentView* documentView = qobject_cast< DocumentView* >(m_tabWidget->currentWidget()); Q_ASSERT(documentView);

            documentView->setCurrentPage(currentPages.at(index).toInt());
            documentView->setPageLayout(static_cast< DocumentView::PageLayout >(pageLayouts.at(index).toUInt()));
            documentView->setScaleMode(static_cast< DocumentView::ScaleMode >(scaleModes.at(index).toUInt()));
            documentView->setScaleFactor(scaleFactors.at(index).toReal());
            documentView->setRotation(static_cast< DocumentView::Rotation >(rotations.at(index).toUInt()));
        }

        m_tabWidget->setCurrentIndex(m_settings.value("mainWindow/tabs/currentIndex", -1).toInt());
    }
}

bool MainWindow::open(const QString& filePath, int page, qreal top)
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView* documentView = qobject_cast< DocumentView* >(m_tabWidget->currentWidget()); Q_ASSERT(documentView);

        if(documentView->open(filePath))
        {
            documentView->setCurrentPage(page, top);

            return true;
        }
        else
        {
            QMessageBox::warning(this, tr("Warning"), tr("Could not open document \"%1\".").arg(QFileInfo(filePath).fileName()));

            return false;
        }
    }
    else
    {
        return openInNewTab(filePath, page, top);
    }
}

bool MainWindow::openInNewTab(const QString& filePath, int page, qreal top)
{
    DocumentView* documentView = new DocumentView();

    if(documentView->open(filePath))
    {
        int index = m_tabWidget->addTab(documentView, QFileInfo(filePath).completeBaseName());
        m_tabWidget->setTabToolTip(index, QFileInfo(filePath).completeBaseName());
        m_tabWidget->setCurrentIndex(index);

        m_tabMenu->addAction(documentView->tabAction());

        connect(documentView, SIGNAL(filePathChanged(QString)), SLOT(slotFilePathChanged(QString)));
        connect(documentView, SIGNAL(numberOfPagesChanged(int)), SLOT(slotNumberOfPagesChanged(int)));

        connect(documentView, SIGNAL(currentPageChanged(int)), SLOT(slotCurrentPageChanged(int)));

        connect(documentView, SIGNAL(searchProgressed(int)), SLOT(slotSearchProgressed(int)));
        connect(documentView, SIGNAL(searchCanceled()), SLOT(slotSearchCanceled()));
        connect(documentView, SIGNAL(searchFinished()), SLOT(slotSearchFinished()));

        connect(documentView, SIGNAL(pageLayoutChanged(DocumentView::PageLayout)), SLOT(slotPageLayoutChanged(DocumentView::PageLayout)));
        connect(documentView, SIGNAL(scaleModeChanged(DocumentView::ScaleMode)), SLOT(slotScaleModeChanged(DocumentView::ScaleMode)));
        connect(documentView, SIGNAL(scaleFactorChanged(qreal)), SLOT(slotScaleFactorChanged(qreal)));

        connect(documentView, SIGNAL(highlightAllChanged(bool)), SLOT(slotHighlightAllChanged(bool)));

        documentView->setCurrentPage(page, top);

        return true;
    }
    else
    {
        delete documentView;

        QMessageBox::warning(this, tr("Warning"), tr("Could not open document \"%1\".").arg(QFileInfo(filePath).fileName()));

        return false;
    }
}

void MainWindow::closeEvent(QCloseEvent*)
{
    // restore tabs

    if(m_settings.value("mainWindow/restoreTabs", false).toBool())
    {
        QStringList filePaths;
        QList< QVariant > currentPages;
        QList< QVariant > pageLayouts;
        QList< QVariant > scaleModes;
        QList< QVariant > scaleFactors;
        QList< QVariant > rotations;

        for(int index = 0; index < m_tabWidget->count(); index++)
        {
            DocumentView* documentView = qobject_cast< DocumentView* >(m_tabWidget->widget(index));

            filePaths.append(QFileInfo(documentView->filePath()).absoluteFilePath());
            currentPages.append(documentView->currentPage());
            pageLayouts.append(static_cast< uint >(documentView->pageLayout()));
            scaleModes.append(static_cast< uint >(documentView->scaleMode()));
            scaleFactors.append(documentView->scaleFactor());
            rotations.append(static_cast< uint >(documentView->rotation()));
        }

        m_settings.setValue("mainWindow/tabs/filePaths", filePaths);
        m_settings.setValue("mainWindow/tabs/currentPages", currentPages);
        m_settings.setValue("mainWindow/tabs/pageLayouts", pageLayouts);
        m_settings.setValue("mainWindow/tabs/scaleModes", scaleModes);
        m_settings.setValue("mainWindow/tabs/scaleFactors", scaleFactors);
        m_settings.setValue("mainWindow/tabs/rotations", rotations);

        m_settings.setValue("mainWindow/tabs/currentIndex", m_tabWidget->currentIndex());
    }
    else
    {
        m_settings.remove("mainWindow/tabs/filePaths");
        m_settings.remove("mainWindow/tabs/currentPages");
        m_settings.remove("mainWindow/tabs/pageLayouts");
        m_settings.remove("mainWindow/tabs/scaleModes");
        m_settings.remove("mainWindow/tabs/scaleFactors");
        m_settings.remove("mainWindow/tabs/rotations");

        m_settings.remove("mainWindow/tabs/currentIndex");
    }

    // settings

    m_settings.setValue("mainWindow/matchCase", m_matchCaseCheckBox->isChecked());

    if(m_fullscreenAction->isChecked())
    {
        m_settings.setValue("mainWindow/geometry", m_geometry);
    }
    else
    {
        m_settings.setValue("mainWindow/geometry", saveGeometry());
    }

    m_settings.setValue("mainWindow/state", saveState());

    m_settings.setValue("mainWindow/tabPosition", static_cast< uint >(m_tabWidget->tabPosition()));

    slotCloseAllTabs();
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

        foreach(QUrl url, event->mimeData()->urls())
        {
            if(url.scheme() == "file" && QFileInfo(url.path()).exists())
            {
                openInNewTab(url.path());
            }
        }
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if(m_tabWidget->currentIndex() != -1)
    {
        QKeyEvent keyEvent(*event);

        QApplication::sendEvent(m_tabWidget->currentWidget(), &keyEvent);
    }
}

void MainWindow::slotOpen()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        QString filePath = QFileDialog::getOpenFileName(this, tr("Open document"),
                                                        m_settings.value("mainWindow/path", QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation)).toString(),
                                                        "Portable Document Format (*.pdf)");

        if(!filePath.isEmpty())
        {
            if(open(filePath))
            {
                m_recentlyUsedAction->addEntry(filePath);

                m_settings.setValue("mainWindow/path", QFileInfo(filePath).path());
            }
        }
    }
    else
    {
        slotOpenInNewTab();
    }
}

void MainWindow::slotOpenInNewTab()
{
    QStringList filePaths = QFileDialog::getOpenFileNames(this, tr("Open documents"),
                                                          m_settings.value("mainWindow/path", QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation)).toString(),
                                                          "Portable Document Format (*.pdf)");

    disconnect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(slotTabWidgetCurrentChanged(int)));

    foreach(QString filePath, filePaths)
    {
        if(openInNewTab(filePath))
        {
            m_recentlyUsedAction->addEntry(filePath);

            m_settings.setValue("mainWindow/path", QFileInfo(filePath).path());
        }
    }

    slotTabWidgetCurrentChanged(m_tabWidget->currentIndex());

    connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(slotTabWidgetCurrentChanged(int)));
}

void MainWindow::slotRecentyUsedActionEntrySelected(const QString& filePath)
{
    open(filePath);
}

void MainWindow::slotRefresh()
{
    DocumentView* documentView = qobject_cast< DocumentView* >(m_tabWidget->currentWidget()); Q_ASSERT(documentView);

    documentView->refresh();
}

void MainWindow::slotSaveCopy()
{
    QString filePath = QFileDialog::getSaveFileName(this, tr("Save copy"),
                                                    m_settings.value("mainWindow/path", QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation)).toString(),
                                                    "Portable Document Format (*.pdf)");

    if(!filePath.isEmpty())
    {
        DocumentView *documentView = qobject_cast< DocumentView* >(m_tabWidget->currentWidget()); Q_ASSERT(documentView);

        if(documentView->saveCopy(filePath))
        {
            m_settings.setValue("mainWindow/path", QFileInfo(filePath).path());
        }
        else
        {
            QMessageBox::warning(this, tr("Warning"), tr("Could not save copy at \"%1\".").arg(QFileInfo(filePath).filePath()));
        }
    }
}

void MainWindow::slotPrint()
{
    DocumentView* documentView = qobject_cast< DocumentView* >(m_tabWidget->currentWidget()); Q_ASSERT(documentView);

    QPrinter* printer = new QPrinter();

    QPrintDialog printDialog(printer, this);

    printDialog.setOptions(QAbstractPrintDialog::PrintDialogOptions());
    printDialog.setOption(QAbstractPrintDialog::PrintCollateCopies, true);
    printDialog.setOption(QAbstractPrintDialog::PrintPageRange, true);

    printDialog.setMinMax(1, documentView->numberOfPages());

    if(printDialog.exec() == QDialog::Accepted)
    {
        int fromPage = printDialog.fromPage() != 0 ? printDialog.fromPage() : 1;
        int toPage = printDialog.toPage() != 0 ? printDialog.toPage() : documentView->numberOfPages();

        QProgressDialog progressDialog;

        progressDialog.setLabelText(tr("Printing pages %1 to %2...").arg(fromPage).arg(toPage));
        progressDialog.setRange(0, 100);
        progressDialog.setValue(0);

        connect(documentView, SIGNAL(printProgressed(int)), &progressDialog, SLOT(setValue(int)));
        connect(documentView, SIGNAL(printCanceled()), &progressDialog, SLOT(close()));
        connect(documentView, SIGNAL(printFinished()), &progressDialog, SLOT(close()));

        connect(&progressDialog, SIGNAL(canceled()), documentView, SLOT(cancelPrint()));

        documentView->startPrint(printer, fromPage, toPage);

        progressDialog.exec();
    }
}

void MainWindow::slotPreviousPage()
{
    DocumentView* documentView = qobject_cast< DocumentView* >(m_tabWidget->currentWidget()); Q_ASSERT(documentView);

    documentView->previousPage();
}

void MainWindow::slotNextPage()
{
    DocumentView* documentView = qobject_cast< DocumentView* >(m_tabWidget->currentWidget()); Q_ASSERT(documentView);

    documentView->nextPage();
}

void MainWindow::slotFirstPage()
{
    DocumentView* documentView = qobject_cast< DocumentView* >(m_tabWidget->currentWidget()); Q_ASSERT(documentView);

    documentView->firstPage();
}

void MainWindow::slotLastPage()
{
    DocumentView* documentView = qobject_cast< DocumentView* >(m_tabWidget->currentWidget()); Q_ASSERT(documentView);

    documentView->lastPage();
}

void MainWindow::slotSearch()
{
    if(m_searchToolBar->isHidden())
    {
        m_searchToolBar->show();
    }
    else
    {
        m_searchLineEdit->selectAll();
    }

    m_searchLineEdit->setFocus();
}

void MainWindow::slotStartSearch()
{
    m_searchTimer->stop();

    if(m_searchToolBar->isVisible() && !m_searchLineEdit->text().isEmpty())
    {
        DocumentView* documentView = qobject_cast< DocumentView* >(m_tabWidget->currentWidget()); Q_ASSERT(documentView);

        documentView->startSearch(m_searchLineEdit->text(), m_matchCaseCheckBox->isChecked());
    }
}

void MainWindow::slotSearchProgressed(int value)
{
    statusBar()->show();
    statusBar()->showMessage(tr("Searched %1% of the the current document...").arg(value));
}

void MainWindow::slotSearchCanceled()
{
    statusBar()->clearMessage();
    statusBar()->hide();
}

void MainWindow::slotSearchFinished()
{
    statusBar()->clearMessage();
    statusBar()->hide();
}

void MainWindow::slotFindPrevious()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        if(m_searchToolBar->isHidden())
        {
            m_searchToolBar->show();
            m_searchLineEdit->setFocus();
        }
        else
        {
            if(!m_searchLineEdit->text().isEmpty())
            {
                DocumentView* documentView = qobject_cast< DocumentView* >(m_tabWidget->currentWidget()); Q_ASSERT(documentView);

                documentView->findPrevious();
            }
        }
    }
}

void MainWindow::slotFindNext()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        if(m_searchToolBar->isHidden())
        {
            m_searchToolBar->show();
            m_searchLineEdit->setFocus();
        }
        else
        {
            if(!m_searchLineEdit->text().isEmpty())
            {
                DocumentView* documentView = qobject_cast< DocumentView* >(m_tabWidget->currentWidget()); Q_ASSERT(documentView);

                documentView->findNext();
            }
        }
    }
}

void MainWindow::slotCancelSearch()
{
    m_searchLineEdit->clear();
    m_searchTimer->stop();
    m_searchToolBar->hide();

    DocumentView* documentView = qobject_cast< DocumentView* >(m_tabWidget->currentWidget()); Q_ASSERT(documentView);

    documentView->cancelSearch();
}

void MainWindow::slotSettings()
{
    SettingsDialog settingsDialog;

    if(settingsDialog.exec() == QDialog::Accepted)
    {
        DocumentView::fitToEqualWidth = m_settings.value("documentView/fitToEqualWidth", false).toBool();

        DocumentView::highlightLinks = m_settings.value("documentView/highlightLinks", true).toBool();
        DocumentView::externalLinks = m_settings.value("documentView/externalLinks", false).toBool();

        for(int index = 0; index < m_tabWidget->count(); index++)
        {
            DocumentView* documentView = qobject_cast< DocumentView* >(m_tabWidget->widget(index)); Q_ASSERT(documentView);

            documentView->refresh();
        }
    }
}

void MainWindow::slotPageLayoutGroupTriggered(QAction* action)
{
    DocumentView* documentView = qobject_cast< DocumentView* >(m_tabWidget->currentWidget()); Q_ASSERT(documentView);

    documentView->setPageLayout(static_cast< DocumentView::PageLayout >(action->data().toUInt()));
}

void MainWindow::slotScaleModeGroupTriggered(QAction* action)
{
    DocumentView* documentView = qobject_cast< DocumentView* >(m_tabWidget->currentWidget()); Q_ASSERT(documentView);

    documentView->setScaleMode(static_cast< DocumentView::ScaleMode >(action->data().toUInt()));
}

void MainWindow::slotZoomIn()
{
    DocumentView* documentView = qobject_cast< DocumentView* >(m_tabWidget->currentWidget()); Q_ASSERT(documentView);

    documentView->zoomIn();
}

void MainWindow::slotZoomOut()
{
    DocumentView* documentView = qobject_cast< DocumentView* >(m_tabWidget->currentWidget()); Q_ASSERT(documentView);

    documentView->zoomOut();
}

void MainWindow::slotRotateLeft()
{
    DocumentView* documentView = qobject_cast< DocumentView* >(m_tabWidget->currentWidget()); Q_ASSERT(documentView);

    documentView->rotateLeft();
}

void MainWindow::slotRotateRight()
{
    DocumentView* documentView = qobject_cast< DocumentView* >(m_tabWidget->currentWidget()); Q_ASSERT(documentView);

    documentView->rotateRight();
}

void MainWindow::slotFullscreen()
{
    if(m_fullscreenAction->isChecked())
    {
        m_geometry = saveGeometry();

        showFullScreen();
    }
    else
    {
        restoreGeometry(m_geometry);

        showNormal();

        restoreGeometry(m_geometry);
    }
}

void MainWindow::slotPresentation()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView* documentView = qobject_cast< DocumentView* >(m_tabWidget->currentWidget()); Q_ASSERT(documentView);
        PresentationView* presentationView = new PresentationView();

        if(presentationView->open(documentView->filePath()))
        {
            presentationView->setCurrentPage(documentView->currentPage());

            presentationView->show();
            presentationView->setAttribute(Qt::WA_DeleteOnClose);
        }
        else
        {
            delete presentationView;

            QMessageBox::warning(this, tr("Warning"), tr("Could not open document \"%1\".").arg(QFileInfo(documentView->filePath()).fileName()));
        }
    }
}

void MainWindow::slotPreviousTab()
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

void MainWindow::slotNextTab()
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

void MainWindow::slotCloseTab()
{
    delete m_tabWidget->currentWidget();
}

void MainWindow::slotCloseAllTabs()
{
    disconnect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(slotTabWidgetCurrentChanged(int)));

    while(m_tabWidget->count() > 0)
    {
        delete m_tabWidget->widget(0);
    }

    slotTabWidgetCurrentChanged(-1);

    connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(slotTabWidgetCurrentChanged(int)));
}

void MainWindow::slotCloseAllTabsButCurrentTab()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView* documentView = qobject_cast< DocumentView* >(m_tabWidget->currentWidget()); Q_ASSERT(documentView);

        m_tabWidget->removeTab(m_tabWidget->currentIndex());

        slotCloseAllTabs();

        int index = m_tabWidget->addTab(documentView, QFileInfo(documentView->filePath()).completeBaseName());
        m_tabWidget->setTabToolTip(index, QFileInfo(documentView->filePath()).completeBaseName());
        m_tabWidget->setCurrentIndex(index);
    }
}

void MainWindow::slotContents()
{
    HelpDialog helpDialog;

    helpDialog.exec();
}

void MainWindow::slotAbout()
{
    QMessageBox::about(this, tr("About qpdfview"), tr("<p><b>qpdfview %1</b></p><p>qpdfview is a tabbed PDF viewer using the poppler library.</p><p>&copy; 2012 Adam Reichold</p>").arg(QApplication::applicationVersion()));
}

void MainWindow::slotTabWidgetCurrentChanged(int index)
{
    if(index != -1)
    {
        DocumentView* documentView = qobject_cast< DocumentView* >(m_tabWidget->currentWidget()); Q_ASSERT(documentView);

        m_refreshAction->setEnabled(true);
        m_saveCopyAction->setEnabled(true);
        m_printAction->setEnabled(true);

        m_previousPageAction->setEnabled(true);
        m_nextPageAction->setEnabled(true);
        m_firstPageAction->setEnabled(true);
        m_lastPageAction->setEnabled(true);

        m_searchAction->setEnabled(true);
        m_findPreviousAction->setEnabled(true);
        m_findNextAction->setEnabled(true);
        m_cancelSearchAction->setEnabled(true);

        m_pageLayoutGroup->setEnabled(true);

        m_scaleModeGroup->setEnabled(true);

        m_zoomInAction->setEnabled(true);
        m_zoomOutAction->setEnabled(true);

        m_rotateLeftAction->setEnabled(true);
        m_rotateRightAction->setEnabled(true);

        m_presentationAction->setEnabled(true);

        m_previousTabAction->setEnabled(true);
        m_nextTabAction->setEnabled(true);
        m_closeTabAction->setEnabled(true);
        m_closeAllTabsAction->setEnabled(true);
        m_closeAllTabsButCurrentTabAction->setEnabled(true);

        m_editToolBar->setEnabled(true);
        m_viewToolBar->setEnabled(true);

        m_searchToolBar->setEnabled(true);

        if(m_searchToolBar->isVisible())
        {
            m_searchLineEdit->clear();
            m_searchTimer->stop();

            for(int index = 0; index < m_tabWidget->count(); index++)
            {
                DocumentView* documentView = qobject_cast< DocumentView* >(m_tabWidget->widget(index)); Q_ASSERT(documentView);

                documentView->cancelSearch();
            }
        }

        slotCurrentPageChanged(documentView->currentPage());
        slotNumberOfPagesChanged(documentView->numberOfPages());
        slotPageLayoutChanged(documentView->pageLayout());
        slotScaleModeChanged(documentView->scaleMode());
        slotScaleFactorChanged(documentView->scaleFactor());
        slotHighlightAllChanged(documentView->highlightAll());

        m_outlineDock->setWidget(documentView->outlineTreeWidget());
        m_metaInformationDock->setWidget(documentView->metaInformationTableWidget());
        m_thumbnailsDock->setWidget(documentView->thumbnailsGraphicsView());

        setWindowTitle(m_tabWidget->tabText(index) + " - qpdfview");
    }
    else
    {
        m_refreshAction->setEnabled(false);
        m_saveCopyAction->setEnabled(false);
        m_printAction->setEnabled(false);

        m_previousPageAction->setEnabled(false);
        m_nextPageAction->setEnabled(false);
        m_firstPageAction->setEnabled(false);
        m_lastPageAction->setEnabled(false);

        m_searchAction->setEnabled(false);
        m_findPreviousAction->setEnabled(false);
        m_findNextAction->setEnabled(false);
        m_cancelSearchAction->setEnabled(false);

        m_onePageAction->setChecked(true);
        m_pageLayoutGroup->setEnabled(false);

        m_doNotScaleAction->setChecked(true);
        m_scaleModeGroup->setEnabled(false);

        m_zoomInAction->setEnabled(false);
        m_zoomOutAction->setEnabled(false);

        m_rotateLeftAction->setEnabled(false);
        m_rotateRightAction->setEnabled(false);

        m_presentationAction->setEnabled(false);

        m_previousTabAction->setEnabled(false);
        m_nextTabAction->setEnabled(false);
        m_closeTabAction->setEnabled(false);
        m_closeAllTabsAction->setEnabled(false);
        m_closeAllTabsButCurrentTabAction->setEnabled(false);

        m_currentPageLineEdit->setText(QString());
        m_numberOfPagesLabel->setText(QString());
        m_editToolBar->setEnabled(false);

        m_scaleFactorComboBox->setCurrentIndex(2);
        m_viewToolBar->setEnabled(false);

        m_highlightAllCheckBox->setChecked(false);
        m_searchToolBar->setEnabled(false);

        if(m_searchToolBar->isVisible())
        {
            m_searchLineEdit->clear();
            m_searchTimer->stop();
            m_searchToolBar->hide();
        }

        m_outlineDock->setWidget(0);
        m_metaInformationDock->setWidget(0);
        m_thumbnailsDock->setWidget(0);

        setWindowTitle("qpdfview");
    }
}

void MainWindow::slotTabWidgetTabCloseRequested(int index)
{
    delete m_tabWidget->widget(index);
}

void MainWindow::slotCurrentPageLineEditReturnPressed()
{
    DocumentView* documentView = qobject_cast< DocumentView* >(m_tabWidget->currentWidget()); Q_ASSERT(documentView);

    documentView->setCurrentPage(m_currentPageLineEdit->text().toInt());
}

void MainWindow::slotScaleFactorComboBoxCurrentIndexChanged(int index)
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView* documentView = qobject_cast< DocumentView* >(m_tabWidget->currentWidget()); Q_ASSERT(documentView);

        documentView->setScaleMode(static_cast< DocumentView::ScaleMode >(m_scaleFactorComboBox->itemData(index).toUInt()));
    }
}

void MainWindow::slotScaleFactorComboBoxEditingFinished()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView* documentView = qobject_cast< DocumentView* >(m_tabWidget->currentWidget()); Q_ASSERT(documentView);

        QString text = m_scaleFactorComboBox->lineEdit()->text();

        text = text.trimmed();

        text = text.endsWith('%') ? text.left(text.size() - 1) : text;

        text = text.trimmed();

        bool ok = false;
        qreal scaleFactor = QLocale::system().toInt(text, &ok) / 100.0;

        if(ok && scaleFactor >= DocumentView::mininumScaleFactor && scaleFactor <= DocumentView::maximumScaleFactor)
        {
            documentView->setScaleFactor(scaleFactor);
            documentView->setScaleMode(DocumentView::ScaleFactor);
        }

        slotScaleFactorChanged(documentView->scaleFactor());
        slotScaleModeChanged(documentView->scaleMode());
    }
}

void MainWindow::slotHighlightAllCheckBoxClicked(bool checked)
{
    DocumentView* documentView = qobject_cast< DocumentView* >(m_tabWidget->currentWidget()); Q_ASSERT(documentView);

    documentView->setHighlightAll(checked);
}

void MainWindow::slotFilePathChanged(const QString& filePath)
{
    m_tabWidget->setTabText(m_tabWidget->currentIndex(), QFileInfo(filePath).completeBaseName());
    m_tabWidget->setTabToolTip(m_tabWidget->currentIndex(), QFileInfo(filePath).completeBaseName());

    setWindowTitle(QFileInfo(filePath).completeBaseName() + " - qpdfview");
}

void MainWindow::slotNumberOfPagesChanged(int numberOfPages)
{
    m_currentPageValidator->setRange(1, numberOfPages);
    m_numberOfPagesLabel->setText(tr(" of %1").arg(numberOfPages));
}

void MainWindow::slotCurrentPageChanged(int currentPage)
{
    m_currentPageLineEdit->setText(QLocale::system().toString(currentPage));
}

void MainWindow::slotPageLayoutChanged(DocumentView::PageLayout pageLayout)
{
    foreach(QAction* action, m_pageLayoutGroup->actions())
    {
        action->setChecked(action->data().toUInt() == static_cast< uint >(pageLayout));
    }
}

void MainWindow::slotScaleModeChanged(DocumentView::ScaleMode scaleMode)
{
    foreach(QAction* action, m_scaleModeGroup->actions())
    {
        action->setChecked(action->data().toUInt() == static_cast< uint >(scaleMode));
    }

    for(int index = 0; index < m_scaleFactorComboBox->count(); index++)
    {
        if(m_scaleFactorComboBox->itemData(index).toUInt() == static_cast< uint >(scaleMode))
        {
            m_scaleFactorComboBox->setCurrentIndex(index);
        }
    }
}

void MainWindow::slotScaleFactorChanged(qreal scaleFactor)
{
    m_scaleFactorComboBox->setItemText(3, tr("Scale to %1%").arg(100.0 * scaleFactor, 0, 'f', 0));
}

void MainWindow::slotHighlightAllChanged(bool highlightAll)
{
    m_highlightAllCheckBox->setChecked(highlightAll);
}

void MainWindow::createActions()
{
#ifdef DATA_INSTALL_PATH
    QString dataInstallPath(DATA_INSTALL_PATH);
#endif

    // open

    m_openAction = new QAction(tr("&Open..."), this);
    m_openAction->setShortcut(QKeySequence::Open);
    m_openAction->setIcon(QIcon::fromTheme("document-open"));
    m_openAction->setIconVisibleInMenu(true);
    connect(m_openAction, SIGNAL(triggered()), SLOT(slotOpen()));

    // open in new tab

    m_openInNewTabAction = new QAction(tr("Open in new &tab..."), this);
    m_openInNewTabAction->setShortcut(QKeySequence::AddTab);
    m_openInNewTabAction->setIconVisibleInMenu(true);
    connect(m_openInNewTabAction, SIGNAL(triggered()), SLOT(slotOpenInNewTab()));

    if(QIcon::hasThemeIcon("tab-new"))
    {
        m_openInNewTabAction->setIcon(QIcon::fromTheme("tab-new"));
    }
    else
    {
#ifdef DATA_INSTALL_PATH
        m_openInNewTabAction->setIcon(QIcon(dataInstallPath + "/tab-new.svg"));
#else
        m_openInNewTabAction->setIcon(QIcon(":/icons/tab-new.svg"));
#endif
    }

    // recently used

    m_recentlyUsedAction = new RecentlyUsedAction(this);
    m_recentlyUsedAction->setIcon(QIcon::fromTheme("document-open-recent"));
    m_recentlyUsedAction->setIconVisibleInMenu(true);
    connect(m_recentlyUsedAction, SIGNAL(entrySelected(QString)), SLOT(slotRecentyUsedActionEntrySelected(QString)));

    // refresh

    m_refreshAction = new QAction(tr("&Refresh"), this);
    m_refreshAction->setShortcut(QKeySequence::Refresh);
    m_refreshAction->setIconVisibleInMenu(true);
    connect(m_refreshAction, SIGNAL(triggered()), SLOT(slotRefresh()));

    if(QIcon::hasThemeIcon("view-refresh"))
    {
        m_refreshAction->setIcon(QIcon::fromTheme("view-refresh"));
    }
    else
    {
#ifdef DATA_INSTALL_PATH
        m_refreshAction->setIcon(QIcon(dataInstallPath + "/view-refresh.svg"));
#else
        m_refreshAction->setIcon(QIcon(":/icons/view-refresh.svg"));
#endif
    }

    // save copy

    m_saveCopyAction = new QAction(tr("&Save copy..."), this);
    m_saveCopyAction->setShortcut(QKeySequence::Save);
    m_saveCopyAction->setIcon(QIcon::fromTheme("document-save"));
    m_saveCopyAction->setIconVisibleInMenu(true);
    connect(m_saveCopyAction, SIGNAL(triggered()), SLOT(slotSaveCopy()));

    // print

    m_printAction = new QAction(tr("&Print..."), this);
    m_printAction->setShortcut(QKeySequence::Print);
    m_printAction->setIcon(QIcon::fromTheme("document-print"));
    m_printAction->setIconVisibleInMenu(true);
    connect(m_printAction, SIGNAL(triggered()), SLOT(slotPrint()));

    // exit

    m_exitAction = new QAction(tr("&Exit"), this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    m_exitAction->setIcon(QIcon::fromTheme("application-exit"));
    m_exitAction->setIconVisibleInMenu(true);
    connect(m_exitAction, SIGNAL(triggered()), SLOT(close()));

    // previous page

    m_previousPageAction = new QAction(tr("&Previous page"), this);
    m_previousPageAction->setShortcuts(QList< QKeySequence >() << QKeySequence(Qt::Key_Up) << QKeySequence(Qt::Key_Left) << QKeySequence(Qt::Key_Backspace));
    m_previousPageAction->setIconVisibleInMenu(true);
    connect(m_previousPageAction, SIGNAL(triggered()), SLOT(slotPreviousPage()));

    if(QIcon::hasThemeIcon("go-previous"))
    {
        m_previousPageAction->setIcon(QIcon::fromTheme("go-previous"));
    }
    else
    {
#ifdef DATA_INSTALL_PATH
        m_previousPageAction->setIcon(QIcon(dataInstallPath + "/go-previous.svg"));
#else
        m_previousPageAction->setIcon(QIcon(":/icons/go-previous.svg"));
#endif
    }

    // next page

    m_nextPageAction = new QAction(tr("&Next page"), this);
    m_nextPageAction->setShortcuts(QList< QKeySequence >() << QKeySequence(Qt::Key_Down) << QKeySequence(Qt::Key_Right) << QKeySequence(Qt::Key_Space));
    m_nextPageAction->setIconVisibleInMenu(true);
    connect(m_nextPageAction, SIGNAL(triggered()), SLOT(slotNextPage()));

    if(QIcon::hasThemeIcon("go-next"))
    {
        m_nextPageAction->setIcon(QIcon::fromTheme("go-next"));
    }
    else
    {
#ifdef DATA_INSTALL_PATH
        m_nextPageAction->setIcon(QIcon(dataInstallPath + "/go-next.svg"));
#else
        m_nextPageAction->setIcon(QIcon(":/icons/go-next.svg"));
#endif
    }

    // first page

    m_firstPageAction = new QAction(tr("&First page"), this);
    //m_firstPageAction->setShortcut(QKeySequence::MoveToStartOfDocument);
    m_firstPageAction->setShortcut(QKeySequence(Qt::Key_Home));
    m_firstPageAction->setIcon(QIcon::fromTheme("go-first"));
    m_firstPageAction->setIconVisibleInMenu(true);
    connect(m_firstPageAction, SIGNAL(triggered()), SLOT(slotFirstPage()));

    // last page

    m_lastPageAction = new QAction(tr("&Last page"), this);
    //m_lastPageAction->setShortcut(QKeySequence::MoveToEndOfDocument);
    m_lastPageAction->setShortcut(QKeySequence(Qt::Key_End));
    m_lastPageAction->setIcon(QIcon::fromTheme("go-last"));
    m_lastPageAction->setIconVisibleInMenu(true);
    connect(m_lastPageAction, SIGNAL(triggered()), SLOT(slotLastPage()));

    // search

    m_searchAction = new QAction(tr("&Search..."), this);
    m_searchAction->setShortcut(QKeySequence::Find);
    m_searchAction->setIcon(QIcon::fromTheme("edit-find"));
    m_searchAction->setIconVisibleInMenu(true);
    connect(m_searchAction, SIGNAL(triggered()), SLOT(slotSearch()));

    // find previous

    m_findPreviousAction = new QAction(tr("Find previous"), this);
    m_findPreviousAction->setShortcut(QKeySequence::FindPrevious);
    m_findPreviousAction->setIconVisibleInMenu(true);
    connect(m_findPreviousAction, SIGNAL(triggered()), SLOT(slotFindPrevious()));

    if(QIcon::hasThemeIcon("go-up"))
    {
        m_findPreviousAction->setIcon(QIcon::fromTheme("go-up"));
    }
    else
    {
#ifdef DATA_INSTALL_PATH
        m_findPreviousAction->setIcon(QIcon(dataInstallPath + "/go-up.svg"));
#else
        m_findPreviousAction->setIcon(QIcon(":/icons/go-up.svg"));
#endif
    }

    // find next

    m_findNextAction = new QAction(tr("Find next"), this);
    m_findNextAction->setShortcut(QKeySequence::FindNext);
    m_findNextAction->setIconVisibleInMenu(true);
    connect(m_findNextAction, SIGNAL(triggered()), SLOT(slotFindNext()));

    if(QIcon::hasThemeIcon("go-down"))
    {
        m_findNextAction->setIcon(QIcon::fromTheme("go-down"));
    }
    else
    {
#ifdef DATA_INSTALL_PATH
        m_findNextAction->setIcon(QIcon(dataInstallPath + "/go-down.svg"));
#else
        m_findNextAction->setIcon(QIcon(":/icons/go-down.svg"));
#endif
    }

    // cancel search

    m_cancelSearchAction = new QAction(tr("Cancel search"), this);
    m_cancelSearchAction->setShortcut(QKeySequence(Qt::Key_Escape));
    m_cancelSearchAction->setIconVisibleInMenu(true);
    connect(m_cancelSearchAction, SIGNAL(triggered()), SLOT(slotCancelSearch()));

    if(QIcon::hasThemeIcon("process-stop"))
    {
        m_cancelSearchAction->setIcon(QIcon::fromTheme("process-stop"));
    }
    else
    {
#ifdef DATA_INSTALL_PATH
        m_cancelSearchAction->setIcon(QIcon(dataInstallPath + "/process-stop.svg"));
#else
        m_cancelSearchAction->setIcon(QIcon(":/icons/process-stop.svg"));
#endif
    }

    // settings

    m_settingsAction = new QAction(tr("Settings..."), this);
    connect(m_settingsAction, SIGNAL(triggered()), SLOT(slotSettings()));

    // page layout

    m_onePageAction = new QAction(tr("One page"), this);
    m_onePageAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_1));
    m_onePageAction->setCheckable(true);
    m_onePageAction->setData(static_cast< uint >(DocumentView::OnePage));
    m_twoPagesAction = new QAction(tr("Two pages"), this);
    m_twoPagesAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_2));
    m_twoPagesAction->setCheckable(true);
    m_twoPagesAction->setData(static_cast< uint >(DocumentView::TwoPages));
    m_oneColumnAction = new QAction(tr("One column"), this);
    m_oneColumnAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_3));
    m_oneColumnAction->setCheckable(true);
    m_oneColumnAction->setData(static_cast< uint >(DocumentView::OneColumn));
    m_twoColumnsAction = new QAction(tr("Two columns"), this);
    m_twoColumnsAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_4));
    m_twoColumnsAction->setCheckable(true);
    m_twoColumnsAction->setData(static_cast< uint >(DocumentView::TwoColumns));

    m_pageLayoutGroup = new QActionGroup(this);
    m_pageLayoutGroup->addAction(m_onePageAction);
    m_pageLayoutGroup->addAction(m_twoPagesAction);
    m_pageLayoutGroup->addAction(m_oneColumnAction);
    m_pageLayoutGroup->addAction(m_twoColumnsAction);
    connect(m_pageLayoutGroup, SIGNAL(selected(QAction*)), SLOT(slotPageLayoutGroupTriggered(QAction*)));

    // scale mode

    m_fitToPageAction = new QAction(tr("Fit to page"), this);
    m_fitToPageAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_8));
    m_fitToPageAction->setIcon(QIcon::fromTheme("zoom-fit-best"));
    m_fitToPageAction->setCheckable(true);
    m_fitToPageAction->setData(static_cast< uint >(DocumentView::FitToPage));
    m_fitToPageWidthAction = new QAction(tr("Fit to page width"), this);
    m_fitToPageWidthAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_9));
    m_fitToPageWidthAction->setCheckable(true);
    m_fitToPageWidthAction->setData(static_cast< uint >(DocumentView::FitToPageWidth));
    m_doNotScaleAction = new QAction(tr("Do not scale"), this);
    m_doNotScaleAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_0));
    m_doNotScaleAction->setIcon(QIcon::fromTheme("zoom-original"));
    m_doNotScaleAction->setCheckable(true);
    m_doNotScaleAction->setData(static_cast< uint >(DocumentView::DoNotScale));

    m_scaleModeGroup = new QActionGroup(this);
    m_scaleModeGroup->addAction(m_fitToPageAction);
    m_scaleModeGroup->addAction(m_fitToPageWidthAction);
    m_scaleModeGroup->addAction(m_doNotScaleAction);
    connect(m_scaleModeGroup, SIGNAL(selected(QAction*)), SLOT(slotScaleModeGroupTriggered(QAction*)));

    // zoom

    m_zoomInAction = new QAction(tr("Zoom &in"), this);
    m_zoomInAction->setShortcut(QKeySequence::ZoomIn);
    m_zoomInAction->setIconVisibleInMenu(true);
    connect(m_zoomInAction, SIGNAL(triggered()), SLOT(slotZoomIn()));

    if(QIcon::hasThemeIcon("zoom-in"))
    {
        m_zoomInAction->setIcon(QIcon::fromTheme("zoom-in"));
    }
    else
    {
#ifdef DATA_INSTALL_PATH
        m_zoomInAction->setIcon(QIcon(dataInstallPath + "/zoom-in.svg"));
#else
        m_zoomInAction->setIcon(QIcon(":/icons/zoom-in.svg"));
#endif
    }

    m_zoomOutAction = new QAction(tr("Zoom &out"), this);
    m_zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    m_zoomOutAction->setIconVisibleInMenu(true);
    connect(m_zoomOutAction, SIGNAL(triggered()), SLOT(slotZoomOut()));

    if(QIcon::hasThemeIcon("zoom-out"))
    {
        m_zoomOutAction->setIcon(QIcon::fromTheme("zoom-out"));
    }
    else
    {
#ifdef DATA_INSTALL_PATH
        m_zoomOutAction->setIcon(QIcon(dataInstallPath + "/zoom-out.svg"));
#else
        m_zoomOutAction->setIcon(QIcon(":/icons/zoom-out.svg"));
#endif
    }

    // rotate

    m_rotateLeftAction = new QAction(tr("Rotate &left"), this);
    m_rotateLeftAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Left));
    m_rotateLeftAction->setIcon(QIcon::fromTheme("object-rotate-left"));
    m_rotateLeftAction->setIconVisibleInMenu(true);
    connect(m_rotateLeftAction, SIGNAL(triggered()), SLOT(slotRotateLeft()));

    m_rotateRightAction = new QAction(tr("Rotate &right"), this);
    m_rotateRightAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Right));
    m_rotateRightAction->setIcon(QIcon::fromTheme("object-rotate-right"));
    m_rotateRightAction->setIconVisibleInMenu(true);
    connect(m_rotateRightAction, SIGNAL(triggered()), SLOT(slotRotateRight()));

    // fullscreen

    m_fullscreenAction = new QAction(tr("&Fullscreen"), this);
    m_fullscreenAction->setCheckable(true);
    m_fullscreenAction->setShortcut(QKeySequence(Qt::Key_F11));
    m_fullscreenAction->setIcon(QIcon::fromTheme("view-fullscreen"));
    m_fullscreenAction->setIconVisibleInMenu(true);
    connect(m_fullscreenAction, SIGNAL(triggered()), SLOT(slotFullscreen()));

    // presentation

    m_presentationAction = new QAction(tr("&Presentation..."), this);
    m_presentationAction->setShortcut(QKeySequence(Qt::Key_F12));
    m_presentationAction->setIcon(QIcon::fromTheme("x-office-presentation"));
    m_presentationAction->setIconVisibleInMenu(true);
    connect(m_presentationAction, SIGNAL(triggered()), SLOT(slotPresentation()));

    // previous tab

    m_previousTabAction = new QAction(tr("&Previous tab"), this);
    m_previousTabAction->setShortcut(QKeySequence::PreviousChild);
    connect(m_previousTabAction, SIGNAL(triggered()), SLOT(slotPreviousTab()));

    // next tab

    m_nextTabAction = new QAction(tr("&Next tab"), this);
    m_nextTabAction->setShortcut(QKeySequence::NextChild);
    connect(m_nextTabAction, SIGNAL(triggered()), SLOT(slotNextTab()));

    // close tab

    m_closeTabAction = new QAction(tr("&Close tab"), this);
    m_closeTabAction->setShortcut(QKeySequence::Close);
    m_closeTabAction->setIcon(QIcon::fromTheme("window-close"));
    m_closeTabAction->setIconVisibleInMenu(true);
    connect(m_closeTabAction, SIGNAL(triggered()), SLOT(slotCloseTab()));

    // close all tabs

    m_closeAllTabsAction = new QAction(tr("Close all &tabs"), this);
    m_closeAllTabsAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_W));
    connect(m_closeAllTabsAction, SIGNAL(triggered()), SLOT(slotCloseAllTabs()));

    // close all tabs but current tab

    m_closeAllTabsButCurrentTabAction = new QAction(tr("Close all tabs &but current tab"), this);
    m_closeAllTabsButCurrentTabAction->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_W));
    connect(m_closeAllTabsButCurrentTabAction, SIGNAL(triggered()), SLOT(slotCloseAllTabsButCurrentTab()));

    // contents

    m_contentsAction = new QAction(tr("&Contents"), this);
    m_contentsAction->setShortcut(QKeySequence::HelpContents);
    m_contentsAction->setIcon(QIcon::fromTheme("help-contents"));
    m_contentsAction->setIconVisibleInMenu(true);
    connect(m_contentsAction, SIGNAL(triggered()), SLOT(slotContents()));

    // about

    m_aboutAction = new QAction(tr("&About"), this);
    m_aboutAction->setIcon(QIcon::fromTheme("help-about"));
    m_aboutAction->setIconVisibleInMenu(true);
    connect(m_aboutAction, SIGNAL(triggered()), SLOT(slotAbout()));
}

void MainWindow::createWidgets()
{
    // tab

    m_tabWidget = new TabWidget(this);

    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setElideMode(Qt::ElideRight);

    setCentralWidget(m_tabWidget);

    connect(m_tabWidget, SIGNAL(currentChanged(int)), SLOT(slotTabWidgetCurrentChanged(int)));
    connect(m_tabWidget, SIGNAL(tabCloseRequested(int)), SLOT(slotTabWidgetTabCloseRequested(int)));

    // current page

    m_currentPageLineEdit = new LineEdit(this);
    m_currentPageValidator = new QIntValidator(m_currentPageLineEdit);

    m_currentPageLineEdit->setValidator(m_currentPageValidator);
    m_currentPageLineEdit->setAlignment(Qt::AlignCenter);
    m_currentPageLineEdit->setFixedWidth(40);

    connect(m_currentPageLineEdit, SIGNAL(returnPressed()), SLOT(slotCurrentPageLineEditReturnPressed()));

    // number of pages

    m_numberOfPagesLabel = new QLabel(this);

    m_numberOfPagesLabel->setAlignment(Qt::AlignCenter);
    m_numberOfPagesLabel->setFixedWidth(60);

    // scale factor

    m_scaleFactorComboBox = new ComboBox(this);

    m_scaleFactorComboBox->setEditable(true);
    m_scaleFactorComboBox->setInsertPolicy(QComboBox::NoInsert);

    m_scaleFactorComboBox->addItem(tr("Fit to page"), static_cast< uint >(DocumentView::FitToPage));
    m_scaleFactorComboBox->addItem(tr("Fit to page width"), static_cast< uint >(DocumentView::FitToPageWidth));
    m_scaleFactorComboBox->addItem(tr("Do not scale"), static_cast< uint >(DocumentView::DoNotScale));
    m_scaleFactorComboBox->addItem(QString(), static_cast< uint >(DocumentView::ScaleFactor));

    connect(m_scaleFactorComboBox, SIGNAL(currentIndexChanged(int)), SLOT(slotScaleFactorComboBoxCurrentIndexChanged(int)));
    connect(m_scaleFactorComboBox->lineEdit(), SIGNAL(editingFinished()), SLOT(slotScaleFactorComboBoxEditingFinished()));

    // search

    m_searchWidget = new QWidget(this);
    m_searchLineEdit = new QLineEdit(m_searchWidget);
    m_searchTimer = new QTimer(this);
    m_matchCaseCheckBox = new QCheckBox(tr("Match &case"), m_searchWidget);
    m_highlightAllCheckBox = new QCheckBox(tr("Highlight &all"), m_searchWidget);
    m_searchTimer->setInterval(2000);
    m_searchTimer->setSingleShot(true);

    m_searchWidget->setLayout(new QHBoxLayout());
    m_searchWidget->layout()->setContentsMargins(5, 0, 5, 0);
    m_searchWidget->layout()->addWidget(m_searchLineEdit);
    m_searchWidget->layout()->addWidget(m_matchCaseCheckBox);
    m_searchWidget->layout()->addWidget(m_highlightAllCheckBox);

    connect(m_searchLineEdit, SIGNAL(textEdited(QString)), m_searchTimer, SLOT(start()));
    connect(m_searchLineEdit, SIGNAL(returnPressed()), this, SLOT(slotStartSearch()));
    connect(m_searchTimer, SIGNAL(timeout()), this, SLOT(slotStartSearch()));

    connect(m_highlightAllCheckBox, SIGNAL(clicked(bool)), SLOT(slotHighlightAllCheckBoxClicked(bool)));
}

void MainWindow::createToolBars()
{
    // file

    m_fileToolBar = new QToolBar(tr("&File"));
    m_fileToolBar->setObjectName("fileToolBar");

    QStringList fileToolBar;
    fileToolBar << "openInNewTab" << "refresh";

    fileToolBar = m_settings.value("mainWindow/fileToolBar", fileToolBar).toStringList();

    foreach(QString entry, fileToolBar)
    {
        if(entry == "open") { m_fileToolBar->addAction(m_openAction); }
        else if(entry == "openInNewTab") { m_fileToolBar->addAction(m_openInNewTabAction); }
        else if(entry == "refresh") { m_fileToolBar->addAction(m_refreshAction); }
        else if(entry == "saveCopy") { m_fileToolBar->addAction(m_saveCopyAction); }
        else if(entry == "print") { m_fileToolBar->addAction(m_printAction); }
    }

    addToolBar(Qt::TopToolBarArea, m_fileToolBar);

    // edit

    m_editToolBar = new QToolBar(tr("&Edit"));
    m_editToolBar->setObjectName("editToolBar");

    QStringList editToolBar;
    editToolBar << "currentPage" << "numberOfPages" << "previousPage" << "nextPage";

    editToolBar = m_settings.value("mainWindow/editToolBar", editToolBar).toStringList();

    foreach(QString entry, editToolBar)
    {
        if(entry == "currentPage") { m_editToolBar->addWidget(m_currentPageLineEdit); }
        else if(entry == "numberOfPages") { m_editToolBar->addWidget(m_numberOfPagesLabel); }
        else if(entry == "previousPage") { m_editToolBar->addAction(m_previousPageAction); }
        else if(entry == "nextPage") { m_editToolBar->addAction(m_nextPageAction); }
        else if(entry == "firstPage") { m_editToolBar->addAction(m_firstPageAction); }
        else if(entry == "lastPage") { m_editToolBar->addAction(m_lastPageAction); }
        else if(entry == "search") { m_editToolBar->addAction(m_searchAction); }
    }

    addToolBar(Qt::TopToolBarArea, m_editToolBar);

    // view

    m_viewToolBar = new QToolBar(tr("&View"));
    m_viewToolBar->setObjectName("viewToolBar");

    m_viewToolBar->setHidden(true);

    QStringList viewToolBar;
    viewToolBar << "scaleFactor" << "zoomIn" << "zoomOut";

    viewToolBar = m_settings.value("mainWindow/viewToolBar", viewToolBar).toStringList();

    foreach(QString entry, viewToolBar)
    {
        if(entry == "scaleFactor") { m_viewToolBar->addWidget(m_scaleFactorComboBox); }
        else if(entry == "fitToPage") { m_viewToolBar->addAction(m_fitToPageAction); }
        else if(entry == "doNotScale") { m_viewToolBar->addAction(m_doNotScaleAction); }
        else if(entry == "zoomIn") { m_viewToolBar->addAction(m_zoomInAction); }
        else if(entry == "zoomOut") { m_viewToolBar->addAction(m_zoomOutAction); }
        else if(entry == "rotateLeft") { m_viewToolBar->addAction(m_rotateLeftAction); }
        else if(entry == "rotateRight") { m_viewToolBar->addAction(m_rotateRightAction); }
        else if(entry == "fullscreen") { m_viewToolBar->addAction(m_fullscreenAction); }
        else if(entry == "presentation") { m_viewToolBar->addAction(m_presentationAction); }
    }

    addToolBar(Qt::TopToolBarArea, m_viewToolBar);

    // search

    m_searchToolBar = new QToolBar(tr("&Search"));
    m_searchToolBar->setObjectName("searchToolBar");

    m_searchToolBar->setHidden(true);
    m_searchToolBar->setMovable(false);

    m_searchToolBar->addWidget(m_searchWidget);
    m_searchToolBar->addAction(m_findPreviousAction);
    m_searchToolBar->addAction(m_findNextAction);
    m_searchToolBar->addAction(m_cancelSearchAction);

    addToolBar(Qt::BottomToolBarArea, m_searchToolBar);
}

void MainWindow::createDocks()
{
    // outline

    m_outlineDock = new QDockWidget(tr("&Outline"), this);
    m_outlineDock->setObjectName("outlineDock");
    m_outlineDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_outlineDock->setFeatures(QDockWidget::AllDockWidgetFeatures);

    addDockWidget(Qt::LeftDockWidgetArea, m_outlineDock);
    m_outlineDock->hide();

    // meta-information

    m_metaInformationDock = new QDockWidget(tr("&Meta-information"), this);
    m_metaInformationDock->setObjectName("metaInformationDock");
    m_metaInformationDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_metaInformationDock->setFeatures(QDockWidget::AllDockWidgetFeatures);

    addDockWidget(Qt::LeftDockWidgetArea, m_metaInformationDock);
    m_metaInformationDock->hide();

    // thumbnails

    m_thumbnailsDock = new QDockWidget(tr("&Thumbnails"), this);
    m_thumbnailsDock->setObjectName("thumbnailsDock");
    m_thumbnailsDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_thumbnailsDock->setFeatures(QDockWidget::AllDockWidgetFeatures);

    addDockWidget(Qt::RightDockWidgetArea, m_thumbnailsDock);
    m_thumbnailsDock->hide();
}

void MainWindow::createMenus()
{
    // file

    m_fileMenu = menuBar()->addMenu(tr("&File"));
    m_fileMenu->addAction(m_openAction);
    m_fileMenu->addAction(m_openInNewTabAction);
    m_fileMenu->addAction(m_recentlyUsedAction);
    m_fileMenu->addAction(m_refreshAction);
    m_fileMenu->addAction(m_saveCopyAction);
    m_fileMenu->addAction(m_printAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_exitAction);

    // edit

    m_editMenu = menuBar()->addMenu(tr("&Edit"));
    m_editMenu->addAction(m_previousPageAction);
    m_editMenu->addAction(m_nextPageAction);
    m_editMenu->addAction(m_firstPageAction);
    m_editMenu->addAction(m_lastPageAction);
    m_editMenu->addSeparator();
    m_editMenu->addAction(m_searchAction);
    m_editMenu->addAction(m_findPreviousAction);
    m_editMenu->addAction(m_findNextAction);
    m_editMenu->addAction(m_cancelSearchAction);
    m_editMenu->addSeparator();
    m_editMenu->addAction(m_settingsAction);

    // view

    m_viewMenu = menuBar()->addMenu(tr("&View"));
    m_viewMenu->addAction(m_onePageAction);
    m_viewMenu->addAction(m_twoPagesAction);
    m_viewMenu->addAction(m_oneColumnAction);
    m_viewMenu->addAction(m_twoColumnsAction);
    m_viewMenu->addSeparator();
    m_viewMenu->addAction(m_fitToPageAction);
    m_viewMenu->addAction(m_fitToPageWidthAction);
    m_viewMenu->addAction(m_doNotScaleAction);
    m_viewMenu->addAction(m_zoomInAction);
    m_viewMenu->addAction(m_zoomOutAction);
    m_viewMenu->addSeparator();
    m_viewMenu->addAction(m_rotateLeftAction);
    m_viewMenu->addAction(m_rotateRightAction);
    m_viewMenu->addSeparator();

    // toolbars

    QMenu *toolbarsMenu = m_viewMenu->addMenu(tr("&Toolbars"));
    toolbarsMenu->addAction(m_fileToolBar->toggleViewAction());
    toolbarsMenu->addAction(m_editToolBar->toggleViewAction());
    toolbarsMenu->addAction(m_viewToolBar->toggleViewAction());

    // docks

    QMenu *docksMenu = m_viewMenu->addMenu(tr("&Docks"));
    docksMenu->addAction(m_outlineDock->toggleViewAction());
    docksMenu->addAction(m_metaInformationDock->toggleViewAction());
    docksMenu->addAction(m_thumbnailsDock->toggleViewAction());

    m_viewMenu->addAction(m_fullscreenAction);
    m_viewMenu->addAction(m_presentationAction);

    // tab

    m_tabMenu = menuBar()->addMenu(tr("&Tab"));
    m_tabMenu->addAction(m_previousTabAction);
    m_tabMenu->addAction(m_nextTabAction);
    m_tabMenu->addSeparator();
    m_tabMenu->addAction(m_closeTabAction);
    m_tabMenu->addAction(m_closeAllTabsAction);
    m_tabMenu->addAction(m_closeAllTabsButCurrentTabAction);
    m_tabMenu->addSeparator();

    // help

    m_helpMenu = menuBar()->addMenu(tr("&Help"));
    m_helpMenu->addAction(m_contentsAction);
    m_helpMenu->addAction(m_aboutAction);
}

bool MainWindowAdaptor::open(const QString& filePath, int page, qreal top)
{
    MainWindow* mainWindow = qobject_cast< MainWindow* >(parent()); Q_ASSERT(mainWindow);

    return mainWindow->open(filePath, page, top);
}

bool MainWindowAdaptor::openInNewTab(const QString& filePath, int page, qreal top)
{
    MainWindow* mainWindow = qobject_cast< MainWindow* >(parent()); Q_ASSERT(mainWindow);

    return mainWindow->openInNewTab(filePath, page, top);
}

void MainWindowAdaptor::refresh(const QString& filePath, int page, qreal top)
{
    MainWindow* mainWindow = qobject_cast< MainWindow* >(parent()); Q_ASSERT(mainWindow);

    bool openInNewTab = true;

    for(int index = 0; index < mainWindow->m_tabWidget->count(); index++)
    {
        DocumentView* documentView = qobject_cast< DocumentView* >(mainWindow->m_tabWidget->widget(index)); Q_ASSERT(documentView);

        if(QFileInfo(documentView->filePath()).absoluteFilePath() == QFileInfo(filePath).absoluteFilePath())
        {
            documentView->refresh();
            documentView->setCurrentPage(page, top);

            mainWindow->m_tabWidget->setCurrentIndex(index);

            openInNewTab = false;
        }
    }

    if(openInNewTab && QFileInfo(filePath).exists())
    {
        mainWindow->openInNewTab(filePath, page, top);
    }
}
