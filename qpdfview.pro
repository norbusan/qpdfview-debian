TEMPLATE = subdirs
SUBDIRS += qpdfview-application.pro

!without_pdf {
    SUBDIRS += qpdfview-pdf.pro

    static_pdf_plugin {
        qpdfview-application.pro.depends = qpdfview-pdf.pro
    }
}

!without_ps {
    SUBDIRS += qpdfview-ps.pro

    static_ps_plugin {
        qpdfview-application.pro.depends = qpdfview-ps.pro
    }
}
