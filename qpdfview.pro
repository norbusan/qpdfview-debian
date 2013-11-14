include(qpdfview.pri)

TEMPLATE = subdirs
CONFIG += ordered

!without_pdf {
    SUBDIRS += qpdfview-pdf-plugin.pro
    qpdfview-application.pro.depends = qpdfview-pdf-plugin.pro
}

!without_ps {
    SUBDIRS += qpdfview-ps-plugin.pro
    qpdfview-application.pro.depends = qpdfview-ps-plugin.pro
}

!without_djvu {
    SUBDIRS += qpdfview-djvu-plugin.pro
    qpdfview-application.pro.depends = qpdfview-djvu-plugin.pro
}

SUBDIRS += qpdfview-application.pro

TRANSLATIONS += \
    translations/qpdfview_ast.ts \
    translations/qpdfview_az.ts \
    translations/qpdfview_bg.ts \
    translations/qpdfview_bs.ts \
    translations/qpdfview_ca.ts \
    translations/qpdfview_cs.ts \
    translations/qpdfview_da.ts \
    translations/qpdfview_de.ts \
    translations/qpdfview_el.ts \
    translations/qpdfview_en_GB.ts \
    translations/qpdfview_eo.ts \
    translations/qpdfview_es.ts \
    translations/qpdfview_eu.ts \
    translations/qpdfview_fi.ts \
    translations/qpdfview_fr.ts \
    translations/qpdfview_gl.ts \
    translations/qpdfview_he.ts \
    translations/qpdfview_hr.ts \
    translations/qpdfview_id.ts \
    translations/qpdfview_it.ts \
    translations/qpdfview_kk.ts \
    translations/qpdfview_ky.ts \
    translations/qpdfview_lt.ts \
    translations/qpdfview_ms.ts \
    translations/qpdfview_my.ts \
    translations/qpdfview_pl.ts \
    translations/qpdfview_pt.ts \
    translations/qpdfview_pt_BR.ts \
    translations/qpdfview_ro.ts \
    translations/qpdfview_ru.ts \
    translations/qpdfview_sk.ts \
    translations/qpdfview_tr.ts \
    translations/qpdfview_ug.ts \
    translations/qpdfview_uk.ts \
    translations/qpdfview_zh_CN.ts
