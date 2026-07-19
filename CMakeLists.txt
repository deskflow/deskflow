# SPDX-FileCopyrightText: (C) 2024 - 2026 Deskflow Developers
# SPDX-FileCopyrightText: (C) 2012 - 2024 Symless Ltd
# SPDX-FileCopyrightText: (C) 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.24)

# Link items by fill path
cmake_policy(SET CMP0003 NEW)

# Fix define escaping
cmake_policy(SET CMP0005 NEW)

# De-duplicate libraries on linker calls
if(POLICY CMP0156)
  cmake_policy(SET CMP0156 NEW)
endif()

#Project level options
option(BUILD_INSTALLER "Build installer" ON)
if(UNIX)
  if(APPLE)
    option(BUILD_OSX_BUNDLE "Build mac os bundle" ON)
  else()
    option(BUILD_X11_SUPPORT "Build with x11 support" ON)
  endif()
elseif(WIN32)
  option (VCPKG_QT "Use Qt from VCPKG" OFF)
endif()

# Set CXX Requirements
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fallback for when git can not be found
set(DESKFLOW_VERSION_MAJOR 1)
set(DESKFLOW_VERSION_MINOR 26)
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
      COMMAND ${GIT_EXECUTABLE} describe --long --match v* --always
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      OUTPUT_VARIABLE GITREV
      ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    string(FIND "${GITREV}" "v" isRev)
    if(NOT ${isRev} EQUAL -1)
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
    else()
      set(DESKFLOW_VERSION_TWEAK "9999")
    endif()
    unset(GITREV)
  endif()
endif()

#generate vcpkg file if needed
if(WIN32)
    if(VCPKG_QT)
        set(QT_LIBS ", \"qttranslations\", \"qtsvg\"")
    endif()
    configure_file(cmake/vcpkg.json.in ${CMAKE_SOURCE_DIR}/vcpkg.json @ONLY)
endif()

#Define our project
project(
  deskflow
  VERSION "${DESKFLOW_VERSION_MAJOR}.${DESKFLOW_VERSION_MINOR}.${DESKFLOW_VERSION_PATCH}.${DESKFLOW_VERSION_TWEAK}"
  DESCRIPTION "Keyboard and mouse sharing utility"
  HOMEPAGE_URL "https://deskflow.org"
  LANGUAGES C CXX
)

# Define Additional "PROJECT" vars for packaging and metadata
set(CMAKE_PROJECT_PROPER_NAME "Deskflow")
set(CMAKE_PROJECT_VENDOR "${CMAKE_PROJECT_PROPER_NAME} Devs")
set(CMAKE_PROJECT_COPYRIGHT "(C) 2024-2026 ${CMAKE_PROJECT_VENDOR}")
set(CMAKE_PROJECT_CONTACT "${CMAKE_PROJECT_PROPER_NAME} <maintainers@deskflow.org>")
set(CMAKE_PROJECT_REV_FQDN "org.deskflow.deskflow")

#Unset the vars used in the project call
unset(DESKFLOW_VERSION_MAJOR)
unset(DESKFLOW_VERSION_MINOR)
unset(DESKFLOW_VERSION_PATCH)
unset(DESKFLOW_VERSION_TWEAK)

message(STATUS "Building ${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION}")

# Set lib versions
set(REQUIRED_OPENSSL_VERSION 3.0)
set(REQUIRED_LIBEI_VERSION 1.3)
set(REQUIRED_LIBPORTAL_VERSION 0.9.1)
set(REQUIRED_QT_VERSION 6.7.0)

if (WIN32)
  # VSCMD_ARG_TGT_ARCH is set on CI
  if ("$ENV{VSCMD_ARG_TGT_ARCH}" STREQUAL "")
    # NOT on CI
    if (CMAKE_SYSTEM_PROCESSOR MATCHES "[Aa][Rr][Mm]64")
      set(BUILD_ARCHITECTURE arm64)
    else()
      set(BUILD_ARCHITECTURE x64)
    endif()
  else()
    set (BUILD_ARCHITECTURE $ENV{VSCMD_ARG_TGT_ARCH})
  endif()
