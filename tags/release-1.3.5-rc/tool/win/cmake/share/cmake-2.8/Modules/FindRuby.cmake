# - Find Ruby
# This module finds if Ruby is installed and determines where the include files
# and libraries are. Ruby 1.8 and 1.9 are supported. The minimum required version 
# specified in the find_package() command is honored.
# It also determines what the name of the library is. This
# code sets the following variables:
#
#  RUBY_EXECUTABLE   = full path to the ruby binary
#  RUBY_INCLUDE_DIRS = include dirs to be used when using the ruby library
#  RUBY_LIBRARY      = full path to the ruby library
#  RUBY_VERSION      = the version of ruby which was found, e.g. "1.8.7"
#  RUBY_FOUND        = set to true if ruby ws found successfully
#
#  RUBY_INCLUDE_PATH = same as RUBY_INCLUDE_DIRS, only provided for compatibility reasons, don't use it

#=============================================================================
# Copyright 2004-2009 Kitware, Inc.
# Copyright 2008-2009 Alexander Neundorf <neundorf@kde.org>
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

#   RUBY_ARCHDIR=`$RUBY -r rbconfig -e 'printf("%s",Config::CONFIG@<:@"archdir"@:>@)'`
#   RUBY_SITEARCHDIR=`$RUBY -r rbconfig -e 'printf("%s",Config::CONFIG@<:@"sitearchdir"@:>@)'`
#   RUBY_SITEDIR=`$RUBY -r rbconfig -e 'printf("%s",Config::CONFIG@<:@"sitelibdir"@:>@)'`
#   RUBY_LIBDIR=`$RUBY -r rbconfig -e 'printf("%s",Config::CONFIG@<:@"libdir"@:>@)'`
#   RUBY_LIBRUBYARG=`$RUBY -r rbconfig -e 'printf("%s",Config::CONFIG@<:@"LIBRUBYARG_SHARED"@:>@)'`

# uncomment the following line to get debug output for this file
# SET(_RUBY_DEBUG_OUTPUT TRUE)

# Determine the list of possible names of the ruby executable depending
# on which version of ruby is required
SET(_RUBY_POSSIBLE_EXECUTABLE_NAMES ruby)

# if 1.9 is required, don't look for ruby18 and ruby1.8, default to version 1.8
IF(Ruby_FIND_VERSION_MAJOR  AND  Ruby_FIND_VERSION_MINOR)
   SET(Ruby_FIND_VERSION_SHORT_NODOT "${Ruby_FIND_VERSION_MAJOR}${RUBY_FIND_VERSION_MINOR}")
ELSE(Ruby_FIND_VERSION_MAJOR  AND  Ruby_FIND_VERSION_MINOR)
   SET(Ruby_FIND_VERSION_SHORT_NODOT "18")
ENDIF(Ruby_FIND_VERSION_MAJOR  AND  Ruby_FIND_VERSION_MINOR)

SET(_RUBY_POSSIBLE_EXECUTABLE_NAMES ${_RUBY_POSSIBLE_EXECUTABLE_NAMES} ruby1.9 ruby19)

# if we want a version below 1.9, also look for ruby 1.8
IF("${Ruby_FIND_VERSION_SHORT_NODOT}" VERSION_LESS "19")
   SET(_RUBY_POSSIBLE_EXECUTABLE_NAMES ${_RUBY_POSSIBLE_EXECUTABLE_NAMES} ruby1.8 ruby18)
ENDIF("${Ruby_FIND_VERSION_SHORT_NODOT}" VERSION_LESS "19")

FIND_PROGRAM(RUBY_EXECUTABLE NAMES ${_RUBY_POSSIBLE_EXECUTABLE_NAMES})


