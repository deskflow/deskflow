QT += network

TEMPLATE = app
TARGET = qsynergy
DEPENDPATH += . res
INCLUDEPATH += . src

FORMS += \
	res/MainWindowBase.ui \
	res/AboutDialogBase.ui \
	res/ServerConfigDialogBase.ui \
	res/ScreenSettingsDialogBase.ui \
	res/ActionDialogBase.ui \
	res/HotkeyDialogBase.ui \
	res/SettingsDialogBase.ui \
	res/LogDialogBase.ui
SOURCES += \
	src/main.cpp \
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
	src/LogDialog.cpp \
	src/SettingsDialog.cpp \
	src/AppConfig.cpp \
	src/QSynergyApplication.cpp
HEADERS += \
	src/MainWindow.h \
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
	src/LogDialog.h \
	src/SettingsDialog.h \
	src/AppConfig.h \
	src/QSynergyApplication.h
RESOURCES += res/QSynergy.qrc
RC_FILE = res/win/QSynergy.rc

macx {
	QMAKE_INFO_PLIST = res/mac/QSynergy.plist
	QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.4
	TARGET = QSynergy
	QSYNERGY_ICON.files = res/mac/QSynergy.icns
	QSYNERGY_ICON.path = Contents/Resources
	QMAKE_BUNDLE_DATA += QSYNERGY_ICON
}

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

