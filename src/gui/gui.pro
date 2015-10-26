QT += widgets \
    network
TEMPLATE = app
TARGET = synergy
DEFINES += VERSION_STAGE=\\\"$$QMAKE_VERSION_STAGE\\\"
DEFINES += VERSION_REVISION=\\\"$$QMAKE_VERSION_REVISION\\\"
DEPENDPATH += . \
    res
INCLUDEPATH += . \
    src
FORMS += res/MainWindowBase.ui \
    res/AboutDialogBase.ui \
    res/ServerConfigDialogBase.ui \
    res/ScreenSettingsDialogBase.ui \
    res/ActionDialogBase.ui \
    res/HotkeyDialogBase.ui \
    res/SettingsDialogBase.ui \
    res/SetupWizardBase.ui \
    res/AddClientDialogBase.ui \
    res/PluginWizardPageBase.ui
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
    src/QSynergyApplication.cpp \
    src/VersionChecker.cpp \
    src/SetupWizard.cpp \
    src/IpcClient.cpp \
    src/IpcReader.cpp \
    src/Ipc.cpp \
    src/SynergyLocale.cpp \
    src/QUtility.cpp \
    src/ZeroconfServer.cpp \
    src/ZeroconfThread.cpp \
    src/ZeroconfRegister.cpp \
    src/ZeroconfBrowser.cpp \
    src/ZeroconfService.cpp \
    src/DataDownloader.cpp \
    src/AddClientDialog.cpp \
    src/CommandProcess.cpp \
    src/PluginWizardPage.cpp \
    src/PluginManager.cpp \
    src/CoreInterface.cpp \
    src/Fingerprint.cpp \
    src/SslCertificate.cpp \
    src/Plugin.cpp \
    src/WebClient.cpp \
    ../lib/common/PluginVersion.cpp \
    src/SubscriptionManager.cpp
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
    src/QSynergyApplication.h \
    src/VersionChecker.h \
    src/SetupWizard.h \
    src/IpcClient.h \
    src/IpcReader.h \
    src/Ipc.h \
    src/SynergyLocale.h \
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
    src/EditionType.h \
    src/PluginWizardPage.h \
    src/ProcessorArch.h \
    src/PluginManager.h \
    src/CoreInterface.h \
    src/Fingerprint.h \
    src/SslCertificate.h \
    src/Plugin.h \
    src/WebClient.h \
    ../lib/common/PluginVersion.h \
    src/SubscriptionManager.h \
    src/SubscriptionState.h
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
win32 { 
    Debug:DESTDIR = ../../bin/Debug
    Release:DESTDIR = ../../bin/Release
    LIBS += -L"../../ext/bonjour/x64" \
        -ldnssd
    INCLUDEPATH += "$(BONJOUR_SDK_HOME)/Include"
}
else:DESTDIR = ../../bin
