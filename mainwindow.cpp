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
    m_closeTabAction = new QAction(QIcon::fromTheme("window-close"), tr("&Close Tab"), this);
    m_closeTabAction->setShortcut(QKeySequence::Close);
    connect(m_closeTabAction, SIGNAL(triggered()), this, SLOT(closeTab()));

    m_openAction->setEnabled(false);
    m_closeTabAction->setEnabled(false);

    m_reloadAction = new QAction(QIcon::fromTheme("view-refresh"), tr("&Reload"), this);
    m_reloadAction->setShortcut(QKeySequence::Refresh);
    connect(m_reloadAction, SIGNAL(triggered()), this, SLOT(reload()));
    m_saveAction = new QAction(QIcon::fromTheme("document-save"), tr("&Save..."), this);
    m_saveAction->setShortcut(QKeySequence::Save);
    connect(m_saveAction, SIGNAL(triggered()), this, SLOT(save()));
    m_printAction = new QAction(QIcon::fromTheme("document-print"), tr("&Print..."), this);
    m_printAction->setShortcut(QKeySequence::Print);
    connect(m_printAction, SIGNAL(triggered()), this, SLOT(print()));

    m_reloadAction->setEnabled(false);
    m_saveAction->setEnabled(false);
    m_printAction->setEnabled(false);

    m_exitAction = new QAction(QIcon::fromTheme("application-exit"), tr("&Exit"), this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    connect(m_exitAction, SIGNAL(triggered()), this, SLOT(close()));

    m_pageLineEdit = new QLineEdit();
    m_pageLineEdit->setEnabled(false);

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

    m_previousPageAction->setEnabled(false);
    m_nextPageAction->setEnabled(false);
    m_firstPageAction->setEnabled(false);
    m_lastPageAction->setEnabled(false);

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

    m_scaleFactorAction->setChecked(true);
    m_scaleModeGroup->setEnabled(false);

    connect(m_scaleModeGroup, SIGNAL(selected(QAction*)), this, SLOT(selectScaleMode(QAction*)));

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

    m_pagingAction->setChecked(true);
    m_displayModeGroup->setEnabled(false);

    connect(m_displayModeGroup, SIGNAL(selected(QAction*)), this, SLOT(selectDisplayMode(QAction*)));

    m_fullscreenAction = new QAction(QIcon::fromTheme("view-fullscreen"), tr("Fullscreen"), this);
    m_fullscreenAction->setCheckable(true);
    connect(m_fullscreenAction, SIGNAL(triggered()), this, SLOT(fullscreen()));

    // menuBar

    QMenu *fileMenu = this->menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(m_openAction);
    fileMenu->addAction(m_addTabAction);
    fileMenu->addAction(m_closeTabAction);
    // separator
    // previous tab
    // next tab
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
    viewMenu->addAction(m_scaleFactorAction);
    viewMenu->addAction(m_fitToPageAction);
    viewMenu->addAction(m_fitToPageWidthAction);
    viewMenu->addSeparator();
    viewMenu->addAction(m_pagingAction);
    viewMenu->addAction(m_scrollingAction);
    viewMenu->addAction(m_doublePagingAction);
    viewMenu->addAction(m_doubleScrollingAction);
    viewMenu->addSeparator();
    viewMenu->addAction(m_fullscreenAction);

    // toolBar

    QToolBar *fileToolBar = this->addToolBar(tr("&File"));
    fileToolBar->addAction(m_openAction);
    fileToolBar->addSeparator();
    fileToolBar->addAction(m_reloadAction);
    fileToolBar->addAction(m_saveAction);
    fileToolBar->addAction(m_printAction);

    QToolBar *viewToolBar = this->addToolBar(tr("&View"));
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
    connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(changeCurrent(int)));
    connect(m_tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(requestTabClose(int)));

    this->setCentralWidget(m_tabWidget);
}

MainWindow::~MainWindow()
{
    delete m_openAction;
    delete m_addTabAction;
    delete m_closeTabAction;

    delete m_reloadAction;
    delete m_saveAction;
    delete m_printAction;

    delete m_exitAction;

    delete m_previousPageAction;
    delete m_nextPageAction;
    delete m_firstPageAction;
    delete m_lastPageAction;

    delete m_scaleFactorAction;
    delete m_fitToPageAction;
    delete m_fitToPageWidthAction;
    delete m_scaleModeGroup;

    delete m_pagingAction;
    delete m_scrollingAction;
    delete m_doublePagingAction;
    delete m_doubleScrollingAction;
    delete m_displayModeGroup;

    delete m_fullscreenAction;
}

void MainWindow::open()
{
}

void MainWindow::addTab()
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

void MainWindow::selectDisplayMode(QAction *displayModeAction)
{
}


void MainWindow::fullscreen()
{
}


void MainWindow::changeCurrent(const int &index)
{
}

void MainWindow::requestTabClose(const int &index)
{
}
