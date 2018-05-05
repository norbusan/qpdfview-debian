include(qpdfview.pri)

TARGET = qpdfview
TEMPLATE = app

OBJECTS_DIR = objects
MOC_DIR = moc

HEADERS += \
    sources/global.h \
    sources/renderparam.h \
    sources/printoptions.h \
    sources/settings.h \
    sources/model.h \
    sources/pluginhandler.h \
    sources/shortcuthandler.h \
    sources/rendertask.h \
    sources/tileitem.h \
    sources/pageitem.h \
    sources/thumbnailitem.h \
    sources/presentationview.h \
    sources/searchmodel.h \
    sources/searchitemdelegate.h \
    sources/searchtask.h \
    sources/miscellaneous.h \
    sources/documentlayout.h \
    sources/documentview.h \
    sources/printdialog.h \
    sources/settingsdialog.h \
    sources/fontsdialog.h \
    sources/helpdialog.h \
    sources/recentlyusedmenu.h \
    sources/recentlyclosedmenu.h \
    sources/bookmarkmodel.h \
    sources/bookmarkmenu.h \
    sources/bookmarkdialog.h \
    sources/database.h \
    sources/mainwindow.h

SOURCES += \
    sources/settings.cpp \
    sources/pluginhandler.cpp \
    sources/shortcuthandler.cpp \
    sources/rendertask.cpp \
    sources/tileitem.cpp \
    sources/pageitem.cpp \
    sources/thumbnailitem.cpp \
    sources/presentationview.cpp \
    sources/searchmodel.cpp \
    sources/searchitemdelegate.cpp \
    sources/searchtask.cpp \
    sources/miscellaneous.cpp \
    sources/documentlayout.cpp \
    sources/documentview.cpp \
    sources/printdialog.cpp \
    sources/settingsdialog.cpp \
    sources/fontsdialog.cpp \
    sources/helpdialog.cpp \
    sources/recentlyusedmenu.cpp \
    sources/recentlyclosedmenu.cpp \
    sources/bookmarkmenu.cpp \
    sources/bookmarkdialog.cpp \
    sources/bookmarkmodel.cpp \
    sources/database.cpp \
    sources/mainwindow.cpp \
    sources/main.cpp

DEFINES += APPLICATION_VERSION=\\\"$${APPLICATION_VERSION}\\\"

with_lto {
    QMAKE_CFLAG += -flto
    QMAKE_CXXFLAGS += -flto
    QMAKE_LFLAGS += -flto
}

QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += concurrent widgets printsupport

!without_svg {
    DEFINES += WITH_SVG
    QT += svg

    RESOURCES += icons.qrc
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

    !without_pkgconfig:POPPLER_VERSION = $$system(pkg-config --modversion poppler-qt$${QT_MAJOR_VERSION})
    DEFINES += POPPLER_VERSION=\\\"$${POPPLER_VERSION}\\\"

    static_pdf_plugin {
        isEmpty(PDF_PLUGIN_NAME):PDF_PLUGIN_NAME = libqpdfview_pdf.a

        DEFINES += STATIC_PDF_PLUGIN
        LIBS += $$PDF_PLUGIN_NAME $$PDF_PLUGIN_LIBS
        PRE_TARGETDEPS += $$PDF_PLUGIN_NAME

        QT += xml

        !without_pkgconfig {
            CONFIG += link_pkgconfig
            PKGCONFIG += poppler-qt$${QT_MAJOR_VERSION}
        }
    } else {
        isEmpty(PDF_PLUGIN_NAME):PDF_PLUGIN_NAME = libqpdfview_pdf.so
    }

    DEFINES += PDF_PLUGIN_NAME=\\\"$${PDF_PLUGIN_NAME}\\\"
}

