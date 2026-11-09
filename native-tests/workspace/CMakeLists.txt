# SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.24)
project(DeskflowWorkspaceView LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOGEN_PARALLEL 1)
find_package(Qt6 6.7 REQUIRED COMPONENTS Widgets Test LinguistTools)
set(src "${CMAKE_CURRENT_SOURCE_DIR}/../../src/lib")
add_executable(WorkspaceViewTests WorkspaceViewTests.cpp
  "${CMAKE_CURRENT_SOURCE_DIR}/../../src/apps/res/deskflow.qrc"
  "${src}/common/DisplayTopology.cpp" "${src}/common/DesktopWorkspace.cpp"
  "${src}/gui/widgets/DesktopWorkspaceView.cpp" "${src}/gui/widgets/DesktopWorkspaceView.h"
  "${src}/gui/dialogs/DesktopLayoutEditor.cpp" "${src}/gui/dialogs/DesktopLayoutEditor.h"
  "${src}/gui/dialogs/DisplayTopologyDialog.cpp" "${src}/gui/dialogs/DisplayTopologyDialog.h"
  "${src}/gui/config/MonitorTopology.cpp" "${src}/gui/config/MonitorLayoutDraft.cpp"
  "${src}/gui/config/DesktopLayout.cpp" "${src}/experience/LayoutModel.cpp")
target_include_directories(WorkspaceViewTests PRIVATE "${src}")
target_link_libraries(WorkspaceViewTests PRIVATE Qt6::Widgets Qt6::Test)
qt_add_lrelease(TS_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../../translations/deskflow_zh_CN.ts"
  LRELEASE_TARGET WorkspaceTranslations QM_FILES_OUTPUT_VARIABLE qmFiles)
qt_add_resources(WorkspaceViewTests workspace_translations PREFIX "/translations" BASE "${CMAKE_CURRENT_BINARY_DIR}" FILES ${qmFiles})
add_dependencies(WorkspaceViewTests WorkspaceTranslations)
enable_testing()
add_test(NAME WorkspaceViewTests COMMAND WorkspaceViewTests)
set_tests_properties(WorkspaceViewTests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)
