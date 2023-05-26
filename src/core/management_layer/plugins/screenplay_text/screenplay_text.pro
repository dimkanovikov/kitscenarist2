TEMPLATE = lib

CONFIG += plugin c++1z static
CONFIG += force_debug_info
CONFIG += separate_debug_info
QT += widgets

TARGET = screenplaytextplugin

DEFINES += MANAGER_PLUGIN
DEFINES += QT_DEPRECATED_WARNINGS

mac {
    DESTDIR = ../../../../_build/starcapp.app/Contents/PlugIns
    CORELIBDIR = ../../../../_build/starcapp.app/Contents/Frameworks
} else {
    DESTDIR = ../../../../_build/plugins
    CORELIBDIR = ../../../../_build
}

INCLUDEPATH += $$PWD/../../../..

#
# Подключаем библиотеку corelib
#

LIBS += -L$$CORELIBDIR/ -lcorelib
INCLUDEPATH += $$PWD/../../../../corelib
DEPENDPATH += $$PWD/../../../../corelib

#

HEADERS += \
    screenplay_text_manager.h \
    screenplay_text_view.h \
    text/handlers/abstract_key_handler.h \
    text/handlers/act_footer_handler.h \
    text/handlers/act_heading_handler.h \
    text/handlers/action_handler.h \
    text/handlers/beat_heading_handler.h \
    text/handlers/character_handler.h \
    text/handlers/dialog_handler.h \
    text/handlers/inline_note_handler.h \
    text/handlers/key_press_handler_facade.h \
    text/handlers/lyrics_handler.h \
    text/handlers/parenthetical_handler.h \
    text/handlers/pre_handler.h \
    text/handlers/prepare_handler.h \
    text/handlers/scene_characters_handler.h \
    text/handlers/scene_heading_handler.h \
    text/handlers/sequence_footer_handler.h \
    text/handlers/sequence_heading_handler.h \
    text/handlers/shot_handler.h \
    text/handlers/splitter_handler.h \
    text/handlers/standard_key_handler.h \
    text/handlers/transition_handler.h \
    text/handlers/unformatted_text_handler.h \
    text/screenplay_text_edit.h \
    text/screenplay_text_edit_shortcuts_manager.h \
    text/screenplay_text_edit_toolbar.h \
    text/screenplay_text_scrollbar_manager.h \
    text/screenplay_text_search_manager.h \
    text/screenplay_text_search_toolbar.h \
    ui/dictionaries_view.h

SOURCES += \
    screenplay_text_manager.cpp \
    screenplay_text_view.cpp \
    text/handlers/abstract_key_handler.cpp \
    text/handlers/act_footer_handler.cpp \
    text/handlers/act_heading_handler.cpp \
    text/handlers/action_handler.cpp \
    text/handlers/beat_heading_handler.cpp \
    text/handlers/character_handler.cpp \
    text/handlers/dialog_handler.cpp \
    text/handlers/inline_note_handler.cpp \
    text/handlers/key_press_handler_facade.cpp \
    text/handlers/lyrics_handler.cpp \
    text/handlers/parenthetical_handler.cpp \
    text/handlers/pre_handler.cpp \
    text/handlers/prepare_handler.cpp \
    text/handlers/scene_characters_handler.cpp \
    text/handlers/scene_heading_handler.cpp \
    text/handlers/sequence_footer_handler.cpp \
    text/handlers/sequence_heading_handler.cpp \
    text/handlers/shot_handler.cpp \
    text/handlers/standard_key_handler.cpp \
    text/handlers/transition_handler.cpp \
    text/handlers/unformatted_text_handler.cpp \
    text/screenplay_text_edit.cpp \
    text/screenplay_text_edit_shortcuts_manager.cpp \
    text/screenplay_text_edit_toolbar.cpp \
    text/screenplay_text_scrollbar_manager.cpp \
    text/screenplay_text_search_manager.cpp \
    text/screenplay_text_search_toolbar.cpp \
    ui/dictionaries_view.cpp

mac {
    load(resolve_target)
    QMAKE_POST_LINK += install_name_tool -change libcorelib.1.dylib @executable_path/../Frameworks/libcorelib.dylib $$QMAKE_RESOLVED_TARGET
}
