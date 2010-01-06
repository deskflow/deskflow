# - obsolete pkg-config module for CMake
#
# Defines the following macros:
#
# PKGCONFIG(package includedir libdir linkflags cflags)
#
# Calling PKGCONFIG will fill the desired information into the 4 given arguments,
# e.g. PKGCONFIG(libart-2.0 LIBART_INCLUDE_DIR LIBART_LINK_DIR LIBART_LINK_FLAGS LIBART_CFLAGS)
# if pkg-config was NOT found or the specified software package doesn't exist, the
# variable will be empty when the function returns, otherwise they will contain the respective information
#

#=============================================================================
# Copyright 2006-2009 Kitware, Inc.
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

FIND_PROGRAM(PKGCONFIG_EXECUTABLE NAMES pkg-config )

MACRO(PKGCONFIG _package _include_DIR _link_DIR _link_FLAGS _cflags)
  MESSAGE(STATUS
    "WARNING: you are using the obsolete 'PKGCONFIG' macro use FindPkgConfig")
# reset the variables at the beginning
  SET(${_include_DIR})
  SET(${_link_DIR})
  SET(${_link_FLAGS})
  SET(${_cflags})

  # if pkg-config has been found
  IF(PKGCONFIG_EXECUTABLE)

    EXEC_PROGRAM(${PKGCONFIG_EXECUTABLE} ARGS ${_package} --exists RETURN_VALUE _return_VALUE OUTPUT_VARIABLE _pkgconfigDevNull )

    # and if the package of interest also exists for pkg-config, then get the information
    IF(NOT _return_VALUE)

      EXEC_PROGRAM(${PKGCONFIG_EXECUTABLE} ARGS ${_package} --variable=includedir 
        OUTPUT_VARIABLE ${_include_DIR} )
      STRING(REGEX REPLACE "[\r\n]" " " ${_include_DIR} "${${_include_DIR}}")
    

      EXEC_PROGRAM(${PKGCONFIG_EXECUTABLE} ARGS ${_package} --variable=libdir 
        OUTPUT_VARIABLE ${_link_DIR} )
      STRING(REGEX REPLACE "[\r\n]" " " ${_link_DIR} "${${_link_DIR}}")

      EXEC_PROGRAM(${PKGCONFIG_EXECUTABLE} ARGS ${_package} --libs 
        OUTPUT_VARIABLE ${_link_FLAGS} )
      STRING(REGEX REPLACE "[\r\n]" " " ${_link_FLAGS} "${${_link_FLAGS}}")

      EXEC_PROGRAM(${PKGCONFIG_EXECUTABLE} ARGS ${_package} --cflags 
        OUTPUT_VARIABLE ${_cflags} )
      STRING(REGEX REPLACE "[\r\n]" " " ${_cflags} "${${_cflags}}")

    ELSE( NOT _return_VALUE)

      MESSAGE(STATUS "PKGCONFIG() indicates that ${_package} is not installed (install the package which contains ${_package}.pc if you want to support this feature)")

    ENDIF(NOT _return_VALUE)
    
  # if pkg-config has NOT been found, INFORM the user
  ELSE(PKGCONFIG_EXECUTABLE)
  
    MESSAGE(STATUS "WARNING: PKGCONFIG() indicates that the tool pkg-config has not been found on your system. You should install it.")

  ENDIF(PKGCONFIG_EXECUTABLE)

ENDMACRO(PKGCONFIG _include_DIR _link_DIR _link_FLAGS _cflags)

MARK_AS_ADVANCED(PKGCONFIG_EXECUTABLE)
