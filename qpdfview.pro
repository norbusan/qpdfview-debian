TARGET = qpdfview
TEMPLATE = app

QT       += core gui

INCLUDEPATH  += /usr/include/poppler/qt4
LIBS         += -L/usr/lib -lpoppler-qt4

SOURCES += main.cpp mainwindow.cpp \
    documentview.cpp \
    pageobject.cpp

HEADERS  += mainwindow.h \
    documentview.h \
    pageobject.h

RESOURCES += qpdfview.qrc
