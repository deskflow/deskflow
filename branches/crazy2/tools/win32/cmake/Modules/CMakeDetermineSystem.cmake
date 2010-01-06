
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

# This module is used by the Makefile generator to determin the following variables:
# CMAKE_SYSTEM_NAME - on unix this is uname -s, for windows it is Windows
# CMAKE_SYSTEM_VERSION - on unix this is uname -r, for windows it is empty
# CMAKE_SYSTEM - ${CMAKE_SYSTEM}-${CMAKE_SYSTEM_VERSION}, for windows: ${CMAKE_SYSTEM}
#
#  Expected uname -s output:
#
# AIX                           AIX  
# BSD/OS                        BSD/OS  
# FreeBSD                       FreeBSD  
# HP-UX                         HP-UX  
# IRIX                          IRIX  
# Linux                         Linux  
# GNU/kFreeBSD                  GNU/kFreeBSD
# NetBSD                        NetBSD  
# OpenBSD                       OpenBSD  
# OFS/1 (Digital Unix)          OSF1  
# SCO OpenServer 5              SCO_SV  
# SCO UnixWare 7                UnixWare  
# SCO UnixWare (pre release 7)  UNIX_SV  
# SCO XENIX                     Xenix  
# Solaris                       SunOS  
# SunOS                         SunOS  
# Tru64                         Tru64  
# Ultrix                        ULTRIX  
# cygwin                        CYGWIN_NT-5.1
# MacOSX                        Darwin


