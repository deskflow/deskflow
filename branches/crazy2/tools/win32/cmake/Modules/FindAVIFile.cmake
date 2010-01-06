# - Locate AVIFILE library and include paths
# AVIFILE (http://avifile.sourceforge.net/)is a set of libraries for 
# i386 machines
# to use various AVI codecs. Support is limited beyond Linux. Windows
# provides native AVI support, and so doesn't need this library.
# This module defines
#  AVIFILE_INCLUDE_DIR, where to find avifile.h , etc.
#  AVIFILE_LIBRARIES, the libraries to link against
#  AVIFILE_DEFINITIONS, definitions to use when compiling
#  AVIFILE_FOUND, If false, don't try to use AVIFILE

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

IF (UNIX)

  FIND_PATH(AVIFILE_INCLUDE_DIR avifile.h
    /usr/local/avifile/include
    /usr/local/include/avifile
  )

  FIND_LIBRARY(AVIFILE_AVIPLAY_LIBRARY aviplay
    /usr/local/avifile/lib
  )

ENDIF (UNIX)

SET (AVIFILE_FOUND "NO")

IF(AVIFILE_INCLUDE_DIR)
  IF(AVIFILE_AVIPLAY_LIBRARY)
    SET( AVIFILE_LIBRARIES  ${AVIFILE_AVIPLAY_LIBRARY} )
    SET( AVIFILE_FOUND "YES" )
    SET( AVIFILE_DEFINITIONS "")

  ENDIF(AVIFILE_AVIPLAY_LIBRARY)
ENDIF(AVIFILE_INCLUDE_DIR)


MARK_AS_ADVANCED(AVIFILE_INCLUDE_DIR AVIFILE_AVIPLAY_LIBRARY)
