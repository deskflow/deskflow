# - Find a VTK installation or build tree.
# The following variables are set if VTK is found.  If VTK is not
# found, VTK_FOUND is set to false.
#  VTK_FOUND         - Set to true when VTK is found.
#  VTK_USE_FILE      - CMake file to use VTK.
#  VTK_MAJOR_VERSION - The VTK major version number.
#  VTK_MINOR_VERSION - The VTK minor version number 
#                       (odd non-release).
#  VTK_BUILD_VERSION - The VTK patch level 
#                       (meaningless for odd minor).
#  VTK_INCLUDE_DIRS  - Include directories for VTK
#  VTK_LIBRARY_DIRS  - Link directories for VTK libraries
#  VTK_KITS          - List of VTK kits, in CAPS 
#                      (COMMON,IO,) etc.
#  VTK_LANGUAGES     - List of wrapped languages, in CAPS
#                      (TCL, PYHTON,) etc.
# The following cache entries must be set by the user to locate VTK:
#  VTK_DIR  - The directory containing VTKConfig.cmake.  
#             This is either the root of the build tree,
#             or the lib/vtk directory.  This is the 
#             only cache entry.
# The following variables are set for backward compatibility and
# should not be used in new code:
#  USE_VTK_FILE - The full path to the UseVTK.cmake file.
#                 This is provided for backward 
#                 compatibility.  Use VTK_USE_FILE 
#                 instead.
#

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

# Assume not found.
SET(VTK_FOUND 0)

# VTK 4.0 did not provide VTKConfig.cmake.
IF("${VTK_FIND_VERSION}" VERSION_LESS 4.1)
  SET(_VTK_40_ALLOW 1)
  IF(VTK_FIND_VERSION)
    SET(_VTK_40_ONLY 1)
  ENDIF()
ENDIF()

# Construct consitent error messages for use below.
SET(VTK_DIR_DESCRIPTION "directory containing VTKConfig.cmake.  This is either the root of the build tree, or PREFIX/lib/vtk for an installation.")
IF(_VTK_40_ALLOW)
  SET(VTK_DIR_DESCRIPTION "${VTK_DIR_DESCRIPTION}  For VTK 4.0, this is the location of UseVTK.cmake.  This is either the root of the build tree or PREFIX/include/vtk for an installation.")
ENDIF()
SET(VTK_DIR_MESSAGE "VTK not found.  Set the VTK_DIR cmake cache entry to the ${VTK_DIR_DESCRIPTION}")

# Check whether VTK 4.0 has already been found.
IF(_VTK_40_ALLOW AND VTK_DIR)
  IF(EXISTS ${VTK_DIR}/UseVTK.cmake AND NOT EXISTS ${VTK_DIR}/VTKConfig.cmake)
    SET(VTK_FOUND 1)
    INCLUDE(UseVTKConfig40) # No VTKConfig; load VTK 4.0 settings.
  ENDIF()
ENDIF()

# Use the Config mode of the find_package() command to find VTKConfig.
# If this succeeds (possibly because VTK_DIR is already set), the
# command will have already loaded VTKConfig.cmake and set VTK_FOUND.
IF(NOT _VTK_40_ONLY AND NOT VTK_FOUND)
  FIND_PACKAGE(VTK QUIET NO_MODULE)
ENDIF()

# Special search for VTK 4.0.
IF(_VTK_40_ALLOW AND NOT VTK_DIR)
  # Old scripts may set these directories in the CMakeCache.txt file.
  # They can tell us where to find VTKConfig.cmake.
  SET(VTK_DIR_SEARCH_LEGACY "")
  IF(VTK_BINARY_PATH AND USE_BUILT_VTK)
    SET(VTK_DIR_SEARCH_LEGACY ${VTK_DIR_SEARCH_LEGACY} ${VTK_BINARY_PATH})
  ENDIF(VTK_BINARY_PATH AND USE_BUILT_VTK)
  IF(VTK_INSTALL_PATH AND USE_INSTALLED_VTK)
    SET(VTK_DIR_SEARCH_LEGACY ${VTK_DIR_SEARCH_LEGACY}
        ${VTK_INSTALL_PATH}/lib/vtk)
  ENDIF(VTK_INSTALL_PATH AND USE_INSTALLED_VTK)

  # Look for UseVTK.cmake in build trees or under <prefix>/include/vtk.
  FIND_PATH(VTK_DIR
    NAMES UseVTK.cmake
    PATH_SUFFIXES vtk-4.0 vtk
    HINTS $ENV{VTK_DIR}

    PATHS

    # Support legacy cache files.
    ${VTK_DIR_SEARCH_LEGACY}

    # Read from the CMakeSetup registry entries.  It is likely that
    # VTK will have been recently built.
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
    DOC "The ${VTK_DIR_DESCRIPTION}"
    )

  IF(VTK_DIR)
    IF(EXISTS ${VTK_DIR}/UseVTK.cmake AND NOT EXISTS ${VTK_DIR}/VTKConfig.cmake)
      SET(VTK_FOUND 1)
      INCLUDE(UseVTKConfig40) # No VTKConfig; load VTK 4.0 settings.
    ELSE()
      # We found the wrong version.  Pretend we did not find it.
      SET(VTK_DIR "VTK_DIR-NOTFOUND" CACHE PATH "The ${VTK_DIR_DESCRIPTION}" FORCE)
    ENDIF()
  ENDIF()
ENDIF()

#-----------------------------------------------------------------------------
IF(VTK_FOUND)
  # Set USE_VTK_FILE for backward-compatability.
  SET(USE_VTK_FILE ${VTK_USE_FILE})
ELSE(VTK_FOUND)
  # VTK not found, explain to the user how to specify its location.
  IF(VTK_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR ${VTK_DIR_MESSAGE})
  ELSE(VTK_FIND_REQUIRED)
    IF(NOT VTK_FIND_QUIETLY)
      MESSAGE(STATUS ${VTK_DIR_MESSAGE})
    ENDIF(NOT VTK_FIND_QUIETLY)
  ENDIF(VTK_FIND_REQUIRED)
ENDIF(VTK_FOUND)
