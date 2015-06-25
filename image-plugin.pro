include(qpdfview.pri)

TARGET = qpdfview_image
TEMPLATE = lib
CONFIG += plugin
static_image_plugin:CONFIG += static

TARGET_SHORT = qpdfimg
!isEmpty(PLUGIN_DESTDIR): DESTDIR = $$PLUGIN_DESTDIR

OBJECTS_DIR = objects-image
MOC_DIR = moc-image

HEADERS = sources/model.h sources/imagemodel.h
SOURCES = sources/imagemodel.cpp

QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

!static_image_plugin {
    target.path = $${PLUGIN_INSTALL_PATH}
    INSTALLS += target
}
