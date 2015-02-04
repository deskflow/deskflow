QT += widgets
TARGET = plugindownloader
TEMPLATE = app
SOURCES += src/main.cpp \
    src/Authenticate.cpp \
    src/MainDialog.cpp
HEADERS += src/Arguments.h \
    src/Authenticate.h \
    src/MainDialog.h
FORMS += res/MainDialogBase.ui
RESOURCES += res/Synergy.qrc
RC_FILE = res/win/Synergy.rc
macx {
    QMAKE_INFO_PLIST = res/mac/Info.plist
    TARGET = Synergy
    QSYNERGY_ICON.files = res/mac/Synergy.icns
    QSYNERGY_ICON.path = Contents/Resources
    QMAKE_BUNDLE_DATA += QSYNERGY_ICON
    LIBS += $$MACX_LIBS
}
win32 { 
    Debug:DESTDIR = ../../../bin/Debug
    Release:DESTDIR = ../../../bin/Release
}
else:DESTDIR = ../../../bin
debug { 
    OBJECTS_DIR = tmp/debug
    MOC_DIR = tmp/debug
    RCC_DIR = tmp/debug
}
release { 
    OBJECTS_DIR = tmp/release
    MOC_DIR = tmp/release
    RCC_DIR = tmp/release
}
