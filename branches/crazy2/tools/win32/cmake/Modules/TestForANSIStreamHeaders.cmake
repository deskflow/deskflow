# - Test for compiler support of ANSI stream headers iostream, etc.
# check if the compiler supports the standard ANSI iostream header (without the .h)
#  CMAKE_NO_ANSI_STREAM_HEADERS - defined by the results
#

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

INCLUDE(CheckIncludeFileCXX)

IF(NOT CMAKE_NO_ANSI_STREAM_HEADERS)
  CHECK_INCLUDE_FILE_CXX(iostream CMAKE_ANSI_STREAM_HEADERS)
  IF (CMAKE_ANSI_STREAM_HEADERS)
    SET (CMAKE_NO_ANSI_STREAM_HEADERS 0 CACHE INTERNAL 
         "Does the compiler support headers like iostream.")
  ELSE (CMAKE_ANSI_STREAM_HEADERS)   
    SET (CMAKE_NO_ANSI_STREAM_HEADERS 1 CACHE INTERNAL 
       "Does the compiler support headers like iostream.")
  ENDIF (CMAKE_ANSI_STREAM_HEADERS)

  MARK_AS_ADVANCED(CMAKE_NO_ANSI_STREAM_HEADERS)
ENDIF(NOT CMAKE_NO_ANSI_STREAM_HEADERS)


