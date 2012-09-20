/*

Copyright 2012 Adam Reichold

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

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    {
        // settings

        m_settings = new QSettings(this);

        if(m_settings->contains("mainWindow/iconTheme"))
        {
            QIcon::setThemeName(m_settings->value("mainWindow/iconTheme").toString());
        }

        if(m_settings->contains("mainWindow/styleSheet"))
        {
            qApp->setStyleSheet(m_settings->value("mainWindow/styleSheet").toString());
        }

        restoreSettings();
    }

    setAcceptDrops(true);

    createWidgets();
    createActions();
    createToolBars();
    createDocks();
    createMenus();

    restoreGeometry(m_settings->value("mainWindow/geometry").toByteArray());
    restoreState(m_settings->value("mainWindow/state").toByteArray());

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
        if(currentTab()->open(filePath))
        {
            QFileInfo fileInfo(filePath);

            m_settings->setValue("mainWindow/path", fileInfo.absolutePath());
            m_recentlyUsedMenu->addOpenAction(filePath);

            m_tabWidget->setTabText(m_tabWidget->currentIndex(), fileInfo.completeBaseName());
            m_tabWidget->setTabToolTip(m_tabWidget->currentIndex(), fileInfo.absoluteFilePath());

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
    DocumentView* newTab = new DocumentView();

    if(newTab->open(filePath))
    {
        newTab->setContinousMode(m_settings->value("documentView/continuousMode", false).toBool());
        newTab->setLayoutMode(static_cast< DocumentView::LayoutMode >(m_settings->value("documentView/layoutMode", 0).toUInt()));
        newTab->setScaleMode(static_cast< DocumentView::ScaleMode >(m_settings->value("documentView/scaleMode", 0).toUInt()));
        newTab->setScaleFactor(m_settings->value("documentView/scaleFactor", 1.0).toReal());
        newTab->setRotation(static_cast< Poppler::Page::Rotation >(m_settings->value("documentView/rotation", 0).toUInt()));
        newTab->setHighlightAll(m_settings->value("documentView/highlightAll", false).toBool());

        QFileInfo fileInfo(filePath);

        m_settings->setValue("mainWindow/path", fileInfo.absolutePath());
        m_recentlyUsedMenu->addOpenAction(filePath);

        int index = m_tabWidget->insertTab(m_tabWidget->currentIndex() + 1, newTab, fileInfo.completeBaseName());
        m_tabWidget->setTabToolTip(index, fileInfo.absoluteFilePath());
        m_tabWidget->setCurrentIndex(index);

        QAction* tabAction = new QAction(m_tabWidget->tabText(index), newTab);
        connect(tabAction, SIGNAL(triggered()), SLOT(on_tab_triggered()));

        m_tabsMenu->addAction(tabAction);

        connect(newTab, SIGNAL(filePathChanged(QString)), SLOT(on_currentTab_filePathChanged(QString)));
        connect(newTab, SIGNAL(numberOfPagesChanged(int)), SLOT(on_currentTab_numberOfPagesChaned(int)));
        connect(newTab, SIGNAL(currentPageChanged(int)), SLOT(on_currentTab_currentPageChanged(int)));

        connect(newTab, SIGNAL(continousModeChanged(bool)), SLOT(on_currentTab_continuousModeChanged(bool)));
        connect(newTab, SIGNAL(layoutModeChanged(DocumentView::LayoutMode)), SLOT(on_currentTab_layoutModeChanged(DocumentView::LayoutMode)));
        connect(newTab, SIGNAL(scaleModeChanged(DocumentView::ScaleMode)), SLOT(on_currentTab_scaleModeChanged(DocumentView::ScaleMode)));
        connect(newTab, SIGNAL(scaleFactorChanged(qreal)), SLOT(on_currentTab_scaleFactorChanged(qreal)));
        connect(newTab, SIGNAL(rotationChanged(Poppler::Page::Rotation)), SLOT(on_currentTab_rotationChanged(Poppler::Page::Rotation)));

        connect(newTab, SIGNAL(highlightAllChanged(bool)), SLOT(on_currentTab_highlightAllChanged(bool)));
        connect(newTab, SIGNAL(rubberBandModeChanged(PageItem::RubberBandMode)), SLOT(on_currentTab_rubberBandModeChanged(PageItem::RubberBandMode)));

        connect(newTab, SIGNAL(searchProgressed(int)), SLOT(on_currentTab_searchProgressed(int)));
        connect(newTab, SIGNAL(searchFinished()), SLOT(on_currentTab_searchFinished()));
        connect(newTab, SIGNAL(searchCanceled()), SLOT(on_currentTab_searchCanceled()));

        newTab->show();

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
    for(int index = 0; index < m_tabWidget->count(); ++index)
    {
        if(QFileInfo(tab(index)->filePath()).absoluteFilePath() == QFileInfo(filePath).absoluteFilePath())
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
        m_saveCopyAction->setEnabled(true);
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

        m_copyToClipboardAction->setEnabled(true);
        m_addAnnotationAction->setEnabled(true);

        m_continuousModeAction->setEnabled(true);
        m_twoPagesModeAction->setEnabled(true);
        m_twoPagesWithCoverPageModeAction->setEnabled(true);

        m_zoomInAction->setEnabled(true);
        m_zoomOutAction->setEnabled(true);
        m_originalSizeAction->setEnabled(true);
        m_fitToPageWidthAction->setEnabled(true);
        m_fitToPageSizeAction->setEnabled(true);

        m_rotateLeftAction->setEnabled(true);
        m_rotateRightAction->setEnabled(true);

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

        on_currentTab_highlightAllChanged(currentTab()->highlightAll());
        on_currentTab_rubberBandModeChanged(currentTab()->rubberBandMode());
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

        m_jumpToPageAction->setEnabled(false);

        m_searchAction->setEnabled(false);
        m_findPreviousAction->setEnabled(false);
        m_findNextAction->setEnabled(false);
        m_cancelSearchAction->setEnabled(false);

        m_copyToClipboardAction->setEnabled(false);
        m_addAnnotationAction->setEnabled(false);

        m_continuousModeAction->setEnabled(false);
        m_twoPagesModeAction->setEnabled(false);
        m_twoPagesWithCoverPageModeAction->setEnabled(false);

        m_zoomInAction->setEnabled(false);
        m_zoomOutAction->setEnabled(false);
        m_originalSizeAction->setEnabled(false);
        m_fitToPageWidthAction->setEnabled(false);
        m_fitToPageSizeAction->setEnabled(false);

        m_rotateLeftAction->setEnabled(false);
        m_rotateRightAction->setEnabled(false);

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

        m_copyToClipboardAction->setChecked(false);
        m_addAnnotationAction->setChecked(false);

        m_continuousModeAction->setChecked(false);
        m_twoPagesModeAction->setChecked(false);
        m_twoPagesWithCoverPageModeAction->setChecked(false);

        m_fitToPageSizeAction->setChecked(false);
        m_fitToPageWidthAction->setChecked(false);
    }
}

void MainWindow::on_tabWidget_tabCloseRequested(int index)
{
    delete m_tabWidget->widget(index);
}

void MainWindow::on_currentTab_filePathChanged(const QString& filePath)
{
    for(int index = 0; index < m_tabWidget->count(); ++index)
    {
        if(sender() == m_tabWidget->widget(index))
        {
            QFileInfo fileInfo(filePath);

            m_tabWidget->setTabText(m_tabWidget->currentIndex(), fileInfo.completeBaseName());
            m_tabWidget->setTabToolTip(m_tabWidget->currentIndex(), fileInfo.absoluteFilePath());

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

        m_thumbnailsView->centerOn(currentTab()->thumbnailsItem(currentPage));
    }
}

void MainWindow::on_currentTab_continuousModeChanged(bool continuousMode)
{
    if(senderIsCurrentTab())
    {
        m_continuousModeAction->setChecked(continuousMode);

        m_settings->setValue("documentView/continuousMode", continuousMode);
    }
}

void MainWindow::on_currentTab_layoutModeChanged(DocumentView::LayoutMode layoutMode)
{
    if(senderIsCurrentTab())
    {
        m_twoPagesModeAction->setChecked(layoutMode == DocumentView::TwoPagesMode);
        m_twoPagesWithCoverPageModeAction->setChecked(layoutMode == DocumentView::TwoPagesWithCoverPageMode);

        m_settings->setValue("documentView/layoutMode", static_cast< uint >(layoutMode));
    }
}

void MainWindow::on_currentTab_scaleModeChanged(DocumentView::ScaleMode scaleMode)
{
    if(senderIsCurrentTab())
    {
        switch(scaleMode)
        {
        case DocumentView::ScaleFactor:
            m_fitToPageWidthAction->setChecked(false);
            m_fitToPageSizeAction->setChecked(false);

            on_currentTab_scaleFactorChanged(currentTab()->scaleFactor());
            break;
        case DocumentView::FitToPageWidth:
            m_fitToPageWidthAction->setChecked(true);
            m_fitToPageSizeAction->setChecked(false);

            m_scaleFactorComboBox->setCurrentIndex(0);

            m_zoomInAction->setEnabled(true);
            m_zoomOutAction->setEnabled(true);
            break;
        case DocumentView::FitToPageSize:
            m_fitToPageWidthAction->setChecked(false);
            m_fitToPageSizeAction->setChecked(true);

            m_scaleFactorComboBox->setCurrentIndex(1);

            m_zoomInAction->setEnabled(true);
            m_zoomOutAction->setEnabled(true);
            break;
        }

        m_settings->setValue("documentView/scaleMode", static_cast< uint >(scaleMode));
    }
}

void MainWindow::on_currentTab_scaleFactorChanged(qreal scaleFactor)
{
    if(senderIsCurrentTab())
    {
        if(currentTab()->scaleMode() == DocumentView::ScaleFactor)
        {
            m_scaleFactorComboBox->setCurrentIndex(m_scaleFactorComboBox->findData(scaleFactor));
            m_scaleFactorComboBox->lineEdit()->setText(QString("%1 %").arg(qRound(scaleFactor * 100.0)));

            m_zoomInAction->setDisabled(qFuzzyCompare(scaleFactor, DocumentView::maximumScaleFactor()));
            m_zoomOutAction->setDisabled(qFuzzyCompare(scaleFactor, DocumentView::minimumScaleFactor()));
        }

        m_settings->setValue("documentView/scaleFactor", scaleFactor);
    }
}

void MainWindow::on_currentTab_rotationChanged(Poppler::Page::Rotation rotation)
{
    if(senderIsCurrentTab())
    {
        m_settings->setValue("documentView/rotation", static_cast< uint >(rotation));
    }
}

void MainWindow::on_currentTab_highlightAllChanged(bool highlightAll)
{
    if(senderIsCurrentTab())
    {
        m_highlightAllCheckBox->setChecked(highlightAll);

        m_settings->setValue("documentView/highlightAll", highlightAll);
    }
}

void MainWindow::on_currentTab_rubberBandModeChanged(PageItem::RubberBandMode rubberBandMode)
{
    if(senderIsCurrentTab())
    {
        m_copyToClipboardAction->setChecked(rubberBandMode == PageItem::CopyToClipboardMode);
        m_addAnnotationAction->setChecked(rubberBandMode == PageItem::AddAnnotationMode);
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
        currentTab()->setScaleMode(DocumentView::FitToPageWidth);
    }
    else if(index == 1)
    {
        currentTab()->setScaleMode(DocumentView::FitToPageSize);
    }
    else
    {
        bool ok = false;
        qreal scaleFactor = m_scaleFactorComboBox->itemData(index).toReal(&ok);

        if(ok)
        {
            currentTab()->setScaleFactor(scaleFactor);
            currentTab()->setScaleMode(DocumentView::ScaleFactor);
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
            currentTab()->setScaleMode(DocumentView::ScaleFactor);
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
        QString path = m_settings->value("mainWindow/path", QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation)).toString();
        QString filePath = QFileDialog::getOpenFileName(this, tr("Open"), path, "Portable document format (*.pdf)");

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
    QString path = m_settings->value("mainWindow/path", QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation)).toString();
    QStringList filePaths = QFileDialog::getOpenFileNames(this, tr("Open in new tab"), path, "Portable document format (*.pdf)");

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
    QString path = m_settings->value("mainWindow/path", QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation)).toString();
    QString fileName = QFileInfo(currentTab()->filePath()).fileName();
    QString filePath = QFileInfo(QDir(path), fileName).filePath();

    filePath = QFileDialog::getSaveFileName(this, tr("Save copy"), filePath, "Portable document format (*.pdf)");

    if(!filePath.isEmpty())
    {
        if(!currentTab()->saveCopy(filePath))
        {
            QMessageBox::warning(this, tr("Warning"), tr("Could not save copy at '%1'.").arg(filePath));
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

    if(printDialog->exec() == QDialog::Accepted)
    {
        if(!currentTab()->print(printer))
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

void MainWindow::on_copyToClipboard_triggered(bool checked)
{
    currentTab()->setRubberBandMode(checked ? PageItem::CopyToClipboardMode : PageItem::ModifiersMode);
}

void MainWindow::on_addAnnotation_triggered(bool checked)
{
    currentTab()->setRubberBandMode(checked ? PageItem::AddAnnotationMode : PageItem::ModifiersMode);
}

void MainWindow::on_settings_triggered()
{
    SettingsDialog* settingsDialog = new SettingsDialog(this);

    if(settingsDialog->exec() == QDialog::Accepted)
    {
        restoreSettings();

        m_tabWidget->setTabPosition(static_cast< QTabWidget::TabPosition >(m_settings->value("mainWindow/tabPosition", 0).toUInt()));
        m_tabWidget->setTabBarPolicy(static_cast< TabWidget::TabBarPolicy >(m_settings->value("mainWindow/tabVisibility", 0).toUInt()));

        for(int index = 0; index < m_tabWidget->count(); ++index)
        {
            if(!tab(index)->refresh())
            {
                QMessageBox::warning(this, tr("Warning"), tr("Could not refresh '%1'.").arg(currentTab()->filePath()));
            }
        }
    }
}

void MainWindow::on_continuousMode_triggered(bool checked)
{
    currentTab()->setContinousMode(checked);
}

void MainWindow::on_twoPagesMode_triggered(bool checked)
{
    currentTab()->setLayoutMode(checked ? DocumentView::TwoPagesMode : DocumentView::SinglePageMode);
}

void MainWindow::on_twoPagesWithCoverPageMode_triggered(bool checked)
{
    currentTab()->setLayoutMode(checked ? DocumentView::TwoPagesWithCoverPageMode : DocumentView::SinglePageMode);
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

void MainWindow::on_fitToPageWidth_triggered(bool checked)
{
    currentTab()->setScaleMode(checked ? DocumentView::FitToPageWidth : DocumentView::ScaleFactor);
}

void MainWindow::on_fitToPageSize_triggered(bool checked)
{
    currentTab()->setScaleMode(checked ? DocumentView::FitToPageSize : DocumentView::ScaleFactor);
}

void MainWindow::on_rotateLeft_triggered()
{
    currentTab()->rotateLeft();
}

void MainWindow::on_rotateRight_triggered()
{
    currentTab()->rotateRight();
}

void MainWindow::on_fonts_triggered()
{
    QStandardItemModel* fontsModel = currentTab()->fontsModel();
    QDialog* dialog = new QDialog(this);

    QTableView* tableView = new QTableView(dialog);

    tableView->setAlternatingRowColors(true);
    tableView->setSortingEnabled(true);
    tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    tableView->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    tableView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    tableView->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    tableView->verticalHeader()->setVisible(false);

    tableView->setModel(fontsModel);

    QDialogButtonBox* dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, dialog);
    connect(dialogButtonBox, SIGNAL(accepted()), dialog, SLOT(accept()));
    connect(dialogButtonBox, SIGNAL(rejected()), dialog, SLOT(reject()));

    dialog->setLayout(new QVBoxLayout());
    dialog->layout()->addWidget(tableView);
    dialog->layout()->addWidget(dialogButtonBox);

    dialog->resize(m_settings->value("mainWindow/fontsDialogSize", dialog->sizeHint()).toSize());
    dialog->exec();
    m_settings->setValue("mainWindow/fontsDialogSize", dialog->size());

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
    currentTab()->presentation(m_settings->value("presentationView/sync", false).toBool(), m_settings->value("presentationView/screen", -1).toInt());
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
    delete m_tabWidget->currentWidget();
}

void MainWindow::on_closeAllTabs_triggered()
{
    disconnect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(on_tabWidget_currentChanged(int)));

    while(m_tabWidget->count() > 0)
    {
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
            delete m_tabWidget->widget(0);
        }

        connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(on_tabWidget_currentChanged(int)));
    }

    QFileInfo fileInfo(newTab->filePath());

    int index = m_tabWidget->addTab(newTab, fileInfo.completeBaseName());
    m_tabWidget->setTabToolTip(index, fileInfo.absoluteFilePath());
    m_tabWidget->setCurrentIndex(index);
}

void MainWindow::on_tab_triggered()
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
    textBrowser->setSource(QUrl(QString("%1/help.html").arg(DATA_INSTALL_PATH)));

    QDialogButtonBox* dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, dialog);
    connect(dialogButtonBox, SIGNAL(accepted()), dialog, SLOT(accept()));
    connect(dialogButtonBox, SIGNAL(rejected()), dialog, SLOT(reject()));

    dialog->setLayout(new QVBoxLayout());
    dialog->layout()->addWidget(textBrowser);
    dialog->layout()->addWidget(dialogButtonBox);

    dialog->resize(m_settings->value("mainWindow/contentsDialogSize", dialog->sizeHint()).toSize());
    dialog->exec();
    m_settings->setValue("mainWindow/contentsDialogSize", dialog->size());

    delete dialog;
}

void MainWindow::on_about_triggered()
{
    QMessageBox::about(this, tr("About qpdfview"), tr("<p><b>qpdfview %1</b></p><p>qpdfview is a tabbed PDF viewer using the poppler library. See <a href=\"https://launchpad.net/qpdfview\">launchpad.net/qpdfview</a> for more information.</p><p>&copy; 2012 Adam Reichold</p>").arg(QApplication::applicationVersion()));
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

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveTabs();
    saveBookmarks();

    removeToolBar(m_searchToolBar);

    if(m_settings->value("mainWindow/trackRecentlyUsed", false).toBool())
    {
        m_settings->setValue("mainWindow/recentlyUsed", m_recentlyUsedMenu->filePaths());
    }
    else
    {
        m_settings->remove("mainWindow/recentlyUsed");
    }

    m_settings->setValue("mainWindow/geometry", m_fullscreenAction->isChecked() ? m_fullscreenAction->data() : saveGeometry());
    m_settings->setValue("mainWindow/state", saveState());

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
#endif
            {
                openInNewTab(url.toLocalFile());
            }
        }

        connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(on_tabWidget_currentChanged(int)));

        on_tabWidget_currentChanged(m_tabWidget->currentIndex());
    }
}

void MainWindow::restoreSettings()
{
    PageItem::setCacheSize(m_settings->value("pageItem/cacheSize", 32 * 1024 * 1024).toInt());

    PageItem::setDecoratePages(m_settings->value("pageItem/decoratePages", true).toBool());
    PageItem::setDecorateLinks(m_settings->value("pageItem/decorateLinks", true).toBool());

    PageItem::setInvertColors(m_settings->value("pageItem/invertColors", false).toBool());

    PageItem::setCopyModifiers(static_cast< Qt::KeyboardModifier >(m_settings->value("pageItem/copyModifiers", 0x02000000).toInt()));
    PageItem::setAnnotateModifiers(static_cast< Qt::KeyboardModifier >(m_settings->value("pageItem/annotateModifiers", 0x04000000).toInt()));

    DocumentView::setOpenUrl(m_settings->value("documentView/openUrl", false).toBool());

    DocumentView::setAutoRefresh(m_settings->value("documentView/autoRefresh", false).toBool());

    DocumentView::setAntialiasing(m_settings->value("documentView/antialiasing", true).toBool());
    DocumentView::setTextAntialiasing(m_settings->value("documentView/textAntialiasing", true).toBool());
    DocumentView::setTextHinting(m_settings->value("documentView/textHinting", false).toBool());

    DocumentView::setPrefetch(m_settings->value("documentView/prefetch").toBool());

    DocumentView::setPageSpacing(m_settings->value("documentView/pageSpacing", 5.0).toReal());
    DocumentView::setThumbnailSpacing(m_settings->value("documentView/thumbnailSpacing", 3.0).toReal());

    DocumentView::setThumbnailSize(m_settings->value("documentView/thumbnailSize", 150.0).toReal());

    DocumentView::setZoomModifiers(static_cast< Qt::KeyboardModifier >(m_settings->value("documentView/zoomModifiers", 0x04000000).toInt()));
    DocumentView::setRotateModifiers(static_cast< Qt::KeyboardModifier >(m_settings->value("documentView/rotateModifiers", 0x02000000).toInt()));
    DocumentView::setHorizontalModifiers(static_cast< Qt::KeyboardModifier >(m_settings->value("documentView/horizontalModifiers", 0x08000000).toInt()));

    DocumentView::setHighlightDuration(m_settings->value("documentView/highlightDuration", 5000).toInt());

    DocumentView::setSynchronizeProgram(m_settings->value("documentView/synchronizeProgram").toString());
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

BookmarkMenu *MainWindow::bookmarkForCurrentTab() const
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

    m_tabWidget->setTabPosition(static_cast< QTabWidget::TabPosition >(m_settings->value("mainWindow/tabPosition", 0).toUInt()));
    m_tabWidget->setTabBarPolicy(static_cast< TabWidget::TabBarPolicy >(m_settings->value("mainWindow/tabVisibility", 0).toUInt()));

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
    connect(m_searchProgressLineEdit, SIGNAL(returnPressed()), SLOT(on_search_timeout()));
    connect(m_searchTimer, SIGNAL(timeout()), SLOT(on_search_timeout()));

    m_matchCaseCheckBox = new QCheckBox(tr("Match &case"), this);
    m_highlightAllCheckBox = new QCheckBox(tr("Hightlight &all"), this);

    connect(m_highlightAllCheckBox, SIGNAL(clicked(bool)), SLOT(on_highlightAll_clicked(bool)));
}

void MainWindow::createActions()
{
    // open

    m_openAction = new QAction(tr("&Open..."), this);
    m_openAction->setShortcut(QKeySequence::Open);
    m_openAction->setIcon(QIcon::fromTheme("document-open", QIcon(":icons/document-open.svg")));
    m_openAction->setIconVisibleInMenu(true);
    connect(m_openAction, SIGNAL(triggered()), SLOT(on_open_triggered()));

    // open in new tab

    m_openInNewTabAction = new QAction(tr("Open in new &tab..."), this);
    m_openInNewTabAction->setShortcut(QKeySequence::AddTab);
    m_openInNewTabAction->setIcon(QIcon::fromTheme("tab-new", QIcon(":icons/tab-new.svg")));
    m_openInNewTabAction->setIconVisibleInMenu(true);
    connect(m_openInNewTabAction, SIGNAL(triggered()), SLOT(on_openInNewTab_triggered()));

    // refresh

    m_refreshAction = new QAction(tr("&Refresh"), this);
    m_refreshAction->setShortcut(QKeySequence::Refresh);
    m_refreshAction->setIcon(QIcon::fromTheme("view-refresh", QIcon(":icons/view-refresh.svg")));
    m_refreshAction->setIconVisibleInMenu(true);
    connect(m_refreshAction, SIGNAL(triggered()), SLOT(on_refresh_triggered()));

    // save copy

    m_saveCopyAction = new QAction(tr("&Save copy..."), this);
    m_saveCopyAction->setShortcut(QKeySequence::Save);
    m_saveCopyAction->setIcon(QIcon::fromTheme("document-save", QIcon(":icons/document-save.svg")));
    m_saveCopyAction->setIconVisibleInMenu(true);
    connect(m_saveCopyAction, SIGNAL(triggered()), SLOT(on_saveCopy_triggered()));

    // print

    m_printAction = new QAction(tr("&Print..."), this);
    m_printAction->setShortcut(QKeySequence::Print);
    m_printAction->setIcon(QIcon::fromTheme("document-print", QIcon(":icons/document-print.svg")));
    m_printAction->setIconVisibleInMenu(true);
    connect(m_printAction, SIGNAL(triggered()), SLOT(on_print_triggered()));

    // exit

    m_exitAction = new QAction(tr("&Exit"), this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    m_exitAction->setIcon(QIcon::fromTheme("application-exit"));
    m_exitAction->setIconVisibleInMenu(true);
    connect(m_exitAction, SIGNAL(triggered()), SLOT(close()));

    // previous page

    m_previousPageAction = new QAction(tr("&Previous page"), this);
    m_previousPageAction->setShortcut(QKeySequence(Qt::Key_Backspace));
    m_previousPageAction->setIcon(QIcon::fromTheme("go-previous", QIcon(":icons/go-previous.svg")));
    m_previousPageAction->setIconVisibleInMenu(true);
    connect(m_previousPageAction, SIGNAL(triggered()), SLOT(on_previousPage_triggered()));

    // next page

    m_nextPageAction = new QAction(tr("&Next page"), this);
    m_nextPageAction->setShortcut(QKeySequence(Qt::Key_Space));
    m_nextPageAction->setIcon(QIcon::fromTheme("go-next", QIcon(":icons/go-next.svg")));
    m_nextPageAction->setIconVisibleInMenu(true);
    connect(m_nextPageAction, SIGNAL(triggered()), SLOT(on_nextPage_triggered()));

    // first page

    m_firstPageAction = new QAction(tr("&First page"), this);
    m_firstPageAction->setShortcut(QKeySequence(Qt::Key_Home));
    m_firstPageAction->setIcon(QIcon::fromTheme("go-first", QIcon(":icons/go-first.svg")));
    m_firstPageAction->setIconVisibleInMenu(true);
    connect(m_firstPageAction, SIGNAL(triggered()), SLOT(on_firstPage_triggered()));

    // last page

    m_lastPageAction = new QAction(tr("&Last page"), this);
    m_lastPageAction->setShortcut(QKeySequence(Qt::Key_End));
    m_lastPageAction->setIcon(QIcon::fromTheme("go-last", QIcon(":icons/go-last.svg")));
    m_lastPageAction->setIconVisibleInMenu(true);
    connect(m_lastPageAction, SIGNAL(triggered()), SLOT(on_lastPage_triggered()));

    // jump to page

    m_jumpToPageAction = new QAction(tr("&Jump to page..."), this);
    m_jumpToPageAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_J));
    m_jumpToPageAction->setIcon(QIcon::fromTheme("go-jump", QIcon(":icons/go-jump.svg")));
    m_jumpToPageAction->setIconVisibleInMenu(true);
    connect(m_jumpToPageAction, SIGNAL(triggered()), SLOT(on_jumpToPage_triggered()));

    // search

    m_searchAction = new QAction(tr("&Search..."), this);
    m_searchAction->setShortcut(QKeySequence::Find);
    m_searchAction->setIcon(QIcon::fromTheme("edit-find", QIcon(":icons/edit-find.svg")));
    m_searchAction->setIconVisibleInMenu(true);
    connect(m_searchAction, SIGNAL(triggered()), SLOT(on_search_triggered()));

    // find previous

    m_findPreviousAction = new QAction(tr("Find previous"), this);
    m_findPreviousAction->setShortcut(QKeySequence::FindPrevious);
    m_findPreviousAction->setIcon(QIcon::fromTheme("go-up", QIcon(":icons/go-up.svg")));
    m_findPreviousAction->setIconVisibleInMenu(true);
    connect(m_findPreviousAction, SIGNAL(triggered()), SLOT(on_findPrevious_triggered()));

    // find next

    m_findNextAction = new QAction(tr("Find next"), this);
    m_findNextAction->setShortcut(QKeySequence::FindNext);
    m_findNextAction->setIcon(QIcon::fromTheme("go-down", QIcon(":icons/go-down.svg")));
    m_findNextAction->setIconVisibleInMenu(true);
    connect(m_findNextAction, SIGNAL(triggered()), SLOT(on_findNext_triggered()));

    // cancel search

    m_cancelSearchAction = new QAction(tr("Cancel search"), this);
    m_cancelSearchAction->setShortcut(QKeySequence(Qt::Key_Escape));
    m_cancelSearchAction->setIcon(QIcon::fromTheme("process-stop", QIcon(":icons/process-stop.svg")));
    m_cancelSearchAction->setIconVisibleInMenu(true);
    connect(m_cancelSearchAction, SIGNAL(triggered()), SLOT(on_cancelSearch_triggered()));

    // copy to clipboard

    m_copyToClipboardAction = new QAction(tr("&Copy to clipboard"), this);
    m_copyToClipboardAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_C));
    m_copyToClipboardAction->setCheckable(true);
    m_copyToClipboardAction->setIcon(QIcon::fromTheme("edit-copy", QIcon(":icons/edit-copy.svg")));
    connect(m_copyToClipboardAction, SIGNAL(triggered(bool)), SLOT(on_copyToClipboard_triggered(bool)));

    // add annotation

    m_addAnnotationAction = new QAction(tr("&Add annotation"), this);
    m_addAnnotationAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_A));
    m_addAnnotationAction->setCheckable(true);
    m_addAnnotationAction->setIcon(QIcon::fromTheme("mail-attachment", QIcon(":icons/mail-attachment.svg")));
    connect(m_addAnnotationAction, SIGNAL(triggered(bool)), SLOT(on_addAnnotation_triggered(bool)));

    // settings

    m_settingsAction = new QAction(tr("Settings..."), this);
    connect(m_settingsAction, SIGNAL(triggered()), SLOT(on_settings_triggered()));

    // continuous mode

    m_continuousModeAction = new QAction(tr("&Continuous"), this);
    m_continuousModeAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_7));
    m_continuousModeAction->setCheckable(true);
    m_continuousModeAction->setIcon(QIcon(":icons/continuous.svg"));
    connect(m_continuousModeAction, SIGNAL(triggered(bool)), SLOT(on_continuousMode_triggered(bool)));

    // two pages mode

    m_twoPagesModeAction = new QAction(tr("&Two pages"), this);
    m_twoPagesModeAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_6));
    m_twoPagesModeAction->setCheckable(true);
    m_twoPagesModeAction->setIcon(QIcon(":icons/two-pages.svg"));
    connect(m_twoPagesModeAction, SIGNAL(triggered(bool)), SLOT(on_twoPagesMode_triggered(bool)));

    // two pages with cover page mode

    m_twoPagesWithCoverPageModeAction = new QAction(tr("Two pages &with cover page"), this);
    m_twoPagesWithCoverPageModeAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_5));
    m_twoPagesWithCoverPageModeAction->setCheckable(true);
    m_twoPagesWithCoverPageModeAction->setIcon(QIcon(":icons/two-pages-with-cover-page.svg"));
    connect(m_twoPagesWithCoverPageModeAction, SIGNAL(triggered(bool)), SLOT(on_twoPagesWithCoverPageMode_triggered(bool)));

    // zoom in

    m_zoomInAction = new QAction(tr("Zoom &in"), this);
    m_zoomInAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Up));
    m_zoomInAction->setIcon(QIcon::fromTheme("zoom-in", QIcon(":icons/zoom-in.svg")));
    m_zoomInAction->setIconVisibleInMenu(true);
    connect(m_zoomInAction, SIGNAL(triggered()), SLOT(on_zoomIn_triggered()));

    // zoom out

    m_zoomOutAction = new QAction(tr("Zoom &out"), this);
    m_zoomOutAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Down));
    m_zoomOutAction->setIcon(QIcon::fromTheme("zoom-out", QIcon(":icons/zoom-out.svg")));
    m_zoomOutAction->setIconVisibleInMenu(true);
    connect(m_zoomOutAction, SIGNAL(triggered()), SLOT(on_zoomOut_triggered()));

    // original size

    m_originalSizeAction = new QAction(tr("Original &size"), this);
    m_originalSizeAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_0));
    m_originalSizeAction->setIcon(QIcon::fromTheme("zoom-original", QIcon(":icons/zoom-original.svg")));
    m_originalSizeAction->setIconVisibleInMenu(true);
    connect(m_originalSizeAction, SIGNAL(triggered()), SLOT(on_originalSize_triggered()));

    // fit to page width

    m_fitToPageWidthAction = new QAction(tr("Fit to page width"), this);
    m_fitToPageWidthAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_9));
    m_fitToPageWidthAction->setCheckable(true);
    m_fitToPageWidthAction->setIcon(QIcon(":icons/fit-to-page-width.svg"));
    connect(m_fitToPageWidthAction, SIGNAL(triggered(bool)), SLOT(on_fitToPageWidth_triggered(bool)));

    // fit to page size

    m_fitToPageSizeAction = new QAction(tr("Fit to page size"), this);
    m_fitToPageSizeAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_8));
    m_fitToPageSizeAction->setCheckable(true);
    m_fitToPageSizeAction->setIcon(QIcon(":icons/fit-to-page-size.svg"));
    connect(m_fitToPageSizeAction, SIGNAL(triggered(bool)), SLOT(on_fitToPageSize_triggered(bool)));

    // rotate left

    m_rotateLeftAction = new QAction(tr("Rotate &left"), this);
    m_rotateLeftAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Left));
    m_rotateLeftAction->setIcon(QIcon::fromTheme("object-rotate-left", QIcon(":icons/object-rotate-left.svg")));
    m_rotateLeftAction->setIconVisibleInMenu(true);
    connect(m_rotateLeftAction, SIGNAL(triggered()), SLOT(on_rotateLeft_triggered()));

    // rotate right

    m_rotateRightAction = new QAction(tr("Rotate &right"), this);
    m_rotateRightAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Right));
    m_rotateRightAction->setIcon(QIcon::fromTheme("object-rotate-right", QIcon(":icons/object-rotate-right.svg")));
    m_rotateRightAction->setIconVisibleInMenu(true);
    connect(m_rotateRightAction, SIGNAL(triggered()), SLOT(on_rotateRight_triggered()));

    // fonts

    m_fontsAction = new QAction(tr("Fonts..."), this);
    connect(m_fontsAction, SIGNAL(triggered()), SLOT(on_fonts_triggered()));

    // fullscreen

    m_fullscreenAction = new QAction(tr("&Fullscreen"), this);
    m_fullscreenAction->setCheckable(true);
    m_fullscreenAction->setShortcut(QKeySequence(Qt::Key_F11));
    m_fullscreenAction->setIcon(QIcon::fromTheme("view-fullscreen", QIcon(":icons/view-fullscreen.svg")));
    connect(m_fullscreenAction, SIGNAL(triggered(bool)), SLOT(on_fullscreen_triggered(bool)));

    // presentation

    m_presentationAction = new QAction(tr("&Presentation..."), this);
    m_presentationAction->setShortcut(QKeySequence(Qt::Key_F12));
    m_presentationAction->setIcon(QIcon::fromTheme("x-office-presentation", QIcon(":icons/x-office-presentation.svg")));
    m_presentationAction->setIconVisibleInMenu(true);
    connect(m_presentationAction, SIGNAL(triggered()), SLOT(on_presentation_triggered()));

    // previous tab

    m_previousTabAction = new QAction(tr("&Previous tab"), this);
    m_previousTabAction->setShortcut(QKeySequence::PreviousChild);
    connect(m_previousTabAction, SIGNAL(triggered()), SLOT(on_previousTab_triggered()));

    // next tab

    m_nextTabAction = new QAction(tr("&Next tab"), this);
    m_nextTabAction->setShortcut(QKeySequence::NextChild);
    connect(m_nextTabAction, SIGNAL(triggered()), SLOT(on_nextTab_triggered()));

    // close tab

    m_closeTabAction = new QAction(tr("&Close tab"), this);
    m_closeTabAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_W));
    m_closeTabAction->setIcon(QIcon::fromTheme("window-close"));
    m_closeTabAction->setIconVisibleInMenu(true);
    connect(m_closeTabAction, SIGNAL(triggered()), SLOT(on_closeTab_triggered()));

    // close all tabs

    m_closeAllTabsAction = new QAction(tr("Close &all tabs"), this);
    m_closeAllTabsAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_W));
    connect(m_closeAllTabsAction, SIGNAL(triggered()), SLOT(on_closeAllTabs_triggered()));

    // close all tabs but current tab

    m_closeAllTabsButCurrentTabAction = new QAction(tr("Close all tabs &but current tab"), this);
    m_closeAllTabsButCurrentTabAction->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_W));
    connect(m_closeAllTabsButCurrentTabAction, SIGNAL(triggered()), SLOT(on_closeAllTabsButCurrentTab_triggered()));

    // previous bookmark

    m_previousBookmarkAction = new QAction(tr("&Previous bookmark"), this);
    m_previousBookmarkAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_PageUp));
    connect(m_previousBookmarkAction, SIGNAL(triggered()), SLOT(on_previousBookmark_triggered()));

    // next bookmark

    m_nextBookmarkAction = new QAction(tr("&Next bookmark"), this);
    m_nextBookmarkAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_PageDown));
    connect(m_nextBookmarkAction, SIGNAL(triggered()), SLOT(on_nextBookmark_triggered()));

    // add bookmark

    m_addBookmarkAction = new QAction(tr("&Add bookmark"), this);
    m_addBookmarkAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_B));
    connect(m_addBookmarkAction, SIGNAL(triggered()), SLOT(on_addBookmark_triggered()));

    // remove bookmark

    m_removeBookmarkAction = new QAction(tr("&Remove bookmark"), this);
    m_removeBookmarkAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_B));
    connect(m_removeBookmarkAction, SIGNAL(triggered()), SLOT(on_removeBookmark_triggered()));

    // remove all bookmarks

    m_removeAllBookmarksAction = new QAction(tr("Remove all bookmarks"), this);
    m_removeAllBookmarksAction->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_B));
    connect(m_removeAllBookmarksAction, SIGNAL(triggered()), SLOT(on_removeAllBookmarks_triggered()));

    // contents

    m_contentsAction = new QAction(tr("&Contents"), this);
    m_contentsAction->setShortcut(QKeySequence::HelpContents);
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
    m_fileToolBar->setObjectName("fileToolBar");

    foreach(QString action, m_settings->value("mainWindow/fileToolBar", QStringList() << "openInNewTab" << "refresh").toStringList())
    {
        if(action == "open") { m_fileToolBar->addAction(m_openAction); }
        else if(action == "openInNewTab") { m_fileToolBar->addAction(m_openInNewTabAction); }
        else if(action == "refresh") { m_fileToolBar->addAction(m_refreshAction); }
        else if(action == "saveCopy") { m_fileToolBar->addAction(m_saveCopyAction); }
        else if(action == "print") { m_fileToolBar->addAction(m_printAction); }
    }

    // edit

    m_editToolBar = addToolBar(tr("&Edit"));
    m_editToolBar->setObjectName("editToolBar");

    foreach(QString action, m_settings->value("mainWindow/editToolBar", QStringList() << "currentPage" << "previousPage" << "nextPage").toStringList())
    {
        if(action == "currentPage") { m_currentPageSpinBox->setVisible(true); m_editToolBar->addWidget(m_currentPageSpinBox); }
        else if(action == "previousPage") { m_editToolBar->addAction(m_previousPageAction); }
        else if(action == "nextPage") { m_editToolBar->addAction(m_nextPageAction); }
        else if(action == "firstPage") { m_editToolBar->addAction(m_firstPageAction); }
        else if(action == "lastPage") { m_editToolBar->addAction(m_lastPageAction); }
        else if(action == "jumpToPage") { m_editToolBar->addAction(m_jumpToPageAction); }
        else if(action == "search") { m_editToolBar->addAction(m_searchAction); }
        else if(action == "copyToClipboard") { m_editToolBar->addAction(m_copyToClipboardAction); }
        else if(action == "addAnnotation") { m_editToolBar->addAction(m_addAnnotationAction); }
    }

    // view

    m_viewToolBar = addToolBar(tr("&View"));
    m_viewToolBar->setObjectName("viewToolBar");

    foreach(QString action, m_settings->value("mainWindow/viewToolBar", QStringList() << "scaleFactor" << "zoomIn" << "zoomOut").toStringList())
    {
        if(action == "continuousMode") { m_viewToolBar->addAction(m_continuousModeAction); }
        else if(action == "twoPagesMode") { m_viewToolBar->addAction(m_twoPagesModeAction); }
        else if(action == "twoPagesWithCoverPageMode") { m_viewToolBar->addAction(m_twoPagesWithCoverPageModeAction); }
        else if(action == "scaleFactor") { m_scaleFactorComboBox->setVisible(true); m_viewToolBar->addWidget(m_scaleFactorComboBox); }
        else if(action == "zoomIn") { m_viewToolBar->addAction(m_zoomInAction); }
        else if(action == "zoomOut") { m_viewToolBar->addAction(m_zoomOutAction); }
        else if(action == "originalSize") { m_viewToolBar->addAction(m_originalSizeAction); }
        else if(action == "fitToPageWidth") { m_viewToolBar->addAction(m_fitToPageWidthAction); }
        else if(action == "fitToPageSize") { m_viewToolBar->addAction(m_fitToPageSizeAction); }
        else if(action == "rotateLeft") { m_viewToolBar->addAction(m_rotateLeftAction); }
        else if(action == "rotateRight") { m_viewToolBar->addAction(m_rotateRightAction); }
        else if(action == "fullscreen") { m_viewToolBar->addAction(m_fullscreenAction); }
        else if(action == "presentation") { m_viewToolBar->addAction(m_presentationAction); }
    }

    // search

    m_searchToolBar = new QToolBar(tr("&Search"), this);
    m_searchToolBar->setObjectName("searchToolBar");

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
    m_outlineDock->setObjectName("outlineDock");
    m_outlineDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_outlineDock->setFeatures(QDockWidget::AllDockWidgetFeatures);

    addDockWidget(Qt::LeftDockWidgetArea, m_outlineDock);

    m_outlineDock->toggleViewAction()->setShortcut(QKeySequence(Qt::Key_F6));
    m_outlineDock->hide();

    m_outlineView = new QTreeView(this);
    m_outlineView->setAlternatingRowColors(true);
    m_outlineView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_outlineView->header()->setVisible(false);
    m_outlineView->header()->setResizeMode(QHeaderView::Stretch);

    connect(m_outlineView, SIGNAL(clicked(QModelIndex)), SLOT(on_outline_clicked(QModelIndex)));

    m_outlineDock->setWidget(m_outlineView);

    // properties

    m_propertiesDock = new QDockWidget(tr("&Properties"), this);
    m_propertiesDock->setObjectName("propertiesDock");
    m_propertiesDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_propertiesDock->setFeatures(QDockWidget::AllDockWidgetFeatures);

    addDockWidget(Qt::LeftDockWidgetArea, m_propertiesDock);

    m_propertiesDock->toggleViewAction()->setShortcut(QKeySequence(Qt::Key_F7));
    m_propertiesDock->hide();

    m_propertiesView = new QTableView(this);
    m_propertiesView->setAlternatingRowColors(true);
    m_propertiesView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_propertiesView->horizontalHeader()->setVisible(false);
    m_propertiesView->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
    m_propertiesView->verticalHeader()->setVisible(false);
    m_propertiesView->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);

    m_propertiesDock->setWidget(m_propertiesView);

    // thumbnails

    m_thumbnailsDock = new QDockWidget(tr("&Thumbnails"), this);
    m_thumbnailsDock->setObjectName("thumbnailsDock");
    m_thumbnailsDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_thumbnailsDock->setFeatures(QDockWidget::AllDockWidgetFeatures);

    addDockWidget(Qt::RightDockWidgetArea, m_thumbnailsDock);

    m_thumbnailsDock->toggleViewAction()->setShortcut(QKeySequence(Qt::Key_F8));
    m_thumbnailsDock->hide();

    m_thumbnailsView = new QGraphicsView(this);

    m_thumbnailsDock->setWidget(m_thumbnailsView);
}

void MainWindow::createMenus()
{
    // file

    m_fileMenu = menuBar()->addMenu(tr("&File"));
    m_fileMenu->addAction(m_openAction);
    m_fileMenu->addAction(m_openInNewTabAction);

    m_recentlyUsedMenu = new RecentlyUsedMenu(this);

    if(m_settings->value("mainWindow/trackRecentlyUsed", false).toBool())
    {
        foreach(QString filePath, m_settings->value("mainWindow/recentlyUsed").toStringList())
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
    m_editMenu->addAction(m_copyToClipboardAction);
    m_editMenu->addAction(m_addAnnotationAction);
    m_editMenu->addSeparator();
    m_editMenu->addAction(m_settingsAction);

    // view

    m_viewMenu = menuBar()->addMenu(tr("&View"));
    m_viewMenu->addAction(m_continuousModeAction);
    m_viewMenu->addAction(m_twoPagesModeAction);
    m_viewMenu->addAction(m_twoPagesWithCoverPageModeAction);
    m_viewMenu->addSeparator();
    m_viewMenu->addAction(m_zoomInAction);
    m_viewMenu->addAction(m_zoomOutAction);
    m_viewMenu->addAction(m_originalSizeAction);
    m_viewMenu->addAction(m_fitToPageWidthAction);
    m_viewMenu->addAction(m_fitToPageSizeAction);
    m_viewMenu->addSeparator();
    m_viewMenu->addAction(m_rotateLeftAction);
    m_viewMenu->addAction(m_rotateRightAction);
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

void MainWindow::restoreTabs()
{
    if(m_settings->value("mainWindow/restoreTabs", false).toBool())
    {
        QFile file(QFileInfo(QDir(QFileInfo(m_settings->fileName()).path()), "tabs.xml").filePath());

        if(file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QDomDocument document;

            if(document.setContent(&file))
            {
                disconnect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(on_tabWidget_currentChanged(int)));

                QDomElement rootElement = document.firstChildElement();
                QDomElement tabElement = rootElement.firstChildElement();

                while(!tabElement.isNull())
                {
                    if(openInNewTab(tabElement.attribute("filePath")))
                    {
                        currentTab()->setContinousMode(static_cast< bool >(tabElement.attribute("continuousMode").toUInt()));
                        currentTab()->setLayoutMode(static_cast< DocumentView::LayoutMode >(tabElement.attribute("layoutMode").toUInt()));

                        currentTab()->setScaleMode(static_cast< DocumentView::ScaleMode >(tabElement.attribute("scaleMode").toUInt()));
                        currentTab()->setScaleFactor(tabElement.attribute("scaleFactor").toFloat());

                        currentTab()->setRotation(static_cast< Poppler::Page::Rotation >(tabElement.attribute("rotation").toUInt()));

                        currentTab()->jumpToPage(tabElement.attribute("currentPage").toInt());
                    }

                    tabElement = tabElement.nextSiblingElement();
                }

                m_tabWidget->setCurrentIndex(rootElement.attribute("currentIndex").toInt());

                connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(on_tabWidget_currentChanged(int)));
            }

            file.close();
        }
    }
}

void MainWindow::saveTabs()
{
    QFile file(QFileInfo(QDir(QFileInfo(m_settings->fileName()).path()), "tabs.xml").filePath());

    if(m_settings->value("mainWindow/restoreTabs", false).toBool())
    {
        if(file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QDomDocument document;

            QDomElement rootElement = document.createElement("tabs");
            document.appendChild(rootElement);

            rootElement.setAttribute("currentIndex", m_tabWidget->currentIndex());

            for(int index = 0; index < m_tabWidget->count(); ++index)
            {
                QDomElement tabElement = document.createElement("tab");
                rootElement.appendChild(tabElement);

                tabElement.setAttribute("filePath", QFileInfo(tab(index)->filePath()).absoluteFilePath());
                tabElement.setAttribute("currentPage", tab(index)->currentPage());

                tabElement.setAttribute("continuousMode", static_cast< uint >(tab(index)->continousMode()));
                tabElement.setAttribute("layoutMode", static_cast< uint >(tab(index)->layoutMode()));

                tabElement.setAttribute("scaleMode", static_cast< uint >(tab(index)->scaleMode()));
                tabElement.setAttribute("scaleFactor", tab(index)->scaleFactor());

                tabElement.setAttribute("rotation", static_cast< uint >(tab(index)->rotation()));
            }

            QTextStream textStream(&file);
            document.save(textStream, 4);

            file.close();
        }
    }
    else
    {
        file.remove();
    }
}

void MainWindow::restoreBookmarks()
{
    if(m_settings->value("mainWindow/restoreBookmarks", false).toBool())
    {
        QFile file(QFileInfo(QDir(QFileInfo(m_settings->fileName()).path()), "bookmarks.xml").filePath());

        if(file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QDomDocument document;

            if(document.setContent(&file))
            {
                QDomElement rootElement = document.firstChildElement();
                QDomElement bookmarkElement = rootElement.firstChildElement();

                while(!bookmarkElement.isNull())
                {
                    BookmarkMenu* bookmark = new BookmarkMenu(bookmarkElement.attribute("filePath"));

                    QDomElement jumpToPageElement = bookmarkElement.firstChildElement();

                    while(!jumpToPageElement.isNull())
                    {
                        bookmark->addJumpToPageAction(jumpToPageElement.attribute("page").toInt());

                        jumpToPageElement = jumpToPageElement.nextSiblingElement();
                    }

                    connect(bookmark, SIGNAL(openTriggered(QString)), SLOT(on_bookmark_openTriggered(QString)));
                    connect(bookmark, SIGNAL(openInNewTabTriggered(QString)), SLOT(on_bookmark_openInNewTabTriggered(QString)));
                    connect(bookmark, SIGNAL(jumpToPageTriggered(QString,int)), SLOT(on_bookmark_jumpToPageTriggered(QString,int)));

                    m_bookmarksMenu->addMenu(bookmark);

                    bookmarkElement = bookmarkElement.nextSiblingElement();
                }
            }

            file.close();
        }
    }
}

void MainWindow::saveBookmarks()
{
    QFile file(QFileInfo(QDir(QFileInfo(m_settings->fileName()).path()), "bookmarks.xml").filePath());

    if(m_settings->value("mainWindow/restoreBookmarks", false).toBool())
    {
        if(file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QDomDocument document;

            QDomElement rootElement = document.createElement("bookmarks");
            document.appendChild(rootElement);

            foreach(QAction* action, m_bookmarksMenu->actions())
            {
                BookmarkMenu* bookmark = qobject_cast< BookmarkMenu* >(action->menu());

                if(bookmark != 0)
                {
                    QDomElement bookmarkElement = document.createElement("bookmark");
                    rootElement.appendChild(bookmarkElement);

                    bookmarkElement.setAttribute("filePath", QFileInfo(bookmark->filePath()).absoluteFilePath());

                    foreach(int page, bookmark->pages())
                    {
                        QDomElement jumpToPageElement = document.createElement("jumpToPage");
                        bookmarkElement.appendChild(jumpToPageElement);

                        jumpToPageElement.setAttribute("page", page);                        
                    }
                }
            }

            QTextStream textStream(&file);
            document.save(textStream, 4);

            file.close();
        }
    }
    else
    {
        file.remove();
    }
}

#ifdef WITH_DBUS

MainWindowAdaptor::MainWindowAdaptor(MainWindow* mainWindow) : QDBusAbstractAdaptor(mainWindow)
{
}

MainWindow* MainWindowAdaptor::mainWindow() const
{
    return qobject_cast< MainWindow* >(parent());
}

bool MainWindowAdaptor::open(const QString& filePath, int page, const QRectF &highlight)
{
    return mainWindow()->open(filePath, page, highlight);
}

bool MainWindowAdaptor::openInNewTab(const QString& filePath, int page, const QRectF &highlight)
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

# endif // WITH_DBUS
