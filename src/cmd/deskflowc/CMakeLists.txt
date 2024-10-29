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

set(target deskflow-client)

if(WIN32)
  set(PLATFORM_SOURCES
    deskflowc.exe.manifest
    deskflowc.ico
    deskflowc.rc
    MSWindowsClientTaskBarReceiver.cpp
    MSWindowsClientTaskBarReceiver.h
    resource.h
    tb_error.ico
    tb_idle.ico
    tb_run.ico
    tb_wait.ico
    ${PROJECT_BINARY_DIR}/src/version.rc
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

add_executable(${target} ${PLATFORM_SOURCES} deskflowc.cpp)

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
  install(TARGETS ${target} DESTINATION ${DESKFLOW_BUNDLE_BINARY_DIR})
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  install(TARGETS ${target} DESTINATION bin)
endif()

post_config()
