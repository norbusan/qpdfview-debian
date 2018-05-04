include(qpdfview.pri)

TARGET = qpdfview_fitz
TEMPLATE = lib
CONFIG += plugin
static_fitz_plugin:CONFIG += static

TARGET_SHORT = qpdffitz
!isEmpty(PLUGIN_DESTDIR): DESTDIR = $$PLUGIN_DESTDIR

OBJECTS_DIR = objects-fitz
MOC_DIR = moc-fitz

HEADERS = sources/model.h sources/fitzmodel.h
SOURCES = sources/fitzmodel.cpp

QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

DEFINES += $$FITZ_PLUGIN_DEFINES
INCLUDEPATH += $$FITZ_PLUGIN_INCLUDEPATH

isEmpty(FITZ_PLUGIN_LIBS):FITZ_PLUGIN_LIBS = -lmupdf -lfreetype -ljpeg -lz -lm -lcrypto
LIBS += $$FITZ_PLUGIN_LIBS

!static_fitz_plugin {
    target.path = $${PLUGIN_INSTALL_PATH}
    INSTALLS += target
}
