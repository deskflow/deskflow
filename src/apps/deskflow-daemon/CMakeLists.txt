# SPDX-FileCopyrightText: 2024 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-FileCopyrightText: 2012 - 2024 Symless Ltd
# SPDX-FileCopyrightText: 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

# Daemon is only needed on Windows for elevating processes to deal with UAC.
if(WIN32)
  set(target ${CMAKE_PROJECT_NAME}-daemon)

  # Generate rc file
  set(EXE_DESCRIPTION "${CMAKE_PROJECT_PROPER_NAME} Daemon for handling secure desktops (UAC prompts, login screen, etc)")
  set(EXE_ICON "IDI_DESKFLOW ICON DISCARDABLE \"${CMAKE_SOURCE_DIR}/src/apps/res/deskflow.ico\"")
  configure_file(${CMAKE_SOURCE_DIR}/src/apps/res/windows.rc.in ${target}.rc)

  add_executable(${target} WIN32 ${target}.cpp ${CMAKE_CURRENT_BINARY_DIR}/${target}.rc)

  target_link_libraries(
    ${target}
    arch
    base
    io
    mt
    net
    platform
    app
    common
    ${libs}
    Qt6::Core)

  install(
    TARGETS ${target}
    RUNTIME_DEPENDENCY_SET deamonDeps
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
  install(RUNTIME_DEPENDENCY_SET daemonDeps
    PRE_EXCLUDE_REGEXES
      "api-ms-win-.*"
      "ext-ms-.*"
      "^hvsifiletrust\\.dll$"
    POST_EXCLUDE_REGEXES
      ".*system32.*"
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
endif()
