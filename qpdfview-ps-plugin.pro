include(qpdfview.pri)

TARGET = qpdfview_ps
TEMPLATE = lib
CONFIG += plugin
static_ps_plugin:CONFIG += static

OBJECTS_DIR = objects-ps
MOC_DIR = moc-ps

HEADERS = sources/global.h sources/model.h sources/psmodel.h
SOURCES = sources/psmodel.cpp

QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

!without_pkgconfig {
    CONFIG += link_pkgconfig
    PKGCONFIG += libspectre
}

!static_ps_plugin {
    target.path = $${PLUGIN_INSTALL_PATH}
    INSTALLS += target
}
