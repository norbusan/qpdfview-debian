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

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
    m_settings(),
    m_geometry()
{
    this->createActions();
    this->createWidgets();
    this->createToolBars();
    this->createDocks();
    this->createMenus();

    this->setAcceptDrops(true);

    this->slotTabWidgetCurrentChanged(-1);

    // settings

    m_matchCaseCheckBox->setChecked(m_settings.value("mainWindow/matchCase", true).toBool());

    this->restoreGeometry(m_settings.value("mainWindow/geometry").toByteArray());
    this->restoreState(m_settings.value("mainWindow/state").toByteArray());
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
        return this->openInNewTab(filePath, page, top);
    }
}

bool MainWindow::openInNewTab(const QString &filePath, int page, qreal top)
{
    DocumentView *documentView = new DocumentView();

    if(documentView->open(filePath))
    {
        int index = m_tabWidget->addTab(documentView, QFileInfo(filePath).completeBaseName());
        m_tabWidget->setTabToolTip(index, QFileInfo(filePath).completeBaseName());
        m_tabWidget->setCurrentIndex(index);

        m_tabMenu->addAction(documentView->tabAction());

        connect(documentView, SIGNAL(filePathChanged(QString)), this, SLOT(slotFilePathChanged(QString)));
        connect(documentView, SIGNAL(numberOfPagesChanged(int)), this, SLOT(slotNumberOfPagesChanged(int)));

        connect(documentView, SIGNAL(currentPageChanged(int)), this, SLOT(slotCurrentPageChanged(int)));

        connect(documentView, SIGNAL(searchProgressed(int)), this, SLOT(slotSearchProgressed(int)));
        connect(documentView, SIGNAL(searchCanceled()), this, SLOT(slotSearchCanceled()));
        connect(documentView, SIGNAL(searchFinished()), this, SLOT(slotSearchFinished()));

        connect(documentView, SIGNAL(pageLayoutChanged(DocumentView::PageLayout)), this, SLOT(slotPageLayoutChanged(DocumentView::PageLayout)));
        connect(documentView, SIGNAL(scalingChanged(DocumentView::Scaling)), this, SLOT(slotScalingChanged(DocumentView::Scaling)));
        connect(documentView, SIGNAL(rotationChanged(DocumentView::Rotation)), this, SLOT(slotRotationChanged(DocumentView::Rotation)));

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
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QMainWindow::closeEvent(event);

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
                this->openInNewTab(url.path());
            }
        }
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
            if(this->open(filePath))
            {
                m_recentlyUsedAction->addEntry(filePath);

                m_settings.setValue("mainWindow/path", QFileInfo(filePath).path());
            }
        }
    }
    else
    {
        this->slotOpenInNewTab();
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
        if(this->openInNewTab(filePath))
        {
            m_settings.setValue("mainWindow/path", QFileInfo(filePath).path());

            m_recentlyUsedAction->addEntry(filePath);
        }
    }

    this->slotTabWidgetCurrentChanged(m_tabWidget->currentIndex());

    connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(slotTabWidgetCurrentChanged(int)));
}

void MainWindow::slotRefresh()
{
    DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

    documentView->refresh();
}

void MainWindow::slotSaveCopy()
{
    QString filePath = QFileDialog::getSaveFileName(this, tr("Save copy"),
                                                    m_settings.value("mainWindow/path", QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation)).toString(),
                                                    "Portable Document Format (*.pdf)");

    if(!filePath.isEmpty())
    {
        DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

        if(documentView->saveCopy(filePath))
        {
            m_settings.setValue("mainWindow/path", QFileInfo(filePath).path());
        }
        else
        {
            QMessageBox::warning(this, tr("Warning"), tr("Could not save copy at \"%1\".").arg(filePath));
        }
    }
}

void MainWindow::slotPrint()
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

void MainWindow::slotPreviousPage()
{
    DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

    documentView->previousPage();
}

void MainWindow::slotNextPage()
{
    DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

    documentView->nextPage();
}

void MainWindow::slotFirstPage()
{
    DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

    documentView->firstPage();
}

