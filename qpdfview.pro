include(qpdfview.pri)

TARGET = qpdfview
TEMPLATE = app

HEADERS += \
    sources/annotationdialog.h \
    sources/formfielddialog.h \
    sources/pageitem.h \
    sources/searchthread.h \
    sources/presentationview.h \
    sources/documentview.h \
    sources/miscellaneous.h \
    sources/settingsdialog.h \
    sources/printoptionswidget.h \
    sources/recentlyusedmenu.h \
    sources/bookmarkmenu.h \
    sources/mainwindow.h

SOURCES += \
    sources/annotationdialog.cpp \
    sources/formfielddialog.cpp \
    sources/pageitem.cpp \
    sources/searchthread.cpp \
    sources/presentationview.cpp \
    sources/documentview.cpp \
    sources/miscellaneous.cpp \
    sources/settingsdialog.cpp \
    sources/printoptionswidget.cpp \
    sources/recentlyusedmenu.cpp \
    sources/bookmarkmenu.cpp \
    sources/mainwindow.cpp \
    sources/main.cpp

TRANSLATIONS += \
    translations/qpdfview_ast.ts \
    translations/qpdfview_cs.ts \
    translations/qpdfview_da.ts \
    translations/qpdfview_de.ts \
    translations/qpdfview_el.ts \
    translations/qpdfview_es.ts \
    translations/qpdfview_fi.ts \
    translations/qpdfview_fr.ts \
    translations/qpdfview_he.ts \
    translations/qpdfview_hr.ts \
    translations/qpdfview_pt_BR.ts \
    translations/qpdfview_ru.ts \
    translations/qpdfview_sk.ts \
    translations/qpdfview_tr.ts \
    translations/qpdfview_ug.ts \
    translations/qpdfview_uk.ts

QT += core xml gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

!without_svg {
    DEFINES += WITH_SVG
    QT += svg

    RESOURCES += qpdfview.qrc
}

!without_dbus {
    DEFINES += WITH_DBUS
    QT += dbus
}

!without_pkgconfig {
    CONFIG += link_pkgconfig
    PKGCONFIG += poppler-qt4

    system(pkg-config --atleast-version=0.14 poppler-qt4):DEFINES += HAS_POPPLER_14
    system(pkg-config --atleast-version=0.20.1 poppler-qt4):DEFINES += HAS_POPPLER_20
    system(pkg-config --atleast-version=0.22 poppler-qt4):DEFINES += HAS_POPPLER_22
}

!without_cups {
    DEFINES += WITH_CUPS
    LIBS += $$system(cups-config --libs)
}

!without_synctex {
    DEFINES += WITH_SYNCTEX
    LIBS += -lz

    INCLUDEPATH += synctex
    SOURCES += synctex/synctex_parser.c synctex/synctex_parser_utils.c
}

target.path = $${TARGET_INSTALL_PATH}

data.files = icons/qpdfview.svg translations/*.qm miscellaneous/help.html
data.path = $${DATA_INSTALL_PATH}

DEFINES += DATA_INSTALL_PATH=\\\"$${DATA_INSTALL_PATH}\\\"

launcher.files = miscellaneous/qpdfview.desktop
launcher.path = $${LAUNCHER_INSTALL_PATH}

system(sed \"s/DATA_INSTALL_PATH/$$replace(DATA_INSTALL_PATH, "/", "\\/")/\" miscellaneous/qpdfview.desktop.in > miscellaneous/qpdfview.desktop)

manual.files = miscellaneous/qpdfview.1
manual.path = $${MANUAL_INSTALL_PATH}

INSTALLS += target data launcher manual
