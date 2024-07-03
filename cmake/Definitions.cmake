# Synergy -- mouse and keyboard sharing utility
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

  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_EXTENSIONS OFF)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

  configure_ninja()
  configure_options()

  if(ENABLE_ACTIVATION)
    add_definitions(-DENABLE_ACTIVATION=1)
  endif()

  if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DNDEBUG)
  endif()

  # TODO: Find out why we need these, and remove them if we don't
  if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
    cmake_policy(SET CMP0005 NEW)
  endif()

  # Add headers to source list
  if(${CMAKE_GENERATOR} STREQUAL "Unix Makefiles")
    set(SYNERGY_ADD_HEADERS FALSE)
  else()
    set(SYNERGY_ADD_HEADERS TRUE)
  endif()
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
  set(DEFAULT_BUILD_INSTALLER ON)
  set(DEFAULT_BUILD_TESTS ON)
  set(DEFAULT_BUILD_UNIFIED OFF)
  set(DEFAULT_ENABLE_ACTIVATION OFF)
  set(DEFAULT_ENABLE_COVERAGE OFF)

  if(DEFINED $ENV{SYNERGY_BUILD_MINIMAL})
    set(DEFAULT_BUILD_GUI OFF)
    set(DEFAULT_BUILD_INSTALLER OFF)
  endif()

  if(DEFINED $ENV{SYNERGY_NO_TESTS})
    set(DEFAULT_BUILD_TESTS OFF)
  endif()

  if(DEFINED $ENV{SYNERGY_BUILD_UNIFIED})
    set(DEFAULT_BUILD_UNIFIED ON)
  endif()

  if(DEFINED $ENV{SYNERGY_ENABLE_ACTIVATION})
    set(DEFAULT_ENABLE_ACTIVATION ON)
  endif()

  option(BUILD_GUI "Build GUI" ${DEFAULT_BUILD_GUI})
  option(BUILD_INSTALLER "Build installer" ${DEFAULT_BUILD_INSTALLER})
  option(BUILD_TESTS "Build tests" ${DEFAULT_BUILD_TESTS})
  option(BUILD_UNIFIED "Build unified binary" ${DEFAULT_BUILD_UNIFIED})
  option(ENABLE_ACTIVATION "Enable activation" ${DEFAULT_ENABLE_ACTIVATION})
  option(ENABLE_COVERAGE "Enable test coverage" ${DEFAULT_ENABLE_COVERAGE})

endmacro()
