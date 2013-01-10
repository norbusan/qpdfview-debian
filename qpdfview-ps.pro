include(qpdfview.pri)

TARGET = qpdfview-ps
TEMPLATE = lib
CONFIG += plugin

HEADERS = sources/global.h sources/model.h sources/psmodel.h
SOURCES = sources/model.cpp sources/psmodel.cpp

QT += core gui

!without_pkgconfig {
    CONFIG += link_pkgconfig
    PKGCONFIG += libspectre
}

target.path = $${PLUGIN_INSTALL_PATH}

INSTALLS += target
