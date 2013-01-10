include(qpdfview.pri)

TARGET = qpdfview_pdf
TEMPLATE = lib
CONFIG += plugin
static_pdf_plugin:CONFIG += static

OBJECTS_DIR = .objects-pdf

HEADERS = sources/global.h sources/model.h sources/pdfmodel.h sources/annotationdialog.h sources/formfielddialog.h
SOURCES = sources/model.cpp sources/pdfmodel.cpp sources/annotationdialog.cpp sources/formfielddialog.cpp

QT += core xml gui

!without_pkgconfig {
    CONFIG += link_pkgconfig
    PKGCONFIG += poppler-qt4

    system(pkg-config --atleast-version=0.14 poppler-qt4):DEFINES += HAS_POPPLER_14
    system(pkg-config --atleast-version=0.20.1 poppler-qt4):DEFINES += HAS_POPPLER_20
    system(pkg-config --atleast-version=0.22 poppler-qt4):DEFINES += HAS_POPPLER_22
}

target.path = $${PLUGIN_INSTALL_PATH}

INSTALLS += target
