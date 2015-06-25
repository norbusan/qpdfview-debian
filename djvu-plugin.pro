include(qpdfview.pri)

TARGET = qpdfview_djvu
TEMPLATE = lib
CONFIG += plugin
static_djvu_plugin:CONFIG += static

TARGET_SHORT = qpdfdjvu
!isEmpty(PLUGIN_DESTDIR): DESTDIR = $$PLUGIN_DESTDIR

OBJECTS_DIR = objects-djvu
MOC_DIR = moc-djvu

HEADERS = sources/model.h sources/djvumodel.h
SOURCES = sources/djvumodel.cpp

QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

!without_pkgconfig {
    CONFIG += link_pkgconfig
    PKGCONFIG += ddjvuapi
} else {
    DEFINES += $$DJVU_PLUGIN_DEFINES
    INCLUDEPATH += $$DJVU_PLUGIN_INCLUDEPATH
    LIBS += $$DJVU_PLUGIN_LIBS
}

!static_djvu_plugin {
    target.path = $${PLUGIN_INSTALL_PATH}
    INSTALLS += target
}
