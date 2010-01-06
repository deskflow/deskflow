
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

# determine the compiler to use for C++ programs
# NOTE, a generator may set CMAKE_CXX_COMPILER before
# loading this file to force a compiler.
# use environment variable CXX first if defined by user, next use 
# the cmake variable CMAKE_GENERATOR_CXX which can be defined by a generator
# as a default compiler
# If the internal cmake variable _CMAKE_TOOLCHAIN_PREFIX is set, this is used 
# as prefix for the tools (e.g. arm-elf-g++, arm-elf-ar etc.)
#
# Sets the following variables:
#   CMAKE_CXX_COMPILER
#   CMAKE_COMPILER_IS_GNUCXX
#   CMAKE_AR
#   CMAKE_RANLIB
#
# If not already set before, it also sets
#   _CMAKE_TOOLCHAIN_PREFIX

IF(NOT CMAKE_CXX_COMPILER)
  SET(CMAKE_CXX_COMPILER_INIT NOTFOUND)

  # prefer the environment variable CXX
  IF($ENV{CXX} MATCHES ".+")
    GET_FILENAME_COMPONENT(CMAKE_CXX_COMPILER_INIT $ENV{CXX} PROGRAM PROGRAM_ARGS CMAKE_CXX_FLAGS_ENV_INIT)
    IF(CMAKE_CXX_FLAGS_ENV_INIT)
      SET(CMAKE_CXX_COMPILER_ARG1 "${CMAKE_CXX_FLAGS_ENV_INIT}" CACHE STRING "First argument to CXX compiler")
    ENDIF(CMAKE_CXX_FLAGS_ENV_INIT)
    IF(NOT EXISTS ${CMAKE_CXX_COMPILER_INIT})
      MESSAGE(FATAL_ERROR "Could not find compiler set in environment variable CXX:\n$ENV{CXX}.\n${CMAKE_CXX_COMPILER_INIT}")
    ENDIF(NOT EXISTS ${CMAKE_CXX_COMPILER_INIT})
  ENDIF($ENV{CXX} MATCHES ".+")

  # next prefer the generator specified compiler
  IF(CMAKE_GENERATOR_CXX)
    IF(NOT CMAKE_CXX_COMPILER_INIT)
      SET(CMAKE_CXX_COMPILER_INIT ${CMAKE_GENERATOR_CXX})
    ENDIF(NOT CMAKE_CXX_COMPILER_INIT)
  ENDIF(CMAKE_GENERATOR_CXX)

  # finally list compilers to try
  IF(CMAKE_CXX_COMPILER_INIT)
    SET(CMAKE_CXX_COMPILER_LIST ${CMAKE_CXX_COMPILER_INIT})
  ELSE(CMAKE_CXX_COMPILER_INIT)
    SET(CMAKE_CXX_COMPILER_LIST ${_CMAKE_TOOLCHAIN_PREFIX}c++ ${_CMAKE_TOOLCHAIN_PREFIX}g++ CC aCC cl bcc xlC)
  ENDIF(CMAKE_CXX_COMPILER_INIT)

  # Find the compiler.
  IF (_CMAKE_USER_C_COMPILER_PATH)
    FIND_PROGRAM(CMAKE_CXX_COMPILER NAMES ${CMAKE_CXX_COMPILER_LIST} PATHS ${_CMAKE_USER_C_COMPILER_PATH} DOC "C++ compiler" NO_DEFAULT_PATH)
  ENDIF (_CMAKE_USER_C_COMPILER_PATH)
  FIND_PROGRAM(CMAKE_CXX_COMPILER NAMES ${CMAKE_CXX_COMPILER_LIST} DOC "C++ compiler")
  
  IF(CMAKE_CXX_COMPILER_INIT AND NOT CMAKE_CXX_COMPILER)
    SET(CMAKE_CXX_COMPILER "${CMAKE_CXX_COMPILER_INIT}" CACHE FILEPATH "C++ compiler" FORCE)
  ENDIF(CMAKE_CXX_COMPILER_INIT AND NOT CMAKE_CXX_COMPILER)
