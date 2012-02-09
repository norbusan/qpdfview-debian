TARGET = qpdfview
TEMPLATE = app

QT       += core gui

INCLUDEPATH  += /usr/include/poppler/qt4
LIBS         += -L/usr/lib -lpoppler-qt4

SOURCES += main.cpp mainwindow.cpp \
    documentview.cpp \
    documentmodel.cpp \
    printerthread.cpp \
    pageitem.cpp

HEADERS  += mainwindow.h \
    documentview.h \
    documentmodel.h \
    printerthread.h \
    pageitem.h
