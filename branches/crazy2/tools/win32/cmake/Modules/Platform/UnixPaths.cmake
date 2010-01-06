
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

# Block multiple inclusion because "CMakeCInformation.cmake" includes
# "Platform/${CMAKE_SYSTEM_NAME}" even though the generic module
# "CMakeSystemSpecificInformation.cmake" already included it.
# The extra inclusion is a work-around documented next to the include()
# call, so this can be removed when the work-around is removed.
IF(__UNIX_PATHS_INCLUDED)
  RETURN()
ENDIF()
SET(__UNIX_PATHS_INCLUDED 1)

SET(UNIX 1)

# also add the install directory of the running cmake to the search directories
# CMAKE_ROOT is CMAKE_INSTALL_PREFIX/share/cmake, so we need to go two levels up
GET_FILENAME_COMPONENT(_CMAKE_INSTALL_DIR "${CMAKE_ROOT}" PATH)
GET_FILENAME_COMPONENT(_CMAKE_INSTALL_DIR "${_CMAKE_INSTALL_DIR}" PATH)

# List common installation prefixes.  These will be used for all
# search types.
LIST(APPEND CMAKE_SYSTEM_PREFIX_PATH
  # Standard
  /usr/local / /usr

  # CMake install location
  "${_CMAKE_INSTALL_DIR}"

  # Project install destination.
  "${CMAKE_INSTALL_PREFIX}"
  )

# List common include file locations not under the common prefixes.
LIST(APPEND CMAKE_SYSTEM_INCLUDE_PATH
  # Windows API on Cygwin
  /usr/include/w32api

  # X11
  /usr/X11R6/include /usr/include/X11

  # Other
  /opt/local/include /usr/pkg/include
  /opt/csw/include /opt/include  
  /usr/openwin/include
  )

LIST(APPEND CMAKE_SYSTEM_LIBRARY_PATH
  # Windows API on Cygwin
  /usr/lib/w32api

  # X11
  /usr/X11R6/lib /usr/lib/X11

  # Other
  /opt/local/lib /usr/pkg/lib
  /opt/csw/lib /opt/lib 
  /usr/openwin/lib
  )

LIST(APPEND CMAKE_SYSTEM_PROGRAM_PATH
  /usr/pkg/bin
  )

LIST(APPEND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES
  /lib /usr/lib /usr/lib32 /usr/lib64
  )

LIST(APPEND CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES
  /usr/include
  )
LIST(APPEND CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES
  /usr/include
  )

# Enable use of lib64 search path variants by default.
SET_PROPERTY(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS TRUE)
