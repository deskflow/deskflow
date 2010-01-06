
#=============================================================================
# Copyright 2004-2009 Kitware, Inc.
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

# determine the compiler to use for C programs
# NOTE, a generator may set CMAKE_C_COMPILER before
# loading this file to force a compiler.
# use environment variable CCC first if defined by user, next use 
# the cmake variable CMAKE_GENERATOR_CC which can be defined by a generator
# as a default compiler
IF(NOT CMAKE_RC_COMPILER)
  # prefer the environment variable CC
  IF($ENV{RC} MATCHES ".+")
    GET_FILENAME_COMPONENT(CMAKE_RC_COMPILER_INIT $ENV{RC} PROGRAM PROGRAM_ARGS CMAKE_RC_FLAGS_ENV_INIT)
    IF(CMAKE_RC_FLAGS_ENV_INIT)
      SET(CMAKE_RC_COMPILER_ARG1 "${CMAKE_RC_FLAGS_ENV_INIT}" CACHE STRING "First argument to RC compiler")
    ENDIF(CMAKE_RC_FLAGS_ENV_INIT)
    IF(EXISTS ${CMAKE_RC_COMPILER_INIT})
    ELSE(EXISTS ${CMAKE_RC_COMPILER_INIT})
      MESSAGE(FATAL_ERROR "Could not find compiler set in environment variable RC:\n$ENV{RC}.") 
    ENDIF(EXISTS ${CMAKE_RC_COMPILER_INIT})
  ENDIF($ENV{RC} MATCHES ".+")
  
  # next try prefer the compiler specified by the generator
  IF(CMAKE_GENERATOR_RC) 
    IF(NOT CMAKE_RC_COMPILER_INIT)
      SET(CMAKE_RC_COMPILER_INIT ${CMAKE_GENERATOR_RC})
    ENDIF(NOT CMAKE_RC_COMPILER_INIT)
  ENDIF(CMAKE_GENERATOR_RC)

  # finally list compilers to try
  IF(CMAKE_RC_COMPILER_INIT)
    SET(CMAKE_RC_COMPILER_LIST ${CMAKE_RC_COMPILER_INIT})
  ELSE(CMAKE_RC_COMPILER_INIT)
    SET(CMAKE_RC_COMPILER_LIST rc)
  ENDIF(CMAKE_RC_COMPILER_INIT)

  # Find the compiler.
  FIND_PROGRAM(CMAKE_RC_COMPILER NAMES ${CMAKE_RC_COMPILER_LIST} DOC "RC compiler")
  IF(CMAKE_RC_COMPILER_INIT AND NOT CMAKE_RC_COMPILER)
    SET(CMAKE_RC_COMPILER "${CMAKE_RC_COMPILER_INIT}" CACHE FILEPATH "RC compiler" FORCE)
  ENDIF(CMAKE_RC_COMPILER_INIT AND NOT CMAKE_RC_COMPILER)
ENDIF(NOT CMAKE_RC_COMPILER)

MARK_AS_ADVANCED(CMAKE_RC_COMPILER)  


# configure variables set in this file for fast reload later on
CONFIGURE_FILE(${CMAKE_ROOT}/Modules/CMakeRCCompiler.cmake.in 
               ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeRCCompiler.cmake IMMEDIATE)
SET(CMAKE_RC_COMPILER_ENV_VAR "RC")
