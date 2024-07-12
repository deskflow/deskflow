QT += network
QT -= gui
TARGET = guitests
CONFIG += qtestlib
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app
INCLUDEPATH += ../../gui/src
SOURCES += src/main.cpp \
    src/VersionCheckerTests.cpp
HEADERS += src/VersionCheckerTests.h
win32 { 
    Debug:DESTDIR = ../../../bin/Debug
    Release:DESTDIR = ../../../bin/Release
}
else:DESTDIR = ../../../bin
