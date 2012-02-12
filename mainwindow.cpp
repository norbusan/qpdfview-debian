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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_openAction = new QAction(QIcon::fromTheme("document-open"), tr("&Open..."), this);
    m_openAction->setShortcut(QKeySequence::Open);
    connect(m_openAction, SIGNAL(triggered()), this, SLOT(open()));
    m_addTabAction = new QAction(QIcon::fromTheme("window-new"), tr("&Add tab..."), this);
    m_addTabAction->setShortcut(QKeySequence::AddTab);
    connect(m_addTabAction, SIGNAL(triggered()), this, SLOT(addTab()));
    m_previousTabAction = new QAction(tr("Previous tab"), this);
    m_previousTabAction->setShortcut(QKeySequence::PreviousChild);
    connect(m_previousTabAction, SIGNAL(triggered()), this, SLOT(previousTab()));
    m_nextTabAction = new QAction(tr("Next tab"), this);
    m_nextTabAction->setShortcut(QKeySequence::NextChild);
    connect(m_nextTabAction, SIGNAL(triggered()), this, SLOT(nextTab()));
    m_closeTabAction = new QAction(QIcon::fromTheme("window-close"), tr("&Close Tab"), this);
    m_closeTabAction->setShortcut(QKeySequence::Close);
    connect(m_closeTabAction, SIGNAL(triggered()), this, SLOT(closeTab()));

    m_reloadAction = new QAction(QIcon::fromTheme("view-refresh"), tr("&Reload"), this);
    m_reloadAction->setShortcut(QKeySequence::Refresh);
    connect(m_reloadAction, SIGNAL(triggered()), this, SLOT(reload()));
    m_saveAction = new QAction(QIcon::fromTheme("document-save"), tr("&Save..."), this);
    m_saveAction->setShortcut(QKeySequence::Save);
    connect(m_saveAction, SIGNAL(triggered()), this, SLOT(save()));
    m_printAction = new QAction(QIcon::fromTheme("document-print"), tr("&Print..."), this);
    m_printAction->setShortcut(QKeySequence::Print);
    connect(m_printAction, SIGNAL(triggered()), this, SLOT(print()));

    m_exitAction = new QAction(QIcon::fromTheme("application-exit"), tr("&Exit"), this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    connect(m_exitAction, SIGNAL(triggered()), this, SLOT(close()));

    m_pageLineEdit = new QLineEdit();

    m_previousPageAction = new QAction(QIcon::fromTheme("go-previous"), tr("&Previous page"), this);
    m_previousPageAction->setShortcut(QKeySequence::MoveToPreviousPage);
    connect(m_previousPageAction, SIGNAL(triggered()), this, SLOT(previousPage()));
    m_nextPageAction = new QAction(QIcon::fromTheme("go-next"), tr("&Next page"), this);
    m_nextPageAction->setShortcut(QKeySequence::MoveToNextPage);
    connect(m_nextPageAction, SIGNAL(triggered()), this, SLOT(nextPage()));
    m_firstPageAction = new QAction(QIcon::fromTheme("go-first"), tr("&First page"), this);
    m_firstPageAction->setShortcut(QKeySequence::MoveToStartOfDocument);
    connect(m_firstPageAction, SIGNAL(triggered()), this, SLOT(firstPage()));
    m_lastPageAction = new QAction(QIcon::fromTheme("go-last"), tr("&Last page"), this);
    m_lastPageAction->setShortcut(QKeySequence::MoveToEndOfDocument);
    connect(m_lastPageAction, SIGNAL(triggered()), this, SLOT(lastPage()));

    m_pagingAction = new QAction(tr("Paging display"), this);
    m_pagingAction->setCheckable(true);
    m_scrollingAction = new QAction(tr("Scrolling display"), this);
    m_scrollingAction->setCheckable(true);
    m_doublePagingAction = new QAction(tr("Double paging display"), this);
    m_doublePagingAction->setCheckable(true);
    m_doubleScrollingAction = new QAction(tr("Double scrolling display"), this);
    m_doubleScrollingAction->setCheckable(true);

    m_displayModeGroup = new QActionGroup(this);
    m_displayModeGroup->addAction(m_pagingAction);
    m_displayModeGroup->addAction(m_scrollingAction);
    m_displayModeGroup->addAction(m_doublePagingAction);
    m_displayModeGroup->addAction(m_doubleScrollingAction);
    connect(m_displayModeGroup, SIGNAL(selected(QAction*)), this, SLOT(selectDisplayMode(QAction*)));

    m_scaleFactorAction = new QAction(tr("Use scale factor"), this);
    m_scaleFactorAction->setCheckable(true);
    m_fitToPageAction = new QAction(tr("Fit to page"), this);
    m_fitToPageAction->setCheckable(true);
    m_fitToPageWidthAction = new QAction(tr("Fit to page width"), this);
    m_fitToPageWidthAction->setCheckable(true);

    m_scaleModeGroup = new QActionGroup(this);
    m_scaleModeGroup->addAction(m_scaleFactorAction);
    m_scaleModeGroup->addAction(m_fitToPageAction);
    m_scaleModeGroup->addAction(m_fitToPageWidthAction);
    connect(m_scaleModeGroup, SIGNAL(selected(QAction*)), this, SLOT(selectScaleMode(QAction*)));

    m_doNotRotateAction = new QAction(tr("Do not rotate"), this);
    m_doNotRotateAction->setCheckable(true);
    m_rotateBy90Action = new QAction(tr("Rotate by 90 degrees"), this);
    m_rotateBy90Action->setCheckable(true);
    m_rotateBy180Action = new QAction(tr("Rotate by 180 degrees"), this);
    m_rotateBy180Action->setCheckable(true);
    m_rotateBy270Action = new QAction(tr("Rotate by 270 degrees"), this);
    m_rotateBy270Action->setCheckable(true);

    m_rotationModeGroup = new QActionGroup(this);
    m_rotationModeGroup->addAction(m_doNotRotateAction);
    m_rotationModeGroup->addAction(m_rotateBy90Action);
    m_rotationModeGroup->addAction(m_rotateBy180Action);
    m_rotationModeGroup->addAction(m_rotateBy270Action);
    connect(m_rotationModeGroup, SIGNAL(selected(QAction*)), this, SLOT(selectRotationMode(QAction*)));

    m_fullscreenAction = new QAction(QIcon::fromTheme("view-fullscreen"), tr("Fullscreen"), this);
    m_fullscreenAction->setShortcut(QKeySequence(Qt::Key_F11));
    m_fullscreenAction->setCheckable(true);
    connect(m_fullscreenAction, SIGNAL(triggered()), this, SLOT(fullscreen()));

    // menuBar

    QMenu *fileMenu = this->menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(m_openAction);
    fileMenu->addAction(m_addTabAction);
    fileMenu->addAction(m_previousTabAction);
    fileMenu->addAction(m_nextTabAction);
    fileMenu->addAction(m_closeTabAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_reloadAction);
    fileMenu->addAction(m_saveAction);
    fileMenu->addAction(m_printAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_exitAction);

    QMenu *viewMenu = this->menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(m_previousPageAction);
    viewMenu->addAction(m_nextPageAction);
    viewMenu->addAction(m_firstPageAction);
    viewMenu->addAction(m_lastPageAction);
    viewMenu->addSeparator();
    viewMenu->addAction(m_pagingAction);
    viewMenu->addAction(m_scrollingAction);
    viewMenu->addAction(m_doublePagingAction);
    viewMenu->addAction(m_doubleScrollingAction);
    viewMenu->addSeparator();
    viewMenu->addAction(m_scaleFactorAction);
    viewMenu->addAction(m_fitToPageAction);
    viewMenu->addAction(m_fitToPageWidthAction);
    viewMenu->addSeparator();
    viewMenu->addAction(m_doNotRotateAction);
    viewMenu->addAction(m_rotateBy90Action);
    viewMenu->addAction(m_rotateBy180Action);
    viewMenu->addAction(m_rotateBy270Action);
    viewMenu->addSeparator();
    viewMenu->addAction(m_fullscreenAction);

    // toolBar

    QToolBar *fileToolBar = this->addToolBar(tr("&File"));
    fileToolBar->setObjectName("FileToolBar");
    fileToolBar->addAction(m_openAction);
    fileToolBar->addSeparator();
    fileToolBar->addAction(m_reloadAction);
    fileToolBar->addAction(m_saveAction);
    fileToolBar->addAction(m_printAction);

    QToolBar *viewToolBar = this->addToolBar(tr("&View"));
    viewToolBar->setObjectName("ViewToolBar");
    viewToolBar->addAction(m_firstPageAction);
    viewToolBar->addAction(m_previousPageAction);
    viewToolBar->addWidget(m_pageLineEdit);
    viewToolBar->addAction(m_nextPageAction);
    viewToolBar->addAction(m_lastPageAction);

    // centralWidget

    m_tabWidget = new QTabWidget();
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setElideMode(Qt::ElideRight);
    connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(changeCurrent(int)));
    connect(m_tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(requestTabClose(int)));

    this->setCentralWidget(m_tabWidget);
    this->changeCurrent(-1);

    // geometry

    QSettings settings;

    this->restoreGeometry(settings.value("MainWindow/geometry").toByteArray());
    this->restoreState(settings.value("MainWindow/state").toByteArray());

    // drag and drop

    this->setAcceptDrops(true);

    // arguments

    QStringList arguments = QCoreApplication::arguments();

    foreach(QString argument, arguments)
    {
        if(QFile(argument).exists()) {
            DocumentView *documentView = new DocumentView();

            if(documentView->load(argument))
            {
                int index = m_tabWidget->addTab(documentView, QFileInfo(argument).baseName());
                m_tabWidget->setTabToolTip(index, QFileInfo(argument).baseName());
                m_tabWidget->setCurrentIndex(index);

                documentView->show();
            }
            else
            {
                delete documentView;
            }
        }
    }
}

