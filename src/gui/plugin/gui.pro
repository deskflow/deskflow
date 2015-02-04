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
