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

#include "documentview.h"
#include "miscellaneous.h"
#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
    m_settings(),
    m_geometry()
{
    this->createActions();
    this->createWidgets();
    this->createToolBars();
    this->createDocks();
    this->createMenus();

    this->setCentralWidget(m_tabWidget);
    this->setAcceptDrops(true);

    this->slotTabWidgetCurrentChanged(-1);

    // settings

    m_matchCaseCheckBox->setChecked(m_settings.value("mainWindow/matchCase", true).toBool());

    this->restoreGeometry(m_settings.value("mainWindow/geometry").toByteArray());
    this->restoreState(m_settings.value("mainWindow/state").toByteArray());

    // command line arguments

    QStringList arguments = QCoreApplication::arguments();
    arguments.removeFirst();

    foreach(QString argument, arguments)
    {
        if(QFileInfo(argument).exists()) {
            this->addTab(argument);
        }
    }
}

QMenu *MainWindow::createPopupMenu()
{
    QMenu *menu = new QMenu();

    menu->addAction(m_fileToolBar->toggleViewAction());
    menu->addAction(m_editToolBar->toggleViewAction());
    menu->addAction(m_viewToolBar->toggleViewAction());
    menu->addSeparator();
    menu->addAction(m_outlineDock->toggleViewAction());
    menu->addAction(m_thumbnailsDock->toggleViewAction());

    return menu;
}

bool MainWindow::open(const QString &filePath, int page, qreal top)
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

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
        return this->addTab(filePath, page, top);
    }
}

bool MainWindow::addTab(const QString &filePath, int page, qreal top)
{
    DocumentView *documentView = new DocumentView();

    if(documentView->open(filePath))
    {
        int index = m_tabWidget->addTab(documentView, QFileInfo(filePath).completeBaseName());
        m_tabWidget->setTabToolTip(index, QFileInfo(filePath).completeBaseName());
        m_tabWidget->setCurrentIndex(index);

        m_tabMenu->addAction(documentView->makeCurrentTabAction());

        connect(documentView, SIGNAL(searchProgressed(int)), this, SLOT(slotSearchProgressed(int)));
        connect(documentView, SIGNAL(searchCanceled()), this, SLOT(slotSearchCanceled()));
        connect(documentView, SIGNAL(searchFinished()), this, SLOT(slotSearchFinished()));

        connect(documentView, SIGNAL(numberOfPagesChanged(int)), this, SLOT(slotNumberOfPagesChanged(int)));
        connect(documentView, SIGNAL(currentPageChanged(int)), this, SLOT(slotCurrentPageChanged(int)));
        connect(documentView, SIGNAL(pageLayoutChanged(PageLayout)), this, SLOT(slotPageLayoutChanged(PageLayout)));
        connect(documentView, SIGNAL(scalingChanged(Scaling)), this, SLOT(slotScalingChanged(Scaling)));
        connect(documentView, SIGNAL(rotationChanged(Rotation)), this, SLOT(slotRotationChanged(Rotation)));
        connect(documentView, SIGNAL(highlightAllChanged(bool)), this, SLOT(slotHighlightAllChanged(bool)));

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

void MainWindow::closeTab(int index)
{
    if(index >= 0 && index < m_tabWidget->count())
    {
        QWidget *widget = m_tabWidget->widget(index);
        m_tabWidget->removeTab(index);

        delete widget;
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Escape)
    {
        if(m_searchToolBar->isVisible())
        {
            this->slotCancelSearch();

            m_searchLineEdit->clear();

            m_searchToolBar->hide();
        }
    }
    else
    {
        if(m_tabWidget->currentIndex() != -1)
        {
            QApplication::sendEvent(m_tabWidget->currentWidget(), event);
        }
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if(event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    if(event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();

        foreach(QUrl url, event->mimeData()->urls())
        {
            if(url.scheme() == "file" && QFileInfo(url.path()).exists())
            {
                this->addTab(url.path());
            }
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    this->slotCloseAllTabs();

    m_searchToolBar->hide();

    m_settings.setValue("mainWindow/matchCase", m_matchCaseCheckBox->isChecked());

    if(m_fullscreenAction->isChecked())
    {
        m_settings.setValue("mainWindow/geometry", m_geometry);
    }
    else
    {
        m_settings.setValue("mainWindow/geometry", this->saveGeometry());
    }

    m_settings.setValue("mainWindow/state", this->saveState());

    QMainWindow::closeEvent(event);
}

void MainWindow::slotOpen()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        QString filePath = QFileDialog::getOpenFileName(this, tr("Open document"),
                                                        m_settings.value("mainWindow/path", QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation)).toString(),
                                                        tr("Portable Document Format (*.pdf)"));

        if(!filePath.isEmpty())
        {
            if(this->open(filePath))
            {
                m_recentlyUsedAction->addEntry(filePath);

                m_settings.setValue("mainWindow/path", QFileInfo(filePath).path());
            }
        }
    }
    else
    {
        this->slotAddTab();
    }
}

void MainWindow::slotRefresh()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

        documentView->refresh();
    }
}

void MainWindow::slotSaveCopy()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        QString filePath = QFileDialog::getSaveFileName(this, tr("Save copy"),
                                                        m_settings.value("mainWindow/path", QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation)).toString(),
                                                        tr("Portable Document Format (*.pdf)"));

        if(!filePath.isEmpty())
        {
            DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

            if(documentView->saveCopy(filePath))
            {
                m_settings.setValue("mainWindow/path", QFileInfo(filePath).path());
            }
            else
            {
                QMessageBox::warning(this, tr("Warning"), tr("Could not save copy at \"%1\".").arg(QFileInfo(filePath).fileName()));
            }
        }
    }
}

