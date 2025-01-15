# Deskflow -- mouse and keyboard sharing utility
# Copyright (C) 2024 Chris Rizzitello <sithlord48@gmail.com>
# Copyright (C) 2012-2024 Symless Ltd.
# Copyright (C) 2009-2012 Nick Bolton
#
# This package is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# found in the file LICENSE that should have accompanied this file.
#
# This package is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

set(target ${CMAKE_PROJECT_NAME}-client)

if(WIN32)
  # Generate rc file
  set(EXE_DESCRIPTION "${CMAKE_PROJECT_DESCRIPTION}\\n CLI client application")

  set(EXE_ICON "
#include \"${CMAKE_CURRENT_SOURCE_DIR}/resource.h\"
    IDI_DESKFLOW              ICON  DISCARDABLE  \"${CMAKE_SOURCE_DIR}/src/apps/res/deskflow.ico\"
    IDI_TASKBAR_NOT_RUNNING   ICON  DISCARDABLE  \"${CMAKE_SOURCE_DIR}/src/apps/res/tb_idle.ico\"
    IDI_TASKBAR_NOT_WORKING   ICON  DISCARDABLE  \"${CMAKE_SOURCE_DIR}/src/apps/res/tb_error.ico\"
    IDI_TASKBAR_NOT_CONNECTED ICON  DISCARDABLE  \"${CMAKE_SOURCE_DIR}/src/apps/res/deskflow.ico\"
    IDI_TASKBAR_CONNECTED     ICON  DISCARDABLE  \"${CMAKE_SOURCE_DIR}/src/apps/res/tb_run.ico\"
  ")

  set(EXE_RC_DIALOG "
    IDD_TASKBAR_STATUS DIALOG DISCARDABLE  0, 0, 145, 18
    STYLE DS_MODALFRAME | WS_POPUP
    FONT 8, \"MS Sans Serif\"
    BEGIN
      EDITTEXT  IDC_TASKBAR_STATUS_STATUS,3,3,139,12,ES_AUTOHSCROLL | ES_READONLY | NOT WS_BORDER
    END
  ")

  set(EXE_RC_MENU "
    IDR_TASKBAR MENU DISCARDABLE
    BEGIN
      POPUP \"${CMAKE_PROJECT_PROPER_NAME}\"
      BEGIN
        MENUITEM \"Show Status\",            IDC_TASKBAR_STATUS
        MENUITEM \"Show Log\",               IDC_TASKBAR_SHOW_LOG
        MENUITEM \"Copy Log To Clipboard\",  IDC_TASKBAR_LOG
          POPUP  \"Set Log Level\"
          BEGIN
            MENUITEM \"Error\",              IDC_TASKBAR_LOG_LEVEL_ERROR
            MENUITEM \"Warning\",            IDC_TASKBAR_LOG_LEVEL_WARNING
            MENUITEM \"Note\",               IDC_TASKBAR_LOG_LEVEL_NOTE
            MENUITEM \"Info\",               IDC_TASKBAR_LOG_LEVEL_INFO
            MENUITEM \"Debug\",              IDC_TASKBAR_LOG_LEVEL_DEBUG
            MENUITEM \"Debug1\",             IDC_TASKBAR_LOG_LEVEL_DEBUG1
            MENUITEM \"Debug2\",             IDC_TASKBAR_LOG_LEVEL_DEBUG2
          END
          MENUITEM SEPARATOR
          MENUITEM \"Quit\",                 IDC_TASKBAR_QUIT
      END
    END
  ")

  set(EXE_RC_STRINGTABLE "
    STRINGTABLE DISCARDABLE
    BEGIN
      IDS_FAILED              \"${CMAKE_PROJECT_PROPER_NAME} is about to quit with errors or warnings.  Please check the log then click OK.\"
      IDS_INIT_FAILED         \"${CMAKE_PROJECT_PROPER_NAME} failed to initialize: %{1}\"
      IDS_UNCAUGHT_EXCEPTION  \"Uncaught exception: %{1}\"
    END
  ")

  configure_file(${CMAKE_SOURCE_DIR}/src/apps/res/rc_template.rc.in ${target}.rc)

  set(PLATFORM_SOURCES
    ${target}.exe.manifest
    MSWindowsClientTaskBarReceiver.cpp
    MSWindowsClientTaskBarReceiver.h
    resource.h
    ${PROJECT_SOURCE_DIR}/src/apps/res/deskflow.ico
    ${PROJECT_SOURCE_DIR}/src/apps/res/tb_error.ico
    ${PROJECT_SOURCE_DIR}/src/apps/res/tb_idle.ico
    ${PROJECT_SOURCE_DIR}/src/apps/res/tb_run.ico
    ${CMAKE_CURRENT_BINARY_DIR}/${target}.rc
  )
elseif(APPLE)
  set(PLATFORM_SOURCES
    OSXClientTaskBarReceiver.cpp
    OSXClientTaskBarReceiver.h
  )
elseif(UNIX)
  set(PLATFORM_SOURCES
    XWindowsClientTaskBarReceiver.cpp
    XWindowsClientTaskBarReceiver.h
  )
endif()

add_executable(${target} ${PLATFORM_SOURCES} ${target}.cpp)

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
  ${libs})

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY $<TARGET_BUNDLE_CONTENT_DIR:${CMAKE_PROJECT_PROPER_NAME}>/MacOS)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  install(TARGETS ${target} DESTINATION bin)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  install(
    TARGETS ${target}
    RUNTIME_DEPENDENCY_SET clientDeps
    DESTINATION .
  )
  install(RUNTIME_DEPENDENCY_SET clientDeps
    PRE_EXCLUDE_REGEXES
      "api-ms-win-.*"
      "ext-ms-.*"
      "^hvsifiletrust\\.dll$"
    POST_EXCLUDE_REGEXES
      ".*system32.*"
    RUNTIME DESTINATION .
  )
endif()