void MainWindow::slotLastPage()
{
    DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

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
    this->slotCancelSearch();

    if(m_searchToolBar->isVisible() && !m_searchLineEdit->text().isEmpty())
    {
        DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

        documentView->startSearch(m_searchLineEdit->text(), m_matchCaseCheckBox->isChecked());
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

    if(settingsDialog.exec() == QDialog::Accepted)
    {
        for(int index = 0; index < m_tabWidget->count(); index++)
        {
            DocumentView * documentView = qobject_cast<DocumentView*>(m_tabWidget->widget(index));

            documentView->refresh();
        }
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

void MainWindow::slotPresentation()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

        PresentationView *presentationView = new PresentationView();

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
        m_tabWidget->setCurrentIndex(m_tabWidget->currentIndex()-1);
    }
    else
    {
        m_tabWidget->setCurrentIndex(m_tabWidget->count()-1);
    }
}

void MainWindow::slotNextTab()
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

void MainWindow::slotCloseTab()
{
    delete m_tabWidget->currentWidget();
}

void MainWindow::slotCloseAllTabs()
{
    while(m_tabWidget->count() > 0)
    {
        delete m_tabWidget->widget(0);
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
        m_closeAllTabsAction->setEnabled(true);
        m_closeAllTabsButCurrentTabAction->setEnabled(true);

        m_editToolBar->setEnabled(true);
        m_viewToolBar->setEnabled(true);

        m_searchToolBar->setEnabled(true);

        if(m_searchToolBar->isVisible())
        {
            this->slotCancelSearch();

            m_searchLineEdit->clear();
        }

        this->slotCurrentPageChanged(documentView->currentPage());
        this->slotNumberOfPagesChanged(documentView->numberOfPages());
        this->slotPageLayoutChanged(documentView->pageLayout());
        this->slotScalingChanged(documentView->scaling());
        this->slotRotationChanged(documentView->rotation());
        this->slotHighlightAllChanged(documentView->highlightAll());

        m_outlineDock->setWidget(documentView->outlineTreeWidget());
        m_metaInformationDock->setWidget(documentView->metaInformationTableWidget());
        m_thumbnailsDock->setWidget(documentView->thumbnailsGraphicsView());

        this->setWindowTitle(m_tabWidget->tabText(index) + " - qpdfview");
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
        m_closeAllTabsAction->setEnabled(false);
        m_closeAllTabsButCurrentTabAction->setEnabled(false);

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

        m_outlineDock->setWidget(0);
        m_metaInformationDock->setWidget(0);
        m_thumbnailsDock->setWidget(0);

        this->setWindowTitle("qpdfview");
    }
}

void MainWindow::slotTabWidgetTabCloseRequested(int index)
{
    delete m_tabWidget->widget(index);
}

void MainWindow::slotFilePathChanged(const QString &filePath)
{
    m_tabWidget->setTabText(m_tabWidget->currentIndex(), QFileInfo(filePath).completeBaseName());
    m_tabWidget->setTabToolTip(m_tabWidget->currentIndex(), QFileInfo(filePath).completeBaseName());
}

void MainWindow::slotNumberOfPagesChanged(int numberOfPages)
{
    m_currentPageValidator->setRange(1, numberOfPages);
    m_numberOfPagesLabel->setText(tr(" of %1").arg(numberOfPages));
}

void MainWindow::slotCurrentPageLineEditReturnPressed()
{
    DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

    documentView->setCurrentPage(m_currentPageLineEdit->text().toInt());
}

void MainWindow::slotCurrentPageChanged(int currentPage)
{
    m_currentPageLineEdit->setText(QLocale::system().toString(currentPage));
}

void MainWindow::slotPageLayoutTriggered(QAction *action)
{
    DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

    documentView->setPageLayout(static_cast<DocumentView::PageLayout>(action->data().toUInt()));
}

void MainWindow::slotPageLayoutCurrentIndexChanged(int index)
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

        documentView->setPageLayout(static_cast<DocumentView::PageLayout>(m_pageLayoutComboBox->itemData(index).toUInt()));
    }
}

void MainWindow::slotPageLayoutChanged(DocumentView::PageLayout pageLayout)
{
    foreach(QAction* action, m_pageLayoutGroup->actions())
    {
        if(action->data().toUInt() == static_cast<uint>(pageLayout))
        {
            action->setChecked(true);
        }
    }

    for(int index = 0; index < m_pageLayoutComboBox->count(); index++)
    {
        if(m_pageLayoutComboBox->itemData(index).toUInt() == static_cast<uint>(pageLayout))
        {
            m_pageLayoutComboBox->setCurrentIndex(index);
        }
    }
}

