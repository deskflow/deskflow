
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
IF(__WINDOWS_PATHS_INCLUDED)
  RETURN()
ENDIF()
SET(__WINDOWS_PATHS_INCLUDED 1)

# Add the program-files folder(s) to the list of installation
# prefixes.
#
# Windows 64-bit Binary:
#   ENV{ProgramFiles(x86)} = [C:\Program Files (x86)]
#   ENV{ProgramFiles} = [C:\Program Files]
#   ENV{ProgramW6432} = <not set>
# (executed from cygwin):
#   ENV{ProgramFiles(x86)} = <not set>
#   ENV{ProgramFiles} = [C:\Program Files]
#   ENV{ProgramW6432} = <not set>
#
# Windows 32-bit Binary:
#   ENV{ProgramFiles(x86)} = [C:\Program Files (x86)]
#   ENV{ProgramFiles} = [C:\Program Files (x86)]
#   ENV{ProgramW6432} = [C:\Program Files]
# (executed from cygwin):
#   ENV{ProgramFiles(x86)} = <not set>
#   ENV{ProgramFiles} = [C:\Program Files (x86)]
#   ENV{ProgramW6432} = [C:\Program Files]
IF(DEFINED "ENV{ProgramW6432}")
  # 32-bit binary on 64-bit windows.
  # The 64-bit program files are in ProgramW6432.
  LIST(APPEND CMAKE_SYSTEM_PREFIX_PATH "$ENV{ProgramW6432}")

  # The 32-bit program files are in ProgramFiles.
  IF(DEFINED "ENV{ProgramFiles}")
    LIST(APPEND CMAKE_SYSTEM_PREFIX_PATH "$ENV{ProgramFiles}")
  ENDIF()
ELSE()
  # 64-bit binary, or 32-bit binary on 32-bit windows.
  IF(DEFINED "ENV{ProgramFiles}")
    LIST(APPEND CMAKE_SYSTEM_PREFIX_PATH "$ENV{ProgramFiles}")
  ENDIF()
  IF(DEFINED "ENV{ProgramFiles(x86)}")
    # 64-bit binary.  32-bit program files are in ProgramFiles(x86).
    LIST(APPEND CMAKE_SYSTEM_PREFIX_PATH "$ENV{ProgramFiles(x86)}")
  ELSEIF(DEFINED "ENV{SystemDrive}")
    # Guess the 32-bit program files location.
    IF(EXISTS "$ENV{SystemDrive}/Program Files (x86)")
      LIST(APPEND CMAKE_SYSTEM_PREFIX_PATH
        "$ENV{SystemDrive}/Program Files (x86)")
    ENDIF()
  ENDIF()
ENDIF()

# Add the CMake install location.
GET_FILENAME_COMPONENT(_CMAKE_INSTALL_DIR "${CMAKE_ROOT}" PATH)
GET_FILENAME_COMPONENT(_CMAKE_INSTALL_DIR "${_CMAKE_INSTALL_DIR}" PATH)
LIST(APPEND CMAKE_SYSTEM_PREFIX_PATH "${_CMAKE_INSTALL_DIR}")

# Add other locations.
LIST(APPEND CMAKE_SYSTEM_PREFIX_PATH
  # Project install destination.
  "${CMAKE_INSTALL_PREFIX}"

  # MinGW (useful when cross compiling from linux with CMAKE_FIND_ROOT_PATH set)
  /
  )

LIST(APPEND CMAKE_SYSTEM_INCLUDE_PATH
  )

# mingw can also link against dlls which can also be in /bin, so list this too
LIST(APPEND CMAKE_SYSTEM_LIBRARY_PATH
  "${CMAKE_INSTALL_PREFIX}/bin"
  "${_CMAKE_INSTALL_DIR}/bin"
  /bin
  )

LIST(APPEND CMAKE_SYSTEM_PROGRAM_PATH
  )