else()
  set (BUILD_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
endif()

if (MSVC)
  # Require the same MSVC runtime that built the binary. Mitigates access violations
  # from accidental ABI-compatibility breakage across runtime minor versions.
  #
  # Derive the minor from the compiler toolset (cl major 19 maps to CRT major 14 and
  # the minor matches, e.g. 19.44 -> 14.44) rather than the host's installed redist
  # from the registry. The toolset version tracks the runtime it actually produces,
  # including the debug CRT (vcruntime140d.dll); the registry redist can be newer than
  # the loaded DLL and make the runtime guard reject a valid local build.
  set(REQUIRED_MSVC_RUNTIME_MAJOR 14)
  if (CMAKE_CXX_COMPILER_VERSION MATCHES "^[0-9]+\\.([0-9]+)")
    set(REQUIRED_MSVC_RUNTIME_MINOR "${CMAKE_MATCH_1}")
    message(STATUS "MSVC runtime: ${REQUIRED_MSVC_RUNTIME_MAJOR}.${REQUIRED_MSVC_RUNTIME_MINOR}")
  else()
    message(FATAL_ERROR "Failed to parse MSVC compiler version: ${CMAKE_CXX_COMPILER_VERSION}")
  endif()
endif()

# Control debug item visibility
# When not set logging is forced to DEBUG and show code locations
# Also exposes a test menu
if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Disabling debug build")
  add_definitions(-DNDEBUG)
endif()

# Set Output Folders
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")

# disables the use of signals,slots and emit
# Instead use Q_SIGNAL, Q_SLOT and Q_EMIT
# prevents issues when used with glib for libportal
add_definitions(-DQT_NO_KEYWORDS)

#Find and setup Qt
find_package(Qt6 ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Core Widgets Network)
if(UNIX AND NOT APPLE)
    find_package(Qt6 ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS DBus Xml)
endif()

# Define the location of Qt deployment tool
if(WIN32)
  if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND VCPKG_QT)
    set(DEPLOY_TOOL windeployqt.debug.bat)
  else()
    set(DEPLOY_TOOL windeployqt)
  endif()
elseif(APPLE)
    set(DEPLOY_TOOL macdeployqt)
endif()

if (WIN32 OR APPLE)
  find_program(DEPLOYQT ${DEPLOY_TOOL})
  if(DEPLOYQT STREQUAL "DEPLOYQT-NOTFOUND")
    message(FATAL_ERROR "Unable to locate the Qt Deploy Tool: \"${DEPLOY_TOOL}\"")
  endif()
  unset(DEPLOY_TOOL)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

message(STATUS "Qt version: ${Qt6_VERSION}")

include(cmake/Libraries.cmake)
configure_libs()

if(BUILD_OSX_BUNDLE AND APPLE_CODESIGN_DEV)
  include(cmake/MacCodesign.cmake)
endif()

# setup install paths
include(GNUInstallDirs)
if (WIN32)
  set(CMAKE_INSTALL_BINDIR .)
  set(CMAKE_INSTALL_LIBDIR .)
  set(CMAKE_INSTALL_DOCDIR docs)
  set(CMAKE_INSTALL_LICENSE_DIR .)
  set(CMAKE_INSTALL_I18N_DIR translations)
elseif(BUILD_OSX_BUNDLE)
  set(CMAKE_INSTALL_BINDIR ${CMAKE_PROJECT_PROPER_NAME}.app/Contents/MacOS)
  set(CMAKE_INSTALL_LICENSE_DIR ${CMAKE_PROJECT_PROPER_NAME}.app/Contents/Resources)
else()
  set(CMAKE_INSTALL_LICENSE_DIR ${CMAKE_INSTALL_DATADIR}/licenses/${CMAKE_PROJECT_NAME})
  set(CMAKE_INSTALL_I18N_DIR ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}/translations)
endif()

add_subdirectory(docs)

# build translations before source, I18N unit tests fail if they are missing
add_subdirectory(translations)

add_subdirectory(src)

if(BUILD_INSTALLER)
    add_subdirectory(deploy)
endif()

install(
  FILES ${PROJECT_SOURCE_DIR}/LICENSE
  DESTINATION ${CMAKE_INSTALL_LICENSE_DIR}
)

install(
  FILES ${PROJECT_SOURCE_DIR}/LICENSES/LicenseRef-OpenSSL-Exception.txt
  DESTINATION ${CMAKE_INSTALL_LICENSE_DIR}
  RENAME LICENSE_EXCEPTION
)