!without_ps {
    DEFINES += WITH_PS

    !without_pkgconfig:LIBSPECTRE_VERSION = $$system(pkg-config --modversion libspectre)
    DEFINES += LIBSPECTRE_VERSION=\\\"$${LIBSPECTRE_VERSION}\\\"

    static_ps_plugin {
        isEmpty(PS_PLUGIN_NAME):PS_PLUGIN_NAME = libqpdfview_ps.a

        DEFINES += STATIC_PS_PLUGIN
        LIBS += $$PS_PLUGIN_NAME $$PS_PLUGIN_LIBS
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

    !without_pkgconfig:DJVULIBRE_VERSION = $$system(pkg-config --modversion ddjvuapi)
    DEFINES += DJVULIBRE_VERSION=\\\"$${DJVULIBRE_VERSION}\\\"

    static_djvu_plugin {
        isEmpty(DJVU_PLUGIN_NAME):DJVU_PLUGIN_NAME = libqpdfview_djvu.a

        DEFINES += STATIC_DJVU_PLUGIN
        LIBS += $$DJVU_PLUGIN_NAME $$DJVU_PLUGIN_LIBS
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

with_fitz {
    DEFINES += WITH_FITZ

    DEFINES += FITZ_VERSION=\\\"$${FITZ_VERSION}\\\"

    static_fitz_plugin {
        isEmpty(FITZ_PLUGIN_NAME):FITZ_PLUGIN_NAME = libqpdfview_fitz.a

        DEFINES += STATIC_FITZ_PLUGIN
        LIBS += $$FITZ_PLUGIN_NAME $$FITZ_PLUGIN_LIBS
        PRE_TARGETDEPS += $$FITZ_PLUGIN_NAME

        isEmpty(FITZ_PLUGIN_LIBS) {
            LIBS += -lmupdf -lfreetype -ljbig2dec -lopenjp2 -ljpeg -lz -lm
        } else {
            LIBS += $$FITZ_PLUGIN_LIBS
        }
    } else {
        isEmpty(FITZ_PLUGIN_NAME):FITZ_PLUGIN_NAME = libqpdfview_fitz.so
    }

    DEFINES += FITZ_PLUGIN_NAME=\\\"$${FITZ_PLUGIN_NAME}\\\"
}

!without_image {
    DEFINES += WITH_IMAGE

    static_image_plugin {
        isEmpty(IMAGE_PLUGIN_NAME):IMAGE_PLUGIN_NAME = libqpdfview_image.a

        DEFINES += STATIC_IMAGE_PLUGIN
        LIBS += $$IMAGE_PLUGIN_NAME $$IMAGE_PLUGIN_LIBS
        PRE_TARGETDEPS += $$IMAGE_PLUGIN_NAME
    }
    else {
        isEmpty(IMAGE_PLUGIN_NAME):IMAGE_PLUGIN_NAME = libqpdfview_image.so
    }

    DEFINES += IMAGE_PLUGIN_NAME=\\\"$${IMAGE_PLUGIN_NAME}\\\"
}

!without_cups {
    DEFINES += WITH_CUPS

    isEmpty(CUPS_VERSION):CUPS_VERSION = $$system(cups-config --version)
    isEmpty(CUPS_LIBS):CUPS_LIBS = $$system(cups-config --libs)

    DEFINES += CUPS_VERSION=\\\"$${CUPS_VERSION}\\\"
    LIBS += $$CUPS_LIBS
}

!without_synctex {
    DEFINES += WITH_SYNCTEX

    !without_pkgconfig:system(pkg-config --exists synctex) {
        CONFIG += link_pkgconfig
        PKGCONFIG += synctex

        system(pkg-config --atleast-version=2.0.0 synctex):DEFINES += HAS_SYNCTEX_2
    } else {
        HEADERS += synctex/synctex_parser.h synctex/synctex_parser_utils.h synctex/synctex_parser_local.h
        SOURCES += synctex/synctex_parser.c synctex/synctex_parser_utils.c

        INCLUDEPATH += synctex
        LIBS += -lz
    }
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


static_resources {
    RESOURCES += help.qrc translations.qrc
}

DEFINES += DATA_INSTALL_PATH=\\\"$${DATA_INSTALL_PATH}\\\"


DESKTOP_FILE = miscellaneous/qpdfview.desktop
APPDATA_FILE = miscellaneous/qpdfview.appdata.xml

!without_pdf|with_fitz:MIME_TYPES += application/pdf application/x-pdf text/pdf text/x-pdf image/pdf image/x-pdf
!without_ps:MIME_TYPES += application/postscript
!without_djvu:MIME_TYPES += image/vnd.djvu image/x-djvu

system("sed -e \"s:DATA_INSTALL_PATH:$${DATA_INSTALL_PATH}:\" -e \"s:MIME_TYPES:$$join(MIME_TYPES,";","",";"):\" $${DESKTOP_FILE}.in > $${DESKTOP_FILE}")


target.path = $${TARGET_INSTALL_PATH}

data.files = translations/*.qm help/help*.html
data.path = $${DATA_INSTALL_PATH}

manual.files = miscellaneous/qpdfview.1
manual.path = $${MANUAL_INSTALL_PATH}

icon.files = icons/qpdfview.svg
icon.path = $${ICON_INSTALL_PATH}

launcher.files = $${DESKTOP_FILE}
launcher.path = $${LAUNCHER_INSTALL_PATH}

appdata.files = $${APPDATA_FILE}
appdata.path = $${APPDATA_INSTALL_PATH}

INSTALLS += target data manual icon launcher appdata

INCLUDEPATH += icons
win32:RC_FILE = icons/qpdfview_win32.rc
os2:RC_FILE = icons/qpdfview_os2.rc
