# Locate QuickTime
# This module defines
# QUICKTIME_LIBRARY
# QUICKTIME_FOUND, if false, do not try to link to gdal 
# QUICKTIME_INCLUDE_DIR, where to find the headers
#
# $QUICKTIME_DIR is an environment variable that would
# correspond to the ./configure --prefix=$QUICKTIME_DIR
#
# Created by Eric Wing. 

#=============================================================================
# Copyright 2007-2009 Kitware, Inc.
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

# QuickTime on OS X looks different than QuickTime for Windows,
# so I am going to case the two.

IF(APPLE)
  FIND_PATH(QUICKTIME_INCLUDE_DIR QuickTime/QuickTime.h)
  FIND_LIBRARY(QUICKTIME_LIBRARY QuickTime)
ELSE(APPLE)
  FIND_PATH(QUICKTIME_INCLUDE_DIR QuickTime.h
    HINTS
    $ENV{QUICKTIME_DIR}/include
    $ENV{QUICKTIME_DIR}
  )
  FIND_LIBRARY(QUICKTIME_LIBRARY QuickTime
    HINTS
    $ENV{QUICKTIME_DIR}/lib
    $ENV{QUICKTIME_DIR}
  )
ENDIF(APPLE)

SET(QUICKTIME_FOUND "NO")
IF(QUICKTIME_LIBRARY AND QUICKTIME_INCLUDE_DIR)
  SET(QUICKTIME_FOUND "YES")
ENDIF(QUICKTIME_LIBRARY AND QUICKTIME_INCLUDE_DIR)


