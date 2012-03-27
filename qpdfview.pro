include(qpdfview.pri)

TARGET = qpdfview
TEMPLATE = app

QT += core xml gui

# append the poppler headers and libraries manually or use pkgconfig

# INCLUDEPATH += /usr/include/poppler/qt4
# LIBS += -L/usr/lib -lpoppler-qt4

CONFIG += link_pkgconfig
PKGCONFIG += poppler-qt4

SOURCES += \
    sources/documentmodel.cpp \
    sources/documentview.cpp \
    sources/pageobject.cpp \
    sources/miscellaneous.cpp \
    sources/mainwindow.cpp \
    sources/main.cpp \
    sources/presentationview.cpp

HEADERS += \
    sources/documentmodel.h \
    sources/documentview.h \
    sources/pageobject.h \
    sources/miscellaneous.h \
    sources/mainwindow.h \
    sources/presentationview.h

# use the Qt resource system or the installed data files
    
# RESOURCES += qpdfview.qrc

DEFINES += DATA_INSTALL_PATH=\\\"$${DATA_INSTALL_PATH}\\\"

TRANSLATIONS += \
    translations/qpdfview_be.ts \
    translations/qpdfview_de.ts \
    translations/qpdfview_fr.ts \
    translations/qpdfview_ru.ts \
    translations/qpdfview_uk.ts

target.path = $${TARGET_INSTALL_PATH}

data.path = $${DATA_INSTALL_PATH}
data.files = icons/*.svg translations/*.qm

launcher.path = $${LAUNCHER_INSTALL_PATH}
launcher.files = miscellaneous/qpdfview.desktop

manual.path = $${MANUAL_INSTALL_PATH}
manual.files = miscellaneous/qpdfview.1

INSTALLS += target data launcher manual
