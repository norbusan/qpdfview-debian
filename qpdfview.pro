TARGET = qpdfview
TEMPLATE = app

QT       += core xml gui

INCLUDEPATH  += /usr/include/poppler/qt4
LIBS         += -L/usr/lib -lpoppler-qt4

SOURCES += main.cpp mainwindow.cpp \
    documentview.cpp \
    pageobject.cpp \
    outlineview.cpp \
    thumbnailsview.cpp \
    settingsdialog.cpp

HEADERS  += mainwindow.h \
    documentview.h \
    pageobject.h \
    outlineview.h \
    thumbnailsview.h \
    settingsdialog.h

RESOURCES += qpdfview.qrc

ICON = icons/qpdfview.svg

TRANSLATIONS += translations/qpdfview_be.ts translations/qpdfview_de.ts translations/qpdfview_ru.ts translations/qpdfview_uk.ts

target.path = /usr/bin

pixmap.path = /usr/share/pixmaps
pixmap.files = icons/qpdfview.png

manpage.path = /usr/man/man1
manpage.files = miscellaneous/qpdfview.1

launcher.path = /usr/share/applications
launcher.files = miscellaneous/qpdfview.desktop

INSTALLS += target pixmap manpage launcher
