# SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.24)
project(DeskflowStatusBarIsolated LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOGEN_PARALLEL 1)

find_package(Qt6 6.7 REQUIRED COMPONENTS Widgets Test Network)

set(deskflowSource "${CMAKE_CURRENT_SOURCE_DIR}/..")
# Generate the same public headers as the upstream common target. There is no
# Settings implementation or runnable core in these isolated test executables.
set(CMAKE_PROJECT_PROPER_NAME "Deskflow")
set(CMAKE_PROJECT_REV_FQDN "org.deskflow.lab.tests")
set(CMAKE_PROJECT_VERSION "1.27.0.0")
set(CMAKE_PROJECT_VERSION_MAJOR 1)
set(CMAKE_PROJECT_VERSION_MINOR 27)
set(CMAKE_PROJECT_VERSION_PATCH 0)
set(CMAKE_PROJECT_VERSION_TWEAK 0)
set(GIT_SHA_SHORT "081f6478-lab")
set(CORE_BINARY "deskflow-lab-core-disabled")
set(DESKFLOW_RUNTIME_ID "deskflow-lab-tests")
set(REQUIRED_MSVC_RUNTIME_MAJOR 14)
set(REQUIRED_MSVC_RUNTIME_MINOR 0)
configure_file("${deskflowSource}/src/lib/common/Constants.h.in" "generated/common/Constants.h" @ONLY)
configure_file("${deskflowSource}/src/lib/common/VersionInfo.h.in" "generated/common/VersionInfo.h" @ONLY)
set(DESKFLOW_STATUSBAR_SOURCE_DIR "${deskflowSource}/src/lib/gui/widgets" CACHE PATH
  "Production widget directory, or baseline copy for a red/green regression check"
)
set(statusBarSource "${DESKFLOW_STATUSBAR_SOURCE_DIR}")
set(statusBarTests "${deskflowSource}/src/unittests/gui/widgets")

# Compile the production widget and its QTests without linking the Deskflow app,
# settings implementation, core, IPC clients, networking or platform input code.
add_executable(StatusBarIsolatedTests
  "${statusBarSource}/StatusBar.cpp"
  "${statusBarSource}/StatusBar.h"
  "${statusBarTests}/StatusBarTests.cpp"
  "${statusBarTests}/StatusBarTests.h"
  "${deskflowSource}/src/lib/common/Enums.h"
)
target_include_directories(StatusBarIsolatedTests BEFORE PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/stubs"
  "${CMAKE_CURRENT_BINARY_DIR}/generated"
  "${deskflowSource}/src/lib"
)
target_link_libraries(StatusBarIsolatedTests PRIVATE Qt6::Widgets Qt6::Test)

enable_testing()
add_subdirectory(permission-guide)
add_subdirectory(apple-ui)
add_test(NAME StatusBarIsolatedTests COMMAND StatusBarIsolatedTests)
set_tests_properties(StatusBarIsolatedTests PROPERTIES
  ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_LOGGING_RULES=*.debug=false"
  TIMEOUT 20
)

add_executable(RecentServersIsolatedTests
  "${deskflowSource}/src/lib/common/Enums.h"
  "${deskflowSource}/src/lib/gui/RecentServerHistory.cpp"
  "${deskflowSource}/src/lib/gui/RecentServerHistory.h"
  "${deskflowSource}/src/lib/gui/widgets/RecentServersButton.cpp"
  "${deskflowSource}/src/lib/gui/widgets/RecentServersButton.h"
  "${statusBarTests}/RecentServersButtonTests.cpp"
  "${statusBarTests}/RecentServersButtonTests.h"
)
target_include_directories(RecentServersIsolatedTests PRIVATE "${deskflowSource}/src/lib")
target_link_libraries(RecentServersIsolatedTests PRIVATE Qt6::Widgets Qt6::Test)
add_test(NAME RecentServersIsolatedTests COMMAND RecentServersIsolatedTests)
set_tests_properties(RecentServersIsolatedTests PROPERTIES
  ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_LOGGING_RULES=*.debug=false"
  TIMEOUT 20
)

