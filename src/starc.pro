TEMPLATE = subdirs ordered

SUBDIRS += \
    3rd_party \
    app \
    corelib \
    core/management_layer/plugins \
    core \
    starcapi \
    testapp

CONFIG += ordered

HEADERS += \
    interfaces/management_layer/i_application_manager.h \
    interfaces/management_layer/i_document_manager.h \
    interfaces/ui/i_document_view.h

TRANSLATIONS += \
    core/translations/_en_source.ts \
    core/translations/translation_ar.ts \
    core/translations/translation_az.ts \
    core/translations/translation_be.ts \
    core/translations/translation_ca.ts \
    core/translations/translation_da_DK.ts \
    core/translations/translation_de.ts \
    core/translations/translation_en.ts \
    core/translations/translation_eo.ts \
    core/translations/translation_es.ts \
    core/translations/translation_fa.ts \
    core/translations/translation_fr.ts \
    core/translations/translation_gl_ES.ts \
    core/translations/translation_he.ts \
    core/translations/translation_hi.ts \
    core/translations/translation_hr.ts \
    core/translations/translation_hu.ts \
    core/translations/translation_id.ts \
    core/translations/translation_it.ts \
    core/translations/translation_kn.ts \
    core/translations/translation_ko.ts \
    core/translations/translation_nl.ts \
    core/translations/translation_pl.ts \
    core/translations/translation_pt_BR.ts \
    core/translations/translation_pt_PT.ts \
    core/translations/translation_ro_RO.ts \
    core/translations/translation_ru.ts \
    core/translations/translation_sl.ts \
    core/translations/translation_ta.ts \
    core/translations/translation_tl_PH.ts \
    core/translations/translation_tr.ts \
    core/translations/translation_uk.ts \
    core/translations/translation_zh_CN.ts

#
# Настраиваем вывод в консоль в Windows в нормальной кодировке
#
win32-msvc* {
    QMAKE_EXTRA_TARGETS += before_build makefilehook

    makefilehook.target = $(MAKEFILE)
    makefilehook.depends = .beforebuild

    PRE_TARGETDEPS += .beforebuild

    before_build.target = .beforebuild
    before_build.depends = FORCE
    before_build.commands = chcp 1251
}
