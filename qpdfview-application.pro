include(qpdfview.pri)

TARGET = qpdfview
TEMPLATE = app

OBJECTS_DIR = objects
MOC_DIR = moc

HEADERS += \
    sources/global.h \
    sources/printoptions.h \
    sources/settings.h \
    sources/model.h \
    sources/pluginhandler.h \
    sources/shortcuthandler.h \
    sources/rendertask.h \
    sources/pageitem.h \
    sources/presentationview.h \
    sources/searchtask.h \
    sources/miscellaneous.h \
    sources/documentlayout.h \
    sources/documentview.h \
    sources/printdialog.h \
    sources/settingsdialog.h \
    sources/helpdialog.h \
    sources/recentlyusedmenu.h \
    sources/bookmarkmenu.h \
    sources/database.h \
    sources/mainwindow.h

SOURCES += \
    sources/settings.cpp \
    sources/pluginhandler.cpp \
    sources/shortcuthandler.cpp \
    sources/rendertask.cpp \
    sources/pageitem.cpp \
    sources/presentationview.cpp \
    sources/searchtask.cpp \
    sources/miscellaneous.cpp \
    sources/documentlayout.cpp \
    sources/documentview.cpp \
    sources/printdialog.cpp \
    sources/settingsdialog.cpp \
    sources/helpdialog.cpp \
    sources/recentlyusedmenu.cpp \
    sources/bookmarkmenu.cpp \
    sources/database.cpp \
    sources/mainwindow.cpp \
    sources/main.cpp

DEFINES += APPLICATION_VERSION=\\\"$${APPLICATION_VERSION}\\\"

QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

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

!without_djvu {
    DEFINES += WITH_DJVU

    static_djvu_plugin {
        isEmpty(DJVU_PLUGIN_NAME):DJVU_PLUGIN_NAME = libqpdfview_djvu.a

        DEFINES += STATIC_DJVU_PLUGIN
        LIBS += $$DJVU_PLUGIN_NAME
        PRE_TARGETDEPS += $$DJVU_PLUGIN_NAME

        !without_pkgconfig {
            CONFIG += link_pkgconfig
            PKGCONFIG += ddjvuapi
        }
    } else {
        isEmpty(DJVU_PLUGIN_NAME):DJVU_PLUGIN_NAME = libqpdfview_djvu.so
    }

    DEFINES += DJVU_PLUGIN_NAME=\\\"$${DJVU_PLUGIN_NAME}\\\"
}

!without_cups {
    DEFINES += WITH_CUPS

    !isEmpty(CUPS_LIBS) {
        LIBS += $$CUPS_LIBS
    } else {
        LIBS += $$system(cups-config --libs)
    }
}

!without_synctex {
    DEFINES += WITH_SYNCTEX
    LIBS += -lz

    INCLUDEPATH += synctex
    SOURCES += synctex/synctex_parser.c synctex/synctex_parser_utils.c
}

lessThan(QT_MAJOR_VERSION, 5) : !without_magic {
    DEFINES += WITH_MAGIC
    LIBS += -lmagic
}

!without_signals {
    DEFINES += WITH_SIGNALS

    HEADERS += sources/signalhandler.h
    SOURCES += sources/signalhandler.cpp
}

DEFINES += DATA_INSTALL_PATH=\\\"$${DATA_INSTALL_PATH}\\\"


DESKTOP_FILE = miscellaneous/qpdfview.desktop
!without_pdf:MIME_TYPES += application/pdf application/x-pdf text/pdf text/x-pdf image/pdf image/x-pdf
!without_ps:MIME_TYPES += application/postscript
!without_djvu:MIME_TYPES += image/vnd.djvu image/x-djvu

system("sed -e \"s:DATA_INSTALL_PATH:$${DATA_INSTALL_PATH}:\" -e \"s:MIME_TYPES:$$join(MIME_TYPES,";","",";"):\" $${DESKTOP_FILE}.in > $${DESKTOP_FILE}")


target.path = $${TARGET_INSTALL_PATH}

data.files = icons/qpdfview.svg translations/*.qm miscellaneous/help*.html
data.path = $${DATA_INSTALL_PATH}

launcher.files = $${DESKTOP_FILE}
launcher.path = $${LAUNCHER_INSTALL_PATH}

manual.files = miscellaneous/qpdfview.1
manual.path = $${MANUAL_INSTALL_PATH}

INSTALLS += target data launcher manual

INCLUDEPATH += icons
win32:RC_FILE = icons/qpdfview_win32.rc
os2:RC_FILE = icons/qpdfview_os2.rc
