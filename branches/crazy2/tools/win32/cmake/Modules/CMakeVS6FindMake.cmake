
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

FIND_PROGRAM(CMAKE_MAKE_PROGRAM
  NAMES msdev
  PATHS
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\6.0\\Setup;VsCommonDir]/MSDev98/Bin
  "c:/Program Files/Microsoft Visual Studio/Common/MSDev98/Bin"
  "c:/Program Files/Microsoft Visual Studio/Common/MSDev98/Bin"
  "/Program Files/Microsoft Visual Studio/Common/MSDev98/Bin"
  )
MARK_AS_ADVANCED(CMAKE_MAKE_PROGRAM)
SET(MSVC60 1)
