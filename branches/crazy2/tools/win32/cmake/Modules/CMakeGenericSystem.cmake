
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

SET(CMAKE_SHARED_LIBRARY_C_FLAGS "")            # -pic 
SET(CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS "-shared")       # -shared
SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")         # +s, flag for exe link to use shared lib
SET(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG "")       # -rpath
SET(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG_SEP "")   # : or empty
SET(CMAKE_INCLUDE_FLAG_C "-I")       # -I
SET(CMAKE_INCLUDE_FLAG_C_SEP "")     # , or empty
SET(CMAKE_LIBRARY_PATH_FLAG "-L")
SET(CMAKE_LIBRARY_PATH_TERMINATOR "")  # for the Digital Mars D compiler the link paths have to be terminated with a "/"
SET(CMAKE_LINK_LIBRARY_FLAG "-l")

SET(CMAKE_LINK_LIBRARY_SUFFIX "")
SET(CMAKE_STATIC_LIBRARY_PREFIX "lib")
SET(CMAKE_STATIC_LIBRARY_SUFFIX ".a")
SET(CMAKE_SHARED_LIBRARY_PREFIX "lib")          # lib
SET(CMAKE_SHARED_LIBRARY_SUFFIX ".so")          # .so
SET(CMAKE_EXECUTABLE_SUFFIX "")          # .exe
SET(CMAKE_DL_LIBS "dl")

SET(CMAKE_FIND_LIBRARY_PREFIXES "lib")
SET(CMAKE_FIND_LIBRARY_SUFFIXES ".so" ".a")

# basically all general purpose OSs support shared libs
SET_PROPERTY(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS TRUE)

SET (CMAKE_SKIP_RPATH "NO" CACHE BOOL
     "If set, runtime paths are not added when using shared libraries.")

SET(CMAKE_VERBOSE_MAKEFILE FALSE CACHE BOOL "If this value is on, makefiles will be generated without the .SILENT directive, and all commands will be echoed to the console during the make.  This is useful for debugging only. With Visual Studio IDE projects all commands are done without /nologo.") 

IF(CMAKE_GENERATOR MATCHES "Makefiles")
  SET(CMAKE_COLOR_MAKEFILE ON CACHE BOOL
    "Enable/Disable color output during build."
    )
  MARK_AS_ADVANCED(CMAKE_COLOR_MAKEFILE)
  IF(DEFINED CMAKE_RULE_MESSAGES)
    SET_PROPERTY(GLOBAL PROPERTY RULE_MESSAGES ${CMAKE_RULE_MESSAGES})
  ENDIF(DEFINED CMAKE_RULE_MESSAGES)
ENDIF(CMAKE_GENERATOR MATCHES "Makefiles")

# Set a variable to indicate whether the value of CMAKE_INSTALL_PREFIX
# was initialized by the block below.  This is useful for user
# projects to change the default prefix while still allowing the
# command line to override it.
IF(NOT DEFINED CMAKE_INSTALL_PREFIX)
  SET(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT 1)
ENDIF(NOT DEFINED CMAKE_INSTALL_PREFIX)

# Choose a default install prefix for this platform.
IF(CMAKE_HOST_UNIX)
  SET(CMAKE_INSTALL_PREFIX "/usr/local"
    CACHE PATH "Install path prefix, prepended onto install directories.")
ELSE(CMAKE_HOST_UNIX)
  IF("$ENV{ProgramFiles}" MATCHES "^$")
    IF("$ENV{SystemDrive}" MATCHES "^$")
      SET(CMAKE_GENERIC_PROGRAM_FILES "C:/Program Files")
    ELSE("$ENV{SystemDrive}" MATCHES "^$")
      SET(CMAKE_GENERIC_PROGRAM_FILES "$ENV{SystemDrive}/Program Files")
    ENDIF("$ENV{SystemDrive}" MATCHES "^$")
  ELSE("$ENV{ProgramFiles}" MATCHES "^$")
    SET(CMAKE_GENERIC_PROGRAM_FILES "$ENV{ProgramFiles}")
  ENDIF("$ENV{ProgramFiles}" MATCHES "^$")
  SET(CMAKE_INSTALL_PREFIX
    "${CMAKE_GENERIC_PROGRAM_FILES}/${PROJECT_NAME}"
    CACHE PATH "Install path prefix, prepended onto install directories.")
  SET(CMAKE_GENERIC_PROGRAM_FILES)

  # Make sure the prefix uses forward slashes.
  STRING(REGEX REPLACE "\\\\" "/"
    CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
ENDIF(CMAKE_HOST_UNIX)

MARK_AS_ADVANCED(
  CMAKE_SKIP_RPATH
  CMAKE_VERBOSE_MAKEFILE
)
