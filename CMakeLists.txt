# SPDX-FileCopyrightText: 2024 Deskflow Developers
# SPDX-FileCopyrightText: 2012 - 2024 Symless Ltd
# SPDX-FileCopyrightText: 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

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
set(DESKFLOW_VERSION_MINOR 19)
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

# Define Additional "PROJECT" vars for packaging and metadata
set(CMAKE_PROJECT_PROPER_NAME "Deskflow")
set(CMAKE_PROJECT_VENDOR "${CMAKE_PROJECT_PROPER_NAME} Devs")
set(CMAKE_PROJECT_COPYRIGHT "(C) 2024-2025 ${CMAKE_PROJECT_VENDOR}")
set(CMAKE_PROJECT_CONTACT "${CMAKE_PROJECT_PROPER_NAME} <maintainers@deskflow.org>")

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
    FILES ${PROJECT_SOURCE_DIR}/LICENSE
    DESTINATION .
  )
  install(
    FILES ${PROJECT_SOURCE_DIR}/LICENSES/LicenseRef-OpenSSL-Exception.txt
    DESTINATION .
    RENAME LICENSE_EXCEPTION
  )
elseif(UNIX AND NOT APPLE)
  install(
    FILES ${PROJECT_SOURCE_DIR}/LICENSE
    DESTINATION share/licenses/deskflow
  )
  install(
    FILES ${PROJECT_SOURCE_DIR}/LICENSES/LicenseRef-OpenSSL-Exception.txt
    DESTINATION share/licenses/deskflow
    RENAME LICENSE_EXCEPTION
  )
endif()

option(BUILD_INSTALLER "Build installer" ON)
if(BUILD_INSTALLER)
    add_subdirectory(deploy)
endif()
