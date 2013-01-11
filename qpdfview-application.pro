include(qpdfview.pri)

TARGET = qpdfview
TEMPLATE = app

OBJECTS_DIR = objects
MOC_DIR = moc

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

QT += core gui

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

DEFINES += PLUGIN_INSTALL_PATH=\\\"$${PLUGIN_INSTALL_PATH}\\\"

!without_pdf {
    DEFINES += WITH_PDF

    static_pdf_plugin {
        isEmpty(PDF_PLUGIN_NAME):PDF_PLUGIN_NAME = libqpdfview_pdf.a

        DEFINES += STATIC_PDF_PLUGIN
        LIBS += $$PDF_PLUGIN_NAME
        PRE_TARGETDEPS += $$PDF_PLUGIN_NAME

        QT += xml

        !without_pkgconfig {
            CONFIG += link_pkgconfig
            PKGCONFIG += poppler-qt4
        }
    } else {
        isEmpty(PDF_PLUGIN_NAME):PDF_PLUGIN_NAME = libqpdfview_pdf.so
    }

    DEFINES += PDF_PLUGIN_NAME=\\\"$${PDF_PLUGIN_NAME}\\\"
}

!without_ps {
    DEFINES += WITH_PS

    static_ps_plugin {
        isEmpty(PS_PLUGIN_NAME):PS_PLUGIN_NAME = libqpdfview_ps.a

        DEFINES += STATIC_PS_PLUGIN
        LIBS += $$PS_PLUGIN_NAME
        PRE_TARGETDEPS += $$PS_PLUGIN_NAME

        !without_pkgconfig {
            CONFIG += link_pkgconfig
            PKGCONFIG += libspectre
        }
    } else {
        isEmpty(PS_PLUGIN_NAME):PS_PLUGIN_NAME = libqpdfview_ps.so
    }

    DEFINES += PS_PLUGIN_NAME=\\\"$${PS_PLUGIN_NAME}\\\"
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
