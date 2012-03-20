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
    m_settings(),m_normalGeometry()
{
    this->createActions();
    this->createToolbars();
    this->createDocks();
    this->createMenus();

    // tabWidget

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setElideMode(Qt::ElideRight);
    this->setCentralWidget(m_tabWidget);
    this->changeCurrentTab(-1);

    connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(changeCurrentTab(int)));
    connect(m_tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(requestTabClose(int)));

    // geometry

    this->restoreGeometry(m_settings.value("mainWindow/geometry").toByteArray());
    this->restoreState(m_settings.value("mainWindow/state").toByteArray());

    // miscellaneous

    PageObject::setPageCacheThreading(m_settings.value("pageObject/pageCacheThreading", true).toBool());
    PageObject::setPageCacheSize(m_settings.value("pageObject/pageCacheSize", 134217728).toUInt());

    this->setAcceptDrops(true);

    QStringList arguments = QCoreApplication::arguments();
    arguments.removeFirst();

    foreach(QString argument, arguments)
    {
        if(QFile(argument).exists()) {
            DocumentView *documentView = new DocumentView();

            if(documentView->open(argument))
            {
                int index = m_tabWidget->addTab(documentView, QFileInfo(argument).baseName());
                m_tabWidget->setTabToolTip(index, QFileInfo(argument).baseName());
                m_tabWidget->setCurrentIndex(index);

                m_tabMenu->addAction(documentView->tabMenuAction());

                connect(documentView, SIGNAL(filePathChanged(QString)), this, SLOT(updateFilePath(QString)));
                connect(documentView, SIGNAL(currentPageChanged(int)), this, SLOT(updateCurrentPage(int)));
                connect(documentView, SIGNAL(numberOfPagesChanged(int)), this, SLOT(updateNumberOfPages(int)));
                connect(documentView, SIGNAL(pageLayoutChanged(DocumentView::PageLayout)), this, SLOT(updatePageLayout(DocumentView::PageLayout)));
                connect(documentView, SIGNAL(scalingChanged(DocumentView::Scaling)), this, SLOT(updateScaling(DocumentView::Scaling)));
                connect(documentView, SIGNAL(rotationChanged(DocumentView::Rotation)), this, SLOT(updateRotation(DocumentView::Rotation)));

                connect(documentView, SIGNAL(searchingProgressed(int)), this, SLOT(updateSearchProgress(int)));
                connect(documentView, SIGNAL(searchingCanceled()), this, SLOT(updateSearchProgress()));
                connect(documentView, SIGNAL(searchingFinished()), this, SLOT(updateSearchProgress()));
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
    delete m_refreshAction;
    delete m_printAction;

    delete m_exitAction;

    delete m_previousPageAction;
    delete m_nextPageAction;
    delete m_firstPageAction;
    delete m_lastPageAction;

    delete m_searchAction;
    delete m_findPreviousAction;
    delete m_findNextAction;
    delete m_copyTextAction;

    delete m_onePageAction;
    delete m_twoPagesAction;
    delete m_oneColumnAction;
    delete m_twoColumnsAction;

    delete m_pageLayoutGroup;

    delete m_fitToPageAction;
    delete m_fitToPageWidthAction;
    delete m_scaleTo50Action;
    delete m_scaleTo75Action;
    delete m_scaleTo100Action;
    delete m_scaleTo125Action;
    delete m_scaleTo150Action;
    delete m_scaleTo200Action;
    delete m_scaleTo400Action;

    delete m_scalingGroup;

    delete m_rotateBy0Action;
    delete m_rotateBy90Action;
    delete m_rotateBy180Action;
    delete m_rotateBy270Action;

    delete m_rotationGroup;

    delete m_fullscreenAction;

    delete m_addTabAction;
    delete m_previousTabAction;
    delete m_nextTabAction;
    delete m_closeTabAction;

    delete m_closeAllTabsAction;
    delete m_closeAllTabsButCurrentAction;

    delete m_aboutAction;

    delete m_currentPageLabel;
    delete m_currentPageLineEdit;
    delete m_currentPageValidator;
    delete m_numberOfPagesLabel;

    delete m_pageLayoutLabel;
    delete m_pageLayoutComboBox;

    delete m_scalingLabel;
    delete m_scalingComboBox;

    delete m_rotationLabel;
    delete m_rotationWidget;

    delete m_searchLabel;
    delete m_searchLineEdit;
    delete m_matchCaseCheckBox;
    delete m_highlightAllCheckBox;
    delete m_searchTimer;
    delete m_findPreviousButton;
    delete m_findNextButton;

    delete m_outlineView;
    delete m_thumbnailsView;
}

QSize MainWindow::sizeHint() const
{
    return QSize(500,700);
}

QMenu *MainWindow::createPopupMenu()
{
    QMenu *popupMenu = new QMenu();

    popupMenu->addAction(m_fileToolBar->toggleViewAction());
    popupMenu->addAction(m_editToolBar->toggleViewAction());
    popupMenu->addAction(m_viewToolBar->toggleViewAction());

    return popupMenu;
}

void MainWindow::createActions()
{
    m_openAction = new QAction(tr("&Open..."), this);
    m_openAction->setShortcut(QKeySequence::Open);
    if(QIcon::hasThemeIcon("document-open"))
    {
        m_openAction->setIcon(QIcon::fromTheme("document-open"));
    }
    else
    {
        m_openAction->setIcon(QIcon(":/icons/document-open.svg"));
    }
    m_openAction->setIconVisibleInMenu(true);
    m_refreshAction = new QAction(tr("&Refresh"), this);
    m_refreshAction->setShortcut(QKeySequence::Refresh);
    if(QIcon::hasThemeIcon("view-refresh"))
    {
        m_refreshAction->setIcon(QIcon::fromTheme("view-refresh"));
    }
    else
    {
        m_refreshAction->setIcon(QIcon(":/icons/view-refresh.svg"));
    }
    m_refreshAction->setIconVisibleInMenu(true);
    m_saveCopyAction = new QAction(tr("&Save copy..."), this);
    m_saveCopyAction->setShortcut(QKeySequence::Save);
    if(QIcon::hasThemeIcon("document-save"))
    {
        m_saveCopyAction->setIcon(QIcon::fromTheme("document-save"));
    }
    else
    {
        m_saveCopyAction->setIcon(QIcon(":/icons/document-save.svg"));
    }
    m_saveCopyAction->setIconVisibleInMenu(true);
    m_printAction = new QAction(tr("&Print..."), this);
    m_printAction->setShortcut(QKeySequence::Print);
    if(QIcon::hasThemeIcon("document-print"))
    {
        m_printAction->setIcon(QIcon::fromTheme("document-print"));
    }
    else
    {
        m_printAction->setIcon(QIcon(":/icons/document-print.svg"));
    }
    m_printAction->setIconVisibleInMenu(true);

    connect(m_openAction, SIGNAL(triggered()), this, SLOT(open()));
    connect(m_refreshAction, SIGNAL(triggered()), this, SLOT(refresh()));
    connect(m_saveCopyAction, SIGNAL(triggered()), this, SLOT(saveCopy()));
    connect(m_printAction, SIGNAL(triggered()), this, SLOT(print()));

    m_exitAction = new QAction(tr("&Exit"), this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    m_exitAction->setIcon(QIcon::fromTheme("application-exit"));
    m_exitAction->setIconVisibleInMenu(true);

    connect(m_exitAction, SIGNAL(triggered()), this, SLOT(close()));

    m_previousPageAction = new QAction(tr("&Previous page"), this);
    m_previousPageAction->setShortcut(QKeySequence::MoveToPreviousPage);
    if(QIcon::hasThemeIcon("go-previous"))
    {
        m_previousPageAction->setIcon(QIcon::fromTheme("go-previous"));
    }
    else
    {
        m_previousPageAction->setIcon(QIcon(":/icons/go-previous.svg"));
    }
    m_previousPageAction->setIconVisibleInMenu(true);
    m_nextPageAction = new QAction(tr("&Next page"), this);
    m_nextPageAction->setShortcut(QKeySequence::MoveToNextPage);
    if(QIcon::hasThemeIcon("go-next"))
    {
        m_nextPageAction->setIcon(QIcon::fromTheme("go-next"));
    }
    else
    {
        m_nextPageAction->setIcon(QIcon(":/icons/go-next.svg"));
    }
    m_nextPageAction->setIconVisibleInMenu(true);
    m_firstPageAction = new QAction(tr("&First page"), this);
    m_firstPageAction->setShortcut(QKeySequence::MoveToStartOfDocument);
    if(QIcon::hasThemeIcon("go-first"))
    {
        m_firstPageAction->setIcon(QIcon::fromTheme("go-first"));
    }
    else
    {
        m_firstPageAction->setIcon(QIcon(":/icons/go-first.svg"));
    }
    m_firstPageAction->setIconVisibleInMenu(true);
    m_lastPageAction = new QAction(tr("&Last page"), this);
    m_lastPageAction->setShortcut(QKeySequence::MoveToEndOfDocument);
    if(QIcon::hasThemeIcon("go-last"))
    {
        m_lastPageAction->setIcon(QIcon::fromTheme("go-last"));
    }
    else
    {
        m_lastPageAction->setIcon(QIcon(":/icons/go-last.svg"));
    }
    m_lastPageAction->setIconVisibleInMenu(true);

    connect(m_previousPageAction, SIGNAL(triggered()), this, SLOT(previousPage()));
    connect(m_nextPageAction, SIGNAL(triggered()), this, SLOT(nextPage()));
    connect(m_firstPageAction, SIGNAL(triggered()), this, SLOT(firstPage()));
    connect(m_lastPageAction, SIGNAL(triggered()), this, SLOT(lastPage()));

    m_searchAction = new QAction(tr("&Search..."), this);
    m_searchAction->setShortcut(QKeySequence::Find);
    m_searchAction->setIcon(QIcon::fromTheme("edit-find"));
    m_searchAction->setIconVisibleInMenu(true);

    m_findPreviousAction = new QAction(tr("Find previous"), this);
    m_findPreviousAction->setShortcut(QKeySequence::FindPrevious);
    m_findNextAction = new QAction(tr("Find next"), this);
    m_findNextAction->setShortcut(QKeySequence::FindNext);

    m_copyTextAction = new QAction(tr("&Copy text"), this);
    m_copyTextAction->setShortcut(QKeySequence::Copy);
    m_copyTextAction->setIcon(QIcon::fromTheme("edit-copy"));
    m_copyTextAction->setIconVisibleInMenu(true);

    m_editSettingsAction = new QAction(tr("Edit settings..."), this);

    connect(m_searchAction, SIGNAL(triggered()), this, SLOT(search()));
    connect(m_findPreviousAction, SIGNAL(triggered()), this, SLOT(findPrevious()));
    connect(m_findNextAction, SIGNAL(triggered()), this, SLOT(findNext()));
    connect(m_copyTextAction, SIGNAL(triggered()), this, SLOT(copyText()));
    connect(m_editSettingsAction, SIGNAL(triggered()), this, SLOT(editSettings()));

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

    connect(m_pageLayoutGroup, SIGNAL(selected(QAction*)), this, SLOT(selectPageLayout(QAction*)));

    m_fitToPageAction = new QAction(tr("Fit to page"), this);
    m_fitToPageAction->setCheckable(true);
    m_fitToPageWidthAction = new QAction(tr("Fit to page width"), this);
    m_fitToPageWidthAction->setCheckable(true);
    m_scaleTo50Action = new QAction(tr("Scale to 50%"), this);
    m_scaleTo50Action->setCheckable(true);
    m_scaleTo75Action = new QAction(tr("Scale to 75%"), this);
    m_scaleTo75Action->setCheckable(true);
    m_scaleTo100Action = new QAction(tr("Scale to 100%"), this);
    m_scaleTo100Action->setCheckable(true);
    m_scaleTo125Action = new QAction(tr("Scale to 125%"), this);
    m_scaleTo125Action->setCheckable(true);
    m_scaleTo150Action = new QAction(tr("Scale to 150%"), this);
    m_scaleTo150Action->setCheckable(true);
    m_scaleTo200Action = new QAction(tr("Scale to 200%"), this);
    m_scaleTo200Action->setCheckable(true);
    m_scaleTo400Action = new QAction(tr("Scale to 400%"), this);
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

    connect(m_scalingGroup, SIGNAL(selected(QAction*)), this, SLOT(selectScaling(QAction*)));

    m_rotateBy0Action = new QAction(trUtf8("Rotate by 0°"), this);
    m_rotateBy0Action->setCheckable(true);
    m_rotateBy90Action = new QAction(trUtf8("Rotate by 90°"), this);
    m_rotateBy90Action->setCheckable(true);
    m_rotateBy180Action = new QAction(trUtf8("Rotate by 180°"), this);
    m_rotateBy180Action->setCheckable(true);
    m_rotateBy270Action = new QAction(trUtf8("Rotate by 270°"), this);
    m_rotateBy270Action->setCheckable(true);

    m_rotationGroup = new QActionGroup(this);
    m_rotationGroup->addAction(m_rotateBy0Action);
    m_rotationGroup->addAction(m_rotateBy90Action);
    m_rotationGroup->addAction(m_rotateBy180Action);
    m_rotationGroup->addAction(m_rotateBy270Action);

    connect(m_rotationGroup, SIGNAL(selected(QAction*)), this, SLOT(selectRotation(QAction*)));

    m_fullscreenAction = new QAction(tr("&Fullscreen"), this);
    m_fullscreenAction->setCheckable(true);
    m_fullscreenAction->setShortcut(QKeySequence(Qt::Key_F11));
    m_fullscreenAction->setIcon(QIcon::fromTheme("view-fullscreen"));
    m_fullscreenAction->setIconVisibleInMenu(true);

    connect(m_fullscreenAction, SIGNAL(changed()), this, SLOT(changeFullscreen()));

    m_addTabAction = new QAction(tr("&Add tab..."), this);
    m_addTabAction->setShortcut(QKeySequence::AddTab);
    m_previousTabAction = new QAction(tr("&Previous tab"), this);
    m_previousTabAction->setShortcut(QKeySequence::PreviousChild);
    m_nextTabAction = new QAction(tr("&Next tab"), this);
    m_nextTabAction->setShortcut(QKeySequence::NextChild);
    m_closeTabAction = new QAction(tr("&Close tab"), this);
    m_closeTabAction->setShortcut(QKeySequence::Close);
    m_closeTabAction->setIcon(QIcon::fromTheme("window-close"));
    m_closeTabAction->setIconVisibleInMenu(true);

    connect(m_addTabAction, SIGNAL(triggered()), this, SLOT(addTab()));
    connect(m_previousTabAction, SIGNAL(triggered()), this, SLOT(previousTab()));
    connect(m_nextTabAction, SIGNAL(triggered()), this, SLOT(nextTab()));
    connect(m_closeTabAction, SIGNAL(triggered()), this, SLOT(closeTab()));

    m_closeAllTabsAction = new QAction(tr("Close all &tabs"), this);
    m_closeAllTabsAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_W));
    m_closeAllTabsButCurrentAction = new QAction(tr("Close all tabs &but current"), this);
    m_closeAllTabsButCurrentAction->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_W));

    connect(m_closeAllTabsAction, SIGNAL(triggered()), this, SLOT(closeAllTabs()));
    connect(m_closeAllTabsButCurrentAction, SIGNAL(triggered()), this, SLOT(closeAllTabsButCurrent()));

    m_aboutAction = new QAction(tr("&About"), this);
    m_aboutAction->setIcon(QIcon::fromTheme("help-about"));
    m_aboutAction->setIconVisibleInMenu(true);

    connect(m_aboutAction, SIGNAL(triggered()), this, SLOT(about()));
}