void MainWindow::slotPrint()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

        QPrinter *printer = new QPrinter();
        printer->setFullPage(true);

        QPrintDialog printDialog(printer, this);
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
}

void MainWindow::slotPreviousPage()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

        documentView->previousPage();
    }
}

void MainWindow::slotNextPage()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

        documentView->nextPage();
    }
}

void MainWindow::slotFirstPage()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

        documentView->firstPage();
    }
}

void MainWindow::slotLastPage()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

        documentView->lastPage();
    }
}

void MainWindow::slotSearch()
{
    if(m_tabWidget->currentIndex() != -1)
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
}

void MainWindow::slotStartSearch()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        this->slotCancelSearch();

        if(m_searchToolBar->isVisible() && !m_searchLineEdit->text().isEmpty())
        {
            DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

            documentView->startSearch(m_searchLineEdit->text(), m_matchCaseCheckBox->isChecked());
        }
    }
}

void MainWindow::slotCancelSearch()
{
    m_searchTimer->stop();

    for(int index = 0; index < m_tabWidget->count(); index++)
    {
        DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->widget(index));

        documentView->cancelSearch();
    }
}

void MainWindow::slotSearchProgressed(int value)
{
    this->statusBar()->show();
    this->statusBar()->showMessage(tr("Searched %1% of the the current document...").arg(value));
}

void MainWindow::slotSearchCanceled()
{
    this->statusBar()->clearMessage();
    this->statusBar()->hide();
}

void MainWindow::slotSearchFinished()
{
    this->statusBar()->clearMessage();
    this->statusBar()->hide();
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
                DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

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
                DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

                documentView->findNext();
            }
        }
    }
}

void MainWindow::slotSettings()
{
    SettingsDialog settingsDialog;

    settingsDialog.exec();
}

void MainWindow::slotPresentation()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

        PresentationView *presentationView = new PresentationView();
        presentationView->setDocumentView(documentView);

        presentationView->show();

        presentationView->setAttribute(Qt::WA_DeleteOnClose);
    }
}

void MainWindow::slotFullscreen()
{
    if(m_fullscreenAction->isChecked())
    {
        m_geometry = this->saveGeometry();

        this->showFullScreen();
    }
    else
    {
        this->restoreGeometry(m_geometry);

        this->showNormal();

        this->restoreGeometry(m_geometry);
    }
}

void MainWindow::slotAddTab()
{
    QStringList filePaths = QFileDialog::getOpenFileNames(this, tr("Open documents"),
                                                          m_settings.value("mainWindow/path", QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation)).toString(),
                                                          tr("Portable Document Format (*.pdf)"));

    foreach(QString filePath, filePaths)
    {
        if(!filePath.isEmpty())
        {
            if(this->addTab(filePath))
            {
                m_settings.setValue("mainWindow/path", QFileInfo(filePath).path());

                m_recentlyUsedAction->addEntry(filePath);
            }
        }
    }
}

void MainWindow::slotPreviousTab()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        if(m_tabWidget->currentIndex() > 0)
        {
            m_tabWidget->setCurrentIndex(m_tabWidget->currentIndex()-1);
        }
        else
        {
            m_tabWidget->setCurrentIndex(m_tabWidget->count()-1);
        }
    }
}

void MainWindow::slotNextTab()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        if(m_tabWidget->currentIndex() < m_tabWidget->count()-1)
        {
            m_tabWidget->setCurrentIndex(m_tabWidget->currentIndex()+1);
        }
        else
        {
            m_tabWidget->setCurrentIndex(0);
        }
    }
}

void MainWindow::slotCloseTab()
{
    this->closeTab(m_tabWidget->currentIndex());
}

void MainWindow::slotCloseAllTabs()
{
    while(m_tabWidget->count() > 0)
    {
        this->closeTab(0);
    }
}

void MainWindow::slotCloseAllTabsButCurrentTab()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

        m_tabWidget->removeTab(m_tabWidget->currentIndex());

        this->slotCloseAllTabs();

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
    QMessageBox::about(this, tr("About qpdfview"), tr("<p><b>qpdfview</b></p><p>qpdfview is a tabbed PDF viewer using the poppler library.</p><p>&copy; 2012 Adam Reichold</p>"));
}

void MainWindow::slotTabWidgetCurrentChanged(int index)
{
    if(index != -1)
    {
        DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

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

        m_pageLayoutGroup->setEnabled(true);
        m_scalingGroup->setEnabled(true);
        m_rotationGroup->setEnabled(true);

        m_presentationAction->setEnabled(true);

        m_previousTabAction->setEnabled(true);
        m_nextTabAction->setEnabled(true);
        m_closeTabAction->setEnabled(true);

        m_editToolBar->setEnabled(true);
        m_viewToolBar->setEnabled(true);

        m_searchToolBar->setEnabled(true);

        this->slotCurrentPageChanged(documentView->currentPage());
        this->slotNumberOfPagesChanged(documentView->numberOfPages());
        this->slotPageLayoutChanged(documentView->pageLayout());
        this->slotScalingChanged(documentView->scaling());
        this->slotRotationChanged(documentView->rotation());
        this->slotHighlightAllChanged(documentView->highlightAll());

        if(m_searchToolBar->isVisible())
        {
            this->slotCancelSearch();

            m_searchLineEdit->clear();
        }

        m_outlineView->setDocumentView(documentView);
        m_thumbnailsView->setDocumentView(documentView);
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

        m_onePageAction->setChecked(true);
        m_pageLayoutGroup->setEnabled(false);

        m_scaleTo100Action->setChecked(true);
        m_scalingGroup->setEnabled(false);

        m_rotateBy0Action->setChecked(true);
        m_rotationGroup->setEnabled(false);

        m_presentationAction->setEnabled(false);

        m_previousTabAction->setEnabled(false);
        m_nextTabAction->setEnabled(false);
        m_closeTabAction->setEnabled(false);

        m_currentPageLineEdit->setText(QString());
        m_numberOfPagesLabel->setText(QString());
        m_editToolBar->setEnabled(false);

        m_pageLayoutComboBox->setCurrentIndex(0);
        m_scalingComboBox->setCurrentIndex(4);
        m_rotationComboBox->setCurrentIndex(0);
        m_viewToolBar->setEnabled(false);

        m_highlightAllCheckBox->setChecked(false);
        m_searchToolBar->setEnabled(false);

        if(m_searchToolBar->isVisible())
        {
            m_searchLineEdit->clear();

            m_searchToolBar->hide();
        }

        m_outlineView->setDocumentView(0);
        m_thumbnailsView->setDocumentView(0);
    }
}

