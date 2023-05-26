TEMPLATE = app
TARGET = starcapp

CONFIG += c++1z
CONFIG += force_debug_info
CONFIG += separate_debug_info
QT += core gui widgets
QT += concurrent widgets multimedia network sql websockets

DEFINES += QT_DEPRECATED_WARNINGS

DESTDIR = ../_build/

INCLUDEPATH += ..

LIBS += -L$$DESTDIR
LIBS += -L$$DESTDIR/plugins
LIBS += -L$$DESTDIR/plugins -lcoreplugin
LIBS += -L$$DESTDIR/plugins -lscreenplaytextplugin


mac {
    CORELIBDIR = ../_build/starcapp.app/Contents/Frameworks
} else {
    CORELIBDIR = ../_build
}
LIBSDIR = ../_build/libs

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

include(../3rd_party/qbreakpad/qBreakpad.pri)

SOURCES += \
        application.cpp \
        main.cpp \
        ../interfaces/management_layer/i_application_manager.h

HEADERS += \
    application.h

win32:RC_FILE = app.rc
macx {
    ICON = icon.icns
    QMAKE_INFO_PLIST = Info.plist
}