void MainWindow::createToolbars()
{
    // currentPageWidget

    m_currentPageWidget = new QWidget();
    m_currentPageLabel = new QLabel(tr("Page:"));
    m_currentPageLineEdit = new QLineEdit();
    m_currentPageValidator = new QIntValidator();
    m_numberOfPagesLabel = new QLabel();

    m_currentPageLineEdit->setAlignment(Qt::AlignCenter);
    m_currentPageLineEdit->setValidator(m_currentPageValidator);

    connect(m_currentPageLineEdit, SIGNAL(returnPressed()), this, SLOT(changeCurrentPage()));

    m_currentPageWidget->setLayout(new QHBoxLayout());
    m_currentPageWidget->layout()->addWidget(m_currentPageLabel);
    m_currentPageWidget->layout()->addWidget(m_currentPageLineEdit);
    m_currentPageWidget->layout()->addWidget(m_numberOfPagesLabel);
    m_currentPageWidget->setMaximumWidth(200);

    // pageLayoutWidget

    m_pageLayoutWidget = new QWidget();
    m_pageLayoutLabel = new QLabel(tr("Page layout:"));
    m_pageLayoutComboBox = new QComboBox();

    m_pageLayoutComboBox->addItem(tr("One page"), DocumentView::OnePage);
    m_pageLayoutComboBox->addItem(tr("Two pages"), DocumentView::TwoPages);
    m_pageLayoutComboBox->addItem(tr("One column"), DocumentView::OneColumn);
    m_pageLayoutComboBox->addItem(tr("Two columns"), DocumentView::TwoColumns);

    connect(m_pageLayoutComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changePageLayoutIndex(int)));

    m_pageLayoutWidget->setLayout(new QHBoxLayout());
    m_pageLayoutWidget->layout()->addWidget(m_pageLayoutLabel);
    m_pageLayoutWidget->layout()->addWidget(m_pageLayoutComboBox);
    m_pageLayoutWidget->setMaximumWidth(300);

    // scalingWidget

    m_scalingWidget = new QWidget();
    m_scalingLabel = new QLabel(tr("Scaling:"));
    m_scalingComboBox = new QComboBox();

    m_scalingComboBox->addItem(tr("Fit to page"), DocumentView::FitToPage);
    m_scalingComboBox->addItem(tr("Fit to page width"), DocumentView::FitToPageWidth);
    m_scalingComboBox->addItem(tr("Scale to 50%"), DocumentView::ScaleTo50);
    m_scalingComboBox->addItem(tr("Scale to 75%"), DocumentView::ScaleTo75);
    m_scalingComboBox->addItem(tr("Scale to 100%"), DocumentView::ScaleTo100);
    m_scalingComboBox->addItem(tr("Scale to 125%"), DocumentView::ScaleTo125);
    m_scalingComboBox->addItem(tr("Scale to 150%"), DocumentView::ScaleTo150);
    m_scalingComboBox->addItem(tr("Scale to 200%"), DocumentView::ScaleTo200);
    m_scalingComboBox->addItem(tr("Scale to 400%"), DocumentView::ScaleTo400);

    connect(m_scalingComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changeScalingIndex(int)));

    m_scalingWidget->setLayout(new QHBoxLayout());
    m_scalingWidget->layout()->addWidget(m_scalingLabel);
    m_scalingWidget->layout()->addWidget(m_scalingComboBox);
    m_scalingWidget->setMaximumWidth(300);

    // rotationWidget

    m_rotationWidget = new QWidget();
    m_rotationLabel = new QLabel(tr("Rotation:"));
    m_rotationComboBox = new QComboBox();

    m_rotationComboBox->addItem(trUtf8("Rotate by 0°"), DocumentView::RotateBy0);
    m_rotationComboBox->addItem(trUtf8("Rotate by 90°"), DocumentView::RotateBy90);
    m_rotationComboBox->addItem(trUtf8("Rotate by 180°"), DocumentView::RotateBy180);
    m_rotationComboBox->addItem(trUtf8("Rotate by 270°"), DocumentView::RotateBy270);

    connect(m_rotationComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changeRotationIndex(int)));

    m_rotationWidget->setLayout(new QHBoxLayout());
    m_rotationWidget->layout()->addWidget(m_rotationLabel);
    m_rotationWidget->layout()->addWidget(m_rotationComboBox);
    m_rotationWidget->setMaximumWidth(300);

    // searchWidget

    m_searchWidget = new QWidget();
    m_searchLabel = new QLabel(tr("Search:"));
    m_searchLineEdit = new QLineEdit();
    m_matchCaseCheckBox = new QCheckBox(tr("Match &case"));
    m_highlightAllCheckBox = new QCheckBox(tr("Highlight &all"));
    m_searchTimer = new QTimer(this);
    m_findPreviousButton = new QPushButton(tr("Find &previous"));
    m_findNextButton = new QPushButton(tr("Find &next"));

    m_matchCaseCheckBox->setChecked(m_settings.value("mainWindow/matchCase", true).toBool());
    m_highlightAllCheckBox->setChecked(m_settings.value("mainWindow/highlightAll", false).toBool());

    m_searchTimer->setInterval(1000);
    m_searchTimer->setSingleShot(true);

    connect(m_searchLineEdit, SIGNAL(textEdited(QString)), this, SLOT(searchStart()));
    connect(m_searchTimer, SIGNAL(timeout()), this, SLOT(searchTimeout()));
    connect(m_findPreviousButton, SIGNAL(clicked()), this, SLOT(findPrevious()));
    connect(m_findNextButton, SIGNAL(clicked()), this, SLOT(findNext()));

    m_searchWidget->setLayout(new QHBoxLayout());
    m_searchWidget->layout()->addWidget(m_searchLabel);
    m_searchWidget->layout()->addWidget(m_searchLineEdit);
    m_searchWidget->layout()->addWidget(m_matchCaseCheckBox);
    m_searchWidget->layout()->addWidget(m_highlightAllCheckBox);
    m_searchWidget->layout()->addWidget(m_findPreviousButton);
    m_searchWidget->layout()->addWidget(m_findNextButton);

    // fileToolBar

    m_fileToolBar = new QToolBar(tr("&File"));
    m_fileToolBar->setObjectName("fileToolBar");
    m_fileToolBar->addAction(m_openAction);
    m_fileToolBar->addAction(m_refreshAction);
    m_fileToolBar->addAction(m_saveCopyAction);
    m_fileToolBar->addAction(m_printAction);
    this->addToolBar(Qt::TopToolBarArea, m_fileToolBar);

    // editToolBar

    m_editToolBar = new QToolBar(tr("&Edit"));
    m_editToolBar->setObjectName("editToolBar");
    m_editToolBar->addAction(m_firstPageAction);
    m_editToolBar->addAction(m_previousPageAction);
    m_editToolBar->addWidget(m_currentPageWidget);
    m_editToolBar->addAction(m_nextPageAction);
    m_editToolBar->addAction(m_lastPageAction);
    this->addToolBar(Qt::TopToolBarArea, m_editToolBar);

    // searchToolBar

    m_searchToolBar = new QToolBar(tr("&Search"));
    m_searchToolBar->setObjectName("searchToolBar");
    m_searchToolBar->setHidden(true);
    m_searchToolBar->setMovable(false);
    m_searchToolBar->addWidget(m_searchWidget);
    this->addToolBar(Qt::BottomToolBarArea, m_searchToolBar);

    // viewToolBar

    m_viewToolBar = new QToolBar(tr("&View"));
    m_viewToolBar->setObjectName("viewToolBar");
    m_viewToolBar->setHidden(true);
    m_viewToolBar->addWidget(m_pageLayoutWidget);
    m_viewToolBar->addWidget(m_scalingWidget);
    m_viewToolBar->addWidget(m_rotationWidget);
    this->addToolBar(Qt::TopToolBarArea, m_viewToolBar);
}

