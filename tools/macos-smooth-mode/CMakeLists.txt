# SPDX-FileCopyrightText: (C) 2026 lxf82dev <lxf82dev@gmail.com>
# SPDX-License-Identifier: MIT

find_library(SMOOTH_FOUNDATION Foundation REQUIRED)
find_library(SMOOTH_APPKIT AppKit REQUIRED)
find_program(SMOOTH_CODESIGN codesign REQUIRED)

add_executable(DeskflowAWDLGuard awdl-guard.c)
set_target_properties(DeskflowAWDLGuard PROPERTIES OUTPUT_NAME awdl-guard RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(DeskflowAWDLGuard PRIVATE proc)

add_executable(DeskflowSmoothSetup SmoothSetup.m)
set_target_properties(DeskflowSmoothSetup PROPERTIES OUTPUT_NAME smooth-setup RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_options(DeskflowSmoothSetup PRIVATE -fobjc-arc)
target_link_libraries(DeskflowSmoothSetup PRIVATE "${SMOOTH_FOUNDATION}" proc)

add_executable(DeskflowSmoothMenu SmoothMenu.m)
set_target_properties(DeskflowSmoothMenu PROPERTIES OUTPUT_NAME SmoothMenu RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_options(DeskflowSmoothMenu PRIVATE -fobjc-arc)
target_link_libraries(DeskflowSmoothMenu PRIVATE "${SMOOTH_APPKIT}")

set(SMOOTH_APP "${CMAKE_CURRENT_BINARY_DIR}/Deskflow Smooth Mode.app")
file(MAKE_DIRECTORY "${SMOOTH_APP}/Contents/Resources")
configure_file(resources/Info.plist "${SMOOTH_APP}/Contents/Info.plist" COPYONLY)
configure_file(README.md "${SMOOTH_APP}/Contents/Resources/README.txt" COPYONLY)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/resources")
configure_file(resources/Info.plist "${CMAKE_CURRENT_BINARY_DIR}/resources/Info.plist" COPYONLY)
configure_file(README.md "${CMAKE_CURRENT_BINARY_DIR}/resources/README.txt" COPYONLY)
foreach(control "Install Server.command" "Install Client.command" "Uninstall Smooth Mode.command" "Build from source.command"
    README.md awdl-guard.c smooth_policy.h SmoothSetup.m SmoothMenu.m)
  configure_file("${control}" "${CMAKE_CURRENT_BINARY_DIR}/${control}" COPYONLY)
endforeach()

add_custom_target(macos-smooth-mode ALL
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${SMOOTH_APP}/Contents/MacOS"
  COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:DeskflowSmoothMenu>" "${SMOOTH_APP}/Contents/MacOS/SmoothMenu"
  COMMAND "${SMOOTH_CODESIGN}" --force --sign - --identifier local.deskflow.mx-smooth-mode "${SMOOTH_APP}"
  DEPENDS DeskflowAWDLGuard DeskflowSmoothSetup DeskflowSmoothMenu
  VERBATIM)

if(BUILD_TESTS)
  enable_testing()
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  add_test(NAME AWDLGuardPolicy COMMAND $<TARGET_FILE:DeskflowAWDLGuard> --self-test)
  add_test(NAME SmoothSetupPolicy COMMAND $<TARGET_FILE:DeskflowSmoothSetup> --self-test)
  add_test(NAME AWDLGuardIntegration COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test_guard.py" $<TARGET_FILE:DeskflowAWDLGuard>)
endif()
