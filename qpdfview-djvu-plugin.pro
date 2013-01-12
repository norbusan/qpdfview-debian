include(qpdfview.pri)

TARGET = qpdfview_djvu
TEMPLATE = lib
CONFIG += plugin
static_djvu_plugin:CONFIG += static

OBJECTS_DIR = objects-djvu
MOC_DIR = moc-dvju

HEADERS = sources/global.h sources/model.h sources/djvumodel.h
SOURCES = sources/model.cpp sources/djvumodel.cpp

QT += core gui

!without_pkgconfig {
    CONFIG += link_pkgconfig
    PKGCONFIG += ddjvuapi
}

!static_dvju_plugin {
    target.path = $${PLUGIN_INSTALL_PATH}
    INSTALLS += target
}
