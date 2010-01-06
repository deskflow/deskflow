# - Find TIFF library
# Find the native TIFF includes and library
# This module defines
#  TIFF_INCLUDE_DIR, where to find tiff.h, etc.
#  TIFF_LIBRARIES, libraries to link against to use TIFF.
#  TIFF_FOUND, If false, do not try to use TIFF.
# also defined, but not for general use are
#  TIFF_LIBRARY, where to find the TIFF library.

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

FIND_PATH(TIFF_INCLUDE_DIR tiff.h)

SET(TIFF_NAMES ${TIFF_NAMES} tiff libtiff libtiff3)
FIND_LIBRARY(TIFF_LIBRARY NAMES ${TIFF_NAMES} )

# handle the QUIETLY and REQUIRED arguments and set TIFF_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(TIFF  DEFAULT_MSG  TIFF_LIBRARY  TIFF_INCLUDE_DIR)

IF(TIFF_FOUND)
  SET( TIFF_LIBRARIES ${TIFF_LIBRARY} )
ENDIF(TIFF_FOUND)

MARK_AS_ADVANCED(TIFF_INCLUDE_DIR TIFF_LIBRARY)