void MainWindow::slotScalingTriggered(QAction *action)
{
    DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

    documentView->setScaling(static_cast<DocumentView::Scaling>(action->data().toUInt()));
}

void MainWindow::slotScalingCurrentIndexChanged(int index)
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

        documentView->setScaling(static_cast<DocumentView::Scaling>(m_scalingComboBox->itemData(index).toUInt()));
    }
}

void MainWindow::slotScalingChanged(DocumentView::Scaling scaling)
{
    foreach(QAction* action, m_scalingGroup->actions())
    {
        if(action->data().toUInt() == static_cast<uint>(scaling))
        {
            action->setChecked(true);
        }
    }

    for(int index = 0; index < m_scalingComboBox->count(); index++)
    {
        if(m_scalingComboBox->itemData(index).toUInt() == static_cast<uint>(scaling))
        {
            m_scalingComboBox->setCurrentIndex(index);
        }
    }
}

void MainWindow::slotRotationTriggered(QAction *action)
{
    DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

    documentView->setRotation(static_cast<DocumentView::Rotation>(action->data().toUInt()));
}

void MainWindow::slotRotationCurrentIndexChanged(int index)
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

        documentView->setRotation(static_cast<DocumentView::Rotation>(m_rotationComboBox->itemData(index).toUInt()));
    }
}

void MainWindow::slotRotationChanged(DocumentView::Rotation rotation)
{
    foreach(QAction* action, m_rotationGroup->actions())
    {
        if(action->data().toUInt() == static_cast<uint>(rotation))
        {
            action->setChecked(true);
        }
    }

    for(int index = 0; index < m_rotationComboBox->count(); index++)
    {
        if(m_rotationComboBox->itemData(index).toUInt() == static_cast<uint>(rotation))
        {
            m_rotationComboBox->setCurrentIndex(index);
        }
    }
}

void MainWindow::slotHighlightAllCheckBoxClicked(bool checked)
{
    DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

    documentView->setHighlightAll(checked);
}

void MainWindow::slotHighlightAllChanged(bool highlightAll)
{
    m_highlightAllCheckBox->setChecked(highlightAll);
}

