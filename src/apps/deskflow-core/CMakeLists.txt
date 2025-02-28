# SPDX-FileCopyrightText: 2024 - 2025 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-FileCopyrightText: 2012 - 2024 Symless Ltd
# SPDX-FileCopyrightText: 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

set(target ${CMAKE_PROJECT_NAME}-core)

add_executable(${target} "${target}.cpp")
if(WIN32)
  # Generate rc file
  set(EXE_DESCRIPTION "${CMAKE_PROJECT_DESCRIPTION}\\n CLI combined server and client")
  set(EXE_ICON "IDI_DESKFLOW ICON DISCARDABLE \"${CMAKE_SOURCE_DIR}/src/apps/res/deskflow.ico\"")
  configure_file(${CMAKE_SOURCE_DIR}/src/apps/res/windows.rc.in ${target}.rc)

  target_sources(${target} PRIVATE
    ${target}.exe.manifest
    ${CMAKE_SOURCE_DIR}/src/apps/res/deskflow.ico
    ${CMAKE_CURRENT_BINARY_DIR}/${target}.rc
  )
endif()

target_link_libraries(
  ${target}
  arch
  base
  client
  io
  mt
  net
  platform
  server
  app
  ${libs})

if(APPLE)
  set_target_properties(${target} PROPERTIES
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "@loader_path/../Libraries;@loader_path/../Frameworks"
    RUNTIME_OUTPUT_DIRECTORY $<TARGET_BUNDLE_CONTENT_DIR:${CMAKE_PROJECT_PROPER_NAME}>/MacOS
  )
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
