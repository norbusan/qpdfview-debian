include(qpdfview.pri)

TARGET = qpdfview
TEMPLATE = app

SOURCES += \
    sources/documentview.cpp \
    sources/miscellaneous.cpp \
    sources/mainwindow.cpp \
    sources/main.cpp

HEADERS += \
    sources/documentview.h \
    sources/miscellaneous.h \
    sources/mainwindow.h

TRANSLATIONS += \
    translations/qpdfview_de.ts \
    translations/qpdfview_fr.ts \
    translations/qpdfview_ro.ts \
    translations/qpdfview_ru.ts \
    translations/qpdfview_uk.ts

# libraries

QT += core xml gui

# uncomment to append the poppler headers and libraries manually
#
#INCLUDEPATH += /usr/include/poppler/qt4
#LIBS += -L/usr/lib -lpoppler-qt4
#
#DEFINES += HAS_POPPLER_14 HAS_POPPLER_18

# uncomment to use pkgconfig to find the poppler headers and libraries
#
CONFIG += link_pkgconfig
PKGCONFIG += poppler-qt4

system(pkg-config --atleast-version=0.14 poppler-qt4):DEFINES += HAS_POPPLER_14
system(pkg-config --atleast-version=0.18 poppler-qt4):DEFINES += HAS_POPPLER_18

# build-time options

!without_svg {
    QT += svg
}

!without_dbus {
    DEFINES += WITH_DBUS
    QT += dbus
}

!without_cups {
    DEFINES += WITH_CUPS
    LIBS += $$system(cups-config --libs)
}

render_in_paint:DEFINES += RENDER_IN_PAINT
render_from_disk:DEFINES += RENDER_FROM_DISK

# installation

target.path = $${TARGET_INSTALL_PATH}

launcher.path = $${LAUNCHER_INSTALL_PATH}
launcher.files = miscellaneous/qpdfview.desktop

manual.path = $${MANUAL_INSTALL_PATH}
manual.files = miscellaneous/qpdfview.1

data.path = $${DATA_INSTALL_PATH}

INSTALLS += target launcher manual data

# uncomment to use the Qt resource system for data files
#
#data.files = icons/qpdfview.svg
#RESOURCES += qpdfview.qrc

# uncomment to install data files into DATA_INSTALL_PATH
#
data.files = icons/*.svg translations/*.qm miscellaneous/help.html
DEFINES += DATA_INSTALL_PATH=\\\"$${DATA_INSTALL_PATH}\\\"
