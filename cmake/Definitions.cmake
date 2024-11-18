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

macro(configure_definitions)

  configure_ninja()
  configure_options()

  set(INTEG_TESTS_BIN integtests)
  set(UNIT_TESTS_BIN unittests)

  if(NOT "$ENV{GIT_SHA}" STREQUAL "")
    # Shorten the Git SHA to 8 chars for readability
    string(SUBSTRING "$ENV{GIT_SHA}" 0 8 GIT_SHA_SHORT)
    message(STATUS "Short Git SHA: ${GIT_SHA_SHORT}")
    add_definitions(-DGIT_SHA_SHORT="${GIT_SHA_SHORT}")
  endif()

  if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Disabling debug build")
    add_definitions(-DNDEBUG)
  endif()

  # TODO: find out why we need these, and remove them if we don't.
  if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
    cmake_policy(SET CMP0005 NEW)
  endif()

  # TODO: explain why we're adding headers to sources.
  if(${CMAKE_GENERATOR} STREQUAL "Unix Makefiles")
    set(ADD_HEADERS_TO_SOURCES FALSE)
  else()
    set(ADD_HEADERS_TO_SOURCES TRUE)
  endif()

  set(BIN_TEMP_DIR ${PROJECT_BINARY_DIR}/temp/bin)

endmacro()

macro(configure_ninja)
  # use response files so that ninja can compile on windows, otherwise you get
  # an error when linking qt: "The input line is too long."
  set(CMAKE_C_USE_RESPONSE_FILE_FOR_OBJECTS 1)
  set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_OBJECTS 1)
  set(CMAKE_C_RESPONSE_FILE_LINK_FLAG "@")
  set(CMAKE_CXX_RESPONSE_FILE_LINK_FLAG "@")
  set(CMAKE_NINJA_FORCE_RESPONSE_FILE
      1
      CACHE INTERNAL "")
endmacro()

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