ELSE(NOT CMAKE_CXX_COMPILER)

# we only get here if CMAKE_CXX_COMPILER was specified using -D or a pre-made CMakeCache.txt
# (e.g. via ctest) or set in CMAKE_TOOLCHAIN_FILE
#
# if CMAKE_CXX_COMPILER is a list of length 2, use the first item as 
# CMAKE_CXX_COMPILER and the 2nd one as CMAKE_CXX_COMPILER_ARG1

  LIST(LENGTH CMAKE_CXX_COMPILER _CMAKE_CXX_COMPILER_LIST_LENGTH)
  IF("${_CMAKE_CXX_COMPILER_LIST_LENGTH}" EQUAL 2)
    LIST(GET CMAKE_CXX_COMPILER 1 CMAKE_CXX_COMPILER_ARG1)
    LIST(GET CMAKE_CXX_COMPILER 0 CMAKE_CXX_COMPILER)
  ENDIF("${_CMAKE_CXX_COMPILER_LIST_LENGTH}" EQUAL 2)

# if a compiler was specified by the user but without path, 
# now try to find it with the full path
# if it is found, force it into the cache, 
# if not, don't overwrite the setting (which was given by the user) with "NOTFOUND"
# if the CXX compiler already had a path, reuse it for searching the C compiler
  GET_FILENAME_COMPONENT(_CMAKE_USER_CXX_COMPILER_PATH "${CMAKE_CXX_COMPILER}" PATH)
  IF(NOT _CMAKE_USER_CXX_COMPILER_PATH)
    FIND_PROGRAM(CMAKE_CXX_COMPILER_WITH_PATH NAMES ${CMAKE_CXX_COMPILER})
    MARK_AS_ADVANCED(CMAKE_CXX_COMPILER_WITH_PATH)
    IF(CMAKE_CXX_COMPILER_WITH_PATH)
      SET(CMAKE_CXX_COMPILER ${CMAKE_CXX_COMPILER_WITH_PATH} CACHE STRING "CXX compiler" FORCE)
    ENDIF(CMAKE_CXX_COMPILER_WITH_PATH)
  ENDIF(NOT _CMAKE_USER_CXX_COMPILER_PATH)
ENDIF(NOT CMAKE_CXX_COMPILER)
MARK_AS_ADVANCED(CMAKE_CXX_COMPILER)

IF (NOT _CMAKE_TOOLCHAIN_LOCATION)
  GET_FILENAME_COMPONENT(_CMAKE_TOOLCHAIN_LOCATION "${CMAKE_CXX_COMPILER}" PATH)
ENDIF (NOT _CMAKE_TOOLCHAIN_LOCATION)

# This block was used before the compiler was identified by building a
# source file.  Unless g++ crashes when building a small C++
# executable this should no longer be needed.
#
# The g++ that comes with BeOS 5 segfaults if you run "g++ -E"
#  ("gcc -E" is fine), which throws up a system dialog box that hangs cmake
#  until the user clicks "OK"...so for now, we just assume it's g++.
# IF(BEOS)
#   SET(CMAKE_COMPILER_IS_GNUCXX 1)
#   SET(CMAKE_COMPILER_IS_GNUCXX_RUN 1)
# ENDIF(BEOS)

# Build a small source file to identify the compiler.
IF(${CMAKE_GENERATOR} MATCHES "Visual Studio")
  SET(CMAKE_CXX_COMPILER_ID_RUN 1)
  SET(CMAKE_CXX_PLATFORM_ID "Windows")

  # TODO: Set the compiler id.  It is probably MSVC but
  # the user may be using an integrated Intel compiler.
  # SET(CMAKE_CXX_COMPILER_ID "MSVC")
