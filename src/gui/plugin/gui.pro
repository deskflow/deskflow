TARGET = plugindownloader
TEMPLATE = app
SOURCES += src/main.cpp \
    src/MainWindow.cpp \
    src/Authenticate.cpp
HEADERS += src/MainWindow.h \
    src/Arguments.h \
    src/Authenticate.h
FORMS += res/MainWindowBase.ui
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
