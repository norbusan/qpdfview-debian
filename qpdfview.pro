TARGET = qpdfview
TEMPLATE = app

QT       += core xml gui

INCLUDEPATH  += /usr/include/poppler/qt4
LIBS         += -L/usr/lib -lpoppler-qt4

SOURCES += sources/main.cpp sources/mainwindow.cpp \
    sources/documentview.cpp \
    sources/pageobject.cpp \
    sources/outlineview.cpp \
    sources/thumbnailsview.cpp \
    sources/settingsdialog.cpp

HEADERS  += sources/mainwindow.h \
    sources/documentview.h \
    sources/pageobject.h \
    sources/outlineview.h \
    sources/thumbnailsview.h \
    sources/settingsdialog.h

RESOURCES += qpdfview.qrc

ICON = icons/qpdfview.svg

TRANSLATIONS += translations/qpdfview_be.ts translations/qpdfview_de.ts translations/qpdfview_ru.ts translations/qpdfview_uk.ts

target.path = /usr/bin

pixmap.path = /usr/share/pixmaps
pixmap.files = icons/qpdfview.png

manpage.path = /usr/share/man/man1
manpage.files = miscellaneous/qpdfview.1

launcher.path = /usr/share/applications
launcher.files = miscellaneous/qpdfview.desktop

INSTALLS += target pixmap manpage launcher