void MainWindow::createDocks()
{
    // outlineView

    m_outlineView = new OutlineView(this);
    this->addDockWidget(Qt::LeftDockWidgetArea, m_outlineView);
    m_outlineView->close();

    // thumbnailsView

    m_thumbnailsView = new ThumbnailsView(this);
    this->addDockWidget(Qt::RightDockWidgetArea, m_thumbnailsView);
    m_thumbnailsView->close();
}

void MainWindow::createMenus()
{
    m_fileMenu = this->menuBar()->addMenu(tr("&File"));
    m_fileMenu->addAction(m_openAction);
    m_fileMenu->addAction(m_refreshAction);
    m_fileMenu->addAction(m_saveCopyAction);
    m_fileMenu->addAction(m_printAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_exitAction);

    m_editMenu = this->menuBar()->addMenu(tr("&Edit"));
    m_editMenu->addAction(m_nextPageAction);
    m_editMenu->addAction(m_previousPageAction);
    m_editMenu->addAction(m_firstPageAction);
    m_editMenu->addAction(m_lastPageAction);
    m_editMenu->addSeparator();
    m_editMenu->addAction(m_searchAction);
    m_editMenu->addAction(m_findPreviousAction);
    m_editMenu->addAction(m_findNextAction);
    m_editMenu->addAction(m_copyTextAction);
    m_editMenu->addSeparator();
    m_editMenu->addAction(m_editSettingsAction);

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
    QMenu *toolbarMenu = m_viewMenu->addMenu(tr("&Toolbars"));
    toolbarMenu->addAction(m_fileToolBar->toggleViewAction());
    toolbarMenu->addAction(m_editToolBar->toggleViewAction());
    toolbarMenu->addAction(m_viewToolBar->toggleViewAction());
    QMenu *dockMenu = m_viewMenu->addMenu(tr("&Docks"));
    dockMenu->addAction(m_outlineView->toggleViewAction());
    dockMenu->addAction(m_thumbnailsView->toggleViewAction());
    m_viewMenu->addAction(m_fullscreenAction);

    m_tabMenu = this->menuBar()->addMenu(tr("&Tab"));
    m_tabMenu->addAction(m_addTabAction);
    m_tabMenu->addAction(m_previousTabAction);
    m_tabMenu->addAction(m_nextTabAction);
    m_tabMenu->addAction(m_closeTabAction);
    m_tabMenu->addSeparator();
    m_tabMenu->addAction(m_closeAllTabsAction);
    m_tabMenu->addAction(m_closeAllTabsButCurrentAction);
    m_tabMenu->addSeparator();

    m_helpMenu = this->menuBar()->addMenu(tr("&Help"));
    m_helpMenu->addAction(m_aboutAction);
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
            DocumentView *documentView = static_cast<DocumentView*>(m_tabWidget->currentWidget());

            if(documentView->open(filePath))
            {
                m_tabWidget->setTabText(m_tabWidget->currentIndex(), QFileInfo(filePath).baseName());
                m_tabWidget->setTabToolTip(m_tabWidget->currentIndex(), QFileInfo(filePath).baseName());

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
        DocumentView *documentView = static_cast<DocumentView*>(m_tabWidget->currentWidget());

        documentView->refresh();
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
            DocumentView *documentView = static_cast<DocumentView*>(m_tabWidget->currentWidget());

            if(!documentView->saveCopy(filePath))
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
        DocumentView *documentView = static_cast<DocumentView*>(m_tabWidget->currentWidget());

        documentView->print();
    }
}


