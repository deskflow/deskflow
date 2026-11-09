# SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.24)
project(DeskflowStartupReview LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOGEN_PARALLEL 1)
find_package(Qt6 6.7 REQUIRED COMPONENTS Widgets Network Test LinguistTools)
set(src "${CMAKE_CURRENT_SOURCE_DIR}/../../src/lib")
add_executable(StartupRecoveryTests StartupRecoveryTests.cpp
  "${src}/gui/core/StartupRecovery.cpp" "${src}/gui/core/StartupRecoveryReader.cpp"
  "${src}/gui/dialogs/StartupRecoveryDialog.cpp" "${src}/common/CandidatePolicy.cpp")
target_sources(StartupRecoveryTests PRIVATE "${src}/gui/core/MacStartupExecutor.cpp" "${src}/gui/core/StartupHandoff.cpp")
target_compile_definitions(StartupRecoveryTests PRIVATE DESKFLOW_LAB_BUILD)
target_include_directories(StartupRecoveryTests PRIVATE "${src}")
target_link_libraries(StartupRecoveryTests PRIVATE Qt6::Widgets Qt6::Network Qt6::Test)
# Compile the real read-only platform branch too, without linking/running it.
add_library(StartupReadOnlyNativeCompile OBJECT "${src}/gui/core/StartupRecoveryReader.cpp")
target_include_directories(StartupReadOnlyNativeCompile PRIVATE "${src}")
target_link_libraries(StartupReadOnlyNativeCompile PRIVATE Qt6::Widgets)
qt_add_lrelease(TS_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../../translations/deskflow_zh_CN.ts"
  LRELEASE_TARGET StartupTranslations QM_FILES_OUTPUT_VARIABLE qmFiles)
add_dependencies(StartupRecoveryTests StartupTranslations)
qt_add_resources(StartupRecoveryTests startup_translations PREFIX "/translations" BASE "${CMAKE_CURRENT_BINARY_DIR}" FILES ${qmFiles})
enable_testing()
if(APPLE)
add_executable(MacStartupExecutorTests MacStartupExecutorTests.cpp
  "${src}/gui/core/StartupHandoff.cpp"
  "${src}/gui/core/StartupRecoveryReader.cpp"
  "${src}/gui/core/MacStartupExecutor.cpp" "${src}/gui/core/StartupRecovery.cpp" "${src}/common/CandidatePolicy.cpp")
target_compile_definitions(MacStartupExecutorTests PRIVATE DESKFLOW_LAB_BUILD)
target_include_directories(MacStartupExecutorTests PRIVATE "${src}")
target_link_libraries(MacStartupExecutorTests PRIVATE Qt6::Widgets Qt6::Network Qt6::Test)
add_library(StartupExecutorNativeCompile OBJECT "${src}/gui/core/MacStartupExecutor.cpp")
target_compile_definitions(StartupExecutorNativeCompile PRIVATE DESKFLOW_CANDIDATE_BUILD DESKFLOW_STARTUP_MUTATIONS)
target_include_directories(StartupExecutorNativeCompile PRIVATE "${src}")
target_link_libraries(StartupExecutorNativeCompile PRIVATE Qt6::Widgets)
endif()
add_test(NAME StartupRecoveryTests COMMAND StartupRecoveryTests)
set_tests_properties(StartupRecoveryTests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 60)
