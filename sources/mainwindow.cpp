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
    m_normalGeometry()
{
    this->createActions();
    this->createWidgets();
    this->createToolBars();
    this->createDocks();
    this->createMenus();

    this->setCentralWidget(m_tabWidget);
    this->setAcceptDrops(true);

    this->changeCurrentTab(-1);

    // settings

    m_matchCaseCheckBox->setChecked(m_settings.value("mainWindow/matchCase", true).toBool());

    this->restoreGeometry(m_settings.value("mainWindow/geometry").toByteArray());
    this->restoreState(m_settings.value("mainWindow/state").toByteArray());

    //this->menuBar()->setVisible(m_settings.value("mainWindow/menuBar", true).toBool());

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

void MainWindow::createActions()
{
#ifdef DATA_INSTALL_PATH
    QString dataInstallPath(DATA_INSTALL_PATH);
#endif

    // open

    m_openAction = new QAction(tr("&Open..."), this);
    m_openAction->setShortcut(QKeySequence::Open);
    m_openAction->setIconVisibleInMenu(true);
    connect(m_openAction, SIGNAL(triggered()), this, SLOT(open()));

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
    connect(m_recentlyUsedAction, SIGNAL(entrySelected(QString)), this, SLOT(openRecentlyUsed(QString)));

    // refresh

    m_refreshAction = new QAction(tr("&Refresh"), this);
    m_refreshAction->setShortcut(QKeySequence::Refresh);
    m_refreshAction->setIconVisibleInMenu(true);
    connect(m_refreshAction, SIGNAL(triggered()), this, SLOT(refresh()));

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
    connect(m_saveCopyAction, SIGNAL(triggered()), this, SLOT(saveCopy()));

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
    connect(m_printAction, SIGNAL(triggered()), this, SLOT(print()));

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
    connect(m_previousPageAction, SIGNAL(triggered()), this, SLOT(previousPage()));

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
    connect(m_nextPageAction, SIGNAL(triggered()), this, SLOT(nextPage()));

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
    connect(m_firstPageAction, SIGNAL(triggered()), this, SLOT(firstPage()));

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
    connect(m_lastPageAction, SIGNAL(triggered()), this, SLOT(lastPage()));

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
    connect(m_searchAction, SIGNAL(triggered()), this, SLOT(search()));

    // findPrevious

    m_findPreviousAction = new QAction(tr("Find previous"), this);
    m_findPreviousAction->setShortcut(QKeySequence::FindPrevious);
    connect(m_findPreviousAction, SIGNAL(triggered()), this, SLOT(findPrevious()));

    // findNext

    m_findNextAction = new QAction(tr("Find next"), this);
    m_findNextAction->setShortcut(QKeySequence::FindNext);
    connect(m_findNextAction, SIGNAL(triggered()), this, SLOT(findNext()));

    // settings

    m_settingsAction = new QAction(tr("Settings..."), this);
    connect(m_settingsAction, SIGNAL(triggered()), this, SLOT(settings()));

    // pageLayout

    m_onePageAction = new QAction(tr("One page"), this);
    m_onePageAction->setCheckable(true);
    m_twoPagesAction = new QAction(tr("Two pages"), this);
    m_twoPagesAction->setCheckable(true);
    m_oneColumnAction = new QAction(tr("One column"), this);
    m_oneColumnAction->setCheckable(true);
    m_twoColumnsAction = new QAction(tr("Two columns"), this);
    m_twoColumnsAction->setCheckable(true);

    m_pageLayoutGroup = new QActionGroup(this);
    m_pageLayoutGroup->addAction(m_onePageAction);
    m_pageLayoutGroup->addAction(m_twoPagesAction);
    m_pageLayoutGroup->addAction(m_oneColumnAction);
    m_pageLayoutGroup->addAction(m_twoColumnsAction);
    connect(m_pageLayoutGroup, SIGNAL(selected(QAction*)), this, SLOT(changePageLayout(QAction*)));

    // scaling

    m_fitToPageAction = new QAction(tr("Fit to page"), this);
    m_fitToPageAction->setCheckable(true);
    m_fitToPageWidthAction = new QAction(tr("Fit to page width"), this);
    m_fitToPageWidthAction->setCheckable(true);
    m_scaleTo50Action = new QAction(tr("Scale to %1%").arg(50), this);
    m_scaleTo50Action->setCheckable(true);
    m_scaleTo75Action = new QAction(tr("Scale to %1%").arg(75), this);
    m_scaleTo75Action->setCheckable(true);
    m_scaleTo100Action = new QAction(tr("Scale to %1%").arg(100), this);
    m_scaleTo100Action->setCheckable(true);
    m_scaleTo125Action = new QAction(tr("Scale to %1%").arg(125), this);
    m_scaleTo125Action->setCheckable(true);
    m_scaleTo150Action = new QAction(tr("Scale to %1%").arg(150), this);
    m_scaleTo150Action->setCheckable(true);
    m_scaleTo200Action = new QAction(tr("Scale to %1%").arg(200), this);
    m_scaleTo200Action->setCheckable(true);
    m_scaleTo400Action = new QAction(tr("Scale to %1%").arg(400), this);
    m_scaleTo400Action->setCheckable(true);

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
    connect(m_scalingGroup, SIGNAL(selected(QAction*)), this, SLOT(changeScaling(QAction*)));

    // rotation

    m_rotateBy0Action = new QAction(trUtf8("Rotate by %1°").arg(0), this);
    m_rotateBy0Action->setCheckable(true);
    m_rotateBy90Action = new QAction(trUtf8("Rotate by %1°").arg(90), this);
    m_rotateBy90Action->setCheckable(true);
    m_rotateBy180Action = new QAction(trUtf8("Rotate by %1°").arg(180), this);
    m_rotateBy180Action->setCheckable(true);
    m_rotateBy270Action = new QAction(trUtf8("Rotate by %1°").arg(270), this);
    m_rotateBy270Action->setCheckable(true);

    m_rotationGroup = new QActionGroup(this);
    m_rotationGroup->addAction(m_rotateBy0Action);
    m_rotationGroup->addAction(m_rotateBy90Action);
    m_rotationGroup->addAction(m_rotateBy180Action);
    m_rotationGroup->addAction(m_rotateBy270Action);
    connect(m_rotationGroup, SIGNAL(selected(QAction*)), this, SLOT(changeRotation(QAction*)));

    // presentation

    m_presentationAction = new QAction(tr("Presentation..."), this);
    m_presentationAction->setShortcut(QKeySequence(Qt::Key_F10));
    connect(m_presentationAction, SIGNAL(triggered()), this, SLOT(presentation()));

    // fullscreen

    m_fullscreenAction = new QAction(tr("&Fullscreen"), this);
    m_fullscreenAction->setCheckable(true);
    m_fullscreenAction->setShortcut(QKeySequence(Qt::Key_F11));
    m_fullscreenAction->setIcon(QIcon::fromTheme("view-fullscreen"));
    m_fullscreenAction->setIconVisibleInMenu(true);
    connect(m_fullscreenAction, SIGNAL(changed()), this, SLOT(changeFullscreen()));

    // addTab

    m_addTabAction = new QAction(tr("&Add tab..."), this);
    m_addTabAction->setShortcut(QKeySequence::AddTab);
    connect(m_addTabAction, SIGNAL(triggered()), this, SLOT(addTab()));

    // previousTab

    m_previousTabAction = new QAction(tr("&Previous tab"), this);
    m_previousTabAction->setShortcut(QKeySequence::PreviousChild);
    connect(m_previousTabAction, SIGNAL(triggered()), this, SLOT(previousTab()));

    // nextTab

    m_nextTabAction = new QAction(tr("&Next tab"), this);
    m_nextTabAction->setShortcut(QKeySequence::NextChild);
    connect(m_nextTabAction, SIGNAL(triggered()), this, SLOT(nextTab()));

    // closeTab

    m_closeTabAction = new QAction(tr("&Close tab"), this);
    m_closeTabAction->setShortcut(QKeySequence::Close);
    m_closeTabAction->setIcon(QIcon::fromTheme("window-close"));
    m_closeTabAction->setIconVisibleInMenu(true);
    connect(m_closeTabAction, SIGNAL(triggered()), this, SLOT(closeTab()));

    // closeAllTabs

    m_closeAllTabsAction = new QAction(tr("Close all &tabs"), this);
    m_closeAllTabsAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_W));
    connect(m_closeAllTabsAction, SIGNAL(triggered()), this, SLOT(closeAllTabs()));

    // closeAllTabsButCurrent

    m_closeAllTabsButCurrentAction = new QAction(tr("Close all tabs &but current"), this);
    m_closeAllTabsButCurrentAction->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_W));
    connect(m_closeAllTabsButCurrentAction, SIGNAL(triggered()), this, SLOT(closeAllTabsButCurrent()));

    // about

    m_aboutAction = new QAction(tr("&About"), this);
    m_aboutAction->setIcon(QIcon::fromTheme("help-about"));
    m_aboutAction->setIconVisibleInMenu(true);
    connect(m_aboutAction, SIGNAL(triggered()), this, SLOT(about()));
}