void MainWindow::search()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        if(m_searchToolBar->isHidden())
        {
            m_searchToolBar->setHidden(false);
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
        if(!m_searchToolBar->isHidden() && !m_searchLineEdit->text().isEmpty())
        {
            m_searchTimer->start();
        }
    }
}

void MainWindow::searchTimeout()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        if(!m_searchToolBar->isHidden() && !m_searchLineEdit->text().isEmpty())
        {
            DocumentView *documentView = static_cast<DocumentView*>(m_tabWidget->currentWidget());

            documentView->search(m_searchLineEdit->text(), m_matchCaseCheckBox->isChecked(), m_highlightAllCheckBox->isChecked());

            this->statusBar()->show();
        }
    }
}

void MainWindow::findPrevious()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        if(m_searchToolBar->isHidden())
        {
            m_searchToolBar->setHidden(false);

            m_searchLineEdit->setFocus();
        }
        else
        {
            if(!m_searchLineEdit->text().isEmpty())
            {
                DocumentView *documentView = static_cast<DocumentView*>(m_tabWidget->currentWidget());

                documentView->findPrevious();
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
            m_searchToolBar->setHidden(false);

            m_searchLineEdit->setFocus();
        }
        else
        {
            if(!m_searchLineEdit->text().isEmpty())
            {
                DocumentView *documentView = static_cast<DocumentView*>(m_tabWidget->currentWidget());

                documentView->findNext();
            }
        }
    }
}