MainWindow::~MainWindow()
{
    delete m_openAction;
    delete m_addTabAction;
    delete m_previousTabAction;
    delete m_nextTabAction;
    delete m_closeTabAction;

    delete m_reloadAction;
    delete m_saveAction;
    delete m_printAction;

    delete m_exitAction;

    delete m_previousPageAction;
    delete m_nextPageAction;
    delete m_firstPageAction;
    delete m_lastPageAction;

    delete m_pagingAction;
    delete m_scrollingAction;
    delete m_doublePagingAction;
    delete m_doubleScrollingAction;
    delete m_displayModeGroup;

    delete m_scaleFactorAction;
    delete m_fitToPageAction;
    delete m_fitToPageWidthAction;
    delete m_scaleModeGroup;

    delete m_doNotRotateAction;
    delete m_rotateBy90Action;
    delete m_rotateBy180Action;
    delete m_rotateBy270Action;
    delete m_rotationModeGroup;

    delete m_fullscreenAction;
}

void MainWindow::open()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        QString filePath = QFileDialog::getOpenFileName(this, tr("Open document"), QDir::homePath(), tr("Portable Document Format (*.pdf)"));

        DocumentView *documentView = static_cast<DocumentView*>(m_tabWidget->currentWidget());

        if(documentView->load(filePath))
        {
            m_tabWidget->setTabText(m_tabWidget->currentIndex(), QFileInfo(filePath).baseName());
            m_tabWidget->setTabToolTip(m_tabWidget->currentIndex(), QFileInfo(filePath).baseName());
        }
    }
    else
    {
        this->addTab();
    }
}

