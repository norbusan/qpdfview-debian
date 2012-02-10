TARGET = qpdfview
TEMPLATE = app

QT       += core gui

INCLUDEPATH  += /usr/include/poppler/qt4
LIBS         += -L/usr/lib -lpoppler-qt4

SOURCES += main.cpp mainwindow.cpp \
    printerthread.cpp \
    documentview.cpp

HEADERS  += mainwindow.h \
    printerthread.h \
    documentview.h