void MainWindow::copyText()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *documentView = static_cast<DocumentView*>(m_tabWidget->currentWidget());

        documentView->copyText();
    }
}

void MainWindow::editSettings()
{
    SettingsDialog settingsDialog;

    settingsDialog.exec();
}


void MainWindow::previousPage()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *documentView = static_cast<DocumentView*>(m_tabWidget->currentWidget());

        documentView->previousPage();
    }
}

void MainWindow::nextPage()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *documentView = static_cast<DocumentView*>(m_tabWidget->currentWidget());

        documentView->nextPage();
    }
}

void MainWindow::firstPage()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *documentView = static_cast<DocumentView*>(m_tabWidget->currentWidget());

        documentView->firstPage();
    }
}

void MainWindow::lastPage()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *documentView = static_cast<DocumentView*>(m_tabWidget->currentWidget());

        documentView->lastPage();
    }
}

void MainWindow::changeCurrentPage()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *documentView = static_cast<DocumentView*>(m_tabWidget->currentWidget());

        documentView->setCurrentPage(m_currentPageLineEdit->text().toInt());
    }
}


void MainWindow::selectPageLayout(QAction *pageLayoutAction)
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *documentView = static_cast<DocumentView*>(m_tabWidget->currentWidget());

        if(pageLayoutAction == m_onePageAction)
        {
            documentView->setPageLayout(DocumentView::OnePage);
        }
        else if(pageLayoutAction == m_twoPagesAction)
        {
            documentView->setPageLayout(DocumentView::TwoPages);
        }
        else if(pageLayoutAction == m_oneColumnAction)
        {
            documentView->setPageLayout(DocumentView::OneColumn);
        }
        else if(pageLayoutAction == m_twoColumnsAction)
        {
            documentView->setPageLayout(DocumentView::TwoColumns);
        }
    }
}

