TARGET = qpdfview
TEMPLATE = app

QT       += core xml gui

INCLUDEPATH  += /usr/include/poppler/qt4
LIBS         += -L/usr/lib -lpoppler-qt4

SOURCES += \
    sources/documentmodel.cpp \
    sources/documentview.cpp \
    sources/pageobject.cpp \
    sources/miscellaneous.cpp \
    sources/mainwindow.cpp \
    sources/main.cpp \
    sources/presentationview.cpp

HEADERS  += \
    sources/documentmodel.h \
    sources/documentview.h \
    sources/pageobject.h \
    sources/miscellaneous.h \
    sources/mainwindow.h \
    sources/presentationview.h

RESOURCES += qpdfview.qrc

ICON = icons/qpdfview.svg

TRANSLATIONS += translations/qpdfview_be.ts translations/qpdfview_de.ts translations/qpdfview_fr.ts translations/qpdfview_ru.ts translations/qpdfview_uk.ts

target.path = /usr/bin

pixmap.path = /usr/share/pixmaps
pixmap.files = icons/qpdfview.png

launcher.path = /usr/share/applications
launcher.files = miscellaneous/qpdfview.desktop

manual.path = /usr/share/man/man1
manual.files = miscellaneous/qpdfview.1

INSTALLS += target pixmap launcher manual
