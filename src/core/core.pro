#-------------------------------------------------
#
# Project created by QtCreator 2019-08-27T08:23:46
#
#-------------------------------------------------

TEMPLATE = lib

CONFIG += plugin c++1z
CONFIG += force_debug_info
CONFIG += separate_debug_info
QT += concurrent widgets multimedia network sql

TARGET = coreplugin

exists("../cloud/cloud.pri") {
    include(../cloud/cloud.pri)
}

DEFINES += CORE_PLUGIN
DEFINES += QT_DEPRECATED_WARNINGS

mac {
    DESTDIR = ../_build/starcapp.app/Contents/PlugIns
    CORELIBDIR = ../_build/starcapp.app/Contents/Frameworks
} else {
    DESTDIR = ../_build/plugins
    CORELIBDIR = ../_build
}
LIBSDIR = ../_build/libs

INCLUDEPATH += ..

#
# Подключаем библиотеку corelib
#
LIBS += -L$$CORELIBDIR/ -lcorelib
INCLUDEPATH += $$PWD/../corelib
DEPENDPATH += $$PWD/../corelib
#

#
# Подключаем библиотеку Webloader
#
LIBS += -L$$LIBSDIR/ -lwebloader
INCLUDEPATH += $$PWD/../3rd_party/webloader/src
DEPENDPATH += $$PWD/../3rd_party/webloader
#

SOURCES += \
    management_layer/application_manager.cpp \
    management_layer/content/account/account_manager.cpp \
    management_layer/content/export/export_manager.cpp \
    management_layer/content/import/import_manager.cpp \
    management_layer/content/notifications/notifications_manager.cpp \
    management_layer/content/onboarding/onboarding_manager.cpp \
    management_layer/content/project/project_manager.cpp \
    management_layer/content/project/project_models_facade.cpp \
    management_layer/content/projects/projects_manager.cpp \
    management_layer/content/projects/projects_model.cpp \
    management_layer/content/projects/projects_model_item.cpp \
    management_layer/content/projects/projects_model_project_item.cpp \
    management_layer/content/projects/projects_model_team_item.cpp \
    management_layer/content/settings/settings_manager.cpp \
    management_layer/content/settings/template_options_manager.cpp \
    management_layer/content/shortcuts/shortcuts_manager.cpp \
    management_layer/content/writing_session/writing_session_manager.cpp \
    management_layer/content/writing_session/writing_session_storage.cpp \
    management_layer/plugins_builder.cpp \
    ui/about_application_dialog.cpp \
    ui/account/account_navigator.cpp \
    ui/account/account_tool_bar.cpp \
    ui/account/account_view.cpp \
    ui/account/account_view_teams.cpp \
    ui/account/account_view_teams_sidebar.cpp \
    ui/account/collaborators_tool_bar.cpp \
    ui/account/compare_subscriptions_dialog.cpp \
    ui/account/connection_status_tool_bar.cpp \
    ui/account/login_dialog.cpp \
    ui/account/purchase_dialog.cpp \
    ui/account/purchase_dialog_option_widget.cpp \
    ui/account/purchase_gift_dialog.cpp \
    ui/account/session_widget.cpp \
    ui/account/subscription_widget.cpp \
    ui/account/team_dialog.cpp \
    ui/application_style.cpp \
    ui/application_view.cpp \
    ui/crash_report_dialog.cpp \
    ui/export/abstract_export_dialog.cpp \
    ui/export/audioplay_export_dialog.cpp \
    ui/export/character_export_dialog.cpp \
    ui/export/characters_export_dialog.cpp \
    ui/export/comic_book_export_dialog.cpp \
    ui/export/document_export_dialog.cpp \
    ui/export/documents_export_dialog.cpp \
    ui/export/location_export_dialog.cpp \
    ui/export/locations_export_dialog.cpp \
    ui/export/novel_export_dialog.cpp \
    ui/export/screenplay_export_dialog.cpp \
    ui/export/script_export_dialog.cpp \
    ui/export/simple_text_export_dialog.cpp \
    ui/export/stageplay_export_dialog.cpp \
    ui/import/file_name_delegate.cpp \
    ui/import/import_dialog.cpp \
    ui/menu_view.cpp \
    ui/notifications/credits_view.cpp \
    ui/notifications/release_view.cpp \
    ui/notifications/subscription_view.cpp \
    ui/onboarding/onboarding_navigator.cpp \
    ui/onboarding/onboarding_tool_bar.cpp \
    ui/onboarding/onboarding_view.cpp \
    ui/project/create_document_dialog.cpp \
    ui/project/create_document_dialog_option.cpp \
    ui/project/create_draft_dialog.cpp \
    ui/project/project_navigator.cpp \
    ui/project/project_tool_bar.cpp \
    ui/project/project_tree_delegate.cpp \
    ui/project/project_view.cpp \
    ui/projects/create_project_dialog.cpp \
    ui/projects/project_card.cpp \
    ui/projects/project_team_card.cpp \
    ui/projects/projects_cards_graphics_view.cpp \
    ui/projects/projects_navigator.cpp \
    ui/projects/projects_tool_bar.cpp \
    ui/projects/projects_view.cpp \
    ui/session_statistics/counter_widget.cpp \
    ui/session_statistics/overview_widget.cpp \
    ui/session_statistics/session_statistics_navigator.cpp \
    ui/session_statistics/session_statistics_tool_bar.cpp \
    ui/session_statistics/session_statistics_view.cpp \
    ui/settings/language_dialog.cpp \
    ui/settings/screenplay_template/screenplay_template_navigator.cpp \
    ui/settings/screenplay_template/screenplay_template_page_view.cpp \
    ui/settings/screenplay_template/screenplay_template_paragraphs_view.cpp \
    ui/settings/screenplay_template/screenplay_template_tool_bar.cpp \
    ui/settings/screenplay_template/screenplay_template_view_tool_bar.cpp \
    ui/settings/settings_navigator.cpp \
    ui/settings/settings_tool_bar.cpp \
    ui/settings/settings_view.cpp \
    ui/settings/theme_setup_view.cpp \
    ui/settings/widgets/page_layout.cpp \
    ui/settings/widgets/theme_preview.cpp \
    ui/writing_session/writing_sprint_panel.cpp