# Compile the actual integration units with real headers/uic/moc, but do not link
# or run MainWindow: its upstream constructor owns production IPC and settings.
add_library(DeskflowGuiIntegrationCompile OBJECT EXCLUDE_FROM_ALL
  "${deskflowSource}/src/lib/gui/MainWindow.cpp"
  "${deskflowSource}/src/lib/gui/MainWindow.h"
  "${deskflowSource}/src/lib/gui/MainWindow.ui"
  "${deskflowSource}/src/lib/gui/core/CoreProcess.cpp"
  "${deskflowSource}/src/lib/gui/core/CoreProcess.h"
  "${deskflowSource}/src/lib/gui/RecentServerHistory.cpp"
  "${deskflowSource}/src/lib/gui/RecentServerHistory.h"
  "${deskflowSource}/src/lib/gui/widgets/RecentServersButton.cpp"
  "${deskflowSource}/src/lib/gui/widgets/RecentServersButton.h"
  "${deskflowSource}/src/lib/gui/widgets/StatusBar.cpp"
  "${deskflowSource}/src/lib/gui/widgets/StatusBar.h"
  "${deskflowSource}/src/lib/gui/widgets/ConnectionDiagnostics.cpp"
  "${deskflowSource}/src/lib/gui/widgets/ConnectionDiagnostics.h"
  "${deskflowSource}/src/lib/gui/config/ServerConfig.cpp"
  "${deskflowSource}/src/lib/gui/config/ServerConfig.h"
  "${deskflowSource}/src/lib/gui/config/DesktopLayout.cpp"
  "${deskflowSource}/src/lib/gui/config/DesktopLayout.h"
  "${deskflowSource}/src/lib/gui/dialogs/ServerConfigDialog.cpp"
  "${deskflowSource}/src/lib/gui/dialogs/ServerConfigDialog.h"
  "${deskflowSource}/src/lib/gui/dialogs/ServerConfigDialog.ui"
  "${deskflowSource}/src/lib/gui/dialogs/DesktopLayoutEditor.cpp"
  "${deskflowSource}/src/lib/gui/dialogs/DesktopLayoutEditor.h"
  "${deskflowSource}/src/lib/experience/LayoutModel.cpp"
)
set_target_properties(DeskflowGuiIntegrationCompile PROPERTIES AUTOMOC ON AUTOUIC ON)
target_include_directories(DeskflowGuiIntegrationCompile PRIVATE
  "${CMAKE_CURRENT_BINARY_DIR}/generated" "${deskflowSource}/src/lib" "${deskflowSource}/src/lib/gui"
)
target_compile_definitions(DeskflowGuiIntegrationCompile PRIVATE QT_NO_KEYWORDS)
target_link_libraries(DeskflowGuiIntegrationCompile PRIVATE Qt6::Widgets Qt6::Network)

add_executable(DeskflowWidgetPreview EXCLUDE_FROM_ALL
  WidgetPreview.cpp
  "${deskflowSource}/src/lib/gui/MainWindow.ui"
  "${deskflowSource}/src/lib/gui/RecentServerHistory.cpp"
  "${deskflowSource}/src/lib/gui/RecentServerHistory.h"
  "${deskflowSource}/src/lib/gui/widgets/RecentServersButton.cpp"
  "${deskflowSource}/src/lib/gui/widgets/RecentServersButton.h"
  "${deskflowSource}/src/lib/gui/widgets/StatusBar.cpp"
  "${deskflowSource}/src/lib/gui/widgets/StatusBar.h"
  "${deskflowSource}/src/lib/common/Enums.h"
)
set_target_properties(DeskflowWidgetPreview PROPERTIES AUTOMOC ON AUTOUIC ON
  AUTOUIC_SEARCH_PATHS "${deskflowSource}/src/lib/gui")
target_include_directories(DeskflowWidgetPreview BEFORE PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/stubs" "${CMAKE_CURRENT_BINARY_DIR}/generated" "${deskflowSource}/src/lib"
)
target_link_libraries(DeskflowWidgetPreview PRIVATE Qt6::Widgets)