void MainWindow::createWidgets()
{
    // tabWidget

    m_tabWidget = new QTabWidget(this);

    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setElideMode(Qt::ElideRight);

    connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(changeCurrentTab(int)));
    connect(m_tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));

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

    connect(m_currentPageLineEdit, SIGNAL(returnPressed()), this, SLOT(changeCurrentPage()));

    // pageLayout

    m_pageLayoutWidget = new QWidget();
    m_pageLayoutLabel = new QLabel(tr("Page &layout:"), m_pageLayoutWidget);
    m_pageLayoutComboBox = new QComboBox(m_pageLayoutWidget);

    m_pageLayoutWidget->setMaximumWidth(300);
    m_pageLayoutLabel->setBuddy(m_pageLayoutComboBox);

    m_pageLayoutComboBox->addItem(tr("One page"), DocumentView::OnePage);
    m_pageLayoutComboBox->addItem(tr("Two pages"), DocumentView::TwoPages);
    m_pageLayoutComboBox->addItem(tr("One column"), DocumentView::OneColumn);
    m_pageLayoutComboBox->addItem(tr("Two columns"), DocumentView::TwoColumns);

    m_pageLayoutWidget->setLayout(new QHBoxLayout());
    m_pageLayoutWidget->layout()->addWidget(m_pageLayoutLabel);
    m_pageLayoutWidget->layout()->addWidget(m_pageLayoutComboBox);

    connect(m_pageLayoutComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changePageLayout(int)));

    // scaling

    m_scalingWidget = new QWidget();
    m_scalingLabel = new QLabel(tr("&Scaling:"), m_scalingWidget);
    m_scalingComboBox = new QComboBox(m_scalingWidget);

    m_scalingWidget->setMaximumWidth(300);
    m_scalingLabel->setBuddy(m_scalingComboBox);

    m_scalingComboBox->addItem(tr("Fit to page"), DocumentView::FitToPage);
    m_scalingComboBox->addItem(tr("Fit to page width"), DocumentView::FitToPageWidth);
    m_scalingComboBox->addItem(tr("Scale to %1%").arg(50), DocumentView::ScaleTo50);
    m_scalingComboBox->addItem(tr("Scale to %1%").arg(75), DocumentView::ScaleTo75);
    m_scalingComboBox->addItem(tr("Scale to %1%").arg(100), DocumentView::ScaleTo100);
    m_scalingComboBox->addItem(tr("Scale to %1%").arg(125), DocumentView::ScaleTo125);
    m_scalingComboBox->addItem(tr("Scale to %1%").arg(150), DocumentView::ScaleTo150);
    m_scalingComboBox->addItem(tr("Scale to %1%").arg(200), DocumentView::ScaleTo200);
    m_scalingComboBox->addItem(tr("Scale to %1%").arg(400), DocumentView::ScaleTo400);

    m_scalingWidget->setLayout(new QHBoxLayout());
    m_scalingWidget->layout()->addWidget(m_scalingLabel);
    m_scalingWidget->layout()->addWidget(m_scalingComboBox);

    connect(m_scalingComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changeScalingIndex(int)));

    // rotationWidget

    m_rotationWidget = new QWidget();
    m_rotationLabel = new QLabel(tr("&Rotation:"), m_rotationWidget);
    m_rotationComboBox = new QComboBox(m_rotationWidget);

    m_rotationWidget->setMaximumWidth(300);
    m_rotationLabel->setBuddy(m_rotationComboBox);

    m_rotationComboBox->addItem(trUtf8("Rotate by %1°").arg(0), DocumentView::RotateBy0);
    m_rotationComboBox->addItem(trUtf8("Rotate by %1°").arg(90), DocumentView::RotateBy90);
    m_rotationComboBox->addItem(trUtf8("Rotate by %1°").arg(180), DocumentView::RotateBy180);
    m_rotationComboBox->addItem(trUtf8("Rotate by %1°").arg(270), DocumentView::RotateBy270);

    m_rotationWidget->setLayout(new QHBoxLayout());
    m_rotationWidget->layout()->addWidget(m_rotationLabel);
    m_rotationWidget->layout()->addWidget(m_rotationComboBox);

    connect(m_rotationComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changeRotationIndex(int)));

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

    connect(m_searchLineEdit, SIGNAL(textEdited(QString)), this, SLOT(searchStart()));
    connect(m_searchLineEdit, SIGNAL(returnPressed()), this, SLOT(searchTimeout()));
    connect(m_searchTimer, SIGNAL(timeout()), this, SLOT(searchTimeout()));
    connect(m_highlightAllCheckBox, SIGNAL(toggled(bool)), this, SLOT(changeHighlightAll()));
    connect(m_findPreviousButton, SIGNAL(clicked()), this, SLOT(findPrevious()));
    connect(m_findNextButton, SIGNAL(clicked()), this, SLOT(findNext()));
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

    connect(m_outlineDock, SIGNAL(visibilityChanged(bool)), m_outlineView, SLOT(updateVisibility(bool)));
    m_outlineDock->hide();

    this->addDockWidget(Qt::LeftDockWidgetArea, m_outlineDock);

    // thumbnails

    m_thumbnailsDock = new QDockWidget(tr("&Thumbnails"), this);
    m_thumbnailsDock->setObjectName("thumbnailsDock");
    m_thumbnailsDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_thumbnailsDock->setFeatures(QDockWidget::AllDockWidgetFeatures);

    m_thumbnailsView = new ThumbnailsView(m_thumbnailsDock);
    m_thumbnailsDock->setWidget(m_thumbnailsView);

    connect(m_thumbnailsDock, SIGNAL(visibilityChanged(bool)), m_thumbnailsView, SLOT(updateVisibility(bool)));
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
    m_tabMenu->addAction(m_closeAllTabsButCurrentAction);
    m_tabMenu->addSeparator();

    // help

    m_helpMenu = this->menuBar()->addMenu(tr("&Help"));
    m_helpMenu->addAction(m_aboutAction);
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