void MainWindow::slotTabWidgetTabCloseRequested(int index)
{
    delete m_tabWidget->widget(index);
}

void MainWindow::slotCurrentPageLineEditReturnPressed()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

        documentView->setCurrentPage(m_currentPageLineEdit->text().toInt());
    }
}

void MainWindow::slotHighlightAllCheckBoxToggled()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

        documentView->setHighlightAll(m_highlightAllCheckBox->isChecked());
    }
}

void MainWindow::slotRecentyUsedEntrySelected(const QString &filePath)
{
    this->open(filePath);
}

void MainWindow::slotPageLayoutTriggered(QAction *action)
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

        documentView->setPageLayout(static_cast<PageLayout>(action->data().toUInt()));
    }
}

void MainWindow::slotPageLayoutCurrentIndexChanged(int index)
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

        documentView->setPageLayout(static_cast<PageLayout>(m_pageLayoutComboBox->itemData(index).toUInt()));
    }
}

void MainWindow::slotScalingTriggered(QAction *action)
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

        documentView->setScaling(static_cast<Scaling>(action->data().toUInt()));
    }
}

void MainWindow::slotScalingCurrentIndexChanged(int index)
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

        documentView->setScaling(static_cast<Scaling>(m_scalingComboBox->itemData(index).toUInt()));
    }
}

void MainWindow::slotRotationTriggered(QAction *action)
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

        documentView->setRotation(static_cast<Rotation>(action->data().toUInt()));
    }
}

void MainWindow::slotRotationCurrentIndexChanged(int index)
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

        documentView->setRotation(static_cast<Rotation>(m_rotationComboBox->itemData(index).toUInt()));
    }
}

void MainWindow::slotNumberOfPagesChanged(int numberOfPages)
{
    m_currentPageValidator->setRange(1, numberOfPages);
    m_numberOfPagesLabel->setText(tr(" of %1").arg(numberOfPages));
}

void MainWindow::slotCurrentPageChanged(int currentPage)
{
    m_currentPageLineEdit->setText(tr("%1").arg(currentPage));
}

void MainWindow::slotPageLayoutChanged(PageLayout pageLayout)
{
    switch(pageLayout)
    {
    case OnePage:
        m_onePageAction->setChecked(true);
        m_pageLayoutComboBox->setCurrentIndex(0);
        break;
    case TwoPages:
        m_twoPagesAction->setChecked(true);
        m_pageLayoutComboBox->setCurrentIndex(1);
        break;
    case OneColumn:
        m_oneColumnAction->setChecked(true);
        m_pageLayoutComboBox->setCurrentIndex(2);
        break;
    case TwoColumns:
        m_twoColumnsAction->setChecked(true);
        m_pageLayoutComboBox->setCurrentIndex(3);
        break;
    }
}

void MainWindow::slotScalingChanged(Scaling scaling)
{
    switch(scaling)
    {
    case FitToPage:
        m_fitToPageAction->setChecked(true);
        m_scalingComboBox->setCurrentIndex(0);
        break;
    case FitToPageWidth:
        m_fitToPageWidthAction->setChecked(true);
        m_scalingComboBox->setCurrentIndex(1);
        break;
    case ScaleTo50:
        m_scaleTo50Action->setChecked(true);
        m_scalingComboBox->setCurrentIndex(2);
        break;
    case ScaleTo75:
        m_scaleTo75Action->setChecked(true);
        m_scalingComboBox->setCurrentIndex(3);
        break;
    case ScaleTo100:
        m_scaleTo100Action->setChecked(true);
        m_scalingComboBox->setCurrentIndex(4);
        break;
    case ScaleTo125:
        m_scaleTo125Action->setChecked(true);
        m_scalingComboBox->setCurrentIndex(5);
        break;
    case ScaleTo150:
        m_scaleTo150Action->setChecked(true);
        m_scalingComboBox->setCurrentIndex(6);
        break;
    case ScaleTo200:
        m_scaleTo200Action->setChecked(true);
        m_scalingComboBox->setCurrentIndex(7);
        break;
    case ScaleTo400:
        m_scaleTo400Action->setChecked(true);
        m_scalingComboBox->setCurrentIndex(8);
        break;
    }
}

