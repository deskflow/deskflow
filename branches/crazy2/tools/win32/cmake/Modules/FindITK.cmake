# - Find an ITK installation or build tree.

# When ITK is found, the ITKConfig.cmake file is sourced to setup the
# location and configuration of ITK.  Please read this file, or
# ITKConfig.cmake.in from the ITK source tree for the full list of
# definitions.  Of particular interest is ITK_USE_FILE, a CMake source file
# that can be included to set the include directories, library directories,
# and preprocessor macros.  In addition to the variables read from
# ITKConfig.cmake, this find module also defines
#  ITK_DIR  - The directory containing ITKConfig.cmake.  
#             This is either the root of the build tree, 
#             or the lib/InsightToolkit directory.  
#             This is the only cache entry.
#   
#  ITK_FOUND - Whether ITK was found.  If this is true, 
#              ITK_DIR is okay.
#
#  USE_ITK_FILE - The full path to the UseITK.cmake file.  
#                 This is provided for backward 
#                 compatability.  Use ITK_USE_FILE
#                 instead.

#=============================================================================
# Copyright 2001-2009 Kitware, Inc.
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

SET(ITK_DIR_STRING "directory containing ITKConfig.cmake.  This is either the root of the build tree, or PREFIX/lib/InsightToolkit for an installation.")

# Search only if the location is not already known.
IF(NOT ITK_DIR)
  # Get the system search path as a list.
  IF(UNIX)
    STRING(REGEX MATCHALL "[^:]+" ITK_DIR_SEARCH1 "$ENV{PATH}")
  ELSE(UNIX)
    STRING(REGEX REPLACE "\\\\" "/" ITK_DIR_SEARCH1 "$ENV{PATH}")
  ENDIF(UNIX)
  STRING(REGEX REPLACE "/;" ";" ITK_DIR_SEARCH2 ${ITK_DIR_SEARCH1})

  # Construct a set of paths relative to the system search path.
  SET(ITK_DIR_SEARCH "")
  FOREACH(dir ${ITK_DIR_SEARCH2})
    SET(ITK_DIR_SEARCH ${ITK_DIR_SEARCH} "${dir}/../lib/InsightToolkit")
  ENDFOREACH(dir)

  #
  # Look for an installation or build tree.
  #
  FIND_PATH(ITK_DIR ITKConfig.cmake
    # Look for an environment variable ITK_DIR.
    $ENV{ITK_DIR}

    # Look in places relative to the system executable search path.
    ${ITK_DIR_SEARCH}

    # Look in standard UNIX install locations.
    /usr/local/lib/InsightToolkit
    /usr/lib/InsightToolkit

    # Read from the CMakeSetup registry entries.  It is likely that
    # ITK will have been recently built.
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild1]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild2]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild3]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild4]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild5]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild6]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild7]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild8]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild9]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild10]

    # Help the user find it if we cannot.
    DOC "The ${ITK_DIR_STRING}"
  )
ENDIF(NOT ITK_DIR)

# If ITK was found, load the configuration file to get the rest of the
# settings.
IF(ITK_DIR)
  SET(ITK_FOUND 1)
  INCLUDE(${ITK_DIR}/ITKConfig.cmake)

  # Set USE_ITK_FILE for backward-compatability.
  SET(USE_ITK_FILE ${ITK_USE_FILE})
ELSE(ITK_DIR)
  SET(ITK_FOUND 0)
  IF(ITK_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Please set ITK_DIR to the ${ITK_DIR_STRING}")
  ENDIF(ITK_FIND_REQUIRED)
ENDIF(ITK_DIR)
