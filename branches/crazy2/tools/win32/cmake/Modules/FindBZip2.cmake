# - Try to find BZip2
# Once done this will define
#
#  BZIP2_FOUND - system has BZip2
#  BZIP2_INCLUDE_DIR - the BZip2 include directory
#  BZIP2_LIBRARIES - Link these to use BZip2
#  BZIP2_DEFINITIONS - Compiler switches required for using BZip2
#  BZIP2_NEED_PREFIX - this is set if the functions are prefixed with BZ2_

#=============================================================================
# Copyright 2006-2009 Kitware, Inc.
# Copyright 2006 Alexander Neundorf <neundorf@kde.org>
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

IF (BZIP2_INCLUDE_DIR AND BZIP2_LIBRARIES)
    SET(BZip2_FIND_QUIETLY TRUE)
ENDIF (BZIP2_INCLUDE_DIR AND BZIP2_LIBRARIES)

FIND_PATH(BZIP2_INCLUDE_DIR bzlib.h )

FIND_LIBRARY(BZIP2_LIBRARIES NAMES bz2 bzip2 )

# handle the QUIETLY and REQUIRED arguments and set BZip2_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(BZip2 DEFAULT_MSG BZIP2_LIBRARIES BZIP2_INCLUDE_DIR)

IF (BZIP2_FOUND)
   INCLUDE(CheckLibraryExists)
   CHECK_LIBRARY_EXISTS(${BZIP2_LIBRARIES} BZ2_bzCompressInit "" BZIP2_NEED_PREFIX)
ENDIF (BZIP2_FOUND)

MARK_AS_ADVANCED(BZIP2_INCLUDE_DIR BZIP2_LIBRARIES)

