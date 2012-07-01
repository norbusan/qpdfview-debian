TARGET = qpdfview
TEMPLATE = app

HEADERS += \
    pageitem.h \
    presentationview.h \
    documentview.h \
    miscellaneous.h \
    mainwindow.h

SOURCES += \
    pageitem.cpp \
    presentationview.cpp \
    documentview.cpp \
    miscellaneous.cpp \
    mainwindow.cpp \
    main.cpp

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

launcher.files = qpdfview.desktop
launcher.path = /usr/share/applications

data.files = icons/qpdfview.svg translations/*.qm
data.path = /usr/share/qpdfview

INSTALLS += target launcher data
