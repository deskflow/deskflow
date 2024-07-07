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

  if("${VERSION_URL}" STREQUAL "")
    set(VERSION_URL "https://api.symless.com/version?version=v1")
  endif()
  add_definitions(-DSYNERGY_VERSION_URL="${VERSION_URL}")

  if(NOT "${SYNERGY_GIT_SHA}" STREQUAL "")
    set(GIT_SHA "${SYNERGY_GIT_SHA}")
    message(STATUS "Git SHA: ${GIT_SHA}")
    add_definitions(-DSYNERGY_GIT_SHA="${GIT_SHA}")
  endif()

  if(ENABLE_LICENSING)
    message(STATUS "Licensing enabled")
    add_definitions(-DSYNERGY_ENABLE_LICENSING=1)
  else()
    set(PRODUCT_NAME "Synergy 1 Community Edition")
    if(NOT "$ENV{SYNERGY_PRODUCT_NAME}" STREQUAL "")
      set(PRODUCT_NAME $ENV{SYNERGY_PRODUCT_NAME})
    endif()
    message(STATUS "Product name: ${PRODUCT_NAME}")
    add_definitions(-DSYNERGY_PRODUCT_NAME="${PRODUCT_NAME}")
  endif()

  if(ENABLE_AUTO_CONFIG)
    message(STATUS "Auto config enabled")
    add_definitions(-DSYNERGY_ENABLE_AUTO_CONFIG=1)
  endif()

  if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Disabling debug build")
    add_definitions(-DNDEBUG)
  endif()

  # TODO: find out why we need these, and remove them if we don't
  if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
    cmake_policy(SET CMP0005 NEW)
  endif()

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

  # unified binary is off by default for now, for backwards compatibility.
  set(DEFAULT_BUILD_UNIFIED OFF)

  # coverage is off by default because it's GCC only and a developer preference.
  set(DEFAULT_ENABLE_COVERAGE OFF)

  # licensing is off by default to make life easier for contributors.
  set(DEFAULT_ENABLE_LICENSING OFF)

  if("$ENV{SYNERGY_BUILD_MINIMAL}" STREQUAL "true")
    set(DEFAULT_BUILD_GUI OFF)
    set(DEFAULT_BUILD_INSTALLER OFF)
  endif()

  if("$ENV{SYNERGY_BUILD_TESTS}" STREQUAL "false")
    set(DEFAULT_BUILD_TESTS OFF)
  endif()

  if("$ENV{SYNERGY_BUILD_UNIFIED}" STREQUAL "true")
    set(DEFAULT_BUILD_UNIFIED ON)
  endif()

  if("$ENV{SYNERGY_ENABLE_LICENSING}" STREQUAL "true")
    set(DEFAULT_ENABLE_LICENSING ON)
  endif()

  if("$ENV{SYNERGY_ENABLE_COVERAGE}" STREQUAL "true")
    set(DEFAULT_ENABLE_COVERAGE ON)
  endif()

  option(BUILD_GUI "Build GUI" ${DEFAULT_BUILD_GUI})
  option(BUILD_INSTALLER "Build installer" ${DEFAULT_BUILD_INSTALLER})
  option(BUILD_TESTS "Build tests" ${DEFAULT_BUILD_TESTS})
  option(BUILD_UNIFIED "Build unified binary" ${DEFAULT_BUILD_UNIFIED})
  option(ENABLE_LICENSING "Enable licensing" ${DEFAULT_ENABLE_LICENSING})
  option(ENABLE_COVERAGE "Enable test coverage" ${DEFAULT_ENABLE_COVERAGE})

  # auto config is off by default because it requires bonjour, which sucks.
  option(ENABLE_AUTO_CONFIG "Enable auto config (zeroconf)" OFF)

endmacro()
