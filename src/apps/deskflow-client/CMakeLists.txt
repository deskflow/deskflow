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
    IDI_DESKFLOW              ICON  DISCARDABLE  \"${CMAKE_SOURCE_DIR}/src/apps/res/deskflow.ico\"
  ")

  configure_file(${CMAKE_SOURCE_DIR}/src/apps/res/rc_template.rc.in ${target}.rc)

  set(PLATFORM_SOURCES
    ${target}.exe.manifest
    ${PROJECT_SOURCE_DIR}/src/apps/res/deskflow.ico
    ${CMAKE_CURRENT_BINARY_DIR}/${target}.rc
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

if(APPLE)
  set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY $<TARGET_BUNDLE_CONTENT_DIR:${CMAKE_PROJECT_PROPER_NAME}>/MacOS)
elseif(UNIX)
  install(TARGETS ${target} DESTINATION bin)
elseif(WIN32)
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