void MainWindow::slotRotationChanged(Rotation rotation)
{
    switch(rotation)
    {
    case RotateBy0:
        m_rotateBy0Action->setChecked(true);
        m_rotationComboBox->setCurrentIndex(0);
        break;
    case RotateBy90:
        m_rotateBy90Action->setChecked(true);
        m_rotationComboBox->setCurrentIndex(1);
        break;
    case RotateBy180:
        m_rotateBy180Action->setChecked(true);
        m_rotationComboBox->setCurrentIndex(2);
        break;
    case RotateBy270:
        m_rotateBy270Action->setChecked(true);
        m_rotationComboBox->setCurrentIndex(3);
        break;
    }
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
    m_openAction->setIconVisibleInMenu(true);
    connect(m_openAction, SIGNAL(triggered()), this, SLOT(slotOpen()));

    if(QIcon::hasThemeIcon("document-open"))
    {
        m_openAction->setIcon(QIcon::fromTheme("document-open"));
    }
    else
    {
#ifdef DATA_INSTALL_PATH
        m_openAction->setIcon(QIcon(dataInstallPath + "/document-open.svg"));
#else
        m_openAction->setIcon(QIcon(":/icons/document-open.svg"));
#endif
    }

    // recently used

    m_recentlyUsedAction = new RecentlyUsedAction(this);
    m_recentlyUsedAction->setIcon(QIcon::fromTheme("document-open-recent"));
    m_recentlyUsedAction->setIconVisibleInMenu(true);
    connect(m_recentlyUsedAction, SIGNAL(entrySelected(QString)), this, SLOT(slotRecentyUsedEntrySelected(QString)));

    // refresh

    m_refreshAction = new QAction(tr("&Refresh"), this);
    m_refreshAction->setShortcut(QKeySequence::Refresh);
    m_refreshAction->setIconVisibleInMenu(true);
    connect(m_refreshAction, SIGNAL(triggered()), this, SLOT(slotRefresh()));

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

    // saveCopy

    m_saveCopyAction = new QAction(tr("&Save copy..."), this);
    m_saveCopyAction->setShortcut(QKeySequence::Save);
    m_saveCopyAction->setIconVisibleInMenu(true);
    connect(m_saveCopyAction, SIGNAL(triggered()), this, SLOT(slotSaveCopy()));

    if(QIcon::hasThemeIcon("document-save"))
    {
        m_saveCopyAction->setIcon(QIcon::fromTheme("document-save"));
    }
    else
    {
#ifdef DATA_INSTALL_PATH
        m_saveCopyAction->setIcon(QIcon(dataInstallPath + "/document-save.svg"));
#else
        m_saveCopyAction->setIcon(QIcon(":/icons/document-save.svg"));
#endif
    }

    // print

    m_printAction = new QAction(tr("&Print..."), this);
    m_printAction->setShortcut(QKeySequence::Print);
    m_printAction->setIconVisibleInMenu(true);
    connect(m_printAction, SIGNAL(triggered()), this, SLOT(slotPrint()));

    if(QIcon::hasThemeIcon("document-print"))
    {
        m_printAction->setIcon(QIcon::fromTheme("document-print"));
    }
    else
    {
#ifdef DATA_INSTALL_PATH
        m_printAction->setIcon(QIcon(dataInstallPath + "/document-print.svg"));
#else
        m_printAction->setIcon(QIcon(":/icons/document-print.svg"));
#endif
    }

    // exit

    m_exitAction = new QAction(tr("&Exit"), this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    m_exitAction->setIcon(QIcon::fromTheme("application-exit"));
    m_exitAction->setIconVisibleInMenu(true);
    connect(m_exitAction, SIGNAL(triggered()), this, SLOT(close()));

    // previousPage

    m_previousPageAction = new QAction(tr("&Previous page"), this);
    m_previousPageAction->setShortcut(QKeySequence::MoveToPreviousPage);
    m_previousPageAction->setIconVisibleInMenu(true);
    connect(m_previousPageAction, SIGNAL(triggered()), this, SLOT(slotPreviousPage()));

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

    // nextPage

    m_nextPageAction = new QAction(tr("&Next page"), this);
    m_nextPageAction->setShortcut(QKeySequence::MoveToNextPage);
    m_nextPageAction->setIconVisibleInMenu(true);
    connect(m_nextPageAction, SIGNAL(triggered()), this, SLOT(slotNextPage()));

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

    // firstPage

    m_firstPageAction = new QAction(tr("&First page"), this);
    m_firstPageAction->setShortcut(QKeySequence::MoveToStartOfDocument);
    m_firstPageAction->setIconVisibleInMenu(true);
    connect(m_firstPageAction, SIGNAL(triggered()), this, SLOT(slotFirstPage()));

    if(QIcon::hasThemeIcon("go-first"))
    {
        m_firstPageAction->setIcon(QIcon::fromTheme("go-first"));
    }
    else
    {
#ifdef DATA_INSTALL_PATH
        m_firstPageAction->setIcon(QIcon(dataInstallPath + "/go-first.svg"));
#else
        m_firstPageAction->setIcon(QIcon(":/icons/go-first.svg"));
#endif
    }

    // lastPage

    m_lastPageAction = new QAction(tr("&Last page"), this);
    m_lastPageAction->setShortcut(QKeySequence::MoveToEndOfDocument);
    m_lastPageAction->setIconVisibleInMenu(true);
    connect(m_lastPageAction, SIGNAL(triggered()), this, SLOT(slotLastPage()));

    if(QIcon::hasThemeIcon("go-last"))
    {
        m_lastPageAction->setIcon(QIcon::fromTheme("go-last"));
    }
    else
    {
#ifdef DATA_INSTALL_PATH
        m_lastPageAction->setIcon(QIcon(dataInstallPath + "/go-last.svg"));
#else
        m_lastPageAction->setIcon(QIcon(":/icons/go-last.svg"));
#endif
    }

    // search

    m_searchAction = new QAction(tr("&Search..."), this);
    m_searchAction->setShortcut(QKeySequence::Find);
    m_searchAction->setIcon(QIcon::fromTheme("edit-find"));
    m_searchAction->setIconVisibleInMenu(true);
    connect(m_searchAction, SIGNAL(triggered()), this, SLOT(slotSearch()));

    // findPrevious

    m_findPreviousAction = new QAction(tr("Find previous"), this);
    m_findPreviousAction->setShortcut(QKeySequence::FindPrevious);
    connect(m_findPreviousAction, SIGNAL(triggered()), this, SLOT(slotFindPrevious()));

    // findNext

    m_findNextAction = new QAction(tr("Find next"), this);
    m_findNextAction->setShortcut(QKeySequence::FindNext);
    connect(m_findNextAction, SIGNAL(triggered()), this, SLOT(slotFindNext()));

    // settings

    m_settingsAction = new QAction(tr("Settings..."), this);
    connect(m_settingsAction, SIGNAL(triggered()), this, SLOT(slotSettings()));

    // pageLayout

    m_onePageAction = new QAction(tr("One page"), this);
    m_onePageAction->setCheckable(true);
    m_onePageAction->setData(OnePage);
    m_twoPagesAction = new QAction(tr("Two pages"), this);
    m_twoPagesAction->setCheckable(true);
    m_twoPagesAction->setData(TwoPages);
    m_oneColumnAction = new QAction(tr("One column"), this);
    m_oneColumnAction->setCheckable(true);
    m_oneColumnAction->setData(OneColumn);
    m_twoColumnsAction = new QAction(tr("Two columns"), this);
    m_twoColumnsAction->setCheckable(true);
    m_twoColumnsAction->setData(TwoColumns);

    m_pageLayoutGroup = new QActionGroup(this);
    m_pageLayoutGroup->addAction(m_onePageAction);
    m_pageLayoutGroup->addAction(m_twoPagesAction);
    m_pageLayoutGroup->addAction(m_oneColumnAction);
    m_pageLayoutGroup->addAction(m_twoColumnsAction);
    connect(m_pageLayoutGroup, SIGNAL(selected(QAction*)), this, SLOT(slotPageLayoutTriggered(QAction*)));

    // scaling

    m_fitToPageAction = new QAction(tr("Fit to page"), this);
    m_fitToPageAction->setCheckable(true);
    m_fitToPageAction->setData(FitToPage);
    m_fitToPageWidthAction = new QAction(tr("Fit to page width"), this);
    m_fitToPageWidthAction->setCheckable(true);
    m_fitToPageWidthAction->setData(FitToPageWidth);
    m_scaleTo50Action = new QAction(tr("Scale to %1%").arg(50), this);
    m_scaleTo50Action->setCheckable(true);
    m_scaleTo50Action->setData(ScaleTo50);
    m_scaleTo75Action = new QAction(tr("Scale to %1%").arg(75), this);
    m_scaleTo75Action->setCheckable(true);
    m_scaleTo75Action->setData(ScaleTo75);
    m_scaleTo100Action = new QAction(tr("Scale to %1%").arg(100), this);
    m_scaleTo100Action->setCheckable(true);
    m_scaleTo100Action->setData(ScaleTo100);
    m_scaleTo125Action = new QAction(tr("Scale to %1%").arg(125), this);
    m_scaleTo125Action->setCheckable(true);
    m_scaleTo125Action->setData(ScaleTo125);
    m_scaleTo150Action = new QAction(tr("Scale to %1%").arg(150), this);
    m_scaleTo150Action->setCheckable(true);
    m_scaleTo150Action->setData(ScaleTo150);
    m_scaleTo200Action = new QAction(tr("Scale to %1%").arg(200), this);
    m_scaleTo200Action->setCheckable(true);
    m_scaleTo200Action->setData(ScaleTo200);
    m_scaleTo400Action = new QAction(tr("Scale to %1%").arg(400), this);
    m_scaleTo400Action->setCheckable(true);
    m_scaleTo400Action->setData(ScaleTo400);

    m_scalingGroup = new QActionGroup(this);
    m_scalingGroup->addAction(m_fitToPageAction);
    m_scalingGroup->addAction(m_fitToPageWidthAction);
    m_scalingGroup->addAction(m_scaleTo50Action);
    m_scalingGroup->addAction(m_scaleTo75Action);
    m_scalingGroup->addAction(m_scaleTo100Action);
    m_scalingGroup->addAction(m_scaleTo125Action);
    m_scalingGroup->addAction(m_scaleTo150Action);
    m_scalingGroup->addAction(m_scaleTo200Action);
    m_scalingGroup->addAction(m_scaleTo400Action);
    connect(m_scalingGroup, SIGNAL(selected(QAction*)), this, SLOT(slotScalingTriggered(QAction*)));

    // rotation

    m_rotateBy0Action = new QAction(trUtf8("Rotate by %1°").arg(0), this);
    m_rotateBy0Action->setCheckable(true);
    m_rotateBy0Action->setData(RotateBy0);
    m_rotateBy90Action = new QAction(trUtf8("Rotate by %1°").arg(90), this);
    m_rotateBy90Action->setCheckable(true);
    m_rotateBy90Action->setData(RotateBy90);
    m_rotateBy180Action = new QAction(trUtf8("Rotate by %1°").arg(180), this);
    m_rotateBy180Action->setCheckable(true);
    m_rotateBy180Action->setData(RotateBy180);
    m_rotateBy270Action = new QAction(trUtf8("Rotate by %1°").arg(270), this);
    m_rotateBy270Action->setCheckable(true);
    m_rotateBy270Action->setData(RotateBy270);

    m_rotationGroup = new QActionGroup(this);
    m_rotationGroup->addAction(m_rotateBy0Action);
    m_rotationGroup->addAction(m_rotateBy90Action);
    m_rotationGroup->addAction(m_rotateBy180Action);
    m_rotationGroup->addAction(m_rotateBy270Action);
    connect(m_rotationGroup, SIGNAL(selected(QAction*)), this, SLOT(slotRotationTriggered(QAction*)));

    // presentation

    m_presentationAction = new QAction(tr("Presentation..."), this);
    m_presentationAction->setShortcut(QKeySequence(Qt::Key_F10));
    connect(m_presentationAction, SIGNAL(triggered()), this, SLOT(slotPresentation()));

    // fullscreen

    m_fullscreenAction = new QAction(tr("&Fullscreen"), this);
    m_fullscreenAction->setCheckable(true);
    m_fullscreenAction->setShortcut(QKeySequence(Qt::Key_F11));
    m_fullscreenAction->setIcon(QIcon::fromTheme("view-fullscreen"));
    m_fullscreenAction->setIconVisibleInMenu(true);
    connect(m_fullscreenAction, SIGNAL(triggered()), this, SLOT(slotFullscreen()));

    // addTab

    m_addTabAction = new QAction(tr("&Add tab..."), this);
    m_addTabAction->setShortcut(QKeySequence::AddTab);
    connect(m_addTabAction, SIGNAL(triggered()), this, SLOT(slotAddTab()));

    // previousTab

    m_previousTabAction = new QAction(tr("&Previous tab"), this);
    m_previousTabAction->setShortcut(QKeySequence::PreviousChild);
    connect(m_previousTabAction, SIGNAL(triggered()), this, SLOT(slotPreviousTab()));

    // nextTab

    m_nextTabAction = new QAction(tr("&Next tab"), this);
    m_nextTabAction->setShortcut(QKeySequence::NextChild);
    connect(m_nextTabAction, SIGNAL(triggered()), this, SLOT(slotNextTab()));

    // closeTab

    m_closeTabAction = new QAction(tr("&Close tab"), this);
    m_closeTabAction->setShortcut(QKeySequence::Close);
    m_closeTabAction->setIcon(QIcon::fromTheme("window-close"));
    m_closeTabAction->setIconVisibleInMenu(true);
    connect(m_closeTabAction, SIGNAL(triggered()), this, SLOT(slotCloseTab()));

    // closeAllTabs

    m_closeAllTabsAction = new QAction(tr("Close all &tabs"), this);
    m_closeAllTabsAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_W));
    connect(m_closeAllTabsAction, SIGNAL(triggered()), this, SLOT(slotCloseAllTabs()));

    // closeAllTabsButCurrent

    m_closeAllTabsButCurrentTabAction = new QAction(tr("Close all tabs &but current tab"), this);
    m_closeAllTabsButCurrentTabAction->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_W));
    connect(m_closeAllTabsButCurrentTabAction, SIGNAL(triggered()), this, SLOT(slotCloseAllTabsButCurrentTab()));

    // contents

    m_contentsAction = new QAction(tr("&Contents"), this);
    m_contentsAction->setShortcut(QKeySequence::HelpContents);
    m_contentsAction->setIcon(QIcon::fromTheme("help-contents"));
    m_contentsAction->setIconVisibleInMenu(true);
    connect(m_contentsAction, SIGNAL(triggered()), this, SLOT(slotContents()));

    // about

    m_aboutAction = new QAction(tr("&About"), this);
    m_aboutAction->setIcon(QIcon::fromTheme("help-about"));
    m_aboutAction->setIconVisibleInMenu(true);
    connect(m_aboutAction, SIGNAL(triggered()), this, SLOT(slotAbout()));
}