void MainWindow::addTab()
{
    QStringList filePaths = QFileDialog::getOpenFileNames(this, tr("Open document"), QDir::homePath(), tr("Portable Document Format (*.pdf)"));

    foreach(QString filePath, filePaths)
    {
        DocumentView *documentView = new DocumentView();

        if(documentView->load(filePath))
        {
            int index = m_tabWidget->addTab(documentView, QFileInfo(filePath).baseName());
            m_tabWidget->setTabToolTip(index, QFileInfo(filePath).baseName());
            m_tabWidget->setCurrentIndex(index);

            documentView->show();
        }
        else
        {
            delete documentView;
        }
    }
}

void MainWindow::previousTab()
{
}

void MainWindow::nextTab()
{
}

void MainWindow::closeTab()
{
}


void MainWindow::reload()
{
}

void MainWindow::save()
{
}

void MainWindow::print()
{
}


void MainWindow::previousPage()
{
}

void MainWindow::nextPage()
{
}

void MainWindow::firstPage()
{
}

void MainWindow::lastPage()
{
}


void MainWindow::selectScaleMode(QAction *scaleModeAction)
{
}

void MainWindow::selectRotationMode(QAction *rotationModeAction)
{
}

void MainWindow::selectDisplayMode(QAction *displayModeAction)
{
}


void MainWindow::fullscreen()
{
    if(m_fullscreenAction->isChecked())
    {
        this->showFullScreen();
    }
    else
    {
        this->showNormal();
    }
}


void MainWindow::changeCurrent(const int &index)
{
    if(index != -1)
    {
    }
    else
    {
        m_previousTabAction->setEnabled(false);
        m_nextTabAction->setEnabled(false);
        m_closeTabAction->setEnabled(false);

        m_reloadAction->setEnabled(false);
        m_saveAction->setEnabled(false);
        m_printAction->setEnabled(false);

        m_pageLineEdit->setEnabled(false);

        m_previousPageAction->setEnabled(false);
        m_nextPageAction->setEnabled(false);
        m_firstPageAction->setEnabled(false);
        m_lastPageAction->setEnabled(false);

        m_pagingAction->setChecked(true);
        m_displayModeGroup->setEnabled(false);

        m_scaleFactorAction->setChecked(true);
        m_scaleModeGroup->setEnabled(false);

        m_doNotRotateAction->setChecked(true);
        m_rotationModeGroup->setEnabled(false);
    }
}

void MainWindow::requestTabClose(const int &index)
{
}


void MainWindow::dropEvent(QDropEvent *dropEvent)
{
}

void MainWindow::closeEvent(QCloseEvent *closeEvent)
{
    QSettings settings;

    if(!m_fullscreenAction->isChecked())
    {
        settings.setValue("MainWindow/geometry", this->saveGeometry());
        settings.setValue("MainWindow/state", this->saveState());
    }

    QMainWindow::closeEvent(closeEvent);
}
