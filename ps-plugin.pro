include(qpdfview.pri)

TARGET = qpdfview_ps
TEMPLATE = lib
CONFIG += plugin
static_ps_plugin:CONFIG += static

TARGET_SHORT = qpdfps
!isEmpty(PLUGIN_DESTDIR): DESTDIR = $$PLUGIN_DESTDIR

OBJECTS_DIR = objects-ps
MOC_DIR = moc-ps

HEADERS = sources/model.h sources/psmodel.h
SOURCES = sources/psmodel.cpp

QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

!without_pkgconfig {
    CONFIG += link_pkgconfig
    PKGCONFIG += libspectre
} else {
    DEFINES += $$PS_PLUGIN_DEFINES
    INCLUDEPATH += $$PS_PLUGIN_INCLUDEPATH
    LIBS += $$PS_PLUGIN_LIBS
}

!static_ps_plugin {
    target.path = $${PLUGIN_INSTALL_PATH}
    INSTALLS += target
}