void MainWindow::slotRecentyUsedEntrySelected(const QString &filePath)
{
    this->open(filePath);
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

    // openInNewTab

    m_openInNewTabAction = new QAction(tr("Open in new &tab..."), this);
    m_openInNewTabAction->setShortcut(QKeySequence::AddTab);
    m_openInNewTabAction->setIconVisibleInMenu(true);
    connect(m_openInNewTabAction, SIGNAL(triggered()), this, SLOT(slotOpenInNewTab()));

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
    m_onePageAction->setData(static_cast<uint>(DocumentView::OnePage));
    m_twoPagesAction = new QAction(tr("Two pages"), this);
    m_twoPagesAction->setCheckable(true);
    m_twoPagesAction->setData(static_cast<uint>(DocumentView::TwoPages));
    m_oneColumnAction = new QAction(tr("One column"), this);
    m_oneColumnAction->setCheckable(true);
    m_oneColumnAction->setData(static_cast<uint>(DocumentView::OneColumn));
    m_twoColumnsAction = new QAction(tr("Two columns"), this);
    m_twoColumnsAction->setCheckable(true);
    m_twoColumnsAction->setData(static_cast<uint>(DocumentView::TwoColumns));

    m_pageLayoutGroup = new QActionGroup(this);
    m_pageLayoutGroup->addAction(m_onePageAction);
    m_pageLayoutGroup->addAction(m_twoPagesAction);
    m_pageLayoutGroup->addAction(m_oneColumnAction);
    m_pageLayoutGroup->addAction(m_twoColumnsAction);
    connect(m_pageLayoutGroup, SIGNAL(selected(QAction*)), this, SLOT(slotPageLayoutTriggered(QAction*)));

    // scaling

    m_fitToPageAction = new QAction(tr("Fit to page"), this);
    m_fitToPageAction->setCheckable(true);
    m_fitToPageAction->setData(static_cast<uint>(DocumentView::FitToPage));
    m_fitToPageWidthAction = new QAction(tr("Fit to page width"), this);
    m_fitToPageWidthAction->setCheckable(true);
    m_fitToPageWidthAction->setData(static_cast<uint>(DocumentView::FitToPageWidth));
    m_scaleTo50Action = new QAction(tr("Scale to %1%").arg(50), this);
    m_scaleTo50Action->setCheckable(true);
    m_scaleTo50Action->setData(static_cast<uint>(DocumentView::ScaleTo50));
    m_scaleTo75Action = new QAction(tr("Scale to %1%").arg(75), this);
    m_scaleTo75Action->setCheckable(true);
    m_scaleTo75Action->setData(static_cast<uint>(DocumentView::ScaleTo75));
    m_scaleTo100Action = new QAction(tr("Scale to %1%").arg(100), this);
    m_scaleTo100Action->setCheckable(true);
    m_scaleTo100Action->setData(static_cast<uint>(DocumentView::ScaleTo100));
    m_scaleTo125Action = new QAction(tr("Scale to %1%").arg(125), this);
    m_scaleTo125Action->setCheckable(true);
    m_scaleTo125Action->setData(static_cast<uint>(DocumentView::ScaleTo125));
    m_scaleTo150Action = new QAction(tr("Scale to %1%").arg(150), this);
    m_scaleTo150Action->setCheckable(true);
    m_scaleTo150Action->setData(static_cast<uint>(DocumentView::ScaleTo150));
    m_scaleTo200Action = new QAction(tr("Scale to %1%").arg(200), this);
    m_scaleTo200Action->setCheckable(true);
    m_scaleTo200Action->setData(static_cast<uint>(DocumentView::ScaleTo200));
    m_scaleTo400Action = new QAction(tr("Scale to %1%").arg(400), this);
    m_scaleTo400Action->setCheckable(true);
    m_scaleTo400Action->setData(static_cast<uint>(DocumentView::ScaleTo400));

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
    m_rotateBy0Action->setData(static_cast<uint>(DocumentView::RotateBy0));
    m_rotateBy90Action = new QAction(trUtf8("Rotate by %1°").arg(90), this);
    m_rotateBy90Action->setCheckable(true);
    m_rotateBy90Action->setData(static_cast<uint>(DocumentView::RotateBy90));
    m_rotateBy180Action = new QAction(trUtf8("Rotate by %1°").arg(180), this);
    m_rotateBy180Action->setCheckable(true);
    m_rotateBy180Action->setData(static_cast<uint>(DocumentView::RotateBy180));
    m_rotateBy270Action = new QAction(trUtf8("Rotate by %1°").arg(270), this);
    m_rotateBy270Action->setCheckable(true);
    m_rotateBy270Action->setData(static_cast<uint>(DocumentView::RotateBy270));

    m_rotationGroup = new QActionGroup(this);
    m_rotationGroup->addAction(m_rotateBy0Action);
    m_rotationGroup->addAction(m_rotateBy90Action);
    m_rotationGroup->addAction(m_rotateBy180Action);
    m_rotationGroup->addAction(m_rotateBy270Action);
    connect(m_rotationGroup, SIGNAL(selected(QAction*)), this, SLOT(slotRotationTriggered(QAction*)));

    // fullscreen

    m_fullscreenAction = new QAction(tr("&Fullscreen"), this);
    m_fullscreenAction->setCheckable(true);
    m_fullscreenAction->setShortcut(QKeySequence(Qt::Key_F11));
    m_fullscreenAction->setIcon(QIcon::fromTheme("view-fullscreen"));
    m_fullscreenAction->setIconVisibleInMenu(true);
    connect(m_fullscreenAction, SIGNAL(triggered()), this, SLOT(slotFullscreen()));

    // presentation

    m_presentationAction = new QAction(tr("Presentation..."), this);
    m_presentationAction->setShortcut(QKeySequence(Qt::Key_F12));
    connect(m_presentationAction, SIGNAL(triggered()), this, SLOT(slotPresentation()));

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

    // closeAllTabsButCurrentTab

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
    // central

    m_tabWidget = new QTabWidget(this);

    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setElideMode(Qt::ElideRight);

    this->setCentralWidget(m_tabWidget);

    connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(slotTabWidgetCurrentChanged(int)));
    connect(m_tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(slotTabWidgetTabCloseRequested(int)));

    // currentPage

    m_currentPageWidget = new QWidget();
    m_currentPageLabel = new QLabel(tr("&Page:"), m_currentPageWidget);
    m_currentPageLineEdit = new QLineEdit(m_currentPageWidget);
    m_currentPageValidator = new QIntValidator(m_currentPageWidget);
    m_numberOfPagesLabel = new QLabel(m_currentPageWidget);

    m_currentPageLabel->setBuddy(m_currentPageLineEdit);
    m_currentPageLineEdit->setValidator(m_currentPageValidator);

    m_currentPageLineEdit->setAlignment(Qt::AlignCenter);
    m_currentPageLineEdit->setMinimumWidth(60);
    m_currentPageLineEdit->setMaximumWidth(60);

    m_numberOfPagesLabel->setAlignment(Qt::AlignCenter);
    m_numberOfPagesLabel->setMinimumWidth(60);
    m_numberOfPagesLabel->setMaximumWidth(60);

    m_currentPageWidget->setLayout(new QHBoxLayout());
    m_currentPageWidget->layout()->addWidget(m_currentPageLabel);
    m_currentPageWidget->layout()->setSizeConstraint(QLayout::SetFixedSize);
    m_currentPageWidget->layout()->addWidget(m_currentPageLineEdit);
    m_currentPageWidget->layout()->addWidget(m_numberOfPagesLabel);

    connect(m_currentPageLineEdit, SIGNAL(returnPressed()), this, SLOT(slotCurrentPageLineEditReturnPressed()));

    // pageLayout

    m_pageLayoutWidget = new QWidget();
    m_pageLayoutLabel = new QLabel(tr("Page &layout:"), m_pageLayoutWidget);
    m_pageLayoutComboBox = new QComboBox(m_pageLayoutWidget);

    m_pageLayoutWidget->setMaximumWidth(300);
    m_pageLayoutLabel->setBuddy(m_pageLayoutComboBox);

    m_pageLayoutComboBox->addItem(tr("One page"), static_cast<uint>(DocumentView::OnePage));
    m_pageLayoutComboBox->addItem(tr("Two pages"), static_cast<uint>(DocumentView::TwoPages));
    m_pageLayoutComboBox->addItem(tr("One column"), static_cast<uint>(DocumentView::OneColumn));
    m_pageLayoutComboBox->addItem(tr("Two columns"), static_cast<uint>(DocumentView::TwoColumns));

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

    m_scalingComboBox->addItem(tr("Fit to page"), static_cast<uint>(DocumentView::FitToPage));
    m_scalingComboBox->addItem(tr("Fit to page width"), static_cast<uint>(DocumentView::FitToPageWidth));
    m_scalingComboBox->addItem(tr("Scale to %1%").arg(50), static_cast<uint>(DocumentView::ScaleTo50));
    m_scalingComboBox->addItem(tr("Scale to %1%").arg(75), static_cast<uint>(DocumentView::ScaleTo75));
    m_scalingComboBox->addItem(tr("Scale to %1%").arg(100), static_cast<uint>(DocumentView::ScaleTo100));
    m_scalingComboBox->addItem(tr("Scale to %1%").arg(125), static_cast<uint>(DocumentView::ScaleTo125));
    m_scalingComboBox->addItem(tr("Scale to %1%").arg(150), static_cast<uint>(DocumentView::ScaleTo150));
    m_scalingComboBox->addItem(tr("Scale to %1%").arg(200), static_cast<uint>(DocumentView::ScaleTo200));
    m_scalingComboBox->addItem(tr("Scale to %1%").arg(400), static_cast<uint>(DocumentView::ScaleTo400));

    m_scalingWidget->setLayout(new QHBoxLayout());
    m_scalingWidget->layout()->addWidget(m_scalingLabel);
    m_scalingWidget->layout()->addWidget(m_scalingComboBox);

    connect(m_scalingComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slotScalingCurrentIndexChanged(int)));

    // rotation

    m_rotationWidget = new QWidget();
    m_rotationLabel = new QLabel(tr("&Rotation:"), m_rotationWidget);
    m_rotationComboBox = new QComboBox(m_rotationWidget);

    m_rotationWidget->setMaximumWidth(300);
    m_rotationLabel->setBuddy(m_rotationComboBox);

    m_rotationComboBox->addItem(trUtf8("Rotate by %1°").arg(0), static_cast<uint>(DocumentView::RotateBy0));
    m_rotationComboBox->addItem(trUtf8("Rotate by %1°").arg(90), static_cast<uint>(DocumentView::RotateBy90));
    m_rotationComboBox->addItem(trUtf8("Rotate by %1°").arg(180), static_cast<uint>(DocumentView::RotateBy180));
    m_rotationComboBox->addItem(trUtf8("Rotate by %1°").arg(270), static_cast<uint>(DocumentView::RotateBy270));

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
    m_searchTimer->setInterval(2000);
    m_searchTimer->setSingleShot(true);

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

    connect(m_highlightAllCheckBox, SIGNAL(clicked(bool)), this, SLOT(slotHighlightAllCheckBoxClicked(bool)));

    connect(m_findPreviousButton, SIGNAL(clicked()), this, SLOT(slotFindPrevious()));
    connect(m_findNextButton, SIGNAL(clicked()), this, SLOT(slotFindNext()));
}