# Real persistence, isolated before Settings::instance() is ever touched.
add_library(DeskflowIsolatedEnums STATIC "${deskflowSource}/src/lib/common/Enums.h")
target_link_libraries(DeskflowIsolatedEnums PUBLIC Qt6::Core)
add_library(DeskflowLayoutComponents STATIC
  "${deskflowSource}/src/lib/common/Settings.cpp"
  "${deskflowSource}/src/lib/common/Settings.h"
  "${deskflowSource}/src/lib/common/LogLevel.h"
  "${deskflowSource}/src/lib/common/QSettingsProxy.cpp"
  "${deskflowSource}/src/lib/common/Action.cpp"
  "${deskflowSource}/src/lib/common/Hotkey.cpp"
  "${deskflowSource}/src/lib/common/KeySequence.cpp"
  "${deskflowSource}/src/lib/common/DisplayTopology.cpp"
  "${deskflowSource}/src/lib/common/DesktopWorkspace.cpp"
  "${deskflowSource}/src/lib/gui/config/Computer.cpp"
  "${deskflowSource}/src/lib/gui/config/ComputerList.cpp"
  "${deskflowSource}/src/lib/gui/config/ServerConfig.cpp"
  "${deskflowSource}/src/lib/gui/config/DesktopLayout.cpp"
  "${deskflowSource}/src/lib/gui/config/MonitorLayoutDraft.cpp"
  "${deskflowSource}/src/lib/experience/LayoutModel.cpp"
  "${deskflowSource}/src/lib/gui/dialogs/DesktopLayoutEditor.cpp"
  "${deskflowSource}/src/lib/gui/dialogs/DesktopLayoutEditor.h"
)
target_include_directories(DeskflowLayoutComponents PUBLIC
  "${CMAKE_CURRENT_BINARY_DIR}/generated" "${deskflowSource}/src/lib" "${deskflowSource}/src/lib/gui")
target_link_libraries(DeskflowLayoutComponents PUBLIC Qt6::Widgets DeskflowIsolatedEnums)
add_library(DeskflowServerDialogComponents STATIC
  "${deskflowSource}/src/lib/gui/ComputerSetupModel.cpp"
  "${deskflowSource}/src/lib/gui/dialogs/ServerConfigDialog.cpp"
  "${deskflowSource}/src/lib/gui/dialogs/ServerConfigDialog.ui"
  "${deskflowSource}/src/lib/gui/dialogs/ComputerSettingsDialog.cpp"
  "${deskflowSource}/src/lib/gui/dialogs/ComputerSettingsDialog.ui"
  "${deskflowSource}/src/lib/gui/dialogs/ActionDialog.cpp"
  "${deskflowSource}/src/lib/gui/dialogs/ActionDialog.ui"
  "${deskflowSource}/src/lib/gui/dialogs/HotkeyDialog.cpp"
  "${deskflowSource}/src/lib/gui/dialogs/HotkeyDialog.ui"
  "${deskflowSource}/src/lib/gui/widgets/ComputerSetupView.cpp"
  "${deskflowSource}/src/lib/gui/widgets/NewComputerWidget.cpp"
  "${deskflowSource}/src/lib/gui/widgets/RemoveComputerWidget.cpp"
  "${deskflowSource}/src/lib/gui/widgets/SettingsDialogButtonBox.cpp"
  "${deskflowSource}/src/lib/gui/widgets/KeySequenceWidget.cpp"
)
foreach(validator Alias ComputerDuplications ComputerLineName ComputerName EmptyString IpAddress IString LineEdit Spaces)
  target_sources(DeskflowServerDialogComponents PRIVATE "${deskflowSource}/src/lib/gui/validators/${validator}Validator.cpp")
endforeach()
target_sources(DeskflowServerDialogComponents PRIVATE "${deskflowSource}/src/lib/gui/validators/ValidationError.cpp")
set_target_properties(DeskflowServerDialogComponents PROPERTIES AUTOUIC ON)
target_link_libraries(DeskflowServerDialogComponents PUBLIC DeskflowLayoutComponents)
add_executable(DesktopLayoutIntegrationTests DesktopLayoutIntegrationTests.cpp)
target_link_libraries(DesktopLayoutIntegrationTests PRIVATE DeskflowServerDialogComponents Qt6::Test)
add_test(NAME DesktopLayoutIntegrationTests COMMAND DesktopLayoutIntegrationTests)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/isolated-profiles")
set_tests_properties(DesktopLayoutIntegrationTests PROPERTIES
  ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_LOGGING_RULES=*.debug=false;DESKFLOW_TEST_ROOT=${CMAKE_CURRENT_BINARY_DIR}/isolated-profiles"
  TIMEOUT 20)

