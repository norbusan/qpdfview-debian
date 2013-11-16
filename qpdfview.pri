isEmpty(APPLICATION_VERSION):APPLICATION_VERSION = 0.4.7beta1

isEmpty(TARGET_INSTALL_PATH):TARGET_INSTALL_PATH = /usr/bin
isEmpty(PLUGIN_INSTALL_PATH):PLUGIN_INSTALL_PATH = /usr/lib/qpdfview
isEmpty(DATA_INSTALL_PATH):DATA_INSTALL_PATH = /usr/share/qpdfview
isEmpty(LAUNCHER_INSTALL_PATH):LAUNCHER_INSTALL_PATH = /usr/share/applications
isEmpty(MANUAL_INSTALL_PATH):MANUAL_INSTALL_PATH = /usr/share/man/man1

win32:include(qpdfview_win32.pri)
os2:include(qpdfview_os2.pri)
