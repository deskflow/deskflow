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

# Daemon is only needed on Windows for elevating processes to deal with UAC.
if(WIN32)
  set(target ${CMAKE_PROJECT_NAME}-daemon)

  # Generate rc file
  set(EXE_DESCRIPTION "This service runs in the background to help ${CMAKE_PROJECT_NAME} deal with UAC prompts")
  configure_file(${CMAKE_SOURCE_DIR}/src/apps/res/rc_template.rc.in ${target}.rc)

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