void MainWindow::open()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        QString filePath = QFileDialog::getOpenFileName(this, tr("Open document"),
                                                        m_settings.value("mainWindow/path", QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation)).toString(),
                                                        tr("Portable Document Format (*.pdf)"));

        if(!filePath.isEmpty())
        {
            DocumentView *view = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());
            DocumentModel *model = view->model();

            if(model->open(filePath))
            {
                m_recentlyUsedAction->addEntry(filePath);

                m_settings.setValue("mainWindow/path", QFileInfo(filePath).path());
            }
            else
            {
                QMessageBox::warning(this, tr("Warning"), tr("Could not open document \"%1\".").arg(QFileInfo(filePath).fileName()));
            }
        }
    }
    else
    {
        this->addTab();
    }
}

void MainWindow::refresh()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *view = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());
        DocumentModel *model = view->model();

        model->refresh();
    }
}

void MainWindow::saveCopy()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        QString filePath = QFileDialog::getSaveFileName(this, tr("Save copy"),
                                                        m_settings.value("mainWindow/path", QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation)).toString(),
                                                        tr("Portable Document Format (*.pdf)"));

        if(!filePath.isEmpty())
        {
            DocumentView *view = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());
            DocumentModel *model = view->model();

            if(model->saveCopy(filePath))
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