HEADERS += \
    core_global.h \
    management_layer/application_manager.h \
    management_layer/content/account/account_manager.h \
    management_layer/content/export/export_manager.h \
    management_layer/content/import/import_manager.h \
    management_layer/content/notifications/notifications_manager.h \
    management_layer/content/onboarding/onboarding_manager.h \
    management_layer/content/project/project_manager.h \
    management_layer/content/project/project_models_facade.h \
    management_layer/content/projects/projects_manager.h \
    management_layer/content/projects/projects_model.h \
    management_layer/content/projects/projects_model_item.h \
    management_layer/content/projects/projects_model_project_item.h \
    management_layer/content/projects/projects_model_team_item.h \
    management_layer/content/settings/settings_manager.h \
    management_layer/content/settings/template_options_manager.h \
    management_layer/content/shortcuts/shortcuts_manager.h \
    management_layer/content/writing_session/writing_session_manager.h \
    management_layer/content/writing_session/writing_session_storage.h \
    management_layer/plugins_builder.h \
    ui/about_application_dialog.h \
    ui/account/account_navigator.h \
    ui/account/account_tool_bar.h \
    ui/account/account_view.h \
    ui/account/account_view_teams.h \
    ui/account/account_view_teams_sidebar.h \
    ui/account/collaborators_tool_bar.h \
    ui/account/compare_subscriptions_dialog.h \
    ui/account/connection_status_tool_bar.h \
    ui/account/login_dialog.h \
    ui/account/purchase_dialog.h \
    ui/account/purchase_dialog_option_widget.h \
    ui/account/purchase_gift_dialog.h \
    ui/account/session_widget.h \
    ui/account/subscription_widget.h \
    ui/account/team_dialog.h \
    ui/application_style.h \
    ui/application_view.h \
    ui/crash_report_dialog.h \
    ui/export/abstract_export_dialog.h \
    ui/export/audioplay_export_dialog.h \
    ui/export/character_export_dialog.h \
    ui/export/characters_export_dialog.h \
    ui/export/comic_book_export_dialog.h \
    ui/export/document_export_dialog.h \
    ui/export/documents_export_dialog.h \
    ui/export/location_export_dialog.h \
    ui/export/locations_export_dialog.h \
    ui/export/novel_export_dialog.h \
    ui/export/screenplay_export_dialog.h \
    ui/export/script_export_dialog.h \
    ui/export/simple_text_export_dialog.h \
    ui/export/stageplay_export_dialog.h \
    ui/import/file_name_delegate.h \
    ui/import/import_dialog.h \
    ui/menu_view.h \
    ui/notifications/credits_view.h \
    ui/notifications/release_view.h \
    ui/notifications/subscription_view.h \
    ui/onboarding/onboarding_navigator.h \
    ui/onboarding/onboarding_tool_bar.h \
    ui/onboarding/onboarding_view.h \
    ui/project/create_document_dialog.h \
    ui/project/create_document_dialog_option.h \
    ui/project/create_draft_dialog.h \
    ui/project/project_navigator.h \
    ui/project/project_tool_bar.h \
    ui/project/project_tree_delegate.h \
    ui/project/project_view.h \
    ui/projects/create_project_dialog.h \
    ui/projects/project_card.h \
    ui/projects/project_team_card.h \
    ui/projects/projects_cards_graphics_view.h \
    ui/projects/projects_navigator.h \
    ui/projects/projects_tool_bar.h \
    ui/projects/projects_view.h \
    ui/session_statistics/counter_widget.h \
    ui/session_statistics/overview_widget.h \
    ui/session_statistics/session_statistics_navigator.h \
    ui/session_statistics/session_statistics_tool_bar.h \
    ui/session_statistics/session_statistics_view.h \
    ui/settings/language_dialog.h \
    ui/settings/screenplay_template/screenplay_template_navigator.h \
    ui/settings/screenplay_template/screenplay_template_page_view.h \
    ui/settings/screenplay_template/screenplay_template_paragraphs_view.h \
    ui/settings/screenplay_template/screenplay_template_tool_bar.h \
    ui/settings/screenplay_template/screenplay_template_view_tool_bar.h \
    ui/settings/settings_navigator.h \
    ui/settings/settings_tool_bar.h \
    ui/settings/settings_view.h \
    ui/settings/theme_setup_view.h \
    ui/settings/widgets/page_layout.h \
    ui/settings/widgets/theme_preview.h \
    ui/writing_session/writing_sprint_panel.h

RESOURCES += \
    resources.qrc \
    translations.qrc

mac {
    load(resolve_target)
    QMAKE_POST_LINK += install_name_tool -change libcorelib.1.dylib @executable_path/../Frameworks/libcorelib.dylib $$QMAKE_RESOLVED_TARGET
}