void MainWindow::createWidgets()
{
    // tabWidget

    m_tabWidget = new QTabWidget(this);

    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setElideMode(Qt::ElideRight);

    connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(slotTabWidgetCurrentChanged(int)));
    connect(m_tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(slotTabWidgetTabCloseRequested(int)));

    // currentPage

    m_currentPageWidget = new QWidget();
    m_currentPageLabel = new QLabel(tr("&Page:"), m_currentPageWidget);
    m_currentPageLineEdit = new QLineEdit(m_currentPageWidget);
    m_currentPageValidator = new QIntValidator(m_currentPageWidget);
    m_numberOfPagesLabel = new QLabel(m_currentPageWidget);

    m_currentPageWidget->setMaximumWidth(200);
    m_currentPageLabel->setBuddy(m_currentPageLineEdit);
    m_currentPageLineEdit->setAlignment(Qt::AlignCenter);
    m_currentPageLineEdit->setValidator(m_currentPageValidator);

    m_currentPageWidget->setLayout(new QHBoxLayout());
    m_currentPageWidget->layout()->addWidget(m_currentPageLabel);
    m_currentPageWidget->layout()->addWidget(m_currentPageLineEdit);
    m_currentPageWidget->layout()->addWidget(m_numberOfPagesLabel);

    connect(m_currentPageLineEdit, SIGNAL(returnPressed()), this, SLOT(slotCurrentPageLineEditReturnPressed()));

    // pageLayout

    m_pageLayoutWidget = new QWidget();
    m_pageLayoutLabel = new QLabel(tr("Page &layout:"), m_pageLayoutWidget);
    m_pageLayoutComboBox = new QComboBox(m_pageLayoutWidget);

    m_pageLayoutWidget->setMaximumWidth(300);
    m_pageLayoutLabel->setBuddy(m_pageLayoutComboBox);

    m_pageLayoutComboBox->addItem(tr("One page"), OnePage);
    m_pageLayoutComboBox->addItem(tr("Two pages"), TwoPages);
    m_pageLayoutComboBox->addItem(tr("One column"), OneColumn);
    m_pageLayoutComboBox->addItem(tr("Two columns"), TwoColumns);

    m_pageLayoutWidget->setLayout(new QHBoxLayout());
    m_pageLayoutWidget->layout()->addWidget(m_pageLayoutLabel);
    m_pageLayoutWidget->layout()->addWidget(m_pageLayoutComboBox);

    connect(m_pageLayoutComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slotPageLayoutCurrentIndexChanged(int)));

    // scaling

    m_scalingWidget = new QWidget();
    m_scalingLabel = new QLabel(tr("&Scaling:"), m_scalingWidget);
    m_scalingComboBox = new QComboBox(m_scalingWidget);

    m_scalingWidget->setMaximumWidth(300);
    m_scalingLabel->setBuddy(m_scalingComboBox);

    m_scalingComboBox->addItem(tr("Fit to page"), FitToPage);
    m_scalingComboBox->addItem(tr("Fit to page width"), FitToPageWidth);
    m_scalingComboBox->addItem(tr("Scale to %1%").arg(50), ScaleTo50);
    m_scalingComboBox->addItem(tr("Scale to %1%").arg(75), ScaleTo75);
    m_scalingComboBox->addItem(tr("Scale to %1%").arg(100), ScaleTo100);
    m_scalingComboBox->addItem(tr("Scale to %1%").arg(125), ScaleTo125);
    m_scalingComboBox->addItem(tr("Scale to %1%").arg(150), ScaleTo150);
    m_scalingComboBox->addItem(tr("Scale to %1%").arg(200), ScaleTo200);
    m_scalingComboBox->addItem(tr("Scale to %1%").arg(400), ScaleTo400);

    m_scalingWidget->setLayout(new QHBoxLayout());
    m_scalingWidget->layout()->addWidget(m_scalingLabel);
    m_scalingWidget->layout()->addWidget(m_scalingComboBox);

    connect(m_scalingComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slotScalingCurrentIndexChanged(int)));

    // rotationWidget

    m_rotationWidget = new QWidget();
    m_rotationLabel = new QLabel(tr("&Rotation:"), m_rotationWidget);
    m_rotationComboBox = new QComboBox(m_rotationWidget);

    m_rotationWidget->setMaximumWidth(300);
    m_rotationLabel->setBuddy(m_rotationComboBox);

    m_rotationComboBox->addItem(trUtf8("Rotate by %1°").arg(0), RotateBy0);
    m_rotationComboBox->addItem(trUtf8("Rotate by %1°").arg(90), RotateBy90);
    m_rotationComboBox->addItem(trUtf8("Rotate by %1°").arg(180), RotateBy180);
    m_rotationComboBox->addItem(trUtf8("Rotate by %1°").arg(270), RotateBy270);

    m_rotationWidget->setLayout(new QHBoxLayout());
    m_rotationWidget->layout()->addWidget(m_rotationLabel);
    m_rotationWidget->layout()->addWidget(m_rotationComboBox);

    connect(m_rotationComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slotRotationCurrentIndexChanged(int)));

    // search

    m_searchWidget = new QWidget();
    m_searchLabel = new QLabel(tr("Se&arch:"), m_searchWidget);
    m_searchLineEdit = new QLineEdit(m_searchWidget);
    m_searchTimer = new QTimer(this);
    m_matchCaseCheckBox = new QCheckBox(tr("Match &case"), m_searchWidget);
    m_highlightAllCheckBox = new QCheckBox(tr("H&ighlight all"), m_searchWidget);
    m_findPreviousButton = new QPushButton(tr("Find &previous"), m_searchWidget);
    m_findNextButton = new QPushButton(tr("Find &next"), m_searchWidget);

    m_searchLabel->setBuddy(m_searchLineEdit);
    m_searchTimer->setSingleShot(true);
    m_searchTimer->setInterval(2000);

    m_searchWidget->setLayout(new QHBoxLayout());
    m_searchWidget->layout()->addWidget(m_searchLabel);
    m_searchWidget->layout()->addWidget(m_searchLineEdit);
    m_searchWidget->layout()->addWidget(m_matchCaseCheckBox);
    m_searchWidget->layout()->addWidget(m_highlightAllCheckBox);
    m_searchWidget->layout()->addWidget(m_findPreviousButton);
    m_searchWidget->layout()->addWidget(m_findNextButton);

    connect(m_searchLineEdit, SIGNAL(textEdited(QString)), m_searchTimer, SLOT(start()));
    connect(m_searchLineEdit, SIGNAL(returnPressed()), this, SLOT(slotStartSearch()));
    connect(m_searchTimer, SIGNAL(timeout()), this, SLOT(slotStartSearch()));

    connect(m_highlightAllCheckBox, SIGNAL(toggled(bool)), this, SLOT(slotHighlightAllCheckBoxToggled()));

    connect(m_findPreviousButton, SIGNAL(clicked()), this, SLOT(slotFindPrevious()));
    connect(m_findNextButton, SIGNAL(clicked()), this, SLOT(slotFindNext()));
}

