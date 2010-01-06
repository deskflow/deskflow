
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

# determine the compiler to use for ASM programs

IF(NOT CMAKE_ASM${ASM_DIALECT}_COMPILER)
  # prefer the environment variable ASM
  IF($ENV{ASM${ASM_DIALECT}} MATCHES ".+")
    SET(CMAKE_ASM${ASM_DIALECT}_COMPILER_INIT "$ENV{ASM${ASM_DIALECT}}")
  ENDIF($ENV{ASM${ASM_DIALECT}} MATCHES ".+")

  # finally list compilers to try
  IF(CMAKE_ASM${ASM_DIALECT}_COMPILER_INIT)
    SET(CMAKE_ASM${ASM_DIALECT}_COMPILER_LIST ${CMAKE_ASM${ASM_DIALECT}_COMPILER_INIT})
  ELSE(CMAKE_ASM${ASM_DIALECT}_COMPILER_INIT)
    SET(CMAKE_ASM${ASM_DIALECT}_COMPILER_LIST ${_CMAKE_TOOLCHAIN_PREFIX}as ${_CMAKE_TOOLCHAIN_PREFIX}gas)
  ENDIF(CMAKE_ASM${ASM_DIALECT}_COMPILER_INIT)

  # Find the compiler.
  IF (_CMAKE_USER_CXX_COMPILER_PATH OR _CMAKE_USER_C_COMPILER_PATH)
    FIND_PROGRAM(CMAKE_ASM${ASM_DIALECT}_COMPILER NAMES ${CMAKE_ASM${ASM_DIALECT}_COMPILER_LIST} PATHS ${_CMAKE_USER_C_COMPILER_PATH} ${_CMAKE_USER_CXX_COMPILER_PATH} DOC "Assembler" NO_DEFAULT_PATH)
  ENDIF (_CMAKE_USER_CXX_COMPILER_PATH OR _CMAKE_USER_C_COMPILER_PATH)
  FIND_PROGRAM(CMAKE_ASM${ASM_DIALECT}_COMPILER NAMES ${CMAKE_ASM${ASM_DIALECT}_COMPILER_LIST} PATHS ${_CMAKE_TOOLCHAIN_LOCATION} DOC "Assembler")

ELSE(NOT CMAKE_ASM${ASM_DIALECT}_COMPILER)

  # we only get here if CMAKE_ASM${ASM_DIALECT}_COMPILER was specified using -D or a pre-made CMakeCache.txt
  # (e.g. via ctest) or set in CMAKE_TOOLCHAIN_FILE
  #
  # if a compiler was specified by the user but without path, 
  # now try to find it with the full path
  # if it is found, force it into the cache, 
  # if not, don't overwrite the setting (which was given by the user) with "NOTFOUND"
  GET_FILENAME_COMPONENT(_CMAKE_USER_ASM${ASM_DIALECT}_COMPILER_PATH "${CMAKE_ASM${ASM_DIALECT}_COMPILER}" PATH)
  IF(NOT _CMAKE_USER_ASM${ASM_DIALECT}_COMPILER_PATH)
    FIND_PROGRAM(CMAKE_ASM${ASM_DIALECT}_COMPILER_WITH_PATH NAMES ${CMAKE_ASM${ASM_DIALECT}_COMPILER})
    MARK_AS_ADVANCED(CMAKE_ASM${ASM_DIALECT}_COMPILER_WITH_PATH)
    IF(CMAKE_ASM${ASM_DIALECT}_COMPILER_WITH_PATH)
      SET(CMAKE_ASM${ASM_DIALECT}_COMPILER ${CMAKE_ASM${ASM_DIALECT}_COMPILER_WITH_PATH} CACHE FILEPATH "Assembler" FORCE)
    ENDIF(CMAKE_ASM${ASM_DIALECT}_COMPILER_WITH_PATH)
  ENDIF(NOT _CMAKE_USER_ASM${ASM_DIALECT}_COMPILER_PATH)
ENDIF(NOT CMAKE_ASM${ASM_DIALECT}_COMPILER)
MARK_AS_ADVANCED(CMAKE_ASM${ASM_DIALECT}_COMPILER)

IF (NOT _CMAKE_TOOLCHAIN_LOCATION)
  GET_FILENAME_COMPONENT(_CMAKE_TOOLCHAIN_LOCATION "${CMAKE_ASM${ASM_DIALECT}_COMPILER}" PATH)
ENDIF (NOT _CMAKE_TOOLCHAIN_LOCATION)

# If we have a gas/as cross compiler, they have usually some prefix, like 
# e.g. powerpc-linux-gas, arm-elf-gas or i586-mingw32msvc-gas .
# The other tools of the toolchain usually have the same prefix
# NAME_WE cannot be used since then this test will fail for names lile
# "arm-unknown-nto-qnx6.3.0-gas.exe", where BASENAME would be 
# "arm-unknown-nto-qnx6" instead of the correct "arm-unknown-nto-qnx6.3.0-"
IF (NOT _CMAKE_TOOLCHAIN_PREFIX)
  GET_FILENAME_COMPONENT(COMPILER_BASENAME "${CMAKE_ASM${ASM_DIALECT}_COMPILER}" NAME)
  IF (COMPILER_BASENAME MATCHES "^(.+-)g?as(\\.exe)?$")
    STRING(REGEX REPLACE "^(.+-)g?as(\\.exe)?$"  "\\1" _CMAKE_TOOLCHAIN_PREFIX "${COMPILER_BASENAME}")
  ENDIF (COMPILER_BASENAME MATCHES "^(.+-)g?as(\\.exe)?$")
ENDIF (NOT _CMAKE_TOOLCHAIN_PREFIX)

INCLUDE(CMakeFindBinUtils)

SET(CMAKE_ASM${ASM_DIALECT}_COMPILER_ENV_VAR "ASM${ASM_DIALECT}")

IF(CMAKE_ASM${ASM_DIALECT}_COMPILER)
  MESSAGE(STATUS "Found assembler: ${CMAKE_ASM${ASM_DIALECT}_COMPILER}")
ELSE(CMAKE_ASM${ASM_DIALECT}_COMPILER)
  MESSAGE(STATUS "Didn't find assembler")
ENDIF(CMAKE_ASM${ASM_DIALECT}_COMPILER)


SET(_CMAKE_ASM_COMPILER "${CMAKE_ASM${ASM_DIALECT}_COMPILER}")
SET(_CMAKE_ASM_COMPILER_ARG1 "${CMAKE_ASM${ASM_DIALECT}_COMPILER_ARG1}")
SET(_CMAKE_ASM_COMPILER_ENV_VAR "${CMAKE_ASM${ASM_DIALECT}_COMPILER_ENV_VAR}")

# configure variables set in this file for fast reload later on
CONFIGURE_FILE(${CMAKE_ROOT}/Modules/CMakeASMCompiler.cmake.in 
  ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeASM${ASM_DIALECT}Compiler.cmake IMMEDIATE @ONLY)

SET(_CMAKE_ASM_COMPILER)
SET(_CMAKE_ASM_COMPILER_ARG1)
SET(_CMAKE_ASM_COMPILER_ENV_VAR)