ENDIF(${CMAKE_GENERATOR} MATCHES "Visual Studio")
IF(NOT CMAKE_CXX_COMPILER_ID_RUN)
  SET(CMAKE_CXX_COMPILER_ID_RUN 1)

  # Each entry in this list is a set of extra flags to try
  # adding to the compile line to see if it helps produce
  # a valid identification file.
  SET(CMAKE_CXX_COMPILER_ID_TEST_FLAGS
    # Try compiling to an object file only.
    "-c"
    )

  # Try to identify the compiler.
  SET(CMAKE_CXX_COMPILER_ID)
  FILE(READ ${CMAKE_ROOT}/Modules/CMakePlatformId.h.in
    CMAKE_CXX_COMPILER_ID_PLATFORM_CONTENT)
  INCLUDE(${CMAKE_ROOT}/Modules/CMakeDetermineCompilerId.cmake)
  CMAKE_DETERMINE_COMPILER_ID(CXX CXXFLAGS CMakeCXXCompilerId.cpp)

  # Set old compiler and platform id variables.
  IF("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    SET(CMAKE_COMPILER_IS_GNUCXX 1)
  ENDIF("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
  IF("${CMAKE_CXX_PLATFORM_ID}" MATCHES "MinGW")
    SET(CMAKE_COMPILER_IS_MINGW 1)
  ELSEIF("${CMAKE_CXX_PLATFORM_ID}" MATCHES "Cygwin")
    SET(CMAKE_COMPILER_IS_CYGWIN 1)
  ENDIF("${CMAKE_CXX_PLATFORM_ID}" MATCHES "MinGW")
ENDIF(NOT CMAKE_CXX_COMPILER_ID_RUN)

# if we have a g++ cross compiler, they have usually some prefix, like 
# e.g. powerpc-linux-g++, arm-elf-g++ or i586-mingw32msvc-g++
# the other tools of the toolchain usually have the same prefix
# NAME_WE cannot be used since then this test will fail for names lile
# "arm-unknown-nto-qnx6.3.0-gcc.exe", where BASENAME would be 
# "arm-unknown-nto-qnx6" instead of the correct "arm-unknown-nto-qnx6.3.0-"
IF (CMAKE_CROSSCOMPILING  
    AND "${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU"
    AND NOT _CMAKE_TOOLCHAIN_PREFIX)
  GET_FILENAME_COMPONENT(COMPILER_BASENAME "${CMAKE_CXX_COMPILER}" NAME)
  IF (COMPILER_BASENAME MATCHES "^(.+-)[gc]\\+\\+(\\.exe)?$")
    SET(_CMAKE_TOOLCHAIN_PREFIX ${CMAKE_MATCH_1})
  ENDIF (COMPILER_BASENAME MATCHES "^(.+-)[gc]\\+\\+(\\.exe)?$")

  # if "llvm-" is part of the prefix, remove it, since llvm doesn't have its own binutils
  # but uses the regular ar, objcopy, etc. (instead of llvm-objcopy etc.)
  IF ("${_CMAKE_TOOLCHAIN_PREFIX}" MATCHES "(.+-)?llvm-$")
    SET(_CMAKE_TOOLCHAIN_PREFIX ${CMAKE_MATCH_1})
  ENDIF ("${_CMAKE_TOOLCHAIN_PREFIX}" MATCHES "(.+-)?llvm-$")

ENDIF (CMAKE_CROSSCOMPILING  
    AND "${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU"
    AND NOT _CMAKE_TOOLCHAIN_PREFIX)

INCLUDE(CMakeFindBinUtils)
IF(MSVC_CXX_ARCHITECTURE_ID)
  SET(SET_MSVC_CXX_ARCHITECTURE_ID
    "SET(MSVC_CXX_ARCHITECTURE_ID ${MSVC_CXX_ARCHITECTURE_ID})")
ENDIF(MSVC_CXX_ARCHITECTURE_ID)
# configure all variables set in this file
CONFIGURE_FILE(${CMAKE_ROOT}/Modules/CMakeCXXCompiler.cmake.in
  ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeCXXCompiler.cmake
  @ONLY IMMEDIATE # IMMEDIATE must be here for compatibility mode <= 2.0
  )

SET(CMAKE_CXX_COMPILER_ENV_VAR "CXX")