void MainWindow::changePageLayoutIndex(const int &index)
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *documentView = static_cast<DocumentView*>(m_tabWidget->currentWidget());

        documentView->setPageLayout(static_cast<DocumentView::PageLayout>(m_pageLayoutComboBox->itemData(index).toUInt()));
    }
}


void MainWindow::selectScaling(QAction *scalingAction)
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *documentView = static_cast<DocumentView*>(m_tabWidget->currentWidget());

        if(scalingAction == m_fitToPageAction)
        {
            documentView->setScaling(DocumentView::FitToPage);
        }
        else if(scalingAction == m_fitToPageWidthAction)
        {
            documentView->setScaling(DocumentView::FitToPageWidth);
        }
        else if(scalingAction == m_scaleTo50Action)
        {
            documentView->setScaling(DocumentView::ScaleTo50);            
        }
        else if(scalingAction == m_scaleTo75Action)
        {
            documentView->setScaling(DocumentView::ScaleTo75);
        }
        else if(scalingAction == m_scaleTo100Action)
        {
            documentView->setScaling(DocumentView::ScaleTo100);
        }
        else if(scalingAction == m_scaleTo125Action)
        {
            documentView->setScaling(DocumentView::ScaleTo125);
        }
        else if(scalingAction == m_scaleTo150Action)
        {
            documentView->setScaling(DocumentView::ScaleTo150);
        }
        else if(scalingAction == m_scaleTo200Action)
        {
            documentView->setScaling(DocumentView::ScaleTo200);
        }
        else if(scalingAction == m_scaleTo400Action)
        {
            documentView->setScaling(DocumentView::ScaleTo400);
        }
    }
}

