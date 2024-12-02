# Deskflow -- mouse and keyboard sharing utility
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

macro(configure_options)

  set(DEFAULT_BUILD_GUI ON)
  set(DEFAULT_BUILD_TESTS ON)

  # unified binary is off by default for now, for backwards compatibility.
  set(DEFAULT_BUILD_UNIFIED OFF)

  # coverage is off by default because it's GCC only and a developer preference.
  set(DEFAULT_ENABLE_COVERAGE OFF)

  if("$ENV{DESKFLOW_BUILD_MINIMAL}" STREQUAL "true")
    set(DEFAULT_BUILD_GUI OFF)
    set(DEFAULT_BUILD_INSTALLER OFF)
  endif()

  if("$ENV{DESKFLOW_BUILD_TESTS}" STREQUAL "false")
    set(DEFAULT_BUILD_TESTS OFF)
  endif()

  if("$ENV{DESKFLOW_BUILD_UNIFIED}" STREQUAL "true")
    set(DEFAULT_BUILD_UNIFIED ON)
  endif()

  if("$ENV{DESKFLOW_ENABLE_COVERAGE}" STREQUAL "true")
    set(DEFAULT_ENABLE_COVERAGE ON)
  endif()

  option(BUILD_GUI "Build GUI" ${DEFAULT_BUILD_GUI})
  option(BUILD_TESTS "Build tests" ${DEFAULT_BUILD_TESTS})
  option(BUILD_UNIFIED "Build unified binary" ${DEFAULT_BUILD_UNIFIED})
  option(ENABLE_COVERAGE "Enable test coverage" ${DEFAULT_ENABLE_COVERAGE})

endmacro()
