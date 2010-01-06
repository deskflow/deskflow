
#=============================================================================
# Copyright 2002-2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distributed this file outside of CMake, substitute the full
#  License text for the above reference.)

# This module is purposely no longer documented.  It does nothing useful.
IF(NOT "${CMAKE_MINIMUM_REQUIRED_VERSION}" VERSION_LESS 2.7)
  MESSAGE(FATAL_ERROR
    "The functionality of this module has been dropped as of CMake 2.8.  "
    "It was deemed harmful (confusing users by changing their compiler).  "
    "Please remove calls to the CMAKE_EXPORT_BUILD_SETTINGS macro and "
    "stop including this module.  "
    "If this project generates any files for use by external projects, "
    "remove any use of the CMakeImportBuildSettings module from them.")
ENDIF()

# This macro used to store build settings of a project in a file to be
# loaded by another project using CMAKE_IMPORT_BUILD_SETTINGS.  Now it
# creates a file that refuses to load (with comment explaining why).
MACRO(CMAKE_EXPORT_BUILD_SETTINGS SETTINGS_FILE)
  IF(${SETTINGS_FILE} MATCHES ".+")
    CONFIGURE_FILE(${CMAKE_ROOT}/Modules/CMakeBuildSettings.cmake.in
                   ${SETTINGS_FILE} @ONLY IMMEDIATE)
  ELSE(${SETTINGS_FILE} MATCHES ".+")
    MESSAGE(SEND_ERROR "CMAKE_EXPORT_BUILD_SETTINGS called with no argument.")
  ENDIF(${SETTINGS_FILE} MATCHES ".+")
ENDMACRO(CMAKE_EXPORT_BUILD_SETTINGS)