void MainWindow::changeScalingIndex(const int &index)
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *documentView = static_cast<DocumentView*>(m_tabWidget->currentWidget());

        documentView->setScaling(static_cast<DocumentView::Scaling>(m_scalingComboBox->itemData(index).toUInt()));
    }
}

void MainWindow::selectRotation(QAction *rotationAction)
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *documentView = static_cast<DocumentView*>(m_tabWidget->currentWidget());

        if(rotationAction == m_rotateBy0Action)
        {
            documentView->setRotation(DocumentView::RotateBy0);
        }
        else if(rotationAction == m_rotateBy90Action)
        {
            documentView->setRotation(DocumentView::RotateBy90);
        }
        else if(rotationAction == m_rotateBy180Action)
        {
            documentView->setRotation(DocumentView::RotateBy180);
        }
        else if(rotationAction == m_rotateBy270Action)
        {
            documentView->setRotation(DocumentView::RotateBy270);
        }
    }
}

void MainWindow::changeRotationIndex(const int &index)
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *documentView = static_cast<DocumentView*>(m_tabWidget->currentWidget());

        documentView->setRotation(static_cast<DocumentView::Rotation>(m_rotationComboBox->itemData(index).toUInt()));
    }
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


void MainWindow::addTab()
{
    QStringList filePaths = QFileDialog::getOpenFileNames(this, tr("Open documents"),
                                                          m_settings.value("mainWindow/path", QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation)).toString(),
                                                          tr("Portable Document Format (*.pdf)"));

    foreach(QString filePath, filePaths)
    {
        DocumentView *documentView = new DocumentView();

        if(documentView->open(filePath))
        {
            int index = m_tabWidget->addTab(documentView, QFileInfo(filePath).baseName());
            m_tabWidget->setTabToolTip(index, QFileInfo(filePath).baseName());
            m_tabWidget->setCurrentIndex(index);

            m_tabMenu->addAction(documentView->tabMenuAction());

            connect(documentView, SIGNAL(filePathChanged(QString)), this, SLOT(updateFilePath(QString)));
            connect(documentView, SIGNAL(currentPageChanged(int)), this, SLOT(updateCurrentPage(int)));
            connect(documentView, SIGNAL(numberOfPagesChanged(int)), this, SLOT(updateNumberOfPages(int)));
            connect(documentView, SIGNAL(pageLayoutChanged(DocumentView::PageLayout)), this, SLOT(updatePageLayout(DocumentView::PageLayout)));
            connect(documentView, SIGNAL(scalingChanged(DocumentView::Scaling)), this, SLOT(updateScaling(DocumentView::Scaling)));
            connect(documentView, SIGNAL(rotationChanged(DocumentView::Rotation)), this, SLOT(updateRotation(DocumentView::Rotation)));

            connect(documentView, SIGNAL(searchingProgressed(int)), this, SLOT(updateSearchProgress(int)));
            connect(documentView, SIGNAL(searchingCanceled()), this, SLOT(updateSearchProgress()));
            connect(documentView, SIGNAL(searchingFinished()), this, SLOT(updateSearchProgress()));

            m_settings.setValue("mainWindow/path", QFileInfo(filePath).path());
        }
        else
        {
            delete documentView;

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
        DocumentView *documentView = static_cast<DocumentView*>(m_tabWidget->currentWidget());

        delete documentView;
    }
}


void MainWindow::closeAllTabs()
{
    while(m_tabWidget->count() > 0)
    {

        DocumentView *documentView = static_cast<DocumentView*>(m_tabWidget->widget(0));

        delete documentView;
    }
}

void MainWindow::closeAllTabsButCurrent()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        DocumentView *documentView = static_cast<DocumentView*>(m_tabWidget->currentWidget());

        m_tabWidget->removeTab(m_tabWidget->currentIndex());

        closeAllTabs();

        int index = m_tabWidget->addTab(documentView, QFileInfo(documentView->filePath()).baseName());
        m_tabWidget->setTabToolTip(index, QFileInfo(documentView->filePath()).baseName());
        m_tabWidget->setCurrentIndex(index);
    }
}


void MainWindow::changeCurrentTab(const int &index)
{
    if(index != -1)
    {
        DocumentView *documentView = static_cast<DocumentView*>(m_tabWidget->currentWidget());

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
        m_copyTextAction->setEnabled(true);

        m_pageLayoutGroup->setEnabled(true);
        m_scalingGroup->setEnabled(true);
        m_rotationGroup->setEnabled(true);

        m_previousTabAction->setEnabled(true);
        m_nextTabAction->setEnabled(true);
        m_closeTabAction->setEnabled(true);

        m_editToolBar->setEnabled(true);
        m_searchToolBar->setEnabled(true);
        m_viewToolBar->setEnabled(true);

        this->updateCurrentPage(documentView->currentPage());
        this->updateNumberOfPages(documentView->numberOfPages());
        this->updatePageLayout(documentView->pageLayout());
        this->updateScaling(documentView->scaling());
        this->updateRotation(documentView->rotation());

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
        m_copyTextAction->setEnabled(false);

        m_onePageAction->setChecked(true);
        m_pageLayoutGroup->setEnabled(false);

        m_scaleTo100Action->setChecked(true);
        m_scalingGroup->setEnabled(false);

        m_rotateBy0Action->setChecked(true);
        m_rotationGroup->setEnabled(false);

        m_previousTabAction->setEnabled(false);
        m_nextTabAction->setEnabled(false);
        m_closeTabAction->setEnabled(false);

        m_currentPageLineEdit->setText(QString());
        m_numberOfPagesLabel->setText(QString());
        m_editToolBar->setEnabled(false);

        m_searchToolBar->setEnabled(false);
        m_searchToolBar->setHidden(true);

        m_pageLayoutComboBox->setCurrentIndex(0);
        m_scalingComboBox->setCurrentIndex(4);
        m_rotationComboBox->setCurrentIndex(0);
        m_viewToolBar->setEnabled(false);

        m_outlineView->setDocumentView(0);
        m_thumbnailsView->setDocumentView(0);
    }
}

