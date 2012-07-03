TARGET = qpdfview
TEMPLATE = app

HEADERS += \
    sources/pageitem.h \
    sources/presentationview.h \
    sources/documentview.h \
    sources/miscellaneous.h \
    sources/mainwindow.h

SOURCES += \
    sources/pageitem.cpp \
    sources/presentationview.cpp \
    sources/documentview.cpp \
    sources/miscellaneous.cpp \
    sources/mainwindow.cpp \
    sources/main.cpp

RESOURCES += qpdfview.qrc

TRANSLATIONS += \
    translations/qpdfview_de.ts

QT += core xml gui

!without_svg {
    DEFINES += WITH_SVG
    QT += svg
}

!without_dbus {
    DEFINES += WITH_DBUS
    QT += dbus
}

CONFIG += link_pkgconfig
PKGCONFIG += poppler-qt4

system(pkg-config --atleast-version=0.14 poppler-qt4):DEFINES += HAS_POPPLER_14
system(pkg-config --atleast-version=0.22 poppler-qt4):DEFINES += HAS_POPPLER_22

!without_cups {
    DEFINES += WITH_CUPS
    LIBS += $$system(cups-config --libs)
}

target.path = /usr/bin

data.files = icons/qpdfview.svg translations/*.qm miscellaneous/help.html
data.path = /usr/share/qpdfview

launcher.files = miscellaneous/qpdfview.desktop
launcher.path = /usr/share/applications

INSTALLS += target data launcher