void MainWindow::print()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *view = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());
        DocumentModel *model = view->model();

        QPrinter *printer = new QPrinter();
        printer->setFullPage(true);

        QPrintDialog printDialog(printer, this);
        printDialog.setMinMax(1, model->pageCount());

        if(printDialog.exec() == QDialog::Accepted)
        {
            int fromPage = printDialog.fromPage() != 0 ? printDialog.fromPage() : 1;
            int toPage = printDialog.toPage() != 0 ? printDialog.toPage() : model->pageCount();

            QProgressDialog progressDialog;

            progressDialog.setLabelText(tr("Printing pages %1 to %2...").arg(fromPage).arg(toPage));
            progressDialog.setRange(0, 100);
            progressDialog.setValue(0);

            connect(model, SIGNAL(printProgressed(int)), &progressDialog, SLOT(setValue(int)));
            connect(model, SIGNAL(printCanceled()), &progressDialog, SLOT(close()));
            connect(model, SIGNAL(printFinished()), &progressDialog, SLOT(close()));

            connect(&progressDialog, SIGNAL(canceled()), model, SLOT(cancelPrint()));

            model->startPrint(printer, fromPage, toPage);

            progressDialog.exec();
        }
    }
}

void MainWindow::previousPage()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *view = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

        view->previousPage();
    }
}

