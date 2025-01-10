# Deskflow -- mouse and keyboard sharing utility
# Copyright (C) 2024 Symless Ltd.
# Copyright (C) 2009 Nick Bolton
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

# Why CMake 3.8?
# This allows package maintainers to create reproducible builds:
# > New in version 3.8: If the SOURCE_DATE_EPOCH environment variable is set,
# > its value will be used instead of the current time.
# > See https://reproducible-builds.org/specs/source-date-epoch/ for details.
cmake_minimum_required(VERSION 3.24)

# Link items by fill path
cmake_policy(SET CMP0003 NEW)

# Fix define escaping
cmake_policy(SET CMP0005 NEW)

# Set CXX Requirements
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fallback for when git can not be found
set(DESKFLOW_VERSION_MAJOR 1)
set(DESKFLOW_VERSION_MINOR 18)
set(DESKFLOW_VERSION_PATCH 0)
set(DESKFLOW_VERSION_TWEAK 0)

# Get the version from git if it's a git repository
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git)
  find_package(Git)
  if(GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse --short=8 HEAD
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      OUTPUT_VARIABLE GIT_SHA_SHORT
      ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-list --tags --count
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      OUTPUT_VARIABLE GIT_TAG_COUNT
      ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(GIT_TAG_COUNT EQUAL 0)
      set(DESKFLOW_VERSION_TWEAK "9999")
    else()
      execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --long --match v* --always
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE GITREV
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
      )
      string(FIND ${GITREV} "v" isRev)
      if(NOT ifRev EQUAL -1)
        string(REGEX MATCH [0-9]+ MAJOR ${GITREV})
        string(REGEX MATCH \\.[0-9]+ MINOR ${GITREV})
        string(REPLACE "." "" MINOR "${MINOR}")
        string(REGEX MATCH [0-9]+\- PATCH ${GITREV})
        string(REPLACE "-" "" PATCH "${PATCH}")
        string(REGEX MATCH \-[0-9]+\- TWEAK ${GITREV})
        string(REPLACE "-" "" TWEAK "${TWEAK}")
        set(DESKFLOW_VERSION_MAJOR ${MAJOR})
        set(DESKFLOW_VERSION_MINOR ${MINOR})
        set(DESKFLOW_VERSION_PATCH ${PATCH})
        set(DESKFLOW_VERSION_TWEAK ${TWEAK})
      elseif(NOT ${GITREV} STREQUAL "")
        set(DESKFLOW_VERSION_TWEAK ${GITREV})
      endif()
    endif()
  endif()
endif()

#Define our project
project(
  deskflow
  VERSION "${DESKFLOW_VERSION_MAJOR}.${DESKFLOW_VERSION_MINOR}.${DESKFLOW_VERSION_PATCH}.${DESKFLOW_VERSION_TWEAK}"
  DESCRIPTION "Mouse and keyboard sharing utility"
  LANGUAGES C CXX)

#Used for embedded copyright on binaries, limited space.
set(CMAKE_PROJECT_COPYRIGHT "(C) 2024-2025 Deskflow Devs")

#Unset the vars used in the project call
unset(DESKFLOW_VERSION_MAJOR)
unset(DESKFLOW_VERSION_MINOR)
unset(DESKFLOW_VERSION_PATCH)
unset(DESKFLOW_VERSION_TWEAK)

message(STATUS "Building ${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION}")

# Set lib versions
set(REQUIRED_OPENSSL_VERSION 3.0)
set(REQUIRED_LIBEI_VERSION 1.3)
set(REQUIRED_LIBPORTAL_VERSION 0.8)
set(REQUIRED_QT_VERSION 6.7.0)

# Control debug item visibility
# When not set logging is forced to DEBUG and show code locations
# Also exposes a test menu
if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Disabling debug build")
  add_definitions(-DNDEBUG)
endif()

# Set required macOS SDK
if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET 12)
endif()

# Set Output Folders
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")

include(cmake/Libraries.cmake)
include(GNUInstallDirs)

configure_libs()

add_subdirectory(doc)
add_subdirectory(src)

# Install License, License is in the App Bundle on mac os (src/gui)
if(WIN32)
  install(
    FILES
      ${PROJECT_SOURCE_DIR}/LICENSE
      ${PROJECT_SOURCE_DIR}/LICENSE_EXCEPTION
    DESTINATION .
  )
elseif(UNIX AND NOT APPLE)
  install(
    FILES
      ${PROJECT_SOURCE_DIR}/LICENSE
      ${PROJECT_SOURCE_DIR}/LICENSE_EXCEPTION
    DESTINATION share/licenses/deskflow
  )
endif()

option(BUILD_INSTALLER "Build installer" ON)
if(BUILD_INSTALLER)
    add_subdirectory(deploy)
endif()
