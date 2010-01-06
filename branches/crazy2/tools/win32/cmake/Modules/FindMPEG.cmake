# - Find the native MPEG includes and library
# This module defines
#  MPEG_INCLUDE_DIR, where to find MPEG.h, etc.
#  MPEG_LIBRARIES, the libraries required to use MPEG.
#  MPEG_FOUND, If false, do not try to use MPEG.
# also defined, but not for general use are
#  MPEG_mpeg2_LIBRARY, where to find the MPEG library.
#  MPEG_vo_LIBRARY, where to find the vo library.

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

FIND_PATH(MPEG_INCLUDE_DIR mpeg2dec/include/video_out.h
  /usr/local/livid
)

FIND_LIBRARY(MPEG_mpeg2_LIBRARY mpeg2
  /usr/local/livid/mpeg2dec/libmpeg2/.libs
)

FIND_LIBRARY( MPEG_vo_LIBRARY vo
  /usr/local/livid/mpeg2dec/libvo/.libs
)

# handle the QUIETLY and REQUIRED arguments and set MPEG2_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(MPEG DEFAULT_MSG MPEG_INCLUDE_DIR MPEG_mpeg2_LIBRARY)

IF(MPEG_FOUND)
  SET( MPEG_LIBRARIES ${MPEG_mpeg2_LIBRARY} ${MPEG_vo_LIBRARY} )
ENDIF(MPEG_FOUND)
