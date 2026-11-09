# SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.24)
project(DeskflowLayoutPersistence LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOGEN_PARALLEL 1)
find_package(Qt6 6.7 REQUIRED COMPONENTS Widgets Test Network)
get_filename_component(defaultSource "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(DESKFLOW_BASE_SOURCE_DIR "${defaultSource}" CACHE PATH "Complete source tree")
set(DESKFLOW_CHANGE_SOURCE_DIR "${DESKFLOW_BASE_SOURCE_DIR}" CACHE PATH "Optional source overlay")
option(USE_BASELINE_PERSISTENCE "Use unchanged source for red regression" OFF)
set(base "${DESKFLOW_BASE_SOURCE_DIR}/src/lib")
set(changes "${DESKFLOW_CHANGE_SOURCE_DIR}/src/lib")
set(CMAKE_PROJECT_PROPER_NAME Deskflow)
set(CMAKE_PROJECT_REV_FQDN org.deskflow.layouttests)
set(CMAKE_PROJECT_VERSION 1.27.0.10)
set(CMAKE_PROJECT_VERSION_MAJOR 1)
set(CMAKE_PROJECT_VERSION_MINOR 27)
set(CMAKE_PROJECT_VERSION_PATCH 0)
set(CMAKE_PROJECT_VERSION_TWEAK 10)
set(GIT_SHA_SHORT isolated-layout-tests)
set(CORE_BINARY deskflow-core-disabled)
set(DESKFLOW_RUNTIME_ID deskflow-layout-tests)
set(REQUIRED_MSVC_RUNTIME_MAJOR 14)
set(REQUIRED_MSVC_RUNTIME_MINOR 0)
configure_file("${base}/common/Constants.h.in" "generated/common/Constants.h" @ONLY)
configure_file("${base}/common/VersionInfo.h.in" "generated/common/VersionInfo.h" @ONLY)
set(sources
  common/DisplayTopology.cpp gui/config/MonitorLayoutDraft.cpp
  common/Settings.cpp common/Settings.h common/QSettingsProxy.cpp common/Enums.h common/LogLevel.h
  common/Action.cpp common/Hotkey.cpp common/KeySequence.cpp common/IsolatedSession.cpp common/RuntimeProfile.cpp
  gui/config/Computer.cpp gui/config/ComputerList.cpp gui/config/ServerConfig.cpp gui/config/DesktopLayout.cpp
  gui/config/MonitorTopology.cpp experience/LayoutModel.cpp gui/ComputerSetupModel.cpp
  gui/dialogs/DesktopLayoutEditor.cpp gui/dialogs/DesktopLayoutEditor.h
  gui/dialogs/DisplayTopologyDialog.cpp gui/dialogs/DisplayTopologyDialog.h
  gui/dialogs/ServerConfigDialog.cpp gui/dialogs/ServerConfigDialog.h gui/dialogs/ServerConfigDialog.ui
  gui/dialogs/ComputerSettingsDialog.cpp gui/dialogs/ComputerSettingsDialog.ui
  gui/dialogs/ActionDialog.cpp gui/dialogs/ActionDialog.ui gui/dialogs/HotkeyDialog.cpp gui/dialogs/HotkeyDialog.ui
  gui/widgets/ComputerSetupView.cpp gui/widgets/NewComputerWidget.cpp gui/widgets/RemoveComputerWidget.cpp
  gui/widgets/SettingsDialogButtonBox.cpp gui/widgets/KeySequenceWidget.cpp)
foreach(validator Alias ComputerDuplications ComputerLineName ComputerName EmptyString IpAddress IString LineEdit Spaces ValidationError)
  list(APPEND sources "gui/validators/${validator}Validator.cpp")
endforeach()
list(REMOVE_ITEM sources gui/validators/ValidationErrorValidator.cpp)
list(APPEND sources gui/validators/ValidationError.cpp)
set(actualSources)
foreach(source IN LISTS sources)
  if(NOT USE_BASELINE_PERSISTENCE AND EXISTS "${changes}/${source}")
    list(APPEND actualSources "${changes}/${source}")
  else()
    list(APPEND actualSources "${base}/${source}")
  endif()
endforeach()
add_library(LayoutPersistenceComponents STATIC ${actualSources})
if(NOT USE_BASELINE_PERSISTENCE)
  target_include_directories(LayoutPersistenceComponents BEFORE PUBLIC "${changes}" "${changes}/gui")
endif()
target_include_directories(LayoutPersistenceComponents PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/generated" "${base}" "${base}/common" "${base}/gui" "${base}/gui/dialogs")
set_target_properties(LayoutPersistenceComponents PROPERTIES AUTOUIC_SEARCH_PATHS "${base}/gui/dialogs")
# The test links settings and widgets only. This definition also forces the
# supplied XDG profile on Windows; no production profile, core or IPC is linked.
target_compile_definitions(LayoutPersistenceComponents PUBLIC DESKFLOW_LAB_BUILD)
target_link_libraries(LayoutPersistenceComponents PUBLIC Qt6::Widgets Qt6::Network)
add_executable(LayoutPersistenceProcessTests LayoutPersistenceProcessTests.cpp)
target_link_libraries(LayoutPersistenceProcessTests PRIVATE LayoutPersistenceComponents Qt6::Test)
enable_testing()
add_test(NAME LayoutPersistenceProcessTests COMMAND LayoutPersistenceProcessTests)
set_tests_properties(LayoutPersistenceProcessTests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 60)
# Re-run the real layout/dialog suite too, including its corrected Save tests.
get_filename_component(changeRoot "${DESKFLOW_CHANGE_SOURCE_DIR}/.." ABSOLUTE)
get_filename_component(baseRoot "${DESKFLOW_BASE_SOURCE_DIR}/.." ABSOLUTE)
set(integration "${baseRoot}/native-tests/DesktopLayoutIntegrationTests.cpp")
if(EXISTS "${changeRoot}/native-tests/DesktopLayoutIntegrationTests.cpp")
  set(integration "${changeRoot}/native-tests/DesktopLayoutIntegrationTests.cpp")
endif()
add_executable(LayoutDialogRegressionTests "${integration}")
target_link_libraries(LayoutDialogRegressionTests PRIVATE LayoutPersistenceComponents Qt6::Test)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/isolated-profiles")
add_test(NAME LayoutDialogRegressionTests COMMAND LayoutDialogRegressionTests)
set_tests_properties(LayoutDialogRegressionTests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;DESKFLOW_TEST_ROOT=${CMAKE_CURRENT_BINARY_DIR}/isolated-profiles" SKIP_RETURN_CODE 77 TIMEOUT 30)

add_executable(MonitorPersistenceTests MonitorPersistenceTests.cpp)
target_link_libraries(MonitorPersistenceTests PRIVATE LayoutPersistenceComponents Qt6::Test)
add_test(NAME MonitorPersistenceTests COMMAND MonitorPersistenceTests)
set_tests_properties(MonitorPersistenceTests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 60)
