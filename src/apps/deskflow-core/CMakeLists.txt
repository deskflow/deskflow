# SPDX-FileCopyrightText: 2024 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-FileCopyrightText: 2012 - 2024 Symless Ltd
# SPDX-FileCopyrightText: 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

set(target ${CMAKE_PROJECT_NAME}-core)

if(WIN32)
  # Generate rc file
  set(EXE_DESCRIPTION "${CMAKE_PROJECT_DESCRIPTION}\\n CLI combined server and client")

  set(EXE_ICON "
    IDI_DESKFLOW              ICON  DISCARDABLE  \"${CMAKE_SOURCE_DIR}/src/apps/res/deskflow.ico\"
  ")

  configure_file(${CMAKE_SOURCE_DIR}/src/apps/res/windows.rc.in ${target}.rc)

  set(platform_sources
    ${target}.exe.manifest
    ${PROJECT_SOURCE_DIR}/src/apps/res/deskflow.ico
    ${CMAKE_CURRENT_BINARY_DIR}/${target}.rc
  )
endif()

# Runs the program in the "invisible" Win32 subsystem instead of attaching a console.
# If you want to run from CLI, then turn this off to see the console output.
option(CORE_WINDOWS_SUBSYSTEM "Use Windows subsystem" ON)
if (CORE_WINDOWS_SUBSYSTEM)
  set(subsystem WIN32)
endif()

add_executable(${target} ${subsystem} ${platform_sources} ${target}.cpp)

find_package(Qt6 ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Core)
target_link_libraries(
  ${target}
  arch
  base
  client
  io
  mt
  net
  ipc
  platform
  server
  app
  ${libs}
  Qt6::Core)

if(APPLE)
  set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY $<TARGET_BUNDLE_CONTENT_DIR:${CMAKE_PROJECT_PROPER_NAME}>/MacOS)
elseif(UNIX)
  install(TARGETS ${target} DESTINATION bin)
elseif(WIN32)
  install(
    TARGETS ${target}
    RUNTIME_DEPENDENCY_SET coreDeps
    DESTINATION .
  )
  install(RUNTIME_DEPENDENCY_SET coreDeps
    PRE_EXCLUDE_REGEXES
      "api-ms-win-.*"
      "ext-ms-.*"
      "^hvsifiletrust\\.dll$"
    POST_EXCLUDE_REGEXES
      ".*system32.*"
    RUNTIME DESTINATION .
  )
endif()
