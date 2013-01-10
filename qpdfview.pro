TEMPLATE = subdirs
SUBDIRS += qpdfview-application.pro

!without_pkgconfig {
    !without_pdf : system(pkg-config --exists poppler-qt4):SUBDIRS += qpdfview-pdf.pro
    !without_ps : system(pkg-config --exists libspectre):SUBDIRS += qpdfview-ps.pro
} else {
    !without_pdf:SUBDIRS += qpdfview-pdf.pro
    !without_ps:SUBDIRS += qpdfview-ps.pro
}