IF(RUBY_EXECUTABLE  AND NOT  RUBY_MAJOR_VERSION)
  # query the ruby version
   EXECUTE_PROCESS(COMMAND ${RUBY_EXECUTABLE} -r rbconfig -e "print Config::CONFIG['MAJOR']"
      OUTPUT_VARIABLE RUBY_VERSION_MAJOR)

   EXECUTE_PROCESS(COMMAND ${RUBY_EXECUTABLE} -r rbconfig -e "print Config::CONFIG['MINOR']"
      OUTPUT_VARIABLE RUBY_VERSION_MINOR)

   EXECUTE_PROCESS(COMMAND ${RUBY_EXECUTABLE} -r rbconfig -e "print Config::CONFIG['TEENY']"
      OUTPUT_VARIABLE RUBY_VERSION_PATCH)

   # query the different directories
   EXECUTE_PROCESS(COMMAND ${RUBY_EXECUTABLE} -r rbconfig -e "print Config::CONFIG['archdir']"
      OUTPUT_VARIABLE RUBY_ARCH_DIR)

   EXECUTE_PROCESS(COMMAND ${RUBY_EXECUTABLE} -r rbconfig -e "print Config::CONFIG['arch']"
      OUTPUT_VARIABLE RUBY_ARCH)

   EXECUTE_PROCESS(COMMAND ${RUBY_EXECUTABLE} -r rbconfig -e "print Config::CONFIG['rubyhdrdir']"
      OUTPUT_VARIABLE RUBY_HDR_DIR)

   EXECUTE_PROCESS(COMMAND ${RUBY_EXECUTABLE} -r rbconfig -e "print Config::CONFIG['libdir']"
      OUTPUT_VARIABLE RUBY_POSSIBLE_LIB_DIR)

   EXECUTE_PROCESS(COMMAND ${RUBY_EXECUTABLE} -r rbconfig -e "print Config::CONFIG['rubylibdir']"
      OUTPUT_VARIABLE RUBY_RUBY_LIB_DIR)

   # site_ruby
   EXECUTE_PROCESS(COMMAND ${RUBY_EXECUTABLE} -r rbconfig -e "print Config::CONFIG['sitearchdir']"
      OUTPUT_VARIABLE RUBY_SITEARCH_DIR)

   EXECUTE_PROCESS(COMMAND ${RUBY_EXECUTABLE} -r rbconfig -e "print Config::CONFIG['sitelibdir']"
      OUTPUT_VARIABLE RUBY_SITELIB_DIR)

   # vendor_ruby available ?
   EXECUTE_PROCESS(COMMAND ${RUBY_EXECUTABLE} -r vendor-specific -e "print 'true'"
      OUTPUT_VARIABLE RUBY_HAS_VENDOR_RUBY  ERROR_QUIET)

   IF(RUBY_HAS_VENDOR_RUBY)
      EXECUTE_PROCESS(COMMAND ${RUBY_EXECUTABLE} -r rbconfig -e "print Config::CONFIG['vendorlibdir']"
         OUTPUT_VARIABLE RUBY_VENDORLIB_DIR)

      EXECUTE_PROCESS(COMMAND ${RUBY_EXECUTABLE} -r rbconfig -e "print Config::CONFIG['vendorarchdir']"
         OUTPUT_VARIABLE RUBY_VENDORARCH_DIR)
   ENDIF(RUBY_HAS_VENDOR_RUBY)

   # save the results in the cache so we don't have to run ruby the next time again
   SET(RUBY_VERSION_MAJOR    ${RUBY_VERSION_MAJOR}    CACHE PATH "The Ruby major version" FORCE)
   SET(RUBY_VERSION_MINOR    ${RUBY_VERSION_MINOR}    CACHE PATH "The Ruby minor version" FORCE)
   SET(RUBY_VERSION_PATCH    ${RUBY_VERSION_PATCH}    CACHE PATH "The Ruby patch version" FORCE)
   SET(RUBY_ARCH_DIR         ${RUBY_ARCH_DIR}         CACHE PATH "The Ruby arch dir" FORCE)
   SET(RUBY_HDR_DIR          ${RUBY_HDR_DIR}          CACHE PATH "The Ruby header dir (1.9)" FORCE)
   SET(RUBY_POSSIBLE_LIB_DIR ${RUBY_POSSIBLE_LIB_DIR} CACHE PATH "The Ruby lib dir" FORCE)
   SET(RUBY_RUBY_LIB_DIR     ${RUBY_RUBY_LIB_DIR}     CACHE PATH "The Ruby ruby-lib dir" FORCE)
   SET(RUBY_SITEARCH_DIR     ${RUBY_SITEARCH_DIR}     CACHE PATH "The Ruby site arch dir" FORCE)
   SET(RUBY_SITELIB_DIR      ${RUBY_SITELIB_DIR}      CACHE PATH "The Ruby site lib dir" FORCE)
   SET(RUBY_HAS_VENDOR_RUBY  ${RUBY_HAS_VENDOR_RUBY}  CACHE BOOL "Vendor Ruby is available" FORCE)
   SET(RUBY_VENDORARCH_DIR   ${RUBY_VENDORARCH_DIR}   CACHE PATH "The Ruby vendor arch dir" FORCE)
   SET(RUBY_VENDORLIB_DIR    ${RUBY_VENDORLIB_DIR}    CACHE PATH "The Ruby vendor lib dir" FORCE)

   MARK_AS_ADVANCED(
     RUBY_ARCH_DIR
     RUBY_ARCH
     RUBY_HDR_DIR
     RUBY_POSSIBLE_LIB_DIR
     RUBY_RUBY_LIB_DIR
     RUBY_SITEARCH_DIR
     RUBY_SITELIB_DIR
     RUBY_HAS_VENDOR_RUBY
     RUBY_VENDORARCH_DIR
     RUBY_VENDORLIB_DIR
     RUBY_VERSION_MAJOR
     RUBY_VERSION_MINOR
     RUBY_VERSION_PATCH
     )
