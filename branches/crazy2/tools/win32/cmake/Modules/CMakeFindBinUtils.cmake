
# search for additional tools required for C/C++ (and other languages ?)
#
# If the internal cmake variable _CMAKE_TOOLCHAIN_PREFIX is set, this is used 
# as prefix for the tools (e.g. arm-elf-gcc etc.)
# If the cmake variable _CMAKE_TOOLCHAIN_LOCATION is set, the compiler is
# searched only there. The other tools are at first searched there, then 
# also in the default locations.
#
# Sets the following variables: 
#   CMAKE_AR
#   CMAKE_RANLIB
#   CMAKE_LINKER
#   CMAKE_STRIP
#   CMAKE_INSTALL_NAME_TOOL

# on UNIX, cygwin and mingw

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

# if it's the MS C/CXX compiler, search for link
IF("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC" 
   OR "${CMAKE_C_COMPILER_ID}" MATCHES "MSVC"
   OR "${CMAKE_GENERATOR}" MATCHES "Visual Studio")

  FIND_PROGRAM(CMAKE_LINKER NAMES link HINTS ${_CMAKE_TOOLCHAIN_LOCATION})

  MARK_AS_ADVANCED(CMAKE_LINKER)

# in all other cases search for ar, ranlib, etc.
ELSE("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC" 
   OR "${CMAKE_C_COMPILER_ID}" MATCHES "MSVC"
   OR "${CMAKE_GENERATOR}" MATCHES "Visual Studio")

  FIND_PROGRAM(CMAKE_AR NAMES ${_CMAKE_TOOLCHAIN_PREFIX}ar HINTS ${_CMAKE_TOOLCHAIN_LOCATION})

  FIND_PROGRAM(CMAKE_RANLIB NAMES ${_CMAKE_TOOLCHAIN_PREFIX}ranlib HINTS ${_CMAKE_TOOLCHAIN_LOCATION})
  IF(NOT CMAKE_RANLIB)
    SET(CMAKE_RANLIB : CACHE INTERNAL "noop for ranlib")
  ENDIF(NOT CMAKE_RANLIB)

  FIND_PROGRAM(CMAKE_STRIP NAMES ${_CMAKE_TOOLCHAIN_PREFIX}strip HINTS ${_CMAKE_TOOLCHAIN_LOCATION})
  FIND_PROGRAM(CMAKE_LINKER NAMES ${_CMAKE_TOOLCHAIN_PREFIX}ld HINTS ${_CMAKE_TOOLCHAIN_LOCATION})
  FIND_PROGRAM(CMAKE_NM NAMES ${_CMAKE_TOOLCHAIN_PREFIX}nm HINTS ${_CMAKE_TOOLCHAIN_LOCATION})
  FIND_PROGRAM(CMAKE_OBJDUMP NAMES ${_CMAKE_TOOLCHAIN_PREFIX}objdump HINTS ${_CMAKE_TOOLCHAIN_LOCATION})
  FIND_PROGRAM(CMAKE_OBJCOPY NAMES ${_CMAKE_TOOLCHAIN_PREFIX}objcopy HINTS ${_CMAKE_TOOLCHAIN_LOCATION})

  MARK_AS_ADVANCED(CMAKE_AR CMAKE_RANLIB CMAKE_STRIP CMAKE_LINKER CMAKE_NM CMAKE_OBJDUMP CMAKE_OBJCOPY)

ENDIF("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC" 
   OR "${CMAKE_C_COMPILER_ID}" MATCHES "MSVC"
   OR "${CMAKE_GENERATOR}" MATCHES "Visual Studio")


# on Apple there really should be install_name_tool
IF(APPLE)
  FIND_PROGRAM(CMAKE_INSTALL_NAME_TOOL NAMES install_name_tool HINTS ${_CMAKE_TOOLCHAIN_LOCATION})

  IF(NOT CMAKE_INSTALL_NAME_TOOL)
    MESSAGE(FATAL_ERROR "Could not find install_name_tool, please check your installation.")
  ENDIF(NOT CMAKE_INSTALL_NAME_TOOL)

  MARK_AS_ADVANCED(CMAKE_INSTALL_NAME_TOOL)
ENDIF(APPLE)
