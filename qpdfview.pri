TARGET_INSTALL_PATH = /usr/bin
DATA_INSTALL_PATH = /usr/share/qpdfview
LAUNCHER_INSTALL_PATH = /usr/share/applications
MANUAL_INSTALL_PATH = /usr/share/man/man1

DEFINES += DATA_INSTALL_PATH=\\\"$${DATA_INSTALL_PATH}\\\"

system(sed \"s/DATA_INSTALL_PATH/$$replace(DATA_INSTALL_PATH, "/", "\\/")/\" miscellaneous/qpdfview.desktop.in > miscellaneous/qpdfview.desktop)