void MainWindow::nextPage()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *view = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

        view->nextPage();
    }
}

void MainWindow::firstPage()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *view = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

        view->firstPage();
    }
}

void MainWindow::lastPage()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *view = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

        view->lastPage();
    }
}

void MainWindow::search()
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

void MainWindow::searchStart()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        m_searchTimer->start();
    }
}

void MainWindow::searchTimeout()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        this->invalidateSearches();

        if(m_searchToolBar->isVisible() && !m_searchLineEdit->text().isEmpty())
        {
            DocumentView *view = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());
            DocumentModel *model = view->model();

            model->startSearch(m_searchLineEdit->text(), m_matchCaseCheckBox->isChecked());
        }
    }
}

void MainWindow::findPrevious()
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
                DocumentView *view = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

                view->findPrevious();
            }
        }
    }
}

void MainWindow::findNext()
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
                DocumentView *view = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

                view->findNext();
            }
        }
    }
}

void MainWindow::settings()
{
    SettingsDialog settingsDialog;

    settingsDialog.exec();
}

void MainWindow::presentation()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *documentView = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());
        PresentationView *presentationView = new PresentationView(documentView->model(), documentView->currentPage());

        presentationView->show();

        presentationView->setAttribute(Qt::WA_DeleteOnClose);
    }
}

void MainWindow::addTab()
{
    QStringList filePaths = QFileDialog::getOpenFileNames(this, tr("Open documents"),
                                                          m_settings.value("mainWindow/path", QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation)).toString(),
                                                          tr("Portable Document Format (*.pdf)"));

    foreach(QString filePath, filePaths)
    {
        if(this->addTab(filePath))
        {
            m_settings.setValue("mainWindow/path", QFileInfo(filePath).path());
        }
        else
        {
            QMessageBox::warning(this, tr("Warning"), tr("Could not open document \"%1\".").arg(QFileInfo(filePath).fileName()));
        }
    }
}

void MainWindow::previousTab()
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

void MainWindow::nextTab()
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

void MainWindow::closeTab()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        this->closeTab(m_tabWidget->currentIndex());
    }
}

void MainWindow::closeAllTabs()
{
    while(m_tabWidget->count() > 0)
    {
        this->closeTab(0);
    }
}

void MainWindow::closeAllTabsButCurrent()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *view = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());
        DocumentModel *model = view->model();

        m_tabWidget->removeTab(m_tabWidget->currentIndex());

        closeAllTabs();

        int index = m_tabWidget->addTab(view, QFileInfo(model->filePath()).completeBaseName());
        m_tabWidget->setTabToolTip(index, QFileInfo(model->filePath()).completeBaseName());
        m_tabWidget->setCurrentIndex(index);
    }
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About qpdfview"), tr("<p><b>qpdfview</b></p><p>qpdfview is a tabbed PDF viewer using the poppler library.</p><p>&copy; 2012 Adam Reichold</p>"));
}


void MainWindow::changeFullscreen()
{
    if(m_fullscreenAction->isChecked())
    {
        m_normalGeometry = this->saveGeometry();

        this->showFullScreen();
    }
    else
    {
        this->restoreGeometry(m_normalGeometry);

        this->showNormal();

        this->restoreGeometry(m_normalGeometry);
    }
}

void MainWindow::changeHighlightAll()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *view = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

        view->setHighlightAll(m_highlightAllCheckBox->isChecked());
    }
}

