# SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.24)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(DeskflowPermissionGuideIsolated LANGUAGES CXX)
  enable_testing()
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOGEN_PARALLEL 1)
find_package(Qt6 6.7 REQUIRED COMPONENTS Widgets Test)

# This target deliberately links only the dialog and Qt. The native AX provider
# lives in the real app entry point, which is never compiled or run by this test.
set(permissionSource "${CMAKE_CURRENT_SOURCE_DIR}/../../src/lib")
add_executable(CandidatePermissionDialogTests
  ../CandidatePermissionDialogTests.cpp
  "${permissionSource}/gui/dialogs/CandidatePermissionDialog.cpp"
  "${permissionSource}/gui/dialogs/CandidatePermissionDialog.h"
)
target_include_directories(CandidatePermissionDialogTests PRIVATE "${permissionSource}")
target_compile_definitions(CandidatePermissionDialogTests PRIVATE DESKFLOW_PERMISSION_GUIDE_TEST)
add_executable(UnsupportedPermissionTests UnsupportedPermissionTests.cpp
  "${permissionSource}/gui/dialogs/CandidatePermissionDialog.cpp")
target_compile_definitions(UnsupportedPermissionTests PRIVATE DESKFLOW_PERMISSION_GUIDE_UNSUPPORTED_TEST)
target_include_directories(UnsupportedPermissionTests PRIVATE "${permissionSource}")
target_link_libraries(UnsupportedPermissionTests PRIVATE Qt6::Widgets Qt6::Test)
add_test(NAME UnsupportedPermissionTests COMMAND UnsupportedPermissionTests)
set_tests_properties(UnsupportedPermissionTests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 20)
target_link_libraries(CandidatePermissionDialogTests PRIVATE Qt6::Widgets Qt6::Test)
add_test(NAME CandidatePermissionDialogTests COMMAND CandidatePermissionDialogTests)
set_tests_properties(CandidatePermissionDialogTests PROPERTIES
  ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_LOGGING_RULES=*.debug=false"
  TIMEOUT 20
)