ENDIF(RUBY_EXECUTABLE  AND NOT  RUBY_MAJOR_VERSION)

# In case RUBY_EXECUTABLE could not be executed (e.g. cross compiling) 
# try to detect which version we found. This is not too good.
IF(NOT RUBY_VERSION_MAJOR)
   # by default assume 1.8.0
   SET(RUBY_VERSION_MAJOR 1)
   SET(RUBY_VERSION_MINOR 8)
   SET(RUBY_VERSION_PATCH 0)
   # check whether we found 1.9.x
   IF(${RUBY_EXECUTABLE} MATCHES "ruby1.?9"  OR  RUBY_HDR_DIR)
      SET(RUBY_VERSION_MAJOR 1)
      SET(RUBY_VERSION_MINOR 9)
   ENDIF(${RUBY_EXECUTABLE} MATCHES "ruby1.?9"  OR  RUBY_HDR_DIR)
ENDIF(NOT RUBY_VERSION_MAJOR)


SET(RUBY_VERSION "${RUBY_VERSION_MAJOR}.${RUBY_VERSION_MINOR}.${RUBY_VERSION_PATCH}")
SET(_RUBY_VERSION_SHORT "${RUBY_VERSION_MAJOR}.${RUBY_VERSION_MINOR}")
SET(_RUBY_VERSION_SHORT_NODOT "${RUBY_VERSION_MAJOR}${RUBY_VERSION_MINOR}")

# Now we know which version we found
IF(Ruby_FIND_VERSION)
   IF(${RUBY_VERSION}  VERSION_LESS  ${Ruby_FIND_VERSION})
      # force running ruby the next time again
      SET(RUBY_VERSION_MAJOR    ""    CACHE PATH "The Ruby major version" FORCE)
      IF(Ruby_FIND_REQUIRED)
         MESSAGE(FATAL_ERROR "Ruby version ${Ruby_FIND_VERSION} required, but only version ${RUBY_VERSION} found.")
      ELSE(Ruby_FIND_REQUIRED)
         IF(NOT Ruby_FIND_QUIETLY)
            MESSAGE(STATUS "Ruby version ${Ruby_FIND_VERSION} required, but only version ${RUBY_VERSION} found.")
         ENDIF(NOT Ruby_FIND_QUIETLY)
         RETURN()
      ENDIF(Ruby_FIND_REQUIRED)
   ENDIF(${RUBY_VERSION}  VERSION_LESS  ${Ruby_FIND_VERSION})
ENDIF(Ruby_FIND_VERSION)

FIND_PATH(RUBY_INCLUDE_DIR
   NAMES ruby.h
   HINTS
   ${RUBY_HDR_DIR}
   ${RUBY_ARCH_DIR}
   /usr/lib/ruby/${_RUBY_VERSION_SHORT}/i586-linux-gnu/ )

SET(RUBY_INCLUDE_DIRS ${RUBY_INCLUDE_DIR} )

# if ruby > 1.8 is required or if ruby > 1.8 was found, search for the config.h dir
IF( ${Ruby_FIND_VERSION_SHORT_NODOT} GREATER 18  OR  ${_RUBY_VERSION_SHORT_NODOT} GREATER 18  OR  RUBY_HDR_DIR)
   message(STATUS "lookign for config.h")
   FIND_PATH(RUBY_CONFIG_INCLUDE_DIR
     NAMES ruby/config.h  config.h
     HINTS 
     ${RUBY_HDR_DIR}/${RUBY_ARCH}
     ${RUBY_ARCH_DIR} 
     )

   SET(RUBY_INCLUDE_DIRS ${RUBY_INCLUDE_DIRS} ${RUBY_CONFIG_INCLUDE_DIR} )