# find out on which system cmake runs
IF(CMAKE_HOST_UNIX)
  FIND_PROGRAM(CMAKE_UNAME uname /bin /usr/bin /usr/local/bin )
  IF(CMAKE_UNAME)
    EXEC_PROGRAM(uname ARGS -s OUTPUT_VARIABLE CMAKE_HOST_SYSTEM_NAME)
    EXEC_PROGRAM(uname ARGS -r OUTPUT_VARIABLE CMAKE_HOST_SYSTEM_VERSION)
    IF(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
      EXEC_PROGRAM(uname ARGS -m OUTPUT_VARIABLE CMAKE_HOST_SYSTEM_PROCESSOR
        RETURN_VALUE val)
    ELSE(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
      EXEC_PROGRAM(uname ARGS -p OUTPUT_VARIABLE CMAKE_HOST_SYSTEM_PROCESSOR
        RETURN_VALUE val)
      IF("${val}" GREATER 0)
        EXEC_PROGRAM(uname ARGS -m OUTPUT_VARIABLE CMAKE_HOST_SYSTEM_PROCESSOR
          RETURN_VALUE val)
      ENDIF("${val}" GREATER 0)
    ENDIF(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
    # check the return of the last uname -m or -p 
    IF("${val}" GREATER 0)
        SET(CMAKE_HOST_SYSTEM_PROCESSOR "unknown")
    ENDIF("${val}" GREATER 0)
    SET(CMAKE_UNAME ${CMAKE_UNAME} CACHE INTERNAL "uname command")
    # processor may have double quote in the name, and that needs to be removed
    STRING(REGEX REPLACE "\"" "" CMAKE_HOST_SYSTEM_PROCESSOR "${CMAKE_HOST_SYSTEM_PROCESSOR}")
    STRING(REGEX REPLACE "/" "_" CMAKE_HOST_SYSTEM_PROCESSOR "${CMAKE_HOST_SYSTEM_PROCESSOR}")
  ENDIF(CMAKE_UNAME)
ELSE(CMAKE_HOST_UNIX)
  IF(CMAKE_HOST_WIN32)
    SET (CMAKE_HOST_SYSTEM_NAME "Windows")
    SET (CMAKE_HOST_SYSTEM_PROCESSOR "$ENV{PROCESSOR_ARCHITECTURE}")
  ENDIF(CMAKE_HOST_WIN32)
ENDIF(CMAKE_HOST_UNIX)

# if a toolchain file is used, the user wants to cross compile.
# in this case read the toolchain file and keep the CMAKE_HOST_SYSTEM_*
# variables around so they can be used in CMakeLists.txt. 
# In all other cases, the host and target platform are the same.
IF(CMAKE_TOOLCHAIN_FILE)
  # at first try to load it as path relative to the directory from which cmake has been run
  INCLUDE("${CMAKE_BINARY_DIR}/${CMAKE_TOOLCHAIN_FILE}" OPTIONAL RESULT_VARIABLE _INCLUDED_TOOLCHAIN_FILE)
  IF(NOT _INCLUDED_TOOLCHAIN_FILE)
     # if the file isn't found there, check the default locations
     INCLUDE("${CMAKE_TOOLCHAIN_FILE}" OPTIONAL RESULT_VARIABLE _INCLUDED_TOOLCHAIN_FILE)
  ENDIF(NOT _INCLUDED_TOOLCHAIN_FILE)

  IF(_INCLUDED_TOOLCHAIN_FILE)
    SET(CMAKE_TOOLCHAIN_FILE "${_INCLUDED_TOOLCHAIN_FILE}" CACHE FILEPATH "The CMake toolchain file" FORCE)
  ELSE(_INCLUDED_TOOLCHAIN_FILE)
    MESSAGE(FATAL_ERROR "Could not find toolchain file: ${CMAKE_TOOLCHAIN_FILE}") 
    SET(CMAKE_TOOLCHAIN_FILE "NOTFOUND" CACHE FILEPATH "The CMake toolchain file" FORCE)
  ENDIF(_INCLUDED_TOOLCHAIN_FILE)
ENDIF(CMAKE_TOOLCHAIN_FILE)


# if CMAKE_SYSTEM_NAME is here already set, either it comes from a toolchain file
# or it was set via -DCMAKE_SYSTEM_NAME=...
# if that's the case, assume we are crosscompiling
IF(CMAKE_SYSTEM_NAME)
  IF(NOT DEFINED CMAKE_CROSSCOMPILING)
    SET(CMAKE_CROSSCOMPILING TRUE)
  ENDIF(NOT DEFINED CMAKE_CROSSCOMPILING)
  SET(PRESET_CMAKE_SYSTEM_NAME TRUE)
ELSE(CMAKE_SYSTEM_NAME)
  SET(CMAKE_SYSTEM_NAME      "${CMAKE_HOST_SYSTEM_NAME}")
  SET(CMAKE_SYSTEM_VERSION   "${CMAKE_HOST_SYSTEM_VERSION}")
  SET(CMAKE_SYSTEM_PROCESSOR "${CMAKE_HOST_SYSTEM_PROCESSOR}")
  SET(CMAKE_CROSSCOMPILING FALSE)
  SET(PRESET_CMAKE_SYSTEM_NAME FALSE)
ENDIF(CMAKE_SYSTEM_NAME)


MACRO(ADJUST_CMAKE_SYSTEM_VARIABLES _PREFIX)
  IF(NOT ${_PREFIX}_NAME)
    SET(${_PREFIX}_NAME "UnknownOS")
  ENDIF(NOT ${_PREFIX}_NAME)

  # fix for BSD/OS , remove the /
  IF(${_PREFIX}_NAME MATCHES BSD.OS)
    SET(${_PREFIX}_NAME BSDOS)
  ENDIF(${_PREFIX}_NAME MATCHES BSD.OS)

  # fix for GNU/kFreeBSD, remove the GNU/
  IF(${_PREFIX}_NAME MATCHES kFreeBSD)
    SET(${_PREFIX}_NAME kFreeBSD)
  ENDIF(${_PREFIX}_NAME MATCHES kFreeBSD)

  # fix for CYGWIN which has windows version in it 
  IF(${_PREFIX}_NAME MATCHES CYGWIN)
    SET(${_PREFIX}_NAME CYGWIN)
  ENDIF(${_PREFIX}_NAME MATCHES CYGWIN)

  # set CMAKE_SYSTEM to the CMAKE_SYSTEM_NAME
  SET(${_PREFIX}  ${${_PREFIX}_NAME})
  # if there is a CMAKE_SYSTEM_VERSION then add a -${CMAKE_SYSTEM_VERSION}
  IF(${_PREFIX}_VERSION)
    SET(${_PREFIX} ${${_PREFIX}}-${${_PREFIX}_VERSION})
  ENDIF(${_PREFIX}_VERSION)

ENDMACRO(ADJUST_CMAKE_SYSTEM_VARIABLES _PREFIX)

ADJUST_CMAKE_SYSTEM_VARIABLES(CMAKE_SYSTEM)
ADJUST_CMAKE_SYSTEM_VARIABLES(CMAKE_HOST_SYSTEM)

# this file is also executed from cpack, then we don't need to generate these files 
# in this case there is no CMAKE_BINARY_DIR
IF(CMAKE_BINARY_DIR)
  # write entry to the log file
  IF(PRESET_CMAKE_SYSTEM_NAME)
    FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log 
                "The target system is: ${CMAKE_SYSTEM_NAME} - ${CMAKE_SYSTEM_VERSION} - ${CMAKE_SYSTEM_PROCESSOR}\n")
    FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log 
                "The host system is: ${CMAKE_HOST_SYSTEM_NAME} - ${CMAKE_HOST_SYSTEM_VERSION} - ${CMAKE_HOST_SYSTEM_PROCESSOR}\n")
  ELSE(PRESET_CMAKE_SYSTEM_NAME)
    FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log 
                "The system is: ${CMAKE_SYSTEM_NAME} - ${CMAKE_SYSTEM_VERSION} - ${CMAKE_SYSTEM_PROCESSOR}\n")
  ENDIF(PRESET_CMAKE_SYSTEM_NAME)

  # if a toolchain file is used, it needs to be included in the configured file,
  # so settings done there are also available if they don't go in the cache and in TRY_COMPILE()
  SET(INCLUDE_CMAKE_TOOLCHAIN_FILE_IF_REQUIRED)
  IF(DEFINED CMAKE_TOOLCHAIN_FILE)
    SET(INCLUDE_CMAKE_TOOLCHAIN_FILE_IF_REQUIRED "INCLUDE(\"${CMAKE_TOOLCHAIN_FILE}\")")
  ENDIF(DEFINED CMAKE_TOOLCHAIN_FILE)

  # configure variables set in this file for fast reload, the template file is defined at the top of this file
  CONFIGURE_FILE(${CMAKE_ROOT}/Modules/CMakeSystem.cmake.in
                ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeSystem.cmake 
                IMMEDIATE @ONLY)

ENDIF(CMAKE_BINARY_DIR)
