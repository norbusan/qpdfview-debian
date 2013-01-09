include(qpdfview.pri)

TARGET = qpdfview
TEMPLATE = app

HEADERS += \
    sources/global.h \
    sources/model.h \
    sources/pageitem.h \
    sources/searchthread.h \
    sources/presentationview.h \
    sources/printoptions.h \
    sources/documentview.h \
    sources/printoptionswidget.h \
    sources/miscellaneous.h \
    sources/settings.h \
    sources/settingsdialog.h \
    sources/recentlyusedmenu.h \
    sources/bookmarkmenu.h \
    sources/mainwindow.h

SOURCES += \
    sources/model.cpp \
    sources/pageitem.cpp \
    sources/searchthread.cpp \
    sources/presentationview.cpp \
    sources/documentview.cpp \
    sources/printoptionswidget.cpp \
    sources/miscellaneous.cpp \
    sources/settings.cpp \
    sources/settingsdialog.cpp \
    sources/recentlyusedmenu.cpp \
    sources/bookmarkmenu.cpp \
    sources/mainwindow.cpp \
    sources/main.cpp

PDF_HEADERS = sources/pdfmodel.h sources/annotationdialog.h sources/formfielddialog.h
PDF_SOURCES = sources/pdfmodel.cpp sources/annotationdialog.cpp sources/formfielddialog.cpp

PS_HEADERS = sources/psmodel.h
PS_SOURCES = sources/psmodel.cpp

TRANSLATIONS += \
    translations/qpdfview_ast.ts \
    translations/qpdfview_bs.ts \
    translations/qpdfview_ca.ts \
    translations/qpdfview_cs.ts \
    translations/qpdfview_da.ts \
    translations/qpdfview_de.ts \
    translations/qpdfview_el.ts \
    translations/qpdfview_en_GB.ts \
    translations/qpdfview_es.ts \
    translations/qpdfview_eu.ts \
    translations/qpdfview_fi.ts \
    translations/qpdfview_fr.ts \
    translations/qpdfview_he.ts \
    translations/qpdfview_hr.ts \
    translations/qpdfview_id.ts \
    translations/qpdfview_it.ts \
    translations/qpdfview_ky.ts \
    translations/qpdfview_my.ts \
    translations/qpdfview_pl.ts \
    translations/qpdfview_pt_BR.ts \
    translations/qpdfview_ro.ts \
    translations/qpdfview_ru.ts \
    translations/qpdfview_sk.ts \
    translations/qpdfview_tr.ts \
    translations/qpdfview_ug.ts \
    translations/qpdfview_uk.ts \
    translations/qpdfview_zh_CN.ts

QT += core xml gui

greaterThan(QT_MAJOR_VERSION, 4): QT += concurrent widgets printsupport

!without_svg {
    DEFINES += WITH_SVG
    QT += svg

    RESOURCES += qpdfview.qrc
}

!without_sql {
    DEFINES += WITH_SQL
    QT += sql
}

!without_dbus {
    DEFINES += WITH_DBUS
    QT += dbus
}

!without_pkgconfig {
    CONFIG += link_pkgconfig

    !without_pdf : system(pkg-config --exists poppler-qt4) {
        HEADERS += $$PDF_HEADERS
        SOURCES += $$PDF_SOURCES

        DEFINES += WITH_PDF
        PKGCONFIG += poppler-qt4

        system(pkg-config --atleast-version=0.14 poppler-qt4):DEFINES += HAS_POPPLER_14
        system(pkg-config --atleast-version=0.20.1 poppler-qt4):DEFINES += HAS_POPPLER_20
        system(pkg-config --atleast-version=0.22 poppler-qt4):DEFINES += HAS_POPPLER_22
    }

    !without_ps : system(pkg-config --exists libspectre) {
        HEADERS += $$PS_HEADERS
        SOURCES += $$PS_SOURCES

        DEFINES += WITH_PS
        PKGCONFIG += libspectre
    }
} else {
    !without_pdf {
        HEADERS += $$PDF_HEADERS
        SOURCES += $$PDF_SOURCES

        DEFINES += WITH_PDF
    }

    !without_ps {
        HEADERS += $$PS_HEADERS
        SOURCES += $$PS_SOURCES

        DEFINES += WITH_PS
    }
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

!without_signals {
    DEFINES += WITH_SIGNALS

    HEADERS += sources/signalhandler.h
    SOURCES += sources/signalhandler.cpp
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