void MainWindow::createToolBars()
{
    // file

    m_fileToolBar = new QToolBar(tr("&File"));
    m_fileToolBar->setObjectName("fileToolBar");

    m_fileToolBar->addAction(m_openAction);
    m_fileToolBar->addAction(m_openInNewTabAction);
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

    // view

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

    this->addDockWidget(Qt::LeftDockWidgetArea, m_outlineDock);
    m_outlineDock->hide();

    // meta-information

    m_metaInformationDock = new QDockWidget(tr("&Meta-information"), this);
    m_metaInformationDock->setObjectName("metaInformationDock");
    m_metaInformationDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_metaInformationDock->setFeatures(QDockWidget::AllDockWidgetFeatures);

    this->addDockWidget(Qt::LeftDockWidgetArea, m_metaInformationDock);
    m_metaInformationDock->hide();

    // thumbnails

    m_thumbnailsDock = new QDockWidget(tr("&Thumbnails"), this);
    m_thumbnailsDock->setObjectName("thumbnailsDock");
    m_thumbnailsDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_thumbnailsDock->setFeatures(QDockWidget::AllDockWidgetFeatures);

    this->addDockWidget(Qt::RightDockWidgetArea, m_thumbnailsDock);
    m_thumbnailsDock->hide();
}

void MainWindow::createMenus()
{
    // file

    m_fileMenu = this->menuBar()->addMenu(tr("&File"));
    m_fileMenu->addAction(m_openAction);
    m_fileMenu->addAction(m_openInNewTabAction);
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

    m_tabMenu = this->menuBar()->addMenu(tr("&Tab"));
    m_tabMenu->addAction(m_previousTabAction);
    m_tabMenu->addAction(m_nextTabAction);
    m_tabMenu->addSeparator();
    m_tabMenu->addAction(m_closeTabAction);
    m_tabMenu->addAction(m_closeAllTabsAction);
    m_tabMenu->addAction(m_closeAllTabsButCurrentTabAction);
    m_tabMenu->addSeparator();

    // help

    m_helpMenu = this->menuBar()->addMenu(tr("&Help"));
    m_helpMenu->addAction(m_contentsAction);
    m_helpMenu->addAction(m_aboutAction);
}

MainWindowAdaptor::MainWindowAdaptor(MainWindow *mainWindow) : QDBusAbstractAdaptor(mainWindow)
{
}

bool MainWindowAdaptor::open(const QString &filePath, int page, qreal top)
{
    MainWindow *mainWindow = qobject_cast<MainWindow*>(parent()); Q_ASSERT(mainWindow);

    return mainWindow->open(filePath, page, top);
}

bool MainWindowAdaptor::openInNewTab(const QString &filePath, int page, qreal top)
{
    MainWindow *mainWindow = qobject_cast<MainWindow*>(parent()); Q_ASSERT(mainWindow);

    return mainWindow->openInNewTab(filePath, page, top);
}

void MainWindowAdaptor::refresh(const QString &filePath, int page, qreal top)
{
    MainWindow *mainWindow = qobject_cast<MainWindow*>(parent()); Q_ASSERT(mainWindow);

    bool openInNewTab = true;

    for(int index = 0; index < mainWindow->m_tabWidget->count(); index++)
    {
        DocumentView *documentView = qobject_cast<DocumentView*>(mainWindow->m_tabWidget->widget(index));

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
