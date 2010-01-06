
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

# determine the compiler to use for C programs
# NOTE, a generator may set CMAKE_C_COMPILER before
# loading this file to force a compiler.
# use environment variable CC first if defined by user, next use 
# the cmake variable CMAKE_GENERATOR_CC which can be defined by a generator
# as a default compiler
# If the internal cmake variable _CMAKE_TOOLCHAIN_PREFIX is set, this is used 
# as prefix for the tools (e.g. arm-elf-gcc, arm-elf-ar etc.). This works
# currently with the GNU crosscompilers.
#
# Sets the following variables: 
#   CMAKE_C_COMPILER
#   CMAKE_AR
#   CMAKE_RANLIB
#   CMAKE_COMPILER_IS_GNUCC
#
# If not already set before, it also sets
#   _CMAKE_TOOLCHAIN_PREFIX

IF(NOT CMAKE_C_COMPILER)
  SET(CMAKE_CXX_COMPILER_INIT NOTFOUND)

  # prefer the environment variable CC
  IF($ENV{CC} MATCHES ".+")
    GET_FILENAME_COMPONENT(CMAKE_C_COMPILER_INIT $ENV{CC} PROGRAM PROGRAM_ARGS CMAKE_C_FLAGS_ENV_INIT)
    IF(CMAKE_C_FLAGS_ENV_INIT)
      SET(CMAKE_C_COMPILER_ARG1 "${CMAKE_C_FLAGS_ENV_INIT}" CACHE STRING "First argument to C compiler")
    ENDIF(CMAKE_C_FLAGS_ENV_INIT)
    IF(NOT EXISTS ${CMAKE_C_COMPILER_INIT})
      MESSAGE(FATAL_ERROR "Could not find compiler set in environment variable CC:\n$ENV{CC}.") 
    ENDIF(NOT EXISTS ${CMAKE_C_COMPILER_INIT})
  ENDIF($ENV{CC} MATCHES ".+")

  # next try prefer the compiler specified by the generator
  IF(CMAKE_GENERATOR_CC) 
    IF(NOT CMAKE_C_COMPILER_INIT)
      SET(CMAKE_C_COMPILER_INIT ${CMAKE_GENERATOR_CC})
    ENDIF(NOT CMAKE_C_COMPILER_INIT)
  ENDIF(CMAKE_GENERATOR_CC)

  # finally list compilers to try
  IF(CMAKE_C_COMPILER_INIT)
    SET(CMAKE_C_COMPILER_LIST ${CMAKE_C_COMPILER_INIT})
  ELSE(CMAKE_C_COMPILER_INIT)
    SET(CMAKE_C_COMPILER_LIST ${_CMAKE_TOOLCHAIN_PREFIX}gcc ${_CMAKE_TOOLCHAIN_PREFIX}cc cl bcc xlc)
  ENDIF(CMAKE_C_COMPILER_INIT)

  # Find the compiler.
  IF (_CMAKE_USER_CXX_COMPILER_PATH)
    FIND_PROGRAM(CMAKE_C_COMPILER NAMES ${CMAKE_C_COMPILER_LIST} PATHS ${_CMAKE_USER_CXX_COMPILER_PATH} DOC "C compiler" NO_DEFAULT_PATH)
  ENDIF (_CMAKE_USER_CXX_COMPILER_PATH)
  FIND_PROGRAM(CMAKE_C_COMPILER NAMES ${CMAKE_C_COMPILER_LIST} DOC "C compiler")
  
  IF(CMAKE_C_COMPILER_INIT AND NOT CMAKE_C_COMPILER)
    SET(CMAKE_C_COMPILER "${CMAKE_C_COMPILER_INIT}" CACHE FILEPATH "C compiler" FORCE)
  ENDIF(CMAKE_C_COMPILER_INIT AND NOT CMAKE_C_COMPILER)
ELSE(NOT CMAKE_C_COMPILER)

  # we only get here if CMAKE_C_COMPILER was specified using -D or a pre-made CMakeCache.txt
  # (e.g. via ctest) or set in CMAKE_TOOLCHAIN_FILE
  # if CMAKE_C_COMPILER is a list of length 2, use the first item as 
  # CMAKE_C_COMPILER and the 2nd one as CMAKE_C_COMPILER_ARG1

  LIST(LENGTH CMAKE_C_COMPILER _CMAKE_C_COMPILER_LIST_LENGTH)
  IF("${_CMAKE_C_COMPILER_LIST_LENGTH}" EQUAL 2)
    LIST(GET CMAKE_C_COMPILER 1 CMAKE_C_COMPILER_ARG1)
    LIST(GET CMAKE_C_COMPILER 0 CMAKE_C_COMPILER)
  ENDIF("${_CMAKE_C_COMPILER_LIST_LENGTH}" EQUAL 2)

  # if a compiler was specified by the user but without path, 
  # now try to find it with the full path
  # if it is found, force it into the cache, 
  # if not, don't overwrite the setting (which was given by the user) with "NOTFOUND"
  # if the C compiler already had a path, reuse it for searching the CXX compiler
  GET_FILENAME_COMPONENT(_CMAKE_USER_C_COMPILER_PATH "${CMAKE_C_COMPILER}" PATH)
  IF(NOT _CMAKE_USER_C_COMPILER_PATH)
    FIND_PROGRAM(CMAKE_C_COMPILER_WITH_PATH NAMES ${CMAKE_C_COMPILER})
    MARK_AS_ADVANCED(CMAKE_C_COMPILER_WITH_PATH)
    IF(CMAKE_C_COMPILER_WITH_PATH)
      SET(CMAKE_C_COMPILER ${CMAKE_C_COMPILER_WITH_PATH} CACHE STRING "C compiler" FORCE)
    ENDIF(CMAKE_C_COMPILER_WITH_PATH)
  ENDIF(NOT _CMAKE_USER_C_COMPILER_PATH)
