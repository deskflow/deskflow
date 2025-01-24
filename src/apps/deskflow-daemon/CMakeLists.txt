# SPDX-FileCopyrightText: 2024 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-FileCopyrightText: 2012 - 2024 Symless Ltd
# SPDX-FileCopyrightText: 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

# Daemon is only needed on Windows for elevating processes to deal with UAC.
if(WIN32)
  set(target ${CMAKE_PROJECT_NAME}-daemon)

  # Generate rc file
  set(EXE_DESCRIPTION "This service runs in the background to help ${CMAKE_PROJECT_NAME} deal with UAC prompts")
  configure_file(${CMAKE_SOURCE_DIR}/src/apps/res/windows.rc.in ${target}.rc)

  add_executable(${target} WIN32 ${target}.cpp ${CMAKE_CURRENT_BINARY_DIR}/${target}.rc)

  target_link_libraries(
    ${target}
    arch
    base
    io
    ipc
    mt
    net
    platform
    app
    ${libs})

  install(
    TARGETS ${target}
    RUNTIME_DEPENDENCY_SET deamonDeps
    DESTINATION .
  )
  install(RUNTIME_DEPENDENCY_SET daemonDeps
    PRE_EXCLUDE_REGEXES
      "api-ms-win-.*"
      "ext-ms-.*"
      "^hvsifiletrust\\.dll$"
    POST_EXCLUDE_REGEXES
      ".*system32.*"
    RUNTIME DESTINATION .
  )
endif()
