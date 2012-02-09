#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_openAction = new QAction(tr("&Open..."), this);
    m_openAction->setIcon(QIcon::fromTheme("document-open"));
    connect(m_openAction, SIGNAL(triggered()), this, SLOT(open()));
    m_closeTabAction = new QAction(tr("&Close Tab"), this);
    m_closeTabAction->setIcon(QIcon::fromTheme("window-close"));
    connect(m_closeTabAction, SIGNAL(triggered()), this, SLOT(closeTab()));

    m_saveAction = new QAction(tr("&Save..."), this);
    m_saveAction->setIcon(QIcon::fromTheme("document-save"));
    connect(m_saveAction, SIGNAL(triggered()), this, SLOT(save()));
    m_printAction = new QAction(tr("&Print..."), this);
    m_printAction->setIcon(QIcon::fromTheme("document-print"));
    connect(m_printAction, SIGNAL(triggered()), this, SLOT(print()));

    m_exitAction = new QAction(tr("&Exit"), this);
    m_exitAction->setIcon(QIcon::fromTheme("application-exit"));
    connect(m_exitAction, SIGNAL(triggered()), this, SLOT(close()));

    m_previousPageAction = new QAction(tr("Previous page"), this);
    m_previousPageAction->setIcon(QIcon::fromTheme("go-previous"));
    connect(m_previousPageAction, SIGNAL(triggered()), this, SLOT(previousPage()));
    m_nextPageAction = new QAction(tr("&Next page"), this);
    m_nextPageAction->setIcon(QIcon::fromTheme("go-next"));
    connect(m_nextPageAction, SIGNAL(triggered()), this, SLOT(nextPage()));
    m_firstPageAction = new QAction(tr("&First page"), this);
    m_firstPageAction->setIcon(QIcon::fromTheme("go-first"));
    connect(m_firstPageAction, SIGNAL(triggered()), this, SLOT(firstPage()));
    m_lastPageAction = new QAction(tr("&Last page"), this);
    m_lastPageAction->setIcon(QIcon::fromTheme("go-last"));
    connect(m_lastPageAction, SIGNAL(triggered()), this, SLOT(lastPage()));

    // menuBar

    QMenu *fileMenu = this->menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(m_openAction);
    fileMenu->addAction(m_closeTabAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_saveAction);
    fileMenu->addAction(m_printAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_exitAction);

    QMenu *viewMenu = this->menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(m_previousPageAction);
    viewMenu->addAction(m_nextPageAction);
    viewMenu->addAction(m_firstPageAction);
    viewMenu->addAction(m_lastPageAction);

    // toolBar

    QToolBar *viewToolBar = this->addToolBar(tr("&View"));
    viewToolBar->addAction(m_previousPageAction);
    viewToolBar->addAction(m_nextPageAction);
    viewToolBar->addAction(m_firstPageAction);
    viewToolBar->addAction(m_lastPageAction);

    // centralWidget

    m_tabWidget = new QTabWidget();
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setDocumentMode(true);

    this->setCentralWidget(m_tabWidget);
}

MainWindow::~MainWindow()
{
    delete m_openAction;
    delete m_closeTabAction;

    delete m_saveAction;
    delete m_printAction;

    delete m_exitAction;

    delete m_previousPageAction;
    delete m_nextPageAction;
    delete m_firstPageAction;
    delete m_lastPageAction;
}

void MainWindow::open()
{
}

void MainWindow::closeTab()
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