ENDIF(NOT CMAKE_C_COMPILER)
MARK_AS_ADVANCED(CMAKE_C_COMPILER)

IF (NOT _CMAKE_TOOLCHAIN_LOCATION)
  GET_FILENAME_COMPONENT(_CMAKE_TOOLCHAIN_LOCATION "${CMAKE_C_COMPILER}" PATH)
ENDIF (NOT _CMAKE_TOOLCHAIN_LOCATION)

# Build a small source file to identify the compiler.
IF(${CMAKE_GENERATOR} MATCHES "Visual Studio")
  SET(CMAKE_C_COMPILER_ID_RUN 1)
  SET(CMAKE_C_PLATFORM_ID "Windows")

  # TODO: Set the compiler id.  It is probably MSVC but
  # the user may be using an integrated Intel compiler.
  # SET(CMAKE_C_COMPILER_ID "MSVC")
ENDIF(${CMAKE_GENERATOR} MATCHES "Visual Studio")

IF(NOT CMAKE_C_COMPILER_ID_RUN)
  SET(CMAKE_C_COMPILER_ID_RUN 1)

  # Each entry in this list is a set of extra flags to try
  # adding to the compile line to see if it helps produce
  # a valid identification file.
  SET(CMAKE_C_COMPILER_ID_TEST_FLAGS
    # Try compiling to an object file only.
    "-c"

    # Try enabling ANSI mode on HP.
    "-Aa"
    )

  # Try to identify the compiler.
  SET(CMAKE_C_COMPILER_ID)
  FILE(READ ${CMAKE_ROOT}/Modules/CMakePlatformId.h.in
    CMAKE_C_COMPILER_ID_PLATFORM_CONTENT)
  INCLUDE(${CMAKE_ROOT}/Modules/CMakeDetermineCompilerId.cmake)
  CMAKE_DETERMINE_COMPILER_ID(C CFLAGS CMakeCCompilerId.c)

  # Set old compiler and platform id variables.
  IF("${CMAKE_C_COMPILER_ID}" MATCHES "GNU")
    SET(CMAKE_COMPILER_IS_GNUCC 1)
  ENDIF("${CMAKE_C_COMPILER_ID}" MATCHES "GNU")
  IF("${CMAKE_C_PLATFORM_ID}" MATCHES "MinGW")
    SET(CMAKE_COMPILER_IS_MINGW 1)
  ELSEIF("${CMAKE_C_PLATFORM_ID}" MATCHES "Cygwin")
    SET(CMAKE_COMPILER_IS_CYGWIN 1)
  ENDIF("${CMAKE_C_PLATFORM_ID}" MATCHES "MinGW")
ENDIF(NOT CMAKE_C_COMPILER_ID_RUN)

# If we have a gcc cross compiler, they have usually some prefix, like 
# e.g. powerpc-linux-gcc, arm-elf-gcc or i586-mingw32msvc-gcc .
# The other tools of the toolchain usually have the same prefix
# NAME_WE cannot be used since then this test will fail for names lile
# "arm-unknown-nto-qnx6.3.0-gcc.exe", where BASENAME would be 
# "arm-unknown-nto-qnx6" instead of the correct "arm-unknown-nto-qnx6.3.0-"
IF (CMAKE_CROSSCOMPILING  
    AND "${CMAKE_C_COMPILER_ID}" MATCHES "GNU"
    AND NOT _CMAKE_TOOLCHAIN_PREFIX)
  GET_FILENAME_COMPONENT(COMPILER_BASENAME "${CMAKE_C_COMPILER}" NAME)
  IF (COMPILER_BASENAME MATCHES "^(.+-)g?cc(\\.exe)?$")
    SET(_CMAKE_TOOLCHAIN_PREFIX ${CMAKE_MATCH_1})
  ENDIF (COMPILER_BASENAME MATCHES "^(.+-)g?cc(\\.exe)?$")

  # if "llvm-" is part of the prefix, remove it, since llvm doesn't have its own binutils
  # but uses the regular ar, objcopy, etc. (instead of llvm-objcopy etc.)
  IF ("${_CMAKE_TOOLCHAIN_PREFIX}" MATCHES "(.+-)?llvm-$")
    SET(_CMAKE_TOOLCHAIN_PREFIX ${CMAKE_MATCH_1})
  ENDIF ("${_CMAKE_TOOLCHAIN_PREFIX}" MATCHES "(.+-)?llvm-$")

ENDIF (CMAKE_CROSSCOMPILING  
    AND "${CMAKE_C_COMPILER_ID}" MATCHES "GNU"
    AND NOT _CMAKE_TOOLCHAIN_PREFIX)




INCLUDE(CMakeFindBinUtils)
IF(MSVC_C_ARCHITECTURE_ID)
  SET(SET_MSVC_C_ARCHITECTURE_ID
    "SET(MSVC_C_ARCHITECTURE_ID ${MSVC_C_ARCHITECTURE_ID})")
ENDIF(MSVC_C_ARCHITECTURE_ID)
# configure variables set in this file for fast reload later on
CONFIGURE_FILE(${CMAKE_ROOT}/Modules/CMakeCCompiler.cmake.in
  "${CMAKE_PLATFORM_ROOT_BIN}/CMakeCCompiler.cmake"
  @ONLY IMMEDIATE # IMMEDIATE must be here for compatibility mode <= 2.0
  ) 
SET(CMAKE_C_COMPILER_ENV_VAR "CC")
