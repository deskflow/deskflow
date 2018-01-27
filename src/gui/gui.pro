QT += widgets \
    network
TEMPLATE = app
TARGET = barrier
DEFINES += VERSION_STAGE=\\\"$$QMAKE_VERSION_STAGE\\\"
DEFINES += VERSION_REVISION=\\\"$$QMAKE_VERSION_REVISION\\\"
DEFINES -= UNICODE
DEFINES += _MBCS
DEPENDPATH += . \
    res
INCLUDEPATH += . \
    src \
    ../lib/shared/
FORMS += src/MainWindowBase.ui \
    src/AboutDialogBase.ui \
    src/ServerConfigDialogBase.ui \
    src/ScreenSettingsDialogBase.ui \
    src/ActionDialogBase.ui \
    src/HotkeyDialogBase.ui \
    src/SettingsDialogBase.ui \
    src/SetupWizardBase.ui \
    src/AddClientDialogBase.ui \
    src/ActivationDialog.ui \
    src/CancelActivationDialog.ui \
    src/FailedLoginDialog.ui
SOURCES += src/main.cpp \
    src/MainWindow.cpp \
    src/AboutDialog.cpp \
    src/ServerConfig.cpp \
    src/ServerConfigDialog.cpp \
    src/ScreenSetupView.cpp \
    src/Screen.cpp \
    src/ScreenSetupModel.cpp \
    src/NewScreenWidget.cpp \
    src/TrashScreenWidget.cpp \
    src/ScreenSettingsDialog.cpp \
    src/BaseConfig.cpp \
    src/HotkeyDialog.cpp \
    src/ActionDialog.cpp \
    src/Hotkey.cpp \
    src/Action.cpp \
    src/KeySequence.cpp \
    src/KeySequenceWidget.cpp \
    src/SettingsDialog.cpp \
    src/AppConfig.cpp \
    src/QBarrierApplication.cpp \
    src/VersionChecker.cpp \
    src/SetupWizard.cpp \
    src/IpcClient.cpp \
    src/IpcReader.cpp \
    src/Ipc.cpp \
    src/BarrierLocale.cpp \
    src/QUtility.cpp \
    src/ZeroconfServer.cpp \
    src/ZeroconfThread.cpp \
    src/ZeroconfRegister.cpp \
    src/ZeroconfBrowser.cpp \
    src/ZeroconfService.cpp \
    src/DataDownloader.cpp \
    src/AddClientDialog.cpp \
    src/CommandProcess.cpp \
    src/CoreInterface.cpp \
    src/Fingerprint.cpp \
    src/SslCertificate.cpp \
    src/WebClient.cpp \
    src/ActivationNotifier.cpp \
    src/ActivationDialog.cpp \
    src/CancelActivationDialog.cpp \
    src/FailedLoginDialog.cpp \
    ../lib/shared/SerialKey.cpp \
    src/LicenseManager.cpp
HEADERS += src/MainWindow.h \
    src/AboutDialog.h \
    src/ServerConfig.h \
    src/ServerConfigDialog.h \
    src/ScreenSetupView.h \
    src/Screen.h \
    src/ScreenSetupModel.h \
    src/NewScreenWidget.h \
    src/TrashScreenWidget.h \
    src/ScreenSettingsDialog.h \
    src/BaseConfig.h \
    src/HotkeyDialog.h \
    src/ActionDialog.h \
    src/Hotkey.h \
    src/Action.h \
    src/KeySequence.h \
    src/KeySequenceWidget.h \
    src/SettingsDialog.h \
    src/AppConfig.h \
    src/QBarrierApplication.h \
    src/VersionChecker.h \
    src/SetupWizard.h \
    src/IpcClient.h \
    src/IpcReader.h \
    src/Ipc.h \
    src/BarrierLocale.h \
    src/QUtility.h \
    src/ZeroconfServer.h \
    src/ZeroconfThread.h \
    src/ZeroconfRegister.h \
    src/ZeroconfRecord.h \
    src/ZeroconfBrowser.h \
    src/ZeroconfService.h \
    src/DataDownloader.h \
    src/AddClientDialog.h \
    src/CommandProcess.h \
    src/ProcessorArch.h \
    src/CoreInterface.h \
    src/Fingerprint.h \
    src/SslCertificate.h \
    src/WebClient.h \
    src/ActivationNotifier.h \
    src/ElevateMode.h \
    src/ActivationDialog.h \
    src/CancelActivationDialog.h \
    src/FailedLoginDialog.h \
    ../lib/shared/EditionType.h \
    ../lib/shared/SerialKey.h \
    src/LicenseManager.h
RESOURCES += res/Barrier.qrc
RC_FILE = res/win/Barrier.rc
macx { 
    QMAKE_INFO_PLIST = res/mac/Info.plist
    TARGET = Barrier
    QBARRIER_ICON.files = res/mac/Barrier.icns
    QBARRIER_ICON.path = Contents/Resources
    QMAKE_BUNDLE_DATA += QBARRIER_ICON
    LIBS += $$MACX_LIBS
}
unix:!macx:LIBS += -ldns_sd
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
win32-msvc2015 {
    LIBS += -lAdvapi32
    QMAKE_LFLAGS += /NODEFAULTLIB:LIBCMT
}
win32-msvc* {
    contains(QMAKE_HOST.arch, x86):{
        QMAKE_LFLAGS *= /MACHINE:X86
        LIBS += -L"$$(BONJOUR_SDK_HOME)/Lib/Win32" -ldnssd
    }

    contains(QMAKE_HOST.arch, x86_64):{
        QMAKE_LFLAGS *= /MACHINE:X64
        LIBS += -L"$$(BONJOUR_SDK_HOME)/Lib/x64" -ldnssd
    }
}
win32 { 
    Debug:DESTDIR = ../../bin/Debug
    Release:DESTDIR = ../../bin/Release
    INCLUDEPATH += "$$(BONJOUR_SDK_HOME)/Include"
}
else:DESTDIR = ../../bin