ENDIF( ${Ruby_FIND_VERSION_SHORT_NODOT} GREATER 18  OR  ${_RUBY_VERSION_SHORT_NODOT} GREATER 18  OR  RUBY_HDR_DIR)


# Determine the list of possible names for the ruby library
SET(_RUBY_POSSIBLE_LIB_NAMES ruby ruby-static ruby${_RUBY_VERSION_SHORT})

IF(WIN32)
   SET( _RUBY_MSVC_RUNTIME "" )
   IF( MSVC60 )
     SET( _RUBY_MSVC_RUNTIME "60" )
   ENDIF( MSVC60 )
   IF( MSVC70 )
     SET( _RUBY_MSVC_RUNTIME "70" )
   ENDIF( MSVC70 )
   IF( MSVC71 )
     SET( _RUBY_MSVC_RUNTIME "71" )
   ENDIF( MSVC71 )
   IF( MSVC80 )
     SET( _RUBY_MSVC_RUNTIME "80" )
   ENDIF( MSVC80 )
   IF( MSVC90 )
     SET( _RUBY_MSVC_RUNTIME "90" )
   ENDIF( MSVC90 )

   LIST(APPEND _RUBY_POSSIBLE_LIB_NAMES
               "msvcr${_RUBY_MSVC_RUNTIME}-ruby${RUBY_NODOT_VERSION}"
               "msvcr${_RUBY_MSVC_RUNTIME}-ruby${RUBY_NODOT_VERSION}-static" 
               "msvcrt-ruby${RUBY_NODOT_VERSION}"
               "msvcrt-ruby${RUBY_NODOT_VERSION}-static" )
ENDIF(WIN32)

FIND_LIBRARY(RUBY_LIBRARY NAMES ${_RUBY_POSSIBLE_LIB_NAMES} HINTS ${RUBY_POSSIBLE_LIB_DIR} )

INCLUDE(FindPackageHandleStandardArgs)
SET(_RUBY_REQUIRED_VARS RUBY_EXECUTABLE RUBY_INCLUDE_DIR RUBY_LIBRARY)
IF(_RUBY_VERSION_SHORT_NODOT GREATER 18)
   LIST(APPEND _RUBY_REQUIRED_VARS RUBY_CONFIG_INCLUDE_DIR)
ENDIF(_RUBY_VERSION_SHORT_NODOT GREATER 18)

IF(_RUBY_DEBUG_OUTPUT)
   MESSAGE(STATUS "--------FindRuby.cmake debug------------")
   MESSAGE(STATUS "_RUBY_POSSIBLE_EXECUTABLE_NAMES: ${_RUBY_POSSIBLE_EXECUTABLE_NAMES}")
   MESSAGE(STATUS "_RUBY_POSSIBLE_LIB_NAMES: ${_RUBY_POSSIBLE_LIB_NAMES}")
   MESSAGE(STATUS "RUBY_ARCH_DIR: ${RUBY_ARCH_DIR}")
   MESSAGE(STATUS "RUBY_HDR_DIR: ${RUBY_HDR_DIR}")
   MESSAGE(STATUS "RUBY_POSSIBLE_LIB_DIR: ${RUBY_POSSIBLE_LIB_DIR}")
   MESSAGE(STATUS "Found RUBY_VERSION: \"${RUBY_VERSION}\" , short: \"${_RUBY_VERSION_SHORT}\", nodot: \"${_RUBY_VERSION_SHORT_NODOT}\"")
   MESSAGE(STATUS "_RUBY_REQUIRED_VARS: ${_RUBY_REQUIRED_VARS}")
   MESSAGE(STATUS "--------------------")
ENDIF(_RUBY_DEBUG_OUTPUT)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(Ruby  DEFAULT_MSG  ${_RUBY_REQUIRED_VARS})

MARK_AS_ADVANCED(
  RUBY_EXECUTABLE
  RUBY_LIBRARY
  RUBY_INCLUDE_DIR
  RUBY_CONFIG_INCLUDE_DIR
  )

# Set some variables for compatibility with previous version of this file
SET(RUBY_POSSIBLE_LIB_PATH ${RUBY_POSSIBLE_LIB_DIR})
SET(RUBY_RUBY_LIB_PATH ${RUBY_RUBY_LIB_DIR})
SET(RUBY_INCLUDE_PATH ${RUBY_INCLUDE_DIRS})