void MainWindow::createToolBars()
{
    // file

    m_fileToolBar = new QToolBar(tr("&File"));
    m_fileToolBar->setObjectName("fileToolBar");

    m_fileToolBar->addAction(m_openAction);
    m_fileToolBar->addAction(m_refreshAction);
    m_fileToolBar->addAction(m_saveCopyAction);
    m_fileToolBar->addAction(m_printAction);

    this->addToolBar(Qt::TopToolBarArea, m_fileToolBar);

    // edit

    m_editToolBar = new QToolBar(tr("&Edit"));
    m_editToolBar->setObjectName("editToolBar");

    m_editToolBar->addAction(m_firstPageAction);
    m_editToolBar->addAction(m_previousPageAction);
    m_editToolBar->addWidget(m_currentPageWidget);
    m_editToolBar->addAction(m_nextPageAction);
    m_editToolBar->addAction(m_lastPageAction);

    this->addToolBar(Qt::TopToolBarArea, m_editToolBar);

    // viewToolBar

    m_viewToolBar = new QToolBar(tr("&View"));
    m_viewToolBar->setObjectName("viewToolBar");

    m_viewToolBar->setHidden(true);

    m_viewToolBar->addWidget(m_pageLayoutWidget);
    m_viewToolBar->addWidget(m_scalingWidget);
    m_viewToolBar->addWidget(m_rotationWidget);

    this->addToolBar(Qt::TopToolBarArea, m_viewToolBar);

    // search

    m_searchToolBar = new QToolBar(tr("&Search"));
    m_searchToolBar->setObjectName("searchToolBar");

    m_searchToolBar->setHidden(true);
    m_searchToolBar->setMovable(false);

    m_searchToolBar->addWidget(m_searchWidget);

    this->addToolBar(Qt::BottomToolBarArea, m_searchToolBar);
}

