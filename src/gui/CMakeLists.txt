cmake_minimum_required (VERSION 3.4)

find_package (Qt5 REQUIRED COMPONENTS Core Widgets Network)
set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTORCC ON)
set (CMAKE_AUTOUIC ON)
set (CMAKE_INCLUDE_CURRENT_DIR ON)

# files that are used both in tests and the app
set(GUI_COMMON_SOURCE_FILES
    src/Action.cpp
    src/Hotkey.cpp
    src/KeySequence.cpp
)

set(GUI_COMMON_HEADER_FILES
    src/Action.h
    src/Hotkey.h
    src/KeySequence.h
)

set(GUI_SOURCE_FILES
    src/AboutDialog.cpp
    src/ActionDialog.cpp
    src/AddClientDialog.cpp
    src/AppConfig.cpp
    src/BarrierLocale.cpp
    src/BaseConfig.cpp
    src/CommandProcess.cpp
    src/DataDownloader.cpp
    src/DisplayIsValid.cpp
    src/Fingerprint.cpp
    src/HotkeyDialog.cpp
    src/IpcClient.cpp
    src/Ipc.cpp
    src/IpcReader.cpp
    src/KeySequenceWidget.cpp
    src/LogWindow.cpp
    src/main.cpp
    src/MainWindow.cpp
    src/NewScreenWidget.cpp
    src/QBarrierApplication.cpp
    src/QUtility.cpp
    src/Screen.cpp
    src/ScreenSettingsDialog.cpp
    src/ScreenSetupModel.cpp
    src/ScreenSetupView.cpp
    src/ServerConfig.cpp
    src/ServerConfigDialog.cpp
    src/SettingsDialog.cpp
    src/SetupWizard.cpp
    src/SslCertificate.cpp
    src/TrashScreenWidget.cpp
    src/VersionChecker.cpp
    src/ZeroconfBrowser.cpp
    src/ZeroconfRegister.cpp
    src/ZeroconfServer.cpp
    src/ZeroconfService.cpp
    src/ZeroconfThread.cpp
)

set(GUI_HEADER_FILES
    src/AboutDialog.h
    src/ActionDialog.h
    src/AddClientDialog.h
    src/AppConfig.h
    src/BarrierLocale.h
    src/BaseConfig.h
    src/CommandProcess.h
    src/DataDownloader.h
    src/DisplayIsValid.h
    src/ElevateMode.h
    src/Fingerprint.h
    src/HotkeyDialog.h
    src/IpcClient.h
    src/Ipc.h
    src/IpcReader.h
    src/KeySequenceWidget.h
    src/LogWindow.h
    src/MainWindow.h
    src/NewScreenWidget.h
    src/ProcessorArch.h
    src/QBarrierApplication.h
    src/QUtility.h
    src/Screen.h
    src/ScreenSettingsDialog.h
    src/ScreenSetupModel.h
    src/ScreenSetupView.h
    src/ServerConfigDialog.h
    src/ServerConfig.h
    src/SettingsDialog.h
    src/SetupWizard.h
    src/ShutdownCh.h
    src/SslCertificate.h
    src/TrashScreenWidget.h
    src/VersionChecker.h
    src/ZeroconfBrowser.h
    src/ZeroconfRecord.h
    src/ZeroconfRegister.h
    src/ZeroconfServer.h
    src/ZeroconfService.h
    src/ZeroconfThread.h
)

set(GUI_UI_FILES
    src/AboutDialogBase.ui
    src/ActionDialogBase.ui
    src/AddClientDialogBase.ui
    src/HotkeyDialogBase.ui
    src/LogWindowBase.ui
    src/MainWindowBase.ui
    src/ScreenSettingsDialogBase.ui
    src/ServerConfigDialogBase.ui
    src/SettingsDialogBase.ui
    src/SetupWizardBase.ui
)

if (WIN32)
    set (GUI_RC_FILES res/win/Barrier.rc)
endif()

add_executable (barrier WIN32
    ${GUI_COMMON_SOURCE_FILES}
    ${GUI_COMMON_HEADER_FILES}
    ${GUI_SOURCE_FILES}
    ${GUI_HEADER_FILES}
    ${GUI_UI_FILES}
    ${GUI_RC_FILES}
    res/Barrier.qrc
)

include_directories (./src)

target_link_libraries (barrier Qt5::Core Qt5::Widgets Qt5::Network ${OPENSSL_LIBS})
target_compile_definitions (barrier PRIVATE -DBARRIER_VERSION_STAGE="${BARRIER_VERSION_STAGE}")
target_compile_definitions (barrier PRIVATE -DBARRIER_REVISION="${BARRIER_REVISION}")

if (WIN32)
    include_directories ($ENV{BONJOUR_SDK_HOME}/Include)
    find_library (DNSSD_LIB dnssd.lib
                  HINTS ENV BONJOUR_SDK_HOME
                  PATH_SUFFIXES "Lib/x64")
    set_target_properties (barrier PROPERTIES LINK_FLAGS "/NODEFAULTLIB:LIBCMT")
    target_link_libraries (barrier ${DNSSD_LIB})
elseif (APPLE)
    find_library(APPSERVICES_LIB ApplicationServices)
    target_link_libraries(barrier ${APPSERVICES_LIB})
else()
    target_link_libraries (barrier dns_sd)
endif()

if (HAVE_X11)
    target_link_libraries (barrier X11)
endif()

target_link_libraries (barrier common)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    install (TARGETS barrier DESTINATION ${BARRIER_BUNDLE_BINARY_DIR})
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR ${CMAKE_SYSTEM_NAME} MATCHES "BSD")
    install (TARGETS barrier DESTINATION bin)
endif()

if (BARRIER_BUILD_TESTS)
    set(GUI_TEST_SOURCE_FILES
        test/KeySequenceTests.cpp
        test/HotkeyTests.cpp
        test/main.cpp
    )

    add_executable(guiunittests
        ${GUI_TEST_SOURCE_FILES}
        ${GUI_COMMON_SOURCE_FILES}
        ${GUI_COMMON_HEADER_FILES}
    )

    add_test(guiunittests guiunittests)

    target_include_directories(guiunittests PUBLIC ../../ext)
    target_link_libraries(guiunittests gtest gmock Qt5::Core Qt5::Widgets Qt5::Network ${libs})
endif()