bool MainWindow::addTab(const QString &filePath)
{
    DocumentModel *model = new DocumentModel();

    if(model->open(filePath))
    {
        DocumentView *view = new DocumentView(model);

        int index = m_tabWidget->addTab(view, QFileInfo(filePath).completeBaseName());
        m_tabWidget->setTabToolTip(index, QFileInfo(filePath).completeBaseName());
        m_tabWidget->setCurrentIndex(index);

        m_tabMenu->addAction(view->makeCurrentTabAction());

        m_recentlyUsedAction->addEntry(filePath);

        connect(model, SIGNAL(pageCountChanged(int)), this, SLOT(updateNumberOfPages(int)));

        connect(model, SIGNAL(searchProgressed(int)), this, SLOT(searchProgressed(int)));
        connect(model, SIGNAL(searchCanceled()), this, SLOT(searchCanceled()));
        connect(model, SIGNAL(searchFinished()), this, SLOT(searchFinished()));

        connect(view, SIGNAL(currentPageChanged(int)), this, SLOT(updateCurrentPage(int)));
        connect(view, SIGNAL(pageLayoutChanged(DocumentView::PageLayout)), this, SLOT(updatePageLayout(DocumentView::PageLayout)));
        connect(view, SIGNAL(scalingChanged(DocumentView::Scaling)), this, SLOT(updateScaling(DocumentView::Scaling)));
        connect(view, SIGNAL(rotationChanged(DocumentView::Rotation)), this, SLOT(updateRotation(DocumentView::Rotation)));
        connect(view, SIGNAL(highlightAllChanged(bool)), this, SLOT(updateHighlightAll(bool)));

        return true;
    }
    else
    {
        delete model;

        return false;
    }
}

void MainWindow::closeTab(int index)
{
    DocumentView *view = qobject_cast<DocumentView*>(m_tabWidget->widget(index));

    if(view)
    {
        DocumentModel *model = view->model();

        delete view;
        delete model;
    }
}

void MainWindow::changeCurrentTab(int index)
{
    if(index != -1)
    {
        DocumentView *view = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());
        DocumentModel *model = view->model();

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

        this->updateCurrentPage(view->currentPage());
        this->updateNumberOfPages(model->pageCount());
        this->updatePageLayout(view->pageLayout());
        this->updateScaling(view->scaling());
        this->updateRotation(view->rotation());
        this->updateHighlightAll(view->highlightAll());

        if(m_searchToolBar->isVisible())
        {
            this->invalidateSearches();

            m_searchLineEdit->clear();
        }

        m_outlineView->setView(view);
        m_thumbnailsView->setView(view);
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

        m_outlineView->setView(0);
        m_thumbnailsView->setView(0);
    }
}

void MainWindow::changeCurrentPage()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *view = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

        view->setCurrentPage(m_currentPageLineEdit->text().toInt());
    }
}

void MainWindow::changePageLayout(QAction *action)
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *view = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

        if(action == m_onePageAction)
        {
            view->setPageLayout(DocumentView::OnePage);
        }
        else if(action == m_twoPagesAction)
        {
            view->setPageLayout(DocumentView::TwoPages);
        }
        else if(action == m_oneColumnAction)
        {
            view->setPageLayout(DocumentView::OneColumn);
        }
        else if(action == m_twoColumnsAction)
        {
            view->setPageLayout(DocumentView::TwoColumns);
        }
    }
}

void MainWindow::changePageLayout(int index)
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *view = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

        view->setPageLayout(static_cast<DocumentView::PageLayout>(m_pageLayoutComboBox->itemData(index).toUInt()));
    }
}

void MainWindow::changeScaling(QAction *action)
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *view = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

        if(action == m_fitToPageAction)
        {
            view->setScaling(DocumentView::FitToPage);
        }
        else if(action == m_fitToPageWidthAction)
        {
            view->setScaling(DocumentView::FitToPageWidth);
        }
        else if(action == m_scaleTo50Action)
        {
            view->setScaling(DocumentView::ScaleTo50);
        }
        else if(action == m_scaleTo75Action)
        {
            view->setScaling(DocumentView::ScaleTo75);
        }
        else if(action == m_scaleTo100Action)
        {
            view->setScaling(DocumentView::ScaleTo100);
        }
        else if(action == m_scaleTo125Action)
        {
            view->setScaling(DocumentView::ScaleTo125);
        }
        else if(action == m_scaleTo150Action)
        {
            view->setScaling(DocumentView::ScaleTo150);
        }
        else if(action == m_scaleTo200Action)
        {
            view->setScaling(DocumentView::ScaleTo200);
        }
        else if(action == m_scaleTo400Action)
        {
            view->setScaling(DocumentView::ScaleTo400);
        }
    }
}

void MainWindow::changeScalingIndex(int index)
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *view = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

        view->setScaling(static_cast<DocumentView::Scaling>(m_scalingComboBox->itemData(index).toUInt()));
    }
}

