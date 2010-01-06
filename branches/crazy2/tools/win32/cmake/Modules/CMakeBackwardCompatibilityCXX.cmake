# - define a bunch of backwards compatibility variables
#  CMAKE_ANSI_CXXFLAGS - flag for ansi c++ 
#  CMAKE_HAS_ANSI_STRING_STREAM - has <strstream>
#  INCLUDE(TestForANSIStreamHeaders)
#  INCLUDE(CheckIncludeFileCXX)
#  INCLUDE(TestForSTDNamespace)
#  INCLUDE(TestForANSIForScope)

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

IF(NOT CMAKE_SKIP_COMPATIBILITY_TESTS)
  # check for some ANSI flags in the CXX compiler if it is not gnu
  IF(NOT CMAKE_COMPILER_IS_GNUCXX)
    INCLUDE(TestCXXAcceptsFlag)
    SET(CMAKE_TRY_ANSI_CXX_FLAGS "")
    IF(CMAKE_SYSTEM MATCHES "IRIX.*")
      SET(CMAKE_TRY_ANSI_CXX_FLAGS "-LANG:std")
    ENDIF(CMAKE_SYSTEM MATCHES "IRIX.*")
    IF(CMAKE_SYSTEM MATCHES "OSF.*")
      SET(CMAKE_TRY_ANSI_CXX_FLAGS "-std strict_ansi -nopure_cname")
    ENDIF(CMAKE_SYSTEM MATCHES "OSF.*")
    # if CMAKE_TRY_ANSI_CXX_FLAGS has something in it, see
    # if the compiler accepts it
    IF( CMAKE_TRY_ANSI_CXX_FLAGS MATCHES ".+")
      CHECK_CXX_ACCEPTS_FLAG(${CMAKE_TRY_ANSI_CXX_FLAGS} CMAKE_CXX_ACCEPTS_FLAGS)
      # if the compiler liked the flag then set CMAKE_ANSI_CXXFLAGS
      # to the flag
      IF(CMAKE_CXX_ACCEPTS_FLAGS)
        SET(CMAKE_ANSI_CXXFLAGS ${CMAKE_TRY_ANSI_CXX_FLAGS} CACHE INTERNAL 
        "What flags are required by the c++ compiler to make it ansi." )
      ENDIF(CMAKE_CXX_ACCEPTS_FLAGS)
    ENDIF( CMAKE_TRY_ANSI_CXX_FLAGS MATCHES ".+")
  ENDIF(NOT CMAKE_COMPILER_IS_GNUCXX)
  SET(CMAKE_CXX_FLAGS_SAVE ${CMAKE_CXX_FLAGS})
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_ANSI_CXXFLAGS}")
  INCLUDE(TestForANSIStreamHeaders)
  INCLUDE(CheckIncludeFileCXX)
  INCLUDE(TestForSTDNamespace)
  INCLUDE(TestForANSIForScope)
  INCLUDE(TestForSSTREAM)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_SAVE}")
ENDIF(NOT CMAKE_SKIP_COMPATIBILITY_TESTS)