add_library(DeskflowDiagnosticsComponent STATIC
  "${deskflowSource}/src/lib/gui/widgets/ConnectionDiagnostics.cpp"
  "${deskflowSource}/src/lib/gui/widgets/ConnectionDiagnostics.h")
target_include_directories(DeskflowDiagnosticsComponent PUBLIC "${deskflowSource}/src/lib")
target_link_libraries(DeskflowDiagnosticsComponent PUBLIC Qt6::Widgets DeskflowIsolatedEnums)
add_executable(ConnectionDiagnosticsTests ConnectionDiagnosticsTests.cpp)
target_link_libraries(ConnectionDiagnosticsTests PRIVATE DeskflowDiagnosticsComponent Qt6::Test)
add_test(NAME ConnectionDiagnosticsTests COMMAND ConnectionDiagnosticsTests)
set_tests_properties(ConnectionDiagnosticsTests PROPERTIES
  ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_LOGGING_RULES=*.debug=false" TIMEOUT 20)

add_executable(DeskflowStage5Preview EXCLUDE_FROM_ALL Stage5Preview.cpp
  "${deskflowSource}/src/lib/gui/MainWindow.ui"
  "${deskflowSource}/src/lib/gui/RecentServerHistory.cpp"
  "${deskflowSource}/src/lib/gui/widgets/RecentServersButton.cpp"
  "${deskflowSource}/src/lib/gui/widgets/RecentServersButton.h")
set_target_properties(DeskflowStage5Preview PROPERTIES AUTOUIC ON
  AUTOUIC_SEARCH_PATHS "${deskflowSource}/src/lib/gui")
target_link_libraries(DeskflowStage5Preview PRIVATE DeskflowLayoutComponents DeskflowDiagnosticsComponent)

# Production macOS transport, native Apple declarations, in-process DNS-SD API
# replacements only. No real daemon/socket/LAN APIs are invoked by this target.
if(APPLE)
  add_executable(ScopedMacDiscoveryTests
    ScopedMacDiscoveryTests.cpp
    "${deskflowSource}/src/lib/gui/core/DiscoveryScope.cpp"
    "${deskflowSource}/src/lib/gui/core/DiscoveryTransport.h"
    "${deskflowSource}/src/lib/gui/core/DiscoveryTransportMac.cpp"
    "${deskflowSource}/src/lib/gui/core/DiscoveryCatalog.cpp"
    "${deskflowSource}/src/lib/gui/core/DiscoveryCatalog.h")
  target_include_directories(ScopedMacDiscoveryTests PRIVATE "${deskflowSource}/src/lib")
  target_link_libraries(ScopedMacDiscoveryTests PRIVATE Qt6::Core Qt6::Network Qt6::Test)
  add_test(NAME ScopedMacDiscoveryTests COMMAND ScopedMacDiscoveryTests)
  set_tests_properties(ScopedMacDiscoveryTests PROPERTIES TIMEOUT 15)
endif()

# Host-side execution of the actual Windows adapter with in-process, documented
# API-shape replacements. Explicitly NOT a Windows SDK/ABI or DLL validation.
if(APPLE)
add_executable(ScopedWindowsAdapterTests
  ScopedWindowsAdapterTests.cpp DnsApiTestShim.h
  "${deskflowSource}/src/lib/gui/core/DiscoveryScope.cpp"
  "${deskflowSource}/src/lib/gui/core/DiscoveryTransport.h"
  "${deskflowSource}/src/lib/gui/core/DiscoveryTransportWin.cpp"
  "${deskflowSource}/src/lib/gui/core/DiscoveryCatalog.cpp"
  "${deskflowSource}/src/lib/gui/core/DiscoveryCatalog.h")
target_compile_definitions(ScopedWindowsAdapterTests PRIVATE DESKFLOW_DNSSD_WINDOWS_TEST=1)
target_include_directories(ScopedWindowsAdapterTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${deskflowSource}/src/lib")
target_link_libraries(ScopedWindowsAdapterTests PRIVATE Qt6::Core Qt6::Network Qt6::Test)
add_test(NAME ScopedWindowsAdapterTests COMMAND ScopedWindowsAdapterTests)
set_tests_properties(ScopedWindowsAdapterTests PROPERTIES TIMEOUT 20)

endif()

if(APPLE)
  target_link_libraries(ScopedMacDiscoveryTests PRIVATE "-framework SystemConfiguration" "-framework CoreFoundation")
  target_link_libraries(ScopedWindowsAdapterTests PRIVATE "-framework SystemConfiguration" "-framework CoreFoundation")
endif()

if(APPLE)
  add_executable(DiscoveryMetadataProbe EXCLUDE_FROM_ALL
    DiscoveryProbe.cpp
    "${deskflowSource}/src/lib/gui/core/DiscoveryScope.cpp"
    "${deskflowSource}/src/lib/gui/core/DiscoveryTransport.h"
    "${deskflowSource}/src/lib/gui/core/DiscoveryTransportMac.cpp"
    "${deskflowSource}/src/lib/gui/core/DiscoveryCatalog.cpp"
    "${deskflowSource}/src/lib/gui/core/DiscoveryCatalog.h")
  target_include_directories(DiscoveryMetadataProbe PRIVATE "${deskflowSource}/src/lib")
  target_link_libraries(DiscoveryMetadataProbe PRIVATE Qt6::Core Qt6::Network "-framework SystemConfiguration" "-framework CoreFoundation")
endif()

# Stage 10: read-only core contract, identity state, real review/export/geometry UI.
target_sources(DeskflowLayoutComponents PRIVATE
 "${deskflowSource}/src/lib/gui/config/MonitorTopology.cpp"
 "${deskflowSource}/src/lib/gui/dialogs/DisplayTopologyDialog.cpp"
 "${deskflowSource}/src/lib/gui/dialogs/DisplayTopologyDialog.h")
target_sources(DeskflowDiagnosticsComponent PRIVATE
 "${deskflowSource}/src/lib/gui/dialogs/DiagnosticExportDialog.cpp"
 "${deskflowSource}/src/lib/gui/dialogs/DiagnosticExportDialog.h")
add_executable(Stage10OfflineTests Stage10OfflineTests.cpp
 "${deskflowSource}/src/lib/gui/IdentityBindings.cpp"
 "${deskflowSource}/src/lib/gui/dialogs/TrustedDevicesDialog.cpp"
 "${deskflowSource}/src/lib/gui/dialogs/TrustedDevicesDialog.h"
 "${deskflowSource}/src/lib/gui/dialogs/PairingReviewDialog.cpp"
 "${deskflowSource}/src/lib/gui/dialogs/PairingReviewDialog.h")
target_include_directories(Stage10OfflineTests PRIVATE "${deskflowSource}/src/lib")
target_link_libraries(Stage10OfflineTests PRIVATE DeskflowLayoutComponents DeskflowDiagnosticsComponent Qt6::Test)
add_test(NAME Stage10OfflineTests COMMAND Stage10OfflineTests)
set_tests_properties(Stage10OfflineTests PROPERTIES
 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_LOGGING_RULES=*.debug=false" TIMEOUT 30)

target_sources(DeskflowLayoutComponents PRIVATE "${deskflowSource}/src/lib/common/IsolatedSession.cpp"
   "${deskflowSource}/src/lib/common/RuntimeProfile.cpp")

if(APPLE)
 add_executable(WindowsCredentialBackendTests WindowsCredentialBackendTests.cpp WindowsSecurityApiShim.h
   "${deskflowSource}/src/lib/common/WindowsCredentialFiles.cpp"
   "${deskflowSource}/src/lib/common/CandidatePolicy.cpp"
   "${deskflowSource}/src/lib/common/IsolatedSession.cpp"
   "${deskflowSource}/src/lib/common/RuntimeProfile.cpp")
 target_compile_definitions(WindowsCredentialBackendTests PRIVATE DESKFLOW_WINDOWS_CREDENTIAL_TEST=1)
 target_include_directories(WindowsCredentialBackendTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${deskflowSource}/src/lib")
 target_link_libraries(WindowsCredentialBackendTests PRIVATE Qt6::Core Qt6::Test)
 add_test(NAME WindowsCredentialBackendTests COMMAND WindowsCredentialBackendTests)
 set_tests_properties(WindowsCredentialBackendTests PROPERTIES TIMEOUT 15)
endif()