void MainWindow::changeRotation(QAction *rotationAction)
{
    if(m_tabWidget->currentIndex() != -1)
    {
       DocumentView *view = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

        if(rotationAction == m_rotateBy0Action)
        {
            view->setRotation(DocumentView::RotateBy0);
        }
        else if(rotationAction == m_rotateBy90Action)
        {
            view->setRotation(DocumentView::RotateBy90);
        }
        else if(rotationAction == m_rotateBy180Action)
        {
            view->setRotation(DocumentView::RotateBy180);
        }
        else if(rotationAction == m_rotateBy270Action)
        {
            view->setRotation(DocumentView::RotateBy270);
        }
    }
}

void MainWindow::changeRotationIndex(int index)
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *view = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

        view->setRotation(static_cast<DocumentView::Rotation>(m_rotationComboBox->itemData(index).toUInt()));
    }
}

void MainWindow::updateNumberOfPages(int numberOfPages)
{
    m_currentPageValidator->setRange(1, numberOfPages);
    m_numberOfPagesLabel->setText(tr(" of %1").arg(numberOfPages));
}

void MainWindow::updateCurrentPage(int currentPage)
{
    m_currentPageLineEdit->setText(tr("%1").arg(currentPage));
}

void MainWindow::updatePageLayout(DocumentView::PageLayout pageLayout)
{
    switch(pageLayout)
    {
    case DocumentView::OnePage:
        m_onePageAction->setChecked(true);
        m_pageLayoutComboBox->setCurrentIndex(0);
        break;
    case DocumentView::TwoPages:
        m_twoPagesAction->setChecked(true);
        m_pageLayoutComboBox->setCurrentIndex(1);
        break;
    case DocumentView::OneColumn:
        m_oneColumnAction->setChecked(true);
        m_pageLayoutComboBox->setCurrentIndex(2);
        break;
    case DocumentView::TwoColumns:
        m_twoColumnsAction->setChecked(true);
        m_pageLayoutComboBox->setCurrentIndex(3);
        break;
    }
}

void MainWindow::updateScaling(DocumentView::Scaling scaling)
{
    switch(scaling)
    {
    case DocumentView::FitToPage:
        m_fitToPageAction->setChecked(true);
        m_scalingComboBox->setCurrentIndex(0);
        break;
    case DocumentView::FitToPageWidth:
        m_fitToPageWidthAction->setChecked(true);
        m_scalingComboBox->setCurrentIndex(1);
        break;
    case DocumentView::ScaleTo50:
        m_scaleTo50Action->setChecked(true);
        m_scalingComboBox->setCurrentIndex(2);
        break;
    case DocumentView::ScaleTo75:
        m_scaleTo75Action->setChecked(true);
        m_scalingComboBox->setCurrentIndex(3);
        break;
    case DocumentView::ScaleTo100:
        m_scaleTo100Action->setChecked(true);
        m_scalingComboBox->setCurrentIndex(4);
        break;
    case DocumentView::ScaleTo125:
        m_scaleTo125Action->setChecked(true);
        m_scalingComboBox->setCurrentIndex(5);
        break;
    case DocumentView::ScaleTo150:
        m_scaleTo150Action->setChecked(true);
        m_scalingComboBox->setCurrentIndex(6);
        break;
    case DocumentView::ScaleTo200:
        m_scaleTo200Action->setChecked(true);
        m_scalingComboBox->setCurrentIndex(7);
        break;
    case DocumentView::ScaleTo400:
        m_scaleTo400Action->setChecked(true);
        m_scalingComboBox->setCurrentIndex(8);
        break;
    }
}

void MainWindow::updateRotation(DocumentView::Rotation rotation)
{
    switch(rotation)
    {
    case DocumentView::RotateBy0:
        m_rotateBy0Action->setChecked(true);
        m_rotationComboBox->setCurrentIndex(0);
        break;
    case DocumentView::RotateBy90:
        m_rotateBy90Action->setChecked(true);
        m_rotationComboBox->setCurrentIndex(1);
        break;
    case DocumentView::RotateBy180:
        m_rotateBy180Action->setChecked(true);
        m_rotationComboBox->setCurrentIndex(2);
        break;
    case DocumentView::RotateBy270:
        m_rotateBy270Action->setChecked(true);
        m_rotationComboBox->setCurrentIndex(3);
        break;
    }
}

void MainWindow::updateHighlightAll(bool highlightAll)
{
    m_highlightAllCheckBox->setChecked(highlightAll);
}