void MainWindow::requestTabClose(const int &index)
{
    DocumentView *documentView = static_cast<DocumentView*>(m_tabWidget->widget(index));

    delete documentView;
}


void MainWindow::about()
{
    QMessageBox::about(this, tr("About qpdfview"), tr("<p><b>qpdfview</b></p><p>qpdfview is a tabbed PDF viewer using the poppler library.</p><p>&copy; 2012 Adam Reichold</p>"));
}

void MainWindow::updateFilePath(const QString &filePath)
{
    if(m_tabWidget->currentIndex() != -1)
    {
        m_tabWidget->setTabText(m_tabWidget->currentIndex(), QFileInfo(filePath).baseName());
        m_tabWidget->setTabToolTip(m_tabWidget->currentIndex(), QFileInfo(filePath).baseName());
    }
}

void MainWindow::updateCurrentPage(const int &currentPage)
{
    if(m_tabWidget->currentIndex() != -1)
    {
        m_currentPageLineEdit->setText(tr("%1").arg(currentPage));
    }
}

void MainWindow::updateNumberOfPages(const int &numberOfPages)
{
    if(m_tabWidget->currentIndex() != -1)
    {
        m_currentPageValidator->setRange(1, numberOfPages);
        m_numberOfPagesLabel->setText(tr(" of %1").arg(numberOfPages));
    }
}

void MainWindow::updatePageLayout(const DocumentView::PageLayout &pageLayout)
{
    if(m_tabWidget->currentIndex() != -1)
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
}

void MainWindow::updateScaling(const DocumentView::Scaling &scaling)
{
    if(m_tabWidget->currentIndex() != -1)
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
}

void MainWindow::updateRotation(const DocumentView::Rotation &rotation)
{
    if(m_tabWidget->currentIndex() != -1)
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
}


void MainWindow::updateSearchProgress(int value)
{
    this->statusBar()->showMessage(tr("Searched %1% of the current document...").arg(value));
}

void MainWindow::updateSearchProgress()
{
    this->statusBar()->clearMessage();

    this->statusBar()->hide();
}

void MainWindow::keyPressEvent(QKeyEvent *keyEvent)
{
    if(keyEvent->key() == Qt::Key_Escape)
    {
        if(m_tabWidget->currentIndex() != -1)
        {
            if(!m_searchToolBar->isHidden())
            {
                m_searchToolBar->setHidden(true);

                m_searchLineEdit->clear();

                DocumentView *documentView = static_cast<DocumentView*>(m_tabWidget->currentWidget());

                documentView->clearResults();
            }
        }
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
                DocumentView *documentView = new DocumentView();

                if(documentView->open(url.path()))
                {
                    int index = m_tabWidget->addTab(documentView, QFileInfo(url.path()).baseName());
                    m_tabWidget->setTabToolTip(index, QFileInfo(url.path()).baseName());
                    m_tabWidget->setCurrentIndex(index);

                    m_tabMenu->addAction(documentView->tabMenuAction());

                    connect(documentView, SIGNAL(filePathChanged(QString)), this, SLOT(updateFilePath(QString)));
                    connect(documentView, SIGNAL(currentPageChanged(int)), this, SLOT(updateCurrentPage(int)));
                    connect(documentView, SIGNAL(numberOfPagesChanged(int)), this, SLOT(updateNumberOfPages(int)));
                    connect(documentView, SIGNAL(pageLayoutChanged(DocumentView::PageLayout)), this, SLOT(updatePageLayout(DocumentView::PageLayout)));
                    connect(documentView, SIGNAL(scalingChanged(DocumentView::Scaling)), this, SLOT(updateScaling(DocumentView::Scaling)));
                    connect(documentView, SIGNAL(rotationChanged(DocumentView::Rotation)), this, SLOT(updateRotation(DocumentView::Rotation)));

                    connect(documentView, SIGNAL(searchingProgressed(int)), this, SLOT(updateSearchProgress(int)));
                    connect(documentView, SIGNAL(searchingCanceled()), this, SLOT(updateSearchProgress()));
                    connect(documentView, SIGNAL(searchingFinished()), this, SLOT(updateSearchProgress()));
                }
                else
                {
                    delete documentView;
                }
            }
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *closeEvent)
{
    m_settings.setValue("mainWindow/matchCase", m_matchCaseCheckBox->isChecked());
    m_settings.setValue("mainWindow/highlightAll", m_highlightAllCheckBox->isChecked());

    if(m_fullscreenAction->isChecked())
    {
        m_settings.setValue("mainWindow/geometry", m_normalGeometry);
    }
    else
    {
        m_settings.setValue("mainWindow/geometry", this->saveGeometry());
    }

    m_searchToolBar->setHidden(true);

    m_settings.setValue("mainWindow/state", this->saveState());

    QMainWindow::closeEvent(closeEvent);
}