void MainWindow::createDocks()
{
    // outline

    m_outlineDock = new QDockWidget(tr("&Outline"), this);
    m_outlineDock->setObjectName("outlineDock");
    m_outlineDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_outlineDock->setFeatures(QDockWidget::AllDockWidgetFeatures);

    m_outlineView = new OutlineView(m_outlineDock);
    m_outlineDock->setWidget(m_outlineView);
    m_outlineDock->hide();

    this->addDockWidget(Qt::LeftDockWidgetArea, m_outlineDock);

    // thumbnails

    m_thumbnailsDock = new QDockWidget(tr("&Thumbnails"), this);
    m_thumbnailsDock->setObjectName("thumbnailsDock");
    m_thumbnailsDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_thumbnailsDock->setFeatures(QDockWidget::AllDockWidgetFeatures);

    m_thumbnailsView = new ThumbnailsView(m_thumbnailsDock);
    m_thumbnailsDock->setWidget(m_thumbnailsView);
    m_thumbnailsDock->hide();

    this->addDockWidget(Qt::RightDockWidgetArea, m_thumbnailsDock);
}

void MainWindow::createMenus()
{
    // file

    m_fileMenu = this->menuBar()->addMenu(tr("&File"));
    m_fileMenu->addAction(m_openAction);
    m_fileMenu->addAction(m_recentlyUsedAction);
    m_fileMenu->addAction(m_refreshAction);
    m_fileMenu->addAction(m_saveCopyAction);
    m_fileMenu->addAction(m_printAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_exitAction);

    // edit

    m_editMenu = this->menuBar()->addMenu(tr("&Edit"));
    m_editMenu->addAction(m_nextPageAction);
    m_editMenu->addAction(m_previousPageAction);
    m_editMenu->addAction(m_firstPageAction);
    m_editMenu->addAction(m_lastPageAction);
    m_editMenu->addSeparator();
    m_editMenu->addAction(m_searchAction);
    m_editMenu->addAction(m_findPreviousAction);
    m_editMenu->addAction(m_findNextAction);
    m_editMenu->addSeparator();
    m_editMenu->addAction(m_settingsAction);

    // view

    m_viewMenu = this->menuBar()->addMenu(tr("&View"));
    m_viewMenu->addAction(m_onePageAction);
    m_viewMenu->addAction(m_twoPagesAction);
    m_viewMenu->addAction(m_oneColumnAction);
    m_viewMenu->addAction(m_twoColumnsAction);
    m_viewMenu->addSeparator();
    m_viewMenu->addAction(m_fitToPageAction);
    m_viewMenu->addAction(m_fitToPageWidthAction);
    m_viewMenu->addAction(m_scaleTo50Action);
    m_viewMenu->addAction(m_scaleTo75Action);
    m_viewMenu->addAction(m_scaleTo100Action);
    m_viewMenu->addAction(m_scaleTo125Action);
    m_viewMenu->addAction(m_scaleTo150Action);
    m_viewMenu->addAction(m_scaleTo200Action);
    m_viewMenu->addAction(m_scaleTo400Action);
    m_viewMenu->addSeparator();
    m_viewMenu->addAction(m_rotateBy0Action);
    m_viewMenu->addAction(m_rotateBy90Action);
    m_viewMenu->addAction(m_rotateBy180Action);
    m_viewMenu->addAction(m_rotateBy270Action);
    m_viewMenu->addSeparator();

    // toolBar

    QMenu *toolBarMenu = m_viewMenu->addMenu(tr("&Toolbars"));
    toolBarMenu->addAction(m_fileToolBar->toggleViewAction());
    toolBarMenu->addAction(m_editToolBar->toggleViewAction());
    toolBarMenu->addAction(m_viewToolBar->toggleViewAction());

    // dock

    QMenu *dockMenu = m_viewMenu->addMenu(tr("&Docks"));
    dockMenu->addAction(m_outlineDock->toggleViewAction());
    dockMenu->addAction(m_thumbnailsDock->toggleViewAction());

    m_viewMenu->addAction(m_presentationAction);
    m_viewMenu->addAction(m_fullscreenAction);

    // tab

    m_tabMenu = this->menuBar()->addMenu(tr("&Tab"));
    m_tabMenu->addAction(m_addTabAction);
    m_tabMenu->addAction(m_previousTabAction);
    m_tabMenu->addAction(m_nextTabAction);
    m_tabMenu->addAction(m_closeTabAction);
    m_tabMenu->addSeparator();
    m_tabMenu->addAction(m_closeAllTabsAction);
    m_tabMenu->addAction(m_closeAllTabsButCurrentTabAction);
    m_tabMenu->addSeparator();

    // help

    m_helpMenu = this->menuBar()->addMenu(tr("&Help"));
    m_helpMenu->addAction(m_contentsAction);
    m_helpMenu->addAction(m_aboutAction);
}