void MainWindow::searchProgressed(int value)
{
    this->statusBar()->show();
    this->statusBar()->showMessage(tr("Searched %1% of the current document...").arg(value));
}

void MainWindow::searchCanceled()
{
    this->statusBar()->clearMessage();
    this->statusBar()->hide();
}

void MainWindow::searchFinished()
{
    this->statusBar()->clearMessage();
    this->statusBar()->hide();

    this->findNext();
}

void MainWindow::invalidateSearches()
{
    m_searchTimer->stop();

    for(int index = 0; index < m_tabWidget->count(); index++)
    {
        DocumentView *view = qobject_cast<DocumentView*>(m_tabWidget->widget(index));
        DocumentModel *model = view->model();

        model->cancelSearch();
    }
}

void MainWindow::openRecentlyUsed(const QString &filePath)
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *view = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());
        DocumentModel *model = view->model();

        if(!model->open(filePath))
        {
            QMessageBox::warning(this, tr("Warning"), tr("Could not open document \"%1\".").arg(QFileInfo(filePath).fileName()));
        }
    }
    else
    {
        if(!this->addTab(filePath))
        {
            QMessageBox::warning(this, tr("Warning"), tr("Could not open document \"%1\".").arg(QFileInfo(filePath).fileName()));
        }
    }
}

void MainWindow::openExternalLink(const QString &filePath, int pageNumber, qreal top, bool addTab)
{
    if(m_tabWidget->currentIndex() != -1 && !addTab)
    {
        DocumentView *view = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());
        DocumentModel *model = view->model();

        if(model->open(filePath))
        {
            view->setCurrentPage(pageNumber, top);
        }
        else
        {
            QMessageBox::warning(this, tr("Warning"), tr("Could not open document \"%1\".").arg(QFileInfo(filePath).fileName()));
        }
    }
    else
    {
        if(this->addTab(filePath))
        {
            DocumentView *view = qobject_cast<DocumentView*>(m_tabWidget->currentWidget());

            view->setCurrentPage(pageNumber, top);
        }
        else
        {
            QMessageBox::warning(this, tr("Warning"), tr("Could not open document \"%1\".").arg(QFileInfo(filePath).fileName()));
        }
    }
}

void MainWindow::keyPressEvent(QKeyEvent *keyEvent)
{
    if(keyEvent->key() == Qt::Key_Escape)
    {
        if(m_searchToolBar->isVisible())
        {
            this->invalidateSearches();

            m_searchLineEdit->clear();

            m_searchToolBar->hide();
        }
    }
    /*else if(keyEvent->modifiers() == Qt::ALT && keyEvent->key() == Qt::Key_M)
    {
        this->menuBar()->setVisible(!this->menuBar()->isVisible());

        m_settings.setValue("mainWindow/menuBar", this->menuBar()->isVisible());
    }
    else
    {
        if(this->menuBar()->isHidden())
        {
            QKeySequence shortcut(keyEvent->modifiers() + keyEvent->key());

            foreach(QObject *object, this->children())
            {
                QAction *action = qobject_cast<QAction*>(object);

                if(action)
                {
                    if(action->shortcut() == shortcut)
                    {
                        action->trigger();
                    }
                }
            }
        }
    }*/

    if(m_tabWidget->currentIndex() != -1)
    {
        QApplication::sendEvent(m_tabWidget->currentWidget(), keyEvent);
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *dragEnterEvent)
{
    if(dragEnterEvent->mimeData()->hasUrls())
    {
        dragEnterEvent->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *dropEvent)
{
    if(dropEvent->mimeData()->hasUrls())
    {
        dropEvent->acceptProposedAction();

        foreach(QUrl url, dropEvent->mimeData()->urls())
        {
            if(url.scheme() == "file" && QFileInfo(url.path()).exists())
            {
                this->addTab(url.path());
            }
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *closeEvent)
{
    this->closeAllTabs();

    m_searchToolBar->hide();

    m_settings.setValue("mainWindow/matchCase", m_matchCaseCheckBox->isChecked());

    if(m_fullscreenAction->isChecked())
    {
        m_settings.setValue("mainWindow/geometry", m_normalGeometry);
    }
    else
    {
        m_settings.setValue("mainWindow/geometry", this->saveGeometry());
    }

    m_settings.setValue("mainWindow/state", this->saveState());

    QMainWindow::closeEvent(closeEvent);
}
